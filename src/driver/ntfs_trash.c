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
* NTFS related, but not effective code.
*/

#include "driver.h"
#include "ntfs.h"

#if 0

extern UINT MftScanDirection;
PBLOCKMAP MftBlockmap = NULL;
ULONGLONG MftClusters = 0; /* never access this variable if MaxMftEntriesNumberUpdated is not TRUE */

/* all time intervals are invalid due to one fucking bug!? */
/* 128 kb cache - 3 times slower than classical MFT scan on dmitriar's pc, volume L: */
//#define RECORDS_SEQUENCE_LENGTH 128

//#define RECORDS_SEQUENCE_LENGTH 1		/* 5 times slower */
//#define RECORDS_SEQUENCE_LENGTH 2		/* 3.5 times slower */
//#define RECORDS_SEQUENCE_LENGTH 4		/* 2.2 times slower */
//#define RECORDS_SEQUENCE_LENGTH 8		/* 2 times slower */
//#define RECORDS_SEQUENCE_LENGTH 16	/* 2 times slower */
//#define RECORDS_SEQUENCE_LENGTH 32	/* 2 times slower */
//#define RECORDS_SEQUENCE_LENGTH 64	/* 2 times slower */
//#define RECORDS_SEQUENCE_LENGTH 128	/* 2 times slower */
//#define RECORDS_SEQUENCE_LENGTH 256	/* 2 times slower */
//#define RECORDS_SEQUENCE_LENGTH 512	/* 1.5 times slower */
#define RECORDS_SEQUENCE_LENGTH 1024	/* 20% slower */ /* 1.5 times slower with InUse flag check */
//#define RECORDS_SEQUENCE_LENGTH 2048	/* 1.5 times slower */
//#define RECORDS_SEQUENCE_LENGTH 4096	/* 1.5 times slower */

//#define RECORDS_SEQUENCE_LENGTH 32768	/* 1.5 times slower */

/* this code works slow regardless of a scan direction */
void ScanMftDirectly(UDEFRAG_DEVICE_EXTENSION *dx,PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,
	ULONG nfrob_size,PMY_FILE_INFORMATION pmfi)
{
	ScanMftDirectlyLTR(dx,pnfrob,nfrob_size,pmfi);
}

