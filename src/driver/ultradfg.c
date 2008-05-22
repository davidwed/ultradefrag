/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

/* Don't use them in any functions except DriverEntry!!! */
INIT_FUNCTION NTSTATUS CreateDevice(IN PDRIVER_OBJECT DriverObject,
									IN short *name,
									IN short *link,
									OUT UDEFRAG_DEVICE_EXTENSION **dev_ext)
{
	UNICODE_STRING devName;
	UNICODE_STRING symLinkName;
	NTSTATUS status;
	DEVICE_OBJECT *fdo;
	UDEFRAG_DEVICE_EXTENSION *dx;

	RtlInitUnicodeString(&devName,name);
	status = IoCreateDevice(DriverObject,
				sizeof(UDEFRAG_DEVICE_EXTENSION),
				&devName,
				FILE_DEVICE_UNKNOWN,
				0,
				FALSE, /* without exclusive access */
				&fdo);
	if(status != STATUS_SUCCESS){
		DebugPrint("=Ultradfg= IoCreateDevice failed for %ws: %x\n",
				name,(UINT)status);
		return status;
	}

	dx = (PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);
	dx->fdo = fdo;  /* save backward pointer */
	fdo->Flags |= DO_DIRECT_IO;
	*dev_ext = dx;

	/* IMPORTANT: links cannot be created twice */
	RtlInitUnicodeString(&symLinkName,link);
	status = IoCreateSymbolicLink(&symLinkName,&devName);
	if(!NT_SUCCESS(status))
		DebugPrint("=Ultradfg= IoCreateSymbolicLink failed for %ws: %x\n",
				link,(UINT)status);
	return status;
}
									
INIT_FUNCTION void driver_entry_cleanup(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING us;
	DEVICE_OBJECT *pDevice, *pNextDevice;
	
	/* destroy all created symbolic links */
	RtlInitUnicodeString(&us,link_name);
	IoDeleteSymbolicLink(&us);
	RtlInitUnicodeString(&us,L"\\DosDevices\\ultradfgmap");
	IoDeleteSymbolicLink(&us);
	RtlInitUnicodeString(&us,L"\\DosDevices\\ultradfgstat");
	IoDeleteSymbolicLink(&us);

	/* delete device objects */
	pNextDevice = DriverObject->DeviceObject;
	while(pNextDevice){
		pDevice = pNextDevice;
		pNextDevice = pDevice->NextDevice;
		IoDeleteDevice(pDevice);
	}
}

