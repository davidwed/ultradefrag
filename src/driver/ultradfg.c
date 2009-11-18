/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
#include "../include/ultradfgver.h"

#if !defined(__GNUC__)
#pragma code_seg("INIT") /* begin of section INIT */
#endif

/* Don't use them in any functions except DriverEntry!!! */

INIT_FUNCTION void GetKernelProcAddresses(void)
{
	ULONG mj,mn;

	/* print driver version */
	DebugPrint("=Ultradfg= %s\n",VERSIONINTITLE);

	/* get windows version */
	if(PsGetVersion(&mj,&mn,NULL,NULL))
		DebugPrint("=Ultradfg= NT %u.%u checked.\n",mj,mn);
	else
		DebugPrint("=Ultradfg= NT %u.%u free.\n",mj,mn);
	/*dx->xp_compatible = (((mj << 6) + mn) > ((5 << 6) + 0));*/
	nt4_system = (mj == 4) ? 1 : 0;
	w2k_system = (mj == 5 && mn == 0) ? 1 : 0;
	
	/* get nt kernel base address */
	kernel_addr = KernelGetModuleBase("ntoskrnl.exe");
	if(!kernel_addr) kernel_addr = KernelGetModuleBase("ntkrnlpa.exe");
	if(!kernel_addr) kernel_addr = KernelGetModuleBase("ntkrnlmp.exe");
	if(!kernel_addr) kernel_addr = KernelGetModuleBase("ntkrpamp.exe");
	/* FIXME: add all other kernel names */

	if(kernel_addr){
		ptrMmMapLockedPagesSpecifyCache = KernelGetProcAddress(kernel_addr,
			"MmMapLockedPagesSpecifyCache");
		ptrExFreePoolWithTag = KernelGetProcAddress(kernel_addr,
			"ExFreePoolWithTag");
		ptrKeRegisterBugCheckReasonCallback = KernelGetProcAddress(kernel_addr,
			"KeRegisterBugCheckReasonCallback");
		ptrKeDeregisterBugCheckReasonCallback = KernelGetProcAddress(kernel_addr,
			"KeDeregisterBugCheckReasonCallback");
		ptrKeQueryInterruptTime = KernelGetProcAddress(kernel_addr,
			"KeQueryInterruptTime");
	}

	/* print kernel functions addresses */
	DebugPrint("=Ultradfg= Kernel address: %p\n",kernel_addr);
	DebugPrint("=Ultradfg= ExFreePoolWithTag address: %p\n",
		ptrExFreePoolWithTag);
	DebugPrint("=Ultradfg= KeDeregisterBugCheckReasonCallback address: %p\n",
		ptrKeDeregisterBugCheckReasonCallback);
	DebugPrint("=Ultradfg= KeRegisterBugCheckReasonCallback address: %p\n",
		ptrKeRegisterBugCheckReasonCallback);
	DebugPrint("=Ultradfg= MmMapLockedPagesSpecifyCache address: %p\n",
		ptrMmMapLockedPagesSpecifyCache);
	DebugPrint("=Ultradfg= KeQueryInterruptTime address: %p\n",
		ptrKeQueryInterruptTime);
}

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
	DeregisterBugCheckCallbacks();
}

