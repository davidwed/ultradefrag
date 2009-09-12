/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

/*
* udefrag.dll - middle layer between driver and user interfaces:
* driver interaction functions.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> /* for toupper() on mingw */
#include "../../include/ntndk.h"
#include "../../include/udefrag.h"
#include "../../include/ultradfg.h"
#include "../zenwinx/zenwinx.h"

#define NtCloseSafe(h) if(h) { NtClose(h); h = NULL; }

#ifndef __FUNCTION__
#define __FUNCTION__ "udefrag_xxx"
#endif
#define CHECK_INIT_EVENT() { \
	if(!init_event){ \
		winx_raise_error("E: %s call without initialization!", __FUNCTION__); \
		return -1; \
	} \
}

/* global variables */
char result_msg[4096]; /* buffer for the default formatted result message */
char user_mode_buffer[65536]; /* for nt 4.0 */
HANDLE init_event = NULL;
WINX_FILE *f_ud = NULL;
WINX_FILE *f_map = NULL, *f_stat = NULL, *f_stop = NULL;

extern int refresh_interval;

unsigned char c, lett;
BOOL done_flag;
int cmd_status;

ERRORHANDLERPROC eh;

void __stdcall ErrorHandler(short *msg)
{
	if(wcsstr(msg, L"c0000035")) /* STATUS_OBJECT_NAME_COLLISION */
		eh(L"You can run only one instance of UltraDefrag!");
	else eh(msg);
}

void __stdcall DefragErrorHandler(short *msg)
{
	if(wcsstr(msg, L"c0000002")) /* STATUS_NOT_IMPLEMENTED */
		eh(L"NTFS volumes with cluster size greater than 4 kb\n"
		   L"cannot be defragmented on Windows 2000.");
	else eh(msg);
}

/* functions */
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	/*
	* This code was commented 20 mar 2009, because it is 
	* a little bit dangerous:
	* GUI can restart itself and than unload the driver.
	*/
	/* here we have last chance to unload the driver */
/*	if(dwReason == DLL_PROCESS_DETACH)
		udefrag_unload();
*/	return 1;
}

/****f* udefrag.common/udefrag_init
* NAME
*    udefrag_init
* SYNOPSIS
*    error = udefrag_init(mapsize);
* FUNCTION
*    Ultra Defragmenter initialization procedure.
* INPUTS
*    mapsize - cluster map size, may be zero
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    int main(int argc, char **argv)
*    {
*        if(udefrag_init(0) < 0){
*            printf("udefrag_init() call unsuccessful!");
*            exit(1);
*        }
*        // your program code here
*        // ...
*    }
* NOTES
*    You must call this function before any other interaction
*    with driver attempts.
* SEE ALSO
*    udefrag_unload
******/
int __stdcall udefrag_init(long map_size)
{
	/* 1. Enable neccessary privileges */
	/*if(!EnablePrivilege(UserToken,SE_MANAGE_VOLUME_PRIVILEGE)) return (-1)*/
	if(winx_enable_privilege(SE_LOAD_DRIVER_PRIVILEGE) < 0) return (-1);

	/* 2. only one instance of the program ! */
	/* create init_event - this must be after privileges enabling */
	eh = winx_set_error_handler(ErrorHandler);
	if(winx_create_event(L"\\udefrag_init",SynchronizationEvent,&init_event) < 0){
		winx_set_error_handler(eh);
		return (-1);
	}
	winx_set_error_handler(eh);
	/* 3. Load the driver */
	if(winx_load_driver(L"ultradfg") < 0) return (-1);
	/* 4. Open our device */
	f_ud = winx_fopen("\\Device\\UltraDefrag","w");
	if(!f_ud) goto init_fail;
	f_map = winx_fopen("\\Device\\UltraDefragMap","r");
	if(!f_map) goto init_fail;
	f_stat = winx_fopen("\\Device\\UltraDefragStat","r");
	if(!f_stat) goto init_fail;
	f_stop = winx_fopen("\\Device\\UltraDefragStop","w");
	if(!f_stop) goto init_fail;
	/* 6. Set user mode buffer - nt 4.0 specific */
	if(winx_ioctl(f_ud,IOCTL_SET_USER_MODE_BUFFER,"User mode buffer setup",
		user_mode_buffer,0,NULL,0,NULL) < 0) goto init_fail;
	/* 7. Set cluster map size */
	if(winx_ioctl(f_ud,IOCTL_SET_CLUSTER_MAP_SIZE,"Cluster map buffer setup",
		&map_size,sizeof(long),NULL,0,NULL) < 0) goto init_fail;
	/* 8. Load settings */
	if(udefrag_reload_settings() < 0) goto init_fail;
	return 0;
init_fail:
	/*if(init_event)*/ udefrag_unload();
	return (-1);
}

