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
 * @file defrag.c
 * @brief Volume defragmentation.
 * @addtogroup Defrag
 * @{
 */

#include "udefrag-internals.h"

/**
 * @brief Defines whether the file
 * can be defragmented or not.
 */
int can_defragment(winx_file_info *f,udefrag_job_parameters *jp)
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
	
	/* skip files for which moving failed already to avoid infinite loops */
	if(is_moving_failed(f))
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
 * @brief Searches for best matching free space region.
 * @param[in] file_start_cluster lcn of first file cluster.
 * @param[in] file_length length of file in clusters.
 * @param[in] preferred_position flag for preferred region position
 * 1 ... region before the files start preferred
 * 0 ... any region accepted
 * @note In case of termination request returns
 * NULL immediately.
 */
winx_volume_region *find_matching_free_region(udefrag_job_parameters *jp,
    ULONGLONG file_start_cluster, ULONGLONG file_length, int preferred_position)
{
	winx_volume_region *rgn, *rgn_matching;
	ULONGLONG length, clcn;
	
	rgn_matching = NULL, length = 0, clcn = 0;
	for(rgn = jp->free_regions; rgn; rgn = rgn->next){
		if(jp->termination_router((void *)jp))
			return NULL;
		if(rgn->length >= file_length){
            if( length == 0 || rgn->length < length){
                rgn_matching = rgn;
                length = rgn->length;
                clcn = rgn->lcn;
            }
		}
        if(rgn_matching != NULL){
            if(preferred_position == 1 && clcn > file_start_cluster){
                break;
            } else {
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
winx_volume_region *find_largest_free_region(udefrag_job_parameters *jp)
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
 * @brief Performs a volume defragmentation.
 * @return Zero for success, negative value otherwise.
 */
int defragment(udefrag_job_parameters *jp)
{
	udefrag_fragmented_file *f, *f_largest;
	winx_volume_region *rgn, *rgn_largest;
	ULONGLONG length;
	ULONGLONG time;
	ULONGLONG moved_clusters;
	ULONGLONG defragmented_files;
	ULONGLONG joined_fragments;
	winx_blockmap *block, *first_block, *longest_sequence;
	ULONGLONG n_blocks, max_n_blocks, longest_sequence_length;
	ULONGLONG remaining_clusters;
	char buffer[32];
	int result;
	
	/* analyze volume */
	result = analyze(jp);
	if(result < 0)
		return result;
	
	/* open the volume */
	// fVolume = new_winx_vopen(winx_toupper(jp->volume_letter));
	jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
	if(jp->fVolume == NULL)
		return (-1);
	
	/* reset statistics */
	jp->pi.clusters_to_process = 0;
	jp->pi.processed_clusters = 0;
	jp->pi.moved_clusters = 0;
	jp->pi.current_operation = VOLUME_DEFRAGMENTATION;
	for(f = jp->fragmented_files; f; f = f->next){
		if(jp->termination_router((void *)jp)) goto done;
		if(can_defragment(f->f,jp))
			jp->pi.clusters_to_process += f->f->disp.clusters;
		if(f->next == jp->fragmented_files) break;
	}
	
	/* defragment all small files */
	time = start_timing("defragmentation",jp);

	/* fill free regions in the beginning of the volume */
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
					if(can_defragment(f->f,jp) && !is_intended_for_part_defrag(f->f)){
						f_largest = f;
						length = f->f->disp.clusters;
					}
				}
				if(f->next == jp->fragmented_files) break;
			}
			if(f_largest == NULL) goto next_rgn;
			
			/* move the file */
			if(move_file(f_largest->f,f_largest->f->disp.blockmap->vcn,
			 f_largest->f->disp.clusters,rgn->lcn,jp) >= 0){
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

	/* display amount of moved data and number of defragmented files */
	moved_clusters = jp->pi.moved_clusters;
	DebugPrint("%I64u files defragmented",defragmented_files);
	DebugPrint("%I64u clusters moved",moved_clusters);
	winx_fbsize(moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
	DebugPrint("%s moved",buffer);
	stop_timing("defragmentation",time,jp);
	
	/* defragment all large files partially */
	/* UD_FILE_TOO_LARGE flag should not be set for success of the operation */
	time = start_timing("partial defragmentation",jp);
	
	/* fill largest free region first */
	defragmented_files = 0;
	do {
		joined_fragments = 0;
		
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
				if(f->f->disp.clusters > length && can_defragment(f->f,jp)){
					f_largest = f;
					length = f->f->disp.clusters;
				}
				if(f->next == jp->fragmented_files) break;
			}
			if(f_largest == NULL) goto part_defrag_done;
			
			/* find longest sequence of file blocks which fits in the current free region */
			longest_sequence = NULL, max_n_blocks = 0, longest_sequence_length = 0;
			for(first_block = f_largest->f->disp.blockmap; first_block; first_block = first_block->next){
				n_blocks = 0, remaining_clusters = rgn_largest->length;
				for(block = first_block; block; block = block->next){
					if(jp->termination_router((void *)jp)) goto done;
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
				if(first_block->next == f_largest->f->disp.blockmap) break;
			}
			
			if(longest_sequence == NULL){
				/* fragments cannot be joined */
				f_largest->f->user_defined_flags |= UD_FILE_TOO_LARGE;
			} else {
				/* join fragments */
				if(move_file(f_largest->f,longest_sequence->vcn,longest_sequence_length,rgn_largest->lcn,jp) >= 0){
					DebugPrint("Partial defrag success for %ws",f_largest->f->path);
					joined_fragments += max_n_blocks;
					defragmented_files ++;
				} else {
					DebugPrint("Partial defrag failure for %ws",f_largest->f->path);
				}
			}
		} while(is_too_large(f_largest->f));
	} while(joined_fragments);

part_defrag_done:
	/* display amount of moved data and number of partially defragmented files */
	moved_clusters = jp->pi.moved_clusters - moved_clusters;
	DebugPrint("%I64u files partially defragmented",defragmented_files);
	DebugPrint("%I64u clusters moved",moved_clusters);
	winx_fbsize(moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
	DebugPrint("%s moved",buffer);
	stop_timing("partial defragmentation",time,jp);
	
	/* mark all files not processed yet as too large */
	for(f = jp->fragmented_files; f; f = f->next){
		if(jp->termination_router((void *)jp)) goto done;
		if(can_defragment(f->f,jp))
			f->f->user_defined_flags |= UD_FILE_TOO_LARGE;
		if(f->next == jp->fragmented_files) break;
	}
	
	/* redraw all temporary system space as free */
	redraw_all_temporary_system_space_as_free(jp);

done:
	winx_fclose(jp->fVolume);
	jp->fVolume = NULL;
	return 0;
}

/**
 * @brief Removes unfragmented files
 * from the list of fragmented.
 */
void update_fragmented_files_list(udefrag_job_parameters *jp)
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

/** @} */
