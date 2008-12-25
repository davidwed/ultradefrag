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
* functions for program settings manipulations.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../include/ntndk.h"
#include "../../include/udefrag.h"
#include "../../include/ultradfg.h"
#include "../zenwinx/zenwinx.h"

#ifndef __FUNCTION__
#define __FUNCTION__ "udefrag_xxx"
#endif
#define CHECK_INIT_EVENT() { \
	if(!init_event){ \
		winx_raise_error("E: %s call without initialization!", __FUNCTION__); \
		return -1; \
	} \
}

#define LOG_CACHE_SIZE 30 /* 1/2 of maximum number of files in cache */

/* global variables */
extern HANDLE init_event;
extern WINX_FILE *f_ud;

#define MAX_FILTER_SIZE 4096
#define MAX_FILTER_BYTESIZE (MAX_FILTER_SIZE * sizeof(short))
short in_filter[MAX_FILTER_SIZE + 1] = L"";
short ex_filter[MAX_FILTER_SIZE + 1] = L"";
ULONGLONG sizelimit  = 0;
int refresh_interval  = 500;
ULONG disable_reports = FALSE;
ULONG dbgprint_level = DBG_NORMAL;

/* also is used in udefrag_set_dbg_cache() function */
short env_buffer[8192];

#define ENV_BUF_SIZE (sizeof(env_buffer) / sizeof(short))

BOOL query_env_variable(short *name)
{
	if(winx_query_env_variable(name,env_buffer,ENV_BUF_SIZE) >= 0)
		return TRUE;
	return FALSE;
}

/* load settings: always successful */
int __stdcall udefrag_load_settings()
{
	char buf[64];
	
	/* reset all parameters */
	in_filter[0] = ex_filter[0] = 0;
	sizelimit = 0;
	disable_reports = FALSE;
	dbgprint_level = DBG_NORMAL;

	/*
	* Since 2.0.0 version all options will be received 
	* from the Environment.
	*/
	if(query_env_variable(L"UD_IN_FILTER"))	wcsncpy(in_filter,env_buffer,MAX_FILTER_SIZE);
	if(query_env_variable(L"UD_EX_FILTER"))	wcsncpy(ex_filter,env_buffer,MAX_FILTER_SIZE);
	
	if(query_env_variable(L"UD_SIZELIMIT")){
		_snprintf(buf,sizeof(buf) - 1,"%ws",env_buffer);
		buf[sizeof(buf) - 1] = 0;
		winx_dfbsize(buf,&sizelimit);
	}

	if(query_env_variable(L"UD_REFRESH_INTERVAL")) refresh_interval = _wtoi(env_buffer);

	if(query_env_variable(L"UD_DISABLE_REPORTS")) {
		if(!wcscmp(env_buffer,L"1")) disable_reports = TRUE;
	}

	if(query_env_variable(L"UD_DBGPRINT_LEVEL")){
		_wcsupr(env_buffer);
		if(!wcscmp(env_buffer,L"DETAILED"))
			dbgprint_level = DBG_DETAILED;
		else if(!wcscmp(env_buffer,L"PARANOID"))
			dbgprint_level = DBG_PARANOID;
		else if(!wcscmp(env_buffer,L"NORMAL"))
			dbgprint_level = DBG_NORMAL;
	}
	return 0;
}

int __stdcall udefrag_apply_settings()
{
	CHECK_INIT_EVENT();

	/* set debug print level */
	if(winx_ioctl(f_ud,IOCTL_SET_DBGPRINT_LEVEL,"Debug print level setup",
		&dbgprint_level,sizeof(ULONG),NULL,0,NULL) < 0) return (-1);
	/* set sizelimit */
	if(winx_ioctl(f_ud,IOCTL_SET_SIZELIMIT,"Size limit setup",
		&sizelimit,sizeof(ULONGLONG),NULL,0,NULL) < 0) return (-1);
	/* set report state */
	if(winx_ioctl(f_ud,IOCTL_SET_REPORT_STATE,"Report state setup",
		&disable_reports,sizeof(ULONG),NULL,0,NULL) < 0) return (-1);
	/* set filters */
	if(winx_ioctl(f_ud,IOCTL_SET_INCLUDE_FILTER,"Include filter setup",
		in_filter,(wcslen(in_filter) + 1) << 1,NULL,0,NULL) < 0) return (-1);
	if(winx_ioctl(f_ud,IOCTL_SET_EXCLUDE_FILTER,"Exclude filter setup",
		ex_filter,(wcslen(ex_filter) + 1) << 1,NULL,0,NULL) < 0) return (-1);
	return 0;
}