/* DriverEntry - driver initialization */
INIT_FUNCTION NTSTATUS NTAPI DriverEntry(IN PDRIVER_OBJECT DriverObject,
						IN PUNICODE_STRING RegistryPath)
{
	UDEFRAG_DEVICE_EXTENSION *dx, *dx_map, *dx_stat, *dx_stop;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG mj,mn;

	/* Export entry points */
	DriverObject->DriverUnload = UnloadRoutine;
	DriverObject->MajorFunction[IRP_MJ_CREATE]= Create_File_IRPprocessing;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = Close_HandleIRPprocessing;
	DriverObject->MajorFunction[IRP_MJ_READ]  = Read_IRPhandler;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = Write_IRPhandler;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
											  DeviceControlRoutine;

#ifdef NT4_DBG
	OpenLog(); /* We should call them before any DebugPrint() calls !!! */
#endif

	/* Create the main device. */
	status = CreateDevice(DriverObject,device_name,link_name,&dx);
	if(!NT_SUCCESS(status)){
		driver_entry_cleanup(DriverObject);
		return status;
	}
	dx->second_device = 0; dx->map_device = 0; dx->stat_device = 0;
	dx->stop_device = 0;
	dx->main_dx = dx;

	/* Create the map device. */
	status = CreateDevice(DriverObject,L"\\Device\\UltraDefragMap",
			L"\\DosDevices\\ultradfgmap",&dx_map);
	if(!NT_SUCCESS(status)){
		driver_entry_cleanup(DriverObject);
		return status;
	}
	dx_map->second_device = 1; dx_map->map_device = 1; dx_map->stat_device = 0;
	dx_map->stop_device = 0;
	dx_map->main_dx = dx;

/*	RtlInitUnicodeString(&symLinkName,L"\\??\\X:");
	status = IoCreateSymbolicLink(&symLinkName,&devName);
	if(!NT_SUCCESS(status)){
		DebugPrint("=Ultradfg= IoCreateSymbolicLink failed %x\n",(UINT)status);
	}
*/
	/*
	* Create the separated device for statistical data
	* representation.
	*/
	status = CreateDevice(DriverObject,L"\\Device\\UltraDefragStat",
			L"\\DosDevices\\ultradfgstat",&dx_stat);
	if(!NT_SUCCESS(status)){
		driver_entry_cleanup(DriverObject);
		return status;
	}
	dx_stat->second_device = 1; dx_stat->map_device = 0; dx_stat->stat_device = 1;
	dx_stat->stop_device = 0;
	dx_stat->main_dx = dx;

	/* create a special device for 'stop' requests processing */
	status = CreateDevice(DriverObject,L"\\Device\\UltraDefragStop",
			L"\\DosDevices\\ultradfgstop",&dx_stop);
	if(!NT_SUCCESS(status)){
		driver_entry_cleanup(DriverObject);
		return status;
	}
	dx_stop->second_device = 1; dx_stop->map_device = 0; dx_stop->stat_device = 0;
	dx_stop->stop_device = 1;
	dx_stop->main_dx = dx;

	/* data initialization */
	InitDX_0(dx);
	new_cluster_map = NULL; /* console and native tools don't needs them */
	map_size = 0;
	dbg_level = 0;

	/* allocate memory */
#ifndef NT4_TARGET
	dx->FileMap = AllocatePool(NonPagedPool,FILEMAPSIZE * sizeof(ULONGLONG));
	dx->BitMap = AllocatePool(NonPagedPool,BITMAPSIZE * sizeof(UCHAR));
	if(!dx->FileMap || !dx->BitMap) goto no_mem;
#endif
	dx->tmp_buf = AllocatePool(NonPagedPool,32768 + 5);
	if(!dx->tmp_buf){
#ifndef NT4_TARGET
no_mem:
#endif
		DbgPrintNoMem();
#ifndef NT4_TARGET
		ExFreePoolSafe(dx->FileMap);
		ExFreePoolSafe(dx->BitMap);
#endif
		ExFreePoolSafe(dx->tmp_buf);

		driver_entry_cleanup(DriverObject);
		return STATUS_NO_MEMORY;
	}

	DebugPrint("=Ultradfg= Main FDO %p, DevExt=%p\n",dx->fdo,dx);
	DebugPrint("=Ultradfg= Map  FDO %p, DevExt=%p\n",dx_map->fdo,dx_map);
	DebugPrint("=Ultradfg= Stat FDO %p, DevExt=%p\n",dx_stat->fdo,dx_stat);

	/* Get Windows version */
	if(PsGetVersion(&mj,&mn,NULL,NULL))
		DebugPrint("=Ultradfg= NT %u.%u checked.\n",mj,mn);
	else
		DebugPrint("=Ultradfg= NT %u.%u free.\n",mj,mn);
	dx->xp_compatible = (((mj << 6) + mn) > ((5 << 6) + 0));
	DebugPrint("=Ultradfg= DriverEntry successfully completed\n");
	return STATUS_SUCCESS;
}
#if !defined(__GNUC__)
#pragma code_seg() /* end INIT section */
#endif

/* CompleteIrp: Set IoStatus and complete IRP processing */ 
NTSTATUS CompleteIrp(PIRP Irp,NTSTATUS status,ULONG info)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return status;
}

