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
* FAT related common procedures.
*/

#include "driver.h"
#include "fat.h"

BPB FirstSector;

/* updates dx->partition_type member */
void CheckForFatPartition(UDEFRAG_DEVICE_EXTENSION *dx)
{
	NTSTATUS Status;
	char signature[9];
	BOOLEAN fat_found = FALSE;
	
	ULONG RootDirSectors; /* USHORT ? */
	USHORT FatSectors;
	ULONG TotalSectors;
	ULONG DataSectors;
	ULONG CountOfClusters;
	
	
	/* read first 512 bytes of the partition */
	Status = ReadSector(dx,0,&FirstSector,sizeof(BPB));
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- cannot read the first sector of the partition!\n",NULL);
		dx->partition_type = UNKNOWN_PARTITION;
		return;
	}

	/* search for FAT signatures */
	signature[8] = 0;
	memcpy((void *)signature,(const void *)FirstSector.Fat16.BS_FilSysType,8);
	if(strstr(signature,"FAT")) fat_found = TRUE;
	else {
		memcpy((void *)signature,(const void *)FirstSector.Fat32.BS_FilSysType,8);
		if(strstr(signature,"FAT")) fat_found = TRUE;
	}
	if(!fat_found){
		DebugPrint("-Ultradfg- FAT signatures not found in the first sector.\n",NULL);
		dx->partition_type = UNKNOWN_PARTITION;
		return;
	}

	/* determine which type of FAT we have */
	RootDirSectors = ((FirstSector.RootDirEnts * 32) + (FirstSector.BytesPerSec - 1)) / FirstSector.BytesPerSec;

	if(FirstSector.FAT16sectors) FatSectors = FirstSector.FAT16sectors;
	else FatSectors = FirstSector.Fat32.FAT32sectors;
	
	if(FirstSector.FAT16totalsectors) TotalSectors = FirstSector.FAT16totalsectors;
	else TotalSectors = FirstSector.FAT32totalsectors;
	
	DataSectors = TotalSectors - (FirstSector.ReservedSectors + (FirstSector.NumFATs * FatSectors) + RootDirSectors);
	CountOfClusters = DataSectors / FirstSector.SecPerCluster;
	
	if(CountOfClusters < 4085) {
		/* Volume is FAT12 */
		DebugPrint("-Ultradfg- FAT12 partition found!\n",NULL);
		dx->partition_type = FAT12_PARTITION;
	} else if(CountOfClusters < 65525) {
		/* Volume is FAT16 */
		DebugPrint("-Ultradfg- FAT16 partition found!\n",NULL);
		dx->partition_type = FAT16_PARTITION;
	} else {
		/* Volume is FAT32 */
		DebugPrint("-Ultradfg- FAT32 partition found!\n",NULL);
		dx->partition_type = FAT32_PARTITION;
	}
}

/* LSN, buffer, length must be valid before this call! */
NTSTATUS ReadSector(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG lsn,PVOID buffer,ULONG length)
{
	IO_STATUS_BLOCK ioStatus;
	LARGE_INTEGER offset;
	NTSTATUS Status;

	offset.QuadPart = lsn * dx->bytes_per_sector;
	Status = ZwReadFile(dx->hVol,NULL,NULL,NULL,&ioStatus,buffer,length,&offset,NULL);
	if(Status == STATUS_PENDING){
		///DebugPrint("-Ultradfg- Is waiting for write to logfile request completion.\n",NULL);
		if(nt4_system)
			Status = NtWaitForSingleObject(dx->hVol,FALSE,NULL);
		else
			Status = ZwWaitForSingleObject(dx->hVol,FALSE,NULL);
		if(NT_SUCCESS(Status)) Status = ioStatus.Status;
	}
	return Status;
}
