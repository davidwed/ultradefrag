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
ULONG Fat16Entries;

#define LONG_PATH_OFFSET_MAX_VALUE (MAX_LONG_PATH - 1)
WCHAR LongName[MAX_LONG_PATH]; /* do not use 255 here! */
USHORT LongNameOffset = LONG_PATH_OFFSET_MAX_VALUE;
BOOLEAN LongNameIsOrphan = FALSE;
UCHAR CheckSum = 0x0;

BOOLEAN InitFat16(UDEFRAG_DEVICE_EXTENSION *dx);
BOOLEAN ScanFat16RootDirectory(UDEFRAG_DEVICE_EXTENSION *dx);
BOOLEAN AnalyseFat16DirEntry(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,WCHAR *ParentDirPath);
void ProcessFat16File(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,WCHAR *ParentDirPath,WCHAR *FileName);
void ScanFat16Directory(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,WCHAR *ParentDirPath,WCHAR *DirName);

BOOLEAN ScanFat16Partition(UDEFRAG_DEVICE_EXTENSION *dx)
{
	/* cache file allocation table */
	if(!InitFat16(dx)) return FALSE;
	
	/* scan filesystem */
	ScanFat16RootDirectory(dx);
	
	/* free allocated resources */
	ExFreePoolSafe(Fat16);
	return TRUE;
}

BOOLEAN InitFat16(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ULONGLONG FirstFatSector;
	ULONG FatSize;
	NTSTATUS Status;
	
	/* allocate memory */
	Fat16Entries = (dx->FatCountOfClusters + 1) + 1; /* ? */
	FatSize = (Bpb.FAT16sectors * Bpb.BytesPerSec); /* must be an integral of sector size */
	DebugPrint("-Ultradfg- FAT16 table has %u entries.\n",
		NULL,Fat16Entries);
	if(FatSize < (Fat16Entries * sizeof(short))){
		DebugPrint("-Ultradfg- FatSize (%u) is less than Fat16Entries (%u) * 2!\n",
			NULL,FatSize,Fat16Entries);
		return FALSE;
	}
		
	Fat16 = AllocatePool(NonPagedPool,FatSize);
	if(Fat16 == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for InitFat16()!\n",NULL);
		return FALSE;
	}
	
	/* cache the first File Allocation Table */
	FirstFatSector = 0 + Bpb.ReservedSectors;
	Status = ReadSectors(dx,FirstFatSector,(PVOID)Fat16,FatSize);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- cannot read the first FAT: %x!\n",NULL,(UINT)Status);
		ExFreePoolSafe(Fat16);
		return FALSE;
	}
	return TRUE;
}

BOOLEAN ScanFat16RootDirectory(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ULONGLONG FirstRootDirSector;
	ULONG RootDirSectors;
	USHORT RootDirEntries;
	DIRENTRY *RootDir = NULL;
	NTSTATUS Status;
	USHORT i;
	WCHAR Path[] = L"A:\\";
	
	/* Initialize LongName related global variables before directories scan. */
	LongNameOffset = LONG_PATH_OFFSET_MAX_VALUE;
	LongNameIsOrphan = FALSE;

	/*
	* On FAT16 volumes the root directory
	* has fixed disposition and size.
	*/
	FirstRootDirSector = Bpb.ReservedSectors + Bpb.NumFATs * Bpb.FAT16sectors;
	RootDirSectors = ((Bpb.RootDirEnts * 32) + (Bpb.BytesPerSec - 1)) / Bpb.BytesPerSec;
	RootDirEntries = Bpb.RootDirEnts;
	
	/* allocate memory */
	RootDir = (DIRENTRY *)AllocatePool(NonPagedPool,RootDirSectors * Bpb.BytesPerSec);
	if(RootDir == NULL){
		DebugPrint("-Ultradfg- cannot allocate %u bytes of memory for RootDirectory!\n",
			NULL,(ULONG)(RootDirSectors * Bpb.BytesPerSec));
		return FALSE;
	}
	
	/* read the complete root directory contents */
	Status = ReadSectors(dx,FirstRootDirSector,(PVOID)RootDir,RootDirSectors * Bpb.BytesPerSec);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- cannot read the root directory: %x!\n",NULL,(UINT)Status);
		ExFreePoolSafe(RootDir);
		return FALSE;
	}
	
	/* analyse root directory contents */
	Path[0] = (WCHAR)(dx->letter);
	for(i = 0; i < RootDirEntries; i++){
		if(KeReadStateEvent(&stop_event) == 0x1) break;
		if(!AnalyseFat16DirEntry(dx,&(RootDir[i]),Path)) break;
	}
	
	/* free allocated resources */
	ExFreePoolSafe(RootDir);
	return TRUE;
}

/*
* How to determine - is long entry an orphan or not:
* 1. compare checksum with value calculated from the corresponding short entry.
* 2. orders must run from 1 to N | 0x40
*/