/* read requests handler */
NTSTATUS NTAPI Read_IRPhandler(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	char *pBuffer;
	ULONG length;
	STATISTIC *st;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	UDEFRAG_DEVICE_EXTENSION *dx = \
			(PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);
	UDEFRAG_DEVICE_EXTENSION *main_dx = dx->main_dx;;

	CHECK_IRP(Irp);

	if(KeReadStateEvent(&dx->unload_event) == 0x1){
		DebugPrint1("-Ultradfg- is busy!\n");
		return CompleteIrp(Irp,STATUS_DEVICE_BUSY,0);
	}
	InterlockedIncrement(&dx->working_rq);
//	DebugPrint("-Ultradfg- in Read_IRPhandler\n");

	length = IrpStack->Parameters.Read.Length;
#ifdef NT4_TARGET
	pBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
#else
	pBuffer = (char *)MmGetSystemAddressForMdlSafe(Irp->MdlAddress,NormalPagePriority);
#endif

	if(dx->map_device){
		DebugPrint2("-Ultradfg- Map request\n");
		if(length != map_size || !pBuffer || !new_cluster_map){
			InterlockedDecrement(&dx->working_rq);
			return CompleteIrp(Irp,STATUS_INVALID_PARAMETER,0);
		}
		GetMap(pBuffer);
		InterlockedDecrement(&dx->working_rq);
		return CompleteIrp(Irp,STATUS_SUCCESS,map_size);
	}

	if(dx->stat_device){
		DebugPrint2("-Ultradfg- Statistics request\n");
		if(length != sizeof(STATISTIC) || !pBuffer){
			InterlockedDecrement(&dx->working_rq);
			return CompleteIrp(Irp,STATUS_INVALID_PARAMETER,0);
		}
		st = (STATISTIC *)pBuffer;
		st->filecounter = main_dx->filecounter;
		st->dircounter = main_dx->dircounter;
		st->compressedcounter = main_dx->compressedcounter;
		st->fragmcounter = main_dx->fragmcounter;
		st->fragmfilecounter = main_dx->fragmfilecounter;
		st->free_space = main_dx->free_space;
		st->total_space = main_dx->total_space;
		st->mft_size = main_dx->mft_size;
		st->processed_clusters = main_dx->processed_clusters;
		st->bytes_per_cluster = main_dx->bytes_per_cluster;
		st->current_operation = main_dx->current_operation;
		st->clusters_to_move_initial = main_dx->clusters_to_move_initial;
		st->clusters_to_move = main_dx->clusters_to_move;
		InterlockedDecrement(&dx->working_rq);
		return CompleteIrp(Irp,STATUS_SUCCESS,sizeof(STATISTIC));
	}
	InterlockedDecrement(&dx->working_rq);		
	return CompleteIrp(Irp,STATUS_SUCCESS,0);
}

/*
* waits for safe to unload state when all requests were finished
*/
void wait_for_idle_state(UDEFRAG_DEVICE_EXTENSION *dx)
{
	int i;
	LARGE_INTEGER interval;
	
	/* set events */
	KeSetEvent(&dx->stop_event,IO_NO_INCREMENT,FALSE);
	KeSetEvent(&dx->unload_event,IO_NO_INCREMENT,FALSE);
	/*
	* Wait two minutes, then show BSOD 
	* if some request was not finished.
	*/
	DebugPrint("Waiting started.\n");
	interval.QuadPart = (-1) * 10000 * 10; /* 10 ms */
	for(i = 0; i < (2 * 60 * 100); i++){
		KeDelayExecutionThread(KernelMode,FALSE,&interval);
		if(!dx->working_rq){
			DebugPrint("Waiting finished.\n");
			return;
		}
	}
	DebugPrint("Waiting failure.\n");
	//KeBugCheckEx(0,(ULONG_PTR)dx->working_rq,0,0,0);
}

/* NOTE: always change state to busy */
BOOLEAN is_device_busy(UDEFRAG_DEVICE_EXTENSION *dx)
{
	KIRQL oldIrql;

	KeAcquireSpinLock(&dx->spin_lock,&oldIrql);
	if(KeSetEvent(&dx->sync_event,IO_NO_INCREMENT,FALSE)){
		DebugPrint1("-Ultradfg- is busy!\n");
		KeReleaseSpinLock(&dx->spin_lock,oldIrql);
		return TRUE;
	}
	KeReleaseSpinLock(&dx->spin_lock,oldIrql);
	return FALSE;
}
/*
void QueryEnvVariable(short *name, short *buffer, int bytes)
{
	UNICODE_STRING n, v;
	NTSTATUS Status;
	
	RtlInitUnicodeString(&n,name);
	v.Buffer = buffer;
	v.Length = 0;
	v.MaximumLength = bytes;
	Status = RtlQueryEnvironmentVariable_U(NULL,&n,&v);
	if(!NT_SUCCESS(Status))
		DebugPrint("Can't query %ws environment variable: %x!",
				name,(UINT)Status);
}
*/
void ReloadSettings(void)
{
	short *b;
	/* reload settings from environment variables */
	b = AllocatePool(NonPagedPool,4097 * sizeof(short));
	if(!b){
		DbgPrintNoMem();
		return;
	}
	ExFreePool(b);
}

