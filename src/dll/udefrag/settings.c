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
	500,           /* update_interval */
	HTML_REPORT,   /* report_type */
	DBG_NORMAL,    /* dbgprint_level */
	L"",           /* sched_letters */
	FALSE,         /* every_boot */
	FALSE,         /* next_boot */
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

short file_buffer[32768];
short line_buffer[8192];
short param_buffer[8192];
short value_buffer[8192];

int getopts(char *filename);

char configfile[MAX_PATH] = "";

extern BOOL n_ioctl(HANDLE handle,HANDLE event,ULONG code,
			PVOID in_buf,ULONG in_size,
			PVOID out_buf,ULONG out_size,
			char *err_format_string);

int get_configfile_location(void)
{
//	HANDLE hKey;
//	int status;
	NTSTATUS Status;
	UNICODE_STRING name, us;
	ANSI_STRING as;
	short buf[MAX_PATH + 1];
//	DWORD dwSize = sizeof(buf) - sizeof(short);

	RtlInitUnicodeString(&name,L"SystemRoot");
	us.Buffer = buf;
	us.Length = 0;
	us.MaximumLength = MAX_PATH * sizeof(short);
	Status = RtlQueryEnvironmentVariable_U(NULL,&name,&us);
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't query SystemRoot variable: %x!",(UINT)Status);
		return -1;
	}

/*	if(winx_reg_open_key(L"\\REGISTRY\\MACHINE\\SYSTEM\\UltraDefrag",&hKey) < 0)
		return -1;
	status = winx_reg_query_value(hKey,L"WindowsDirectory",REG_SZ,buf,&dwSize);
	winx_reg_close_key(hKey);
	if(status < 0) return -1;

	buf[dwSize / sizeof(short)] = 0;
	RtlInitUnicodeString(&us,buf);
*/	if(RtlUnicodeStringToAnsiString(&as,&us,TRUE) != STATUS_SUCCESS){
		winx_push_error("No enough memory!");
		return -1;
	}
	strcpy(configfile,"\\??\\");
	strncat(configfile,as.Buffer,sizeof(configfile) - strlen(configfile) - 1);
	configfile[sizeof(configfile) - 1] = 0;
 	strncat(configfile,"\\UltraDefrag\\options\\udefrag.cfg",
			sizeof(configfile) - strlen(configfile) - 1);
	RtlFreeAnsiString(&as);
	if(!strstr(configfile,"udefrag.cfg")){
		winx_push_error("Invalid config path %s!",configfile);
		return -1;
	}
	return 0;
}