/*
* Test it on '-' file and file with maximum name length.
*/
/* used on recursive basis */
BOOLEAN AnalyseFat16DirEntry(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,WCHAR *ParentDirPath)
{
	UCHAR ShortName[13]; /* 11 + dot + terminal zero */ /* FIXME: names are in OEM code page. */
	UCHAR i, j;
	UCHAR c;
	BOOLEAN extension_exists = FALSE;
	BOOLEAN long_name_exists = FALSE;

	LONGDIRENTRY *LongDirEntry;
	WCHAR PartOfLongName[14]; /* 13 + terminal zero */
	UCHAR PartOfLongNameLength;
	WCHAR wc;
	
	WCHAR LongNameBuffer[MAX_LONG_PATH];
	ANSI_STRING as;
	UNICODE_STRING us;
	
	if(KeReadStateEvent(&stop_event) == 0x1) return FALSE;
	
	if(DirEntry->ShortName[0] == 0x00) return FALSE; /* no more files */
	if(DirEntry->ShortName[0] == 0xE5) return TRUE;  /* skip free structures */
	if(DirEntry->ShortName[0] == '.') return TRUE;  /* skip . and .. entries */
	
	if((DirEntry->Attr & 0x0F) != ATTR_LONG_NAME){
		/* build a short name */
		j = 0;
		for(i = 0; i < 8; i++){
			c = DirEntry->ShortName[i];
			if(c != 0x20){
				ShortName[j] = c;
				j++;
			}
		}
		for(i = 8; i < 11; i++){
			if(DirEntry->ShortName[i] != 0x20){
				extension_exists = TRUE;
				break;
			}
		}
		if(extension_exists){
			ShortName[j] = '.';
			j++;
		}
		for(i = 8; i < 11; i++){
			c = DirEntry->ShortName[i];
			if(c != 0x20){
				ShortName[j] = c;
				j++;
			}
		}
		ShortName[j] = 0; /* terminate string */
		
		if(ShortName[0] == 0x05) ShortName[0] = 0xE5;
		
		/* print filename */
		//DbgPrint("Short name = %s\n",ShortName);
		/* print corresponding long name */
		if(LongNameOffset != LONG_PATH_OFFSET_MAX_VALUE && \
		   LongNameIsOrphan == FALSE && CheckSum == ChkSum(DirEntry->ShortName)){
			//DbgPrint("Long name = %ws\n",LongName + LongNameOffset + 1);
			
			/* save long name */
			wcsncpy(LongNameBuffer,LongName + LongNameOffset + 1,MAX_LONG_PATH);
			LongNameBuffer[MAX_LONG_PATH - 1] = 0;
			long_name_exists = TRUE;
			
			/* reset variables related to LongName */
			LongNameOffset = LONG_PATH_OFFSET_MAX_VALUE;
		}
		
		/* decide here which name to use */
		if(long_name_exists == FALSE){
			/* use short name converted to unicode */
			RtlInitAnsiString(&as,ShortName);
			us.Buffer = LongNameBuffer;
			us.Length = 0;
			us.MaximumLength = MAX_LONG_PATH * sizeof(short);
			RtlAnsiStringToUnicodeString(&us,&as,FALSE); /* always successful ? */
			LongNameBuffer[MAX_LONG_PATH - 1] = 0;
		}
		
		/* here we have file name, attributes, first cluster number, file size */
		ProcessFat16File(dx,DirEntry,ParentDirPath,LongNameBuffer);
	} else {
		/* build a long name */
		LongDirEntry = (LONGDIRENTRY *)DirEntry;
		j = 0;
		for(i = 0; i < 5; i++){
			wc = LongDirEntry->Name1[i];
			if(wc == 0x0000 || wc == 0xFFFF) goto append_term_zero;
			PartOfLongName[j] = wc;
			j++;
		}
		for(i = 0; i < 6; i++){
			wc = LongDirEntry->Name2[i];
			if(wc == 0x0000 || wc == 0xFFFF) goto append_term_zero;
			PartOfLongName[j] = wc;
			j++;
		}
		for(i = 0; i < 2; i++){
			wc = LongDirEntry->Name3[i];
			if(wc == 0x0000 || wc == 0xFFFF) goto append_term_zero;
			PartOfLongName[j] = wc;
			j++;
		}
	append_term_zero:
		PartOfLongName[j] = 0;
		
		/* print part of long name */
		//DbgPrint("Part of long name = %ws\n",PartOfLongName);

		if(LongDirEntry->Order & LAST_LONG_ENTRY_FLAG){
			LongNameIsOrphan = FALSE;
			LongNameOffset = LONG_PATH_OFFSET_MAX_VALUE;
			CheckSum = LongDirEntry->Chksum;
		}
		
		if(LongNameIsOrphan) return TRUE; /* skip orphans */
		
		/* validate this entry */
		if(!(LongDirEntry->Order & LAST_LONG_ENTRY_FLAG) && (LongDirEntry->Chksum != CheckSum)){
			/* this entry has invalid checksum */
			LongNameIsOrphan = TRUE;
			return TRUE;
		}
		/* FIXME: more checks required */
			
		/* append this part of the long name to the right side of LongName buffer */
		if(LongNameOffset == LONG_PATH_OFFSET_MAX_VALUE && LongNameOffset != 0){
			/* insert terminal zero */
			LongName[MAX_LONG_PATH - 1] = 0;
			LongNameOffset--;
		}
		PartOfLongNameLength = wcslen(PartOfLongName);
		if(PartOfLongNameLength > (LongNameOffset + 1) || LongNameOffset == 0){
			/* name is too long, therefore it is an orphan */
			LongNameIsOrphan = TRUE;
			return TRUE;
		}
		LongNameOffset -= (PartOfLongNameLength - 1);
		wcsncpy(LongName + LongNameOffset,PartOfLongName,PartOfLongNameLength);
		if(LongNameOffset > 0) LongNameOffset--;
	}
	
	return TRUE;
}

