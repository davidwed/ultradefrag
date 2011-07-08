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
 * @note completion_status parameter delivers to the caller
 * instead of an appropriate field of jp->pi structure.
 */
static void deliver_progress_info(udefrag_job_parameters *jp,int completion_status)
{
	udefrag_progress_info pi;
	double x, y;
	int i, k, index;
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
			maximum = jp->cluster_map.array[i][0];
			index = 0;
			for(k = 1; k < jp->cluster_map.n_colors; k++){
				n = jp->cluster_map.array[i][k];
				if(n >= maximum){ /* support of colors precedence  */
					maximum = n;
					index = k;
				}
			}
			if(maximum == 0)
				jp->pi.cluster_map[i] = SYSTEM_SPACE;
			else
				jp->pi.cluster_map[i] = (char)index;
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

	switch(jp->job_type){
	case DEFRAGMENTATION_JOB:
		for(f = jp->fragmented_files; f; f = f->next){
			if(jp->termination_router((void *)jp)) break;
			/*
			* Count all fragmented files which 
			* can be processed at least partially.
			*/
			if(can_defragment_partially(f->f,jp))
				clusters_to_process += f->f->disp.clusters;
			if(f->next == jp->fragmented_files) break;
		}
		break;
	case QUICK_OPTIMIZATION_JOB:
	case FULL_OPTIMIZATION_JOB:
		break;
	default:
		break;
	}
	
	return clusters_to_process;
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
* To avoid infinite loops in optimization we define starting point 
* and move it forward on each algorithm pass.
*
* Infinite loops in defragmentation aren't possible because of instant 
* decreasing number of fragmented files.
*/

static DWORD WINAPI start_job_ex(LPVOID p)
{
	udefrag_job_parameters *jp = (udefrag_job_parameters *)p;
	char *action = "analyzing";
	int result = 0;
	//ULONGLONG start_lcn = 0;
	
	/* check for preview masks */
	if(jp->udo.preview_flags & UD_PREVIEW_REPEAT)
		DebugPrint("Preview -> Repeat action until nothing left to move");
	if(jp->udo.preview_flags & UD_PREVIEW_MATCHING)
		DebugPrint("Preview -> Find matching free space");
	else
		DebugPrint("Preview -> Find largest free space");
	if((jp->udo.preview_flags & UD_PREVIEW_QUICK) && jp->job_type == FULL_OPTIMIZATION_JOB){
		DebugPrint("Preview -> Use quick optimize");
		jp->job_type = QUICK_OPTIMIZATION_JOB;
	}

	/* do the job */
	if(jp->job_type == DEFRAGMENTATION_JOB) action = "defragmenting";
	else if(jp->job_type == FULL_OPTIMIZATION_JOB) action = "optimizing";
	else if(jp->job_type == QUICK_OPTIMIZATION_JOB) action = "quick optimizing";
	winx_dbg_print_header(0,0,"Start %s volume %c:",action,jp->volume_letter);
	remove_fragmentation_reports(jp);
	(void)winx_vflush(jp->volume_letter); /* flush all file buffers */
	
	result = analyze(jp); /* needed for the next call */
	if(jp->job_type == ANALYSIS_JOB) goto done;

	jp->pi.clusters_to_process = calculate_amount_of_data_to_be_moved(jp);
	jp->pi.processed_clusters = 0;
	
	while(1){
		/* for dry-run analyze only on first pass */
		if(jp->pi.pass_number > 0 && jp->udo.dry_run == 0)
			result = analyze(jp);
		
		if(jp->termination_router((void *)jp)) break;
		
		if(result == 0){
			/* move all or fragmented clusters to the end of the volume
			starting with cluster 0 or the first fragmented cluster */
			
			if(jp->job_type == FULL_OPTIMIZATION_JOB)
				result = move_files_to_back(jp, MOVE_ALL);
			
			if(jp->job_type == QUICK_OPTIMIZATION_JOB)
				result = move_files_to_back(jp, MOVE_FRAGMENTED);
			
			/* defragment */
			if(jp->termination_router((void *)jp)) break;
			
			if(result == 0)
				result = defragment(jp);
			
			if(jp->termination_router((void *)jp)) break;
			
			/* move not fragmented files from
			the second half of the volume to the beginning of the volume */
			
			if(result == 0 && (jp->job_type == QUICK_OPTIMIZATION_JOB || jp->job_type == FULL_OPTIMIZATION_JOB))
				result = move_files_to_front(jp, MOVE_NOT_FRAGMENTED);
			
			/* partial defragment if nothing moved */
			if(result == 0 && jp->pi.moved_clusters == 0)
				result = defragment_partial(jp);
		
			if(jp->termination_router((void *)jp)) break;
		} else {
			break;
		}
	
		jp->pi.pass_number ++;
		
		/* exit if user selected stop */
		if(jp->termination_router((void *)jp)) break;
		
		/* exit if no repeat */
		if(!(jp->udo.preview_flags & UD_PREVIEW_REPEAT)) break;
		
		/* exit if nothing moved */
		if(jp->pi.moved_clusters == 0) break;
	}

done:	
	redraw_all_temporary_system_space_as_free(jp);
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
 * @param[in] preview_flags combination of UD_PREVIEW_xxx flags defined in udefrag.h
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
int __stdcall udefrag_start_job(char volume_letter,udefrag_job_type job_type,int preview_flags,
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
	
	jp.udo.preview_flags = preview_flags;

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
		time = jp.udo.time_limit * 1000;
	}
	do {
		winx_sleep(jp.udo.refresh_interval);
		deliver_progress_info(&jp,0); /* status = running */
		if(use_limit){
			if(time <= jp.udo.refresh_interval){
				/* time limit exceeded */
				jp.termination_router = killer;
			} else {
				time -= jp.udo.refresh_interval;
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
		       "because of unmoveable directories.";
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
