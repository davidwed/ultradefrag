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
int  MoveAllFilesRTL(UDEFRAG_DEVICE_EXTENSION *dx);
int  OptimizationRoutine(UDEFRAG_DEVICE_EXTENSION *dx);

ULONGLONG StartingPoint = 0;
int pass_number = 0;

/*
* NOTE: Algorithm is effective always,
* even if optimized volume has low amount of free space.
*/
void Optimize(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFREEBLOCKMAP freeblock;
	ULONGLONG threshold;
	ULONGLONG FragmentedClustersBeforeStartingPoint = 0;
	PFILENAME pfn;
	PBLOCKMAP block;
	HANDLE hFile;

	/* define threshold */
	threshold = dx->clusters_total / 200; /* 0.5% */
	if(threshold < 2) threshold = 2;
	/* define starting point */
	StartingPoint = 0; /* start moving from the beginning of the volume */
	pass_number = 1;
	for(freeblock = dx->free_space_map; freeblock != NULL; freeblock = freeblock->next_ptr){
		/* is block larger than 0.5% of the volume space? */
		if(freeblock->lcn > StartingPoint && freeblock->length >= threshold){
			StartingPoint = freeblock->lcn;
			break;
		}
		if(freeblock->next_ptr == dx->free_space_map) return;
	}
	/* validate StartingPoint */
	for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
		if(pfn->is_reparse_point == FALSE && pfn->is_fragm){
			/* skip system files */
			if(OpenTheFile(pfn,&hFile) == STATUS_SUCCESS){
				ZwCloseSafe(hFile);
				for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
					if((block->lcn + block->length) <= StartingPoint)
						FragmentedClustersBeforeStartingPoint += block->length;
					if(block->next_ptr == pfn->blockmap) break;
				}
			}
		}
		if(pfn->next_ptr == dx->filelist) break;
	}
	if(FragmentedClustersBeforeStartingPoint >= threshold) StartingPoint = 0;

	do {
		DebugPrint("-Ultradfg- Optimization pass #%u, StartingPoint = %I64u\n",pass_number,StartingPoint);
		/* call optimization routine */
		if(OptimizationRoutine(dx) < 0) return;
		if(KeReadStateEvent(&stop_event)) return;
		pass_number ++;
		/* redefine starting point */
		for(freeblock = dx->free_space_map; freeblock != NULL; freeblock = freeblock->next_ptr){
			/* is block larger than 0.5% of the volume space? */
			if(freeblock->lcn > StartingPoint && freeblock->length >= threshold){
				StartingPoint = freeblock->lcn;
				break;
			}
			if(freeblock->next_ptr == dx->free_space_map) return;
		}
	} while (1);
}

