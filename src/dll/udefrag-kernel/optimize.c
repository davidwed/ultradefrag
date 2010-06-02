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
 * @file optimize.c
 * @brief Volume optimization code.
 * @addtogroup Optimizer
 * @{
 */

#include "globals.h"

void MovePartOfFileBlock(PFILENAME pfn,ULONGLONG startVcn,
		ULONGLONG targetLcn,ULONGLONG n_clusters);
void DefragmentFreeSpaceRTL(void);
void DefragmentFreeSpaceLTR(void);
int  MoveAllFilesRTL(void);
int  OptimizationRoutine(char *volume_name);
int  MoveRestOfFilesRTL(char *volume_name);

ULONGLONG StartingPoint = 0;
ULONGLONG threshold;
int pass_number = 0;

/**
 * @brief Performs a volume optimization job.
 * @param[in] volume_name the name of the volume.
 * @return Zero for success, negative value otherwise.
 * @note
 * - The algorithm used here is effective. Even
 * when the volume to be optimized has a low amount
 * of free space.
 * - We cannot update a number of fragmented files
 * during the volume optimization process.
 */
int Optimize(char *volume_name)
{
	PFREEBLOCKMAP freeblock;
	ULONGLONG FragmentedClustersBeforeStartingPoint = 0;
	ULONGLONG ClustersBeforeStartingPoint;
	PFILENAME pfn;
	PBLOCKMAP block;
	BOOLEAN optimization_completed = FALSE;

	Stat.clusters_to_process = Stat.processed_clusters = 0;
	Stat.current_operation = 'C';

	/* On FAT volumes it increase distance between dir & files inside it. */
	if(partition_type != NTFS_PARTITION) return (-1);
	
	/* define threshold */
	threshold = clusters_total / 200; /* 0.5% */
	if(threshold < 2) threshold = 2;
	/* define starting point */
	StartingPoint = 0; /* start moving from the beginning of the volume */
	pass_number = 0;
	for(freeblock = free_space_map; freeblock != NULL; freeblock = freeblock->next_ptr){
		/* is block larger than 0.5% of the volume space? */
		if(freeblock->lcn > StartingPoint && freeblock->length >= threshold){
			StartingPoint = freeblock->lcn;
			break;
		}
		if(freeblock->next_ptr == free_space_map){
			DebugPrint("No blocks larger than 0.5%% of the volume found!\n");
			goto part_defrag;
		}
	}

	/* skip all locked files */
	//CheckAllFiles();
	
	/* validate StartingPoint */
	for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
		if(pfn->is_reparse_point == FALSE && pfn->is_fragm){
			ClustersBeforeStartingPoint = 0;
			for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
				if((block->lcn + block->length) <= StartingPoint)
					ClustersBeforeStartingPoint += block->length;
				if(block->next_ptr == pfn->blockmap) break;
			}
			if(ClustersBeforeStartingPoint){
				if(!IsFileLocked(pfn)) /* TODO: speedup here? */
					FragmentedClustersBeforeStartingPoint += ClustersBeforeStartingPoint;
			}
		}
		if(FragmentedClustersBeforeStartingPoint >= threshold) break;
		if(pfn->next_ptr == filelist) break;
	}
	if(FragmentedClustersBeforeStartingPoint >= threshold) StartingPoint = 0;

	do {
		DebugPrint("Optimization pass #%u, StartingPoint = %I64u\n",pass_number,StartingPoint);
		Stat.pass_number = pass_number;
		/* call optimization routine */
		if(OptimizationRoutine(volume_name) < 0) break;
		if(CheckForStopEvent()) break;
		/* if(!movings) break; */
		pass_number ++;
		/* redefine starting point */
		for(freeblock = free_space_map; freeblock != NULL; freeblock = freeblock->next_ptr){
			/* is block larger than 0.5% of the volume space? */
			if(freeblock->lcn > StartingPoint && freeblock->length >= threshold){
				StartingPoint = freeblock->lcn;
				break;
			}
			if(freeblock->next_ptr == free_space_map){
				optimization_completed = TRUE;
				break;
			}
		}
		if(optimization_completed) break;
	} while (1);
		
	/* here StartingPoint points to the last processed part of the volume */