void ScanMftDirectlyRTL(UDEFRAG_DEVICE_EXTENSION *dx,PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,
	ULONG nfrob_size,PMY_FILE_INFORMATION pmfi)
{
	UCHAR *Sequence = NULL; /* memory for sequence of NTFS records */
	ULONG SectorsInSequence;
	PBLOCKMAP block;
	ULONGLONG SectorsInBlockToRead;
	ULONG SectorsInSequenceToRead, N;
	UCHAR *target;
	ULONGLONG lsn;
	NTSTATUS Status;
	signed short i;
	USHORT StartRecord;
	ULONGLONG MftId;
	
	PFILE_RECORD_HEADER pfrh;
	
	MftScanDirection = MFT_SCAN_RTL;

	/* validate length of pnfrob structure */
	if(nfrob_size < (sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + dx->ntfs_record_size - 1))
		return/* STATUS_INVALID_PARAMETER*/;

	/* Apropos, NTFS record size is an integral of sector size here. */
	SectorsInSequence = RECORDS_SEQUENCE_LENGTH * dx->ntfs_record_size / dx->bytes_per_sector;
	MftId = MftClusters * dx->bytes_per_cluster / dx->ntfs_record_size - 1;
	
	/* allocate memory */
	Sequence = (UCHAR *)AllocatePool(PagedPool,RECORDS_SEQUENCE_LENGTH * dx->ntfs_record_size);
	if(Sequence == NULL){
		DebugPrint("-Ultradfg- no enough memory for ScanMftDirectlyRTL()!\n");
		return;
	}
	
	if(MftBlockmap == NULL){
		DebugPrint("-Ultradfg- MftBlockmap invalid?\n");
		ExFreePoolSafe(Sequence);
		return;
	}
	
	/* loop through MftBlockmap */
	SectorsInSequenceToRead = SectorsInSequence;
	for(block = MftBlockmap->prev_ptr; block != NULL; block = block->prev_ptr){
		if(KeReadStateEvent(&stop_event) == 0x1) break;
		SectorsInBlockToRead = block->length * dx->sectors_per_cluster;
		lsn = (block->lcn + block->length) * dx->sectors_per_cluster; /* first sector after the current block */
		while(SectorsInBlockToRead){
			if(KeReadStateEvent(&stop_event) == 0x1) break;
			N = min(SectorsInBlockToRead,SectorsInSequenceToRead);
			target = Sequence + (SectorsInSequenceToRead - N) * dx->bytes_per_sector;
			lsn -= N;
			Status = ReadSectors(dx,lsn,(PVOID)target,N * dx->bytes_per_sector);
			if(!NT_SUCCESS(Status)){
				DebugPrint("-Ultradfg- cannot read the %I64u sector: %x!\n",
					lsn,(UINT)Status);
				ExFreePoolSafe(Sequence);
				return;
			}
			SectorsInBlockToRead -= N;
			SectorsInSequenceToRead -= N;
			if(SectorsInSequenceToRead == 0){
				/* analyse sequence of NTFS records */
				for(i = RECORDS_SEQUENCE_LENGTH - 1; i >= 0; i--){
					if(KeReadStateEvent(&stop_event) == 0x1) break;
					pfrh = (PFILE_RECORD_HEADER)(Sequence + i * dx->ntfs_record_size);
					/* skip free records */
					if(pfrh->Flags & 0x1){
						pnfrob->FileReferenceNumber = MftId;
						pnfrob->FileRecordLength = dx->ntfs_record_size;
						memcpy(pnfrob->FileRecordBuffer,(PVOID)pfrh,dx->ntfs_record_size);
						#ifdef DETAILED_LOGGING
						DebugPrint("-Ultradfg- NTFS record found, id = %I64u\n",pnfrob->FileReferenceNumber);
						#endif
						AnalyseMftRecord(dx,pnfrob,nfrob_size,pmfi);
					}
					if(MftId == 0){
						ExFreePoolSafe(Sequence);
						return;
					}
					MftId --;
				}
				/* reset SectorsInSequenceToRead */
				SectorsInSequenceToRead = SectorsInSequence;
			}
		}
		if(block->prev_ptr == MftBlockmap->prev_ptr) break;
	}

	if(SectorsInSequenceToRead && SectorsInSequenceToRead != SectorsInSequence && KeReadStateEvent(&stop_event) == 0x0){
		/* analyse partially filled sequence */
		StartRecord = SectorsInSequenceToRead * dx->bytes_per_sector / dx->ntfs_record_size;
		for(i = RECORDS_SEQUENCE_LENGTH - 1; i >= StartRecord ; i--){
			if(KeReadStateEvent(&stop_event) == 0x1) break;
			pfrh = (PFILE_RECORD_HEADER)(Sequence + i * dx->ntfs_record_size);
			/* skip free records */
			if(pfrh->Flags & 0x1){
				pnfrob->FileReferenceNumber = MftId;
				pnfrob->FileRecordLength = dx->ntfs_record_size;
				memcpy(pnfrob->FileRecordBuffer,(PVOID)pfrh,dx->ntfs_record_size);
				#ifdef DETAILED_LOGGING
				DebugPrint("-Ultradfg- NTFS record found, id = %I64u\n",pnfrob->FileReferenceNumber);
				#endif
				AnalyseMftRecord(dx,pnfrob,nfrob_size,pmfi);
			}
			if(MftId == 0){
				ExFreePoolSafe(Sequence);
				return;
			}
			MftId --;
		}
	}

	ExFreePoolSafe(Sequence);
}

