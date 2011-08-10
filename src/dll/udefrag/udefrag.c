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
 * @file udefrag.c
 * @brief Entry point.
 * @addtogroup Engine
 * @{
 */

#include "udefrag-internals.h"

/**
 */
static void dbg_print_header(udefrag_job_parameters *jp)
{
	int os_version;
	int mj, mn;
	char ch;
	winx_time t;

	/* print driver version */
	winx_dbg_print_header(0,0,"*");
	winx_dbg_print_header(0x20,0,"%s",VERSIONINTITLE);

	/* print windows version */
	os_version = winx_get_os_version();
	mj = os_version / 10;
	mn = os_version % 10;
	winx_dbg_print_header(0x20,0,"Windows NT %u.%u",mj,mn);
	
	/* print date and time */
	memset(&t,0,sizeof(winx_time));
	(void)winx_get_local_time(&t);
	winx_dbg_print_header(0x20,0,"[%02i.%02i.%04i at %02i:%02i]",
		(int)t.day,(int)t.month,(int)t.year,(int)t.hour,(int)t.minute);
	winx_dbg_print_header(0,0,"*");
	
	/* force MinGW to export both udefrag_tolower and udefrag_toupper */
	ch = 'a';
	ch = winx_tolower(ch) - winx_toupper(ch);
}

/**
 */
static void dbg_print_footer(udefrag_job_parameters *jp)
{
	winx_dbg_print_header(0,0,"*");
	winx_dbg_print_header(0,0,"Processing of %c: %s",
		jp->volume_letter, (jp->pi.completion_status > 0) ? "succeeded" : "failed");
	winx_dbg_print_header(0,0,"*");
}

/**
 * @brief Delivers progress information to the caller.
 * @note 
 * - completion_status parameter delivers to the caller
 * instead of an appropriate field of jp->pi structure.
 * - If cluster map cell is occupied entirely by MFT zone
 * it will be drawn in light magenta if no files exist there.
 * Otherwise, such a cell will be drawn in different color
 * indicating that something still exists inside the zone.
 * This will help people to realize whether they need to run
 * full optimization or not.
 */
static void deliver_progress_info(udefrag_job_parameters *jp,int completion_status)
{
	udefrag_progress_info pi;
	double x, y;
	int i, k, index;
	int mft_zone_detected;
	int free_cell_detected;
	ULONGLONG maximum, n;
	
	if(jp->cb == NULL)
		return;

	/* make a copy of jp->pi */
	memcpy(&pi,&jp->pi,sizeof(udefrag_progress_info));
	
	/* replace completion status */
	pi.completion_status = completion_status;
	
	/* calculate progress percentage */
	/* FIXME: do it more accurate */
	x = (double)(LONGLONG)pi.processed_clusters;
	y = (double)(LONGLONG)pi.clusters_to_process;
	if(y == 0) pi.percentage = 0.00;
	else pi.percentage = (x / y) * 100.00;
	
	/* refill cluster map */
	if(jp->pi.cluster_map && jp->cluster_map.array \
	  && jp->pi.cluster_map_size == jp->cluster_map.map_size){
		for(i = 0; i < jp->cluster_map.map_size; i++){
			/* check for mft zone to apply special rules there */
			mft_zone_detected = free_cell_detected = 0;
			maximum = 1; /* for jp->cluster_map.opposite_order */
			if(!jp->cluster_map.opposite_order){
				if(i == jp->cluster_map.map_size - 1)
					maximum = jp->cluster_map.clusters_per_last_cell;
				else
					maximum = jp->cluster_map.clusters_per_cell;
			}
			if(jp->cluster_map.array[i][MFT_ZONE_SPACE] >= maximum)
				mft_zone_detected = 1;
			if(jp->cluster_map.array[i][FREE_SPACE] >= maximum)
				free_cell_detected = 1;
			if(mft_zone_detected && free_cell_detected){
				jp->pi.cluster_map[i] = MFT_ZONE_SPACE;
			} else {
				maximum = jp->cluster_map.array[i][0];
				index = 0;
				for(k = 1; k < jp->cluster_map.n_colors; k++){
					n = jp->cluster_map.array[i][k];
					if(n >= maximum){ /* support of colors precedence  */
						if(k != MFT_ZONE_SPACE || !mft_zone_detected){
							maximum = n;
							index = k;
						}
					}
				}
				if(maximum == 0)
					jp->pi.cluster_map[i] = SYSTEM_SPACE;
				else
					jp->pi.cluster_map[i] = (char)index;
			}
		}
	}
	
	/* deliver information to the caller */
	jp->cb(&pi,jp->p);
	jp->progress_refresh_time = winx_xtime();
	if(jp->udo.dbgprint_level >= DBG_PARANOID)
		winx_dbg_print_header(0x20,0,"progress update");
}

/*
* This technique forces to refresh progress
* indication not so smoothly as desired.
*/
/**
 * @brief Delivers progress information to the caller,
 * no more frequently than specified in UD_REFRESH_INTERVAL
 * environment variable.
 */
void __stdcall progress_router(void *p)
{
	udefrag_job_parameters *jp = (udefrag_job_parameters *)p;

	if(jp->cb){
		/* ensure that jp->udo.refresh_interval exceeded */
		if((winx_xtime() - jp->progress_refresh_time) > jp->udo.refresh_interval){
			deliver_progress_info(jp,jp->pi.completion_status);
		} else if(jp->pi.completion_status){
			/* deliver completed job information anyway */
			deliver_progress_info(jp,jp->pi.completion_status);
		}
	}
}

/**
 * @brief Calls terminator registered by caller.
 * When time interval specified in UD_TIME_LIMIT
 * environment variable elapses, it terminates
 * the job immediately.
 */
int __stdcall termination_router(void *p)
{
	udefrag_job_parameters *jp = (udefrag_job_parameters *)p;
	int result;

	/* check for time limit */
	if(jp->udo.time_limit){
		if((winx_xtime() - jp->start_time) / 1000 > jp->udo.time_limit){
			winx_dbg_print_header(0,0,"@ time limit exceeded @");
			return 1;
		}
	}

	/* ask caller */
	if(jp->t){
		result = jp->t(jp->p);
		if(result){
			winx_dbg_print_header(0,0,"*");
			winx_dbg_print_header(0x20,0,"termination requested by caller");
			winx_dbg_print_header(0,0,"*");
		}
		return result;
	}

	/* continue */
	return 0;
}

/*
* Another multithreaded technique delivers progress info more smoothly.
*/
static int __stdcall terminator(void *p)
{
	udefrag_job_parameters *jp = (udefrag_job_parameters *)p;
	int result;

	/* ask caller */
	if(jp->t){
		result = jp->t(jp->p);
		if(result){
			winx_dbg_print_header(0,0,"*");
			winx_dbg_print_header(0x20,0,"termination requested by caller");
			winx_dbg_print_header(0,0,"*");
		}
		return result;
	}

	/* continue */
	return 0;
}

static int __stdcall killer(void *p)
{
	winx_dbg_print_header(0,0,"*");
	winx_dbg_print_header(0x20,0,"termination requested by caller");
	winx_dbg_print_header(0,0,"*");
	return 1;
}

/**
 * @brief Calculates a maximum amount of data which may be moved in process.
 */
static ULONGLONG calculate_amount_of_data_to_be_moved(udefrag_job_parameters *jp)
{
	ULONGLONG clusters_to_process = 0;
	udefrag_fragmented_file *f;
	winx_file_info *file;

	switch(jp->job_type){
	case DEFRAGMENTATION_JOB:
		for(f = jp->fragmented_files; f; f = f->next){
			if(jp->termination_router((void *)jp)) break;
			/*
			* Count all fragmented files which can be processed.
			*/
			if(can_defragment(f->f,jp))
				clusters_to_process += get_file_length(jp,f->f);
			if(f->next == jp->fragmented_files) break;
		}
		break;
	case QUICK_OPTIMIZATION_JOB:
	case FULL_OPTIMIZATION_JOB:
		/*
		* We have a chance to move all data
		* to the end of disk and then in contrary
		* direction.
		*/
		for(file = jp->filelist; file; file = file->next){
			if(can_move(file,jp))
				clusters_to_process += get_file_length(jp,file);
			if(file->next == jp->filelist) break;
		}
		/* FIXME: avoid overflow on volumes larger than 8 Eb in size */
		clusters_to_process *= 2;
		break;
	default:
		break;
	}
	
	return clusters_to_process;
}

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
static ULONGLONG calculate_starting_point(udefrag_job_parameters *jp, ULONGLONG old_sp)
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
	if(fragmented < /*jp->free_rgn_size_threshold*/ (new_sp - old_sp) / 20) return new_sp;

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
	if(new_sp <= old_sp + 1)
		return old_sp;
	
	/*
	* Release all remaining data when all space
	* between new_sp and old_sp is heavily fragmented.
	*/
	if(fragmented >= (new_sp - old_sp + 1) / 3)
		return old_sp; /* because at least 1/3 of skipped space is fragmented */
	
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
	if(new_sp <= old_sp + 1)
		return old_sp;
	
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
	return new_sp;
}