/* DriverEntry - driver initialization */
INIT_FUNCTION NTSTATUS NTAPI DriverEntry(IN PDRIVER_OBJECT DriverObject,
						IN PUNICODE_STRING RegistryPath)
{
	UDEFRAG_DEVICE_EXTENSION *dx, *dx_map, *dx_stat, *dx_stop;
	NTSTATUS status = STATUS_SUCCESS;
/*	UNICODE_STRING devName;
	UNICODE_STRING symLinkName;
*/
/*	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK ioStatus;
	HANDLE hFile;
	NTSTATUS Status;
	UNICODE_STRING us;
*/
	ULONG mj,mn;

	/* 1. Export entry points */
	DriverObject->DriverUnload = UnloadRoutine;
	DriverObject->MajorFunction[IRP_MJ_CREATE]= Create_File_IRPprocessing;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = Close_HandleIRPprocessing;
	DriverObject->MajorFunction[IRP_MJ_READ]  = Read_IRPhandler;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = Write_IRPhandler;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
											  DeviceControlRoutine;

	PsGetVersion(&mj,&mn,NULL,NULL);
	nt4_system = (mj == 4) ? 1 : 0;

	/* 3. our driver (since 2.1.0 version) must be os version independent */
	GetKernelProcAddresses();

	/* 4. Set bug check callbacks */
	RegisterBugCheckCallbacks();


/*	RtlInitUnicodeString(&us,L"\\??\\C:\\$MFT");
	InitializeObjectAttributes(&ObjectAttributes,&us,OBJ_KERNEL_HANDLE,NULL,NULL);
	Status = ZwCreateFile(&hFile,FILE_GENERIC_READ,&ObjectAttributes,&ioStatus,
			  NULL,0,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,
			  FILE_NO_INTERMEDIATE_BUFFERING,NULL,0);
	DbgPrint("$MFT opened with status %x\n",(UINT)Status);
	// Always 0xC0000022 - look at http://free.pages.at/blumetools/tools.html 
	// for more details.
*/
	
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

/*	RtlInitUnicodeString(&devName,L"\\Device\\UltraDefragMap");
	RtlInitUnicodeString(&symLinkName,L"\\??\\X:\\abc");
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
	if(!nt4_system){
		/* few kb from nonpaged pool */
		dx->FileMap = AllocatePool(NonPagedPool,FILEMAPSIZE * sizeof(ULONGLONG));
		dx->BitMap = AllocatePool(NonPagedPool,BITMAPSIZE * sizeof(UCHAR));
		if(!dx->FileMap || !dx->BitMap){
			DebugPrint("-Ultradfg- cannot allocate memory for FileMap and BitMap!\n");
			if(!nt4_system){
				ExFreePoolSafe(dx->FileMap);
				ExFreePoolSafe(dx->BitMap);
			}
			driver_entry_cleanup(DriverObject);
			return STATUS_NO_MEMORY;
		}
	}

	DebugPrint("=Ultradfg= Main FDO %p, DevExt=%p\n",dx->fdo,dx);
	DebugPrint("=Ultradfg= Map  FDO %p, DevExt=%p\n",dx_map->fdo,dx_map);
	DebugPrint("=Ultradfg= Stat FDO %p, DevExt=%p\n",dx_stat->fdo,dx_stat);

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

/* CreateFile() request processing. */
NTSTATUS NTAPI Create_File_IRPprocessing(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	UDEFRAG_DEVICE_EXTENSION *dx = \
			(PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);

	if(!dx->second_device){
		DebugPrint("-Ultradfg- CreateFile request for main device\n");
		CHECK_IRP(Irp);
	}
	return CompleteIrp(Irp,STATUS_SUCCESS,0);
}

/* CloseHandle() request processing. */
NTSTATUS NTAPI Close_HandleIRPprocessing(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
	UDEFRAG_DEVICE_EXTENSION *dx = \
			(PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);

	if(!dx->second_device){
		DebugPrint("-Ultradfg- CloseHandle request for main device\n"); 
		CHECK_IRP(Irp);
	
		stop_all_requests();
		FreeAllBuffers(dx);
		KeSetEvent(&sync_event,IO_NO_INCREMENT,FALSE);
		KeSetEvent(&sync_event_2,IO_NO_INCREMENT,FALSE);
		KeClearEvent(&unload_event);
	}
	return CompleteIrp(Irp,STATUS_SUCCESS,0);
}

