/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

/**
 * @file partition.c
 * @brief Disk partition characteristics analysis code.
 * @addtogroup Partition
 * @{
 */

#include "globals.h"
#include "partition.h"

static int IsFatPartition(BPB *bpb);
static int IsNtfsPartition(BPB *bpb);

/**
 * @brief Retrieves the type of the file system
 * contained on the currently processing volume.
 * @return One of the named constants of the
 * FILE_SYSTEM_TYPE type defined in partition.h
 * @note GetDriveGeometry must preceed this call.
 */
int GetFileSystemType(void)
{
	char *first_sector;
	NTSTATUS status;
	BPB *bpb;
	int part_type;
	
	DebugPrint("File system type detection started.\n");
	
	/* allocate memory for the first sector */
	if(bytes_per_sector < sizeof(BPB)){
		DebugPrint("Disk sectors are too small (%u bytes), so cannot detect FS type!\n",
			bytes_per_sector);
		goto fail;
	}
	first_sector = winx_heap_alloc(bytes_per_sector);
	if(first_sector == NULL){
		DebugPrint("Cannot allocate %u bytes of memory for GetFileSystemType()!\n",
			bytes_per_sector);
		goto fail;
	}
	
	/* read the first sector of the partition */
	status = ReadSectors(0,first_sector,bytes_per_sector);
	if(!NT_SUCCESS(status)){
		DebugPrint("Cannot read the first sector: %x!\n",(UINT)status);
		winx_heap_free(first_sector);
		goto fail;
	}
	
	/* check for FAT */
	bpb = (BPB *)first_sector;
	part_type = IsFatPartition(bpb);
	if(part_type != UNKNOWN_PARTITION){
		winx_heap_free(first_sector);
		return part_type;
	}
	
	/* check for NTFS */
	if(IsNtfsPartition(bpb)){
		winx_heap_free(first_sector);
		return NTFS_PARTITION;
	}
	
	/*
	* We have no need to check for UDF
	* since we have no special code
	* for this file system.
	*/

	/* we have some unrecognized file system */
	DebugPrint("File system type is not recognized.\n");
	DebugPrint("Type independent routines will be used to defragment it.\n");
	winx_heap_free(first_sector);
	return UNKNOWN_PARTITION;

fail:
	DebugPrint("File system type detection failed.\n");
	return UNKNOWN_PARTITION;
}

/**
 * @brief Defines whether bios parameter block
 * belongs to the FAT-formatted partition or not.
 * @return One of the following constants:
 * FAT12_PARTITION, FAT16_PARTITION, FAT32_PARTITION,
 * FAT32_UNRECOGNIZED_PARTITION, UNKNOWN_PARTITION.
 */
static int IsFatPartition(BPB *bpb)
{
	char signature[9];
	BOOL fat_found = FALSE;
	ULONG RootDirSectors; /* USHORT ? */
	ULONG FatSectors;
	ULONG TotalSectors;
	ULONG DataSectors;
	ULONG CountOfClusters;
	ULONG mj, mn;

	/* search for FAT signatures */
	signature[8] = 0;
	memcpy((void *)signature,(void *)bpb->Fat1x.BS_FilSysType,8);
	if(strstr(signature,"FAT")) fat_found = TRUE;
	else {
		memcpy((void *)signature,(void *)bpb->Fat32.BS_FilSysType,8);
		if(strstr(signature,"FAT")) fat_found = TRUE;
	}
	if(!fat_found){
		DebugPrint("FAT signatures were not found in the first sector of the partition.\n");
		return UNKNOWN_PARTITION;
	}

	/* determine which type of FAT we have */
	RootDirSectors = ((bpb->RootDirEnts * 32) + (bpb->BytesPerSec - 1)) / bpb->BytesPerSec;

	if(bpb->FAT16sectors) FatSectors = (ULONG)(bpb->FAT16sectors);
	else FatSectors = bpb->Fat32.FAT32sectors;
	
	if(bpb->FAT16totalsectors) TotalSectors = bpb->FAT16totalsectors;
	else TotalSectors = bpb->FAT32totalsectors;
	
	DataSectors = TotalSectors - (bpb->ReservedSectors + (bpb->NumFATs * FatSectors) + RootDirSectors);
	CountOfClusters = DataSectors / bpb->SecPerCluster;
	
	if(CountOfClusters < 4085) {
		/* Volume is FAT12 */
		DebugPrint("FAT12 partition detected!\n");
		return FAT12_PARTITION;
	} else if(CountOfClusters < 65525) {
		/* Volume is FAT16 */
		DebugPrint("FAT16 partition detected!\n");
		return FAT16_PARTITION;
	} else {
		/* Volume is FAT32 */
		DebugPrint("FAT32 partition detected!\n");
	}
	
	/* check FAT32 version */
	mj = (ULONG)((bpb->Fat32.BPB_FSVer >> 8) & 0xFF);
	mn = (ULONG)(bpb->Fat32.BPB_FSVer & 0xFF);
	if(mj > 0 || mn > 0){
		DebugPrint("Cannot recognize FAT32 version %u.%u!\n",mj,mn);
		/* for safe low level access in future releases */
		return FAT32_UNRECOGNIZED_PARTITION;
	}
	return FAT32_PARTITION;
}

/**
 * @brief Defines whether bios parameter block
 * belongs to NTFS-formatted partition or not.
 * @return Boolean value, 1 indicates success,
 * 0 - failure.
 */
static int IsNtfsPartition(BPB *bpb)
{
	char signature[9];

	/* search for NTFS signature */
	signature[8] = 0;
	memcpy((void *)signature,(void *)bpb->OemName,8);
	if(strcmp(signature,"NTFS    ") == 0){
		DebugPrint("NTFS partition detected!\n");
		return 1;
	}
	
	return 0;
}

/** @} */