/*
* How statistical data adjusts in all the volume processing routines:
* 1. we calculate a maximum amount of data which may be moved in process
*    and assign this value to jp->pi.clusters_to_process counter
* 2. when we move something, we adjust jp->pi.processed_clusters
* 3. when we skip something, we adjust that counter too
* Therefore, regardless of number of algorithm passes, we'll have
* always a true progress percentage gradually increasing from 0% to 100%.
*
* To avoid infinite loops in full optimization we define starting point 
* and move it forward on each algorithm pass.
*
* Infinite loops in quick optimization aren't possible because number
* of fragmented files instantly descreases and all files become closer
* and closer to the beginning of the volume. To reduce number of passes 
* and make it more effective we use a starting point concept here too.
*
* Infinite loops in defragmentation aren't possible because of instant 
* decreasing number of fragmented files.
*
* NOTE: progress over 100% means deeper processing than expected.
* This is not a bug, this is an algorithm feature causing by iterational
* nature of multipass processing.
*/

static DWORD WINAPI start_job_ex(LPVOID p)
{
	udefrag_job_parameters *jp = (udefrag_job_parameters *)p;
	char *action = "analyzing";
	int result = 0;
	ULONGLONG start_lcn, new_start_lcn;
	/* variables indicating whether something has been moved or not */
	ULONGLONG mx = 0, my = 0, mz = 0;
	/* variables indicating whether something failed or not */
	int rx = -1, ry = -1, rz = -1;
	int optimize_entire_disk = 0;
	//ULONGLONG fragmented_clusters;
	
	/* check for preview masks */
	if(jp->udo.job_flags & UD_JOB_REPEAT)
		DebugPrint("Repeat action until nothing left to move");
	if(jp->udo.job_flags & UD_PREVIEW_MATCHING)
		DebugPrint("Preview -> Find matching free space");
	else
		DebugPrint("Preview -> Find largest free space");
	
	/* use 'Find largest free space' strategy in optimization */
	if(jp->job_type == FULL_OPTIMIZATION_JOB || jp->job_type == QUICK_OPTIMIZATION_JOB)
		jp->udo.job_flags &= ~UD_PREVIEW_MATCHING;

	/* do the job */
	if(jp->job_type == DEFRAGMENTATION_JOB) action = "defragmenting";
	else if(jp->job_type == FULL_OPTIMIZATION_JOB) action = "optimizing";
	else if(jp->job_type == QUICK_OPTIMIZATION_JOB) action = "quick optimizing";
	winx_dbg_print_header(0,0,"Start %s volume %c:",action,jp->volume_letter);
	remove_fragmentation_reports(jp);
	(void)winx_vflush(jp->volume_letter); /* flush all file buffers */
	
	result = analyze(jp); /* we need to call it once, here */
	if(jp->job_type == ANALYSIS_JOB || result < 0) goto done;

	jp->pi.clusters_to_process = calculate_amount_of_data_to_be_moved(jp);
	jp->pi.processed_clusters = 0;
	start_lcn = 0;
	if(jp->job_type != DEFRAGMENTATION_JOB)
		calculate_free_rgn_size_threshold(jp);
	
	/*
	* Optimize entire disk if free space amount 
	* is less than 20% and at least a half of data
	* is fragmented.
	*/
	/*if(jp->job_type != DEFRAGMENTATION_JOB){
		if(jp->v_info.free_bytes < jp->v_info.total_bytes / 5){
			fragmented_clusters = get_number_of_fragmented_clusters(jp,0,jp->v_info.total_clusters - 1);
			if(fragmented_clusters >= jp->v_info.total_clusters / 2)
				optimize_entire_disk = 1;
		}
	}*/
	
	/* for map redraw testing */
	/*my = 100;
	mx = jp->v_info.total_clusters - my;
	colorize_map_region(jp,mx,my,FRAGM_SPACE,FREE_SPACE);
	goto done;*/
    
    /* optimize MFT separately to keep its optimal location */
	(void)optimize_mft(jp); /* ignore result because this task is not mandatory */
	
	while(!jp->termination_router((void *)jp)){
		/* define starting point */
		if(jp->job_type != DEFRAGMENTATION_JOB){
			if(jp->pi.pass_number == 0 && optimize_entire_disk)
				new_start_lcn = 0;
			else
				new_start_lcn = calculate_starting_point(jp,start_lcn);
			if(new_start_lcn <= start_lcn && jp->pi.pass_number){
				DebugPrint("volume optimization completed: old_sp = %I64u, new_sp = %I64u",
					start_lcn, new_start_lcn);
				break;
			}
			start_lcn = new_start_lcn;
			DebugPrint("volume optimization pass #%u, starting point = %I64u",
				jp->pi.pass_number, start_lcn);
		}
		
		/* cleanup space after start_lcn */
		if(jp->job_type == FULL_OPTIMIZATION_JOB){
			rx = move_files_to_back(jp, start_lcn, MOVE_ALL);
			mx = jp->pi.moved_clusters;
		}
		
		/* TODO: this routine makes a lot of slow move_file() calls,
		try to comment it out and test effectiveness of the quick optimization
		in this case */
		if(jp->job_type == QUICK_OPTIMIZATION_JOB/* && jp->pi.pass_number == 0*/){
			rx = move_files_to_back(jp, start_lcn, MOVE_FRAGMENTED);
			mx = jp->pi.moved_clusters;
		}
		if(jp->termination_router((void *)jp)) break;
		
		/* defragment */
		ry = defragment(jp);
		my = jp->pi.moved_clusters;
		if(jp->termination_router((void *)jp)) break;
		
		/* move not fragmented files as close to the beginning of the volume as possible */
		rz = move_files_to_front(jp, start_lcn, MOVE_NOT_FRAGMENTED);
		mz = jp->pi.moved_clusters;
	
		/* exit if nothing moved */
		if(mx == 0 && my == 0 && mz == 0){
			if(jp->pi.pass_number == 0 && rx < 0 && ry < 0 && rz < 0)
				result = -1; /* no actions succeeded */
			break;
		}
		
		/* exit if no repeat */
		if(!(jp->udo.job_flags & UD_JOB_REPEAT)) break;
		
		/* go to the next pass */
		jp->pi.pass_number ++;
	}

    /* partial defragment only once, but never in optimization */
	if(jp->job_type == DEFRAGMENTATION_JOB){
		if(!(jp->termination_router((void *)jp) || \
			(jp->udo.job_flags & UD_PREVIEW_SKIP_PARTIAL))){
				rx = defragment_partial(jp);
				if(result < 0) /* all previous actions failed */
					result = rx;
		}
	}

done:	
	release_temp_space_regions(jp);
	(void)save_fragmentation_reports(jp);
	
	/* now it is safe to adjust the completion status */
	jp->pi.completion_status = result;
	if(jp->pi.completion_status == 0)
	jp->pi.completion_status ++; /* success */
	
	winx_exit_thread(); /* 8k/12k memory leak here? */
	return 0;
}

