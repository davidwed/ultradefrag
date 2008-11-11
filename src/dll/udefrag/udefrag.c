/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
		winx_push_error("%s call without initialization!", __FUNCTION__); \
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
char error_message[ERR_MSG_SIZE];

/* functions */
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	/* here we have last chance to unload the driver */
	if(dwReason == DLL_PROCESS_DETACH)
		if(udefrag_unload() < 0) winx_pop_error(NULL,0);
	return 1;
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
*        char buffer[ERR_MSG_SIZE];
*
*        if(udefrag_init(0) < 0)
*            udefrag_pop_error(buffer,sizeof(buffer));
*            printf("udefrag_init() call unsuccessful!");
*            printf("\n\n%s\n",buffer);
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
	char buf[ERR_MSG_SIZE];

	/* 0. only one instance of the program ! */
	/* 1. Enable neccessary privileges */
	/*if(!EnablePrivilege(UserToken,SE_MANAGE_VOLUME_PRIVILEGE)) return (-1)*/
	if(winx_enable_privilege(SE_LOAD_DRIVER_PRIVILEGE) < 0) return (-1);
	/* create init_event - this must be after privileges enabling */
	if(winx_create_event(L"\\udefrag_init",SynchronizationEvent,&init_event) < 0){
		winx_save_error(buf,ERR_MSG_SIZE);
		if(strstr(buf,"c0000035")) /* STATUS_OBJECT_NAME_COLLISION */
			winx_push_error("You can run only one instance of UltraDefrag!");
		else winx_restore_error(buf);
		return (-1);
	}
	/* 2. Load the driver */
	if(winx_load_driver(L"ultradfg") < 0) return (-1);
	/* 3. Open our device */
	/* FIXME: detailed error information. "Can't access ULTRADFG driver: %x!" */
	f_ud = winx_fopen("\\Device\\UltraDefrag","w");
	if(!f_ud) goto init_fail;
	f_map = winx_fopen("\\Device\\UltraDefragMap","r");
	if(!f_map) goto init_fail;
	f_stat = winx_fopen("\\Device\\UltraDefragStat","r");
	if(!f_stat) goto init_fail;
	f_stop = winx_fopen("\\Device\\UltraDefragStop","w");
	if(!f_stop) goto init_fail;
	/* 5. Set user mode buffer - nt 4.0 specific */
	if(winx_ioctl(f_ud,IOCTL_SET_USER_MODE_BUFFER,"User mode buffer setup",
		user_mode_buffer,0,NULL,0,NULL) < 0) goto init_fail;
	/* 6. Set cluster map size */
	if(winx_ioctl(f_ud,IOCTL_SET_CLUSTER_MAP_SIZE,"Cluster map buffer setup",
		&map_size,sizeof(long),NULL,0,NULL) < 0) goto init_fail;
	/* 7. Load settings */
	if(udefrag_reload_settings() < 0) goto init_fail;
	return 0;
init_fail:
	if(init_event){
		/* save error message */
		winx_save_error(buf,ERR_MSG_SIZE);
		if(udefrag_unload() < 0) winx_pop_error(NULL,0);
		/* restore error message */
		winx_restore_error(buf);
	}
	return (-1);
}

/****f* udefrag.common/udefrag_unload
* NAME
*    udefrag_unload
* SYNOPSIS
*    error = udefrag_unload(save_opts);
* FUNCTION
*    Unloads the Ultra Defragmenter driver and saves the options.
* INPUTS
*    save_opts - true if options must be saved, false otherwise
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    if(udefrag_unload(TRUE) < 0)
*        udefrag_pop_error(buffer,sizeof(buffer));
*        printf("udefrag_unload() call unsuccessful!");
*        printf("\n\n%s\n",buffer);
*    }
* NOTES
*    You must call this function before terminating
*    the calling process to free allocated resources.
* SEE ALSO
*    udefrag_init
******/
int __stdcall udefrag_unload(void)
{
	CHECK_INIT_EVENT();

	/* close events */
	winx_destroy_event(init_event); init_event = NULL;
	/* close device handle */
	if(f_ud) winx_fclose(f_ud);
	if(f_map) winx_fclose(f_map);
	if(f_stat) winx_fclose(f_stat);
	if(f_stop) winx_fclose(f_stop);
	/* unload the driver */
	if(winx_unload_driver(L"ultradfg") < 0)
		winx_pop_error(NULL,0);
	return 0;
}

int udefrag_send_command(unsigned char command,unsigned char letter)
{
	char cmd[4];

	CHECK_INIT_EVENT();

	/* FIXME: detailed error message! 
	"Can't execute driver command \'%c\' for volume %c: %x!" */
	cmd[0] = command; cmd[1] = letter; cmd[2] = 0;
	return winx_fwrite(cmd,strlen(cmd),1,f_ud) ? 0 : (-1);
}