/* read requests handler */
NTSTATUS NTAPI Read_IRPhandler(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	UDEFRAG_DEVICE_EXTENSION *dx = \
			(PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);
	UDEFRAG_DEVICE_EXTENSION *main_dx = dx->main_dx;
	NTSTATUS Status;
	char *pBuffer;
	STATISTIC *st;
	ULONG length;

	CHECK_IRP(Irp);

	/* wait for other operations to be finished */
	Status = KeWaitForSingleObject(&sync_event_2,Executive,KernelMode,FALSE,NULL);
	if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- is busy (sync_event_2, Read_IRPhandler)!\n");
		return CompleteIrp(Irp,STATUS_DEVICE_BUSY,0);
	}
	if(KeReadStateEvent(&unload_event) == 0x1){
		DebugPrint("-Ultradfg- is busy (unload_event, Read_IRPhandler)!\n");
		KeSetEvent(&sync_event_2,IO_NO_INCREMENT,FALSE);
		return CompleteIrp(Irp,STATUS_DEVICE_BUSY,0);
	}

	length = IrpStack->Parameters.Read.Length;
	pBuffer = Nt_MmGetSystemAddressForMdl(Irp->MdlAddress);

	if(dx->map_device){
		DebugPrint2("-Ultradfg- Map request\n");
		if(length != map_size || !pBuffer || !new_cluster_map)
			goto invalid_request;
		GetMap(pBuffer); goto success;
	}

	if(dx->stat_device){
		DebugPrint2("-Ultradfg- Statistics request\n");
		if(length != sizeof(STATISTIC) || !pBuffer)
			goto invalid_request;
		st = (STATISTIC *)pBuffer;
		st->filecounter = main_dx->filecounter;
		st->dircounter = main_dx->dircounter;
		st->compressedcounter = main_dx->compressedcounter;
		st->fragmcounter = main_dx->fragmcounter;
		st->fragmfilecounter = main_dx->fragmfilecounter;
		st->free_space = main_dx->free_space;
		st->total_space = main_dx->total_space;
		st->mft_size = main_dx->mft_size;
		st->current_operation = main_dx->current_operation;
		st->clusters_to_process = main_dx->clusters_to_process;
		st->processed_clusters = main_dx->processed_clusters;
		goto success;
	}
success:
	KeSetEvent(&sync_event_2,IO_NO_INCREMENT,FALSE);
	return CompleteIrp(Irp,STATUS_SUCCESS,length);

invalid_request:
	KeSetEvent(&sync_event_2,IO_NO_INCREMENT,FALSE);
	return CompleteIrp(Irp,STATUS_INVALID_PARAMETER,0);
}