#if 0
static DWORD WINAPI start_job(LPVOID p)
{
	udefrag_job_parameters *jp = (udefrag_job_parameters *)p;
	char *action = "analyzing";
	int result = 0;

	/* do the job */
	if(jp->job_type == DEFRAGMENTATION_JOB) action = "defragmenting";
	else if(jp->job_type == FULL_OPTIMIZATION_JOB) action = "optimizing";
	else if(jp->job_type == QUICK_OPTIMIZATION_JOB) action = "quick optimizing";
	winx_dbg_print_header(0,0,"Start %s volume %c:",action,jp->volume_letter);
	remove_fragmentation_reports(jp);
	(void)winx_vflush(jp->volume_letter); /* flush all file buffers */
	switch(jp->job_type){
	case ANALYSIS_JOB:
		result = analyze(jp);
		break;
	case DEFRAGMENTATION_JOB:
		result = defragment(jp);
		break;
	case FULL_OPTIMIZATION_JOB:
		result = full_optimize(jp);
		break;
	case QUICK_OPTIMIZATION_JOB:
		result = quick_optimize(jp);
		break;
	default:
		result = 0;
		break;
	}
	(void)save_fragmentation_reports(jp);

	/* now it is safe to adjust the completion status */
	jp->pi.completion_status = result;
	if(jp->pi.completion_status == 0)
		jp->pi.completion_status ++; /* success */

	winx_exit_thread(); /* 8k/12k memory leak here? */
	return 0;
}
#endif

