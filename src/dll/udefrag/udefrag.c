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
#include "../../include/ultradfg.h"
#include "../zenwinx/zenwinx.h"

#include "../../include/udefrag-kernel.h"

#define DbgCheckInitEvent(f) { \
	if(!init_event){ \
		DebugPrint(f " call without initialization!"); \
		return (-1); \
	} \
}

/* global variables */
BOOL kernel_mode_driver = TRUE;
long cluster_map_size = 0;

HANDLE init_event = NULL;
char result_msg[4096]; /* buffer for the default formatted result message */

extern int refresh_interval;
extern ULONGLONG time_limit;

unsigned char c, lett;
BOOL done_flag;
int cmd_status;

int __stdcall kmd_init(long map_size);
int __stdcall kmd_unload(void);
void __stdcall udefrag_load_settings(void);
#ifdef KERNEL_MODE_DRIVER_SUPPORT
WINX_FILE *f_ud = NULL;
WINX_FILE *f_map = NULL, *f_stat = NULL, *f_stop = NULL;
#endif

/**
 * @brief udefrag.dll entry point.
 */
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	return 1;
}

/**
 * @brief Initializes the UltraDefrag driver.
 * @param[in] map_size the cluster map size,
 *                     in bytes. May be zero.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall udefrag_init(long map_size)
{
	int error_code;
	
	/* enable neccessary privileges */
	/*(void)winx_enable_privilege(SE_MANAGE_VOLUME_PRIVILEGE); */
	(void)winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE); /* required by GUI client */
	
	/* only a single instance of the program ! */
	error_code = winx_create_event(L"\\udefrag_init",SynchronizationEvent,&init_event);
	if(error_code < 0){
		if(error_code == STATUS_OBJECT_NAME_COLLISION) return UDEFRAG_ALREADY_RUNNING;
		return (-1);
	}

#if !defined(UDEFRAG_PORTABLE) && defined(KERNEL_MODE_DRIVER_SUPPORT)
	if(kmd_init(map_size) >= 0){
		kernel_mode_driver = TRUE;
		return 0;
	}
#endif

	kernel_mode_driver = FALSE;
	cluster_map_size = map_size;
	(void)udefrag_load_settings(); /* reload udefrag.dll specific options */
	return 0;
}

/**
 * @brief Defines is kernel mode driver loaded or not.
 * @return TRUE if kernel mode driver is loaded, FALSE otherwise.
 */
int __stdcall udefrag_kernel_mode(void)
{
	return (int)kernel_mode_driver;
}

/**
 * @brief Unloads the UltraDefrag driver.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall udefrag_unload(void)
{
	if(!init_event) return 0;
	winx_destroy_event(init_event);
	init_event = NULL;
	(void)kmd_unload();
	return 0;
}

/**
 * @brief Delivers a disk defragmentation command to the driver.
 * @param[in] command the command to be delivered.
 * @param[in] letter the volume letter.
 * @return Zero for success, negative value otherwise.
 * @note Internal use only.
 */
int udefrag_send_command(unsigned char command,unsigned char letter)
{
	char cmd[4];
	char *cmd_description = "OPTIMIZE";
	UDEFRAG_JOB_TYPE job_type;
	int error_code;

	switch(command){
	case 'a':
	case 'A':
		job_type = ANALYSE_JOB;
		cmd_description = "ANALYSE";
		break;
	case 'd':
	case 'D':
		job_type = DEFRAG_JOB;
		cmd_description = "DEFRAG";
		break;
	default:
		job_type = OPTIMIZE_JOB;
	}

#ifdef  KERNEL_MODE_DRIVER_SUPPORT
	if(kernel_mode_driver){
		cmd[0] = command; cmd[1] = letter; cmd[2] = 0;
		if(winx_fwrite(cmd,strlen(cmd),1,f_ud)){
			return 0;
		} else {
			DebugPrint("Cannot execute %s command for volume %c:!",
				cmd_description,(char)toupper((int)letter));
			return (-1);
		}
	} else {
		cmd[0] = letter; cmd[1] = 0;
		error_code = udefrag_kernel_start(cmd,job_type,cluster_map_size);
		if(error_code >= 0){
			return 0;
		} else {
			DebugPrint("Cannot execute %s command for volume %c:!",
				cmd_description,(char)toupper((int)letter));
			return (-1);
		}
	}
#else
	cmd[0] = letter; cmd[1] = 0;
	error_code = udefrag_kernel_start(cmd,job_type,cluster_map_size);
	if(error_code >= 0){
		return 0;
	} else {
		DebugPrint("Cannot execute %s command for volume %c:!",
			cmd_description,(char)toupper((int)letter));
		return (-1);
	}
#endif
	return (-1); /* this point will never be reached */
}

