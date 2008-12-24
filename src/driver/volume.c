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
	NTSTATUS status = STATUS_SUCCESS;
	PARTITION_INFORMATION part_info;

	/* open volume */
	path[4] = (short)(dx->letter);
	RtlInitUnicodeString(&us,path);
	InitializeObjectAttributes(&ObjectAttributes,&us,0,NULL,NULL);
	status = ZwCreateFile(&dx->hVol,FILE_GENERIC_READ | FILE_WRITE_DATA,
				&ObjectAttributes,&iosb,
				NULL,0,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,
				NULL,0);
	if(status != STATUS_SUCCESS){
		DebugPrint("-Ultradfg- Can't open volume %x\n",NULL,(UINT)status);
		dx->hVol = NULL;
		goto done;
	}
	/* try to get fs type */
	status = ZwDeviceIoControlFile(dx->hVol,NULL,NULL,NULL,&iosb, \
				IOCTL_DISK_GET_PARTITION_INFO,NULL,0, \
				&part_info, sizeof(PARTITION_INFORMATION));
	if(status == STATUS_PENDING){
		NtWaitForSingleObject(dx->hVol,FALSE,NULL);
		status = iosb.Status;
	}
	if(!NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- Can't get fs info: %x!\n",NULL,(UINT)status);
		if(status == STATUS_INVALID_DEVICE_REQUEST){
			/* this is usual case for floppies */
			dx->partition_type = FLOPPY_FAT12_PARTITION;
			status = STATUS_SUCCESS;
			goto done;
		}
		/* FIXME: maybe more flexible error handling can be applied */
		ZwCloseSafe(dx->hVol);
		//status = STATUS_WRONG_VOLUME;
		goto done;
	}
	dx->partition_type = part_info.PartitionType;
	DebugPrint("-Ultradfg- possible partition type: %u\n",NULL,dx->partition_type);
done:
	return status;
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
	InitializeObjectAttributes(&ObjectAttributes,&us,
			       FILE_READ_ATTRIBUTES,NULL,NULL);
	status = ZwCreateFile(&hFile,FILE_GENERIC_READ,
				&ObjectAttributes,&iosb,NULL,0,
				FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,
				NULL,0);
	if(status != STATUS_SUCCESS){
		DebugPrint("-Ultradfg- Can't open the root directory: %x!",
				path,(UINT)status);
		hFile = NULL;
		return status;
	}

	/* get logical geometry */
	status = ZwQueryVolumeInformationFile(hFile,&iosb,&FileFsSize,
			  sizeof(FILE_FS_SIZE_INFORMATION),FileFsSizeInformation);
	ZwClose(hFile);
	if(status != STATUS_SUCCESS) return status;

	bpc = FileFsSize.SectorsPerAllocationUnit * FileFsSize.BytesPerSector;
	dx->bytes_per_cluster = bpc;
	dx->total_space = FileFsSize.TotalAllocationUnits.QuadPart * bpc;
	dx->free_space = FileFsSize.AvailableAllocationUnits.QuadPart * bpc;
	dx->clusters_total = (ULONGLONG)(FileFsSize.TotalAllocationUnits.QuadPart);
	dx->clusters_per_256k = _256K / dx->bytes_per_cluster;
	if(!dx->clusters_per_256k) dx->clusters_per_256k ++;
	DebugPrint("-Ultradfg- total clusters: %I64u\n",NULL, dx->clusters_total);
	DebugPrint("-Ultradfg- cluster size: %I64u\n",NULL, dx->bytes_per_cluster);
	
	/* validate geometry */
	if(!dx->clusters_total || !dx->total_space || !dx->bytes_per_cluster)
		return STATUS_WRONG_VOLUME;
	return STATUS_SUCCESS;
}

void ProcessMFT(UDEFRAG_DEVICE_EXTENSION *dx)
{
	IO_STATUS_BLOCK iosb;
	NTFS_DATA ntfs_data;
	ULONGLONG start,len,mft_len = 0;
	NTSTATUS status;

	status = ZwFsControlFile(dx->hVol,NULL,NULL,NULL,&iosb, \
				FSCTL_GET_NTFS_VOLUME_DATA,NULL,0, \
				&ntfs_data, sizeof(NTFS_DATA));
	if(status == STATUS_PENDING){
		NtWaitForSingleObject(dx->hVol,FALSE,NULL);
		status = iosb.Status;
	}
	if(!NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- Can't get ntfs info: %x!\n",NULL,status);
		return;
	}
		
	/* 
	* Not increment dx->processed_clusters here, 
	* because some parts of MFT are really free.
	*/
	DebugPrint("-Ultradfg- MFT_file   : start : length\n",NULL);
	/* $MFT */
	start = ntfs_data.MftStartLcn.QuadPart;
	if(ntfs_data.BytesPerCluster)
		len = ntfs_data.MftValidDataLength.QuadPart / ntfs_data.BytesPerCluster;
	else
		len = 0;
	DebugPrint("-Ultradfg- $MFT       :%I64u :%I64u\n",NULL,start,len);
	//dx->processed_clusters += len;
	ProcessBlock(dx,start,len,MFT_SPACE,SYSTEM_SPACE);
	mft_len += len;
	/* $MFT2 */
	start = ntfs_data.MftZoneStart.QuadPart;
	len = ntfs_data.MftZoneEnd.QuadPart - ntfs_data.MftZoneStart.QuadPart;
	DebugPrint("-Ultradfg- $MFT2      :%I64u :%I64u\n",NULL,start,len);
	//dx->processed_clusters += len;
	ProcessBlock(dx,start,len,MFT_SPACE,SYSTEM_SPACE);
	mft_len += len;
	/* $MFTMirror */
	start = ntfs_data.Mft2StartLcn.QuadPart;
	DebugPrint("-Ultradfg- $MFTMirror :%I64u :1\n",NULL,start);
	ProcessBlock(dx,start,1,MFT_SPACE,SYSTEM_SPACE);
	//dx->processed_clusters ++;
	mft_len ++;
	dx->mft_size = (ULONG)(mft_len * dx->bytes_per_cluster);
}

void CloseVolume(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ZwCloseSafe(dx->hVol);
}