/*
* Write request handler.
* Possible commands are:
* 	aX   - analyse the X volume -
* 	dX   - defragment            | for main device
* 	cX   - compact               |
*   r    - reload settings      -
*
*   s    - stop                 =  for 'stop' device
*/
NTSTATUS NTAPI Write_IRPhandler(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

	UDEFRAG_DEVICE_EXTENSION *dx = \
			(PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);
	PVOID addr;
	ULONG length;
	UCHAR letter;
	NTSTATUS request_status = STATUS_SUCCESS;
	char cmd[32]; /* FIXME: replace it with malloc call */

	CHECK_IRP(Irp);

	if(dx->stop_device){
		DebugPrint("-Ultradfg- STOP request\n");
		KeSetEvent(&dx->main_dx->stop_event,IO_NO_INCREMENT,FALSE);
		return CompleteIrp(Irp,STATUS_SUCCESS,1);
	}
	
	if(dx->second_device)
		return CompleteIrp(Irp,STATUS_SUCCESS,0);
	
	DebugPrint("-Ultradfg- in Write_IRPhandler\n");
	
	if(KeReadStateEvent(&dx->unload_event) == 0x1){
		DebugPrint1("-Ultradfg- is busy!\n");
		return CompleteIrp(Irp,STATUS_DEVICE_BUSY,0);
	}
	InterlockedIncrement(&dx->working_rq);

	length = IrpStack->Parameters.Write.Length;
#ifdef NT4_TARGET
	addr = MmGetSystemAddressForMdl(Irp->MdlAddress);
#else
	addr = MmGetSystemAddressForMdlSafe(Irp->MdlAddress,NormalPagePriority);
#endif
	if(!length || !addr) goto invalid_request;
	RtlCopyMemory(cmd,addr,min(length,31));
	cmd[min(length,31)] = 0;

	DebugPrint("-Ultradfg- Command = %s\n",cmd);

	/*
	* If the previous request isn't complete,
	* return STATUS_DEVICE_BUSY.
	* Because the new one will change global variables.
	*/
	if(is_device_busy(dx)){
		InterlockedDecrement(&dx->working_rq);
		return CompleteIrp(Irp,STATUS_DEVICE_BUSY,0);
	}

	dx->compact_flag = FALSE;
	switch(cmd[0]){
	case 'a':
	case 'A':
		if(length < 2){
			KeClearEvent(&dx->sync_event);
			goto invalid_request;
		}
		/* important for nt 4.0 and w2k - bug #1771887 solution: */
		letter = (UCHAR)toupper((int)cmd[1]);
		/* validate letter */
		if(letter < 'A' || letter > 'Z'){
			KeClearEvent(&dx->sync_event);
			goto invalid_request;
		}
		/* volume letters assigned by SUBST command are wrong... */
		dx->letter = letter;
		request_status = Analyse(dx);
		break;
	case 'c':
	case 'C':
		dx->compact_flag = TRUE;
	case 'd':
	case 'D':
		if(length < 2){
			KeClearEvent(&dx->sync_event);
			goto invalid_request;
		}
		/* important for nt 4.0 and w2k - bug #1771887 solution: */
		letter = (UCHAR)toupper((int)cmd[1]);
		/* validate letter */
		if(letter < 'A' || letter > 'Z'){
			KeClearEvent(&dx->sync_event);
			goto invalid_request;
		}
		/* volume letters assigned by SUBST command are wrong... */
		if(dx->status == STATUS_BEFORE_PROCESSING || \
			dx->letter != letter)
		{
			dx->letter = letter;
			request_status = Analyse(dx);
			if(KeReadStateEvent(&dx->stop_event) == 0x1) break;
			if(!NT_SUCCESS(request_status)) break;
		}
		request_status = STATUS_SUCCESS;
		if(!dx->compact_flag) Defragment(dx);
		else DefragmentFreeSpace(dx);
		break;
	case 'r':
	case 'R':
		ReloadSettings();
		KeClearEvent(&dx->sync_event);
		InterlockedDecrement(&dx->working_rq);
		return CompleteIrp(Irp,STATUS_SUCCESS,1);
		break;
	default:
		KeClearEvent(&dx->sync_event);
		goto invalid_request;
	}
	/* a/d/c request completed */
	KeClearEvent(&dx->sync_event);
	/* if the operation was aborted by user then return success */
	if(KeReadStateEvent(&dx->stop_event) == 0x1){
		InterlockedDecrement(&dx->working_rq);
		return CompleteIrp(Irp,STATUS_SUCCESS,length);
	}
	/* if the operation has unsuccessful result, clear the dx->status field */
	if(!NT_SUCCESS(request_status)){
		dx->status = STATUS_BEFORE_PROCESSING;
		InterlockedDecrement(&dx->working_rq);
		return CompleteIrp(Irp,request_status,0);
	}
	InterlockedDecrement(&dx->working_rq);
	return CompleteIrp(Irp,STATUS_SUCCESS,length);

invalid_request:
	InterlockedDecrement(&dx->working_rq);
	return CompleteIrp(Irp,STATUS_INVALID_PARAMETER,0);
}

