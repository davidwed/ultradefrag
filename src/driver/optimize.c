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
* Volume optimization routines.
*/

#include "driver.h"

void MovePartOfFileBlock(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,ULONGLONG startVcn,
		ULONGLONG targetLcn,ULONGLONG n_clusters);
void DefragmentFreeSpaceRTL(UDEFRAG_DEVICE_EXTENSION *dx);
void DefragmentFreeSpaceLTR(UDEFRAG_DEVICE_EXTENSION *dx);
void MoveAllFilesRTL(UDEFRAG_DEVICE_EXTENSION *dx);

/*
* NOTE: Algorithm is effective only 
* if optimized volume has a lot of free space.
*/
void Optimize(UDEFRAG_DEVICE_EXTENSION *dx)
{
	KSPIN_LOCK spin_lock;
	KIRQL oldIrql;
	PFREEBLOCKMAP freeblock;

	DebugPrint("-Ultradfg- ----- Optimization of %c: -----\n",NULL,dx->letter);
	DeleteLogFile(dx);

	/* Initialize progress counters. */
	KeInitializeSpinLock(&spin_lock);
	KeAcquireSpinLock(&spin_lock,&oldIrql);
	dx->clusters_to_process = dx->processed_clusters = 0;
	KeReleaseSpinLock(&spin_lock,oldIrql);
	for(freeblock = dx->free_space_map; freeblock != NULL; freeblock = freeblock->next_ptr){
		if(freeblock->next_ptr == dx->free_space_map) break;
		dx->clusters_to_process += freeblock->length;
	}
	dx->current_operation = 'C';
	
	/* On FAT volumes it increase distance between dir & files inside it. */
	if(dx->partition_type != NTFS_PARTITION) return;
	
	/*
	* First of all - free the leading part of the volume.
	*/
	DebugPrint("-Ultradfg- ----- First step of optimization of %c: -----\n",NULL,dx->letter);
	DefragmentFreeSpaceLTR(dx);
	if(KeReadStateEvent(&stop_event)) goto optimization_done;
	
	/*
	* Second step - defragment files.
	* Large files will be moved to the beginning of the volume.
	*/
	DebugPrint("-Ultradfg- ----- Second step of optimization of %c: -----\n",NULL,dx->letter);
	Analyse(dx);
	Defragment(dx);

	/*
	* Final step - move all files to the leading part of the volume.
	*/
	DebugPrint("-Ultradfg- ----- Third step of optimization of %c: -----\n",NULL,dx->letter);
	MoveAllFilesRTL(dx);
	
optimization_done:	
	/* Analyse volume again to update fragmented files list. */
	KeClearEvent(&stop_event);
	Analyse(dx);
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
void DefragmentFreeSpaceRTL(UDEFRAG_DEVICE_EXTENSION *dx)
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
		for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
			for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
				if(block->lcn > maxlcn){
					lastblock = block;
					lastpfn = pfn;
					maxlcn = block->lcn;
				}
				if(block->next_ptr == pfn->blockmap) break;
			}
			if(pfn->next_ptr == dx->filelist) break;
		}
		if(!lastblock) break;
		DebugPrint("-Ultradfg- Last block = Lcn:%I64u Length:%I64u\n",lastpfn->name.Buffer,
			lastblock->lcn,lastblock->length);
		if(KeReadStateEvent(&stop_event)) break;
		/* 2. Fill free space areas in the beginning of the volume with lastblock contents. */
		movings = 0;
		for(freeblock = dx->free_space_map; freeblock != NULL; freeblock = freeblock->next_ptr){
			if(freeblock->lcn < lastblock->lcn && freeblock->length){
				/* fill block with lastblock contents */
				length = min(freeblock->length,lastblock->length);
				vcn = lastblock->vcn + (lastblock->length - length);
				MovePartOfFileBlock(dx,lastpfn,vcn,freeblock->lcn,length);
				movings ++;
				dx->processed_clusters += length;
				ProcessBlock(dx,freeblock->lcn,length,UNKNOWN_SPACE,FREE_SPACE);
				ProcessBlock(dx,lastblock->lcn + (lastblock->length - length),length,
					FREE_SPACE,GetSpaceState(lastpfn));
				freeblock->length -= length;
				freeblock->lcn += length;
				lastblock->length -= length;
				if(!lastblock->length){
					RemoveItem((PLIST *)&lastpfn->blockmap,(PLIST)lastblock);
					break;
				}
			}
			if(KeReadStateEvent(&stop_event)) break;
			if(freeblock->next_ptr == dx->free_space_map) break;
		}
		if(!movings) break;
	}
}