part_defrag:
	/* perform a partial defragmentation of all files still fragmented */
	if(CheckForStopEvent()) return AnalyzeForced(volume_name); /* update fragmented files list */
	if(Analyze(volume_name) < 0) return (-1);
	if(MoveRestOfFilesRTL(volume_name) < 0){
		AnalyzeForced(volume_name);
		return (-1);
	}
	return AnalyzeForced(volume_name);
}

/**
 * @brief Performs a single pass of the volume optimization.
 * @param[in] volume_name the name of the volume.
 * @return Zero if at least one file has been
 * defragmented or optimized, negative value otherwise.
 */
int OptimizationRoutine(char *volume_name)
{
	PFREEBLOCKMAP freeblock;
	int defragmenter_result = 0;
	int optimizer_result = 0;

	DebugPrint("----- Optimization of %s: -----\n",volume_name);

	/* Initialize progress counters. */
	Stat.clusters_to_process = Stat.processed_clusters = 0;
	Stat.current_operation = 'C';
	if(free_space_map){
		for(freeblock = free_space_map->prev_ptr; 1; freeblock = freeblock->prev_ptr){
			if(freeblock->lcn >= StartingPoint) Stat.clusters_to_process += freeblock->length;
			if(freeblock->prev_ptr == free_space_map) break;
		}
	}
	
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
	if(Analyze(volume_name) < 0) return (-1);
	///DbgPrintFreeSpaceList();
	defragmenter_result = Defragment(volume_name);
	///DbgPrintFreeSpaceList();

	/*
	* Final step - move all files to the leading part of the volume.
	*/
	DebugPrint("----- Third step of optimization of %s: -----\n",volume_name);
	///AnalyzeFreeSpace(volume_name); /* refresh free space map */
	optimizer_result = MoveAllFilesRTL();
	
optimization_done:	
	/* Analyse volume again to update fragmented files list. */
	if(Analyze(volume_name) < 0) return (-1);
	
	if(defragmenter_result >= 0 || optimizer_result >= 0) return 0;
	return (-1);
}

/**
 * @brief Defragments the free space by moving
 * all files to the beginning part of the volume.
 * @note
 * - StartingPoint takes no effect here.
 * - Usually this function increases a number of fragmented files.
 */
void DefragmentFreeSpaceRTL(void)
{
	PFILENAME pfn, lastpfn;
	PBLOCKMAP block, lastblock;
	PFREEBLOCKMAP freeblock;
	ULONGLONG maxlcn;
	ULONGLONG vcn, length;
	ULONGLONG movings;
	
	/* skip all locked files */
	//CheckAllFiles();
	
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
		if(IsFileLocked(lastpfn)) continue;
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
				/*Stat.processed_clusters += length;*/
				RemarkBlock(freeblock->lcn,length,UNKNOWN_SPACE,FREE_SPACE);
				RemarkBlock(lastblock->lcn + (lastblock->length - length),length,
					FREE_OR_MFT_ZONE_SPACE,GetFileSpaceState(lastpfn));
				freeblock->length -= length;
				freeblock->lcn += length;
				lastblock->length -= length;
				if(!lastblock->length){
					winx_list_remove_item((list_entry **)&lastpfn->blockmap,(list_entry *)lastblock);
					break;
				}
			}
			if(CheckForStopEvent()) break;
			if(freeblock->next_ptr == free_space_map) break;
		}
		if(!movings) break;
	}
}

/**
 * @brief Defragments the free space by moving
 * all files to the terminal part of the volume.
 * @note
 * - StartingPoint works here.
 * - Usually this function increases a number of fragmented files.
 */
