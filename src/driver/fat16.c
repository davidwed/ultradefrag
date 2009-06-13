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
BOOLEAN InsertFileToFileList(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,WCHAR *Path);
BOOLEAN UnwantedStuffOnFatDetected(UDEFRAG_DEVICE_EXTENSION *dx,WCHAR *Path);
void DumpFat16File(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,PFILENAME pfn);

/*----------------- FAT16 related code ----------------------*/

BOOLEAN ScanFat16Partition(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ULONGLONG tm;
	
	DebugPrint("-Ultradfg- FAT16 scan started!\n",NULL);
	tm = _rdtsc();

	/* cache file allocation table */
	if(!InitFat16(dx)){
		DebugPrint("-Ultradfg- FAT16 scan finished!\n",NULL);
		return FALSE;
	}
	
	/* scan filesystem */
	ScanFat16RootDirectory(dx);
	
	/* free allocated resources */
	ExFreePoolSafe(Fat16);
	DebugPrint("-Ultradfg- FAT16 scan finished!\n",NULL);
	DbgPrint("FAT16 scan needs %I64u ms\n",_rdtsc() - tm);
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
	WCHAR Path[] = L"\\??\\A:\\";
	
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
	Path[4] = (WCHAR)(dx->letter);
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

/*------------------------ Defragmentation related code ------------------------------*/

/* adds a file to dx->filelist, if it is a directory - scans it */
void ProcessFat16File(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,WCHAR *ParentDirPath,WCHAR *FileName)
{
	WCHAR *Path = NULL;
	BOOLEAN IsDir = FALSE;
	ULONG InsideFlag = TRUE; /* for context menu handler */
	
	//DbgPrint("%ws%ws\n",ParentDirPath,FileName);
	
	/* 1. skip volume label */
	if(DirEntry->Attr & ATTR_VOLUME_ID){
		DebugPrint("-Ultradfg- Volume label = \n",FileName);
		return;
	}
	
	/* 2. decide is current file directory or not */
	if(DirEntry->Attr & ATTR_DIRECTORY) IsDir = TRUE;
	
	/* 3. allocate memory */
	Path = (WCHAR *)AllocatePool(PagedPool,MAX_LONG_PATH * sizeof(short));
	if(Path == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for Path in ProcessFat16File()!\n",NULL);
		return;
	}
	
	/* 4. initialize Path variable */
	_snwprintf(Path,MAX_LONG_PATH,L"%s%s",ParentDirPath,FileName);
	Path[MAX_LONG_PATH - 1] = 0;

	
	/* 5. add file to dx->filelist */
	/* 5.1 skip unwanted stuff to speed up an analysis in console/native apps */
	if(ConsoleUnwantedStuffDetected(dx,Path,&InsideFlag)){
		ExFreePoolSafe(Path);
		return;
	}
	
	/* 5.2 if directory found scan it */
	if(IsDir == TRUE)
		ScanFat16Directory(dx,DirEntry,ParentDirPath,FileName);
	else if(DirEntry->FileSize == 0){
		ExFreePoolSafe(Path);
		return; /* skip empty files */
	}
	
	/* 5.3 skip parent directories in context menu handler */
	if(context_menu_handler && !InsideFlag){
		ExFreePoolSafe(Path);
		return; /* skip empty files */
	}
	
	/* 5.4 insert pfn structure */
	InsertFileToFileList(dx,DirEntry,Path);

	/* 6. free allocated resources */
	ExFreePoolSafe(Path);
}

BOOLEAN InsertFileToFileList(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,WCHAR *Path)
{
	PFILENAME pfn;
	ULONGLONG filesize;

	/* Add a file only if we need to have its information cached. */
	pfn = (PFILENAME)InsertItem((PLIST *)&dx->filelist,NULL,sizeof(FILENAME),PagedPool);
	if(pfn == NULL) return FALSE;
	
	/* Initialize pfn->name field. */
	if(!RtlCreateUnicodeString(&pfn->name,Path)){
		DebugPrint2("-Ultradfg- no enough memory for pfn->name initialization!\n",NULL);
		RemoveItem((PLIST *)&dx->filelist,(LIST *)pfn);
		return FALSE;
	}

	/* Set flags. */
	if(DirEntry->Attr & ATTR_DIRECTORY) pfn->is_dir = TRUE;
	else pfn->is_dir = FALSE;
	pfn->is_compressed = pfn->is_reparse_point = FALSE;

	filesize = (ULONGLONG)(DirEntry->FileSize);
	if(dx->sizelimit && filesize > dx->sizelimit) pfn->is_overlimit = TRUE;
	else pfn->is_overlimit = FALSE;

	if(UnwantedStuffOnFatDetected(dx,Path)) pfn->is_filtered = TRUE;
	else pfn->is_filtered = FALSE;

	/* Dump the file. */
	DumpFat16File(dx,DirEntry,pfn);

	/* Update statistics and cluster map. */
	dx->filecounter ++;
	if(pfn->is_dir) dx->dircounter ++;
	/* skip here filtered out and big files */
	if(pfn->is_fragm && !pfn->is_filtered && !pfn->is_overlimit){
		dx->fragmfilecounter ++;
		dx->fragmcounter += pfn->n_fragments;
	} else {
		dx->fragmcounter ++;
	}
	dx->processed_clusters += pfn->clusters_total;
	MarkSpace(dx,pfn,SYSTEM_SPACE);

	if(dx->compact_flag || pfn->is_fragm) return TRUE;

	/* 7. Destroy useless data. */
	DeleteBlockmap(pfn);
	RtlFreeUnicodeString(&pfn->name);
	RemoveItem((PLIST *)&dx->filelist,(LIST *)pfn);
	return TRUE;
}

BOOLEAN UnwantedStuffOnFatDetected(UDEFRAG_DEVICE_EXTENSION *dx,WCHAR *Path)
{
	UNICODE_STRING us;

	/* skip all unwanted files by user defined patterns */
	if(!RtlCreateUnicodeString(&us,Path)){
		DebugPrint2("-Ultradfg- cannot allocate memory for UnwantedStuffDetected()!\n",NULL);
		return FALSE;
	}
	_wcslwr(us.Buffer);

	if(dx->in_filter.buffer){
		if(!IsStringInFilter(us.Buffer,&dx->in_filter)){
			RtlFreeUnicodeString(&us); return TRUE; /* not included */
		}
	}

	if(dx->ex_filter.buffer){
		if(IsStringInFilter(us.Buffer,&dx->ex_filter)){
			RtlFreeUnicodeString(&us); return TRUE; /* excluded */
		}
	}
	RtlFreeUnicodeString(&us);

	return FALSE;
}

void DumpFat16File(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,PFILENAME pfn)
{
	USHORT ClusterNumber;
	USHORT PrevClusterNumber = FAT16_EOC;
	PBLOCKMAP block, prev_block;

	/* reset special fields of pfn structure */
	pfn->is_fragm = FALSE;
	pfn->n_fragments = 0;
	pfn->clusters_total = 0;
	pfn->blockmap = NULL;

	ClusterNumber = DirEntry->FirstClusterLO;
	do {
		if(KeReadStateEvent(&stop_event) == 0x1) goto fail;
		if(ClusterNumber > (Fat16Entries - 1)){ /* file seems to be invalid */
			DebugPrint("-Ultradfg- invalid cluster number %u in DumpFat16File()!\n",
				NULL,(ULONG)ClusterNumber);
			goto fail;
		}
		/* add cluster to blockmap */
		if(ClusterNumber == (PrevClusterNumber + 1) && pfn->blockmap){
			pfn->blockmap->prev_ptr->length++;
			pfn->clusters_total++;
		} else {
			if(pfn->blockmap) prev_block = pfn->blockmap->prev_ptr;
			else prev_block = NULL;
			block = (PBLOCKMAP)InsertItem((PLIST *)&pfn->blockmap,(PLIST)prev_block,sizeof(BLOCKMAP),PagedPool);
			if(!block) goto fail;
			pfn->n_fragments++;
			block->lcn = ClusterNumber;
			block->length = 1;
			block->vcn = pfn->clusters_total;
			pfn->clusters_total++;
		}
		PrevClusterNumber = ClusterNumber;
		/* goto the next cluster in chain */
		ClusterNumber = Fat16[ClusterNumber];
	} while(ClusterNumber < FAT16_EOC);

	if(pfn->n_fragments > 1) pfn->is_fragm = TRUE;
	return;

fail:
	DeleteBlockmap(pfn);
	pfn->clusters_total = pfn->n_fragments = 0;
	pfn->is_fragm = FALSE;
	return;
}
