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
* FAT12 specific code.
*/

#include "driver.h"
#include "fat.h"

/*
* 1. FAT contains 1.5 bytes long records 
*    for each cluster on the volume.
* 2. FAT12 volume may contain no more than 4096 clusters.
*
* Therefore, maximum FAT size is 6144 bytes. We can allocate
* this memory once and read complete FAT into it for fast access.
*/

/*
* Works slower for a floppy drive.
*/

UCHAR *Fat12 = NULL;

BOOLEAN InitFat12(UDEFRAG_DEVICE_EXTENSION *dx);

/*----------------- FAT12 related code ----------------------*/

BOOLEAN ScanFat12Partition(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ULONGLONG tm;
	
	DebugPrint("-Ultradfg- FAT12 scan started!\n",NULL);
	tm = _rdtsc();

	/* cache file allocation table */
	if(!InitFat12(dx)){
		DebugPrint("-Ultradfg- FAT12 scan finished!\n",NULL);
		return FALSE;
	}
	
	/* scan filesystem */
	ScanFat1xRootDirectory(dx);
	
	/* free allocated resources */
	ExFreePoolSafe(Fat12);
	DebugPrint("-Ultradfg- FAT12 scan finished!\n",NULL);
	DbgPrint("FAT12 scan needs %I64u ms\n",_rdtsc() - tm);
	return TRUE;
}

BOOLEAN InitFat12(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ULONGLONG FirstFatSector;
	ULONG FatSize;
	NTSTATUS Status;
	
	/* allocate memory */
	FatEntries = (dx->FatCountOfClusters + 1) + 1; /* ? */
	FatSize = (Bpb.FAT16sectors * Bpb.BytesPerSec); /* must be an integral of sector size */
	DebugPrint("-Ultradfg- FAT12 table has %u entries.\n",
		NULL,FatEntries);
	if(((FatEntries % 2) == 0) && (FatSize < (FatEntries + FatEntries / 2))){
		DebugPrint("-Ultradfg- FatSize (%u) is less than FatEntries (%u) * 1.5!\n",
			NULL,FatSize,FatEntries);
		return FALSE;
	}
	if(((FatEntries % 2) == 1) && (FatSize < (FatEntries + FatEntries / 2 + 1))){
		DebugPrint("-Ultradfg- FatSize (%u) is less than FatEntries (%u) * 1.5!\n",
			NULL,FatSize,FatEntries);
		return FALSE;
	}
		
	Fat12 = AllocatePool(NonPagedPool,FatSize);
	if(Fat12 == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for InitFat12()!\n",NULL);
		return FALSE;
	}
	
	/* cache the first File Allocation Table */
	FirstFatSector = 0 + Bpb.ReservedSectors;
	Status = ReadSectors(dx,FirstFatSector,(PVOID)Fat12,FatSize);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- cannot read the first FAT: %x!\n",NULL,(UINT)Status);
		ExFreePoolSafe(Fat12);
		return FALSE;
	}
	return TRUE;
}

ULONG GetNextClusterInFat12Chain(UDEFRAG_DEVICE_EXTENSION *dx,ULONG ClusterNumber)
{
	ULONG FatSize;
	USHORT w;
	USHORT i;

	FatSize = (Bpb.FAT16sectors * Bpb.BytesPerSec);	
	i = (USHORT)(ClusterNumber + ClusterNumber / 2);

	if(i > (FatSize - sizeof(USHORT))) return FAT12_EOC;
	
	w = *((USHORT *)(Fat12 + i));
	if(ClusterNumber & 0x1) w >>= 4;
	else w &= 0x0FFF;
	
	return (ULONG)w;
}
