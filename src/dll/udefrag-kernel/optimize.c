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
* User mode driver - volume optimization routines.
*/

#include "globals.h"

void MovePartOfFileBlock(PFILENAME pfn,ULONGLONG startVcn,
		ULONGLONG targetLcn,ULONGLONG n_clusters);
void DefragmentFreeSpaceRTL(void);
void DefragmentFreeSpaceLTR(void);
void MoveAllFilesRTL(void);

/*
* NOTE: Algorithm is effective only 
* if optimized volume has a lot of free space.
*/
int Optimize(char *volume_name)
{
	PFREEBLOCKMAP freeblock;

	DebugPrint("----- Optimization of %s: -----\n",volume_name);

	/* Initialize progress counters. */
	Stat.clusters_to_process = Stat.processed_clusters = 0;
	if(free_space_map){
		for(freeblock = free_space_map->prev_ptr; 1; freeblock = freeblock->prev_ptr){
			Stat.clusters_to_process += freeblock->length;
			if(freeblock->prev_ptr == free_space_map) break;
		}
	}
	Stat.current_operation = 'C';
	
	/* On FAT volumes it increase distance between dir & files inside it. */
	if(partition_type != NTFS_PARTITION) return 0;
	
	/*
	* First of all - free the leading part of the volume.
	*/
	DebugPrint("----- First step of optimization of %s: -----\n",volume_name);
	DefragmentFreeSpaceLTR();
	if(CheckForStopEvent()) goto optimization_done;
	
	/*
	* Second step - defragment files.
	* Large files will be moved to the beginning of the volume.
	*/
	DebugPrint("----- Second step of optimization of %s: -----\n",volume_name);
	Analyze(volume_name);
	Defragment(volume_name);

	/*
	* Final step - move all files to the leading part of the volume.
	*/
	DebugPrint("----- Third step of optimization of %s: -----\n",volume_name);
	MoveAllFilesRTL();
	
optimization_done:	
	/* Analyse volume again to update fragmented files list. */
	NtClearEvent(hStopEvent);
	Analyze(volume_name);
	return 0;
}

/*
* NOTES:
* 1. On FAT it's bad idea, because dirs aren't moveable.
* 2. On NTFS cycle of attempts is a bad solution,
* because it increases processing time. Also on NTFS all 
* space freed during the defragmentation is still temporarily
* allocated by system for a long time.
* 3. It makes an analysis after each volume optimization to 
* update list of fragmented files.
* 4. Usually this function increases a number of fragmented files.
* 5. We cannot update a number of fragmented files during the 
* optimization process.
*/
void DefragmentFreeSpaceRTL(void)
{
	PFILENAME pfn, lastpfn;
	PBLOCKMAP block, lastblock;
	PFREEBLOCKMAP freeblock;
	ULONGLONG maxlcn;
	ULONGLONG vcn, length;
	ULONGLONG movings;
	
	while(1){
		/* 1. Find the latest file block on the volume. */
		lastblock = NULL; lastpfn = NULL; maxlcn = 0;
		for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
			if(pfn->is_reparse_point == FALSE){
				for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
					if(block->lcn > maxlcn){
						lastblock = block;
						lastpfn = pfn;
						maxlcn = block->lcn;
					}
					if(block->next_ptr == pfn->blockmap) break;
				}
			}
			if(pfn->next_ptr == filelist) break;
		}
		if(!lastblock) break;
		DebugPrint("Last block = %ws: Lcn:%I64u Length:%I64u\n",lastpfn->name.Buffer,
			lastblock->lcn,lastblock->length);
		if(CheckForStopEvent()) break;
		/* 2. Fill free space areas in the beginning of the volume with lastblock contents. */
		movings = 0;
		for(freeblock = free_space_map; freeblock != NULL; freeblock = freeblock->next_ptr){
			if(freeblock->lcn < lastblock->lcn && freeblock->length){
				/* fill block with lastblock contents */
				length = min(freeblock->length,lastblock->length);
				vcn = lastblock->vcn + (lastblock->length - length);
				MovePartOfFileBlock(lastpfn,vcn,freeblock->lcn,length);
				movings ++;
				Stat.processed_clusters += length;
				ProcessBlock(freeblock->lcn,length,UNKNOWN_SPACE,FREE_SPACE);
				ProcessBlock(lastblock->lcn + (lastblock->length - length),length,
					FREE_SPACE,GetSpaceState(lastpfn));
				freeblock->length -= length;
				freeblock->lcn += length;
				lastblock->length -= length;
				if(!lastblock->length){
					RemoveItem((PLIST *)&lastpfn->blockmap,(PLIST)lastblock);
					break;
				}
			}
			if(CheckForStopEvent()) break;
			if(freeblock->next_ptr == free_space_map) break;
		}
		if(!movings) break;
	}
}

