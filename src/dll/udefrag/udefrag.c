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
 * @file udefrag.c
 * @brief Driver interaction code.
 * @addtogroup Driver
 * @{
 */

#include "../../include/ntndk.h"

#include "../../include/udefrag.h"
#include "../zenwinx/zenwinx.h"

#define DbgCheckInitEvent(f) { \
	if(!init_event){ \
		DebugPrint(f " call without initialization!"); \
		return (-1); \
	} \
}

struct kernel_start_parameters {
	char *volume_name;
	UDEFRAG_JOB_TYPE job_type;
	int cluster_map_size;
	int cmd_status;
	BOOL done_flag;
};

struct udefrag_options {
	ULONGLONG time_limit;
	int refresh_interval;
};

/* global variables */
HANDLE init_event = NULL;
char result_msg[4096]; /* buffer for the default formatted result message */

#ifndef STATIC_LIB
/**
 * @brief udefrag.dll entry point.
 */
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	return 1;
}
#endif

/**
 * @brief Initializes all libraries required 
 * for the native application.
 * @note Designed especially to replace DllMain
 * functionality in case of monolithic native application.
 * Call this routine in the beginning of NtProcessStartup() code.
 */
void __stdcall udefrag_monolithic_native_app_init(void)
{
	zenwinx_native_init();
	udefrag_kernel_native_init();
}

/**
 * @brief Frees resources of all libraries required 
 * for the native application.
 * @note Designed especially to replace DllMain
 * functionality in case of monolithic native application.
 * Don't call it before winx_shutdown() and winx_reboot(),
 * but call always before winx_exit().
 */
void __stdcall udefrag_monolithic_native_app_unload(void)
{
	udefrag_kernel_native_unload();
	zenwinx_native_unload();
}

/**
 * @brief Initializes the UltraDefrag engine.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall udefrag_init(void)
{
	int error_code;
	
	/* only a single instance of the program ! */
	error_code = winx_create_event(L"\\udefrag_init",SynchronizationEvent,&init_event);
	if(error_code < 0){
		if(error_code == STATUS_OBJECT_NAME_COLLISION) return UDEFRAG_ALREADY_RUNNING;
		return (-1);
	}
	return 0;
}

/**
 * @brief Unloads the UltraDefrag engine.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall udefrag_unload(void)
{
	NtCloseSafe(init_event);
	return 0;
}

/**
 * @brief Reloads udefrag.dll specific options.
 */
static void udefrag_reload_settings(struct udefrag_options *udo)
{
	#define ENV_BUFFER_LENGTH 128
	short env_buffer[ENV_BUFFER_LENGTH];
	char buf[ENV_BUFFER_LENGTH];
	ULONGLONG i;
	
	/* reset all parameters */
	udo->refresh_interval  = DEFAULT_REFRESH_INTERVAL;
	udo->time_limit = 0;

	if(winx_query_env_variable(L"UD_TIME_LIMIT",env_buffer,ENV_BUFFER_LENGTH) >= 0){
		(void)_snprintf(buf,ENV_BUFFER_LENGTH - 1,"%ws",env_buffer);
		buf[ENV_BUFFER_LENGTH - 1] = 0;
		udo->time_limit = winx_str2time(buf);
	}
	DebugPrint("Time limit = %I64u seconds\n",udo->time_limit);

	if(winx_query_env_variable(L"UD_REFRESH_INTERVAL",env_buffer,ENV_BUFFER_LENGTH) >= 0)
		udo->refresh_interval = _wtoi(env_buffer);
	DebugPrint("Refresh interval = %u msec\n",udo->refresh_interval);
	
	(void)strcpy(buf,"");
	(void)winx_dfbsize(buf,&i); /* to force MinGW export udefrag_dfbsize */
	(void)i;
}

/**
 * @brief Thread procedure delivering a disk 
 *        defragmentation command to the driver.
 * @note
 * - Only a single command may be sent at the same time.
 * - Internal use only.
 */
DWORD WINAPI engine_start(LPVOID p)
{
	struct kernel_start_parameters *pksp;
	
	pksp = (struct kernel_start_parameters *)p;
	pksp->cmd_status = udefrag_kernel_start(pksp->volume_name,pksp->job_type,pksp->cluster_map_size);
	pksp->done_flag = TRUE;
	winx_exit_thread(); /* 8k/12k memory leak here? */
	return 0;
}

/**
 * @brief Delivers a disk defragmentation command to the driver
 *        in a separate thread.
 * @param[in] volume_name the name of the volume.
 * @param[in] job_type the type of the job.
 * @param[in] cluster_map_size the size of the cluster map, in bytes.
 * @param[in] sproc an address of the callback procedure
 *                  to be called periodically during
 *                  the running disk defragmentation job.
 *                  This parameter may be NULL.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall udefrag_start(char *volume_name, UDEFRAG_JOB_TYPE job_type, int cluster_map_size, STATUPDATEPROC sproc)
{
	struct kernel_start_parameters ksp;
	struct udefrag_options udo;
	ULONGLONG t = 0;
	int use_limit = 0;

	DbgCheckInitEvent("udefrag_start");
	
	udefrag_reload_settings(&udo);

	/* initialize kernel_start_parameters structure */
	ksp.volume_name = volume_name;
	ksp.job_type = job_type;
	ksp.cluster_map_size = cluster_map_size;
	ksp.cmd_status = 0;
	ksp.done_flag = FALSE;
	
	/* create a separate thread for the command processing */
	if(winx_create_thread(engine_start,(PVOID)&ksp,NULL) < 0){
		//winx_printf("\nFUCKED!\n\n");
		return (-1);
	}

	/*
	* Call specified callback every refresh_interval milliseconds.
	* http://sourceforge.net/tracker/index.php?func=
	* detail&aid=2886353&group_id=199532&atid=969873
	*/
	if(udo.time_limit){
		use_limit = 1;
		t = udo.time_limit * 1000;
	}
	do {
		winx_sleep(udo.refresh_interval);
		if(sproc) sproc(FALSE);
		if(use_limit){
			if(t <= udo.refresh_interval){
				DebugPrint("Time limit exceeded!\n");
				(void)udefrag_stop();
			} else {
				t -= udo.refresh_interval;
			}
		}
	} while(!ksp.done_flag);
	if(sproc) sproc(TRUE);

	if(ksp.cmd_status < 0)
		DebugPrint("udefrag_start(%s,%u,%u,0x%p) failed!\n",
			volume_name,job_type,cluster_map_size,sproc);
	return ksp.cmd_status;
}