void ScanMftDirectlyLTR(UDEFRAG_DEVICE_EXTENSION *dx,PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,
	ULONG nfrob_size,PMY_FILE_INFORMATION pmfi)
{
	UCHAR *Sequence = NULL; /* memory for sequence of NTFS records */
	ULONG SectorsInSequence;
	PBLOCKMAP block;
	ULONGLONG SectorsInBlockToRead;
	ULONG SectorsInSequenceToRead, N;
	UCHAR *target;
	ULONGLONG lsn;
	NTSTATUS Status;
	signed short i;
	USHORT EndRecord;
	ULONGLONG MftId;
	
	PFILE_RECORD_HEADER pfrh;
	
	MftScanDirection = MFT_SCAN_LTR;

	/* validate length of pnfrob structure */
	if(nfrob_size < (sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + dx->ntfs_record_size - 1))
		return/* STATUS_INVALID_PARAMETER*/;

	/* Apropos, NTFS record size is an integral of sector size here. */
	SectorsInSequence = RECORDS_SEQUENCE_LENGTH * dx->ntfs_record_size / dx->bytes_per_sector;
	MftId = 0;
	
	/* allocate memory */
	Sequence = (UCHAR *)AllocatePool(PagedPool,RECORDS_SEQUENCE_LENGTH * dx->ntfs_record_size);
	if(Sequence == NULL){
		DebugPrint("-Ultradfg- no enough memory for ScanMftDirectlyLTR()!\n");
		return;
	}
	
	if(MftBlockmap == NULL){
		DebugPrint("-Ultradfg- MftBlockmap invalid?\n");
		ExFreePoolSafe(Sequence);
		return;
	}
	
	/* loop through MftBlockmap */
	SectorsInSequenceToRead = SectorsInSequence;
	for(block = MftBlockmap; block != NULL; block = block->next_ptr){
		if(KeReadStateEvent(&stop_event) == 0x1) break;
		SectorsInBlockToRead = block->length * dx->sectors_per_cluster;
		lsn = block->lcn * dx->sectors_per_cluster; /* first sector of the current block */
		while(SectorsInBlockToRead){
			if(KeReadStateEvent(&stop_event) == 0x1) break;
			N = min(SectorsInBlockToRead,SectorsInSequenceToRead);
			target = Sequence + (SectorsInSequence - SectorsInSequenceToRead) * dx->bytes_per_sector;
			Status = ReadSectors(dx,lsn,(PVOID)target,N * dx->bytes_per_sector);
			if(!NT_SUCCESS(Status)){
				DebugPrint("-Ultradfg- cannot read the %I64u sector: %x!\n",
					lsn,(UINT)Status);
				ExFreePoolSafe(Sequence);
				return;
			}
			lsn += N;
			SectorsInBlockToRead -= N;
			SectorsInSequenceToRead -= N;
			if(SectorsInSequenceToRead == 0){
				/* analyse sequence of NTFS records */
				for(i = 0; i < RECORDS_SEQUENCE_LENGTH; i++){
					if(KeReadStateEvent(&stop_event) == 0x1) break;
					pfrh = (PFILE_RECORD_HEADER)(Sequence + i * dx->ntfs_record_size);
					/* skip free records */
					if(pfrh->Flags & 0x1){
						pnfrob->FileReferenceNumber = MftId;
						pnfrob->FileRecordLength = dx->ntfs_record_size;
						memcpy(pnfrob->FileRecordBuffer,(PVOID)pfrh,dx->ntfs_record_size);
						#ifdef DETAILED_LOGGING
						DebugPrint("-Ultradfg- NTFS record found, id = %I64u\n",pnfrob->FileReferenceNumber);
						#endif
						AnalyseMftRecord(dx,pnfrob,nfrob_size,pmfi);
					}
					if(MftId == (MftClusters * dx->bytes_per_cluster / dx->ntfs_record_size - 1)){
						ExFreePoolSafe(Sequence);
						return;
					}
					MftId ++;
				}
				/* reset SectorsInSequenceToRead */
				SectorsInSequenceToRead = SectorsInSequence;
			}
		}
		if(block->next_ptr == MftBlockmap) break;
	}

	if(SectorsInSequenceToRead && SectorsInSequenceToRead != SectorsInSequence && KeReadStateEvent(&stop_event) == 0x0){
		/* analyse partially filled sequence */
		EndRecord = (SectorsInSequence - SectorsInSequenceToRead - 1) * dx->bytes_per_sector / dx->ntfs_record_size;
		for(i = 0; i <= EndRecord ; i++){
			if(KeReadStateEvent(&stop_event) == 0x1) break;
			pfrh = (PFILE_RECORD_HEADER)(Sequence + i * dx->ntfs_record_size);
			/* skip free records */
			if(pfrh->Flags & 0x1){
				pnfrob->FileReferenceNumber = MftId;
				pnfrob->FileRecordLength = dx->ntfs_record_size;
				memcpy(pnfrob->FileRecordBuffer,(PVOID)pfrh,dx->ntfs_record_size);
				#ifdef DETAILED_LOGGING
				DebugPrint("-Ultradfg- NTFS record found, id = %I64u\n",pnfrob->FileReferenceNumber);
				#endif
				AnalyseMftRecord(dx,pnfrob,nfrob_size,pmfi);
			}
			if(MftId == (MftClusters * dx->bytes_per_cluster / dx->ntfs_record_size - 1)){
				ExFreePoolSafe(Sequence);
				return;
			}
			MftId ++;
		}
	}

	ExFreePoolSafe(Sequence);
}