/* returns -1 if no files were defragmented or optimized, zero otherwise */
int OptimizationRoutine(UDEFRAG_DEVICE_EXTENSION *dx)
{
	static KSPIN_LOCK spin_lock;
	static KIRQL oldIrql;
	PFREEBLOCKMAP freeblock;
	int defragmenter_result = 0;
	int optimizer_result = 0;

	DebugPrint("-Ultradfg- ----- Optimization of %c: -----\n",dx->letter);

	/* Initialize progress counters. */
	KeInitializeSpinLock(&spin_lock);
	KeAcquireSpinLock(&spin_lock,&oldIrql);
	dx->clusters_to_process = dx->processed_clusters = 0;
	KeReleaseSpinLock(&spin_lock,oldIrql);
	if(dx->free_space_map){
		for(freeblock = dx->free_space_map->prev_ptr; 1; freeblock = freeblock->prev_ptr){
			if(freeblock->lcn >= StartingPoint) dx->clusters_to_process += freeblock->length;
			if(freeblock->prev_ptr == dx->free_space_map) break;
		}
	}
	dx->current_operation = 'C';
	
	/* On FAT volumes it increase distance between dir & files inside it. */
	if(dx->partition_type != NTFS_PARTITION) return (-1);
	
	/*
	* First of all - free the leading part of the volume.
	*/
	DebugPrint("-Ultradfg- ----- First step of optimization of %c: -----\n",dx->letter);
	DefragmentFreeSpaceLTR(dx);
	if(KeReadStateEvent(&stop_event)) goto optimization_done;
	
	/*
	* Second step - defragment files.
	* Large files will be moved to the beginning of the volume.
	*/
	DebugPrint("-Ultradfg- ----- Second step of optimization of %c: -----\n",dx->letter);
	Analyse(dx);
	defragmenter_result = Defragment(dx);

	/*
	* Final step - move all files to the leading part of the volume.
	*/
	DebugPrint("-Ultradfg- ----- Third step of optimization of %c: -----\n",dx->letter);
	///AnalyseFreeSpace(dx); /* refresh free space map */
	optimizer_result = MoveAllFilesRTL(dx);
	
optimization_done:	
	/* Analyse volume again to update fragmented files list. */
	KeClearEvent(&stop_event);
	Analyse(dx);

	if(defragmenter_result >= 0 || optimizer_result >= 0) return 0;
	return (-1);
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

/* Starting point takes no effect here */
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
			if(pfn->next_ptr == dx->filelist) break;
		}
		if(!lastblock) break;
		DebugPrint("-Ultradfg- Last block = %ws: Lcn:%I64u Length:%I64u\n",lastpfn->name.Buffer,
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

/* Starting point works here */
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
			if(pfn->is_reparse_point == FALSE){
				for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
					if(block->lcn < minlcn && block->lcn >= StartingPoint){
						firstblock = block;
						firstpfn = pfn;
						minlcn = block->lcn;
					}
					if(block->next_ptr == pfn->blockmap) break;
				}
			}
			if(pfn->next_ptr == dx->filelist) break;
		}
		if(!firstblock) break;
		DebugPrint("-Ultradfg- First block = %ws: Lcn:%I64u Length:%I64u\n",firstpfn->name.Buffer,
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

/* For optimizer only, not for defragmenter! */
BOOLEAN MoveTheUnfragmentedFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,ULONGLONG target)
{
	NTSTATUS Status;
	HANDLE hFile;
	PBLOCKMAP block;

	/* Open the file */
	Status = OpenTheFile(pfn,&hFile);
	if(Status){
		DebugPrint("-Ultradfg- Can't open %ws file: %x\n",pfn->name.Buffer,(UINT)Status);
		/* we need to destroy the block map to avoid infinite loops */
		DeleteBlockmap(pfn); /* file is locked by other application, so its state is unknown */
		return FALSE;
	}

	DebugPrint("-Ultradfg- %ws\n",pfn->name.Buffer);
	DebugPrint("-Ultradfg- t: %I64u n: %I64u\n",target,pfn->clusters_total);

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

/* returns -1 if no files were moved, zero otherwise */
int MoveAllFilesRTL(UDEFRAG_DEVICE_EXTENSION *dx)
{
	KSPIN_LOCK spin_lock;
	KIRQL oldIrql;
	PFREEBLOCKMAP block;
	PFILENAME pfn, plargest;
	ULONGLONG length;
	ULONGLONG movings = 0;

	/* Reinitialize progress counters. */
	KeInitializeSpinLock(&spin_lock);
	KeAcquireSpinLock(&spin_lock,&oldIrql);
	dx->clusters_to_process = dx->processed_clusters = 0;
	KeReleaseSpinLock(&spin_lock,oldIrql);
	for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
		if(pfn->blockmap && !pfn->is_reparse_point)
			if(pfn->blockmap->lcn >= StartingPoint)
				dx->clusters_to_process += pfn->clusters_total;
		if(pfn->next_ptr == dx->filelist) break;
	}
	dx->current_operation = 'C';

	for(block = dx->free_space_map; block != NULL; block = block->next_ptr){
	L0:
		if(block->length <= 1) goto L1; /* skip 1 cluster blocks and zero length blocks */
		//if(block->lcn < StartingPoint) goto L1; /* skip blocks before StartingPoint */
		/* find largest unfragmented file that can be stored here */
		plargest = NULL; length = 0;
		for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
			if(pfn->is_reparse_point == FALSE){
				if(pfn->blockmap && pfn->clusters_total <= block->length){
					if(pfn->clusters_total > length && pfn->blockmap->lcn >= StartingPoint){
						plargest = pfn;
						length = pfn->clusters_total;
					}
				}
			}
			if(pfn->next_ptr == dx->filelist) break;
		}
		if(!plargest) goto L1; /* current block is too small */
		/* skip fragmented files */
		///if(plargest->is_fragm) goto L1; /* never do it!!! */
		/* if uncomment the previous line file moving will never be ahieved, because free blocks 
		 will be always skipped by this instruction */
		/* move file */
		if(MoveTheUnfragmentedFile(dx,plargest,block->lcn)){
			DebugPrint("-Ultradfg- Moving success for %ws\n",plargest->name.Buffer);
			movings++;
		} else {
			DebugPrint("-Ultradfg- Moving error for %ws\n",plargest->name.Buffer);
		}
		dx->processed_clusters += plargest->clusters_total;
		if(KeReadStateEvent(&stop_event)) break;
		/* after file moving continue from the first free space block */
		block = dx->free_space_map; if(!block) break; else goto L0;
	L1:
		if(block->next_ptr == dx->free_space_map) break;
	}
	return (movings == 0) ? (-1) : (0);
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
