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
	NTFS_DATA ntfs_data;

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

	/*
	* Try to get fs type through IOCTL_DISK_GET_PARTITION_INFO request.
	* Works only on MBR-formatted disks. To retrieve information about 
	* GPT-formatted disks use IOCTL_DISK_GET_PARTITION_INFO_EX.
	*/
	status = ZwDeviceIoControlFile(dx->hVol,NULL,NULL,NULL,&iosb, \
				IOCTL_DISK_GET_PARTITION_INFO,NULL,0, \
				&part_info, sizeof(PARTITION_INFORMATION));
	if(status == STATUS_PENDING){
		if(nt4_system)
			NtWaitForSingleObject(dx->hVol,FALSE,NULL);
		else
			ZwWaitForSingleObject(dx->hVol,FALSE,NULL);
		status = iosb.Status;
	}
	if(NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- partition type: %u\n",NULL,part_info.PartitionType);
		if(part_info.PartitionType == 0x7){
			DebugPrint("-Ultradfg- NTFS found\n",NULL);
			dx->partition_type = NTFS_PARTITION;
			return STATUS_SUCCESS;
		}
	} else {
		DebugPrint("-Ultradfg- Can't get fs info: %x!\n",NULL,(UINT)status);
	}

	/*
	* To ensure that we have a NTFS formatted partition
	* on GPT disks and when partition type is 0x27
	* FSCTL_GET_NTFS_VOLUME_DATA request can be used.
	*/
	status = ZwFsControlFile(dx->hVol,NULL,NULL,NULL,&iosb, \
				FSCTL_GET_NTFS_VOLUME_DATA,NULL,0, \
				&ntfs_data, sizeof(NTFS_DATA));
	if(status == STATUS_PENDING){
		if(nt4_system)
			NtWaitForSingleObject(dx->hVol,FALSE,NULL);
		else
			ZwWaitForSingleObject(dx->hVol,FALSE,NULL);
		status = iosb.Status;
	}
	if(NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- NTFS found\n",NULL);
		dx->partition_type = NTFS_PARTITION;
		return STATUS_SUCCESS;
	} else {
		DebugPrint("-Ultradfg- Can't get ntfs info: %x!\n",NULL,status);
	}

	/*
	* Let's assume that we have a FAT32 formatted partition.
	* Currently we don't need more detailed information.
	*/
	dx->partition_type = FAT32_PARTITION;
	DebugPrint("-Ultradfg- NTFS not found\n",NULL);
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
		if(nt4_system)
			NtWaitForSingleObject(dx->hVol,FALSE,NULL);
		else
			ZwWaitForSingleObject(dx->hVol,FALSE,NULL);
		status = iosb.Status;
	}
	if(!NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- Can't get ntfs info: %x!\n",NULL,status);
		return;
	}

	/*
	* MFT space must be excluded from the free space list.
	* Because Windows 2000 disallows to move files there.
	* And because on other systems this dirty technique 
	* causes MFT fragmentation.
	*/
	
	/* 
	* Don't increment dx->processed_clusters here, 
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
	ProcessBlock(dx,start,len,MFT_SPACE,SYSTEM_SPACE);
	CleanupFreeSpaceList(dx,start,len);
	mft_len += len;
	/* $MFT2 */
	start = ntfs_data.MftZoneStart.QuadPart;
	len = ntfs_data.MftZoneEnd.QuadPart - ntfs_data.MftZoneStart.QuadPart;
	DebugPrint("-Ultradfg- $MFT2      :%I64u :%I64u\n",NULL,start,len);
	ProcessBlock(dx,start,len,MFT_SPACE,SYSTEM_SPACE);
	CleanupFreeSpaceList(dx,start,len);
	mft_len += len;
	/* $MFTMirror */
	start = ntfs_data.Mft2StartLcn.QuadPart;
	DebugPrint("-Ultradfg- $MFTMirror :%I64u :1\n",NULL,start);
	ProcessBlock(dx,start,1,MFT_SPACE,SYSTEM_SPACE);
	CleanupFreeSpaceList(dx,start,1);
	mft_len ++;
	dx->mft_size = (ULONG)(mft_len * dx->bytes_per_cluster);
	
	DbgPrintFreeSpaceList(dx);
}

void CloseVolume(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ZwCloseSafe(dx->hVol);
}
