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
* Functions for file dump manipulations.
*/

#include "driver.h"

/*
* NOTES: 
* 1. On NTFS volumes files smaller then 1 kb are placed in MFT.
*    And we exclude their from defragmenting process.
* 2. On NTFS we skip 0-filled virtual clusters of compressed files.
* 3. This function must fill pfn->blockmap list, nothing more.
*/

BOOLEAN DumpFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	IO_STATUS_BLOCK ioStatus;
	PGET_RETRIEVAL_DESCRIPTOR fileMappings;
	NTSTATUS Status;
	HANDLE hFile;
	int i;
	long counter;
	#define MAX_COUNTER 1000
	PBLOCKMAP block = NULL;

	/* Data initialization */
	pfn->clusters_total = pfn->n_fragments = 0;
	pfn->is_fragm = FALSE;
	pfn->blockmap = NULL;

	/* Open the file */
	Status = OpenTheFile(pfn,&hFile);
	if(Status != STATUS_SUCCESS){
		DebugPrint("-Ultradfg- System file found: %ws: %x\n",pfn->name.Buffer,(UINT)Status);
		return FALSE; /* File has unknown state! */
	}

	/* Start dumping the mapping information. Go until we hit the end of the file. */
	*dx->pstartVcn = 0;
	fileMappings = (PGET_RETRIEVAL_DESCRIPTOR)(dx->FileMap);
	counter = 0;
	do {
		RtlZeroMemory(fileMappings,FILEMAPSIZE * sizeof(LARGE_INTEGER));
		Status = ZwFsControlFile(hFile, NULL, NULL, 0, &ioStatus, \
						FSCTL_GET_RETRIEVAL_POINTERS, \
						dx->pstartVcn, sizeof(ULONGLONG), \
						fileMappings, FILEMAPSIZE * sizeof(LARGE_INTEGER));
		counter ++;
		if(NT_SUCCESS(Status)/* == STATUS_PENDING*/){
			if(nt4_system)
				NtWaitForSingleObject(hFile,FALSE,NULL);
			else
				ZwWaitForSingleObject(hFile,FALSE,NULL);
			Status = ioStatus.Status;
		}
		if(Status != STATUS_SUCCESS && Status != STATUS_BUFFER_OVERFLOW){
			/* it always returns STATUS_END_OF_FILE for small files placed in MFT */
			if(Status != STATUS_END_OF_FILE)
				DebugPrint("-Ultradfg- Dump failed for %ws: %x\n",pfn->name.Buffer,(UINT)Status);
			goto dump_fail;
		}

		/* user must have a chance to break infinite loops */
		if(KeReadStateEvent(&stop_event)){
			if(counter > MAX_COUNTER)
				DebugPrint("-Ultradfg- Infinite main loop? %ws\n",pfn->name.Buffer);
			goto dump_fail;
		}

		if(!fileMappings->NumberOfPairs && Status != STATUS_SUCCESS){
			DebugPrint("-Ultradfg- Empty map of file %ws\n",pfn->name.Buffer);
			goto dump_fail;
		}
		
		/* Loop through the buffer of number/cluster pairs. */
		 *dx->pstartVcn = fileMappings->StartVcn;
		for(i = 0; i < (ULONGLONG) fileMappings->NumberOfPairs;	i++){
			/* Infinite loop here will cause BSOD. */

			/*
			* A compressed virtual run (0-filled) is
			* identified with a cluster offset of -1.
			*/
			if(fileMappings->Pair[i].Lcn == LLINVALID) goto next_run;
			
			/* The following code may cause an infinite main loop (bug #2053941?), */
			/* but for some 3.99 Gb files on FAT32 it works fine. */
			if(fileMappings->Pair[i].Vcn == 0){
				DebugPrint("-Ultradfg- Wrong map of file %ws\n",pfn->name.Buffer);
				goto next_run;
			}
			
			block = (PBLOCKMAP)InsertItem((PLIST *)&pfn->blockmap,(PLIST)block,sizeof(BLOCKMAP),PagedPool);
			if(!block) goto dump_fail;
			block->lcn = fileMappings->Pair[i].Lcn;
			block->length = fileMappings->Pair[i].Vcn - *dx->pstartVcn;
			block->vcn = *dx->pstartVcn;

			if(!wcscmp(pfn->name.Buffer,L"\\??\\L:\\go.zip")){
				DbgPrint("VCN %I64u : LEN %I64u : LCN %I64u\n",block->vcn,block->length,block->lcn);
			}

			pfn->clusters_total += block->length;
			pfn->n_fragments ++;
			/*
			* Sometimes normal file has more than one fragment, 
			* but is not fragmented yet! *CRAZY* 
			*/
			if(block != pfn->blockmap && \
			  block->lcn != (block->prev_ptr->lcn + block->prev_ptr->length))
				pfn->is_fragm = TRUE;
		next_run:
			*dx->pstartVcn = fileMappings->Pair[i].Vcn;
		}
	} while(Status != STATUS_SUCCESS);

	/* Skip small directories placed in MFT (tested on 32-bit XP system). */
	if(pfn->blockmap){
		ZwClose(hFile);
		return TRUE; /* success */
	}

dump_fail:
	DeleteBlockmap(pfn);
	pfn->clusters_total = pfn->n_fragments = 0;
	pfn->is_fragm = FALSE;
	ZwClose(hFile);
	return FALSE;
}
