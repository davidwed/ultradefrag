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

BLOCKMAP *InsertBlock(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,
					  ULONGLONG startVcn,ULONGLONG startLcn,ULONGLONG length);

/*
* Dump File()
* Dumps the clusters belonging to the specified file until the
* end of the file.
*
* NOTES: 
* 1. On NTFS volumes files smaller then 1 kb are placed in MFT.
*    And we exclude their from defragmenting process.
* 2. On NTFS we skip 0-filled virtual clusters of compressed files.
*/

BOOLEAN DumpFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	IO_STATUS_BLOCK ioStatus;
	PGET_RETRIEVAL_DESCRIPTOR fileMappings;
	NTSTATUS Status;
	HANDLE hFile;
	ULONGLONG startLcn,length;
	int i,cnt = 0;
	long counter, counter2;
	#define MAX_COUNTER 1000

	/* Data initialization */
	pfn->clusters_total = pfn->n_fragments = 0;
	pfn->is_fragm = FALSE;
	pfn->blockmap = NULL;
//	dx->lastblock = NULL;
	/* Open the file */
	Status = OpenTheFile(pfn,&hFile);
	if(Status != STATUS_SUCCESS){
		DebugPrint1("-Ultradfg- System file found: %x\n",pfn->name.Buffer,(UINT)Status);
		hFile = NULL;
		/* number of fragments should be no less than number of files */
		dx->fragmcounter ++;
		return TRUE; /* System file! */
	}

	/* Start dumping the mapping information. Go until we hit the end of the file. */
	*dx->pstartVcn = 0;
	fileMappings = (PGET_RETRIEVAL_DESCRIPTOR)(dx->FileMap);
	counter = 0;
	do {
		Status = ZwFsControlFile(hFile, NULL, NULL, 0, &ioStatus, \
						FSCTL_GET_RETRIEVAL_POINTERS, \
						dx->pstartVcn, sizeof(ULONGLONG), \
						fileMappings, FILEMAPSIZE * sizeof(LARGE_INTEGER));
		counter ++;
		if(Status == STATUS_PENDING){
			NtWaitForSingleObject(hFile,FALSE,NULL);
			Status = ioStatus.Status;
		}
		if(Status != STATUS_SUCCESS && Status != STATUS_BUFFER_OVERFLOW){
			/* it always returns STATUS_END_OF_FILE for small files placed in MFT */
			if(Status != STATUS_END_OF_FILE)
				DebugPrint("-Ultradfg- Dump failed %x\n",pfn->name.Buffer,(UINT)Status);
dump_fail:
			DeleteBlockmap(pfn);
			pfn->clusters_total = pfn->n_fragments = 0;
			pfn->is_fragm = FALSE;
			ZwClose(hFile);
			return FALSE;
		}

		/* user must have a chance to break infinite loops */
		if(KeReadStateEvent(&stop_event)){
			if(counter > MAX_COUNTER)
				DebugPrint("-Ultradfg- Infinite main loop?\n",pfn->name.Buffer);
			goto dump_fail;
		}

		/* Loop through the buffer of number/cluster pairs. */
		*dx->pstartVcn = fileMappings->StartVcn;
		
		if(!fileMappings->NumberOfPairs && Status != STATUS_SUCCESS){
			DebugPrint("-Ultradfg- Empty map of file\n",pfn->name.Buffer);
			goto dump_fail;
		}
		
		counter2 = 0;
		for(i = 0; i < (ULONGLONG) fileMappings->NumberOfPairs; i++){
			counter2 ++;
			/* user must have a chance to break infinite loops */
			if(KeReadStateEvent(&stop_event)){
				if(counter > MAX_COUNTER)
					DebugPrint("-Ultradfg- Infinite main loop?\n",pfn->name.Buffer);
				if(counter2 > MAX_COUNTER)
					DebugPrint("-Ultradfg- Infinite second loop?\n",pfn->name.Buffer);
				goto dump_fail;
			}

			/*
			* On NT 4.0 (and later NT versions),
			* a compressed virtual run (0-filled) is
			* identified with a cluster offset of -1.
			*/
			if(fileMappings->Pair[i].Lcn == LLINVALID)
				goto next_run;
			
			/* the following code will cause an infinite loop (bug #2053941) */
			/*if(fileMappings->Pair[i].Vcn == 0)
				goto next_run;*/ /* only for some 3.99 Gb files on FAT32 */
			
			if(fileMappings->Pair[i].Vcn == 0){
				DebugPrint("-Ultradfg- Wrong map of file\n",pfn->name.Buffer);
				goto next_run;/*dump_fail;*/
			}
			
			startLcn = fileMappings->Pair[i].Lcn;
			length = fileMappings->Pair[i].Vcn - *dx->pstartVcn;
			dx->processed_clusters += length;
			if(!InsertBlock(dx,pfn,*dx->pstartVcn,startLcn,length))
				goto dump_fail;
			cnt ++;	/* block counter */
next_run:
			*dx->pstartVcn = fileMappings->Pair[i].Vcn;
		}
	} while(Status != STATUS_SUCCESS);

	if(!cnt){
		/*
		* It's a small directory placed in MFT
		* (at least on dmitriar's 32-bit XP system).
		*/
		goto dump_fail;
	}
	ZwClose(hFile);

	MarkSpace(dx,pfn,SYSTEM_SPACE);

	pfn->n_fragments = cnt;
	if(pfn->is_fragm){
		dx->fragmfilecounter ++; /* file is true fragmented */
	}
	if(pfn->is_fragm) dx->fragmcounter += cnt;
	else dx->fragmcounter ++;
	return TRUE; /* success */
}