/* CreateFile() request processing. */
NTSTATUS NTAPI Create_File_IRPprocessing(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	UDEFRAG_DEVICE_EXTENSION *dx = \
			(PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);

	if(dx->second_device)
		return CompleteIrp(Irp,STATUS_SUCCESS,0);

	DebugPrint("-Ultradfg- Create File\n");
	CHECK_IRP(Irp);
	KeClearEvent(&dx->unload_event);
	dx->status = STATUS_BEFORE_PROCESSING;
	return CompleteIrp(Irp,STATUS_SUCCESS,0);
}

/* CloseHandle() request processing. */
NTSTATUS NTAPI Close_HandleIRPprocessing(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	UDEFRAG_DEVICE_EXTENSION *dx = \
			(PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);

	if(dx->second_device)
		return CompleteIrp(Irp,STATUS_SUCCESS,0);

	DebugPrint("-Ultradfg- In Close handler\n"); 
	CHECK_IRP(Irp);
	FreeAllBuffersInIdleState(dx);
	return CompleteIrp(Irp,STATUS_SUCCESS,0);
}

/* DeviceControlRoutine: IRP_MJ_DEVICE_CONTROL request handler. */
NTSTATUS NTAPI DeviceControlRoutine(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	UDEFRAG_DEVICE_EXTENSION *dx;
	PIO_STACK_LOCATION IrpStack;
	ULONG IoControlCode;
	short *filter;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG BytesTxd = 0;
	REPORT_TYPE *rt;
	UCHAR *user_mode_buffer; /* for nt 4.0 */
	ULONG in_len, out_len;

	CHECK_IRP(Irp);
	dx = (PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);

	if(dx->second_device)
		return CompleteIrp(Irp,STATUS_SUCCESS,0);

	if(KeReadStateEvent(&dx->unload_event) == 0x1){
		DebugPrint1("-Ultradfg- is busy!\n");
		return CompleteIrp(Irp,STATUS_DEVICE_BUSY,0);
	}
	InterlockedIncrement(&dx->working_rq);

	IrpStack = IoGetCurrentIrpStackLocation(Irp);
	IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
	in_len = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	out_len = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch(IoControlCode){
	case IOCTL_SET_INCLUDE_FILTER:
	case IOCTL_SET_EXCLUDE_FILTER:
		DebugPrint("-Ultradfg- IOCTL_SET_XXX_FILTER\n");
		filter = (short *)Irp->AssociatedIrp.SystemBuffer;
		if(filter){
			if(IoControlCode == IOCTL_SET_INCLUDE_FILTER){
				DebugPrint("-Ultradfg- Include: %ws\n",filter);
				UpdateFilter(dx,&dx->in_filter,filter,in_len);
			} else {
				DebugPrint("-Ultradfg- Exclude: %ws\n",filter);
				UpdateFilter(dx,&dx->ex_filter,filter,in_len);
			}
		}
		BytesTxd = in_len;
		break;
	case IOCTL_SET_DBGPRINT_LEVEL:
		DebugPrint("-Ultradfg- IOCTL_SET_DBGPRINT_LEVEL\n");
		if(in_len != sizeof(ULONG)){
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		dbg_level = *((ULONG *)Irp->AssociatedIrp.SystemBuffer);
		DebugPrint("-Ultradfg- dbg_level=%u\n",dbg_level);
		BytesTxd = in_len;
		break;
	case IOCTL_SET_REPORT_TYPE:
		DebugPrint("-Ultradfg- IOCTL_SET_REPORT_TYPE\n");
		if(in_len != sizeof(REPORT_TYPE)){
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		rt = (REPORT_TYPE *)Irp->AssociatedIrp.SystemBuffer;
		//dx->report_type.format = rt->format;
		dx->report_type.type = rt->type;
		BytesTxd = in_len;
		break;
	case IOCTL_SET_USER_MODE_BUFFER:
		DebugPrint("-Ultradfg- IOCTL_SET_USER_MODE_BUFFER\n");
		DebugPrint("-Ultradfg- Address = %p\n",
			IrpStack->Parameters.DeviceIoControl.Type3InputBuffer);
		user_mode_buffer = IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
		#ifdef NT4_TARGET
		if(user_mode_buffer){
			dx->BitMap = user_mode_buffer;
			dx->FileMap = (ULONGLONG *)(user_mode_buffer + BITMAPSIZE * sizeof(UCHAR));
			dx->pnextLcn = (ULONGLONG *)(dx->FileMap + FILEMAPSIZE * sizeof(ULONGLONG));
			dx->pstartVcn = (ULONGLONG *)(dx->pnextLcn + sizeof(ULONGLONG));
			dx->pmoveFile = (MOVEFILE_DESCRIPTOR *)(dx->pnextLcn + 3 * sizeof(ULONGLONG));
		}
		#endif
		break;
	case IOCTL_SET_CLUSTER_MAP_SIZE:
		DebugPrint("-Ultradfg- IOCTL_SET_CLUSTER_MAP_SIZE\n");
		if(in_len != sizeof(ULONG)){
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		status = AllocateMap(*((ULONG *)Irp->AssociatedIrp.SystemBuffer));
		if(NT_SUCCESS(status))
			BytesTxd = in_len;
		break;
	case IOCTL_SET_SIZELIMIT:
		DebugPrint("-Ultradfg- IOCTL_SET_SIZELIMIT\n");
		if(in_len != sizeof(ULONGLONG)){
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		dx->sizelimit = *((ULONGLONG *)Irp->AssociatedIrp.SystemBuffer);
		DebugPrint("-Ultradfg- Sizelimit = %I64u\n",dx->sizelimit);
		BytesTxd = in_len;
		break;
	/* Invalid request */
	default: status = STATUS_INVALID_DEVICE_REQUEST;
	}
	InterlockedDecrement(&dx->working_rq);
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
	PDEVICE_OBJECT pDevice;
	UDEFRAG_DEVICE_EXTENSION *dx;
	UNICODE_STRING us;

	DebugPrint("-Ultradfg- In Unload Routine\n");

	/* delete all symbolic links */
	RtlInitUnicodeString(&us,link_name);
	IoDeleteSymbolicLink(&us);
	RtlInitUnicodeString(&us,L"\\DosDevices\\ultradfgmap");
	IoDeleteSymbolicLink(&us);
	RtlInitUnicodeString(&us,L"\\DosDevices\\ultradfgstat");
	IoDeleteSymbolicLink(&us);
	RtlInitUnicodeString(&us,L"\\DosDevices\\ultradfgstop");
	IoDeleteSymbolicLink(&us);
	DebugPrint("-Ultradfg- Symbolic links were deleted\n");

	/* loop through all created devices */
	pDevice = pDriverObject->DeviceObject;
	while(pDevice){
		dx = (PUDEFRAG_DEVICE_EXTENSION)pDevice->DeviceExtension;
		pDevice = pDevice->NextDevice;
		if(!dx->second_device){
			FreeAllBuffersInIdleState(dx);
			#ifndef NT4_TARGET
			ExFreePoolSafe(dx->FileMap);
			ExFreePoolSafe(dx->BitMap);
			#endif
			ExFreePoolSafe(dx->tmp_buf);
			ExFreePoolSafe(new_cluster_map);
			DestroyFilter(dx);
		}
		/* delete device */
		DebugPrint("-Ultradfg- Deleted device %p\n",dx->fdo);
		IoDeleteDevice(dx->fdo);
	}

#ifdef NT4_DBG
	CloseLog();
#endif
}
#if !defined(__GNUC__)
#pragma code_seg() /* end PAGE section */
#endif