/* you can send only one command at the same time */
DWORD WINAPI send_command(LPVOID unused)
{
	if(udefrag_send_command(c,lett) < 0){
		/* save error message */
		winx_save_error(error_message,ERR_MSG_SIZE);
	}
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
*    if(udefrag_analyse('C',callback_proc) < 0)
*        udefrag_pop_error(buffer,sizeof(buffer));
*        printf("udefrag_analyse() call unsuccessful!");
*        printf("\n\n%s\n",buffer);
*    }
* NOTES
*    Use udefrag_analyse, udefrag_defragment, udefrag_optimize
*    macro definitions instead.
******/
int __stdcall udefrag_send_command_ex(unsigned char command,unsigned char letter,STATUPDATEPROC sproc)
{
	if(!sproc){
		/* send command directly and return */
		return udefrag_send_command(command,letter);
	}
	done_flag = FALSE;
	error_message[0] = 0;
	/* create a thread for driver command processing */
	c = command; lett = letter;
	if(winx_create_thread(send_command,NULL) < 0)
		return (-1);
	/*
	* Call specified callback 
	* every (settings.refresh_interval) milliseconds.
	*/
	do {
		winx_sleep(refresh_interval);
		sproc(FALSE);
	} while(!done_flag);
	sproc(TRUE);
	/*
	* If an error occured during the send_command() execution 
	* then return -1 and an error description.
	*/
	if(error_message[0]){
		winx_restore_error(error_message);
		return (-1);
	}
	return 0;
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
*    if(udefrag_stop() < 0)
*        udefrag_pop_error(buffer,sizeof(buffer));
*        printf("udefrag_stop() call unsuccessful!");
*        printf("\n\n%s\n",buffer);
*    }
* SEE ALSO
*    udefrag_analyse, udefrag_defragment, udefrag_optimize
******/
int __stdcall udefrag_stop(void)
{
	CHECK_INIT_EVENT();
	/* FIXME: better error messages "Can't stop driver command: %x!" */
	return winx_fwrite("s",1,1,f_stop) ? 0 : (-1);
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
*    if(udefrag_get_progress(&stat,&p) < 0)
*        udefrag_pop_error(buffer,sizeof(buffer));
*        printf("udefrag_get_progress() call unsuccessful!");
*        printf("\n\n%s\n",buffer);
*    }
* SEE ALSO
*    udefrag_analyse, udefrag_defragment, udefrag_optimize
******/
int __stdcall udefrag_get_progress(STATISTIC *pstat, double *percentage)
{
	CHECK_INIT_EVENT();

	/* FIXME: detailed error message! "Statistical data unavailable: %x!" */
	if(!winx_fread(pstat,sizeof(STATISTIC),1,f_stat)) return (-1);

	if(percentage){ /* calculate percentage only if we have such request */
		switch(pstat->current_operation){
		case 'A':
			*percentage = (double)(LONGLONG)pstat->processed_clusters *
					(double)(LONGLONG)pstat->bytes_per_cluster / 
					((double)(LONGLONG)pstat->total_space + 0.1);
			break;
		case 'D':
		case 'C':
			if(pstat->clusters_to_move_initial == 0)
				*percentage = 1.00;
			else
				*percentage = 1.00 - (double)(LONGLONG)pstat->clusters_to_move / 
						((double)(LONGLONG)pstat->clusters_to_move_initial + 0.1);
			break;
		}
		*percentage = (*percentage) * 100.00;
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
*    if(udefrag_get_map(map,sizeof(map)) < 0)
*        udefrag_pop_error(buffer,sizeof(buffer));
*        printf("udefrag_get_map() call unsuccessful!");
*        printf("\n\n%s\n",buffer);
*    }
* SEE ALSO
*    udefrag_analyse, udefrag_defragment, udefrag_optimize
******/
int __stdcall udefrag_get_map(char *buffer,int size)
{
	CHECK_INIT_EVENT();
	/* FIXME: detailed error message! "Cluster map unavailable: %x!" */
	return winx_fread(buffer,size,1,f_map) ? 0 : (-1);
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
*    udefrag_analyse, udefrag_defragment, udefrag_optimize
******/
char * __stdcall udefrag_get_default_formatted_results(STATISTIC *pstat)
{
	char total_space[68];
	char free_space[68];
	double p;
	unsigned int ip;

	winx_fbsize(pstat->total_space,2,total_space,sizeof(total_space));
	winx_fbsize(pstat->free_space,2,free_space,sizeof(free_space));
	p = (double)(pstat->fragmcounter)/((double)(pstat->filecounter) + 0.1);
	ip = (unsigned int)(p * 100.00);
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
	return result_msg;
}