/****f* udefrag.settings/udefrag_reload_settings
* NAME
*    udefrag_reload_settings
* SYNOPSIS
*    error = udefrag_reload_settings();
* FUNCTION
*    Reloads settings and applies them.
* INPUTS
*    Nothing.
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    udefrag_reload_settings();
******/
int __stdcall udefrag_reload_settings()
{
	udefrag_load_settings();
	return udefrag_apply_settings();
}

/* internal function */
int __stdcall udefrag_set_dbg_cache(void)
{
	char buffer[MAX_PATH];
	char log_path[MAX_PATH];
	char cache_path[MAX_PATH];
	char cache_dir[MAX_PATH];
	WINX_FILE *f;
	int log_number,i,j;
	
	/* get windows directory */
	if(winx_get_windows_directory(buffer,MAX_PATH) < 0) return (-1);
	/* read the previous log number from the cache */
	_snprintf(cache_path,MAX_PATH,"%s\\UltraDefrag\\logs\\cache",buffer);
	cache_path[MAX_PATH - 1] = 0;
	f = winx_fopen(cache_path,"r");
	if(!f){ log_number = 0; goto save_log_number; }
	if(!winx_fread(&log_number,sizeof(log_number),1,f)){
		log_number = 0; winx_fclose(f); goto save_log_number;
	}
	winx_fclose(f);
	/* increment the log number */
	log_number ++;
	/* save new log number */
save_log_number:
	_snprintf(cache_dir,MAX_PATH,"%s\\UltraDefrag\\logs",buffer);
	cache_dir[MAX_PATH - 1] = 0;
	if(winx_create_directory(cache_dir) < 0) return (-1);
	f = winx_fopen(cache_path,"w");
	if(!f){
		winx_raise_error("E: Cannot open %s file for write access!",cache_path);
		return (-1);
	}
	if(!winx_fwrite(&log_number,sizeof(log_number),1,f)){
		winx_fclose(f);
		winx_raise_error("E: Cannot save data into %s file!",cache_path);
		return (-1);
	}
	winx_fclose(f);
	/* produce log path */
	_snwprintf(env_buffer,MAX_PATH,L"%S\\UltraDefrag\\logs\\%06u.log",buffer,log_number);
	env_buffer[MAX_PATH - 1] = 0;
	/* set log path */
	if(winx_ioctl(f_ud,IOCTL_SET_LOG_PATH,"Log path setup",
		env_buffer,(wcslen(env_buffer) + 1) << 1,NULL,0,NULL) < 0) return (-1);
	/* clear logs cache - delete old log files */
	if(log_number % LOG_CACHE_SIZE == 0 && log_number > 0){
		/* delete the first half of current cache */
		for(j = 0; j < (LOG_CACHE_SIZE / 2 + 1); j++){
			i = log_number - LOG_CACHE_SIZE + j;
			_snprintf(log_path,MAX_PATH,"%s\\UltraDefrag\\logs\\%06u.log",buffer,i);
			log_path[MAX_PATH - 1] = 0;
			winx_delete_file(log_path);
		}
		/* delete the second half of previous cache */
		if(log_number >= LOG_CACHE_SIZE * 2){
			for(j = 0; j < (LOG_CACHE_SIZE / 2); j++){
				i = log_number - LOG_CACHE_SIZE - LOG_CACHE_SIZE / 2 + j;
				_snprintf(log_path,MAX_PATH,"%s\\UltraDefrag\\logs\\%06u.log",buffer,i);
				log_path[MAX_PATH - 1] = 0;
				winx_delete_file(log_path);
			}
		}
	}
	return 0;
}