/****f* udefrag.common/udefrag_unload
* NAME
*    udefrag_unload
* SYNOPSIS
*    error = udefrag_unload();
* FUNCTION
*    Unloads the Ultra Defragmenter driver.
* INPUTS
*    Nothing.
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    udefrag_unload();
* NOTES
*    You must call this function before terminating
*    the calling process to free allocated resources.
* SEE ALSO
*    udefrag_init
******/
int __stdcall udefrag_unload(void)
{
	//CHECK_INIT_EVENT();
	if(!init_event) return 0;

	/* close events */
	winx_destroy_event(init_event); init_event = NULL;
	/* close device handle */
	if(f_ud) winx_fclose(f_ud);
	if(f_map) winx_fclose(f_map);
	if(f_stat) winx_fclose(f_stat);
	if(f_stop) winx_fclose(f_stop);
	/* unload the driver */
	winx_unload_driver(L"ultradfg");
	return 0;
}

int udefrag_send_command(unsigned char command,unsigned char letter)
{
	char cmd[4];

	eh = winx_set_error_handler(DefragErrorHandler);
	cmd[0] = command; cmd[1] = letter; cmd[2] = 0;
	if(winx_fwrite(cmd,strlen(cmd),1,f_ud)){
		winx_set_error_handler(eh);
		return 0;
	}
	winx_set_error_handler(eh);
	winx_raise_error("E: Can't execute driver command \'%c\' for volume %c!",
		command,letter);
	return (-1);
}

/* you can send only one command at the same time */
DWORD WINAPI send_command(LPVOID unused)
{
	cmd_status = udefrag_send_command(c,lett);
	done_flag = TRUE;
	winx_exit_thread();
	return 0;
}

/****f* udefrag.common/udefrag_send_command_ex
* NAME
*    udefrag_send_command_ex
* SYNOPSIS
*    error = udefrag_send_command_ex(cmd, letter, callback);
* FUNCTION
*    Sends the 'Analyse | Defragment | Optimize' 
*    command to the driver.
* INPUTS
*    cmd      - command: 'a' for Analyse, 'd' for Defragmentation,
*               'c' for Optimization
*    letter   - volume letter
*    callback - address of the callback function;
*               see prototype in udefrag.h header;
*               this parameter may be NULL
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    if(udefrag_analyse('C',callback_proc) < 0){
*        printf("udefrag_analyse() call unsuccessful!");
*    }
* NOTES
*    Use udefrag_analyse, udefrag_defragment, udefrag_optimize
*    macro definitions instead.
******/
int __stdcall udefrag_send_command_ex(unsigned char command,unsigned char letter,STATUPDATEPROC sproc)
{
	WINX_FILE *f;
	char volume[] = "\\??\\A:";
	HANDLE hThread;

	CHECK_INIT_EVENT();

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
		if(f){ winx_fflush(f); winx_fclose(f); }
		winx_set_system_error_mode(1); /* equal to SetErrorMode(0) */
	}

	if(!sproc){
		/* send command directly and return */
		return udefrag_send_command(command,letter);
	}
	done_flag = FALSE;
	/* create a thread for driver command processing */
	cmd_status = 0;
	c = command; lett = letter;
	if(winx_create_thread(send_command,&hThread) < 0)
		return (-1);
	NtCloseSafe(hThread);
	/*
	* Call specified callback 
	* every (settings.refresh_interval) milliseconds.
	*/
	do {
		winx_sleep(refresh_interval);
		sproc(FALSE);
	} while(!done_flag);
	sproc(TRUE);

	return cmd_status;
}

