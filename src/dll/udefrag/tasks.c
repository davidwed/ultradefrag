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

/************************************************************/
/*                    Internal Routines                     */
/************************************************************/

/**
 * @brief Searches for free space region starting at the beginning of the volume.
 * @param[in] jp job parameters structure.
 * @param[in] min_length minimum length of region, in clusters.
 * @note In case of termination request returns NULL immediately.
 */
/*static */winx_volume_region *find_first_free_region(udefrag_job_parameters *jp,ULONGLONG min_length)
{
	winx_volume_region *rgn;
	ULONGLONG time = winx_xtime();

	for(rgn = jp->free_regions; rgn; rgn = rgn->next){
		if(jp->termination_router((void *)jp)) break;
		if(rgn->length >= min_length){
			jp->p_counters.searching_time += winx_xtime() - time;
			return rgn;
		}
		if(rgn->next == jp->free_regions) break;
	}
	jp->p_counters.searching_time += winx_xtime() - time;
	return NULL;
}

/**
 * @brief Searches for free space region starting at the end of the volume.
 * @param[in] jp job parameters structure.
 * @param[in] min_length minimum length of region, in clusters.
 * @note In case of termination request returns NULL immediately.
 */
/*static */winx_volume_region *find_last_free_region(udefrag_job_parameters *jp,ULONGLONG min_length)
{
	winx_volume_region *rgn;
	ULONGLONG time = winx_xtime();

	if(jp->free_regions){
		for(rgn = jp->free_regions->prev; rgn; rgn = rgn->prev){
			if(jp->termination_router((void *)jp)) break;
			if(rgn->length >= min_length){
				jp->p_counters.searching_time += winx_xtime() - time;
				return rgn;
			}
			if(rgn->prev == jp->free_regions->prev) break;
		}
	}
	jp->p_counters.searching_time += winx_xtime() - time;
	return NULL;
}

enum {
	FIND_MATCHING_RGN_FORWARD,
	FIND_MATCHING_RGN_BACKWARD,
	FIND_MATCHING_RGN_ANY
};

/**
 * @brief Searches for best matching free space region.
 * @param[in] start_lcn a point which divides disk into two parts (see below).
 * @param[in] min_length minimal accepted length of the region, in clusters.
 * @param[in] preferred_position one of the FIND_MATCHING_RGN_xxx constants:
 * FIND_MATCHING_RGN_FORWARD - region after the start_lcn preferred
 * FIND_MATCHING_RGN_BACKWARD - region before the start_lcn preferred
 * FIND_MATCHING_RGN_ANY - any region accepted
 * @note In case of termination request returns
 * NULL immediately.
 */