/* TRUE if the file was successfuly moved or placed in MFT. */
BOOLEAN CheckFilePosition(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,
					ULONGLONG targetLcn, ULONGLONG n_clusters)
{
	IO_STATUS_BLOCK ioStatus;
	PGET_RETRIEVAL_DESCRIPTOR fileMappings;
	NTSTATUS Status;
	ULONGLONG startLcn;
	long counter, counter2;
	#define MAX_COUNTER 1000
	int i;

	/* Start dumping the mapping information. Go until we hit the end of the file. */
	*dx->pstartVcn = 0;
	fileMappings = (PGET_RETRIEVAL_DESCRIPTOR)(dx->FileMap);
	counter = 0;
	do {
		Status = ZwFsControlFile(hFile, NULL, NULL, 0, &ioStatus, \
						FSCTL_GET_RETRIEVAL_POINTERS, \
						dx->pstartVcn, sizeof(ULONGLONG), \
						fileMappings, FILEMAPSIZE * sizeof(LARGE_INTEGER));
		counter ++;
		if(Status == STATUS_PENDING){
			NtWaitForSingleObject(hFile,FALSE,NULL);
			Status = ioStatus.Status;
		}
		if(Status != STATUS_SUCCESS && Status != STATUS_BUFFER_OVERFLOW){
			/* it always returns STATUS_END_OF_FILE for small files placed in MFT */
			if(Status != STATUS_END_OF_FILE){
				DebugPrint("-Ultradfg- Dump failed %x\n",NULL,(UINT)Status);
				return FALSE;
			}
			return TRUE;
		}

		/* user must have a chance to break infinite loops */
		if(KeReadStateEvent(&stop_event)){
			if(counter > MAX_COUNTER)
				DebugPrint("-Ultradfg- Infinite main loop?\n",NULL);
			return FALSE;
		}

		/* Loop through the buffer of number/cluster pairs. */
		*dx->pstartVcn = fileMappings->StartVcn;
		
		if(!fileMappings->NumberOfPairs){
			DebugPrint("-Ultradfg- Empty map of file\n",NULL);
			return TRUE;
		}
		
		counter2 = 0;
		for(i = 0; i < (ULONGLONG) fileMappings->NumberOfPairs; i++){
			counter2 ++;
			/* user must have a chance to break infinite loops */
			if(KeReadStateEvent(&stop_event)){
				if(counter > MAX_COUNTER)
					DebugPrint("-Ultradfg- Infinite main loop?\n",NULL);
				if(counter2 > MAX_COUNTER)
					DebugPrint("-Ultradfg- Infinite second loop?\n",NULL);
				return FALSE;
			}

			/*
			* On NT 4.0 (and later NT versions),
			* a compressed virtual run (0-filled) is
			* identified with a cluster offset of -1.
			*/
			if(fileMappings->Pair[i].Lcn == LLINVALID)
				goto next_run;
			
			/* the following code will cause an infinite loop (bug #2053941) */
			/*if(fileMappings->Pair[i].Vcn == 0)
				goto next_run;*/ /* only for some 3.99 Gb files on FAT32 */
			
			if(fileMappings->Pair[i].Vcn == 0){
				DebugPrint("-Ultradfg- Wrong map of file\n",NULL);
				goto next_run;/*dump_fail;*/
			}
			
			startLcn = fileMappings->Pair[i].Lcn;
			if(startLcn < targetLcn || startLcn >= (targetLcn + n_clusters))
				return FALSE;
next_run:
			*dx->pstartVcn = fileMappings->Pair[i].Vcn;
		}
	} while(Status != STATUS_SUCCESS);
	return TRUE;
}

/* inserts the specified block in file's blockmap */
BLOCKMAP *InsertBlock(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,
					ULONGLONG startVcn,ULONGLONG startLcn,ULONGLONG length)
{
	PBLOCKMAP block, lastblock = NULL;

	/* for compressed files it's neccessary: */
	if(pfn->blockmap/*dx->lastblock*/){
		lastblock = pfn->blockmap->prev_ptr;
		if(startLcn != lastblock->lcn + lastblock->length)
			pfn->is_fragm = TRUE;
	}
/*	block = (PBLOCKMAP)InsertLastItem((PLIST *)&pfn->blockmap,
		(PLIST *)&dx->lastblock,sizeof(BLOCKMAP));
*/
	block = (PBLOCKMAP)InsertItem((PLIST *)&pfn->blockmap,(PLIST)lastblock,sizeof(BLOCKMAP));
	if(block){
		block->lcn = startLcn;
		block->length = length;
		block->vcn = startVcn;
		/* change file statistic */
		pfn->clusters_total += length;
	}
	return block;
}

void DeleteBlockmap(PFILENAME pfn)
{
	DestroyList((PLIST *)&pfn->blockmap);
}
