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
#include "../zenwinx/src/zenwinx.h"

/* global variables */
extern int native_mode_flag;
extern HANDLE udefrag_device_handle;
extern HANDLE init_event;
extern HANDLE io_event;

ud_options settings = \
{ \
	L"",          /* in_filter */
	L"system volume information;temp;recycler", /* ex_filter */
	L"windows;winnt;ntuser;pagefile;hiberfil",  /* boot_in_filter */
	L"temp",       /* boot_ex_filter */
	0,             /* sizelimit */
	TRUE,          /* skip_removable */
	500,           /* update_interval */
	TRUE,          /* show_progress */
	HTML_REPORT,   /* report_type */
	ASCII_FORMAT,  /* report_format */
	DBG_NORMAL,    /* dbgprint_level */
	L"",           /* sched_letters */
	FALSE,         /* every_boot */
	FALSE,         /* next_boot */
	FALSE,         /* only_reg_and_pagefile */
	0,             /* x */
	0              /* y */
};

short ud_key[] = \
	L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\UltraDefrag";

#define MAX_FILTER_SIZE 4096
#define MAX_FILTER_BYTESIZE (MAX_FILTER_SIZE * sizeof(short))
short in_filter[MAX_FILTER_SIZE + 1];
short ex_filter[MAX_FILTER_SIZE + 1];
short boot_in_filter[MAX_FILTER_SIZE + 1];
short boot_ex_filter[MAX_FILTER_SIZE + 1];

#define MAX_SCHED_LETTERS 64
short sched_letters[MAX_SCHED_LETTERS + 1];

static __inline int ReadRegValue(HANDLE h, short *name, DWORD type, void *pval, DWORD size)
{
	DWORD dwSize = size;
	if(winx_reg_query_value(h,name,type,pval,&dwSize) < 0){
		winx_pop_error(NULL,0);
		return FALSE;
	}
	return TRUE;
}

static __inline int WriteRegValue(HANDLE h, short *name, DWORD type, void *pval, DWORD size)
{
	if(winx_reg_set_value(h,name,type,pval,size) < 0){
		winx_pop_error(NULL,0);
		return FALSE;
	}
	return TRUE;
}

#define ReadRegDWORD(h,name,pval) ReadRegValue(h,name,REG_DWORD,pval,sizeof(DWORD))
#define WriteRegDWORD(h,name,val) WriteRegValue(h,name,REG_DWORD,&(val),sizeof(DWORD))

extern BOOL n_ioctl(HANDLE handle,HANDLE event,ULONG code,
			PVOID in_buf,ULONG in_size,
			PVOID out_buf,ULONG out_size,
			char *err_format_string);