/*
* Write request handler.
* Possible commands are:
* 	aX   - analyse the X volume -
* 	dX   - defragment            | for main device
* 	cX   - compact               |
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
	LARGE_INTEGER interval;
	NTSTATUS Status;
//	int *p = NULL;

	CHECK_IRP(Irp);

	if(dx->stop_device){
		DebugPrint("-Ultradfg- STOP request\n");
		KeSetEvent(&stop_event,IO_NO_INCREMENT,FALSE);
		return CompleteIrp(Irp,STATUS_SUCCESS,1);
	}
	
	if(dx->second_device)
		return CompleteIrp(Irp,STATUS_SUCCESS,0);
	
	DebugPrint("-Ultradfg- in Write_IRPhandler\n");
	
	length = IrpStack->Parameters.Write.Length;
	addr = Nt_MmGetSystemAddressForMdl(Irp->MdlAddress);
	if(!length || !addr) return CompleteIrp(Irp,STATUS_INVALID_PARAMETER,0);
	RtlCopyMemory(cmd,addr,min(length,31));
	cmd[min(length,31)] = 0;

	DebugPrint("-Ultradfg- Command = %s\n",cmd);

	/*
	* If the previous request isn't complete,
	* return STATUS_DEVICE_BUSY.
	* Because the new one will change global variables.
	*/
	interval.QuadPart = (-1); /* 100 nsec */
	Status = KeWaitForSingleObject(&sync_event,Executive,KernelMode,FALSE,&interval);
	if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- is busy (sync_event, Write_IRPhandler)!\n");
		return CompleteIrp(Irp,STATUS_DEVICE_BUSY,0);
	}
	
	if(cmd[0] == 'a' || cmd[0] == 'A') dx->AnalysisJob = TRUE;
	else dx->AnalysisJob = FALSE;

	dx->compact_flag = FALSE;
	switch(cmd[0]){
	case 'c':
	case 'C':
		dx->compact_flag = TRUE;
	case 'a':
	case 'A':
	case 'd':
	case 'D':
		
/* Make a BSOD for debugging purposes */
//*p = 0x20;
		
		KeClearEvent(&stop_event);
		if(KeReadStateEvent(&unload_event) == 0x1) break;
		
		if(length < 2) goto invalid_request;
		/* important for nt 4.0 and w2k - bug #1771887 solution: */
		letter = (UCHAR)toupper((int)cmd[1]);
		/* validate letter */
		if(letter < 'A' || letter > 'Z') goto invalid_request;

		dx->letter = letter;
		volume_letter = letter;
		DeleteLogFile(dx);

		request_status = Analyse(dx);
		if(cmd[0] == 'a' || cmd[0] == 'A') break;
		if(KeReadStateEvent(&stop_event) == 0x1) break;
		if(!NT_SUCCESS(request_status)) break;

		/*
		* NTFS volumes with cluster size greater than 4 kb
		* cannot be defragmented on Windows 2000.
		* This is a well known limitation of Windows Defrag API.
		*/
		if(dx->partition_type == NTFS_PARTITION && dx->bytes_per_cluster > 4096 && w2k_system){
			DebugPrint("-Ultradfg- Cannot defragment NTFS volumes with ->\n");
			DebugPrint("-Ultradfg- -> cluster size greater than 4 kb   ->\n");
			DebugPrint("-Ultradfg- -> on Windows 2000 (read docs for details).\n");
			request_status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		
		request_status = STATUS_SUCCESS;
		if(!dx->compact_flag) Defragment(dx);
		else Optimize(dx);
		break;
	default:
		goto invalid_request;
	}
	/* a/d/c request completed */
	SaveFragmFilesListToDisk(dx);
	/* free all buffers, free the volume handle (bug #1794336 fixup) */
	FreeAllBuffers(dx);
	KeSetEvent(&sync_event,IO_NO_INCREMENT,FALSE);
	/*
	* If the operation was aborted by user or 
	* has been finished successfully then return success.
	*/
	if(KeReadStateEvent(&stop_event) == 0x1 || NT_SUCCESS(request_status))
		return CompleteIrp(Irp,STATUS_SUCCESS,length);
	/* if the operation has unsuccessful result, clear the dx->status field */
	return CompleteIrp(Irp,request_status,0);

invalid_request:
	KeSetEvent(&sync_event,IO_NO_INCREMENT,FALSE);
	return CompleteIrp(Irp,STATUS_INVALID_PARAMETER,0);
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
	UCHAR *user_mode_buffer; /* for nt 4.0 */
	ULONG in_len, out_len;
	NTSTATUS Status;

	CHECK_IRP(Irp);
	dx = (PUDEFRAG_DEVICE_EXTENSION)(fdo->DeviceExtension);

	if(dx->second_device)
		return CompleteIrp(Irp,STATUS_SUCCESS,0);

	/* wait for other operations to be finished */
	Status = KeWaitForSingleObject(&sync_event_2,Executive,KernelMode,FALSE,NULL);
	if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- is busy (sync_event_2, DeviceControlRoutine)!\n");
		return CompleteIrp(Irp,STATUS_DEVICE_BUSY,0);
	}
	if(KeReadStateEvent(&unload_event) == 0x1){
		DebugPrint("-Ultradfg- is busy (unload_event, DeviceControlRoutine)!\n");
		KeSetEvent(&sync_event_2,IO_NO_INCREMENT,FALSE);
		return CompleteIrp(Irp,STATUS_DEVICE_BUSY,0);
	}

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
				context_menu_handler = CheckForContextMenuHandler(dx);
				if(context_menu_handler)
					DebugPrint("-Ultradfg- Context menu handler?\n");
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
	case IOCTL_SET_REPORT_STATE:
		DebugPrint("-Ultradfg- IOCTL_SET_REPORT_STATE\n");
		if(in_len != sizeof(ULONG)){
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		dx->disable_reports = *((ULONG *)Irp->AssociatedIrp.SystemBuffer);
		if(dx->disable_reports)
			DebugPrint("-Ultradfg- Disable reports: YES\n");
		else
			DebugPrint("-Ultradfg- Disable reports: NO\n");
		BytesTxd = in_len;
		break;
	case IOCTL_SET_USER_MODE_BUFFER:
		DebugPrint("-Ultradfg- IOCTL_SET_USER_MODE_BUFFER\n");
		DebugPrint("-Ultradfg- Address = %p\n",
			IrpStack->Parameters.DeviceIoControl.Type3InputBuffer);
		user_mode_buffer = IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
		if(nt4_system && user_mode_buffer){
			dx->BitMap = user_mode_buffer;
			dx->FileMap = (ULONGLONG *)(user_mode_buffer + BITMAPSIZE * sizeof(UCHAR));
			dx->pnextLcn = (ULONGLONG *)(dx->FileMap + FILEMAPSIZE * sizeof(ULONGLONG));
			dx->pstartVcn = (ULONGLONG *)(dx->pnextLcn + sizeof(ULONGLONG));
			dx->pmoveFile = (MOVEFILE_DESCRIPTOR *)(dx->pnextLcn + 3 * sizeof(ULONGLONG));
		}
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
	case IOCTL_SET_FRAGLIMIT:
		DebugPrint("-Ultradfg- IOCTL_SET_FRAGLIMIT\n");
		if(in_len != sizeof(ULONGLONG)){
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		dx->fraglimit = *((ULONGLONG *)Irp->AssociatedIrp.SystemBuffer);
		DebugPrint("-Ultradfg- Fragments threshold = %I64u\n",dx->fraglimit);
		BytesTxd = in_len;
		break;
	/* Invalid request */
	default: status = STATUS_INVALID_DEVICE_REQUEST;
	}
	KeSetEvent(&sync_event_2,IO_NO_INCREMENT,FALSE);
	return CompleteIrp(Irp,status,BytesTxd);
}

