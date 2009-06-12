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

/*
* FIXME: 
* 1. Volume ditry flag?
* 2. Using second FAT if the first one has invalid data.
*/

BPB Bpb;
ULONGLONG FirstDataSector;

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
	
	UCHAR *FirstSector;
	ULONG BytesPerSector;
	
	ULONG mj, mn;
	
	/* allocate memory */
	BytesPerSector = dx->bytes_per_sector;
	if(BytesPerSector < sizeof(BPB)){
		DebugPrint("-Ultradfg- Sector size is too small: %u bytes!\n",
			NULL,BytesPerSector);
		return;
	}
	FirstSector = (UCHAR *)AllocatePool(NonPagedPool,BytesPerSector);
	if(FirstSector == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for the first sector!\n",NULL);
		return;
	}
	
	/* read first sector of the partition */
	Status = ReadSectors(dx,0,FirstSector,BytesPerSector);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- cannot read the first sector of the partition: %x!\n",
			NULL,(UINT)Status);
		dx->partition_type = UNKNOWN_PARTITION;
		Nt_ExFreePool(FirstSector);
		return;
	}
	memcpy((void *)&Bpb,(void *)FirstSector,sizeof(BPB));

	/* search for FAT signatures */
	signature[8] = 0;
	memcpy((void *)signature,(const void *)Bpb.Fat1x.BS_FilSysType,8);
	if(strstr(signature,"FAT")) fat_found = TRUE;
	else {
		memcpy((void *)signature,(const void *)Bpb.Fat32.BS_FilSysType,8);
		if(strstr(signature,"FAT")) fat_found = TRUE;
	}
	if(!fat_found){
		DebugPrint("-Ultradfg- FAT signatures not found in the first sector.\n",NULL);
		dx->partition_type = UNKNOWN_PARTITION;
		Nt_ExFreePool(FirstSector);
		return;
	}

	/* determine which type of FAT we have */
	RootDirSectors = ((Bpb.RootDirEnts * 32) + (Bpb.BytesPerSec - 1)) / Bpb.BytesPerSec;

	if(Bpb.FAT16sectors) FatSectors = Bpb.FAT16sectors;
	else FatSectors = Bpb.Fat32.FAT32sectors;
	
	if(Bpb.FAT16totalsectors) TotalSectors = Bpb.FAT16totalsectors;
	else TotalSectors = Bpb.FAT32totalsectors;
	
	DataSectors = TotalSectors - (Bpb.ReservedSectors + (Bpb.NumFATs * FatSectors) + RootDirSectors);
	CountOfClusters = DataSectors / Bpb.SecPerCluster;
	
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
	
	if(dx->partition_type == FAT32_PARTITION){
		/* check FAT32 version */
		mj = (ULONG)((Bpb.Fat32.BPB_FSVer >> 8) & 0xFF);
		mn = (ULONG)(Bpb.Fat32.BPB_FSVer & 0xFF);
		if(mj > 0 || mn > 0){
			DebugPrint("-Ultradfg- cannot recognize FAT32 version %u.%u!\n",
				NULL,mj,mn);
			dx->partition_type = UNKNOWN_PARTITION;
			Nt_ExFreePool(FirstSector);
			return;
		}
	}
	
	dx->FatCountOfClusters = CountOfClusters;
	FirstDataSector = Bpb.ReservedSectors + (Bpb.NumFATs * FatSectors) + RootDirSectors;
	Nt_ExFreePool(FirstSector);
}

/* LSN, buffer, length must be valid before this call! */
/* Length MUST BE an integral of sector size */
NTSTATUS ReadSectors(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG lsn,PVOID buffer,ULONG length)
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
	/* FIXME: number of bytes actually read check? */
	return Status;
}

//-----------------------------------------------------------------------------
//	ChkSum()
//	Returns an unsigned byte checksum computed on an unsigned byte
//	array.  The array must be 11 bytes long and is assumed to contain
//	a name stored in the format of a MS-DOS directory entry.
//	Passed:	 pFcbName    Pointer to an unsigned byte array assumed to be
//                       11 bytes long.
//	Returns: Sum         An 8-bit unsigned checksum of the array pointed
//                       to by pFcbName.
//------------------------------------------------------------------------------
unsigned char ChkSum (unsigned char *pFcbName)
{
	short FcbNameLen;
	unsigned char Sum;

	Sum = 0;
	for (FcbNameLen=11; FcbNameLen!=0; FcbNameLen--) {
		// NOTE: The operation is an unsigned char rotate right
		Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *pFcbName++;
	}
	return (Sum);
}
