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
* Volume information related code.
*/

#include "driver.h"

NTSTATUS GetVolumeGeometry(UDEFRAG_DEVICE_EXTENSION *dx);

/* dx->hVol should be zero before this call */
NTSTATUS OpenVolume(UDEFRAG_DEVICE_EXTENSION *dx)
{
	short path[] = L"\\??\\A:";
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK iosb;
	UNICODE_STRING us;
	NTSTATUS status;

	/* open volume */
	CloseVolume(dx);
	path[4] = (short)(dx->letter);
	RtlInitUnicodeString(&us,path);
	if(nt4_system){
		InitializeObjectAttributes(&ObjectAttributes,&us,0,NULL,NULL);
	} else {
		InitializeObjectAttributes(&ObjectAttributes,&us,OBJ_KERNEL_HANDLE,NULL,NULL);
	}
	status = ZwCreateFile(&dx->hVol,FILE_GENERIC_READ | FILE_WRITE_DATA | SYNCHRONIZE,
				&ObjectAttributes,&iosb,
				NULL,0,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,FILE_SYNCHRONOUS_IO_NONALERT,
				NULL,0);
	if(status != STATUS_SUCCESS){
		DebugPrint("-Ultradfg- Can't open volume %x\n",(UINT)status);
		dx->hVol = NULL;
		return status;
	}

	status = GetVolumeGeometry(dx);
	if(!NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- GetVolumeGeometry() failed: %x!\n",(UINT)status);
		return status;
	}

	CheckForNtfsPartition(dx);
	if(dx->partition_type != NTFS_PARTITION)
		CheckForFatPartition(dx);

	return STATUS_SUCCESS;
}

NTSTATUS GetVolumeGeometry(UDEFRAG_DEVICE_EXTENSION *dx)
{
	unsigned short path[] = L"\\??\\A:\\";
	FILE_FS_SIZE_INFORMATION *pFileFsSize;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK iosb;
	UNICODE_STRING us;
	HANDLE hFile;
	NTSTATUS status = STATUS_SUCCESS;
	ULONGLONG bpc; /* bytes per cluster */

	pFileFsSize = AllocatePool(NonPagedPool,sizeof(FILE_FS_SIZE_INFORMATION));
	if(!pFileFsSize){
		DebugPrint("-Ultradfg- GetDriveGeometry(): no enough memory!");
		return STATUS_NO_MEMORY;
	}
	
	/* open the root directory */
	path[4] = (unsigned short)dx->letter;
	RtlInitUnicodeString(&us,path);
	if(nt4_system){
		InitializeObjectAttributes(&ObjectAttributes,&us,0,NULL,NULL);
	} else {
		InitializeObjectAttributes(&ObjectAttributes,&us,OBJ_KERNEL_HANDLE,NULL,NULL);
	}
	status = ZwCreateFile(&hFile,FILE_GENERIC_READ | FILE_READ_ATTRIBUTES,
				&ObjectAttributes,&iosb,NULL,0,
				FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,
				NULL,0);
	if(status != STATUS_SUCCESS){
		DebugPrint("-Ultradfg- Can't open the root directory %ws: %x!\n",
			path,(UINT)status);
		hFile = NULL;
		Nt_ExFreePool(pFileFsSize);
		return status;
	}

	/* get logical geometry */
	RtlZeroMemory((void *)pFileFsSize,sizeof(FILE_FS_SIZE_INFORMATION));
	status = ZwQueryVolumeInformationFile(hFile,&iosb,pFileFsSize,
			  sizeof(FILE_FS_SIZE_INFORMATION),FileFsSizeInformation);
	ZwClose(hFile);
	if(status != STATUS_SUCCESS){
		DebugPrint("-Ultradfg- FileFsSizeInformation() request failed for %ws: %x!\n",
			path,(UINT)status);
		Nt_ExFreePool(pFileFsSize);
		return status;
	}

	bpc = pFileFsSize->SectorsPerAllocationUnit * pFileFsSize->BytesPerSector;
	dx->sectors_per_cluster = pFileFsSize->SectorsPerAllocationUnit;
	dx->bytes_per_cluster = bpc;
	dx->bytes_per_sector = pFileFsSize->BytesPerSector;
	dx->total_space = pFileFsSize->TotalAllocationUnits.QuadPart * bpc;
	dx->free_space = pFileFsSize->AvailableAllocationUnits.QuadPart * bpc;
	dx->clusters_total = (ULONGLONG)(pFileFsSize->TotalAllocationUnits.QuadPart);
	if(dx->bytes_per_cluster)
		dx->clusters_per_256k = _256K / dx->bytes_per_cluster;
	DebugPrint("-Ultradfg- total clusters: %I64u\n",dx->clusters_total);
	DebugPrint("-Ultradfg- cluster size: %I64u\n",dx->bytes_per_cluster);
	if(!dx->clusters_per_256k){
		DebugPrint("-Ultradfg- clusters are larger than 256 kbytes!\n");
		dx->clusters_per_256k ++;
	}
	
	/* validate geometry */
	if(!dx->clusters_total || !dx->bytes_per_cluster){
		DebugPrint("-Ultradfg- wrong volume geometry!\n");
		Nt_ExFreePool(pFileFsSize);
		return STATUS_WRONG_VOLUME;
	}
	Nt_ExFreePool(pFileFsSize);
	return STATUS_SUCCESS;
}

void CloseVolume(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ZwCloseSafe(dx->hVol);
}
