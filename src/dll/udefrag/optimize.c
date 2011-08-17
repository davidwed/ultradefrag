/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @brief Volume optimization.
 * @addtogroup Optimizer
 * @{
 */

#include "udefrag-internals.h"

static void calculate_free_rgn_size_threshold(udefrag_job_parameters *jp);
static ULONGLONG get_number_of_allocated_clusters(
	udefrag_job_parameters *jp, ULONGLONG first_lcn,
	ULONGLONG last_lcn);

/**
 * @brief Performs a volume optimization.
 * @return Zero for success, negative value otherwise.
 */
int optimize(udefrag_job_parameters *jp)
{
	int result, overall_result = -1;
	ULONGLONG start_lcn = 0, new_start_lcn;
	ULONGLONG remaining_clusters;

	/* perform volume analysis */
	result = analyze(jp); /* we need to call it once, here */
	if(result < 0) return result;
	
	/* set free region size threshold, reset counters */
	calculate_free_rgn_size_threshold(jp);
	jp->pi.processed_clusters = 0;
	/* we have a chance to move everything to the end and then in contrary direction */
	jp->pi.clusters_to_process = (jp->v_info.total_clusters - \
		jp->v_info.free_bytes / jp->v_info.bytes_per_cluster) * 2;
	
	/* optimize MFT separately to keep its optimal location */
	result = optimize_mft(jp);
	if(result == 0){
		/* at least mft optimization succeeded */
		overall_result = 0;
	}

	/* do the job */
	jp->pi.pass_number = 0;
	while(!jp->termination_router((void *)jp)){
		/* exclude already optimized part of the volume */
		new_start_lcn = calculate_starting_point(jp,start_lcn);
		if(0){//if(new_start_lcn <= start_lcn && jp->pi.pass_number){
			/* TODO */
			DebugPrint("volume optimization completed: old_sp = %I64u, new_sp = %I64u",
				start_lcn, new_start_lcn);
			break;
		}
		if(new_start_lcn > start_lcn)
			start_lcn = new_start_lcn;
		
		/* reset counters */
		remaining_clusters = get_number_of_allocated_clusters(jp,start_lcn,jp->v_info.total_clusters - 1);
		jp->pi.processed_clusters = 0; /* reset counter */
		jp->pi.clusters_to_process = (jp->v_info.total_clusters - \
			jp->v_info.free_bytes / jp->v_info.bytes_per_cluster) * 2;
		jp->pi.processed_clusters = jp->pi.clusters_to_process - \
			remaining_clusters * 2; /* set counter */
		
		DebugPrint("volume optimization pass #%u, starting point = %I64u, remaining clusters = %I64u",
			jp->pi.pass_number, start_lcn, remaining_clusters);
		
		/* move fragmented files to the terminal part of the volume */
		result = move_files_to_back(jp, 0, MOVE_FRAGMENTED);
		if(result == 0){
			/* at least something succeeded */
			overall_result = 0;
		}
		
		/* full opt: cleanup space after start_lcn */
		if(jp->job_type == FULL_OPTIMIZATION_JOB){
			result = move_files_to_back(jp, start_lcn, MOVE_ALL);
			if(result == 0){
				/* at least something succeeded */
				overall_result = 0;
			}
		}
		
		/* move files back to the beginning */
		result = move_files_to_front(jp, start_lcn, MOVE_ALL);
		if(result == 0){
			/* at least something succeeded */
			overall_result = 0;
		}
		
		/* break if nothing moved */
		if(result < 0 || jp->pi.moved_clusters == 0) break;
		
		/* break if no repeat */
		if(!(jp->udo.job_flags & UD_JOB_REPEAT)) break;
		
		/* go to the next pass */
		jp->pi.pass_number ++;
	}
	
	if(jp->termination_router((void *)jp)) return 0;
	
	return overall_result;
}

/************************* Internal routines ****************************/

/**
 * @brief Calculates free region size
 * threshold used in volume optimization.
 */
static void calculate_free_rgn_size_threshold(udefrag_job_parameters *jp)
{
	winx_volume_region *rgn;
	ULONGLONG length = 0;

	if(jp->v_info.free_bytes >= jp->v_info.total_bytes / 10){
		/*
		* We have at least 10% of free space on the volume, so
		* it seems to be reasonable to put all data together
		* even if the free space is split to many little pieces.
		*/
		DebugPrint("calculate_free_rgn_size_threshold: strategy #1 because of at least 10%% of free space on the volume");
		for(rgn = jp->free_regions; rgn; rgn = rgn->next){
			if(rgn->length > length)
				length = rgn->length;
			if(rgn->next == jp->free_regions) break;
		}
		/* Threshold = 0.5% of the volume or a half of the largest free space region. */
		jp->free_rgn_size_threshold = min(jp->v_info.total_clusters / 200, length / 2);
	} else {
		/*
		* On volumes with less than 10% of free space
		* we're searching for the free space region
		* at least 0.5% long.
		*/
		DebugPrint("calculate_free_rgn_size_threshold: strategy #2 because of less than 10%% of free space on the volume");
		jp->free_rgn_size_threshold = jp->v_info.total_clusters / 200;
	}
	//jp->free_rgn_size_threshold >>= 1;
	if(jp->free_rgn_size_threshold < 2) jp->free_rgn_size_threshold = 2;
	DebugPrint("free region size threshold = %I64u clusters",jp->free_rgn_size_threshold);
}

/**
 * @brief Returns number of allocated clusters
 * locating inside a specified part of the volume.
 */
static ULONGLONG get_number_of_allocated_clusters(udefrag_job_parameters *jp, ULONGLONG first_lcn, ULONGLONG last_lcn)
{
	winx_file_info *file;
	winx_blockmap *block;
	ULONGLONG i, j, n, total = 0;
	
	for(file = jp->filelist; file; file = file->next){
		if(jp->termination_router((void *)jp)) break;
		n = 0;
		for(block = file->disp.blockmap; block; block = block->next){
			if((block->lcn + block->length >= first_lcn + 1) && block->lcn <= last_lcn){
				if(block->lcn > first_lcn) i = block->lcn; else i = first_lcn;
				if(block->lcn + block->length < last_lcn + 1) j = block->lcn + block->length; else j = last_lcn + 1;
				n += (j - i);
			}
			if(block->next == file->disp.blockmap) break;
		}
		total += n;
		if(file->next == jp->filelist) break;
	}
	
	return total;
}

/**
 * @brief Returns number of fragmented clusters
 * locating inside a specified part of the volume.
 */
static ULONGLONG get_number_of_fragmented_clusters(udefrag_job_parameters *jp, ULONGLONG first_lcn, ULONGLONG last_lcn)
{
	udefrag_fragmented_file *f;
	winx_blockmap *block;
	ULONGLONG i, j, n, total = 0;
	
	for(f = jp->fragmented_files; f; f = f->next){
		if(jp->termination_router((void *)jp)) break;
		n = 0;
		for(block = f->f->disp.blockmap; block; block = block->next){
			if((block->lcn + block->length >= first_lcn + 1) && block->lcn <= last_lcn){
				if(block->lcn > first_lcn) i = block->lcn; else i = first_lcn;
				if(block->lcn + block->length < last_lcn + 1) j = block->lcn + block->length; else j = last_lcn + 1;
				n += (j - i);
			}
			if(block->next == f->f->disp.blockmap) break;
		}
		if(n && !is_file_locked(f->f,jp))
			total += n;
		if(f->next == jp->fragmented_files) break;
	}
	
	return total;
}

/**
 * @brief Returns number of free clusters
 * locating inside a specified part of the volume.
 */
static ULONGLONG get_number_of_free_clusters(udefrag_job_parameters *jp, ULONGLONG first_lcn, ULONGLONG last_lcn)
{
	winx_volume_region *rgn;
	ULONGLONG i, j, total = 0;
	
	for(rgn = jp->free_regions; rgn; rgn = rgn->next){
		if(rgn->lcn > last_lcn) break;
		if(rgn->lcn + rgn->length >= first_lcn + 1){
			if(rgn->lcn > first_lcn) i = rgn->lcn; else i = first_lcn;
			if(rgn->lcn + rgn->length < last_lcn + 1) j = rgn->lcn + rgn->length; else j = last_lcn + 1;
			total += (j - i);
		}
		if(rgn->next == jp->free_regions) break;
	}
	
	return total;
}

/**
 * @brief Calculates starting point
 * for a volume optimization process
 * to skip already optimized data.
 * All the clusters before it will
 * be skipped in move_files_to_back
 * routine.
 */
ULONGLONG calculate_starting_point(udefrag_job_parameters *jp, ULONGLONG old_sp)
{
	ULONGLONG new_sp;
	ULONGLONG fragmented, free, lim, i, max_new_sp;
	winx_volume_region *rgn;
	winx_file_info *file;
	winx_blockmap *block;
	
	/* free temporarily allocated space */
	release_temp_space_regions(jp);

	/* search for the first large free space gap after an old starting point */
	new_sp = old_sp;
	for(rgn = jp->free_regions; rgn; rgn = rgn->next){
		if(jp->udo.dbgprint_level >= DBG_PARANOID)
			DebugPrint("Free block start: %I64u len: %I64u",rgn->lcn,rgn->length);
		if(rgn->lcn >= old_sp && rgn->length >= jp->free_rgn_size_threshold){
			new_sp = rgn->lcn;
			break;
		}
		if(rgn->next == jp->free_regions) break;
	}
	
	/* move starting point back to release heavily fragmented data */
	/* allow no more than 5% of fragmented data inside of a skipped part of the disk */
	fragmented = get_number_of_fragmented_clusters(jp,old_sp,new_sp);
	if(fragmented < /*jp->free_rgn_size_threshold*/ (new_sp - old_sp) / 20) goto done;

	/*
	* Fast binary search finds quickly a proper part 
	* of the volume which is heavily fragmented.
	* Based on bsearch() algorithm from ReactOS.
	*/
	i = old_sp;
	for(lim = new_sp - old_sp; lim != 0; lim >>= 1){
		new_sp = i + (lim >> 1);
		fragmented = get_number_of_fragmented_clusters(jp,old_sp,new_sp);
		if(fragmented >= /*jp->free_rgn_size_threshold*/ (new_sp - old_sp) / 20){
			/* move left */
		} else {
			/* move right */
			i = new_sp + 1; lim --;
		}
	}
	if(new_sp <= old_sp + 1){
		new_sp = old_sp;
		goto done;
	}
	
	/*
	* Release all remaining data when all space
	* between new_sp and old_sp is heavily fragmented.
	*/
	if(fragmented >= (new_sp - old_sp + 1) / 3){
		new_sp = old_sp; /* because at least 1/3 of skipped space is fragmented */
		goto done;
	}
	
	/* cut off heavily fragmented free space */
	i = old_sp; max_new_sp = new_sp;
	for(lim = new_sp - old_sp; lim != 0; lim >>= 1){
		new_sp = i + (lim >> 1);
		free = get_number_of_free_clusters(jp,new_sp,max_new_sp);
		if(free >= (max_new_sp - new_sp + 1) / 3){
			/* move left */
		} else {
			/* move right */
			i = new_sp + 1; lim --;
		}
	}
	if(new_sp <= old_sp + 1){
		new_sp = old_sp;
		goto done;
	}
	
	/* is starting point inside a file block? */
	for(file = jp->filelist; file; file = file->next){
		for(block = file->disp.blockmap; block; block = block->next){
			if(new_sp >= block->lcn && new_sp <= block->lcn + block->length - 1){
				if(is_fragmented(file)){
					/* include block */
					return block->lcn;
				} else {
					/* don't skip to avoid slow walk from a block to the next one etc. */
					///* skip block */
					//return (block->lcn + block->length);
					return new_sp;
				}
			}
			if(block->next == file->disp.blockmap) break;
		}
		if(file->next == jp->filelist) break;
	}

done:
	return new_sp;
	/* FIXME: incompatible with next sp calculation */
	/* if starting point is inside a free region, skip it entirely */
	for(rgn = jp->free_regions; rgn; rgn = rgn->next){
		if(new_sp >= rgn->lcn && new_sp < rgn->lcn + rgn->length){
			new_sp = rgn->lcn + rgn->length;
			break;
		}
		if(rgn->next == jp->free_regions) break;
	}
	return new_sp;
}

/** @} */