/**
 * @brief Destroys list of free regions, 
 * list of files and list of fragmented files.
 */
void destroy_lists(udefrag_job_parameters *jp)
{
	winx_scan_disk_release(jp->filelist);
	winx_release_free_volume_regions(jp->free_regions);
	winx_list_destroy((list_entry **)(void *)&jp->fragmented_files);
	jp->filelist = NULL;
	jp->free_regions = NULL;
}

/**
 * @brief Starts disk analysis/defragmentation/optimization job.
 * @param[in] volume_letter the volume letter.
 * @param[in] job_type one of the xxx_JOB constants, defined in udefrag.h
 * @param[in] flags combination of UD_JOB_xxx and UD_PREVIEW_xxx flags defined in udefrag.h
 * @param[in] cluster_map_size size of the cluster map, in cells.
 * Zero value forces to avoid cluster map use.
 * @param[in] cb address of procedure to be called each time when
 * progress information updates, but no more frequently than
 * specified in UD_REFRESH_INTERVAL environment variable.
 * @param[in] t address of procedure to be called each time
 * when requested job would like to know whether it must be terminated or not.
 * Nonzero value, returned by terminator, forces the job to be terminated.
 * @param[in] p pointer to user defined data to be passed to both callbacks.
 * @return Zero for success, negative value otherwise.
 * @note [Callback procedures should complete as quickly
 * as possible to avoid slowdown of the volume processing].
 */