void DefragmentFreeSpaceLTR(void)
{
	PFILENAME pfn, firstpfn;
	PBLOCKMAP block, firstblock;
	PFREEBLOCKMAP freeblock;
	ULONGLONG minlcn;
	ULONGLONG vcn, length;
	ULONGLONG movings;
	
	/* skip all locked files */
	//CheckAllFiles();
	
	while(1){
		/* 1. Find the first file block on the volume: AFTER StartingPoint. */
		firstblock = NULL; firstpfn = NULL; minlcn = (clusters_total - 1);
		for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
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
			if(pfn->next_ptr == filelist) break;
		}
		if(!firstblock) break;
		if(IsFileLocked(firstpfn)) continue;
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
				/*Stat.processed_clusters += length;*/
				RemarkBlock(freeblock->lcn + (freeblock->length - length),length,
					UNKNOWN_SPACE,FREE_SPACE);
				RemarkBlock(firstblock->lcn,length,FREE_OR_MFT_ZONE_SPACE,GetFileSpaceState(firstpfn));
				freeblock->length -= length;
				firstblock->vcn += length;
				firstblock->lcn += length;
				firstblock->length -= length;
				if(!firstblock->length){
					winx_list_remove_item((list_entry **)&firstpfn->blockmap,(list_entry *)firstblock);
					break;
				}
			}
			if(CheckForStopEvent()) break;
			if(freeblock->prev_ptr == free_space_map->prev_ptr) break;
		}
		if(!movings) break;
	}
}

/**
 * @brief Moves a file containing a single fragment.
 * @details Updates the global statistics and map.
 * @param[in] pfn pointer to the structure describing the file.
 * @param[in] target the starting logical cluster number
 * defining position of target space on the volume.
 * @return Boolean value. TRUE indicates success,
 * FALSE indicates failure.
 * @note For optimizer only, not for defragmenter!
 */
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
	RemarkBlock(target,pfn->clusters_total,GetFileSpaceState(pfn),FREE_SPACE);
	TruncateFreeSpaceBlock(target,pfn->clusters_total);

	/* free previously allocated space (after TruncateFreeSpaceBlock() call!) */
	for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
		ProcessFreedBlock(block->lcn,block->length,FRAGM_SPACE/*old_state*/);
		if(block->next_ptr == pfn->blockmap) break;
	}

	DeleteBlockmap(pfn); /* because we don't need this info after file moving */
	return TRUE;
}

/**
 * @brief Moves all files to the beginning part of the volume.
 * @return Zero if at least one file has been
 * moved, negative value otherwise.
 */
int MoveAllFilesRTL(void)
{
	PFREEBLOCKMAP block;
	PFILENAME pfn, plargest;
	ULONGLONG length;
	ULONGLONG locked_clusters;
	ULONGLONG movings = 0;

	Stat.clusters_to_process = Stat.processed_clusters = 0;
	Stat.current_operation = 'C';

	/* skip all locked files */
	//CheckAllFiles();
	
	/* Reinitialize progress counters. */
	for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
		if(pfn->blockmap && !pfn->is_reparse_point)
			if(pfn->blockmap->lcn >= StartingPoint)
				Stat.clusters_to_process += pfn->clusters_total;
		if(pfn->next_ptr == filelist) break;
	}

	for(block = free_space_map; block != NULL; block = block->next_ptr){
	L0:
		if(block->length <= 1) goto L1; /* skip 1 cluster blocks and zero length blocks */
		//if(block->lcn < StartingPoint) goto L1; /* skip blocks before StartingPoint */
		/* find largest unfragmented file that can be stored here */
		plargest = NULL; length = 0;
		for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
			if(pfn->is_reparse_point == FALSE){
				if(pfn->blockmap && pfn->clusters_total <= block->length){
					if(pfn->clusters_total > length && pfn->blockmap->lcn >= StartingPoint){
						plargest = pfn;
						length = pfn->clusters_total;
					}
				}
			}
			if(pfn->next_ptr == filelist) break;
		}
		if(!plargest) goto L1; /* current block is too small */
		locked_clusters = 0;
		if(plargest->blockmap) locked_clusters = plargest->clusters_total;
		if(IsFileLocked(plargest)){
			Stat.processed_clusters += locked_clusters;
			goto L0;
		}
		/* skip fragmented files */
		//if(plargest->is_fragm) goto L1; /* never do it!!! */
		/* if uncomment the previous line the file moving will never start,
		   because free blocks will be always skipped by this instruction */
		/* move the file */
		if(MoveTheUnfragmentedFile(plargest,block->lcn)){
			DebugPrint("Moving success for %ws\n",plargest->name.Buffer);
			movings++;
		} else {
			DebugPrint("Moving error for %ws\n",plargest->name.Buffer);
		}
		/*Stat.processed_clusters += plargest->clusters_total;*/
		if(CheckForStopEvent()) break;
		/* after file moving continue from the first free space block */
		block = free_space_map; if(!block) break; else goto L0;
	L1:
		if(block->next_ptr == free_space_map) break;
	}
	return (movings == 0) ? (-1) : (0);
}

