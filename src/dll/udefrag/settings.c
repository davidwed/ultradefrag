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
		winx_push_error("%s call without initialization!", __FUNCTION__); \
		return -1; \
	} \
}

/* global variables */
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

#define ENV_BUF_SIZE (sizeof(env_buffer) / sizeof(short))

BOOL query_env_variable(short *name)
{
	if(winx_query_env_variable(name,env_buffer,ENV_BUF_SIZE) >= 0)
		return TRUE;
	winx_pop_error(NULL,0);
	return FALSE;
}

/* load settings: always successful */
int __stdcall udefrag_load_settings()
{
	char buf[64];
	
	/* reset all parameters */
	in_filter[0] = ex_filter[0] = 0;
	sizelimit = 0;
	report_type = HTML_REPORT;
	dbgprint_level = DBG_NORMAL;

	/*
	* Since 2.0.0 version all options will be received 
	* from the Environment.
	*/
	if(query_env_variable(L"UD_IN_FILTER"))	wcsncpy(in_filter,env_buffer,MAX_FILTER_SIZE);
	if(query_env_variable(L"UD_EX_FILTER"))	wcsncpy(ex_filter,env_buffer,MAX_FILTER_SIZE);
	
	if(query_env_variable(L"UD_SIZELIMIT")){
		_snprintf(buf,sizeof(buf) - 1,"%ws",env_buffer);
		winx_dfbsize(buf,&sizelimit);
	}

	if(query_env_variable(L"UD_REFRESH_INTERVAL")) refresh_interval = _wtoi(env_buffer);

	if(query_env_variable(L"UD_DISABLE_REPORTS")) {
		if(!wcscmp(env_buffer,L"1")) report_type = NO_REPORT;
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
	REPORT_TYPE rt;

	CHECK_INIT_EVENT();

	/* set debug print level */
	if(winx_ioctl(f_ud,IOCTL_SET_DBGPRINT_LEVEL,"Debug print level setup",
		&dbgprint_level,sizeof(DWORD),NULL,0,NULL) < 0) return (-1);
	/* set sizelimit */
	if(winx_ioctl(f_ud,IOCTL_SET_SIZELIMIT,"Size limit setup",
		&sizelimit,sizeof(ULONGLONG),NULL,0,NULL) < 0) return (-1);
	/* set report characterisics */
	rt.type = report_type;
	if(winx_ioctl(f_ud,IOCTL_SET_REPORT_TYPE,"Report type setup",
		&rt,sizeof(REPORT_TYPE),NULL,0,NULL) < 0) return (-1);
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
*    if(udefrag_reload_settings() < 0){
*        udefrag_pop_error(buffer,sizeof(buffer));
*        // handle error
*    }
******/
int __stdcall udefrag_reload_settings()
{
	udefrag_load_settings();
	return udefrag_apply_settings();
}