int __stdcall udefrag_start_job(char volume_letter,udefrag_job_type job_type,int flags,
		int cluster_map_size,udefrag_progress_callback cb,udefrag_terminator t,void *p)
{
	udefrag_job_parameters jp;
	ULONGLONG time = 0;
	int use_limit = 0;
	int result = -1;
	int win_version = winx_get_os_version();
    
	/* initialize the job */
	dbg_print_header(&jp);

	/* convert volume letter to uppercase - needed for w2k */
	volume_letter = winx_toupper(volume_letter);
	
	/* TEST: may fail on x64? */
	memset(&jp,0,sizeof(udefrag_job_parameters));
	jp.filelist = NULL;
	jp.fragmented_files = NULL;
	jp.free_regions = NULL;
	jp.progress_refresh_time = 0;
	
	jp.volume_letter = volume_letter;
	jp.job_type = job_type;
	jp.cb = cb;
	jp.t = t;
	jp.p = p;

	/*jp.progress_router = progress_router;
	jp.termination_router = termination_router;*/
	/* we'll deliver progress info from the current thread */
	jp.progress_router = NULL;
	/* we'll decide whether to kill or not from the current thread */
	jp.termination_router = terminator;

	jp.start_time = winx_xtime();
	jp.pi.completion_status = 0;
	
	if(get_options(&jp) < 0)
		goto done;
	
	jp.udo.job_flags = flags;

	if(allocate_map(cluster_map_size,&jp) < 0){
		release_options(&jp);
		goto done;
	}
	
    /* set additional privileges for Vista and above */
    if(win_version >= WINDOWS_VISTA){
        (void)winx_enable_privilege(SE_BACKUP_PRIVILEGE);
        
        if(win_version >= WINDOWS_7)
            (void)winx_enable_privilege(SE_MANAGE_VOLUME_PRIVILEGE);
    }
	
	/* run the job in separate thread */
	if(winx_create_thread(start_job_ex,(PVOID)&jp,NULL) < 0){
		free_map(&jp);
		release_options(&jp);
		goto done;
	}

	/*
	* Call specified callback every refresh_interval milliseconds.
	* http://sourceforge.net/tracker/index.php?func=
	* detail&aid=2886353&group_id=199532&atid=969873
	*/
	if(jp.udo.time_limit){
		use_limit = 1;
		/* FIXME: avoid overflow */
		time = jp.udo.time_limit * 1000;
	}
	do {
		winx_sleep(jp.udo.refresh_interval);
		deliver_progress_info(&jp,0); /* status = running */
		if(use_limit){
			if(time <= jp.udo.refresh_interval){
				/* time limit exceeded */
				winx_dbg_print_header(0,0,"*");
				winx_dbg_print_header(0x20,0,"time limit exceeded");
				winx_dbg_print_header(0,0,"*");
				jp.termination_router = killer;
			} else {
				if(jp.start_time){
					/* FIXME: avoid overflow */
					if(winx_xtime() - jp.start_time > jp.udo.time_limit * 1000)
						time = 0;
				} else {
					/* this method gives not so fine results, but requires no winx_xtime calls */
					time -= jp.udo.refresh_interval; 
				}
			}
		}
	} while(jp.pi.completion_status == 0);

	/* cleanup */
	deliver_progress_info(&jp,jp.pi.completion_status);
	/*if(jp.progress_router)
		jp.progress_router(&jp);*/ /* redraw progress */
	destroy_lists(&jp);
	free_map(&jp);
	release_options(&jp);
	
done:
	dbg_print_footer(&jp);
	if(jp.pi.completion_status > 0)
		result = 0;
	else if(jp.pi.completion_status < 0)
		result = jp.pi.completion_status;
	return result;
}