void DefragmentFreeSpaceLTR(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFILENAME pfn, firstpfn;
	PBLOCKMAP block, firstblock;
	PFREEBLOCKMAP freeblock;
	ULONGLONG minlcn;
	ULONGLONG vcn, length;
	ULONGLONG movings;
	
	while(1){
		/* 1. Find the first file block on the volume. */
		firstblock = NULL; firstpfn = NULL; minlcn = (dx->clusters_total - 1);
		for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
			for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
				if(block->lcn < minlcn){
					firstblock = block;
					firstpfn = pfn;
					minlcn = block->lcn;
				}
				if(block->next_ptr == pfn->blockmap) break;
			}
			if(pfn->next_ptr == dx->filelist) break;
		}
		if(!firstblock) break;
		DebugPrint("-Ultradfg- First block = Lcn:%I64u Length:%I64u\n",firstpfn->name.Buffer,
			firstblock->lcn,firstblock->length);
		if(KeReadStateEvent(&stop_event)) break;
		/* 2. Fill free space areas in the terminal part of the volume with firstblock contents. */
		movings = 0;
		if(!dx->free_space_map) break;
		for(freeblock = dx->free_space_map->prev_ptr; 1; freeblock = freeblock->prev_ptr){
			if(freeblock->lcn > firstblock->lcn && freeblock->length){
				/* fill block with firstblock contents */
				length = min(freeblock->length,firstblock->length);
				vcn = firstblock->vcn;
				MovePartOfFileBlock(dx,firstpfn,vcn,
					freeblock->lcn + (freeblock->length - length),length);
				movings ++;
				dx->processed_clusters += length;
				ProcessBlock(dx,freeblock->lcn + (freeblock->length - length),length,
					UNKNOWN_SPACE,FREE_SPACE);
				ProcessBlock(dx,firstblock->lcn,length,FREE_SPACE,GetSpaceState(firstpfn));
				freeblock->length -= length;
				firstblock->vcn += length;
				firstblock->lcn += length;
				firstblock->length -= length;
				if(!firstblock->length){
					RemoveItem((PLIST *)&firstpfn->blockmap,(PLIST)firstblock);
					break;
				}
			}
			if(KeReadStateEvent(&stop_event)) break;
			if(freeblock->prev_ptr == dx->free_space_map->prev_ptr) break;
		}
		if(!movings) break;
	}
}

/* For defragmenter only, not for optimizer! */
BOOLEAN MoveTheUnfragmentedFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,ULONGLONG target)
{
	NTSTATUS Status;
	HANDLE hFile;
	PBLOCKMAP block;

	/* Open the file */
	Status = OpenTheFile(pfn,&hFile);
	if(Status){
		DebugPrint("-Ultradfg- Can't open file: %x\n",pfn->name.Buffer,(UINT)Status);
		/* we need to destroy the block map to avoid infinite loops */
		DeleteBlockmap(pfn); /* file is locked by other application, so its state is unknown */
		return FALSE;
	}

	DebugPrint("-Ultradfg-\n",pfn->name.Buffer);
	DebugPrint("-Ultradfg- t: %I64u n: %I64u\n",NULL,target,pfn->clusters_total);

	Status = MoveBlocksOfFile(dx,pfn,hFile,target);
	ZwClose(hFile);
	
	/* first of all: remove target space from free space pool */
	ProcessBlock(dx,target,pfn->clusters_total,GetSpaceState(pfn),FREE_SPACE);
	TruncateFreeSpaceBlock(dx,target,pfn->clusters_total);

	/* free previously allocated space (after TruncateFreeSpaceBlock() call!) */
	for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
		ProcessFreeBlock(dx,block->lcn,block->length,FRAGM_SPACE/*old_state*/);
		if(block->next_ptr == pfn->blockmap) break;
	}

	DeleteBlockmap(pfn); /* because we don't need this info after file moving */
	return TRUE;
}

void MoveAllFilesRTL(UDEFRAG_DEVICE_EXTENSION *dx)
{
	KSPIN_LOCK spin_lock;
	KIRQL oldIrql;
	PFREEBLOCKMAP block;
	PFILENAME pfn, plargest;
	ULONGLONG length;

	/* Reinitialize progress counters. */
	KeInitializeSpinLock(&spin_lock);
	KeAcquireSpinLock(&spin_lock,&oldIrql);
	dx->clusters_to_process = dx->processed_clusters = 0;
	KeReleaseSpinLock(&spin_lock,oldIrql);
	for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
		if(pfn->blockmap) dx->clusters_to_process += pfn->clusters_total;
		if(pfn->next_ptr == dx->filelist) break;
	}
	dx->current_operation = 'C';

	for(block = dx->free_space_map; block != NULL; block = block->next_ptr){
	L0:
		if(block->length <= 1) goto L1; /* skip 1 cluster blocks and zero length blocks */
		/* find largest unfragmented file that can be stored here */
		plargest = NULL; length = 0;
		for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
			if(pfn->blockmap && pfn->clusters_total <= block->length){
				if(pfn->clusters_total > length){
					plargest = pfn;
					length = pfn->clusters_total;
				}
			}
			if(pfn->next_ptr == dx->filelist) break;
		}
		if(!plargest) goto L1; /* current block is too small */
		/* move file */
		if(MoveTheUnfragmentedFile(dx,plargest,block->lcn))
			DebugPrint("-Ultradfg- Moving success for\n",plargest->name.Buffer);
		else
			DebugPrint("-Ultradfg- Moving error for\n",plargest->name.Buffer);
		dx->processed_clusters += plargest->clusters_total;
		if(KeReadStateEvent(&stop_event)) break;
		/* after file moving continue from the first free space block */
		block = dx->free_space_map; if(!block) break; else goto L0;
	L1:
		if(block->next_ptr == dx->free_space_map) break;
	}
}

void MovePartOfFileBlock(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,ULONGLONG startVcn,
		ULONGLONG targetLcn,ULONGLONG n_clusters)
{
	HANDLE hFile;
	ULONGLONG target;
	ULONGLONG j,n,r;
	NTSTATUS Status;

	Status = OpenTheFile(pfn,&hFile);
	if(Status) return;

	target = targetLcn;
	n = n_clusters / dx->clusters_per_256k;
	for(j = 0; j < n; j++){
		Status = MovePartOfFile(dx,hFile,startVcn + j * dx->clusters_per_256k, \
			target,dx->clusters_per_256k);
		if(Status){ ZwClose(hFile); return; }
		target += dx->clusters_per_256k;
	}
	r = n_clusters % dx->clusters_per_256k;
	if(r){
		Status = MovePartOfFile(dx,hFile,startVcn + j * dx->clusters_per_256k, \
			target,r);
		if(Status){ ZwClose(hFile); return; }
	}

	ZwClose(hFile);
}