/* load settings: always successful */
int __stdcall udefrag_load_settings(int argc, short **argv)
{
	int i;

	if(getopts(configfile) < 0)
		winx_pop_error(NULL,0);

	/* overwrite parameters from the command line */
	if(!argv) return 0;
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

/****f* udefrag.settings/udefrag_update_settings
* NAME
*    udefrag_update_settings
* SYNOPSIS
*    error = udefrag_update_settings();
* FUNCTION
*    Reloads settings and applies them.
* INPUTS
*    Nothing.
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    if(udefrag_update_settings() < 0){
*        udefrag_pop_error(buffer,sizeof(buffer));
*        // handle error
*    }
* SEE ALSO
*    udefrag_set_options
******/
int __stdcall udefrag_update_settings(void)
{
	udefrag_load_settings(0,NULL);
	return udefrag_set_options(&settings);
}

void ExtractToken(short *dest, short *src, int max_chars)
{
	signed int i,cnt;
	short ch;
	
	cnt = 0;
	for(i = 0; i < max_chars; i++){
		ch = src[i];
		/* skip spaces and tabs in the beginning */
		if((ch != 0x20 && ch != '\t') || cnt){
			dest[cnt] = ch;
			cnt++;
		}
	}
	dest[cnt] = 0;
	/* remove spaces and tabs from the end */
	if(cnt == 0) return;
	for(i = (cnt - 1); i >= 0; i--){
		ch = dest[i];
		if(ch != 0x20 && ch != '\t') break;
		dest[i] = 0;
	}
}

void ParseParameter()
{
	UNICODE_STRING us;
	ANSI_STRING as;
	
	int empty_value = value_buffer[0] ? FALSE : TRUE;
	/* check for invalid parameter */
	if(!param_buffer[0]) return;
	if(!wcscmp(param_buffer,L"INCLUDE")){
		wcsncpy(in_filter,value_buffer,MAX_FILTER_SIZE);
		in_filter[MAX_FILTER_SIZE] = 0;
		settings.in_filter = in_filter;
	} else if(!wcscmp(param_buffer,L"EXCLUDE")){
		wcsncpy(ex_filter,value_buffer,MAX_FILTER_SIZE);
		ex_filter[MAX_FILTER_SIZE] = 0;
		settings.ex_filter = ex_filter;
	} else if(!wcscmp(param_buffer,L"SIZELIMIT")){
		if(empty_value)
			settings.sizelimit = 0;
		else {
			RtlInitUnicodeString(&us,value_buffer);
			if(RtlUnicodeStringToAnsiString(&as,&us,TRUE) == STATUS_SUCCESS){
				dfbsize2(as.Buffer,&settings.sizelimit);
				RtlFreeAnsiString(&as);
			}
		}
	} else if(!wcscmp(param_buffer,L"BOOT_TIME_INCLUDE")){
		wcsncpy(boot_in_filter,value_buffer,MAX_FILTER_SIZE);
		boot_in_filter[MAX_FILTER_SIZE] = 0;
		settings.boot_in_filter = boot_in_filter;
	} else if(!wcscmp(param_buffer,L"BOOT_TIME_EXCLUDE")){
		wcsncpy(boot_ex_filter,value_buffer,MAX_FILTER_SIZE);
		boot_ex_filter[MAX_FILTER_SIZE] = 0;
		settings.boot_ex_filter = boot_ex_filter;
	} else if(!wcscmp(param_buffer,L"LETTERS")){
		wcsncpy(sched_letters,value_buffer,MAX_SCHED_LETTERS);
		sched_letters[MAX_SCHED_LETTERS] = 0;
		settings.sched_letters = sched_letters;
	} else if(!wcscmp(param_buffer,L"NEXT_BOOT")){
		_wcsupr(value_buffer);
		settings.next_boot = (wcscmp(value_buffer,L"YES") == 0) ? TRUE : FALSE;
	} else if(!wcscmp(param_buffer,L"EVERY_BOOT")){
		_wcsupr(value_buffer);
		settings.every_boot = (wcscmp(value_buffer,L"YES") == 0) ? TRUE : FALSE;
	} else if(!wcscmp(param_buffer,L"ENABLE_REPORTS")){
		_wcsupr(value_buffer);
		settings.report_type = (wcscmp(value_buffer,L"YES") == 0) ? HTML_REPORT : NO_REPORT;
	} else if(!wcscmp(param_buffer,L"DBGPRINT_LEVEL")){
		_wcsupr(value_buffer);
		if(!wcscmp(value_buffer,L"DETAILED"))
			settings.dbgprint_level = DBG_DETAILED;
		else if(!wcscmp(value_buffer,L"PARANOID"))
			settings.dbgprint_level = DBG_PARANOID;
		else if(!wcscmp(value_buffer,L"NORMAL"))
			settings.dbgprint_level = DBG_NORMAL;
	} else if(!wcscmp(param_buffer,L"MAP_UPDATE_INTERVAL")){
		if(!empty_value){
			settings.update_interval = _wtoi(value_buffer);
		}
	}
}

/* TODO: allow first blanks */
void ParseLine()
{
	short first_char = line_buffer[0];
	short *eq_pos;
	int param_len, value_len;

	/* skip empty lines */
	if(!first_char) return;
	/* skip comments */
	if(first_char == ';' || first_char == '#')
		return;
	/* find the equality sign */
	eq_pos = wcschr(line_buffer,'=');
	if(!eq_pos) return;
	/* extract a parameter-value pair */
	param_buffer[0] = value_buffer[0] = 0;
	param_len = (int)(LONG_PTR)(eq_pos - line_buffer);
	value_len = (int)(LONG_PTR)(line_buffer + wcslen(line_buffer) - eq_pos - 1);
	ExtractToken(param_buffer,line_buffer,param_len);
	ExtractToken(value_buffer,eq_pos + 1,value_len);
	_wcsupr(param_buffer);
	ParseParameter();
}

/* new interface to udefrag.cfg file*/
int getopts(char *filename)
{
	int filesize,i,cnt;
	short ch;

	WINX_FILE *f  = winx_fopen(filename,"r");
	if(!f) return -1;
	filesize = winx_fread(file_buffer,sizeof(short),
			(sizeof(file_buffer) / sizeof(short)) - 1,f);
	winx_fclose(f);
	if(!filesize) return -1;
	file_buffer[filesize] = 0;
	line_buffer[0] = 0;
	cnt = 0;
	for(i = 0; i < filesize; i++){
		ch = file_buffer[i];
		if(ch == '\r' || ch == '\n'){
			/* terminate the line buffer */
			line_buffer[cnt] = 0;
			cnt = 0;
			/* parse line buffer contents */
			ParseLine();
		} else {
			line_buffer[cnt] = ch;
			cnt++;
		}
	}
	return 0;
}