/**
 * @brief Retrieves the default formatted results 
 * of the completed disk defragmentation job.
 * @param[in] pi pointer to udefrag_progress_info structure.
 * @return A string containing default formatted results
 * of the disk defragmentation job. NULL indicates failure.
 * @note This function is used in console and native applications.
 */
char * __stdcall udefrag_get_default_formatted_results(udefrag_progress_info *pi)
{
	#define MSG_LENGTH 4095
	char *msg;
	char total_space[68];
	char free_space[68];
	double p;
	unsigned int ip;
	
	/* allocate memory */
	msg = winx_heap_alloc(MSG_LENGTH + 1);
	if(msg == NULL){
		DebugPrint("udefrag_get_default_formatted_results: cannot allocate %u bytes of memory",
			MSG_LENGTH + 1);
		winx_printf("\nCannot allocate %u bytes of memory!\n\n",
			MSG_LENGTH + 1);
		return NULL;
	}

	(void)winx_fbsize(pi->total_space,2,total_space,sizeof(total_space));
	(void)winx_fbsize(pi->free_space,2,free_space,sizeof(free_space));

	if(pi->files == 0){
		p = 0.00;
	} else {
		/*
		* Conversion to LONGLONG is used because of support
		* of MS Visual Studio 6.0 where conversion from ULONGLONG
		* to double is not implemented.
		*/
		p = (double)(LONGLONG)(pi->fragments)/((double)(LONGLONG)(pi->files));
	}
	ip = (unsigned int)(p * 100.00);
	if(ip < 100)
		ip = 100; /* fix round off error */

	(void)_snprintf(msg,MSG_LENGTH,
			  "Volume information:\n\n"
			  "  Volume size                  = %s\n"
			  "  Free space                   = %s\n\n"
			  "  Total number of files        = %u\n"
			  "  Number of fragmented files   = %u\n"
			  "  Fragments per file           = %u.%02u\n\n",
			  total_space,
			  free_space,
			  pi->files,
			  pi->fragmented,
			  ip / 100, ip % 100
			 );
	msg[MSG_LENGTH] = 0;
	return msg;
}

/**
 * @brief Releases memory allocated
 * by udefrag_get_default_formatted_results.
 * @param[in] results the string to be released.
 */
void __stdcall udefrag_release_default_formatted_results(char *results)
{
	if(results)
		winx_heap_free(results);
}

/**
 * @brief Retrieves a human readable error
 * description for ultradefrag error codes.
 * @param[in] error_code the error code.
 * @return A human readable description.
 */