/* load settings: always successful */
int __stdcall udefrag_load_settings(int argc, short **argv)
{
	HANDLE hKey;
	DWORD x;
	int i;

	/* open program key */
	if(winx_reg_open_key(ud_key,&hKey) < 0){
		winx_pop_error(NULL,0);
		goto analyse_cmdline;
	}
	/* read dword parameters */
	ReadRegDWORD(hKey,L"skip removable",&settings.skip_removable);
	ReadRegDWORD(hKey,L"update interval",(DWORD *)&settings.update_interval);
	ReadRegDWORD(hKey,L"show progress",(DWORD *)&settings.show_progress);
	ReadRegDWORD(hKey,L"dbgprint level",&settings.dbgprint_level);
	ReadRegDWORD(hKey,L"every boot",&settings.every_boot);
	ReadRegDWORD(hKey,L"next boot",&settings.next_boot);
	ReadRegDWORD(hKey,L"only registry and pagefile",&settings.only_reg_and_pagefile);

	if(ReadRegDWORD(hKey,L"report type",&x))
		settings.report_type = (UCHAR)x;

	if(ReadRegDWORD(hKey,L"report format",&x))
		settings.report_format = (UCHAR)x;

	ReadRegDWORD(hKey,L"x",&settings.x);
	ReadRegDWORD(hKey,L"y",&settings.y);
	/* read binary parameters */
	ReadRegValue(hKey,L"sizelimit",REG_BINARY,&settings.sizelimit,sizeof(ULONGLONG));
	/* read strings */
	/* if success then write terminating zero and set ud_settings structure field */
	if(ReadRegValue(hKey,L"include filter",REG_SZ,in_filter,MAX_FILTER_BYTESIZE)){
		in_filter[MAX_FILTER_SIZE] = 0; settings.in_filter = in_filter;
	}
	if(ReadRegValue(hKey,L"exclude filter",REG_SZ,ex_filter,MAX_FILTER_BYTESIZE)){
		ex_filter[MAX_FILTER_SIZE] = 0; settings.ex_filter = ex_filter;
	}
	if(ReadRegValue(hKey,L"boot time include filter",REG_SZ,boot_in_filter,MAX_FILTER_BYTESIZE)){
		boot_in_filter[MAX_FILTER_SIZE] = 0; settings.boot_in_filter = boot_in_filter;
	}
	if(ReadRegValue(hKey,L"boot time exclude filter",REG_SZ,boot_ex_filter,MAX_FILTER_BYTESIZE)){
		boot_ex_filter[MAX_FILTER_SIZE] = 0; settings.boot_ex_filter = boot_ex_filter;
	}
	if(ReadRegValue(hKey,L"scheduled letters",REG_SZ,sched_letters,MAX_SCHED_LETTERS * sizeof(short))){
		sched_letters[MAX_SCHED_LETTERS] = 0; settings.sched_letters = sched_letters;
	}
	winx_reg_close_key(hKey);
analyse_cmdline:
	/* overwrite parameters from the command line */
	if(!argv) goto no_cmdline;
	for(i = 1; i < argc; i++){
		if(argv[i][0] != '-' && argv[i][0] != '/')
			continue;
		switch(argv[i][1]){
		case 's':
		case 'S':
			settings.sizelimit = _wtoi64(argv[i] + 2);
			break;
		case 'i':
		case 'I':
			if(wcslen(argv[i] + 2) <= MAX_FILTER_SIZE)
				wcscpy(settings.in_filter,argv[i] + 2);
			break;
		case 'e':
		case 'E':
			if(wcslen(argv[i] + 2) <= MAX_FILTER_SIZE)
				wcscpy(settings.ex_filter,argv[i] + 2);
			break;
		case 'd':
		case 'D':
			settings.dbgprint_level = _wtoi(argv[i] + 2);
			break;
		}
	}
no_cmdline:
	return 0;
}

/****f* udefrag.settings/udefrag_get_options
* NAME
*    udefrag_get_options
* SYNOPSIS
*    opts = udefrag_get_options();
* FUNCTION
*    Retrieves ud_options structure address
*    containing the defragmenter options.
* INPUTS
*    Nothing.
* RESULT
*    ud_options structure address.
* SEE ALSO
*    udefrag_set_options
******/
ud_options * __stdcall udefrag_get_options(void)
{
	return &settings;
}

/****f* udefrag.settings/udefrag_set_options
* NAME
*    udefrag_set_options
* SYNOPSIS
*    error = udefrag_set_options(opts);
* FUNCTION
*    Sets the defragmenter options.
* INPUTS
*    opts - ud_options structure address
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    if(udefrag_set_options(opts) < 0){
*        udefrag_pop_error(buffer,sizeof(buffer));
*        // handle error
*    }
* SEE ALSO
*    udefrag_get_options
******/
int __stdcall udefrag_set_options(ud_options *ud_opts)
{
	REPORT_TYPE rt;

	if(!init_event){
		winx_push_error("Udefrag.dll apply_settings call without initialization!");
		goto apply_settings_fail;
	}
	if(ud_opts != &settings)
		memcpy(&settings,ud_opts,sizeof(ud_options));
	/* set debug print level */
	if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_DBGPRINT_LEVEL,
		&settings.dbgprint_level,sizeof(DWORD),NULL,0,
		"Can't set debug print level: %x!")) goto apply_settings_fail;
	/* set report characterisics */
	rt.format = settings.report_format;
	rt.type = settings.report_type;
	if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_REPORT_TYPE,
		&rt,sizeof(REPORT_TYPE),NULL,0,
		"Can't set report type: %x!")) goto apply_settings_fail;
	/* set filters */
	if(native_mode_flag){
		if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_INCLUDE_FILTER,
			settings.boot_in_filter,(wcslen(settings.boot_in_filter) + 1) << 1,NULL,0,
			"Can't set include filter: %x!")) goto apply_settings_fail;
		if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_EXCLUDE_FILTER,
			settings.boot_ex_filter,(wcslen(settings.boot_ex_filter) + 1) << 1,NULL,0,
			"Can't set exclude filter: %x!")) goto apply_settings_fail;
	} else {
		if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_INCLUDE_FILTER,
			settings.in_filter,(wcslen(settings.in_filter) + 1) << 1,NULL,0,
			"Can't set include filter: %x!")) goto apply_settings_fail;
		if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_EXCLUDE_FILTER,
			settings.ex_filter,(wcslen(settings.ex_filter) + 1) << 1,NULL,0,
			"Can't set exclude filter: %x!")) goto apply_settings_fail;
	}
	return 0;
