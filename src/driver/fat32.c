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
* FAT32 specific code.
*/

#include "driver.h"
#include "fat.h"

/*
* 1. FAT contains 4 bytes long records 
*    for each cluster on the volume.
* 2. FAT32 volume may contain a lot of clusters.
*
* Therefore, FAT may be many megabytes long. We cannot allocate
* this memory once. We will read it on a one sector basis for fast access.
*/

ULONG *Fat32Sector = NULL;
ULONGLONG CurrentSector = 0;


BOOLEAN InitFat32(UDEFRAG_DEVICE_EXTENSION *dx);

/*----------------- FAT32 related code ----------------------*/

BOOLEAN ScanFat32Partition(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ULONGLONG tm;
	
	DebugPrint("-Ultradfg- FAT32 scan started!\n",NULL);
	tm = _rdtsc();

	/* cache the first sector of file allocation table */
	if(!InitFat32(dx)){
		DebugPrint("-Ultradfg- FAT32 scan finished!\n",NULL);
		return FALSE;
	}
	
	/* scan filesystem */
	ScanFatRootDirectory(dx);
	
	/* free allocated resources */
	ExFreePoolSafe(Fat32Sector);
	DebugPrint("-Ultradfg- FAT32 scan finished!\n",NULL);
	DbgPrint("FAT32 scan needs %I64u ms\n",_rdtsc() - tm);
	return TRUE;
}

BOOLEAN InitFat32(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ULONGLONG FirstFatSector;
	ULONG FatSize;
	NTSTATUS Status;
	
	/* allocate memory for one sector */
	FatSize = (Bpb.Fat32.FAT32sectors * Bpb.BytesPerSec); /* must be an integral of sector size */
	DebugPrint("-Ultradfg- FAT32 table has %u entries.\n",
		NULL,FatEntries);
	if(FatSize < (FatEntries * sizeof(ULONG))){
		DebugPrint("-Ultradfg- FatSize (%u) is less than FatEntries (%u) * 4!\n",
			NULL,FatSize,FatEntries);
		return FALSE;
	}
		
	Fat32Sector = AllocatePool(NonPagedPool,dx->bytes_per_sector);
	if(Fat32Sector == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for InitFat32()!\n",NULL);
		return FALSE;
	}
	
	/* cache the first sector of the first File Allocation Table */
	CurrentSector = 0 + Bpb.ReservedSectors;
	FirstFatSector = 0 + Bpb.ReservedSectors;
	Status = ReadSectors(dx,FirstFatSector,(PVOID)Fat32Sector,dx->bytes_per_sector);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- cannot read the first sector of the first FAT: %x!\n",NULL,(UINT)Status);
		ExFreePoolSafe(Fat32Sector);
		return FALSE;
	}
	return TRUE;
}

ULONG GetNextClusterInFat32Chain(UDEFRAG_DEVICE_EXTENSION *dx,ULONG ClusterNumber)
{
	ULONG SectorNumber;
	ULONG Offset;
	NTSTATUS Status;
	
	if(ClusterNumber > (FatEntries - 1)) return FAT32_EOC;
	
	/* FIXME: sizeof(ULONG) on x64 system ??? */
	SectorNumber = Bpb.ReservedSectors + (ClusterNumber * sizeof(ULONG) / dx->bytes_per_sector);
	Offset = (ClusterNumber * sizeof(ULONG)) % dx->bytes_per_sector;
	Offset = Offset / sizeof(ULONG); /* bytes -> double words */

	if(SectorNumber != CurrentSector){
		Status = ReadSectors(dx,SectorNumber,(PVOID)Fat32Sector,dx->bytes_per_sector);
		if(!NT_SUCCESS(Status)) return FAT32_EOC;
		CurrentSector = SectorNumber;
	}
	
	return (Fat32Sector[Offset] & 0x0FFFFFFF);
}