/**
 * @brief Moves all files to the beginning of the volume
 * regardless of their fragmentation status.
 * @param[in] volume_name the name of the volume.
 * @return Zero for success, negative value otherwise.
 * @note It is assumed that moved files become less
 * fragmented after the routine completion.
 */
int MoveRestOfFilesRTL(char *volume_name)
{
	PFILENAME pfn, plargest;
	PBLOCKMAP block;
	PFREEBLOCKMAP fb;
	ULONGLONG length;
	ULONGLONG vcn;
	BOOLEAN found;
	ULONGLONG part_defrag_threshold;
	
	DebugPrint("----- MoveRestOfFilesRTL() started for %s: -----\n",volume_name);
	DebugPrint("/* cleanup of the terminal part of the volume begins... */\n");
	
	Stat.clusters_to_process = Stat.processed_clusters = 0;
	Stat.current_operation = 'C';
	
	part_defrag_threshold = max(threshold / 10, 100); /* 0.05% of the volume or 100 clusters */
	
	/* actualize StartingPoint */
	for(fb = free_space_map; fb != NULL; fb = fb->next_ptr){
		if(fb->lcn + fb->length >= StartingPoint && fb->length >= part_defrag_threshold){
			StartingPoint = fb->lcn;
			break;
		}
		if(fb->next_ptr == free_space_map){
			DebugPrint("There are no large free space areas after the starting point!\n");
			return 0;
		}
	}
	DebugPrint("StartingPoint = %I64u\n",StartingPoint);

	/* Reinitialize progress counters. */
	for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
		if(!pfn->is_reparse_point){
			for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
				if(block->lcn >= StartingPoint)
					Stat.clusters_to_process += block->length;
				if(block->next_ptr == pfn->blockmap) break;
			}
		}
		if(pfn->next_ptr == filelist) break;
	}
	
	/*
	* Move all files having clusters after a starting point
	* to the free space pointed by StartingPoint.
	*/
	for(fb = free_space_map; fb != NULL; fb = fb->next_ptr){
		if(fb->lcn == StartingPoint) break;
		if(fb->next_ptr == free_space_map){
			DebugPrint("MoveRestOfFilesRTL: unexpected condition encountered!\n");
			return 0;
		}
	}
	while(1){
		/* search for a largest file having clusters after fb->lcn */
		plargest = NULL; length = 0;
		for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
			if(pfn->is_reparse_point == FALSE){
				found = FALSE;
				for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
					if(block->lcn > fb->lcn){
						found = TRUE;
						break;
					}
					if(block->next_ptr == pfn->blockmap) break;
				}
				if(found && pfn->clusters_total > length){
					plargest = pfn;
					length = pfn->clusters_total;
				}
			}
			if(pfn->next_ptr == filelist) break;
		}
		if(!plargest) /* there are no more blocks after fb->lcn */
			return 0;
		/* fill free space starting from fb->lcn with plargest file contents */
		for(block = plargest->blockmap; block != NULL; block = block->next_ptr){
			if(block->lcn > StartingPoint){
				/* move the block entirely to the beginning of the volume */
				while(block->length){
					length = min(fb->length,block->length);
					vcn = block->vcn;
					MovePartOfFileBlock(plargest,vcn,fb->lcn,length);
					RemarkBlock(fb->lcn,length,UNKNOWN_SPACE,FREE_SPACE);
					RemarkBlock(block->lcn,length,TEMPORARY_SYSTEM_SPACE/*FREE_OR_MFT_ZONE_SPACE*/,GetFileSpaceState(plargest));
					fb->lcn += length;
					fb->length -= length;
					block->vcn += length;
					block->lcn += length;
					block->length -= length;
					/* skip small free blocks */
					if(fb->length == 0){
						while(1){
							if(fb->length >= part_defrag_threshold) break;
							/* if there are no more free blocks available then return */
							if(fb->next_ptr == free_space_map)
								return 0;
							fb = fb->next_ptr;
						}
					}
				}
			}
			if(block->next_ptr == plargest->blockmap) break;
		}
		DeleteBlockmap(plargest);
	}
	return 0;
}

/** @} */