char * __stdcall udefrag_get_error_description(int error_code)
{
	switch(error_code){
	case UDEFRAG_UNKNOWN_ERROR:
		return "Some unknown internal bug or some\n"
		       "rarely arising error has been encountered.";
	case UDEFRAG_FAT_OPTIMIZATION:
		return "FAT volumes cannot be optimized\n"
		       "because of unmovable directories.";
	case UDEFRAG_W2K_4KB_CLUSTERS:
		return "NTFS volumes with cluster size greater than 4 kb\n"
		       "cannot be defragmented on Windows 2000 and Windows NT 4.0";
	case UDEFRAG_NO_MEM:
		return "Not enough memory.";
	case UDEFRAG_CDROM:
		return "It is impossible to defragment CDROM drives.";
	case UDEFRAG_REMOTE:
		return "It is impossible to defragment remote volumes.";
	case UDEFRAG_ASSIGNED_BY_SUBST:
		return "It is impossible to defragment volumes\n"
		       "assigned by the \'subst\' command.";
	case UDEFRAG_REMOVABLE:
		return "You are trying to defragment a removable volume.\n"
		       "If the volume type was wrongly identified, send\n"
			   "a bug report to the author, thanks.";
	case UDEFRAG_UDF_DEFRAG:
		return "UDF volumes can neither be defragmented nor optimized,\n"
		       "because the file system driver does not support FSCTL_MOVE_FILE.";
	}
	return "";
}

/**
 * @brief Enables debug logging to the file
 * if <b>\%UD_LOG_FILE_PATH\%</b> is set, otherwise
 * disables the logging.
 * @details If log path does not exist and
 * cannot be created, logs are placed in
 * <b>\%tmp\%\\UltraDefrag_Logs</b> folder.
 * @return Zero for success, negative value
 * otherwise.
 */
int __stdcall udefrag_set_log_file_path(void)
{
	wchar_t *path;
	char *native_path, *path_copy, *filename;
	int result;
	
	path = winx_heap_alloc(ENV_BUFFER_SIZE * sizeof(wchar_t));
	if(path == NULL){
		DebugPrint("set_log_file_path: cannot allocate %u bytes of memory",
			ENV_BUFFER_SIZE * sizeof(wchar_t));
		return (-1);
	}
	
	result = winx_query_env_variable(L"UD_LOG_FILE_PATH",path,ENV_BUFFER_SIZE);
	if(result < 0 || path[0] == 0){
		/* empty variable forces to disable log */
		winx_disable_dbg_log();
		winx_heap_free(path);
		return 0;
	}
	
	/* convert to native path */
	native_path = winx_sprintf("\\??\\%ws",path);
	if(native_path == NULL){
		DebugPrint("set_log_file_path: cannot build native path");
		winx_heap_free(path);
		return (-1);
	}
	
	/* delete old logfile */
	winx_delete_file(native_path);
	
	/* ensure that target path exists */
	result = 0;
	path_copy = winx_strdup(native_path);
	if(path_copy == NULL){
		DebugPrint("set_log_file_path: not enough memory");
	} else {
		winx_path_remove_filename(path_copy);
		result = winx_create_path(path_copy);
		winx_heap_free(path_copy);
	}
	
	/* if target path cannot be created, use %tmp%\UltraDefrag_Logs */
	if(result < 0){
		result = winx_query_env_variable(L"TMP",path,ENV_BUFFER_SIZE);
		if(result < 0 || path[0] == 0){
			DebugPrint("set_log_file_path: failed to query %%TMP%%");
		} else {
			filename = winx_strdup(native_path);
			if(filename == NULL){
				DebugPrint("set_log_file_path: cannot allocate memory for filename");
			} else {
				winx_path_extract_filename(filename);
				winx_heap_free(native_path);
				native_path = winx_sprintf("\\??\\%ws\\UltraDefrag_Logs\\%s",path,filename);
				if(native_path == NULL){
					DebugPrint("set_log_file_path: cannot build %%tmp%%\\UltraDefrag_Logs\\{filename}");
				} else {
					/* delete old logfile from the temporary folder */
					winx_delete_file(native_path);
				}
				winx_heap_free(filename);
			}
		}
	}
	
	if(native_path)
		winx_enable_dbg_log(native_path);
	
	winx_heap_free(native_path);
	winx_heap_free(path);
	return 0;
}

#ifndef STATIC_LIB
/**
 * @brief udefrag.dll entry point.
 */
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	/*
	* Both command line and GUI clients support
	* UD_LOG_FILE_PATH environment variable
	* to control debug logging to the file.
	*/
	if(dwReason == DLL_PROCESS_ATTACH){
		(void)udefrag_set_log_file_path();
	} else if(dwReason == DLL_PROCESS_DETACH){
	}
	return 1;
}
#endif

/** @} */