/****f* udefrag.common/udefrag_stop
* NAME
*    udefrag_stop
* SYNOPSIS
*    error = udefrag_stop();
* FUNCTION
*    Stops the current operation.
* INPUTS
*    Nothing.
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    udefrag_stop();
* SEE ALSO
*    udefrag_send_command_ex
******/
int __stdcall udefrag_stop(void)
{
	CHECK_INIT_EVENT();
	if(winx_fwrite("s",1,1,f_stop)) return 0;
	winx_raise_error("E: Stop request failed!");
	return (-1);
}

/****f* udefrag.common/udefrag_get_progress
* NAME
*    udefrag_get_progress
* SYNOPSIS
*    error = udefrag_get_progress(pstat, percentage);
* FUNCTION
*    Retrieves the progress of current operation.
* INPUTS
*    pstat      - pointer to STATISTIC structure,
*                 defined in ultradfg.h header
*    percentage - pointer to variable of double type
*                 to store progress percentage
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    udefrag_get_progress(&stat,&p);
* SEE ALSO
*    udefrag_send_command_ex
******/
int __stdcall udefrag_get_progress(STATISTIC *pstat, double *percentage)
{
	double x, y;
	
	CHECK_INIT_EVENT();

	if(!winx_fread(pstat,sizeof(STATISTIC),1,f_stat)){
		winx_raise_error("E: Statistical data unavailable!");
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

/****f* udefrag.common/udefrag_get_map
* NAME
*    udefrag_get_map
* SYNOPSIS
*    error = udefrag_get_map(buffer, size);
* FUNCTION
*    Retrieves the cluster map.
* INPUTS
*    buffer - pointer to the map buffer
*    size   - size of the specified buffer
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    udefrag_get_map(map,sizeof(map));
* SEE ALSO
*    udefrag_send_command_ex
******/
int __stdcall udefrag_get_map(char *buffer,int size)
{
	CHECK_INIT_EVENT();
	if(winx_fread(buffer,size,1,f_map)) return 0;
	winx_raise_error("E: Cluster map unavailable!");
	return (-1);
}

/****f* udefrag.common/udefrag_get_default_formatted_results
* NAME
*    udefrag_get_default_formatted_results
* SYNOPSIS
*    msg = udefrag_get_default_formatted_results(pstat);
* FUNCTION
*    Retrieves default formatted results.
* INPUTS
*    pstat - pointer to STATISTIC structure,
*            defined in ultradfg.h header
* RESULT
*    msg - string containing default formatted result
*          of operation defined in pstat.
* EXAMPLE
*   udefrag_analyse('C',NULL);
*   udefrag_get_progress(&pstat,&p);
*   printf("%s\n", udefrag_get_default_formatted_results(&pstat));
* NOTES
*    Useful for native and console applications.
* SEE ALSO
*    udefrag_send_command_ex
******/
char * __stdcall udefrag_get_default_formatted_results(STATISTIC *pstat)
{
	char total_space[68];
	char free_space[68];
	double p;
	unsigned int ip;

	winx_fbsize(pstat->total_space,2,total_space,sizeof(total_space));
	winx_fbsize(pstat->free_space,2,free_space,sizeof(free_space));
	if(pstat->filecounter == 0) p = 0.00;
	else p = (double)(pstat->fragmcounter)/((double)(pstat->filecounter));
	ip = (unsigned int)(p * 100.00);
	if(ip < 100) ip = 100; /* fix round off error */
	_snprintf(result_msg,sizeof(result_msg) - 1,
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
