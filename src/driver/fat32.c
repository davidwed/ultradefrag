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

/*
* Strategy with reading by one sector is very slow.
* Therefore we should cache all FAT or return with failure.
*/

/*
* Strategy of caching the complete FAT is very slow too.
*/

ULONG *Fat32Sector = NULL;
ULONGLONG CurrentSector = 0;

//#define COMPLETE_CACHING

#ifdef COMPLETE_CACHING
ULONG *Fat32 = NULL;
#endif

BOOLEAN InitFat32(UDEFRAG_DEVICE_EXTENSION *dx);

/*----------------- FAT32 related code ----------------------*/

BOOLEAN ScanFat32Partition(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ULONGLONG tm;
	
	DebugPrint("-Ultradfg- FAT32 scan started!\n");
	tm = _rdtsc();

	/* cache the first sector of file allocation table */
	if(!InitFat32(dx)){
		DebugPrint("-Ultradfg- FAT32 scan finished!\n");
		return FALSE;
	}
	
	/* scan filesystem */
	ScanFatRootDirectory(dx);
	
	/* free allocated resources */
	ExFreePoolSafe(Fat32Sector);
#ifdef COMPLETE_CACHING
	ExFreePoolSafe(Fat32);
#endif
	DebugPrint("-Ultradfg- FAT32 scan finished!\n");
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
	DebugPrint("-Ultradfg- FAT32 table has %u entries.\n",FatEntries);
	if(FatSize < (FatEntries * sizeof(ULONG))){
		DebugPrint("-Ultradfg- FatSize (%u) is less than FatEntries (%u) * 4!\n",
			FatSize,FatEntries);
		return FALSE;
	}
		
	Fat32Sector = AllocatePool(NonPagedPool,dx->bytes_per_sector);
	if(Fat32Sector == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for InitFat32()!\n");
		return FALSE;
	}
	
	/* cache the first sector of the first File Allocation Table */
	CurrentSector = 0 + Bpb.ReservedSectors;
	FirstFatSector = 0 + Bpb.ReservedSectors;
	Status = ReadSectors(dx,FirstFatSector,(PVOID)Fat32Sector,dx->bytes_per_sector);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- cannot read the first sector of the first FAT: %x!\n",(UINT)Status);
		ExFreePoolSafe(Fat32Sector);
		return FALSE;
	}
	
#ifdef COMPLETE_CACHING
	/*
	* allocate memory for complete FAT 
	* only if it requires no more than 4 Mb
	*/
	if(FatSize > (4 * 1024 * 1024)){
		DebugPrint("-Ultradfg- FAT is too long: %u bytes!\n",FatSize);
		DebugPrint("-Ultradfg- Specific scan cannot be performed!\n");
		ExFreePoolSafe(Fat32Sector);
		return FALSE;
	}
	
	Fat32 = AllocatePool(PagedPool,FatSize);
	if(Fat32 == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for InitFat32()!\n");
		ExFreePoolSafe(Fat32Sector);
		return FALSE;
	}

	/* cache the first File Allocation Table entirely */
	FirstFatSector = 0 + Bpb.ReservedSectors;
	Status = ReadSectors(dx,FirstFatSector,(PVOID)Fat32,FatSize);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- cannot read the first FAT: %x!\n",(UINT)Status);
		ExFreePoolSafe(Fat32Sector);
		ExFreePoolSafe(Fat32);
		return FALSE;
	}
#endif
	
	return TRUE;
}

ULONG GetNextClusterInFat32Chain(UDEFRAG_DEVICE_EXTENSION *dx,ULONG ClusterNumber)
{
#ifndef COMPLETE_CACHING
	ULONG SectorNumber;
	ULONG Offset;
	NTSTATUS Status;
#endif

	if(ClusterNumber > (FatEntries - 1)) return FAT32_EOC;

#ifndef COMPLETE_CACHING
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
#else
	return (Fat32[ClusterNumber] & 0x0FFFFFFF);
#endif
}
