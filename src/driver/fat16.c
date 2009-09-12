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
* FAT16 specific code.
*/

#include "driver.h"
#include "fat.h"

/*
* 1. FAT contains 2 bytes long records 
*    for each cluster on the volume.
* 2. FAT16 volume may contain no more than 65536 clusters.
*
* Therefore, maximum FAT size is 131072 bytes. We can allocate
* this memory once and read complete FAT into it for fast access.
*/

USHORT *Fat16 = NULL;

BOOLEAN InitFat16(UDEFRAG_DEVICE_EXTENSION *dx);

/*----------------- FAT16 related code ----------------------*/

BOOLEAN ScanFat16Partition(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ULONGLONG tm;
	
	DebugPrint("-Ultradfg- FAT16 scan started!\n");
	tm = _rdtsc();

	/* cache file allocation table */
	if(!InitFat16(dx)){
		DebugPrint("-Ultradfg- FAT16 scan finished!\n");
		return FALSE;
	}
	
	/* scan filesystem */
	ScanFatRootDirectory(dx);
	
	/* free allocated resources */
	ExFreePoolSafe(Fat16);
	DebugPrint("-Ultradfg- FAT16 scan finished!\n");
	DbgPrint("FAT16 scan needs %I64u ms\n",_rdtsc() - tm);
	return TRUE;
}

BOOLEAN InitFat16(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ULONGLONG FirstFatSector;
	ULONG FatSize;
	NTSTATUS Status;
	
	/* allocate memory */
	FatSize = (Bpb.FAT16sectors * Bpb.BytesPerSec); /* must be an integral of sector size */
	DebugPrint("-Ultradfg- FAT16 table has %u entries.\n",FatEntries);
	if(FatSize < (FatEntries * sizeof(short))){
		DebugPrint("-Ultradfg- FatSize (%u) is less than FatEntries (%u) * 2!\n",
			FatSize,FatEntries);
		return FALSE;
	}
		
	Fat16 = AllocatePool(NonPagedPool,FatSize);
	if(Fat16 == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for InitFat16()!\n");
		return FALSE;
	}
	
	/* cache the first File Allocation Table */
	FirstFatSector = 0 + Bpb.ReservedSectors;
	Status = ReadSectors(dx,FirstFatSector,(PVOID)Fat16,FatSize);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- cannot read the first FAT: %x!\n",(UINT)Status);
		ExFreePoolSafe(Fat16);
		return FALSE;
	}
	return TRUE;
}

ULONG GetNextClusterInFat16Chain(UDEFRAG_DEVICE_EXTENSION *dx,ULONG ClusterNumber)
{
	return (ULONG)(Fat16[ClusterNumber]);
}