static winx_volume_region *find_matching_free_region(udefrag_job_parameters *jp,
    ULONGLONG start_lcn, ULONGLONG min_length, int preferred_position)
{
	winx_volume_region *rgn, *rgn_matching;
	ULONGLONG length;
	ULONGLONG time = winx_xtime();
	
	rgn_matching = NULL, length = 0;
	for(rgn = jp->free_regions; rgn; rgn = rgn->next){
		if(jp->termination_router((void *)jp)){
			jp->p_counters.searching_time += winx_xtime() - time;
			return NULL;
		}
		if(preferred_position == FIND_MATCHING_RGN_BACKWARD \
		  && rgn->lcn > start_lcn)
			if(rgn_matching != NULL)
				break;
		if(rgn->length >= min_length){
            if(length == 0 || rgn->length < length){
                rgn_matching = rgn;
                length = rgn->length;
				if(length == min_length \
				  && preferred_position != FIND_MATCHING_RGN_FORWARD)
					break;
            }
		}
		if(rgn->next == jp->free_regions) break;
	}
	jp->p_counters.searching_time += winx_xtime() - time;
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
	ULONGLONG time = winx_xtime();
	
	rgn_largest = NULL, length = 0;
	for(rgn = jp->free_regions; rgn; rgn = rgn->next){
		if(jp->termination_router((void *)jp)){
			jp->p_counters.searching_time += winx_xtime() - time;
			return NULL;
		}
		if(rgn->length > length){
			rgn_largest = rgn;
			length = rgn->length;
		}
		if(rgn->next == jp->free_regions) break;
	}
	jp->p_counters.searching_time += winx_xtime() - time;
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
	if(get_file_length(jp,f) == 0 || \
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
	
	/* avoid infinite loops */
	if(is_moving_failed(f))
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

/**
 * @brief Defines whether MFT can be optimized or not.
 */
static int can_optimize_mft(udefrag_job_parameters *jp,winx_file_info **mft_file)
{
	winx_file_info *f, *file;
	
	if(mft_file)
		*mft_file = NULL;

	/* MFT can be optimized on NTFS formatted volumes only */
	if(jp->fs_type != FS_NTFS)
		return 0;
	
	/* MFT is not movable on systems prior to xp */
	if(winx_get_os_version() < WINDOWS_XP)
		return 0;
	
	/*
	* In defragmentation this task is impossible because
	* of lack of information needed to cleanup space.
	*/
	if(jp->job_type == DEFRAGMENTATION_JOB){
		DebugPrint("optimize_mft: MFT optimization is impossible in defragmentation task");
		return 0;
	}
	
	/* search for $mft file */
	file = NULL;
	for(f = jp->filelist; f; f = f->next){
		if(is_mft(f,jp)){
			file = f;
			break;
		}
		if(f->next == jp->filelist) break;
	}
	if(file == NULL){
		DebugPrint("optimize_mft: cannot find $mft file");
		return 0;
	}
	
	/* is $mft file locked? */
	if(is_file_locked(file,jp)){
		DebugPrint("optimize_mft: $mft file is locked");
		return 0;
	}
	
	/* can we move something? */
	if(!file->disp.blockmap \
	  || !file->disp.clusters \
	  || is_in_improper_state(file)){
		DebugPrint("optimize_mft: no movable data detected");
		return 0;
	}
	  
	/* is MFT already optimized? */
	if(!is_fragmented(file) || file->disp.blockmap->next == file->disp.blockmap){
		DebugPrint("optimize_mft: $mft file is already optimized");
		return 0;
	}

	if(mft_file)
		*mft_file = file;
	return 1;
}

/**
 * @brief Returns length of a movable part of the file.
 */
ULONGLONG get_file_length(udefrag_job_parameters *jp, winx_file_info *f)
{
	return f->disp.clusters;
}

/**
 * @brief Sends list of $mft blocks to the debugger.
 */
static void list_mft_blocks(winx_file_info *mft_file)
{
	winx_blockmap *block;
	ULONGLONG i;

	for(block = mft_file->disp.blockmap, i = 0; block; block = block->next, i++){
		DebugPrint("mft part #%I64u start: %I64u, length: %I64u",
			i,block->lcn,block->length);
		if(block->next == mft_file->disp.blockmap) break;
	}
}

/************************************************************/
/*                      Atomic Tasks                        */
/************************************************************/

/**
 * @brief Optimizes MFT by placing its parts 
 * after the first one as close as possible.
 * @details We cannot adjust MFT zone, but
 * usually it should follow optimized MFT 
 * automatically. Also we have no need to move
 * MFT to a different location than the one 
 * initially chosen by the O/S, since on big 
 * volumes it makes sense to keep it near 
 * the middle of the disk, so the disk head
 * does not have to move back and forth across
 * the whole disk to access files at the back
 * of the volume.
 * @note As a side effect it may increase
 * number of fragmented files.
 */
int optimize_mft(udefrag_job_parameters *jp)
{
	ULONGLONG clusters_to_process; /* the number of $mft clusters not processed yet */
	ULONGLONG start_vcn;           /* VCN of the portion of $mft not processed yet */
	ULONGLONG clusters_to_move;    /* the number of $mft clusters intended for the current move */
	ULONGLONG start_lcn;           /* address of space not processed yet */
	ULONGLONG time, tm;
	winx_volume_region *rlist = NULL, *rgn, *target_rgn;
	winx_file_info *mft_file, *file, *first_file;
	winx_blockmap *block, *first_block;
	ULONGLONG end_lcn, min_lcn, next_vcn;
	ULONGLONG current_vcn, remaining_clusters, n, lcn;
	winx_volume_region region = {0};
	ULONGLONG clusters_to_cleanup;
	int block_cleaned_up;
	char buffer[32];
	
	jp->pi.current_operation = VOLUME_OPTIMIZATION;
	jp->pi.moved_clusters = 0;
	jp->pi.total_moves = 0;
    
	if(!can_optimize_mft(jp,&mft_file))
		return 0;
	
	/* open the volume */
	// fVolume = new_winx_vopen(winx_toupper(jp->volume_letter));
	jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
	if(jp->fVolume == NULL)
		return (-1);

	DebugPrint("optimize_mft: initial $mft map:");
	list_mft_blocks(mft_file);
	
	time = start_timing("mft optimization",jp);

	clusters_to_process = mft_file->disp.clusters - mft_file->disp.blockmap->length;
	start_lcn = mft_file->disp.blockmap->lcn + mft_file->disp.blockmap->length;
	start_vcn = mft_file->disp.blockmap->next->vcn;
	while(clusters_to_process > 0){
		if(jp->termination_router((void *)jp)) break;
		
		/* update list of free regions */
		winx_release_free_volume_regions(rlist);
		tm = winx_xtime();
		rlist = winx_get_free_volume_regions(jp->volume_letter,
			WINX_GVR_ALLOW_PARTIAL_SCAN,NULL,(void *)jp);
		jp->p_counters.temp_space_releasing_time += winx_xtime() - tm;
		if(rlist == NULL) break;
		release_temp_space_regions_internal(jp);
		
		/* search for the first free region after start_lcn */
		target_rgn = NULL; tm = winx_xtime();
		for(rgn = rlist; rgn; rgn = rgn->next){
			if(rgn->lcn >= start_lcn && rgn->length){
				target_rgn = rgn;
				break;
			}
			if(rgn->next == rlist) break;
		}
		jp->p_counters.searching_time += winx_xtime() - tm;
		
		/* process file blocks between start_lcn and target_rgn */
		if(target_rgn) end_lcn = target_rgn->lcn;
		else end_lcn = jp->v_info.total_clusters;
		clusters_to_cleanup = clusters_to_process;
		block_cleaned_up = 0;
		region.length = 0;
		while(clusters_to_cleanup > 0){
			if(jp->termination_router((void *)jp)) goto done;
			tm = winx_xtime();
			first_file = NULL; first_block = NULL; min_lcn = end_lcn;
			for(file = jp->filelist; file; file = file->next){
				if(can_move(file,jp)){
					for(block = file->disp.blockmap; block; block = block->next){
						if(block->lcn >= start_lcn && block->lcn < min_lcn && block->length){
							first_file = file;
							first_block = block;
							min_lcn = block->lcn;
						}
						if(block->next == file->disp.blockmap) break;
					}
				}
				if(file->next == jp->filelist) break;
			}
			if(first_file == NULL){
				jp->p_counters.searching_time += winx_xtime() - tm;
				break;
			}
			if(is_file_locked(first_file,jp)){
				jp->p_counters.searching_time += winx_xtime() - tm;
				continue;
			}
			jp->p_counters.searching_time += winx_xtime() - tm;
			
			/* does the first block follow a previously moved one? */
			if(block_cleaned_up){
				if(first_block->lcn != region.lcn + region.length)
					break;
				if(first_file == mft_file)
					break;
			}
			
			/* don't move already optimized parts of $mft */
			if(first_file == mft_file && first_block->vcn == start_vcn){
				if(clusters_to_process <= first_block->length \
				  || first_block->next == first_file->disp.blockmap){
					clusters_to_process = 0;
					goto done;
				} else {
					clusters_to_process -= first_block->length;
					start_vcn = first_block->next->vcn;
					start_lcn = first_block->lcn + first_block->length;
					continue;
				}
			}
			
			/* cleanup space */
			if(rlist == NULL) goto done;
			lcn = first_block->lcn;
			current_vcn = first_block->vcn;
			clusters_to_move = remaining_clusters = min(clusters_to_cleanup, first_block->length);
			for(rgn = rlist->prev; rgn && remaining_clusters; rgn = rgn->prev){
				if(rgn->length > 0){
					n = min(rgn->length,remaining_clusters);
					if(move_file(first_file,current_vcn,n,rgn->lcn + rgn->length - n,0,jp) < 0){
						if(!block_cleaned_up)
							goto done;
						else
							goto move_mft;
					}
					current_vcn += n;
					remaining_clusters -= n;
				}
				if(rlist == NULL){
					if(remaining_clusters)
						goto done;
					else
						break;
				}
				if(rgn->prev == rlist->prev) break;
			}
			/* space cleaned up successfully */
			region.next = region.prev = &region;
			if(!block_cleaned_up)
				region.lcn = lcn;
			region.length += clusters_to_move;
			target_rgn = &region;
			start_lcn = region.lcn + region.length;
			clusters_to_cleanup -= clusters_to_move;
			block_cleaned_up = 1;
		}
	
move_mft:		
		/* target_rgn points to the target free region, so let's move the next portion of $mft */
		if(target_rgn == NULL) break;
		n = clusters_to_move = min(clusters_to_process,target_rgn->length);
		next_vcn = 0; current_vcn = start_vcn;
		for(block = mft_file->disp.blockmap; block && n; block = block->next){
			if(block->vcn + block->length > start_vcn){
				if(n > block->length - (current_vcn - block->vcn)){
					n -= block->length - (current_vcn - block->vcn);
					current_vcn = block->next->vcn;
				} else {
					if(n == block->length - (current_vcn - block->vcn)){
						if(block->next == mft_file->disp.blockmap)
							next_vcn = block->vcn + block->length;
						else
							next_vcn = block->next->vcn;
					} else {
						next_vcn = current_vcn + n;
					}
					break;
				}
			}
			if(block->next == mft_file->disp.blockmap) break;
		}
		if(next_vcn == 0){
			DebugPrint("optimize_mft: next_vcn calculation failed");
			break;
		}
		if(move_file(mft_file,start_vcn,clusters_to_move,target_rgn->lcn,0,jp) < 0){
			/* on failures exit */
			break;
		}
		/* $mft part moved successfully */
		clusters_to_process -= clusters_to_move;
		start_lcn = target_rgn->lcn + clusters_to_move;
		start_vcn = next_vcn;
        jp->pi.total_moves ++;
	}

done:
	DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
	winx_fbsize(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
	DebugPrint("%s moved",buffer);
	stop_timing("mft optimization",time,jp);
	winx_release_free_volume_regions(rlist);
	winx_fclose(jp->fVolume);
	jp->fVolume = NULL;

	if(jp->termination_router((void *)jp))
		return 0;

	return (clusters_to_process > 0) ? (-1) : 0;
}

/**
 * @brief Defragments all fragmented files entirely, if possible.
 * @details 
 * - This routine fills free space areas from the beginning of the
 * volume regardless of the best matching rules.
 */
int __stdcall defragment_small_files_walk_free_regions(udefrag_job_parameters *jp)
{
	ULONGLONG time, tm;
	ULONGLONG defragmented_files;
	winx_volume_region *rgn;
	udefrag_fragmented_file *f, *f_largest;
	ULONGLONG length;
	char buffer[32];
	ULONGLONG clusters_to_move;
	winx_file_info *file;
	ULONGLONG file_length;
	
	jp->pi.current_operation = VOLUME_DEFRAGMENTATION;
	jp->pi.moved_clusters = 0;

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
	defragmented_files = 0;
	for(rgn = jp->free_regions; rgn; rgn = rgn->next){
		if(jp->termination_router((void *)jp)) break;
		
		/* skip micro regions */
		if(rgn->length < 2) goto next_rgn;

		/* find largest fragmented file that fits in the current region */
		do {
			if(jp->termination_router((void *)jp)) goto done;
			f_largest = NULL, length = 0; tm = winx_xtime();
			for(f = jp->fragmented_files; f; f = f->next){
				file_length = get_file_length(jp,f->f);
				if(file_length > length && file_length <= rgn->length){
					if(can_defragment(f->f,jp) && !is_mft(f->f,jp)){
						f_largest = f;
						length = file_length;
					}
				}
				if(f->next == jp->fragmented_files) break;
			}
			jp->p_counters.searching_time += winx_xtime() - tm;
			if(f_largest == NULL) goto next_rgn;
			file = f_largest->f; /* f_largest may be destroyed by move_file */
			
			/* move the file */
			clusters_to_move = get_file_length(jp,file);
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
int __stdcall defragment_small_files_walk_fragmented_files(udefrag_job_parameters *jp)
{
	ULONGLONG time, tm;
	ULONGLONG defragmented_files;
	winx_volume_region *rgn;
	udefrag_fragmented_file *f, *f_largest;
	ULONGLONG length;
	char buffer[32];
	ULONGLONG clusters_to_move;
	winx_file_info *file;
	ULONGLONG file_length;

	jp->pi.current_operation = VOLUME_DEFRAGMENTATION;
	jp->pi.moved_clusters = 0;

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
	defragmented_files = 0;
	while(jp->termination_router((void *)jp) == 0){
		f_largest = NULL, length = 0; tm = winx_xtime();
		for(f = jp->fragmented_files; f; f = f->next){
			file_length = get_file_length(jp,f->f);
			if(file_length > length){
				if(can_defragment(f->f,jp) && !is_mft(f->f,jp)){
					f_largest = f;
					length = file_length;
				}
			}
			if(f->next == jp->fragmented_files) break;
		}
		jp->p_counters.searching_time += winx_xtime() - tm;
		if(f_largest == NULL) break;
		file = f_largest->f; /* f_largest may be destroyed by move_file */

		rgn = find_matching_free_region(jp,file->disp.blockmap->lcn,get_file_length(jp,file),FIND_MATCHING_RGN_ANY);
		if(jp->termination_router((void *)jp)) break;
		if(rgn == NULL){
			/* exclude file from the current task */
			file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
		} else {
			/* move the file */
			clusters_to_move = get_file_length(jp,file);
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
 * - This routine uses UD_MOVE_FILE_CUT_OFF_MOVED_CLUSTERS
 * flag to avoid infinite loops, therefore processed file
 * maps become cut. This is not a problem while we use
 * a single defragment_big_files call after all other
 * volume processing steps.
 */
int __stdcall defragment_big_files(udefrag_job_parameters *jp)
{
	ULONGLONG time, tm;
	ULONGLONG defragmented_files;
	winx_volume_region *rgn_largest;
	udefrag_fragmented_file *f, *f_largest;
	ULONGLONG target, length;
	winx_blockmap *block, *first_block, *longest_sequence;
	ULONGLONG n_blocks, max_n_blocks, longest_sequence_length;
	ULONGLONG remaining_clusters;
	char buffer[32];
	winx_file_info *file;
	ULONGLONG file_length;

	jp->pi.current_operation = VOLUME_DEFRAGMENTATION;
	jp->pi.moved_clusters = 0;

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
try_again:
			if(jp->termination_router((void *)jp)) goto done;
			f_largest = NULL, length = 0; tm = winx_xtime();
			for(f = jp->fragmented_files; f; f = f->next){
				file_length = get_file_length(jp,f->f);
				if(file_length > length && can_defragment(f->f,jp) && !is_mft(f->f,jp)){
					f_largest = f;
					length = file_length;
				}
				if(f->next == jp->fragmented_files) break;
			}
			if(f_largest == NULL){
				jp->p_counters.searching_time += winx_xtime() - tm;
				goto part_defrag_done;
			}
			if(is_file_locked(f_largest->f,jp)){
				jp->pi.processed_clusters += f_largest->f->disp.clusters;
				jp->p_counters.searching_time += winx_xtime() - tm;
				goto try_again;
			}
			jp->p_counters.searching_time += winx_xtime() - tm;
			
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
		if(can_defragment(f->f,jp) && !is_mft(f->f,jp))
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
 * @param[in] start_lcn the first cluster of an area intended 
 * to be cleaned up - all clusters before it should be skipped.
 * @param[in] flags one of MOVE_xxx flags defined in udefrag.h
 * @return Zero for success, negative value otherwise.
 */
int move_files_to_front(udefrag_job_parameters *jp, ULONGLONG start_lcn, int flags)
{
	ULONGLONG time, tm;
	ULONGLONG moves, pass;
	winx_file_info *file,/* *last_file, */*largest_file;
	ULONGLONG length, rgn_size_threshold;
	int files_found;
	ULONGLONG rgn_lcn, min_file_lcn, min_rgn_lcn, max_rgn_lcn;
	/*ULONGLONG end_lcn, last_lcn;*/
	winx_volume_region *rgn;
	ULONGLONG clusters_to_move;
	ULONGLONG file_length;
	winx_blockmap *block;
	ULONGLONG new_sp;
	char buffer[32];
	
	jp->pi.current_operation = VOLUME_OPTIMIZATION;
	jp->pi.moved_clusters = 0;
	
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
	
	/* do the job */
	/* strategy 1: the most effective one */
	rgn_size_threshold = 1; pass = 0; jp->pi.total_moves = 0;
	while(!jp->termination_router((void *)jp)){
		moves = 0;
	
		/* free as much temporarily allocated space as possible */
		release_temp_space_regions(jp);
		
		min_rgn_lcn = 0; max_rgn_lcn = jp->v_info.total_clusters - 1;
repeat_scan:
		for(rgn = jp->free_regions; rgn; rgn = rgn->next){
			if(jp->termination_router((void *)jp)) break;
			/* break if we have already moved files behind the current region */
			if(rgn->lcn > max_rgn_lcn) break;
			if(rgn->length > rgn_size_threshold && rgn->lcn >= min_rgn_lcn){
				/* break if on the next pass the current region will be moved to the end */
				if(jp->udo.job_flags & UD_JOB_REPEAT){
					/* TODO: speedup */
					new_sp = calculate_starting_point(jp,start_lcn);
					if(rgn->lcn > new_sp){
						DebugPrint("rgn->lcn = %I64u, rgn->length = %I64u",rgn->lcn,rgn->length);
						DebugPrint("move_files_to_front: heavily fragmented space begins at %I64u cluster",new_sp);
						goto done;
					}
				}
try_again:
				/* find largest file after start_lcn which fits here */
				tm = winx_xtime();
				largest_file = NULL; length = 0; files_found = 0;
				for(file = jp->filelist; file; file = file->next){
					if(can_move(file,jp) && !is_mft(file,jp)){
						if((flags == MOVE_FRAGMENTED) && !is_fragmented(file)){
						} else if((flags == MOVE_NOT_FRAGMENTED) && is_fragmented(file)){
						} else {
							if(file->disp.blockmap->lcn >= start_lcn \
							  && file->disp.blockmap->lcn > rgn->lcn){
								files_found = 1;
								file_length = get_file_length(jp,file);
								if(file_length > length \
								  && file_length <= rgn->length){
									largest_file = file;
									length = file_length;
								}
							}
						}
					}
					if(file->next == jp->filelist) break;
				}
				jp->p_counters.searching_time += winx_xtime() - tm;
				if(files_found == 0){
					/* no more files can be moved on the current pass */
					break; /*goto done;*/
				}
				if(largest_file == NULL){
					/* current free region is too small, let's try next one */
					rgn_size_threshold = rgn->length;
				} else {
					if(is_file_locked(largest_file,jp)){
						jp->pi.processed_clusters += largest_file->disp.clusters;
						goto try_again; /* skip locked files */
					}
					/* move the file */
					min_file_lcn = jp->v_info.total_clusters - 1;
					for(block = largest_file->disp.blockmap; block; block = block->next){
						if(block->length && block->lcn < min_file_lcn)
							min_file_lcn = block->lcn;
						if(block->next == largest_file->disp.blockmap) break;
					}
					rgn_lcn = rgn->lcn;
					clusters_to_move = get_file_length(jp,largest_file);
					if(move_file(largest_file,largest_file->disp.blockmap->vcn,clusters_to_move,rgn->lcn,0,jp) >= 0){
						moves ++;
						jp->pi.total_moves ++;
					}
					/* regardless of result, exclude the file */
					largest_file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
					/* continue from the first free region after used one */
					min_rgn_lcn = rgn_lcn + 1;
					if(max_rgn_lcn > min_file_lcn - 1)
						max_rgn_lcn = min_file_lcn - 1;
					goto repeat_scan;
				}
			}
			if(rgn->next == jp->free_regions) break;
		}
	
		if(moves == 0) break;
		DebugPrint("move_files_to_front: pass %I64u completed, %I64u files moved",pass,moves);
		pass ++;
	}
	/* end of strategy 1 */
	goto done;
#if 0
	/* strategy 2: not so effective */
	max_rgn_lcn = jp->v_info.total_clusters - 1;
	for(file = jp->filelist; file; file = file->next){
		if(jp->termination_router((void *)jp)) break;
		if(can_move(file,jp) && !is_mft(file,jp) && !is_fragmented(file)){
			if(file->disp.blockmap->lcn >= start_lcn){
				clusters_to_move = get_file_length(jp,file);
				rgn = find_first_free_region(jp,clusters_to_move);
				if(rgn != NULL){
					if(rgn->lcn < file->disp.blockmap->lcn && rgn->lcn <= max_rgn_lcn){
						file_lcn = file->disp.blockmap->lcn;
						if(move_file(file,file->disp.blockmap->vcn,clusters_to_move,rgn->lcn,0,jp) >= 0)
							jp->pi.total_moves ++;
						if(max_rgn_lcn > file_lcn - 1)
							max_rgn_lcn = file_lcn - 1;
					}
				}
			}
		}
		if(file->next == jp->filelist) break;
	}
	/* end of strategy 2 */
	goto done;
		
	/* strategy 3: very slow */
	moves = 0; end_lcn = jp->v_info.total_clusters;
	while(!jp->termination_router((void *)jp) && end_lcn > start_lcn){
		/* find last not fragmented file between start_lcn and end_lcn */
		last_file = NULL; last_lcn = 0;
		for(file = jp->filelist; file; file = file->next){
			if(can_move(file,jp) && !is_mft(file,jp) && !is_fragmented(file)){
				if(file->disp.blockmap->lcn >= start_lcn && \
				  file->disp.blockmap->lcn < end_lcn && \
				  file->disp.blockmap->lcn > last_lcn){
					last_file = file;
					last_lcn = file->disp.blockmap->lcn;
				}
			}
			if(file->next == jp->filelist) break;
		}
		if(last_file == NULL) break;
		if(is_file_locked(last_file,jp)) continue;
		
		/* move the file to the beginning of the volume */
		end_lcn = last_file->disp.blockmap->lcn;
		DebugPrint("end lcn = %I64u, start lcn = %I64u",end_lcn,start_lcn);
		clusters_to_move = get_file_length(jp,last_file);
		rgn = find_first_free_region(jp,clusters_to_move);
		if(rgn != NULL){
			if(rgn->lcn < last_file->disp.blockmap->lcn){
				if(move_file(last_file,last_file->disp.blockmap->vcn,clusters_to_move,rgn->lcn,0,jp) >= 0){
					moves ++;
					jp->pi.total_moves ++;
				}
			}
		}
		last_file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
	}
#endif
done:
	/* display amount of moved data */
	DebugPrint("%I64u files moved totally",jp->pi.total_moves);
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
 * @param[in] start_lcn the first cluster of an area intended 
 * to be cleaned up - all clusters before it should be skipped.
 * @param[in] flags one of MOVE_xxx flags defined in udefrag.h
 * @return Zero for success, negative value otherwise.
 */
int move_files_to_back(udefrag_job_parameters *jp, ULONGLONG start_lcn, int flags)
{
	ULONGLONG time, tm;
	int nt4w2k_limitations = 0;
	winx_file_info *file, *first_file;
	winx_blockmap *block, *first_block;
	ULONGLONG min_lcn, block_lcn, block_length, n;
	ULONGLONG current_vcn, remaining_clusters;
	winx_volume_region *rgn, *prev_rgn;
	ULONGLONG clusters_to_move;
	char buffer[32];
	
	jp->pi.current_operation = VOLUME_OPTIMIZATION;
	jp->pi.moved_clusters = 0;
    jp->pi.total_moves = 0;
	
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

	if(winx_get_os_version() <= WINDOWS_2K && jp->fs_type == FS_NTFS){
		DebugPrint("Windows NT 4.0 and Windows 2000 have stupid limitations in defrag API");
		DebugPrint("volume optimization is not so much effective on these systems");
		nt4w2k_limitations = 1;
	}
	
	/* do the job */
	while(!jp->termination_router((void *)jp)){
		/* find the first block after start_lcn */
		tm = winx_xtime();
		first_file = NULL; first_block = NULL; min_lcn = jp->v_info.total_clusters;
		for(file = jp->filelist; file; file = file->next){
			if(can_move(file,jp) && !is_mft(file,jp)){
				if((flags == MOVE_FRAGMENTED) && !is_fragmented(file)){
				} else if((flags == MOVE_NOT_FRAGMENTED) && is_fragmented(file)){
				} else {
					for(block = file->disp.blockmap; block; block = block->next){
						if(block->lcn >= start_lcn && block->lcn < min_lcn && block->length){
							first_file = file;
							first_block = block;
							min_lcn = block->lcn;
						}
						if(block->next == file->disp.blockmap) break;
					}
				}
			}
			if(file->next == jp->filelist) break;
		}
		if(first_file == NULL){
			jp->p_counters.searching_time += winx_xtime() - tm;
			break;
		}
		if(is_file_locked(first_file,jp)){
			jp->pi.processed_clusters += first_file->disp.clusters;
			jp->p_counters.searching_time += winx_xtime() - tm;
			continue;
		}
		jp->p_counters.searching_time += winx_xtime() - tm;
		
		/* move the first block to the last free regions */
		block_lcn = first_block->lcn; block_length = first_block->length;
		if(!nt4w2k_limitations){
			/* sure, we can fill by the first block's data the last free regions */
			if(jp->free_regions == NULL) goto done;
			current_vcn = first_block->vcn;
			remaining_clusters = first_block->length;
			for(rgn = jp->free_regions->prev; rgn && remaining_clusters; rgn = prev_rgn){
				prev_rgn = rgn->prev; /* save it now since free regions list may be changed */
				if(rgn->lcn < block_lcn + block_length){
					/* no more moves to the end of disk are possible */
					goto done;
				}
				if(rgn->length > 0){
					n = min(rgn->length,remaining_clusters);
					if(move_file(first_file,current_vcn,n,rgn->lcn + rgn->length - n,0,jp) < 0){
						/* exclude file from the current task to avoid infinite loops */
						file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
					}
					current_vcn += n;
					remaining_clusters -= n;
                    jp->pi.total_moves ++;
				}
				if(jp->free_regions == NULL) goto done;
				if(prev_rgn == jp->free_regions->prev) break;
			}
		} else {
			/* it is safe to move entire files and entire blocks of compressed/sparse files */
			if(is_compressed(first_file) || is_sparse(first_file)){
				/* move entire block */
				current_vcn = first_block->vcn;
				clusters_to_move = first_block->length;
			} else {
				/* move entire file */
				current_vcn = first_file->disp.blockmap->vcn;
				clusters_to_move = get_file_length(jp,first_file);
			}
			rgn = find_matching_free_region(jp,first_block->lcn,clusters_to_move,FIND_MATCHING_RGN_FORWARD);
			//rgn = find_last_free_region(jp,clusters_to_move);
			if(rgn != NULL){
				if(rgn->lcn > first_block->lcn){
					move_file(first_file,current_vcn,clusters_to_move,
						rgn->lcn + rgn->length - clusters_to_move,0,jp);
                    jp->pi.total_moves ++;
				}
			}
			/* otherwise the volume processing is extremely slow and may even go to an infinite loop */
			first_file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
		}
		start_lcn ++; /* the current block will be skipped anyway in this case */
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