/* Saves information about VCN/LCN pairs of the $MFT file. */
void ProcessMFTRunList(UDEFRAG_DEVICE_EXTENSION *dx,PNONRESIDENT_ATTRIBUTE pnr_attr)
{
	PUCHAR run;
	ULONGLONG lcn, vcn, length;

	/* loop through runs */
	lcn = 0; vcn = pnr_attr->LowVcn;
	run = (PUCHAR)((char *)pnr_attr + pnr_attr->RunArrayOffset);
	while(*run){
		lcn += RunLCN(run);
		length = RunCount(run);
		
		/* skip virtual runs */
		if(RunLCN(run)){
			/* check for data consistency */
			if((lcn + length) > dx->clusters_total){
				DebugPrint("-Ultradfg- Error in MFT found, run Check Disk program!\n");
				break;
			}
			ProcessMFTRun(dx,vcn,length,lcn);
		}
		
		/* go to the next run */
		run += RunLength(run);
		vcn += length;
	}
}

void ProcessMFTRun(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG vcn,ULONGLONG length,ULONGLONG lcn)
{
	PBLOCKMAP block, prev_block = NULL;
	
	/* add information to MftBlockmap */
	if(MftBlockmap) prev_block = MftBlockmap->prev_ptr;
	block = (PBLOCKMAP)InsertItem((PLIST *)(PVOID)&MftBlockmap,(PLIST)prev_block,sizeof(BLOCKMAP),PagedPool);
	if(!block) return;
	
	block->vcn = vcn;
	block->length = length;
	block->lcn = lcn;
	
	MftClusters += length;
}

void DestroyMftBlockmap(void)
{
	DestroyList((PLIST *)(PVOID)&MftBlockmap);
}

/* works very slow */
NTSTATUS GetMftRecordFromDisk(UDEFRAG_DEVICE_EXTENSION *dx,PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,
					  ULONG nfrob_size,ULONGLONG mft_id)
{
	ULONGLONG vsn; /* virtual sector number */
	PBLOCKMAP block;
	USHORT SectorsToRead, N, K;
	ULONGLONG lsn, FirstSector;
	NTSTATUS Status;
	
	/* validate length of pnfrob structure */
	if(nfrob_size < (sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + dx->ntfs_record_size - 1))
		return STATUS_INVALID_PARAMETER;
	
	//if(dx->ntfs_record_size % dx->bytes_per_sector)
		//return GetMftRecordThroughWinAPI(dx,pnfrob,nfrob_size,mft_id);
	
	/* record size is an integral of sector size */
	SectorsToRead = dx->ntfs_record_size / dx->bytes_per_sector;
	vsn = mft_id * SectorsToRead;
	
	/* search for specified vsn in MftBlockmap */
	K = 0;
	for(block = MftBlockmap; block != NULL; block = block->next_ptr){
		if((vsn >= block->vcn * dx->sectors_per_cluster) && \
		   (vsn < (block->vcn + block->length) * dx->sectors_per_cluster)
		   ){
			/* read sectors from the current run */
			N = min(SectorsToRead,(block->vcn + block->length) * dx->sectors_per_cluster - vsn);
			/* read N sectors to allocated buffer */
			lsn = block->lcn * dx->sectors_per_cluster + (vsn - block->vcn * dx->sectors_per_cluster);
			FirstSector = lsn;
			Status = ReadSectors(dx,FirstSector,(PVOID)(pnfrob->FileRecordBuffer + K),N * dx->bytes_per_sector);
			if(!NT_SUCCESS(Status)){
				DebugPrint("-Ultradfg- cannot read the %I64u sector: %x!\n",
					FirstSector,(UINT)Status);
				return Status;
			}
			/* correct SectorsToRead, vsn and K */
			K += N * dx->bytes_per_sector;
			vsn += N;
			SectorsToRead -= N;
			if(SectorsToRead == 0) break;
		}
		if(block->next_ptr == MftBlockmap) break;
	}
	
	if(SectorsToRead){
		DebugPrint("-Ultradfg- MftBlockmap invalid?\n");
		return STATUS_INVALID_PARAMETER;
	}

	/* fill nfrob structure members */
	pnfrob->FileReferenceNumber = mft_id;
	pnfrob->FileRecordLength = dx->ntfs_record_size;
	
	return STATUS_SUCCESS;
}

