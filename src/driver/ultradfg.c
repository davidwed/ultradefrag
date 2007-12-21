/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *  Entry point and requests processing.
 */

#include "driver.h"

#if !defined(__GNUC__)
#pragma code_seg("INIT") /* begin of section INIT */
#endif
/* DriverEntry - driver initialization
 */
INIT_FUNCTION NTSTATUS NTAPI DriverEntry(IN PDRIVER_OBJECT DriverObject,
						IN PUNICODE_STRING RegistryPath)
{
	UDEFRAG_DEVICE_EXTENSION *dx;

	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT fdo;
	UNICODE_STRING devName;
	UNICODE_STRING symLinkName;
	ULONG mj,mn;

	/* Export entry points */
	DriverObject->DriverUnload = UnloadRoutine;
	DriverObject->MajorFunction[IRP_MJ_CREATE]= Create_File_IRPprocessing;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = Close_HandleIRPprocessing;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = Write_IRPhandler;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]=
											  DeviceControlRoutine;

#ifdef NT4_DBG
	OpenLog(); /* We should call them before any DebugPrint() calls !!! */
#endif
	RtlInitUnicodeString(&devName,device_name);
	status = IoCreateDevice(DriverObject,
				sizeof(UDEFRAG_DEVICE_EXTENSION),
				&devName, /* can be NULL */
				FILE_DEVICE_UNKNOWN,
				0,
				FALSE, /* without exclusive access */
				&fdo);
	if(status != STATUS_SUCCESS)
	{
		DebugPrint("=Ultradfg= IoCreateDevice %x\n",(UINT)status);
		return status;
	}

	/* get device_extension pointer */
	dx = (PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);
	dx->fdo = fdo;  /* save backward pointer */
	fdo->Flags |= DO_DIRECT_IO;
	/* data initialization */
	PrepareDataFields(dx);
	KeInitializeEvent(&dx->sync_event,NotificationEvent,FALSE);
	KeInitializeEvent(&dx->stop_event,NotificationEvent,FALSE);
	KeInitializeSpinLock(&dx->spin_lock);
	dx->BitMap = NULL; dx->FileMap = NULL; dx->tmp_buf = NULL;
	dx->in_filter.buffer = dx->ex_filter.buffer = NULL;
	dx->in_filter.offsets = dx->ex_filter.offsets = NULL;
	dx->report_type.format = ASCII_FORMAT;
	dx->report_type.type = HTML_REPORT;
	dbg_level = 0;
	new_cluster_map = NULL; /* console and native tools don't needs them */
	map_size = 0;
	/* allocate memory */
#ifndef NT4_TARGET
	dx->FileMap = AllocatePool(NonPagedPool,FILEMAPSIZE * sizeof(ULONGLONG));
	dx->BitMap = AllocatePool(NonPagedPool,BITMAPSIZE * sizeof(UCHAR));
	if(!dx->FileMap || !dx->BitMap) goto no_mem;
#endif
	dx->tmp_buf = AllocatePool(NonPagedPool,32768 + 5);
	if(!dx->tmp_buf)
	{
#ifndef NT4_TARGET
no_mem:
#endif
		DbgPrintNoMem();
#ifndef NT4_TARGET
		ExFreePoolSafe(dx->FileMap);
		ExFreePoolSafe(dx->BitMap);
#endif
		ExFreePoolSafe(dx->tmp_buf);
		IoDeleteDevice(fdo);
		return STATUS_NO_MEMORY;
	}

	DebugPrint("=Ultradfg= FDO %p, DevExt=%p\n",fdo,dx);

	/* For NT-drivers it can be L"\\??\\ultradfg" */
	RtlInitUnicodeString(&symLinkName,link_name);
	status = IoCreateSymbolicLink(&symLinkName,&devName);
	if(!NT_SUCCESS(status))
	{
		DebugPrint("=Ultradfg= IoCreateSymbolicLink %x\n",(UINT)status);
#ifndef NT4_TARGET
		ExFreePoolSafe(dx->FileMap);
		ExFreePoolSafe(dx->BitMap);
#endif
		ExFreePoolSafe(dx->tmp_buf);
		IoDeleteDevice(fdo);
		return status;
	}

	/* Get Windows version */
	if(PsGetVersion(&mj,&mn,NULL,NULL))
		DebugPrint("=Ultradfg= NT %u.%u checked.\n",mj,mn);
	else
		DebugPrint("=Ultradfg= NT %u.%u free.\n",mj,mn);
	dx->xp_compatible = (((mj << 6) + mn) > ((5 << 6) + 0));
	////////////dx->xp_compatible = FALSE;
	DebugPrint("=Ultradfg= DriverEntry successfully completed\n");
	return STATUS_SUCCESS;
}
#if !defined(__GNUC__)
#pragma code_seg() /* end INIT section */
#endif

