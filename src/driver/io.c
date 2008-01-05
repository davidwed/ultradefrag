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

/* io.c - low level I/O functions */

#include "driver.h"

#ifndef NT4_TARGET

/*
 * Synchronize drive.
 * On NT 4.0 (at least under MS Virtual PC) it will crash system.
 */
NTSTATUS NTAPI __NtFlushBuffersFile(HANDLE FileHandle)
{
	PFILE_OBJECT FileObject = NULL;
	PIRP Irp;
	PIO_STACK_LOCATION StackPtr;
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_OBJECT DeviceObject;
	KEVENT Event;
	IO_STATUS_BLOCK IoStatusBlock;

	/* Get the File Object */
	Status = ObReferenceObjectByHandle(FileHandle,0,NULL,KernelMode,
					   (PVOID*)(void *)&FileObject,NULL);
	if(Status != STATUS_SUCCESS) return(Status);

	/* Check if this is a direct open or not */
	DebugPrint("Vol. flags = %x\n",(UINT)FileObject->Flags);
	if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
		DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
	else
		DeviceObject = IoGetRelatedDeviceObject(FileObject);

	KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

	/* Allocate the IRP */
	if(!(Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE)))
	{
		ObDereferenceObject(FileObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	/* Set up the IRP */
	Irp->Flags = IRP_SYNCHRONOUS_API;
	Irp->RequestorMode = KernelMode;
	Irp->UserIosb = &IoStatusBlock;
	Irp->UserEvent = &Event;
	Irp->Tail.Overlay.Thread = PsGetCurrentThread();
	Irp->Tail.Overlay.OriginalFileObject = FileObject;

	/* Set up Stack Data */
	StackPtr = IoGetNextIrpStackLocation(Irp);
	StackPtr->MajorFunction = IRP_MJ_FLUSH_BUFFERS;
	StackPtr->FileObject = FileObject;

	/* Call the Driver */
	Status = IoCallDriver(DeviceObject, Irp);
	if(Status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
		Status = IoStatusBlock.Status;
	}
	/* NO DEREFERENCE FileObject HERE! */
	return Status;
}

#else /* NT4_TARGET */

NTSTATUS NTAPI __NtFlushBuffersFile(HANDLE FileHandle)
{
	return 0;
}

#endif