/* build the full path after all names has been collected */
void BuildPath(UDEFRAG_DEVICE_EXTENSION *dx,PMY_FILE_INFORMATION pmfi)
{
	WCHAR *buffer1;
	WCHAR *buffer2;
	ULONG offset;
	ULONGLONG mft_id,parent_mft_id;
	ULONG name_length;
	WCHAR header[] = L"\\??\\A:";
	
	#ifdef DETAILED_LOGGING
	DebugPrint("-Ultradfg- parent id = %I64u, FILENAME = %ws\n",
		pmfi->ParentDirectoryMftId,pmfi->Name);
	#endif
	
	pmfi->PathBuilt = TRUE; /* in any case ;-) */
	
	/* return immediately to speed up an analysis :-D */
	return;
		
	if(pmfi->Name[0] == 0) return;
		
	/* allocate memory */
	buffer1 = (WCHAR *)AllocatePool(NonPagedPool,(MAX_NTFS_PATH) * sizeof(short));
	if(!buffer1){
		DebugPrint("-Ultradfg- BuildPath(): cannot allocate memory for buffer1\n");
		return;
	}
	buffer2 = (WCHAR *)AllocatePool(NonPagedPool,(MAX_NTFS_PATH) * sizeof(short));
	if(!buffer2){
		DebugPrint("-Ultradfg- BuildPath(): cannot allocate memory for buffer2\n");
		Nt_ExFreePool(buffer1);
		return;
	}

	/* terminate buffer1 with zero */
	offset = MAX_NTFS_PATH - 1;
	buffer1[offset] = 0; /* terminating zero */
	offset --;
	
	/* copy filename to the right side of buffer1 */
	name_length = wcslen(pmfi->Name);
	if(offset < (name_length - 1)){
		DebugPrint("-Ultradfg- BuildPath(): %ws filename is too long (%u characters)\n",
			pmfi->Name,name_length);
		Nt_ExFreePool(buffer1);
		Nt_ExFreePool(buffer2);
		return;
	}

	offset -= (name_length - 1);
	wcsncpy(buffer1 + offset,pmfi->Name,name_length);

	if(offset == 0) goto path_is_too_long;
	offset --;
	
	/* add backslash */
	buffer1[offset] = '\\';
	offset --;
	
	if(offset == 0) goto path_is_too_long;

	parent_mft_id = pmfi->ParentDirectoryMftId;
	while(parent_mft_id != FILE_root){
		if(KeReadStateEvent(&stop_event) == 0x1) break;
		mft_id = parent_mft_id;
		GetFileNameAndParentMftIdFromMftRecord(dx,mft_id,
			&parent_mft_id,buffer2,MAX_NTFS_PATH);
		if(buffer2[0] == 0){
			DebugPrint("-Ultradfg- BuildPath(): cannot retrieve parent directory name!\n");
			goto build_path_done;
		}
		//DbgPrint("%ws\n",buffer2);
		/* append buffer2 contents to the right side of buffer1 */
		name_length = wcslen(buffer2);
		if(offset < (name_length - 1)) goto path_is_too_long;
		offset -= (name_length - 1);
		wcsncpy(buffer1 + offset,buffer2,name_length);
		if(offset == 0) goto path_is_too_long;
		offset --;
		/* add backslash */
		buffer1[offset] = '\\';
		offset --;
		if(offset == 0) goto path_is_too_long;
	}
	
	/* append volume letter */
	header[4] = dx->letter;
	name_length = wcslen(header);
	if(offset < (name_length - 1)) goto path_is_too_long;
	offset -= (name_length - 1);
	wcsncpy(buffer1 + offset,header,name_length);
	
	/* replace pmfi->Name contents with full path */
	wcsncpy(pmfi->Name,buffer1 + offset,MAX_NTFS_PATH);
	pmfi->Name[MAX_NTFS_PATH - 1] = 0;
	
	#ifdef DETAILED_LOGGING
	DebugPrint("-Ultradfg- FULL PATH = %ws\n",pmfi->Name);
	#endif
	
build_path_done:
	Nt_ExFreePool(buffer1);
	Nt_ExFreePool(buffer2);
	return;
	
path_is_too_long:
	DebugPrint("-Ultradfg- BuildPath(): path is too long: %ws\n",buffer1);
	Nt_ExFreePool(buffer1);
	Nt_ExFreePool(buffer2);
}

