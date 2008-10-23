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

/* global variables */
extern HANDLE udefrag_device_handle;
extern HANDLE init_event;
extern WINX_FILE *f_ud;

#define MAX_FILTER_SIZE 4096
#define MAX_FILTER_BYTESIZE (MAX_FILTER_SIZE * sizeof(short))
short in_filter[MAX_FILTER_SIZE + 1] = L"";
short ex_filter[MAX_FILTER_SIZE + 1] = L"";
ULONGLONG sizelimit  = 0;
int refresh_interval  = 500;
UCHAR report_type    = HTML_REPORT;
DWORD dbgprint_level = DBG_NORMAL;

short env_buffer[8192];
char  bf[8192];

#define ENV_BUF_SIZE (sizeof(env_buffer) / sizeof(short))

extern BOOL n_ioctl(HANDLE handle,ULONG code,
			PVOID in_buf,ULONG in_size,
			PVOID out_buf,ULONG out_size,
			char *err_format_string);

/* load settings: always successful */
int __stdcall udefrag_load_settings()
{
	UNICODE_STRING us;
	ANSI_STRING as;
	
	/* reset all parameters */
	wcscpy(in_filter,L"");
	wcscpy(ex_filter,L"");
	sizelimit = 0;
	report_type = HTML_REPORT;
	dbgprint_level = DBG_NORMAL;

	/*
	* Since 2.0.0 version all options will be received 
	* from the Environment.
	*/
	if(winx_query_env_variable(L"UD_IN_FILTER",env_buffer,ENV_BUF_SIZE) >= 0)
		wcsncpy(in_filter,env_buffer,MAX_FILTER_SIZE);
	else
		winx_pop_error(NULL,0);

	if(winx_query_env_variable(L"UD_EX_FILTER",env_buffer,ENV_BUF_SIZE) >= 0)
		wcsncpy(ex_filter,env_buffer,MAX_FILTER_SIZE);
	else
		winx_pop_error(NULL,0);
	
	if(winx_query_env_variable(L"UD_SIZELIMIT",env_buffer,ENV_BUF_SIZE) >= 0){
		RtlInitUnicodeString(&us,env_buffer);
		if(RtlUnicodeStringToAnsiString(&as,&us,TRUE) == STATUS_SUCCESS){
			dfbsize2(as.Buffer,&sizelimit);
			RtlFreeAnsiString(&as);
		}
	} else {
		winx_pop_error(NULL,0);
	}

	if(winx_query_env_variable(L"UD_REFRESH_INTERVAL",env_buffer,ENV_BUF_SIZE) >= 0)
		refresh_interval = _wtoi(env_buffer);
	else
		winx_pop_error(NULL,0);

	if(winx_query_env_variable(L"UD_DISABLE_REPORTS",env_buffer,ENV_BUF_SIZE) >= 0) {
		if(!wcscmp(env_buffer,L"1")) report_type = NO_REPORT;
	} else {
		winx_pop_error(NULL,0);
	}

	if(winx_query_env_variable(L"UD_DBGPRINT_LEVEL",env_buffer,ENV_BUF_SIZE) >= 0){
		_wcsupr(env_buffer);
		if(!wcscmp(env_buffer,L"DETAILED"))
			dbgprint_level = DBG_DETAILED;
		else if(!wcscmp(env_buffer,L"PARANOID"))
			dbgprint_level = DBG_PARANOID;
		else if(!wcscmp(env_buffer,L"NORMAL"))
			dbgprint_level = DBG_NORMAL;
	} else {
		winx_pop_error(NULL,0);
	}
	return 0;
}

int __stdcall udefrag_set_options()
{
	REPORT_TYPE rt;

	if(!init_event){
		winx_push_error("Udefrag.dll apply_settings call without initialization!");
		goto apply_settings_fail;
	}

	/* set debug print level */
	if(!n_ioctl(udefrag_device_handle,IOCTL_SET_DBGPRINT_LEVEL,
		&dbgprint_level,sizeof(DWORD),NULL,0,
		"Can't set debug print level: %x!")) goto apply_settings_fail;
	/* set sizelimit */
	if(!n_ioctl(udefrag_device_handle,IOCTL_SET_SIZELIMIT,
		&sizelimit,sizeof(ULONGLONG),NULL,0,
		"Can't set size limit: %x!")) goto apply_settings_fail;
	/* set report characterisics */
	rt.type = report_type;
	if(!n_ioctl(udefrag_device_handle,IOCTL_SET_REPORT_TYPE,
		&rt,sizeof(REPORT_TYPE),NULL,0,
		"Can't set report type: %x!")) goto apply_settings_fail;
	/* set filters */
	if(!n_ioctl(udefrag_device_handle,IOCTL_SET_INCLUDE_FILTER,
		in_filter,(wcslen(in_filter) + 1) << 1,NULL,0,
		"Can't set include filter: %x!")) goto apply_settings_fail;
	if(!n_ioctl(udefrag_device_handle,IOCTL_SET_EXCLUDE_FILTER,
		ex_filter,(wcslen(ex_filter) + 1) << 1,NULL,0,
		"Can't set exclude filter: %x!")) goto apply_settings_fail;
	return 0;

apply_settings_fail:
	return (-1);
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
*    if(udefrag_reload_settings() < 0){
*        udefrag_pop_error(buffer,sizeof(buffer));
*        // handle error
*    }
******/
int __stdcall udefrag_reload_settings()
{
	udefrag_load_settings();
	return udefrag_set_options();
}