/* adds a file to dx->filelist, if it is a directory - scans it */
void ProcessFat16File(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,WCHAR *ParentDirPath,WCHAR *FileName)
{
	BOOLEAN IsDir = FALSE;
	
	DbgPrint("%ws%ws\n",ParentDirPath,FileName);
	
	/* skip volume label */
	if(DirEntry->Attr & ATTR_VOLUME_ID){
		DebugPrint("-Ultradfg- Volume label = \n",FileName);
		return;
	}
	
	/* decide is current file directory or not */
	if(DirEntry->Attr & ATTR_DIRECTORY) IsDir = TRUE;
	
	/* add file to dx->filelist */
	
	/* if directory found scan it */
	if(IsDir == FALSE) return;
	ScanFat16Directory(dx,DirEntry,ParentDirPath,FileName);
}

void ScanFat16Directory(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,WCHAR *ParentDirPath,WCHAR *DirName)
{
	WCHAR *DirPath = NULL; /* including closing backslash */
	DIRENTRY *Dir = NULL;
	ULONG BytesPerCluster;
	ULONG DirEntriesPerCluster;
	USHORT FirstCluster;
	USHORT ClusterNumber;
	ULONGLONG FirstSectorOfCluster;
	NTSTATUS Status;
	ULONG i;
	
	if(KeReadStateEvent(&stop_event) == 0x1) return;
	
	/* initialize variables */
	BytesPerCluster = Bpb.BytesPerSec * Bpb.SecPerCluster;
	DirEntriesPerCluster = BytesPerCluster / 32; /* let's assume that cluster size is an integral of 32 */
	FirstCluster = DirEntry->FirstClusterLO;
	
	/* allocate memory */
	DirPath = (WCHAR *)AllocatePool(PagedPool,MAX_LONG_PATH * sizeof(short));
	if(DirPath == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for DirPath in ScanFat16Directory()!\n",NULL);
		return;
	}
	Dir = (DIRENTRY *)AllocatePool(PagedPool,BytesPerCluster);
	if(Dir == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for Dir in ScanFat16Directory()!\n",NULL);
		Nt_ExFreePool(DirPath);
		return;
	}
	
	/* generate DirPath */
	_snwprintf(DirPath,MAX_LONG_PATH,L"%s%s\\",ParentDirPath,DirName);
	DirPath[MAX_LONG_PATH - 1] = 0;
	
	/* read clusters sequentially */
	ClusterNumber = FirstCluster;
	do {
		if(KeReadStateEvent(&stop_event) == 0x1) break;
		if(ClusterNumber > (Fat16Entries - 1)){ /* directory seems to be invalid */
			DebugPrint("-Ultradfg- invalid cluster number %u in ScanFat16Directory()!\n",
				NULL,(ULONG)ClusterNumber);
			ExFreePoolSafe(DirPath);
			ExFreePoolSafe(Dir);
			return;
		}
		FirstSectorOfCluster = (ClusterNumber - 2) * Bpb.SecPerCluster + FirstDataSector;
		Status = ReadSectors(dx,FirstSectorOfCluster,(PVOID)Dir,BytesPerCluster);
		if(!NT_SUCCESS(Status)){
			DebugPrint("-Ultradfg- cannot read the %I64u sector: %x!\n",
				NULL,FirstSectorOfCluster,(UINT)Status);
			ExFreePoolSafe(DirPath);
			ExFreePoolSafe(Dir);
			return; /* directory seems to be invalid */
		}
		for(i = 0; i < DirEntriesPerCluster; i++){
			if(!AnalyseFat16DirEntry(dx,&(Dir[i]),DirPath)) break;
		}
		/* goto the next cluster in chain */
		ClusterNumber = Fat16[ClusterNumber];
	} while(ClusterNumber < FAT16_EOC);
	
	/* free allocated resources */
	ExFreePoolSafe(DirPath);
	ExFreePoolSafe(Dir);
}