void stop_all_requests(void)
{
	NTSTATUS Status;
	static KSPIN_LOCK spin_lock;
	static KIRQL oldIrql;

	KeInitializeSpinLock(&spin_lock);
	KeAcquireSpinLock(&spin_lock,&oldIrql);
	KeSetEvent(&stop_event,IO_NO_INCREMENT,FALSE);
	KeSetEvent(&unload_event,IO_NO_INCREMENT,FALSE);
	KeReleaseSpinLock(&spin_lock,oldIrql);

	Status = KeWaitForSingleObject(&sync_event,Executive,KernelMode,FALSE,NULL);
	if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- sync_event waiting failure!\n");
		/* This may cause BSOD... */
	}
	Status = KeWaitForSingleObject(&sync_event_2,Executive,KernelMode,FALSE,NULL);
	if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- sync_event_2 waiting failure!\n");
		/* This may cause BSOD... */
	}
}

/*
* UnloadRoutine: unload the driver and free allocated memory.
* Can be placed in paged memory.
*/
/*
* Neither the PnP manager nor the I/O manager calls Unload routines 
* at system shutdown time. A driver that must perform shutdown 
* processing should register a DispatchShutdown routine.
*/

/*#if !defined(__GNUC__)
#pragma code_seg("PAGE")
#endif
*/
/*PAGED_OUT_FUNCTION*/ VOID NTAPI UnloadRoutine(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pDevice;
	UDEFRAG_DEVICE_EXTENSION *dx;
	UNICODE_STRING us;
	
	/*
	* Here our driver is marked as "Unload Pending".
	* The driver can complete outstanding IRPs, 
	* but the system will not send any new IRPs.
	*/
	DebugPrint("-Ultradfg- In Unload Routine\n");
	
	/* stop all requests */
	stop_all_requests();

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
			FreeAllBuffers(dx);
			if(!nt4_system){
				ExFreePoolSafe(dx->FileMap);
				ExFreePoolSafe(dx->BitMap);
			}
			ExFreePoolSafe(new_cluster_map);
			DestroyFilter(dx);
		}
		/* delete device */
		DebugPrint("-Ultradfg- Deleted device %p\n",dx->fdo);
		IoDeleteDevice(dx->fdo);
	}

	DeregisterBugCheckCallbacks();
	/*KeBugCheck(0);*/
}
/*#if !defined(__GNUC__)
#pragma code_seg() // end PAGE section
#endif
*/