apply_settings_fail:
	return (-1);
}

int __stdcall udefrag_save_settings(void)
{
	HANDLE hKey;
	DWORD x;

	/* native app should clean registry */
	if(native_mode_flag)
		settings.next_boot = FALSE;
	/* create key if not exist */
	if(winx_reg_open_key(ud_key,&hKey) < 0){
		winx_pop_error(NULL,0);
		if(winx_reg_create_key(ud_key,&hKey) < 0) goto save_fail;
	}
	/* set dword values */
	WriteRegDWORD(hKey,L"skip removable",settings.skip_removable);
	WriteRegDWORD(hKey,L"update interval",settings.update_interval);
	WriteRegDWORD(hKey,L"show progress",settings.show_progress);
	WriteRegDWORD(hKey,L"dbgprint level",settings.dbgprint_level);
	WriteRegDWORD(hKey,L"every boot",settings.every_boot);
	WriteRegDWORD(hKey,L"next boot",settings.next_boot);
	WriteRegDWORD(hKey,L"only registry and pagefile",settings.only_reg_and_pagefile);
	x = (DWORD)settings.report_type;
	WriteRegDWORD(hKey,L"report type",x);
	x = (DWORD)settings.report_format;
	WriteRegDWORD(hKey,L"report format",x);
	WriteRegDWORD(hKey,L"x",settings.x);
	WriteRegDWORD(hKey,L"y",settings.y);
	/* set binary data */
	WriteRegValue(hKey,L"sizelimit",REG_BINARY,&settings.sizelimit,sizeof(ULONGLONG));
	/* write strings */
	WriteRegValue(hKey,L"include filter",REG_SZ,settings.in_filter,
		(wcslen(settings.in_filter) + 1) << 1);
	WriteRegValue(hKey,L"exclude filter",REG_SZ,settings.ex_filter,
		(wcslen(settings.ex_filter) + 1) << 1);
	WriteRegValue(hKey,L"boot time include filter",REG_SZ,settings.boot_in_filter,
		(wcslen(settings.boot_in_filter) + 1) << 1);
	WriteRegValue(hKey,L"boot time exclude filter",REG_SZ,settings.boot_ex_filter,
		(wcslen(settings.boot_ex_filter) + 1) << 1);
	WriteRegValue(hKey,L"scheduled letters",REG_SZ,settings.sched_letters,
		(wcslen(settings.sched_letters) + 1) << 1);
	winx_reg_close_key(hKey);
	/* native app should clean registry */
	if(native_mode_flag && !settings.every_boot)
		if(winx_reg_remove_from_boot_execute(L"defrag_native") < 0) goto save_fail;
	if(!native_mode_flag){
		if(settings.next_boot || settings.every_boot){
			if(winx_reg_add_to_boot_execute(L"defrag_native") < 0) goto save_fail;
		} else {
			if(winx_reg_remove_from_boot_execute(L"defrag_native") < 0) goto save_fail;
		}
	}
	return 0;
save_fail:
	return (-1);
}

/* important registry cleanup for uninstaller */
int __stdcall udefrag_clean_registry(void)
{
	return winx_reg_remove_from_boot_execute(L"defrag_native");
}

/* registry cleanup for native executable */
int __stdcall udefrag_native_clean_registry(void)
{
	HANDLE hKey;

	udefrag_load_settings(0,NULL);
	if(settings.next_boot){
		settings.next_boot = 0;
		/* set next boot parameter to zero */
		if(winx_reg_open_key(ud_key,&hKey) >= 0){
			WriteRegDWORD(hKey,L"next boot",settings.next_boot);
			winx_reg_close_key(hKey);
		} else {
			winx_pop_error(NULL,0);
		}
	}
	if(settings.every_boot) return 0;
	return winx_reg_remove_from_boot_execute(L"defrag_native");
}