void GetFileNameAndParentMftIdFromMftRecord(UDEFRAG_DEVICE_EXTENSION *dx,
		ULONGLONG mft_id,ULONGLONG *parent_mft_id,WCHAR *buffer,ULONG length)
{
	PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob = NULL;
	ULONG nfrob_size;
	NTSTATUS status;
	PFILE_RECORD_HEADER pfrh;
	PMY_FILE_INFORMATION pmfi;

	/* initialize data */
	buffer[0] = 0;
	*parent_mft_id = FILE_root;
	
	/* allocate memory for NTFS record */
	nfrob_size = sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + dx->ntfs_record_size - 1;
	pnfrob = (PNTFS_FILE_RECORD_OUTPUT_BUFFER)AllocatePool(NonPagedPool,nfrob_size);
	if(!pnfrob){
		DebugPrint("-Ultradfg- GetFileNameAndParentMftIdFromMftRecord()\n");
		DebugPrint("-Ultradfg- No enough memory for NTFS_FILE_RECORD_OUTPUT_BUFFER!\n");
		return;
	}

	/* get record */
	status = GetMftRecord(dx,pnfrob,nfrob_size,mft_id);
	if(!NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- GetFileNameAndParentMftIdFromMftRecord()\n");
		DebugPrint("-Ultradfg- FSCTL_GET_NTFS_FILE_RECORD failed: %x!\n",status);
		Nt_ExFreePool(pnfrob);
		return;
	}
	if(GetMftIdFromFRN(pnfrob->FileReferenceNumber) != mft_id){
		DebugPrint("-Ultradfg- GetFileNameAndParentMftIdFromMftRecord()\n");
		DebugPrint("-Ultradfg- Unable to get %I64u record.\n",mft_id);
		Nt_ExFreePool(pnfrob);
		return;
	}

	/* analyse filename attributes */
	pfrh = (PFILE_RECORD_HEADER)pnfrob->FileRecordBuffer;
	if(!(pfrh->Flags & 0x1)){
		DebugPrint("-Ultradfg- GetFileNameAndParentMftIdFromMftRecord()\n");
		DebugPrint("-Ultradfg- %I64u record marked as free.\n",mft_id);
		Nt_ExFreePool(pnfrob);
		return; /* skip free records */
	}

	/* allocate memory for MY_FILE_INFORMATION structure */
	pmfi = (PMY_FILE_INFORMATION)AllocatePool(NonPagedPool,sizeof(MY_FILE_INFORMATION));
	if(!pmfi){
		DebugPrint("-Ultradfg- GetFileNameAndParentMftIdFromMftRecord()\n");
		DebugPrint("-Ultradfg- Cannot allocate memory for MY_FILE_INFORMATION structure.\n");
		Nt_ExFreePool(pnfrob);
		return;
	}
	
	pmfi->BaseMftId = mft_id;
	pmfi->ParentDirectoryMftId = FILE_root;
	pmfi->Flags = 0x0;
	pmfi->IsDirectory = FALSE; /* not important here */
	pmfi->IsReparsePoint = FALSE; /* not important here */
	pmfi->NameType = 0x0; /* Assume FILENAME_POSIX */
	pmfi->PathBuilt = FALSE; /* though not neccessary here :-) */
	memset(pmfi->Name,0,MAX_NTFS_PATH);

	EnumerateAttributes(dx,pfrh,GetFileNameAndParentMftIdFromMftRecordCallback,pmfi);
	
	/* update data */
	*parent_mft_id = pmfi->ParentDirectoryMftId;
	wcsncpy(buffer,pmfi->Name,length);
	buffer[length-1] = 0;
	
	/* free allocated memory */
	Nt_ExFreePool(pnfrob);
	Nt_ExFreePool(pmfi);
}

void __stdcall GetFileNameAndParentMftIdFromMftRecordCallback(UDEFRAG_DEVICE_EXTENSION *dx,
															  PATTRIBUTE pattr,PMY_FILE_INFORMATION pmfi)
{
	if(pattr->AttributeType == AttributeFileName)
		GetFileName(dx,(PRESIDENT_ATTRIBUTE)pattr,pmfi);
}

#endif
