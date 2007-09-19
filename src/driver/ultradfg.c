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

#pragma code_seg("INIT") /* begin of section INIT */
/* DriverEntry - driver initialization
 */
NTSTATUS NTAPI DriverEntry(IN PDRIVER_OBJECT DriverObject,
						IN PUNICODE_STRING RegistryPath)
{
	PEXAMPLE_DEVICE_EXTENSION dx;

	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT fdo;
	UNICODE_STRING devName;
	UNICODE_STRING symLinkName;
	ULONG mj,mn;

	/* Export entry points */
	DriverObject->DriverUnload = UnloadRoutine;
	DriverObject->MajorFunction[IRP_MJ_CREATE]= Create_File_IRPprocessing;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = Close_HandleIRPprocessing;
	//DriverObject->MajorFunction[IRP_MJ_READ] = Read_IRPhandler;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = Write_IRPhandler;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]=
											  DeviceControlRoutine;

	RtlInitUnicodeString(&devName,device_name);
	status = IoCreateDevice(DriverObject,
				sizeof(EXAMPLE_DEVICE_EXTENSION),
				&devName, /* can be NULL */
				FILE_DEVICE_UNKNOWN,
				0,
				FALSE, /* without exclusive access */
				&fdo);
	if(status != STATUS_SUCCESS)
	{
		DebugPrint("=Ultradfg= IoCreateDevice %x\n",status);
		return status;
	}

	/* get device_extension pointer */
	dx = (PEXAMPLE_DEVICE_EXTENSION)(fdo->DeviceExtension);
	dx->fdo = fdo;  /* save backward pointer */
	fdo->Flags |= DO_DIRECT_IO;
	/* data initialization */
	PrepareDataFields(dx);
	KeInitializeEvent(&(dx->sync_event),NotificationEvent,FALSE);
	KeInitializeEvent(&(dx->stop_event),NotificationEvent,FALSE);
	KeInitializeEvent(&(dx->unload_event),NotificationEvent,FALSE);
	KeInitializeEvent(&(dx->wait_event),NotificationEvent,FALSE);
	KeInitializeEvent(&(dx->lock_map_event),SynchronizationEvent,TRUE);
	KeInitializeSpinLock(&(dx->spin_lock));
	dx->BitMap = NULL; dx->FileMap = NULL; dx->tmp_buf = NULL;
	dx->user_mode_buffer = NULL;
	dx->in_filter.buffer = dx->ex_filter.buffer = NULL;
	dx->in_filter.offsets = dx->ex_filter.offsets = NULL;
	dx->report_type.format = ASCII_FORMAT;
	dx->report_type.type = HTML_REPORT;
	dbg_level = 0;
	dx->command = 0;

	DebugPrint("=Ultradfg= FDO %X, DevExt=%X\n",fdo,dx);

	status = PsCreateSystemThread(&dx->hThread,0,NULL,NULL,NULL,WorkSystemThread,(PVOID)dx);
	if(!NT_SUCCESS(status))
	{
		DebugPrint("=Ultradfg= PsCreateSystemThread %x\n",status);
		IoDeleteDevice(fdo);
		return status;
	}

	/* For NT-drivers it can be L"\\??\\ultradfg" */
	RtlInitUnicodeString(&symLinkName,link_name);
	dx->ustrSymLinkName = symLinkName;
	
	status = IoCreateSymbolicLink(&symLinkName,&devName);
	if(!NT_SUCCESS(status))
	{
		DebugPrint("=Ultradfg= IoCreateSymbolicLink %x\n",status);
		KeSetEvent(&(dx->unload_event),IO_NO_INCREMENT,FALSE);
		WaitForThreadComplete(dx);
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
#pragma code_seg() /* end INIT section */

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
 * Read request return status of our job processing.
 */
/*
NTSTATUS NTAPI Read_IRPhandler(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	ULONG *pBuffer;
	ULONG length;
	NTSTATUS internal_status = \
		((PEXAMPLE_DEVICE_EXTENSION)(fdo->DeviceExtension))->status;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

	#if DBG
	DbgPrint("-Ultradfg- in Read_IRPhandler\n");
	DbgPrint("-Ultradfg- Status = %u\n",internal_status);
	#endif

	length = IrpStack->Parameters.Read.Length;
	if(length != sizeof(ULONG))
		return CompleteIrp(Irp,STATUS_INVALID_PARAMETER,0);
	pBuffer = (ULONG *)MmGetSystemAddressForMdlSafe(Irp->MdlAddress,NormalPagePriority);

	*pBuffer = internal_status;
	return CompleteIrp(Irp,STATUS_SUCCESS,sizeof(ULONG)); 
}
*/
__inline BOOLEAN validate_letter(char letter)
{
	return ((letter >= 'A' && letter <= 'Z') || (letter >= 'a' && letter <= 'z'));
}

VOID NTAPI WorkSystemThread(PVOID pContext)
{
	PEXAMPLE_DEVICE_EXTENSION dx = \
			(PEXAMPLE_DEVICE_EXTENSION)pContext;
	LARGE_INTEGER interval;
	NTSTATUS Status;

	interval.QuadPart = WAIT_CMD_INTERVAL;
	do
	{
		if(dx->command)
		{
			dx->request_is_successful = TRUE;
			switch(dx->command)
			{
			case 'a':
			case 'A':
				dx->letter = dx->new_letter;
				if(Analyse(dx)) break;
				if(KeReadStateEvent(&(dx->stop_event)) != 0x1)
					dx->request_is_successful = FALSE;
				break;
			case 'c':
			case 'C':
			case 'd':
			case 'D':
				if(dx->status == STATUS_BEFORE_PROCESSING || \
					dx->letter != dx->new_letter)
				{
					dx->letter = dx->new_letter;
					if(!Analyse(dx))
					{
						if(KeReadStateEvent(&(dx->stop_event)) != 0x1)
							dx->request_is_successful = FALSE;
						break;
					}
					if(KeReadStateEvent(&(dx->stop_event)) == 0x1)
						break;
				}
				Defragment(dx);
				break;
			//default:
			//	goto invalid_request;
			}
			/* request completed */
			KeClearEvent(&(dx->sync_event));
			dx->command = 0;
		}
		Status = KeWaitForSingleObject(&(dx->unload_event),
				Executive,KernelMode,FALSE,&interval);
	} while (Status == STATUS_TIMEOUT);

	DebugPrint("-Ultradfg- System thread said good bye ...\n");
	PsTerminateSystemThread(0);
}

VOID WaitForThreadComplete(PEXAMPLE_DEVICE_EXTENSION dx)
{
	PKTHREAD pThreadObject;
	NTSTATUS Status;

	Status = ObReferenceObjectByHandle(dx->hThread,0,NULL,
		KernelMode,(PVOID *)&pThreadObject,NULL);
	if(NT_SUCCESS(Status))
	{
		KeWaitForSingleObject(pThreadObject,Executive,
			KernelMode,FALSE,NULL);
		ObDereferenceObject(pThreadObject);
	}
	ZwClose(dx->hThread);
}

/*
 * Write request: analyse / defrag / compact.
 */
NTSTATUS NTAPI Write_IRPhandler(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	ULTRADFG_COMMAND cmd;
	KIRQL oldIrql;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

	PEXAMPLE_DEVICE_EXTENSION dx = \
			(PEXAMPLE_DEVICE_EXTENSION)(fdo->DeviceExtension);
	PVOID addr;
	LARGE_INTEGER interval;

	/* If previous request isn't complete, return STATUS_DEVICE_BUSY. */
	/* Because new request change global variables. */
	KeAcquireSpinLock(&(dx->spin_lock),&oldIrql);
	if(KeSetEvent(&(dx->sync_event),IO_NO_INCREMENT,FALSE))
	{
		if(dbg_level > 0)
			DebugPrint("-Ultradfg- is busy!\n");
		KeReleaseSpinLock(&(dx->spin_lock),oldIrql);
		return CompleteIrp(Irp,STATUS_DEVICE_BUSY,0);
	}
	KeReleaseSpinLock(&(dx->spin_lock),oldIrql);

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

	if(!validate_letter(cmd.letter)) goto invalid_request;

	dx->compact_flag = (cmd.command == 'c' || cmd.command == 'C') ? TRUE : FALSE;
	dx->sizelimit = cmd.sizelimit;

#ifdef NT4_TARGET
	//////////cmd.mode = __KernelMode;
	///DebugPrint("hDbgLog address %x\n",&hDbgLog);
#endif

	if(cmd.mode == __KernelMode)
	{ /* native app defrag system files */
		dx->command = cmd.command;
		dx->new_letter = cmd.letter;

		/* wait for request completion */
		interval.QuadPart = WAIT_CMD_INTERVAL;
		while(KeReadStateEvent(&(dx->sync_event)))
			KeWaitForSingleObject(&(dx->wait_event),
					Executive,KernelMode,FALSE,&interval);
	}
	else
	{ /* gui or console must have permission to access encrypted files */
		dx->request_is_successful = TRUE;
		switch(cmd.command)
		{
		case 'a':
		case 'A':
			dx->letter = cmd.letter;
			if(Analyse(dx)) break;
			if(KeReadStateEvent(&(dx->stop_event)) != 0x1)
				dx->request_is_successful = FALSE;
			break;
		case 'c':
		case 'C':
		case 'd':
		case 'D':
			if(dx->status == STATUS_BEFORE_PROCESSING || \
				dx->letter != cmd.letter)
			{
				dx->letter = cmd.letter;
				if(!Analyse(dx))
				{
					if(KeReadStateEvent(&(dx->stop_event)) != 0x1)
						dx->request_is_successful = FALSE;
					break;
				}
				if(KeReadStateEvent(&(dx->stop_event)) == 0x1)
					break;
			}
			Defragment(dx);
			break;
		//default:
		//	goto invalid_request;
		}
		/* request completed */
		KeClearEvent(&(dx->sync_event));
	}
	if(dx->request_is_successful)
		return CompleteIrp(Irp,STATUS_SUCCESS,sizeof(ULTRADFG_COMMAND));
	dx->status = STATUS_BEFORE_PROCESSING;
	return CompleteIrp(Irp,STATUS_INVALID_PARAMETER,0);
invalid_request:
	KeClearEvent(&(dx->sync_event));
	return CompleteIrp(Irp,STATUS_INVALID_PARAMETER,0);
}

void DbgPrintNoMem()
{
	DebugPrint("-Ultradfg- No Enough Memory!\n");
}

/* CreateFile() request processing. */
NTSTATUS NTAPI Create_File_IRPprocessing(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	PEXAMPLE_DEVICE_EXTENSION dx = \
			(PEXAMPLE_DEVICE_EXTENSION)(fdo->DeviceExtension);

#ifdef NT4_DBG
	OpenLogFile();
#endif
	DebugPrint("-Ultradfg- Create File\n");
	/* allocate memory for maps processing */
#ifndef NT4_TARGET
	dx->FileMap = AllocatePool(NonPagedPool,FILEMAPSIZE * sizeof(ULONGLONG));
	if(!dx->FileMap) goto no_mem;
	dx->BitMap = AllocatePool(NonPagedPool,BITMAPSIZE * sizeof(UCHAR));
	if(!dx->BitMap)
	{
		ExFreePool(dx->FileMap);
		dx->FileMap = NULL;
		goto no_mem;
	}
#endif
	dx->tmp_buf = AllocatePool(NonPagedPool,32768 + 5);
	if(!dx->tmp_buf)
	{
#ifndef NT4_TARGET
		ExFreePool(dx->FileMap);
		dx->FileMap = NULL;
		ExFreePool(dx->BitMap);
		dx->BitMap = NULL;
#endif
		goto no_mem;
	}
	dx->status = STATUS_BEFORE_PROCESSING;
	return CompleteIrp(Irp,STATUS_SUCCESS,0);
no_mem:
	DbgPrintNoMem();
#ifdef NT4_DBG
	CloseLogFile();
#endif
	return CompleteIrp(Irp,STATUS_NO_MEMORY,0);
}

/* CloseHandle() request processing. */
NTSTATUS NTAPI Close_HandleIRPprocessing(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	PEXAMPLE_DEVICE_EXTENSION dx = \
			(PEXAMPLE_DEVICE_EXTENSION)(fdo->DeviceExtension);

	DebugPrint("-Ultradfg- In Close handler\n"); 
	FreeAllBuffers(dx);
#ifndef NT4_TARGET
	if(dx->FileMap)
	{
		ExFreePool(dx->FileMap);
		dx->FileMap = NULL;
	}
	if(dx->BitMap)
	{
		ExFreePool(dx->BitMap);
		dx->BitMap = NULL;
	}
#endif
	if(dx->tmp_buf)
	{
		ExFreePool(dx->tmp_buf);
		dx->tmp_buf = NULL;
	}
#ifdef NT4_DBG
	CloseLogFile();
#endif
	return CompleteIrp(Irp,STATUS_SUCCESS,0);
}

void UpdateInformation(PEXAMPLE_DEVICE_EXTENSION dx)
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
	if(length)
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

void DestroyFilter(PEXAMPLE_DEVICE_EXTENSION dx)
{
	if(dx->in_filter.buffer) ExFreePool((void *)dx->in_filter.buffer);
	if(dx->ex_filter.buffer) ExFreePool((void *)dx->ex_filter.buffer);
	DestroyList((PLIST *)&dx->in_filter.offsets);
	DestroyList((PLIST *)&dx->ex_filter.offsets);
}

/* DeviceControlRoutine: IRP_MJ_DEVICE_CONTROL request handler. */
NTSTATUS NTAPI DeviceControlRoutine(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	PEXAMPLE_DEVICE_EXTENSION dx;
	PIO_STACK_LOCATION IrpStack;
	ULONG IoControlCode;
	ULONG length;
	PSTATISTIC pst;
	char *map;
	short *filter;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG BytesTxd = 0;
	ULONGLONG cpb; /* clusters per block */
	ULONGLONG i, j;
	UCHAR space_state;
	ULONGLONG x_part[NUM_OF_SPACE_STATES];
	ULONGLONG maximum;
	UCHAR index, k;
	REPORT_TYPE *rt;

	dx = (PEXAMPLE_DEVICE_EXTENSION)(fdo->DeviceExtension);
	IrpStack = IoGetCurrentIrpStackLocation(Irp);
	IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
	length = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	/*
	DebugPrint("-Ultradfg- In DeviceControlRoutine (fdo= %X)\n",fdo);
	DebugPrint("-Ultradfg- DeviceIoControl: IOCTL %X\n", ControlCode );
	*/

	switch(IoControlCode)
	{
		case IOCTL_GET_STATISTIC:
		{
			DebugPrint("-Ultradfg- IOCTL_GET_STATISTIC\n");
			pst = (PSTATISTIC)Irp->AssociatedIrp.SystemBuffer;
			if(length != sizeof(STATISTIC) || !pst)
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
			map = (char *)Irp->AssociatedIrp.SystemBuffer;
			if(!length || !map)
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			KeWaitForSingleObject(&(dx->lock_map_event),
				Executive,KernelMode,FALSE,NULL);
			if(!dx->cluster_map)
			{
				length = 0;
				status = STATUS_UNSUCCESSFUL;
			}
			else
			{
				cpb = dx->clusters_total / length;
				for(i = 0; i < length; i++)
				{
					memset((void *)x_part,0,NUM_OF_SPACE_STATES * sizeof(ULONGLONG));
					for(j = 0; j < cpb; j++)
					{
						space_state = dx->cluster_map[i * cpb + j];
						x_part[space_state] ++;
					}
					maximum = x_part[0];
					index = 0;
					for(k = 1; k < NUM_OF_SPACE_STATES; k++)
					{
						if(x_part[k] > maximum)
						{
							maximum = x_part[k];
							index = k;
						}
					}
					map[i] = index;
				}
			}
			KeSetEvent(&(dx->lock_map_event),IO_NO_INCREMENT,FALSE);
			BytesTxd = length;
			break;
		}
		case IOCTL_ULTRADEFRAG_STOP:
		{
			DebugPrint("-Ultradfg- IOCTL_ULTRADEFRAG_STOP\n");
			KeSetEvent(&(dx->stop_event),IO_NO_INCREMENT,FALSE);
			break;
		}
		case IOCTL_SET_INCLUDE_FILTER:
		case IOCTL_SET_EXCLUDE_FILTER:
		{
			DebugPrint("-Ultradfg- IOCTL_SET_XXX_FILTER\n");
			length = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
			filter = (short *)Irp->AssociatedIrp.SystemBuffer;
			if(IoControlCode == IOCTL_SET_INCLUDE_FILTER)
			{
				DebugPrint("-Ultradfg- Include: %ws\n",filter);
				UpdateFilter(&dx->in_filter,filter,length);
			}
			else
			{
				DebugPrint("-Ultradfg- Exclude: %ws\n",filter);
				UpdateFilter(&dx->ex_filter,filter,length);
			}
			UpdateInformation(dx);
			BytesTxd = length;
			break;
		}
		case IOCTL_SET_DBGPRINT_LEVEL:
		{
			DebugPrint("-Ultradfg- IOCTL_SET_DBGPRINT_LEVEL\n");
			length = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
			if(length != sizeof(ULONG))
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			dbg_level = *((ULONG *)Irp->AssociatedIrp.SystemBuffer);
			DebugPrint("-Ultradfg- dbg_level=%u\n",dbg_level);
			BytesTxd = length;
			break;
		}
		case IOCTL_SET_REPORT_TYPE:
		{
			DebugPrint("-Ultradfg- IOCTL_SET_REPORT_TYPE\n");
			length = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
			if(length != sizeof(REPORT_TYPE))
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			rt = (REPORT_TYPE *)Irp->AssociatedIrp.SystemBuffer;
			dx->report_type.format = rt->format;
			dx->report_type.type = rt->type;
			BytesTxd = length;
			break;
		}
		case IOCTL_SET_USER_MODE_BUFFER:
		{
			DebugPrint("-Ultradfg- IOCTL_SET_USER_MODE_BUFFER\n");
			DebugPrint("-Ultradfg- Address = %x\n",
				IrpStack->Parameters.DeviceIoControl.Type3InputBuffer);
			dx->user_mode_buffer = IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
		#ifdef NT4_TARGET
			if(dx->user_mode_buffer)
			{
				dx->BitMap = dx->user_mode_buffer;
				dx->FileMap = (ULONGLONG *)(dx->user_mode_buffer + BITMAPSIZE * sizeof(UCHAR));
			}
		#endif
			break;
		}
		/* Invalid request */
		default: status = STATUS_INVALID_DEVICE_REQUEST;
	}
	/*
	#if DBG
	DbgPrint("-Ultradfg- DeviceIoControl: %d bytes written\n", (int)BytesTxd);
	#endif
	*/
	return CompleteIrp(Irp,status,BytesTxd);
}

/* UnloadRoutine: unload the driver and free allocated memory.
 * Can be placed in paged memory.
 */
#pragma code_seg("PAGE")

VOID NTAPI UnloadRoutine(IN PDRIVER_OBJECT pDriverObject)
{
	PEXAMPLE_DEVICE_EXTENSION dx;
	UNICODE_STRING *pLinkName;

	dx = (PEXAMPLE_DEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;
	pLinkName = &(dx->ustrSymLinkName);

	DebugPrint("-Ultradfg- In Unload Routine\n");
	DebugPrint("-Ultradfg- Deleted device: pointer to FDO = %X\n",dx->fdo);
	DebugPrint("-Ultradfg- Deleted symlink = %ws\n", pLinkName->Buffer);

	KeSetEvent(&(dx->unload_event),IO_NO_INCREMENT,FALSE);
	FreeAllBuffers(dx);
#ifndef NT4_TARGET
	if(dx->FileMap) ExFreePool(dx->FileMap);
	if(dx->BitMap) ExFreePool(dx->BitMap);
#endif
	if(dx->tmp_buf) ExFreePool(dx->tmp_buf);
	DestroyFilter(dx);
	WaitForThreadComplete(dx);
	/* Delete symbolic link and FDO: */
	IoDeleteSymbolicLink(pLinkName);
	IoDeleteDevice(dx->fdo);
}
#pragma code_seg() /* end PAGE section */