/**
 * @brief Thread procedure delivering a disk 
 *        defragmentation command to the driver.
 * @note
 * - Only a single command may be sent at the same time.
 * - Internal use only.
 */
DWORD WINAPI send_command(LPVOID unused)
{
	cmd_status = udefrag_send_command(c,lett);
	done_flag = TRUE;
	winx_exit_thread(); /* 8k/12k memory leak here? */
	return 0;
}

/**
 * @brief Delivers a disk defragmentation command to the driver
 *        in a separate thread.
 * @param[in] command the command to be delivered.
 * @param[in] letter the volume letter.
 * @param[in] sproc an address of the callback procedure
 *                  to be called periodically during
 *                  the running disk defragmentation job.
 *                  This parameter may be NULL.
 * @return Zero for success, negative value otherwise.
 * @note udefrag_analyse, udefrag_defragment, udefrag_optimize
 *       macro definitions may be used instead of this function.
 */
int __stdcall udefrag_send_command_ex(char command,char letter,STATUPDATEPROC sproc)
{
	WINX_FILE *f;
	char volume[] = "\\??\\A:";
	HANDLE hThread;
	ULONGLONG t = 0;
	int use_limit = 0;

	DbgCheckInitEvent("udefrag_send_command_ex");

	/*
	* Here we can flush all file buffers. We cannot do it 
	* in driver due to te following two reasons:
	* 1. There is no NtFlushBuffersFile() call in kernel mode.
	* 2. IRP_MJ_FLUSH_BUFFERS request causes BSOD on NT 4.0
	* (at least under MS Virtual PC 2004).
	*/
	if(winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS) >= 0){
		volume[4] = letter;
		f = winx_fopen(volume,"r+");
		if(f){
			(void)winx_fflush(f);
			winx_fclose(f);
		}
		(void)winx_set_system_error_mode(1); /* equal to SetErrorMode(0) */
	}

	/* create a separate thread for driver command processing */
	done_flag = FALSE;
	cmd_status = 0;
	c = command; lett = letter;
	if(winx_create_thread(send_command,&hThread) < 0){
		//winx_printf("\nFUCKED!\n\n");
		return (-1);
	}
	NtCloseSafe(hThread);
	/*
	* Call specified callback 
	* every (settings.refresh_interval) milliseconds.
	*/
	if(time_limit){
		use_limit = 1;
		t = time_limit * 1000;
	}
	do {
		winx_sleep(refresh_interval);
		if(sproc) sproc(FALSE);
		if(use_limit){
			if(t <= refresh_interval){
				DebugPrint("Time limit exceeded!\n");
				(void)udefrag_stop();
			} else {
				t -= refresh_interval;
			}
		}
	} while(!done_flag);
	if(sproc) sproc(TRUE);

	return cmd_status;
}

/**
 * @brief Stops the running disk defragmentation job.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall udefrag_stop(void)
{
	DbgCheckInitEvent("udefrag_stop");
#ifdef  KERNEL_MODE_DRIVER_SUPPORT
	if(kernel_mode_driver){
		if(winx_fwrite("s",1,1,f_stop)) return 0;
	} else {
		if(udefrag_kernel_stop() >= 0) return 0;
	}
#else
	if(udefrag_kernel_stop() >= 0) return 0;
#endif
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

#ifdef  KERNEL_MODE_DRIVER_SUPPORT
	if(kernel_mode_driver){
		if(!winx_fread(pstat,sizeof(STATISTIC),1,f_stat)){
			DebugPrint("Statistical data unavailable!");
			return (-1);
		}
	} else {
		if(udefrag_kernel_get_statistic(pstat,NULL,0) < 0){
			DebugPrint("Statistical data unavailable!");
			return (-1);
		}
	}
#else
	if(udefrag_kernel_get_statistic(pstat,NULL,0) < 0){
		DebugPrint("Statistical data unavailable!");
		return (-1);
	}
#endif

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
	
#ifdef  KERNEL_MODE_DRIVER_SUPPORT
	if(kernel_mode_driver){
		if(winx_fread(buffer,size,1,f_map)) return 0;
	} else {
		if(udefrag_kernel_get_statistic(NULL,buffer,(long)size) >= 0)
			return 0;
	}
#else
	if(udefrag_kernel_get_statistic(NULL,buffer,(long)size) >= 0)
		return 0;
#endif

	DebugPrint("Cluster map unavailable!");
	return (-1);
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
			  "  Fragments per file           = %u.%02u\r\n",
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
		return "No enough memory.";
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