/**
 * @brief Stops the running disk defragmentation job.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall udefrag_stop(void)
{
	DbgCheckInitEvent("udefrag_stop");
	if(udefrag_kernel_stop() >= 0) return 0;
	DebugPrint("Stop request failed!");
	return (-1);
}

/**
 * @brief Retrieves the progress information
 *        of the running disk defragmentation job.
 * @param[out] pstat pointer to the STATISTIC structure.
 * @param[out] percentage pointer to the variable 
 *                        receiving progress percentage.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall udefrag_get_progress(STATISTIC *pstat, double *percentage)
{
	double x, y;
	
	DbgCheckInitEvent("udefrag_get_progress");

	if(udefrag_kernel_get_statistic(pstat,NULL,0) < 0){
		DebugPrint("Statistical data unavailable!");
		return (-1);
	}

	if(percentage){ /* calculate percentage only if we have such request */
		/* FIXME: do it more accurate */
		x = (double)(LONGLONG)pstat->processed_clusters;
		y = (double)(LONGLONG)pstat->clusters_to_process;
		if(y == 0) *percentage = 0.00;
		else *percentage = (x / y) * 100.00;
	}
	return 0;
}

/**
 * @brief Retrieves the cluster map
 *        of the currently processing volume.
 * @param[out] buffer pointer to the map buffer.
 * @param[in] size the buffer size, in bytes.
 * @return Zero for success, negative value otherwise. 
 */
int __stdcall udefrag_get_map(char *buffer,int size)
{
	DbgCheckInitEvent("udefrag_get_map");
	
	if(udefrag_kernel_get_statistic(NULL,buffer,(long)size) < 0){
		DebugPrint("Cluster map unavailable!");
		return (-1);
	}
	return 0;
}

/**
 * @brief Retrieves the default formatted results of the 
 *        completed disk defragmentation job.
 * @param[in] pstat pointer to the STATISTIC structure,
 *                  filled by udefrag_get_progress() call.
 * @return A string containing default formatted results of the
 *         disk defragmentation job defined in passed structure.
 * @note This function may be useful for console
 *       and native applications.
 */
char * __stdcall udefrag_get_default_formatted_results(STATISTIC *pstat)
{
	char total_space[68];
	char free_space[68];
	double p;
	unsigned int ip;

	(void)winx_fbsize(pstat->total_space,2,total_space,sizeof(total_space));
	(void)winx_fbsize(pstat->free_space,2,free_space,sizeof(free_space));
	if(pstat->filecounter == 0) p = 0.00;
	else p = (double)(pstat->fragmcounter)/((double)(pstat->filecounter));
	ip = (unsigned int)(p * 100.00);
	if(ip < 100) ip = 100; /* fix round off error */
	(void)_snprintf(result_msg,sizeof(result_msg) - 1,
			  "Volume information:\r\n\r\n"
			  "  Volume size                  = %s\r\n"
			  "  Free space                   = %s\r\n\r\n"
			  "  Total number of files        = %u\r\n"
			  "  Number of fragmented files   = %u\r\n"
			  "  Fragments per file           = %u.%02u\r\n\r\n",
			  total_space,
			  free_space,
			  pstat->filecounter,
			  pstat->fragmfilecounter,
			  ip / 100, ip % 100
			 );
	result_msg[sizeof(result_msg) - 1] = 0;
	return result_msg;
}

/**
 * @brief Retrieves a human readable error description
 * for the error codes defined in udefrag.h header file.
 * @param[in] error_code the error code.
 * @return A pointer to zero-terminated ANSI string 
 * containing detailed error description.
 */
char * __stdcall udefrag_get_error_description(int error_code)
{
	switch(error_code){
	case UDEFRAG_UNKNOWN_ERROR:
		return "Some unknown internal bug or some\n"
		       "rarely arising error has been encountered.";
	case UDEFRAG_ALREADY_RUNNING:
		return "You can run only one instance of UltraDefrag!";
	case UDEFRAG_W2K_4KB_CLUSTERS:
		return "NTFS volumes with cluster size greater than 4 kb\n"
		       "cannot be defragmented on Windows 2000.";
	case UDEFRAG_NO_MEM:
		return "Not enough memory.";
	case UDEFRAG_CDROM:
		return "It is impossible to defragment CDROM drive.";
	case UDEFRAG_REMOTE:
		return "It is impossible to defragment remote volume.";
	case UDEFRAG_ASSIGNED_BY_SUBST:
		return "It is impossible to defragment volumes\n"
		       "assigned by \'subst\' command.";
	case UDEFRAG_REMOVABLE:
		return "You are trying to defragment removable volume.\n"
		       "If the volume type was wrong identified, send\n"
			   "a bug report to the author, please.";
	}
	return "";
}

/** @} */
