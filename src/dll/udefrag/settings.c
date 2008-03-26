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
int saveopts(char *filename);

char configfile[MAX_PATH] = "";

extern int fsz;

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

int get_configfile_location(void)
{
	HANDLE hKey;
	int status;
	UNICODE_STRING us;
	ANSI_STRING as;
	short buf[MAX_PATH];
	DWORD dwSize = sizeof(buf) - sizeof(short);

	if(winx_reg_open_key(L"\\REGISTRY\\MACHINE\\SYSTEM\\UltraDefrag",&hKey) < 0)
		return -1;
	status = winx_reg_query_value(hKey,L"WindowsDirectory",REG_SZ,buf,&dwSize);
	winx_reg_close_key(hKey);
	if(status < 0) return -1;

	buf[dwSize / sizeof(short)] = 0;
	RtlInitUnicodeString(&us,buf);
	if(RtlUnicodeStringToAnsiString(&as,&us,TRUE) != STATUS_SUCCESS){
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
	HANDLE hKey;
	DWORD x;
	int i;

	if(getopts(configfile) >= 0) goto analyse_cmdline;
	winx_pop_error(NULL,0);
	
	/* open program key */
	if(winx_reg_open_key(ud_key,&hKey) < 0){
		winx_pop_error(NULL,0);
		goto analyse_cmdline;
	}
	/* read dword parameters */
	ReadRegDWORD(hKey,L"update interval",(DWORD *)&settings.update_interval);
	ReadRegDWORD(hKey,L"dbgprint level",&settings.dbgprint_level);
	ReadRegDWORD(hKey,L"every boot",&settings.every_boot);
	ReadRegDWORD(hKey,L"next boot",&settings.next_boot);

	if(ReadRegDWORD(hKey,L"report type",&x))
		settings.report_type = (UCHAR)x;

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
//	HANDLE hKey;
//	DWORD x;

	if(native_mode_flag) settings.next_boot = FALSE;
	if(saveopts(configfile) < 0) goto save_fail;

#if 0
	/* create key if not exist */
	if(winx_reg_open_key(ud_key,&hKey) < 0){
		winx_pop_error(NULL,0);
		if(winx_reg_create_key(ud_key,&hKey) < 0) goto save_fail;
	}
	/* set dword values */
	WriteRegDWORD(hKey,L"update interval",settings.update_interval);
	WriteRegDWORD(hKey,L"dbgprint level",settings.dbgprint_level);
	WriteRegDWORD(hKey,L"every boot",settings.every_boot);
	WriteRegDWORD(hKey,L"next boot",settings.next_boot);
	x = (DWORD)settings.report_type;
	WriteRegDWORD(hKey,L"report type",x);
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
#endif
	
	if(settings.next_boot || settings.every_boot){
		if(winx_reg_add_to_boot_execute(L"defrag_native") < 0) goto save_fail;
	} else {
		if(winx_reg_remove_from_boot_execute(L"defrag_native") < 0) goto save_fail;
	}

	return 0;
save_fail:
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

/****f* udefrag.settings/udefrag_clean_registry
* NAME
*    udefrag_clean_registry
* SYNOPSIS
*    error = udefrag_clean_registry();
* FUNCTION
*    Important registry cleanup for the uninstaller.
* INPUTS
*    Nothing.
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    if(udefrag_clean_registry() < 0){
*        udefrag_pop_error(buffer,sizeof(buffer));
*        // handle error
*    }
* SEE ALSO
*    udefrag_native_clean_registry
******/
int __stdcall udefrag_clean_registry(void)
{
	return winx_reg_remove_from_boot_execute(L"defrag_native");
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

int saveopts(char *filename)
{
	int status;
	char sl[68];
	char *dl;
	
	WINX_FILE *f = winx_fopen(filename,"w");
	if(!f) return -1;
	fbsize2(sl,settings.sizelimit);
	switch(settings.dbgprint_level){
	case DBG_DETAILED:
		dl = "DETAILED";
		break;
	case DBG_PARANOID:
		dl = "PARANOID";
		break;
	default:
		dl = "NORMAL";
	}
	_snwprintf(file_buffer,sizeof(file_buffer) / sizeof(short),
			L"; !!! NOTE: THIS FILE MUST BE SAVED IN UNICODE (UTF-16) ENCODING !!!\r\n\r\n"
			L";------------------------------------------------------------------------------\r\n"
			L";\r\n"
			L"; Filter\r\n"
			L";\r\n"
			L"; 1. Enter multiple filter match strings separated by the semicolon (;)\r\n"
			L";    character. Note that filter is case insensitive.\r\n"
			L"; 2. You can specify maximum file size with the following suffixes:\r\n"
			L";    Kb, Mb, Gb, Tb, Eb, Pb.\r\n"
			L"; 3. The patterns you supply for the include and exclude functionality\r\n"
			L";    applies to the whole path. For example, if you specify temp in the\r\n"
			L";    exclude path you would exclude both the temp folder (WINDIR\\temp) and\r\n"
			L";    the internet explorer temporary files folder. Filter strings cannot be\r\n"
			L";    longer than 4096 bytes.\r\n"
			L";\r\n"
			L";------------------------------------------------------------------------------\r\n\r\n"
			L"INCLUDE             = %ws\r\n"
			L"EXCLUDE             = %ws\r\n"
			L"SIZELIMIT           = %S\r\n\r\n"
			L";------------------------------------------------------------------------------\r\n"
			L";\r\n"
			L"; Boot time settings\r\n"
			L";\r\n"
			L"; 1. Enter multiple filter match strings separated by the semicolon (;)\r\n"
			L";    character. Note that filter is case insensitive.\r\n"
			L"; 2. The volume letters must be either separated by semicolons (;)\r\n" 
			L";    or don't contain any separators between.\r\n"
			L"; 3. Set NEXT_BOOT/EVERY_BOOT to YES/NO to chose when to defragment.\r\n"
			L";\r\n"
			L";------------------------------------------------------------------------------\r\n\r\n"
			L"BOOT_TIME_INCLUDE   = %ws\r\n"
			L"BOOT_TIME_EXCLUDE   = %ws\r\n"
			L"LETTERS             = %ws\r\n"
			L"NEXT_BOOT           = %S\r\n"
			L"EVERY_BOOT          = %S\r\n\r\n"
			L";------------------------------------------------------------------------------\r\n"
			L";\r\n"
			L"; Main report settings.\r\n"
			L";\r\n"
			L"; 1. Set ENABLE_REPORTS parameter to YES/NO state \r\n"
			L";    to enable/disable HTML reports.\r\n"
			L"; 2. Set DBGPRINT_LEVEL = NORMAL to view useful messages about the analyse or\r\n"
			L";    defrag progress. Select DETAILED to create a bug report to send to the\r\n"
			L";    author when an error is encountered. Select PARANOID in extraordinary\r\n"
			L";    cases. Of course, you need have DbgView program or DbgPrint Logger\r\n"
			L";    installed to view logs.\r\n"
			L";\r\n"
			L";------------------------------------------------------------------------------\r\n\r\n"
			L"ENABLE_REPORTS      = %S\r\n"
			L"DBGPRINT_LEVEL      = %S\r\n\r\n"
			L";------------------------------------------------------------------------------\r\n"
			L";\r\n"
			L"; Other settings\r\n"
			L";\r\n"
			L"; Specify the map update interval in milliseconds.\r\n"
			L";\r\n"
			L";------------------------------------------------------------------------------\r\n\r\n"
			L"MAP_UPDATE_INTERVAL = %u\r\n"
			,
			settings.in_filter,
			settings.ex_filter,
			sl,
			settings.boot_in_filter,
			settings.boot_ex_filter,
			settings.sched_letters,
			settings.next_boot ? "YES" : "NO",
			settings.every_boot ? "YES" : "NO",
			(settings.report_type != NO_REPORT) ? "YES" : "NO",
			dl,
			settings.update_interval
			);
	file_buffer[(sizeof(file_buffer) / sizeof(short)) - 1] = 0;
	if(!wcslen(file_buffer)) goto failure;
	status = winx_fwrite(file_buffer,
			 sizeof(short),wcslen(file_buffer),f);
	winx_fclose(f);
	if(status == wcslen(file_buffer)) return 0;
	if(status){
failure:
	winx_push_error("Can't save settings!");
	}
	return -1;
}