/* CompleteIrp: Set IoStatus and complete IRP processing
 */ 
NTSTATUS CompleteIrp(PIRP Irp,NTSTATUS status,ULONG info)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return status;
}

/*
 * Write request: analyse / defrag / compact.
 */
NTSTATUS NTAPI Write_IRPhandler(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	ULTRADFG_COMMAND cmd;
	KIRQL oldIrql;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

	UDEFRAG_DEVICE_EXTENSION *dx = \
			(PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);
	PVOID addr;
	NTSTATUS request_status = STATUS_SUCCESS;
	BOOLEAN request_is_successful;

	/* If previous request isn't complete, return STATUS_DEVICE_BUSY. */
	/* Because new request change global variables. */
	KeAcquireSpinLock(&dx->spin_lock,&oldIrql);
	if(KeSetEvent(&dx->sync_event,IO_NO_INCREMENT,FALSE))
	{
		if(dbg_level > 0)
			DebugPrint("-Ultradfg- is busy!\n");
		KeReleaseSpinLock(&dx->spin_lock,oldIrql);
		return CompleteIrp(Irp,STATUS_DEVICE_BUSY,0);
	}
	KeReleaseSpinLock(&dx->spin_lock,oldIrql);

	DebugPrint("-Ultradfg- in Write_IRPhandler\n");
	
	if(IrpStack->Parameters.Write.Length != sizeof(ULTRADFG_COMMAND))
		goto invalid_request;
#ifdef NT4_TARGET
	addr = MmGetSystemAddressForMdl(Irp->MdlAddress);
#else
	addr = MmGetSystemAddressForMdlSafe(Irp->MdlAddress,NormalPagePriority);
#endif
	if(!addr) goto invalid_request;
	RtlCopyMemory(&cmd,addr,sizeof(ULTRADFG_COMMAND));

	DebugPrint("-Ultradfg- Command = %c%c\n",cmd.command,cmd.letter);
	DebugPrint("-Ultradfg- Sizelimit = %I64u\n",cmd.sizelimit);

	cmd.letter = (UCHAR)toupper((int)cmd.letter); /* important for nt 4.0 and w2k - bug #1771887 solution */
	if(cmd.letter < 'A' || cmd.letter > 'Z')
	{
		request_status = STATUS_INVALID_PARAMETER;
		goto request_failure;
	}

	dx->compact_flag = (cmd.command == 'c' || cmd.command == 'C') ? TRUE : FALSE;
	dx->sizelimit = cmd.sizelimit;

	request_is_successful = TRUE;
	switch(cmd.command)
	{
	case 'a':
	case 'A':
		dx->letter = cmd.letter;
		request_status = Analyse(dx);
		if(NT_SUCCESS(request_status)) break;
		if(KeReadStateEvent(&dx->stop_event) != 0x1)
			request_is_successful = FALSE;
		break;
	case 'c':
	case 'C':
	case 'd':
	case 'D':
		if(dx->status == STATUS_BEFORE_PROCESSING || \
			dx->letter != cmd.letter)
		{
			dx->letter = cmd.letter;
			request_status = Analyse(dx);
			if(!NT_SUCCESS(request_status))
			{
				if(KeReadStateEvent(&dx->stop_event) != 0x1)
					request_is_successful = FALSE;
				break;
			}
			if(KeReadStateEvent(&dx->stop_event) == 0x1)
				break;
		}
		Defragment(dx);
		break;
	//default:
	//	goto invalid_request;
	}
	/* request completed */
	KeClearEvent(&dx->sync_event);
	if(request_is_successful)
		return CompleteIrp(Irp,STATUS_SUCCESS,sizeof(ULTRADFG_COMMAND));
request_failure:
	dx->status = STATUS_BEFORE_PROCESSING;
	return CompleteIrp(Irp,request_status,0);
invalid_request:
	KeClearEvent(&dx->sync_event);
	return CompleteIrp(Irp,STATUS_INVALID_PARAMETER,0);
}

