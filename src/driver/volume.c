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
* Volume information getting.
*/

#include "driver.h"

/* dx->hVol should be zero before this call */
NTSTATUS OpenVolume(UDEFRAG_DEVICE_EXTENSION *dx)
{
	short path[] = L"\\??\\A:";
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK iosb;
	UNICODE_STRING us;
	NTSTATUS status;

	/* open volume */
	path[4] = (short)(dx->letter);
	RtlInitUnicodeString(&us,path);
	if(nt4_system){
		InitializeObjectAttributes(&ObjectAttributes,&us,0,NULL,NULL);
	} else {
		InitializeObjectAttributes(&ObjectAttributes,&us,OBJ_KERNEL_HANDLE,NULL,NULL);
	}
	status = ZwCreateFile(&dx->hVol,FILE_GENERIC_READ | FILE_WRITE_DATA,
				&ObjectAttributes,&iosb,
				NULL,0,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,
				NULL,0);
	if(status != STATUS_SUCCESS){
		DebugPrint("-Ultradfg- Can't open volume %x\n",NULL,(UINT)status);
		dx->hVol = NULL;
		return status;
	}

	CheckForNtfsPartition(dx);
	return STATUS_SUCCESS;
}

NTSTATUS GetVolumeInfo(UDEFRAG_DEVICE_EXTENSION *dx)
{
	unsigned short path[] = L"\\??\\A:\\";
	FILE_FS_SIZE_INFORMATION FileFsSize;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK iosb;
	UNICODE_STRING us;
	HANDLE hFile;
	NTSTATUS status = STATUS_SUCCESS;
	ULONGLONG bpc; /* bytes per cluster */

	/* open the volume */
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
		DebugPrint("-Ultradfg- Can't open the root directory: %x!\n",
			path,(UINT)status);
		hFile = NULL;
		return status;
	}

	/* get logical geometry */
	status = ZwQueryVolumeInformationFile(hFile,&iosb,&FileFsSize,
			  sizeof(FILE_FS_SIZE_INFORMATION),FileFsSizeInformation);
	ZwClose(hFile);
	if(status != STATUS_SUCCESS){
		DebugPrint("-Ultradfg- FileFsSizeInformation() request failed: %x!\n",
			path,(UINT)status);
		return status;
	}

	bpc = FileFsSize.SectorsPerAllocationUnit * FileFsSize.BytesPerSector;
	dx->bytes_per_cluster = bpc;
	dx->total_space = FileFsSize.TotalAllocationUnits.QuadPart * bpc;
	dx->free_space = FileFsSize.AvailableAllocationUnits.QuadPart * bpc;
	dx->clusters_total = (ULONGLONG)(FileFsSize.TotalAllocationUnits.QuadPart);
	dx->clusters_per_256k = _256K / dx->bytes_per_cluster;
	DebugPrint("-Ultradfg- total clusters: %I64u\n",NULL, dx->clusters_total);
	DebugPrint("-Ultradfg- cluster size: %I64u\n",NULL, dx->bytes_per_cluster);
	if(!dx->clusters_per_256k){
		DebugPrint("-Ultradfg- clusters are larger than 256 kbytes!\n",NULL);
		dx->clusters_per_256k ++;
	}
	
	/* validate geometry */
	if(!dx->clusters_total || !dx->bytes_per_cluster){
		DebugPrint("-Ultradfg- wrong volume geometry!\n",NULL);
		return STATUS_WRONG_VOLUME;
	}
	return STATUS_SUCCESS;
}

void CloseVolume(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ZwCloseSafe(dx->hVol);
}
