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

#if 0
/**
 * @brief Searches for free space region starting at the beginning of the volume.
 * @param[in] jp job parameters structure.
 * @param[in] start_cluster lcn to start searching from.
 * @param[in] min_length minimum length of free space in clusters.
 * @note In case of termination request returns NULL immediately.
 */
static winx_volume_region *find_free_region_forward(udefrag_job_parameters *jp,
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
static winx_volume_region *find_free_region_backward(udefrag_job_parameters *jp,
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
#endif

enum {
	FIND_MATCHING_RGN_FORWARD,
	FIND_MATCHING_RGN_BACKWARD,
	FIND_MATCHING_RGN_ANY
};

/**
 * @brief Searches for best matching free space region.
 * @param[in] file_start_cluster lcn of first file cluster.
 * @param[in] file_length length of file in clusters.
 * @param[in] preferred_position one of the FIND_MATCHING_RGN_xxx constants:
 * FIND_MATCHING_RGN_FORWARD - region after the file_start_cluster preferred
 * FIND_MATCHING_RGN_BACKWARD - region before the file_start_cluster preferred
 * FIND_MATCHING_RGN_ANY - any region accepted
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
		if(preferred_position == FIND_MATCHING_RGN_BACKWARD \
		  && rgn->lcn > file_start_cluster)
			if(rgn_matching != NULL)
				break;
		if(rgn->length >= file_length){
            if(length == 0 || rgn->length < length){
                rgn_matching = rgn;
                length = rgn->length;
				if(length == file_length \
				  && preferred_position != FIND_MATCHING_RGN_FORWARD)
					break;
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
 * @brief Defines whether the file can be moved or not.
 */
int can_move(winx_file_info *f,udefrag_job_parameters *jp)
{
	/* skip files already excluded by the current task */
	if(is_currently_excluded(f))
		return 0;
	
	/* skip files with undefined cluster map and locked files */
	if(f->disp.blockmap == NULL || is_locked(f))
		return 0;
	
	/* skip files of zero length */
	if(f->disp.clusters == 0 || \
	  (f->disp.blockmap->next == f->disp.blockmap && \
	  f->disp.blockmap->length == 0)){
		f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
		return 0;
	}

	/* skip FAT directories */
	if(is_directory(f) && !jp->actions.allow_dir_defrag)
		return 0;
	
	/* skip file in case of improper state detected */
	if(is_in_improper_state(f))
		return 0;

	return 1;
}

/**
 * @brief Defines whether the file can be defragmented or not.
 */
int can_defragment(winx_file_info *f,udefrag_job_parameters *jp)
{
	if(!can_move(f,jp))
		return 0;

	/* skip files with less than 2 fragments */
	if(f->disp.blockmap->next == f->disp.blockmap \
	  || f->disp.fragments < 2 || !is_fragmented(f))
		return 0;
	
	return 1;
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
	ULONGLONG clusters_to_move;
	winx_file_info *file;
	
	jp->pi.current_operation = VOLUME_DEFRAGMENTATION;

	if(jp->udo.preview_flags & UD_PREVIEW_MATCHING)
		return defragment_small_files_respect_best_matching(jp);
	
	/* free as much temporarily allocated space as possible */
	release_temp_space_regions(jp);
	
	/* no files are excluded by this task currently */
	for(f = jp->fragmented_files; f; f = f->next){
		f->f->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
		if(f->next == jp->fragmented_files) break;
	}

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
		if(jp->termination_router((void *)jp)) break;
		
		/* skip micro regions */
		if(rgn->length < 2) goto next_rgn;

		/* find largest fragmented file that fits in the current region */
		do {
			if(jp->termination_router((void *)jp)) goto done;
			f_largest = NULL, length = 0;
			for(f = jp->fragmented_files; f; f = f->next){
				if(f->f->disp.clusters > length && f->f->disp.clusters <= rgn->length){
					if(can_defragment(f->f,jp)){
						f_largest = f;
						length = f->f->disp.clusters;
					}
				}
				if(f->next == jp->fragmented_files) break;
			}
			if(f_largest == NULL) goto next_rgn;
			file = f_largest->f; /* f_largest may be destroyed by move_file */
			
			/* move the file */
			if(is_mft(file,jp))
				clusters_to_move = jp->moveable_mft_clusters;
			else
				clusters_to_move = file->disp.clusters;
			if(move_file(file,file->disp.blockmap->vcn,
			 clusters_to_move,rgn->lcn,0,jp) >= 0){
				DebugPrint("Defrag success for %ws",file->path);
				defragmented_files ++;
			} else {
				DebugPrint("Defrag failure for %ws",file->path);
				/* exclude file from the current task */
				file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
			}
			
			/* skip locked files here to prevent skipping the current free region */
		} while(is_locked(file));
		
		/* after file moving continue from the first free region */
		if(jp->free_regions == NULL) break;
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
 * @details This routine fills free space areas respect to the best matching rules.
 */
static int defragment_small_files_respect_best_matching(udefrag_job_parameters *jp)
{
	ULONGLONG time;
	ULONGLONG defragmented_files;
	winx_volume_region *rgn;
	udefrag_fragmented_file *f, *f_largest;
	ULONGLONG length;
	char buffer[32];
	ULONGLONG clusters_to_move;
	winx_file_info *file;

	/* free as much temporarily allocated space as possible */
	release_temp_space_regions(jp);

	/* no files are excluded by this task currently */
	for(f = jp->fragmented_files; f; f = f->next){
		f->f->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
		if(f->next == jp->fragmented_files) break;
	}

	/* open the volume */
	// fVolume = new_winx_vopen(winx_toupper(jp->volume_letter));
	jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
	if(jp->fVolume == NULL)
		return (-1);

	time = start_timing("defragmentation",jp);

	/* find best matching free region for each fragmented file */
	jp->pi.moved_clusters = 0;
	defragmented_files = 0;
	while(jp->termination_router((void *)jp) == 0){
		f_largest = NULL, length = 0;
		for(f = jp->fragmented_files; f; f = f->next){
			if(f->f->disp.clusters > length){
				if(can_defragment(f->f,jp)){
					f_largest = f;
					length = f->f->disp.clusters;
				}
			}
			if(f->next == jp->fragmented_files) break;
		}
		if(f_largest == NULL) break;
		file = f_largest->f; /* f_largest may be destroyed by move_file */

		rgn = find_matching_free_region(jp,file->disp.blockmap->lcn,file->disp.clusters,FIND_MATCHING_RGN_ANY);
		if(jp->termination_router((void *)jp)) break;
		if(rgn == NULL){
			/* exclude file from the current task */
			file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
		} else {
			/* move the file */
			if(is_mft(file,jp))
				clusters_to_move = jp->moveable_mft_clusters;
			else
				clusters_to_move = file->disp.clusters;
			if(move_file(file,file->disp.blockmap->vcn,
			 clusters_to_move,rgn->lcn,0,jp) >= 0){
				DebugPrint("Defrag success for %ws",file->path);
				defragmented_files ++;
			} else {
				DebugPrint("Defrag failure for %ws",file->path);
				/* exclude file from the current task */
				file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
			}
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
	winx_file_info *file;

	/* free as much temporarily allocated space as possible */
	release_temp_space_regions(jp);

	/* no files are excluded by this task currently */
	for(f = jp->fragmented_files; f; f = f->next){
		f->f->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
		if(f->next == jp->fragmented_files) break;
	}

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
	
	if(winx_get_os_version() <= WINDOWS_2K && jp->fs_type == FS_NTFS){
		DebugPrint("Windows NT 4.0 and Windows 2000 have stupid limitations in defrag API");
		DebugPrint("a partial file defragmentation is almost impossible on these systems");
		goto done;
	}
	
	while(jp->termination_router((void *)jp) == 0){
		/* find largest free space region */
		rgn_largest = find_largest_free_region(jp);
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
				/* exclude file from the current task */
				f_largest->f->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
			}
		} while(is_too_large(f_largest->f));

		/* join fragments */
		file = f_largest->f; /* f_largest may be destroyed by move_file */
		target = rgn_largest->lcn; 
		if(move_file(file,longest_sequence->vcn,longest_sequence_length,
		  target,UD_MOVE_FILE_CUT_OFF_MOVED_CLUSTERS,jp) >= 0){
			DebugPrint("Partial defrag success for %ws",file->path);
			defragmented_files ++;
		} else {
			DebugPrint("Partial defrag failure for %ws",file->path);
			/* exclude file from the current task */
			file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
		}
		
		/* remove target space from the free space pool anyway */
		jp->free_regions = winx_sub_volume_region(jp->free_regions,target,longest_sequence_length);
	}

part_defrag_done:
	/* mark all files not processed yet as too large */
	for(f = jp->fragmented_files; f; f = f->next){
		if(jp->termination_router((void *)jp)) break;
		if(can_defragment(f->f,jp))
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
 * @details This routine tries to move each file entirely
 * to avoid increasing fragmentation.
 * @param[in] jp job parameters.
 * @param[in] flags one of MOVE_xxx flags defined in udefrag.h
 * @return Zero for success, negative value otherwise.
 * @note For better performance reason only the MOVE_NOT_FRAGMENTED
 * flag is supported currently.
 */
int move_files_to_front(udefrag_job_parameters *jp, int flags)
{
	ULONGLONG time;
	char buffer[32];
	ULONGLONG parts_bound; /* TODO: try to run it without a bound */
	ULONGLONG prev_pass_parts_bound; /* don't count unmoveable files twice when possible */
	ULONGLONG initial_parts_bound;
	ULONGLONG moves;
	winx_file_info *file;
	winx_volume_region *rgn;
	ULONGLONG clusters_to_move;
	
	if(flags != MOVE_NOT_FRAGMENTED){
		DebugPrint("move_files_to_front: 0x%x flag is not supported",(UINT)flags);
		return (-1);
	}

	/* free as much temporarily allocated space as possible */
	release_temp_space_regions(jp);

	/* no files are excluded by this task currently */
	for(file = jp->filelist; file; file = file->next){
		file->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
		if(file->next == jp->filelist) break;
	}

	/* open the volume */
	// fVolume = new_winx_vopen(winx_toupper(jp->volume_letter));
	jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
	if(jp->fVolume == NULL)
		return (-1);

	time = start_timing("file moving to front",jp);
	
	jp->pi.current_operation = VOLUME_OPTIMIZATION;
	jp->pi.moved_clusters = 0;
	
	/* do the job */
	parts_bound = jp->v_info.total_clusters - jp->v_info.free_bytes / jp->v_info.bytes_per_cluster;
	DebugPrint("move_files_to_front: total clusters:      %I64u", jp->v_info.total_clusters);
	DebugPrint("move_files_to_front: free clusters:       %I64u", jp->v_info.free_bytes / jp->v_info.bytes_per_cluster);
	DebugPrint("move_files_to_front: initial parts bound: %I64u", parts_bound);
	prev_pass_parts_bound = jp->v_info.total_clusters - 1;
	while(jp->termination_router((void *)jp) == 0){
		moves = 0;
		initial_parts_bound = parts_bound;
		/* cycle through all not fragmented files */
		for(file = jp->filelist; file; file = file->next){
			if(!is_fragmented(file) && file->disp.blockmap){
				if(file->disp.blockmap->lcn > parts_bound){
					if(!can_move(file,jp)){
						if(!is_in_improper_state(file)){
							/* adjust parts bound */
							if(file->disp.blockmap->lcn < prev_pass_parts_bound){
								parts_bound -= file->disp.clusters;
								moves ++;
								//DebugPrint("parts bound adjusted by %I64u clusters",file->disp.clusters);
							}
						}
					} else {
						/* move the file */
						if(is_mft(file,jp))
							clusters_to_move = jp->moveable_mft_clusters;
						else
							clusters_to_move = file->disp.clusters;
						rgn = find_matching_free_region(jp,file->disp.blockmap->lcn,clusters_to_move,FIND_MATCHING_RGN_BACKWARD);
						if(rgn != NULL){
							if(rgn->lcn < file->disp.blockmap->lcn){
								if(move_file(file,file->disp.blockmap->vcn,clusters_to_move,rgn->lcn,0,jp) >= 0)
									moves ++;
							}
						}
					}
				}
			}
			if(jp->termination_router((void *)jp)) goto done;
			if(file->next == jp->filelist) break;
		}				
		if(moves == 0) break;
		else DebugPrint("go to the next pass, moves = %I64u",moves);
		prev_pass_parts_bound = initial_parts_bound;
	}	

done:
	/* display amount of moved data */
	DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
	winx_fbsize(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
	DebugPrint("%s moved",buffer);
	stop_timing("file moving to front",time,jp);
	winx_fclose(jp->fVolume);
	jp->fVolume = NULL;
	return 0;
}

/**
 * @brief Moves selected group of files
 * to the end of the volume to free its
 * beginning.
 * @brief This routine moves individual clusters
 * and never tries to keep the file not fragmented.
 * @param[in] jp job parameters.
 * @param[in] flags one of MOVE_xxx flags defined in udefrag.h
 * @return Zero for success, negative value otherwise.
 * @note This routine is optimized for speed.
 * To achieve the best performance we devide the disk
 * into two parts. The first one will become empty after
 * the completion of file moving (in case of MOVE_ALL flag
 * passed in). The second part will become fully occupied by files.
 * Therefore, we have no need to move something from the second part
 * to the end of the disk, we have a good reason to move data from
 * the first part only. Then, if something cannot be moved from the
 * first part (or is not intended for moving respect to the flags
 * passed in), we're moving the bound between parts forward, because 
 * we have a chance to move other files instead of excluded ones.
 */
int move_files_to_back(udefrag_job_parameters *jp, int flags)
{
	ULONGLONG time;
	char buffer[32];
	ULONGLONG used_clusters, parts_bound;
	ULONGLONG prev_pass_parts_bound; /* don't count unmoveable files twice when possible */
	ULONGLONG initial_parts_bound;
	ULONGLONG moves;
	winx_file_info *file;
	winx_blockmap *block;
	ULONGLONG current_vcn, remaining_clusters;
	winx_volume_region *rgn, *prev_rgn;
	ULONGLONG length;
	int move_result;
	int block_moved;
	int nt4w2k_limitations = 0;
	ULONGLONG clusters_to_move;

	/* free as much temporarily allocated space as possible */
	release_temp_space_regions(jp);

	/* no files are excluded by this task currently */
	for(file = jp->filelist; file; file = file->next){
		file->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
		if(file->next == jp->filelist) break;
	}

	/* open the volume */
	// fVolume = new_winx_vopen(winx_toupper(jp->volume_letter));
	jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
	if(jp->fVolume == NULL)
		return (-1);

	time = start_timing("file moving to end",jp);
	
	jp->pi.current_operation = VOLUME_OPTIMIZATION;
	jp->pi.moved_clusters = 0;
	
	if(winx_get_os_version() <= WINDOWS_2K && jp->fs_type == FS_NTFS){
		DebugPrint("Windows NT 4.0 and Windows 2000 have stupid limitations in defrag API");
		DebugPrint("volume optimization is not so much effective on these systems");
		nt4w2k_limitations = 1;
	}
	
	/* do the job */
	used_clusters = (jp->v_info.total_bytes - jp->v_info.free_bytes);
	used_clusters /= jp->v_info.bytes_per_cluster;
	parts_bound = jp->v_info.total_clusters - used_clusters;
	DebugPrint("move_files_to_back: total clusters:      %I64u", jp->v_info.total_clusters);
	DebugPrint("move_files_to_back: used clusters:       %I64u", used_clusters);
	DebugPrint("move_files_to_back: initial parts bound: %I64u", parts_bound);
	prev_pass_parts_bound = 0;
	while(jp->termination_router((void *)jp) == 0){
		moves = 0;
		initial_parts_bound = parts_bound;
		/* cycle through all files and all their blocks */
		for(file = jp->filelist; file; file = file->next){
repeat_scan:
			for(block = file->disp.blockmap; block; block = block->next){
				if(jp->termination_router((void *)jp)) goto done;
				if(block->lcn < parts_bound && block->length){
					if(!can_move(file,jp)){
						if(block->lcn > prev_pass_parts_bound){
							/* adjust parts bound */
							parts_bound += block->length;
							//DebugPrint("move_files_to_back: parts bound adjusted: %I64u", parts_bound);
							if(block->length) moves ++;
						}
					} else {
						if((flags == MOVE_FRAGMENTED) && !is_fragmented(file)){
							if(block->lcn > prev_pass_parts_bound){
								/* adjust parts bound */
								parts_bound += block->length;
								//DebugPrint("move_files_to_back: parts bound adjusted: %I64u", parts_bound);
								if(block->length) moves ++;
							}
						} else if((flags == MOVE_NOT_FRAGMENTED) && is_fragmented(file)){
							if(block->lcn > prev_pass_parts_bound){
								/* adjust parts bound */
								parts_bound += block->length;
								//DebugPrint("move_files_to_back: parts bound adjusted: %I64u", parts_bound);
								if(block->length) moves ++;
							}
						} else {
							if(!nt4w2k_limitations){
								/* file block can be moved, let's move it to the end of disk */
								if(jp->free_regions == NULL) goto done;
								current_vcn = block->vcn;
								remaining_clusters = block->length;
								block_moved = 0;
								move_result = 0;
								for(rgn = jp->free_regions->prev; rgn && remaining_clusters; rgn = prev_rgn){
									prev_rgn = rgn->prev; /* save it now since free regions list may be changed */
									if(rgn->lcn < block->lcn){
										//DebugPrint("move_files_to_back: unexpected condition");
										/*DebugPrint("move_files_to_back: optimization completed");
										DebugPrint("rgn->lcn = %I64u, block->lcn = %I64u",rgn->lcn,block->lcn);
										goto done;*/
										break;
									}
									if(rgn->length > 0){
										length = min(rgn->length,remaining_clusters);
										if(jp->termination_router((void *)jp)) goto done;
										if(move_file(file,current_vcn,length,rgn->lcn + rgn->length - length,0,jp) >= 0)
											moves ++;
										else
											move_result = -1;
										block_moved = 1;
										current_vcn += length;
										remaining_clusters -= length;
									}
									if(jp->free_regions == NULL) break;
									if(prev_rgn == jp->free_regions->prev) break;
								}
								if(block_moved){
									/* map of file blocks changed, so let's scan it again from the beginning */
									if(move_result >= 0) goto repeat_scan;
									else goto next_file; /* blockmap may be changed anyway, therefore walk continuation is not safe */
								} else {
									/* go to the next block */
								}
							} else {
								/* it is safe to move entire files and entire blocks of compressed/sparse files */
								if(is_compressed(file) || is_sparse(file)){
									/* move entire block */
									rgn = find_matching_free_region(jp,block->lcn,block->length,FIND_MATCHING_RGN_FORWARD);
									if(rgn != NULL){
										if(rgn->lcn > block->lcn){
											if(move_file(file,block->vcn,block->length,rgn->lcn + rgn->length - block->length,0,jp) >= 0){
												moves ++;
												/* map of file blocks changed, so let's scan it again from the beginning */
												goto repeat_scan;
											} else {
												/* blockmap may be changed anyway, therefore walk continuation is not safe */
												goto next_file;
											}
										}
									}
									/* go to the next block */
								} else {
									/* move entire file */
									if(is_mft(file,jp))
										clusters_to_move = jp->moveable_mft_clusters;
									else
										clusters_to_move = file->disp.clusters;
									rgn = find_matching_free_region(jp,block->lcn,clusters_to_move,FIND_MATCHING_RGN_FORWARD);
									if(rgn != NULL){
										if(rgn->lcn > block->lcn){
											if(move_file(file,file->disp.blockmap->vcn,clusters_to_move,
											  rgn->lcn + rgn->length - clusters_to_move,0,jp) >= 0){
												moves ++;
											}
										}
									}
									/* go to the next file */
									goto next_file;
								}
							}
						}
					}
				}
				if(block->next == file->disp.blockmap) break;
			}
next_file:
			if(file->next == jp->filelist) break;
		}
		if(moves == 0) break;
		else DebugPrint("go to the next pass, moves = %I64u",moves);
		prev_pass_parts_bound = initial_parts_bound;
	}	

done:
	/* display amount of moved data */
	DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
	winx_fbsize(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
	DebugPrint("%s moved",buffer);
	stop_timing("file moving to end",time,jp);
	winx_fclose(jp->fVolume);
	jp->fVolume = NULL;
	return 0;
}

/** @} */