void DefragmentFreeSpaceLTR(void)
{
	PFILENAME pfn, firstpfn;
	PBLOCKMAP block, firstblock;
	PFREEBLOCKMAP freeblock;
	ULONGLONG minlcn;
	ULONGLONG vcn, length;
	ULONGLONG movings;
	
	while(1){
		/* 1. Find the first file block on the volume. */
		firstblock = NULL; firstpfn = NULL; minlcn = (clusters_total - 1);
		for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
			if(pfn->is_reparse_point == FALSE){
				for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
					if(block->lcn < minlcn){
						firstblock = block;
						firstpfn = pfn;
						minlcn = block->lcn;
					}
					if(block->next_ptr == pfn->blockmap) break;
				}
			}
			if(pfn->next_ptr == filelist) break;
		}
		if(!firstblock) break;
		DebugPrint("First block = %ws: Lcn:%I64u Length:%I64u\n",firstpfn->name.Buffer,
			firstblock->lcn,firstblock->length);
		if(CheckForStopEvent()) break;
		/* 2. Fill free space areas in the terminal part of the volume with firstblock contents. */
		movings = 0;
		if(!free_space_map) break;
		for(freeblock = free_space_map->prev_ptr; 1; freeblock = freeblock->prev_ptr){
			if(freeblock->lcn > firstblock->lcn && freeblock->length){
				/* fill block with firstblock contents */
				length = min(freeblock->length,firstblock->length);
				vcn = firstblock->vcn;
				MovePartOfFileBlock(firstpfn,vcn,
					freeblock->lcn + (freeblock->length - length),length);
				movings ++;
				Stat.processed_clusters += length;
				ProcessBlock(freeblock->lcn + (freeblock->length - length),length,
					UNKNOWN_SPACE,FREE_SPACE);
				ProcessBlock(firstblock->lcn,length,FREE_SPACE,GetSpaceState(firstpfn));
				freeblock->length -= length;
				firstblock->vcn += length;
				firstblock->lcn += length;
				firstblock->length -= length;
				if(!firstblock->length){
					RemoveItem((PLIST *)&firstpfn->blockmap,(PLIST)firstblock);
					break;
				}
			}
			if(CheckForStopEvent()) break;
			if(freeblock->prev_ptr == free_space_map->prev_ptr) break;
		}
		if(!movings) break;
	}
}

/* For defragmenter only, not for optimizer! */
BOOLEAN MoveTheUnfragmentedFile(PFILENAME pfn,ULONGLONG target)
{
	NTSTATUS Status;
	HANDLE hFile;
	PBLOCKMAP block;

	/* Open the file */
	Status = OpenTheFile(pfn,&hFile);
	if(Status){
		DebugPrint("Can't open %ws file: %x\n",pfn->name.Buffer,(UINT)Status);
		/* we need to destroy the block map to avoid infinite loops */
		DeleteBlockmap(pfn); /* file is locked by other application, so its state is unknown */
		return FALSE;
	}

	DebugPrint("%ws\n",pfn->name.Buffer);
	DebugPrint("t: %I64u n: %I64u\n",target,pfn->clusters_total);

	Status = MoveBlocksOfFile(pfn,hFile,target);
	NtClose(hFile);
	
	/* first of all: remove target space from free space pool */
	ProcessBlock(target,pfn->clusters_total,GetSpaceState(pfn),FREE_SPACE);
	TruncateFreeSpaceBlock(target,pfn->clusters_total);

	/* free previously allocated space (after TruncateFreeSpaceBlock() call!) */
	for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
		ProcessFreeBlock(block->lcn,block->length,FRAGM_SPACE/*old_state*/);
		if(block->next_ptr == pfn->blockmap) break;
	}

	DeleteBlockmap(pfn); /* because we don't need this info after file moving */
	return TRUE;
}

void MoveAllFilesRTL(void)
{
	PFREEBLOCKMAP block;
	PFILENAME pfn, plargest;
	ULONGLONG length;

	/* Reinitialize progress counters. */
	Stat.clusters_to_process = Stat.processed_clusters = 0;
	for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
		if(pfn->blockmap && !pfn->is_reparse_point)
			Stat.clusters_to_process += pfn->clusters_total;
		if(pfn->next_ptr == filelist) break;
	}
	Stat.current_operation = 'C';

	for(block = free_space_map; block != NULL; block = block->next_ptr){
	L0:
		if(block->length <= 1) goto L1; /* skip 1 cluster blocks and zero length blocks */
		/* find largest unfragmented file that can be stored here */
		plargest = NULL; length = 0;
		for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
			if(pfn->is_reparse_point == FALSE){
				if(pfn->blockmap && pfn->clusters_total <= block->length){
					if(pfn->clusters_total > length){
						plargest = pfn;
						length = pfn->clusters_total;
					}
				}
			}
			if(pfn->next_ptr == filelist) break;
		}
		if(!plargest) goto L1; /* current block is too small */
		/* move file */
		if(MoveTheUnfragmentedFile(plargest,block->lcn))
			DebugPrint("Moving success for %ws\n",plargest->name.Buffer);
		else
			DebugPrint("Moving error for %ws\n",plargest->name.Buffer);
		Stat.processed_clusters += plargest->clusters_total;
		if(CheckForStopEvent()) break;
		/* after file moving continue from the first free space block */
		block = free_space_map; if(!block) break; else goto L0;
	L1:
		if(block->next_ptr == free_space_map) break;
	}
}

void MovePartOfFileBlock(PFILENAME pfn,ULONGLONG startVcn,
		ULONGLONG targetLcn,ULONGLONG n_clusters)
{
	HANDLE hFile;
	ULONGLONG target;
	ULONGLONG j,n,r;
	NTSTATUS Status;

	Status = OpenTheFile(pfn,&hFile);
	if(Status) return;

	target = targetLcn;
	n = n_clusters / clusters_per_256k;
	for(j = 0; j < n; j++){
		Status = MovePartOfFile(hFile,startVcn + j * clusters_per_256k, \
			target,clusters_per_256k);
		if(Status){ NtClose(hFile); return; }
		target += clusters_per_256k;
	}
	r = n_clusters % clusters_per_256k;
	if(r){
		Status = MovePartOfFile(hFile,startVcn + j * clusters_per_256k, \
			target,r);
		if(Status){ NtClose(hFile); return; }
	}

	NtClose(hFile);
}