void DbgPrintNoMem()
{
	DebugPrint("-Ultradfg- No Enough Memory!\n");
}

/* CreateFile() request processing. */
NTSTATUS NTAPI Create_File_IRPprocessing(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	UDEFRAG_DEVICE_EXTENSION *dx = \
			(PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);

	DebugPrint("-Ultradfg- Create File\n");
	dx->status = STATUS_BEFORE_PROCESSING;
	return CompleteIrp(Irp,STATUS_SUCCESS,0);
}

/* CloseHandle() request processing. */
NTSTATUS NTAPI Close_HandleIRPprocessing(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	UDEFRAG_DEVICE_EXTENSION *dx = \
			(PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);

	DebugPrint("-Ultradfg- In Close handler\n"); 
	FreeAllBuffers(dx);
	return CompleteIrp(Irp,STATUS_SUCCESS,0);
}

void UpdateInformation(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFRAGMENTED pf;

	pf = dx->fragmfileslist;
	while(pf)
	{
		ApplyFilter(dx,pf->pfn);
		pf = pf->next_ptr;
	}
}

void UpdateFilter(PFILTER pf,short *buffer,int length)
{
	POFFSET poffset;
	int i;

	if(pf->buffer)
	{
		ExFreePool((void *)pf->buffer);
		pf->buffer = NULL;
		DestroyList((PLIST *)pf->offsets);
	}
	if(length > sizeof(short))
	{
		pf->offsets = NULL;
		pf->buffer = AllocatePool(NonPagedPool,length);
		if(pf->buffer)
		{
			buffer[(length >> 1) - 1] = 0;
			_wcslwr(buffer);

			poffset = (POFFSET)InsertFirstItem((PLIST *)&pf->offsets,sizeof(OFFSET));
			if(!poffset) return;
			poffset->offset = 0;
			for(i = 0; i < (length >> 1) - 1; i++)
			{
				if(buffer[i] == 0x003b)
				{
					buffer[i] = 0;
					poffset = (POFFSET)InsertFirstItem((PLIST *)&pf->offsets,sizeof(OFFSET));
					if(!poffset) break;
					poffset->offset = i + 1;
				}
			}
			RtlCopyMemory(pf->buffer,buffer,length);
		}
	}
}

void DestroyFilter(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ExFreePoolSafe(dx->in_filter.buffer);
	ExFreePoolSafe(dx->ex_filter.buffer);
	DestroyList((PLIST *)&dx->in_filter.offsets);
	DestroyList((PLIST *)&dx->ex_filter.offsets);
}

