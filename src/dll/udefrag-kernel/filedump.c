/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

/**
 * @file filedump.c
 * @brief File cluster chains dumping code.
 * @addtogroup FileDump
 * @{
 */

#include "globals.h"

/**
 * @brief Opens a file.
 * @param[in] pfn pointer to the FILENAME structure
 * containing information about the file.
 * @param[out] phFile pointer to a variable receiving
 * the handle of the opened file.
 * @return An appropriate NTSTATUS code.
 */
NTSTATUS OpenTheFile(PFILENAME pfn,HANDLE *phFile)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK ioStatus;
	ULONG flags = FILE_SYNCHRONOUS_IO_NONALERT;

	if(!pfn || !phFile) return STATUS_INVALID_PARAMETER;
	
	if(pfn->is_dir) flags |= FILE_OPEN_FOR_BACKUP_INTENT;
	else flags |= FILE_NO_INTERMEDIATE_BUFFERING;
	
	InitializeObjectAttributes(&ObjectAttributes,&pfn->name,0,NULL,NULL);
	return NtCreateFile(phFile,FILE_GENERIC_READ | SYNCHRONIZE,&ObjectAttributes,&ioStatus,
			  NULL,0,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,flags,
			  NULL,0);
}

/**
 * @brief Dumps a cluster chains of the file.
 * @param[in] pfn pointer to the FILENAME structure
 * containing information about the file.
 * @return Boolean value indicating the file dumping
 * result: TRUE for success, FALSE otherwise.
 * @note
 * - On NTFS volumes all files smaller then 1 kb are placed in MFT.
 *   We are excluding them from the defragmentation process.
 * - On NTFS we are skipping the 0-filled virtual clusters
 *   of the compressed files.
 * - This function must never modify global statistical counters.
 */
BOOLEAN DumpFile(PFILENAME pfn)
{
	ULONGLONG *FileMap;
	IO_STATUS_BLOCK ioStatus;
	PGET_RETRIEVAL_DESCRIPTOR fileMappings;
	NTSTATUS Status;
	HANDLE hFile;
	int i;
	long counter;
	#define MAX_COUNTER 1000
	PBLOCKMAP block = NULL;
	ULONGLONG startVcn;

	/* Data initialization */
	pfn->clusters_total = pfn->n_fragments = 0;
	pfn->is_fragm = FALSE;
	pfn->blockmap = NULL;

	/* Open the file */
	Status = OpenTheFile(pfn,&hFile);
	if(Status != STATUS_SUCCESS){
		DebugPrint("System file found: %ws: %x\n",pfn->name.Buffer,(UINT)Status);
		return FALSE; /* File has unknown state! */
	}
	
	/* allocate memory */
	FileMap = winx_heap_alloc(FILEMAPSIZE * sizeof(ULONGLONG));
	if(FileMap == NULL){
		DebugPrint("Cannot allocate memory for DumpFile()!\n");
		return FALSE;
	}

	/* Start dumping the mapping information. Go until we hit the end of the file. */
	startVcn = 0;
	fileMappings = (PGET_RETRIEVAL_DESCRIPTOR)(FileMap);
	counter = 0;
	do {
		RtlZeroMemory(fileMappings,FILEMAPSIZE * sizeof(ULONGLONG));
		Status = NtFsControlFile(hFile, NULL, NULL, 0, &ioStatus, \
						FSCTL_GET_RETRIEVAL_POINTERS, \
						&startVcn, sizeof(ULONGLONG), \
						fileMappings, FILEMAPSIZE * sizeof(ULONGLONG));
		counter ++;
		if(NT_SUCCESS(Status)/* == STATUS_PENDING*/){
			NtWaitForSingleObject(hFile,FALSE,NULL);
			Status = ioStatus.Status;
		}
		if(Status != STATUS_SUCCESS && Status != STATUS_BUFFER_OVERFLOW){
			/* it always returns STATUS_END_OF_FILE for small files placed in MFT */
			if(Status != STATUS_END_OF_FILE)
				DebugPrint("Dump failed for %ws: %x\n",pfn->name.Buffer,(UINT)Status);
			goto dump_fail;
		}

		/* user must have a chance to break infinite loops */
		if(CheckForStopEvent()){
			if(counter > MAX_COUNTER)
				DebugPrint("Infinite main loop? %ws\n",pfn->name.Buffer);
			goto dump_fail;
		}

		if(!fileMappings->NumberOfPairs && Status != STATUS_SUCCESS){
			DebugPrint("Empty map of file %ws\n",pfn->name.Buffer);
			goto dump_fail;
		}
		
		/* Loop through the buffer of number/cluster pairs. */
		startVcn = fileMappings->StartVcn;
		for(i = 0; i < (ULONGLONG) fileMappings->NumberOfPairs;	i++){
			/* Infinite loop here will cause BSOD (in kernel mode driver only, of course). */

			/*
			* A compressed virtual run (0-filled) is
			* identified with a cluster offset of -1.
			*/
			if(fileMappings->Pair[i].Lcn == LLINVALID) goto next_run;
			
			/* The following code may cause an infinite main loop (bug #2053941?), */
			/* but for some 3.99 Gb files on FAT32 it works fine. */
			if(fileMappings->Pair[i].Vcn == 0){
				DebugPrint("Wrong map of file %ws\n",pfn->name.Buffer);
				goto next_run;
			}
			
			block = (PBLOCKMAP)winx_list_insert_item((list_entry **)&pfn->blockmap,(list_entry *)block,sizeof(BLOCKMAP));
			if(!block) goto dump_fail;
			block->lcn = fileMappings->Pair[i].Lcn;
			block->length = fileMappings->Pair[i].Vcn - startVcn;
			block->vcn = startVcn;

			if(!wcscmp(pfn->name.Buffer,L"\\??\\L:\\go.zip")){
				DebugPrint("VCN %I64u : LEN %I64u : LCN %I64u\n",block->vcn,block->length,block->lcn);
			}

			pfn->clusters_total += block->length;
			pfn->n_fragments ++;
			/*
			* Sometimes normal file has more than one fragment, 
			* but is not fragmented yet! 8-) 
			*/
			if(block != pfn->blockmap && \
			  block->lcn != (block->prev_ptr->lcn + block->prev_ptr->length))
				pfn->is_fragm = TRUE;
		next_run:
			startVcn = fileMappings->Pair[i].Vcn;
		}
	} while(Status != STATUS_SUCCESS);

	/* Skip small directories placed in MFT (tested on 32-bit XP system). */
	if(pfn->blockmap){
		NtClose(hFile);
		winx_heap_free(FileMap);
		return TRUE; /* success */
	}

dump_fail:
	DeleteBlockmap(pfn);
	pfn->clusters_total = pfn->n_fragments = 0;
	pfn->is_fragm = FALSE;
	NtClose(hFile);
	winx_heap_free(FileMap);
	return FALSE;
}

/** @} */
