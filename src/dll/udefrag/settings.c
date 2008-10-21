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

ud_options settings = \
{ \
	0,             /* sizelimit */
	500,           /* update_interval */
	HTML_REPORT,   /* report_type */
	DBG_NORMAL,    /* dbgprint_level */
};

extern BOOL n_ioctl(HANDLE handle,ULONG code,
			PVOID in_buf,ULONG in_size,
			PVOID out_buf,ULONG out_size,
			char *err_format_string);

/* load settings: always successful */
int __stdcall udefrag_load_settings()
{
	/* reset all parameters */
	wcscpy(in_filter,L"");
	wcscpy(ex_filter,L"");
	settings.sizelimit = 0;
	settings.report_type = HTML_REPORT;
	settings.dbgprint_level = DBG_NORMAL;

	/*
	* Since 2.0.0 version all options will be received 
	* from the Environment.
	*/

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
		&settings.dbgprint_level,sizeof(DWORD),NULL,0,
		"Can't set debug print level: %x!")) goto apply_settings_fail;
	/* set sizelimit */
	if(!n_ioctl(udefrag_device_handle,IOCTL_SET_SIZELIMIT,
		&settings.sizelimit,sizeof(ULONGLONG),NULL,0,
		"Can't set size limit: %x!")) goto apply_settings_fail;
	/* set report characterisics */
	rt.type = settings.report_type;
	if(!n_ioctl(udefrag_device_handle,IOCTL_SET_REPORT_TYPE,
		&rt,sizeof(REPORT_TYPE),NULL,0,
		"Can't set report type: %x!")) goto apply_settings_fail;
	/* set filters */
	if(!n_ioctl(udefrag_device_handle,IOCTL_SET_INCLUDE_FILTER,
		/*settings.*/in_filter,(wcslen(/*settings.*/in_filter) + 1) << 1,NULL,0,
		"Can't set include filter: %x!")) goto apply_settings_fail;
	if(!n_ioctl(udefrag_device_handle,IOCTL_SET_EXCLUDE_FILTER,
		/*settings.*/ex_filter,(wcslen(/*settings.*/ex_filter) + 1) << 1,NULL,0,
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
