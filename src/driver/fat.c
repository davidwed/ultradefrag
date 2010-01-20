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
ULONG FatEntries;
ULONG FatEOC;

WCHAR LongName[MAX_LONG_PATH]; /* do not use 255 here! */
USHORT LongNameOffset = LONG_PATH_OFFSET_MAX_VALUE;
BOOLEAN LongNameIsOrphan = FALSE;
UCHAR CheckSum = 0x0;

/* updates dx->partition_type member */
void CheckForFatPartition(UDEFRAG_DEVICE_EXTENSION *dx)
{
	NTSTATUS Status;
	char signature[9];
	BOOLEAN fat_found = FALSE;
	
	ULONG RootDirSectors; /* USHORT ? */
	ULONG FatSectors;
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
			BytesPerSector);
		return;
	}
	FirstSector = (UCHAR *)AllocatePool(NonPagedPool,BytesPerSector);
	if(FirstSector == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for the first sector!\n");
		return;
	}
	
	/* read first sector of the partition */
	Status = ReadSectors(dx,0,FirstSector,BytesPerSector);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- cannot read the first sector of the partition: %x!\n",
			(UINT)Status);
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
		DebugPrint("-Ultradfg- FAT signatures not found in the first sector.\n");
		dx->partition_type = UNKNOWN_PARTITION;
		Nt_ExFreePool(FirstSector);
		return;
	}

	/* determine which type of FAT we have */
	RootDirSectors = ((Bpb.RootDirEnts * 32) + (Bpb.BytesPerSec - 1)) / Bpb.BytesPerSec;

	if(Bpb.FAT16sectors) FatSectors = (ULONG)(Bpb.FAT16sectors);
	else FatSectors = Bpb.Fat32.FAT32sectors;
	
	if(Bpb.FAT16totalsectors) TotalSectors = Bpb.FAT16totalsectors;
	else TotalSectors = Bpb.FAT32totalsectors;
	
	DataSectors = TotalSectors - (Bpb.ReservedSectors + (Bpb.NumFATs * FatSectors) + RootDirSectors);
	CountOfClusters = DataSectors / Bpb.SecPerCluster;
	
	if(CountOfClusters < 4085) {
		/* Volume is FAT12 */
		DebugPrint("-Ultradfg- FAT12 partition found!\n");
		dx->partition_type = FAT12_PARTITION;
	} else if(CountOfClusters < 65525) {
		/* Volume is FAT16 */
		DebugPrint("-Ultradfg- FAT16 partition found!\n");
		dx->partition_type = FAT16_PARTITION;
	} else {
		/* Volume is FAT32 */
		DebugPrint("-Ultradfg- FAT32 partition found!\n");
		dx->partition_type = FAT32_PARTITION;
	}
	
	if(dx->partition_type == FAT32_PARTITION){
		/* check FAT32 version */
		mj = (ULONG)((Bpb.Fat32.BPB_FSVer >> 8) & 0xFF);
		mn = (ULONG)(Bpb.Fat32.BPB_FSVer & 0xFF);
		if(mj > 0 || mn > 0){
			DebugPrint("-Ultradfg- cannot recognize FAT32 version %u.%u!\n",
				mj,mn);
			dx->partition_type = UNKNOWN_PARTITION;
			Nt_ExFreePool(FirstSector);
			return;
		}
	}
	
	dx->FatCountOfClusters = CountOfClusters;
	FirstDataSector = Bpb.ReservedSectors + (Bpb.NumFATs * FatSectors) + RootDirSectors;
	FatEntries = (dx->FatCountOfClusters + 1) + 1; /* ? */
	if(dx->partition_type == FAT12_PARTITION) FatEOC = FAT12_EOC;
	else if(dx->partition_type == FAT16_PARTITION) FatEOC = FAT16_EOC;
	else if(dx->partition_type == FAT32_PARTITION) FatEOC = FAT32_EOC;
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
	if(NT_SUCCESS(Status)/* == STATUS_PENDING*/){
		///DebugPrint("-Ultradfg- Is waiting for write to logfile request completion.\n");
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

BOOLEAN ScanFatRootDirectory(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ULONGLONG FirstRootDirSector;
	ULONG RootDirSectors;
	USHORT RootDirEntries;
	DIRENTRY *RootDir = NULL;
	DIRENTRY RootDirEntry;
	NTSTATUS Status;
	USHORT i;
	WCHAR Path[] = L"\\??\\A:\\";
	WCHAR PathForFat32[] = L"\\??\\A:";
	
	/* Initialize LongName related global variables before directories scan. */
	LongNameOffset = LONG_PATH_OFFSET_MAX_VALUE;
	LongNameIsOrphan = FALSE;
	Path[4] = PathForFat32[4] = (WCHAR)(dx->letter);

	/*
	* On FAT32 volumes root directory is 
	* as any other directory.
	*/
	if(dx->partition_type == FAT32_PARTITION){
		memset(&RootDirEntry,0,sizeof(DIRENTRY));
		RootDirEntry.FirstClusterLO = (USHORT)(Bpb.Fat32.BPB_RootClus & 0xFFFF);
		RootDirEntry.FirstClusterHI = (USHORT)(Bpb.Fat32.BPB_RootClus >> 16);
		ScanFatDirectory(dx,&RootDirEntry,PathForFat32,L"");
		return TRUE; /* FIXME: better error handling */
	}
	
	/*
	* On FAT1x volumes the root directory
	* has fixed disposition and size.
	*/
	FirstRootDirSector = Bpb.ReservedSectors + Bpb.NumFATs * Bpb.FAT16sectors;
	RootDirSectors = ((Bpb.RootDirEnts * 32) + (Bpb.BytesPerSec - 1)) / Bpb.BytesPerSec;
	RootDirEntries = Bpb.RootDirEnts;
	
	/* allocate memory */
	RootDir = (DIRENTRY *)AllocatePool(NonPagedPool,RootDirSectors * Bpb.BytesPerSec);
	if(RootDir == NULL){
		DebugPrint("-Ultradfg- cannot allocate %u bytes of memory for RootDirectory!\n",
			(ULONG)(RootDirSectors * Bpb.BytesPerSec));
		return FALSE;
	}
	
	/* read the complete root directory contents */
	Status = ReadSectors(dx,FirstRootDirSector,(PVOID)RootDir,RootDirSectors * Bpb.BytesPerSec);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- cannot read the root directory: %x!\n",(UINT)Status);
		ExFreePoolSafe(RootDir);
		return FALSE;
	}
	
	/* analyse root directory contents */
	for(i = 0; i < RootDirEntries; i++){
		if(KeReadStateEvent(&stop_event) == 0x1) break;
		if(!AnalyseFatDirEntry(dx,&(RootDir[i]),Path)) break;
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
BOOLEAN AnalyseFatDirEntry(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,WCHAR *ParentDirPath)
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
		ProcessFatFile(dx,DirEntry,ParentDirPath,LongNameBuffer);
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
		PartOfLongNameLength = (UCHAR)wcslen(PartOfLongName);
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

void ScanFatDirectory(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,WCHAR *ParentDirPath,WCHAR *DirName)
{
	WCHAR *DirPath = NULL; /* including closing backslash */
	DIRENTRY *Dir = NULL;
	ULONG BytesPerCluster;
	ULONG DirEntriesPerCluster;
	ULONG FirstCluster;
	ULONG ClusterNumber;
	ULONGLONG FirstSectorOfCluster;
	NTSTATUS Status;
	ULONG i;
	
	if(KeReadStateEvent(&stop_event) == 0x1) return;
	
	/* initialize variables */
	BytesPerCluster = Bpb.BytesPerSec * Bpb.SecPerCluster;
	DirEntriesPerCluster = BytesPerCluster / 32; /* let's assume that cluster size is an integral of 32 */
	
	FirstCluster = DirEntry->FirstClusterLO;
	if(dx->partition_type == FAT32_PARTITION){
		FirstCluster += ((ULONG)(DirEntry->FirstClusterHI) << 16);
		FirstCluster &= 0x0FFFFFFF;
	}
	
	/* allocate memory */
	DirPath = (WCHAR *)AllocatePool(PagedPool,MAX_LONG_PATH * sizeof(short));
	if(DirPath == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for DirPath in ScanFat16Directory()!\n");
		return;
	}
	Dir = (DIRENTRY *)AllocatePool(PagedPool,BytesPerCluster);
	if(Dir == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for Dir in ScanFat16Directory()!\n");
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
		if(ClusterNumber > (FatEntries - 1)){ /* directory seems to be invalid */
			DebugPrint("-Ultradfg- invalid cluster number %u in ScanFat16Directory()!\n",
				(ULONG)ClusterNumber);
			ExFreePoolSafe(DirPath);
			ExFreePoolSafe(Dir);
			return;
		}
		FirstSectorOfCluster = (ClusterNumber - 2) * Bpb.SecPerCluster + FirstDataSector;
		Status = ReadSectors(dx,FirstSectorOfCluster,(PVOID)Dir,BytesPerCluster);
		if(!NT_SUCCESS(Status)){
			DebugPrint("-Ultradfg- cannot read the %I64u sector: %x!\n",
				FirstSectorOfCluster,(UINT)Status);
			ExFreePoolSafe(DirPath);
			ExFreePoolSafe(Dir);
			return; /* directory seems to be invalid */
		}
		for(i = 0; i < DirEntriesPerCluster; i++){
			if(!AnalyseFatDirEntry(dx,&(Dir[i]),DirPath)) break;
		}
		/* goto the next cluster in chain */
		ClusterNumber = GetNextClusterInChain(dx,ClusterNumber);
	} while(ClusterNumber < FatEOC);
	
	/* free allocated resources */
	ExFreePoolSafe(DirPath);
	ExFreePoolSafe(Dir);
}

ULONG GetNextClusterInChain(UDEFRAG_DEVICE_EXTENSION *dx,ULONG ClusterNumber)
{
	if(dx->partition_type == FAT12_PARTITION) return GetNextClusterInFat12Chain(dx,ClusterNumber);
	else if(dx->partition_type == FAT16_PARTITION) return GetNextClusterInFat16Chain(dx,ClusterNumber);
	else if(dx->partition_type == FAT32_PARTITION) return GetNextClusterInFat32Chain(dx,ClusterNumber);
	else return FAT32_EOC;
}

/*------------------------ Defragmentation related code ------------------------------*/

/* adds a file to dx->filelist, if it is a directory - scans it */
void ProcessFatFile(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,WCHAR *ParentDirPath,WCHAR *FileName)
{
	WCHAR *Path = NULL;
	BOOLEAN IsDir = FALSE;
	ULONG InsideFlag = TRUE; /* for context menu handler */
	
	//DbgPrint("%ws%ws\n",ParentDirPath,FileName);
	
	/* 1. skip volume label */
	if(DirEntry->Attr & ATTR_VOLUME_ID){
		DebugPrint("-Ultradfg- Volume label = %ws\n",FileName);
		return;
	}
	
	/* 2. decide is current file directory or not */
	if(DirEntry->Attr & ATTR_DIRECTORY) IsDir = TRUE;
	
	/* 3. allocate memory */
	Path = (WCHAR *)AllocatePool(PagedPool,MAX_LONG_PATH * sizeof(short));
	if(Path == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for Path in ProcessFat16File()!\n");
		return;
	}
	
	/* 4. initialize Path variable */
	_snwprintf(Path,MAX_LONG_PATH,L"%s%s",ParentDirPath,FileName);
	Path[MAX_LONG_PATH - 1] = 0;

	//DbgPrint("%ws\n",Path);	

	/* 5. add file to dx->filelist */
	/* 5.1 skip unwanted stuff to speed up an analysis in console/native apps */
	if(ConsoleUnwantedStuffDetected(dx,Path,&InsideFlag)){
		ExFreePoolSafe(Path);
		return;
	}
	
	/* 5.2 if directory found scan it */
	if(IsDir == TRUE){
		ScanFatDirectory(dx,DirEntry,ParentDirPath,FileName);
	} else {
		if(DirEntry->FileSize == 0){
			ExFreePoolSafe(Path);
			return; /* skip empty files */
		}
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
		DebugPrint2("-Ultradfg- no enough memory for pfn->name initialization!\n");
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
	DumpFatFile(dx,DirEntry,pfn);

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
	MarkFileSpace(dx,pfn,SYSTEM_SPACE);

	if(dx->compact_flag || pfn->is_fragm) return TRUE;

	/* 7. Destroy useless data. */
	DeleteBlockmap(pfn);
	RtlFreeUnicodeString(&pfn->name);
	RemoveItem((PLIST *)&dx->filelist,(LIST *)pfn);
	return TRUE;
}

void DumpFatFile(UDEFRAG_DEVICE_EXTENSION *dx,DIRENTRY *DirEntry,PFILENAME pfn)
{
	ULONG ClusterNumber;
	ULONG PrevClusterNumber = FatEOC;
	PBLOCKMAP block, prev_block;

	/* reset special fields of pfn structure */
	pfn->is_fragm = FALSE;
	pfn->n_fragments = 0;
	pfn->clusters_total = 0;
	pfn->blockmap = NULL;

	ClusterNumber = DirEntry->FirstClusterLO;
	if(dx->partition_type == FAT32_PARTITION){
		ClusterNumber += ((ULONG)(DirEntry->FirstClusterHI) << 16);
		ClusterNumber &= 0x0FFFFFFF;
	}

	do {
		if(KeReadStateEvent(&stop_event) == 0x1) goto fail;
		if(ClusterNumber > (FatEntries - 1)){ /* file seems to be invalid */
			DebugPrint("-Ultradfg- invalid cluster number %u in DumpFat16File()!\n",
				(ULONG)ClusterNumber);
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
		ClusterNumber = GetNextClusterInChain(dx,ClusterNumber);
	} while(ClusterNumber < FatEOC);

	if(pfn->n_fragments > 1) pfn->is_fragm = TRUE;
	/* mark some files as filtered out */
	CHECK_FOR_FRAGLIMIT(dx,pfn);
	return;

fail:
	DeleteBlockmap(pfn);
	pfn->clusters_total = pfn->n_fragments = 0;
	pfn->is_fragm = FALSE;
	return;
}

BOOLEAN UnwantedStuffOnFatDetected(UDEFRAG_DEVICE_EXTENSION *dx,WCHAR *Path)
{
	UNICODE_STRING us;

	/* skip all unwanted files by user defined patterns */
	if(!RtlCreateUnicodeString(&us,Path)){
		DebugPrint2("-Ultradfg- cannot allocate memory for UnwantedStuffDetected()!\n");
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
