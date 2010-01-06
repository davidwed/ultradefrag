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
 * @file settings.c
 * @brief UltraDefrag settings code.
 * @addtogroup Settings
 * @{
 */

#include "../../include/ntndk.h"

#include "../../include/udefrag.h"
#include "../../include/ultradfg.h"
#include "../zenwinx/zenwinx.h"

#define DbgCheckInitEvent(f) { \
	if(!init_event){ \
		DebugPrint(f " call without initialization!"); \
		return (-1); \
	} \
}

/* global variables */
#define MAX_FILTER_SIZE 4096
short in_filter[MAX_FILTER_SIZE + 1] = L"";
short ex_filter[MAX_FILTER_SIZE + 1] = L"";
ULONGLONG sizelimit = 0;
ULONGLONG fraglimit = 0;
int refresh_interval  = DEFAULT_REFRESH_INTERVAL;

/*
* http://sourceforge.net/tracker/index.php?func=
* detail&aid=2886353&group_id=199532&atid=969873
*/
ULONGLONG time_limit = 0;

ULONG disable_reports = FALSE;
ULONG dbgprint_level = DBG_NORMAL;

short env_buffer[8192];
#define ENV_BUF_SIZE (sizeof(env_buffer) / sizeof(short))

extern HANDLE init_event;
extern WINX_FILE *f_ud;
extern BOOL kernel_mode_driver;

/**
 * @brief Queries an environment variable.
 * @note Internal use only.
 */
BOOL query_env_variable(short *name)
{
	if(winx_query_env_variable(name,env_buffer,ENV_BUF_SIZE) >= 0)
		return TRUE;
	return FALSE;
}

/**
 * @brief Retrieves all settings from the environment.
 * @note Internal use only.
 */
void __stdcall udefrag_load_settings(void)
{
	char buf[256];
	
	/* reset all parameters */
	in_filter[0] = ex_filter[0] = 0;
	sizelimit = fraglimit = 0;
	time_limit = 0;
	disable_reports = FALSE;
	dbgprint_level = DBG_NORMAL;

	if(query_env_variable(L"UD_IN_FILTER"))	wcsncpy(in_filter,env_buffer,MAX_FILTER_SIZE);
	if(query_env_variable(L"UD_EX_FILTER"))	wcsncpy(ex_filter,env_buffer,MAX_FILTER_SIZE);
	
	if(query_env_variable(L"UD_SIZELIMIT")){
		_snprintf(buf,sizeof(buf) - 1,"%ws",env_buffer);
		buf[sizeof(buf) - 1] = 0;
		winx_dfbsize(buf,&sizelimit);
	}

	if(query_env_variable(L"UD_FRAGMENTS_THRESHOLD")) fraglimit = _wtoi64(env_buffer);
	if(query_env_variable(L"UD_TIME_LIMIT")){
		_snprintf(buf,sizeof(buf) - 1,"%ws",env_buffer);
		buf[sizeof(buf) - 1] = 0;
		time_limit = winx_str2time(buf);
	}
	DebugPrint("Time limit = %I64u seconds\n",time_limit);

	if(query_env_variable(L"UD_REFRESH_INTERVAL")) refresh_interval = _wtoi(env_buffer);
	DebugPrint("Refresh interval = %u msec\n",refresh_interval);

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
}

/**
 * @brief Delivers all settings to the driver.
 * @return Zero for success, negative value otherwise.
 * @note Internal use only.
 */
int __stdcall udefrag_apply_settings(void)
{
	DbgCheckInitEvent("udefrag_apply_settings");

	/* set debug print level */
	if(winx_ioctl(f_ud,IOCTL_SET_DBGPRINT_LEVEL,"Debug print level setup",
		&dbgprint_level,sizeof(ULONG),NULL,0,NULL) < 0) return (-1);
	/* set sizelimit */
	if(winx_ioctl(f_ud,IOCTL_SET_SIZELIMIT,"Size limit setup",
		&sizelimit,sizeof(ULONGLONG),NULL,0,NULL) < 0) return (-1);
	/* set fraglimit */
	if(winx_ioctl(f_ud,IOCTL_SET_FRAGLIMIT,"Fragments threshold setup",
		&fraglimit,sizeof(ULONGLONG),NULL,0,NULL) < 0) return (-1);
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

/**
 * @brief Reloads all settings and delivers them to the driver.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall udefrag_reload_settings(void)
{
	udefrag_load_settings();
	if(!kernel_mode_driver) return 0;
	return udefrag_apply_settings();
}

/** @} */