/* DeviceControlRoutine: IRP_MJ_DEVICE_CONTROL request handler. */
NTSTATUS NTAPI DeviceControlRoutine(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	UDEFRAG_DEVICE_EXTENSION *dx;
	PIO_STACK_LOCATION IrpStack;
	ULONG IoControlCode;
	PSTATISTIC pst;
	char *map;
	short *filter;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG BytesTxd = 0;
	ULONGLONG bpb; /* internal blocks per one external block */
	ULONGLONG i, j;
	ULONGLONG x_part[NUM_OF_SPACE_STATES];
	ULONGLONG maximum;
	UCHAR index, k;
	REPORT_TYPE *rt;
	UCHAR *user_mode_buffer; /* for nt 4.0 */
	//PVOID in_buf, out_buf;
	ULONG in_len, out_len;

	dx = (PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);
	IrpStack = IoGetCurrentIrpStackLocation(Irp);
	IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
	in_len = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	out_len = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	/*
	DebugPrint("-Ultradfg- In DeviceControlRoutine (fdo= %p)\n",fdo);
	DebugPrint("-Ultradfg- DeviceIoControl: IOCTL %X\n", (UINT)ControlCode );
	*/

	switch(IoControlCode)
	{
		case IOCTL_GET_STATISTIC:
		{
			DebugPrint("-Ultradfg- IOCTL_GET_STATISTIC\n");
			pst = (PSTATISTIC)Irp->AssociatedIrp.SystemBuffer;
			if(out_len != sizeof(STATISTIC) || !pst)
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			pst->filecounter = dx->filecounter;
			pst->dircounter = dx->dircounter;
			pst->compressedcounter = dx->compressedcounter;
			pst->fragmcounter = dx->fragmcounter;
			pst->fragmfilecounter = dx->fragmfilecounter;
			pst->free_space = dx->free_space;
			pst->total_space = dx->total_space;
			pst->mft_size = dx->mft_size;
			pst->processed_clusters = dx->processed_clusters;
			pst->bytes_per_cluster = dx->bytes_per_cluster;
			pst->current_operation = dx->current_operation;
			pst->clusters_to_move_initial = dx->clusters_to_move_initial;
			pst->clusters_to_move = dx->clusters_to_move;
			pst->clusters_to_compact_initial = dx->clusters_to_compact_initial;
			pst->clusters_to_compact = dx->clusters_to_compact;
			BytesTxd = sizeof(STATISTIC);
			break;
		}
		case IOCTL_GET_CLUSTER_MAP:
		{
			DebugPrint("-Ultradfg- IOCTL_GET_CLUSTER_MAP\n");
			if(!new_cluster_map)
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			map = (char *)Irp->AssociatedIrp.SystemBuffer;
			if(!out_len || !map)
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			bpb = map_size / out_len;
			if(!bpb || (map_size % out_len))
			{
				status = STATUS_INVALID_PARAMETER; /* buffer has invalid size */
				break;
			}
			else
			{
				for(i = 0; i < out_len; i++)
				{
					memset((void *)x_part,0,NUM_OF_SPACE_STATES * sizeof(ULONGLONG));
					for(j = 0; j < bpb; j++)
						for(k = 0; k < NUM_OF_SPACE_STATES; k++)
							x_part[k] += new_cluster_map[i * bpb + j][k];
					maximum = x_part[0];
					index = 0;
					for(k = 1; k < NUM_OF_SPACE_STATES; k++)
					{
						if(x_part[k] >= maximum) /* >= is very important: mft and free */
						{
							maximum = x_part[k];
							index = k;
						}
					}
					map[i] = index;
				}
			}
			BytesTxd = out_len;
			break;
		}
		case IOCTL_ULTRADEFRAG_STOP:
		{
			DebugPrint("-Ultradfg- IOCTL_ULTRADEFRAG_STOP\n");
			KeSetEvent(&dx->stop_event,IO_NO_INCREMENT,FALSE);
			break;
		}
		case IOCTL_SET_INCLUDE_FILTER:
		case IOCTL_SET_EXCLUDE_FILTER:
		{
			DebugPrint("-Ultradfg- IOCTL_SET_XXX_FILTER\n");
			filter = (short *)Irp->AssociatedIrp.SystemBuffer;
			if(filter)
			{
				if(IoControlCode == IOCTL_SET_INCLUDE_FILTER)
				{
					DebugPrint("-Ultradfg- Include: %ws\n",filter);
					UpdateFilter(&dx->in_filter,filter,in_len);
				}
				else
				{
					DebugPrint("-Ultradfg- Exclude: %ws\n",filter);
					UpdateFilter(&dx->ex_filter,filter,in_len);
				}
				UpdateInformation(dx);
			}
			BytesTxd = in_len;
			break;
		}
		case IOCTL_SET_DBGPRINT_LEVEL:
		{
			DebugPrint("-Ultradfg- IOCTL_SET_DBGPRINT_LEVEL\n");
			if(in_len != sizeof(ULONG))
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			dbg_level = *((ULONG *)Irp->AssociatedIrp.SystemBuffer);
			DebugPrint("-Ultradfg- dbg_level=%u\n",dbg_level);
			BytesTxd = in_len;
			break;
		}
		case IOCTL_SET_REPORT_TYPE:
		{
			DebugPrint("-Ultradfg- IOCTL_SET_REPORT_TYPE\n");
			if(in_len != sizeof(REPORT_TYPE))
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			rt = (REPORT_TYPE *)Irp->AssociatedIrp.SystemBuffer;
			dx->report_type.format = rt->format;
			dx->report_type.type = rt->type;
			BytesTxd = in_len;
			break;
		}
		case IOCTL_SET_USER_MODE_BUFFER:
		{
			DebugPrint("-Ultradfg- IOCTL_SET_USER_MODE_BUFFER\n");
			DebugPrint("-Ultradfg- Address = %p\n",
				IrpStack->Parameters.DeviceIoControl.Type3InputBuffer);
			user_mode_buffer = IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
		#ifdef NT4_TARGET
			if(user_mode_buffer)
			{
				dx->BitMap = user_mode_buffer;
				dx->FileMap = (ULONGLONG *)(user_mode_buffer + BITMAPSIZE * sizeof(UCHAR));
			}
		#endif
			break;
		}
		case IOCTL_SET_CLUSTER_MAP_SIZE:
		{
			DebugPrint("-Ultradfg- IOCTL_SET_CLUSTER_MAP_SIZE\n");
			if(new_cluster_map)
			{
				/* map size changing don't supported yet */
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			if(in_len != sizeof(ULONG))
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			map_size = *((ULONG *)Irp->AssociatedIrp.SystemBuffer);
			DebugPrint("-Ultradfg- Map size = %u\n",map_size);
			if(map_size)
			{
				new_cluster_map = AllocatePool(NonPagedPool,NUM_OF_SPACE_STATES * map_size * sizeof(ULONGLONG));
				if(!new_cluster_map)
				{
					DbgPrintNoMem();
					map_size = 0;
					status = STATUS_NO_MEMORY;
					break;
				}
			}
			BytesTxd = in_len;
			break;
		}
		/* Invalid request */
		default: status = STATUS_INVALID_DEVICE_REQUEST;
	}
	return CompleteIrp(Irp,status,BytesTxd);
}

/* UnloadRoutine: unload the driver and free allocated memory.
 * Can be placed in paged memory.
 */
#if !defined(__GNUC__)
#pragma code_seg("PAGE")
#endif

PAGED_OUT_FUNCTION VOID NTAPI UnloadRoutine(IN PDRIVER_OBJECT pDriverObject)
{
	UDEFRAG_DEVICE_EXTENSION *dx;
	UNICODE_STRING symLinkName;

	dx = (PUDEFRAG_DEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;
	RtlInitUnicodeString(&symLinkName,link_name);

	DebugPrint("-Ultradfg- In Unload Routine\n");
	DebugPrint("-Ultradfg- Deleted device: pointer to FDO = %p\n",dx->fdo);
	DebugPrint("-Ultradfg- Deleted symlink = %ws\n", symLinkName.Buffer);

	FreeAllBuffers(dx);
#ifndef NT4_TARGET
	ExFreePoolSafe(dx->FileMap);
	ExFreePoolSafe(dx->BitMap);
#endif
	ExFreePoolSafe(dx->tmp_buf);
	ExFreePoolSafe(new_cluster_map);
	DestroyFilter(dx);
#ifdef NT4_DBG
	CloseLog();
#endif
	/* Delete symbolic link and FDO: */
	IoDeleteSymbolicLink(&symLinkName);
	IoDeleteDevice(dx->fdo);
}
#if !defined(__GNUC__)
#pragma code_seg() /* end PAGE section */
#endif
