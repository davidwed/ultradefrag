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
 * @file tasks.c
 * @brief Volume processing tasks.
 * @details Set of atomic volume processing tasks 
 * defined here is built over the move_file routine
 * and is intended to build defragmentation and 
 * optimization routines over it.
 * @addtogroup Tasks
 * @{
 */

#include "udefrag-internals.h"

static int defragment_small_files_respect_best_matching(udefrag_job_parameters *jp);

/************************************************************/
/*                    Internal Routines                     */
/************************************************************/

/**
 * @brief Searches for free space region starting at the beginning of the volume.
 * @param[in] jp job parameters structure.
 * @param[in] start_cluster lcn to start searching from.
 * @param[in] min_length minimum length of free space in clusters.
 * @note In case of termination request returns NULL immediately.
 */
winx_volume_region *find_free_region_forward(udefrag_job_parameters *jp,
    ULONGLONG start_cluster, ULONGLONG min_length)
{
	winx_volume_region *rgn, *rgn_found;
	ULONGLONG length, clcn;
	
	rgn_found = NULL, length = 0, clcn = 0;
	for(rgn = jp->free_regions; rgn; rgn = rgn->next){
		if(jp->termination_router((void *)jp))
			return NULL;
		if(rgn->length >= min_length){
            if( length == 0 || rgn->length < length){
                rgn_found = rgn;
                length = rgn->length;
                clcn = rgn->lcn;
            }
		}
        if(rgn_found != NULL){
            if(clcn >= start_cluster){
                break;
            } else {
                if(length == min_length) break;
            }
        }
		if(rgn->next == jp->free_regions) break;
	}
	return rgn_found;
}

/**
 * @brief Searches for free space region starting at the end of the volume.
 * @param[in] jp job parameters structure.
 * @param[in] start_cluster lcn to start searching from.
 * @param[in] min_length minimum length of free space in clusters.
 * @note In case of termination request returns NULL immediately.
 */
winx_volume_region *find_free_region_backward(udefrag_job_parameters *jp,
    ULONGLONG start_cluster, ULONGLONG min_length)
{
	winx_volume_region *rgn, *rgn_found;
	ULONGLONG length, clcn;
	
	rgn_found = NULL, length = 0, clcn = jp->v_info.total_clusters;
	for(rgn = jp->free_regions->prev; rgn; rgn = rgn->prev){
		if(jp->termination_router((void *)jp))
			return NULL;
		if(rgn->length >= min_length){
            if( length == 0 || rgn->length < length){
                rgn_found = rgn;
                length = rgn->length;
                clcn = rgn->lcn;
            }
		}
        if(rgn_found != NULL){
            if(clcn <= start_cluster){
                break;
            } else {
                if(length == min_length) break;
            }
        }
		if(rgn->prev == jp->free_regions->prev) break;
	}
	return rgn_found;
}

/**
 * @brief Searches for best matching free space region.
 * @param[in] file_start_cluster lcn of first file cluster.
 * @param[in] file_length length of file in clusters.
 * @param[in] preferred_position flag for preferred region position
 * 1 ... region before the files start preferred
 * 0 ... any region accepted
 * @note In case of termination request returns
 * NULL immediately.
 */
static winx_volume_region *find_matching_free_region(udefrag_job_parameters *jp,
    ULONGLONG file_start_cluster, ULONGLONG file_length, int preferred_position)
{
	winx_volume_region *rgn, *rgn_matching;
	ULONGLONG length;
	
	rgn_matching = NULL, length = 0;
	for(rgn = jp->free_regions; rgn; rgn = rgn->next){
		if(jp->termination_router((void *)jp)) return NULL;
		if(preferred_position == 1 && rgn->lcn > file_start_cluster)
			if(rgn_matching != NULL)
				break;
		if(rgn->length >= file_length){
            if(length == 0 || rgn->length < length){
                rgn_matching = rgn;
                length = rgn->length;
				if(length == file_length) break;
            }
		}
		if(rgn->next == jp->free_regions) break;
	}
	return rgn_matching;
}

/**
 * @brief Searches for largest free space region.
 * @note In case of termination request returns
 * NULL immediately.
 */
static winx_volume_region *find_largest_free_region(udefrag_job_parameters *jp)
{
	winx_volume_region *rgn, *rgn_largest;
	ULONGLONG length;
	
	rgn_largest = NULL, length = 0;
	for(rgn = jp->free_regions; rgn; rgn = rgn->next){
		if(jp->termination_router((void *)jp))
			return NULL;
		if(rgn->length > length){
			rgn_largest = rgn;
			length = rgn->length;
		}
		if(rgn->next == jp->free_regions) break;
	}
	return rgn_largest;
}

/**
 * @brief Defines whether the file
 * can be defragmented partially or not.
 */
int can_defragment_partially(winx_file_info *f,udefrag_job_parameters *jp)
{
	/* skip files with undefined cluster map */
	if(f->disp.blockmap == NULL)
		return 0;
	
	/* skip files with less than 2 fragments */
	if(f->disp.blockmap->next == f->disp.blockmap || f->disp.fragments < 2 || !is_fragmented(f))
		return 0;

	/* skip FAT directories */
	if(is_directory(f) && !jp->actions.allow_dir_defrag)
		return 0;
	
	/* skip file in case of improper state detected */
	if(is_in_improper_state(f))
		return 0;

	/* skip files already marked as too large to avoid infinite loops */
	if(is_too_large(f))
		return 0;
	
	return 1;
}

/**
 * @brief Defines whether the file
 * can be defragmented entirely or not.
 * @details While can_defragment_partially
 * lets to defragment files for which cluster
 * move failures occured, can_defragment_entirely 
 * routine excludes such a files. As well as files
 * intended for partial defragmentation especially.
 */
static int can_defragment_entirely(winx_file_info *f,udefrag_job_parameters *jp)
{
	if(is_intended_for_part_defrag(f))
		return 0;
	
	/* skip files for which moving failed already to avoid infinite loops */
	if(is_moving_failed(f))
		return 0;
	
	return can_defragment_partially(f,jp);
}

/**
 * @brief Defines whether the file
 * can be moved or not.
 */
int can_move(winx_file_info *f,udefrag_job_parameters *jp)
{
	/* skip files with undefined cluster map */
	if(f->disp.blockmap == NULL)
		return 0;
	
	/* skip FAT directories */
	if(is_directory(f) && !jp->actions.allow_dir_defrag)
		return 0;
	
	/* skip file in case of improper state detected */
	if(is_in_improper_state(f))
		return 0;

	return 1;
}

/**
 * @brief Removes unfragmented files
 * from the list of fragmented.
 */
static void update_fragmented_files_list(udefrag_job_parameters *jp)
{
	udefrag_fragmented_file *f, *next, *head;
	
	f = head = jp->fragmented_files;
	while(f){
		next = f->next;
		if(!is_fragmented(f->f)){
			winx_list_remove_item((list_entry **)(void *)&jp->fragmented_files,(list_entry *)f);
			if(jp->fragmented_files == NULL)
				break; /* list is empty */
			if(jp->fragmented_files != head){
				head = jp->fragmented_files;
				f = next;
				continue;
			}
		}
		f = next;
		if(f == head) break;
	}
}

/************************************************************/
/*                      Atomic Tasks                        */
/************************************************************/

/**
 * @brief Defragments all fragmented files entirely, if possible.
 * @details 
 * - This routine fills free space areas from the beginning of the
 * volume regardless of the best matching rules.
 */
int defragment_small_files(udefrag_job_parameters *jp)
{
	ULONGLONG time;
	ULONGLONG defragmented_files;
	winx_volume_region *rgn;
	udefrag_fragmented_file *f, *f_largest;
	ULONGLONG length;
	char buffer[32];
	
	jp->pi.current_operation = VOLUME_DEFRAGMENTATION;

	if(jp->udo.preview_flags & UD_PREVIEW_MATCHING)
		return defragment_small_files_respect_best_matching(jp);

	/* open the volume */
	// fVolume = new_winx_vopen(winx_toupper(jp->volume_letter));
	jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
	if(jp->fVolume == NULL)
		return (-1);

	time = start_timing("defragmentation",jp);

	/* fill free regions in the beginning of the volume */
	jp->pi.moved_clusters = 0;
	defragmented_files = 0;
	for(rgn = jp->free_regions; rgn; rgn = rgn->next){
		if(jp->termination_router((void *)jp)) goto done;
		
		/* skip micro regions */
		if(rgn->length < 2) goto next_rgn;

		/* find largest fragmented file that fits in the current region */
		do {
			if(jp->termination_router((void *)jp)) goto done;
			f_largest = NULL, length = 0;
			for(f = jp->fragmented_files; f; f = f->next){
				if(f->f->disp.clusters > length && f->f->disp.clusters <= rgn->length){
					if(can_defragment_entirely(f->f,jp)){
						f_largest = f;
						length = f->f->disp.clusters;
					}
				}
				if(f->next == jp->fragmented_files) break;
			}
			if(f_largest == NULL) goto next_rgn;
			
			/* move the file */
			if(move_file(f_largest->f,f_largest->f->disp.blockmap->vcn,
			 f_largest->f->disp.clusters,rgn->lcn,0,jp) >= 0){
				DebugPrint("Defrag success for %ws",f_largest->f->path);
				defragmented_files ++;
			} else {
				DebugPrint("Defrag failure for %ws",f_largest->f->path);
			}
			
			/* skip locked files here to prevent skipping the current free region */
		} while(is_locked(f_largest->f));
		
		/* truncate list of fragmented files */
		update_fragmented_files_list(jp);
		
		/* after file moving continue from the first free region */
		rgn = jp->free_regions->prev;
		continue;
		
	next_rgn:
		if(rgn->next == jp->free_regions) break;
	}

done:
	/* display amount of moved data and number of defragmented files */
	DebugPrint("%I64u files defragmented",defragmented_files);
	DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
	winx_fbsize(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
	DebugPrint("%s moved",buffer);
	stop_timing("defragmentation",time,jp);
	winx_fclose(jp->fVolume);
	jp->fVolume = NULL;
	return 0;
}

/**
 * @brief Defragments all fragmented files entirely, if possible.
 * @details 
 * - If some file cannot be defragmented due to its size,
 * this routine marks it by UD_FILE_INTENDED_FOR_PART_DEFRAG flag.
 * - This routine fills free space areas respect to the best matching rule.
 */
static int defragment_small_files_respect_best_matching(udefrag_job_parameters *jp)
{
	ULONGLONG time;
	ULONGLONG defragmented_files;
	winx_volume_region *rgn;
	udefrag_fragmented_file *f, *f_largest;
	ULONGLONG length;
	char buffer[32];

	/* open the volume */
	// fVolume = new_winx_vopen(winx_toupper(jp->volume_letter));
	jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
	if(jp->fVolume == NULL)
		return (-1);

	time = start_timing("defragmentation",jp);

	/* find best matching free region for each fragmented file */
	jp->pi.moved_clusters = 0;
	defragmented_files = 0;
	while(1){
		if(jp->termination_router((void *)jp)) break;
		f_largest = NULL, length = 0;
		for(f = jp->fragmented_files; f; f = f->next){
			if(f->f->disp.clusters > length){
				if(can_defragment_entirely(f->f,jp)){
					f_largest = f;
					length = f->f->disp.clusters;
				}
			}
			if(f->next == jp->fragmented_files) break;
		}
		if(f_largest == NULL) break;

		rgn = find_matching_free_region(jp,f_largest->f->disp.blockmap->lcn,f_largest->f->disp.clusters,0);
		if(jp->termination_router((void *)jp)) break;
		if(rgn == NULL){
			f_largest->f->user_defined_flags |= UD_FILE_INTENDED_FOR_PART_DEFRAG;
		} else {
			/* move the file */
			if(move_file(f_largest->f,f_largest->f->disp.blockmap->vcn,
			 f_largest->f->disp.clusters,rgn->lcn,0,jp) >= 0){
				DebugPrint("Defrag success for %ws",f_largest->f->path);
				defragmented_files ++;
			} else {
				DebugPrint("Defrag failure for %ws",f_largest->f->path);
			}
			/* truncate list of fragmented files */
			update_fragmented_files_list(jp);
		}
	}
	
	/* display amount of moved data and number of defragmented files */
	DebugPrint("%I64u files defragmented",defragmented_files);
	DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
	winx_fbsize(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
	DebugPrint("%s moved",buffer);
	stop_timing("defragmentation",time,jp);
	winx_fclose(jp->fVolume);
	jp->fVolume = NULL;
	return 0;
}

/**
 * @brief Defragments all fragmented files at least 
 * partially, when possible.
 * @details
 * - If some file cannot be defragmented due to its size,
 * this routine marks it by UD_FILE_TOO_LARGE flag.
 * - This routine fills free space areas starting
 * from the biggest one to concatenate as much fragments
 * as possible.
 */
int defragment_big_files(udefrag_job_parameters *jp)
{
	ULONGLONG time;
	ULONGLONG defragmented_files;
	winx_volume_region *rgn_largest;
	udefrag_fragmented_file *f, *f_largest;
	ULONGLONG target, length;
	winx_blockmap *block, *first_block, *longest_sequence;
	ULONGLONG n_blocks, max_n_blocks, longest_sequence_length;
	ULONGLONG remaining_clusters;
	char buffer[32];

	/* open the volume */
	// fVolume = new_winx_vopen(winx_toupper(jp->volume_letter));
	jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
	if(jp->fVolume == NULL)
		return (-1);

	time = start_timing("partial defragmentation",jp);
	
	/* fill largest free region first */
	jp->pi.current_operation = VOLUME_DEFRAGMENTATION;
	jp->pi.moved_clusters = 0;
	defragmented_files = 0;
	while(1) {
		/* find largest free space region */
		rgn_largest = find_largest_free_region(jp);
		if(jp->termination_router((void *)jp)) goto done;
		if(rgn_largest == NULL) break;
		if(rgn_largest->length < 2) break;
		
		/* find largest fragmented file which fragments can be joined there */
		do {
			if(jp->termination_router((void *)jp)) goto done;
			f_largest = NULL, length = 0;
			for(f = jp->fragmented_files; f; f = f->next){
				if(f->f->disp.clusters > length && can_defragment_partially(f->f,jp)){
					f_largest = f;
					length = f->f->disp.clusters;
				}
				if(f->next == jp->fragmented_files) break;
			}
			if(f_largest == NULL) goto part_defrag_done;
			
			/* find longest sequence of file blocks which fits in the current free region */
			longest_sequence = NULL, max_n_blocks = 0, longest_sequence_length = 0;
			for(first_block = f_largest->f->disp.blockmap; first_block; first_block = first_block->next){
				if(!is_block_excluded(first_block)){
					n_blocks = 0, remaining_clusters = rgn_largest->length;
					for(block = first_block; block; block = block->next){
						if(jp->termination_router((void *)jp)) goto done;
						if(is_block_excluded(block)) break;
						if(block->length <= remaining_clusters){
							n_blocks ++;
							remaining_clusters -= block->length;
						} else {
							break;
						}
						if(block->next == f_largest->f->disp.blockmap) break;
					}
					if(n_blocks > 1 && n_blocks > max_n_blocks){
						longest_sequence = first_block;
						max_n_blocks = n_blocks;
						longest_sequence_length = rgn_largest->length - remaining_clusters;
					}
				}
				if(first_block->next == f_largest->f->disp.blockmap) break;
			}
			
			if(longest_sequence == NULL){
				/* fragments of the current file cannot be joined */
				f_largest->f->user_defined_flags |= UD_FILE_TOO_LARGE;
			}
		} while(is_too_large(f_largest->f));

		/* join fragments */
		target = rgn_largest->lcn; 
		if(move_file(f_largest->f,longest_sequence->vcn,longest_sequence_length,
		  target,UD_MOVE_FILE_CUT_OFF_MOVED_CLUSTERS,jp) >= 0){
			DebugPrint("Partial defrag success for %ws",f_largest->f->path);
			defragmented_files ++;
		} else {
			DebugPrint("Partial defrag failure for %ws",f_largest->f->path);
		}
		
		/*
		* Remove target space from the free space pool.
		* This will safely prevent infinite loops
		* on a single free space block.
		*/
		jp->free_regions = winx_sub_volume_region(jp->free_regions,target,longest_sequence_length);
	}

part_defrag_done:
	/* mark all files not processed yet as too large */
	for(f = jp->fragmented_files; f; f = f->next){
		if(jp->termination_router((void *)jp)) break;
		if(can_defragment_partially(f->f,jp))
			f->f->user_defined_flags |= UD_FILE_TOO_LARGE;
		if(f->next == jp->fragmented_files) break;
	}

done:
	/* display amount of moved data and number of partially defragmented files */
	DebugPrint("%I64u files partially defragmented",defragmented_files);
	DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
	winx_fbsize(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
	DebugPrint("%s moved",buffer);
	stop_timing("partial defragmentation",time,jp);
	winx_fclose(jp->fVolume);
	jp->fVolume = NULL;
	return 0;
}

/**
 * @brief Moves selected group of files
 * to the beginning of the volume to free its end.
 * @param[in] jp job parameters.
 * @param[in] flags combination of MOVE_xxx flags defined in udefrag.h
 * @note Volume must be opened before this call,
 * jp->fVolume must contain a proper handle.
 * @return Zero for success, negative value otherwise.
 */
int move_files_to_front(udefrag_job_parameters *jp, int flags)
{
	return 0;
}

/**
 * @brief Moves selected group of files
 * to the end of the volume to free its
 * beginning.
 * @param[in] jp job parameters.
 * @param[in] flags combination of MOVE_xxx flags defined in udefrag.h
 * @note Volume must be opened before this call,
 * jp->fVolume must contain a proper handle.
 * @return Zero for success, negative value otherwise.
 */
int move_files_to_back(udefrag_job_parameters *jp, int flags)
{
	return 0;
}

/** @} */
