/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 *  udefrag.dll - middle layer between driver and user interfaces:
 *  functions to get/set program settings.
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

/* global variables */
char settings_msg[1024];
extern int __native_mode;
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

short smss_key[] = \
	L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager";

/* instead of malloc calls */
char reg_buffer[65536];

#define MAX_FILTER_SIZE 4096
#define MAX_FILTER_BYTESIZE (MAX_FILTER_SIZE * sizeof(short))
short in_filter[MAX_FILTER_SIZE + 1];
short ex_filter[MAX_FILTER_SIZE + 1];
short boot_in_filter[MAX_FILTER_SIZE + 1];
short boot_ex_filter[MAX_FILTER_SIZE + 1];

#define MAX_SCHED_LETTERS 64
short sched_letters[MAX_SCHED_LETTERS + 1];

/* internal functions prototypes */
BOOL OpenKey(short *key_name,PHANDLE phKey);
BOOL CreateKey(short *key_name,PHANDLE phKey);
//BOOL ReadRegDWORD(HANDLE hKey,short *value_name,DWORD *pValue);
//BOOL WriteRegDWORD(HANDLE hKey,short *value_name,DWORD value);
BOOL ReadRegBinary(HANDLE hKey,short *value_name,DWORD type,void *buffer,DWORD size);
BOOL WriteRegBinary(HANDLE hKey,short *value_name,DWORD type,void *buffer,DWORD size);
BOOL AddAppToBootExecute(void);
BOOL RemoveAppFromBootExecute(void);

#define ReadRegDWORD(h,n,pv) ReadRegBinary(h,n,REG_DWORD,pv,sizeof(DWORD))
#define WriteRegDWORD(h,n,v) WriteRegBinary(h,n,REG_DWORD,&(v),sizeof(DWORD))

extern BOOL n_ioctl(HANDLE handle,HANDLE event,ULONG code,
			PVOID in_buf,ULONG in_size,
			PVOID out_buf,ULONG out_size,
			char *err_format_string,char *msg_buffer);

/* load settings: always returns NULL */
char * __stdcall udefrag_load_settings(int argc, short **argv)
{
	HANDLE hKey;
	DWORD x;
	int i;

	/* open program key */
	if(!OpenKey(ud_key,&hKey)) goto analyse_cmdline;
	/* read dword parameters */
	ReadRegDWORD(hKey,L"skip removable",&settings.skip_removable);
	ReadRegDWORD(hKey,L"update interval",&settings.update_interval);
	ReadRegDWORD(hKey,L"show progress",&settings.show_progress);
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
	ReadRegBinary(hKey,L"sizelimit",REG_BINARY,&settings.sizelimit,sizeof(ULONGLONG));
	/* read strings */
	/* if success then write terminating zero and set ud_settings structure field */
	if(ReadRegBinary(hKey,L"include filter",REG_SZ,in_filter,MAX_FILTER_BYTESIZE))
	{
		in_filter[MAX_FILTER_SIZE] = 0; settings.in_filter = in_filter;
	}
	if(ReadRegBinary(hKey,L"exclude filter",REG_SZ,ex_filter,MAX_FILTER_BYTESIZE))
	{
		ex_filter[MAX_FILTER_SIZE] = 0; settings.ex_filter = ex_filter;
	}
	if(ReadRegBinary(hKey,L"boot time include filter",REG_SZ,
		boot_in_filter,MAX_FILTER_BYTESIZE))
	{
		boot_in_filter[MAX_FILTER_SIZE] = 0; settings.boot_in_filter = boot_in_filter;
	}
	if(ReadRegBinary(hKey,L"boot time exclude filter",REG_SZ,
		boot_ex_filter,MAX_FILTER_BYTESIZE))
	{
		boot_ex_filter[MAX_FILTER_SIZE] = 0; settings.boot_ex_filter = boot_ex_filter;
	}
	if(ReadRegBinary(hKey,L"scheduled letters",REG_SZ,sched_letters,
		MAX_SCHED_LETTERS * sizeof(short)))
	{
		sched_letters[MAX_SCHED_LETTERS] = 0; settings.sched_letters = sched_letters;
	}
	NtClose(hKey);
analyse_cmdline:
	/* overwrite parameters from the command line */
	if(!argv) goto no_cmdline;
	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] != '-' && argv[i][0] != '/')
			continue;
		switch(argv[i][1])
		{
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
	return NULL;
}

ud_options * __stdcall udefrag_get_options(void)
{
	return &settings;
}

char * __stdcall i_udefrag_set_options(ud_options *ud_opts)
{
	REPORT_TYPE rt;

	if(!init_event)
	{
		strcpy(settings_msg,"Udefrag.dll apply_settings call without initialization!");
		goto apply_settings_fail;
	}
	if(ud_opts != &settings)
		memcpy(&settings,ud_opts,sizeof(ud_options));
	/* set debug print level */
	if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_DBGPRINT_LEVEL,
		&settings.dbgprint_level,sizeof(DWORD),NULL,0,
		"Can't set debug print level: %x!",settings_msg)) goto apply_settings_fail;
	/* set report characterisics */
	rt.format = settings.report_format;
	rt.type = settings.report_type;
	if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_REPORT_TYPE,
		&rt,sizeof(REPORT_TYPE),NULL,0,
		"Can't set report type: %x!",settings_msg)) goto apply_settings_fail;
	/* set filters */
	if(__native_mode)
	{
		if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_INCLUDE_FILTER,
			settings.boot_in_filter,(wcslen(settings.boot_in_filter) + 1) << 1,NULL,0,
			"Can't set include filter: %x!",settings_msg)) goto apply_settings_fail;
		if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_EXCLUDE_FILTER,
			settings.boot_ex_filter,(wcslen(settings.boot_ex_filter) + 1) << 1,NULL,0,
			"Can't set exclude filter: %x!",settings_msg)) goto apply_settings_fail;
	}
	else
	{
		if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_INCLUDE_FILTER,
			settings.in_filter,(wcslen(settings.in_filter) + 1) << 1,NULL,0,
			"Can't set include filter: %x!",settings_msg)) goto apply_settings_fail;
		if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_EXCLUDE_FILTER,
			settings.ex_filter,(wcslen(settings.ex_filter) + 1) << 1,NULL,0,
			"Can't set exclude filter: %x!",settings_msg)) goto apply_settings_fail;
	}
	return NULL;
apply_settings_fail:
	return settings_msg;
}

char * __stdcall i_udefrag_save_settings(void)
{
	HANDLE hKey;
	DWORD x;

	/* native app should clean registry */
	if(__native_mode)
		settings.next_boot = FALSE;
	/* create key if not exist */
	if(!OpenKey(ud_key,&hKey))
		if(!CreateKey(ud_key,&hKey)) goto save_fail;
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
	WriteRegBinary(hKey,L"sizelimit",REG_BINARY,&settings.sizelimit,sizeof(ULONGLONG));
	/* write strings */
	WriteRegBinary(hKey,L"include filter",REG_SZ,settings.in_filter,
		(wcslen(settings.in_filter) + 1) << 1);
	WriteRegBinary(hKey,L"exclude filter",REG_SZ,settings.ex_filter,
		(wcslen(settings.ex_filter) + 1) << 1);
	WriteRegBinary(hKey,L"boot time include filter",REG_SZ,settings.boot_in_filter,
		(wcslen(settings.boot_in_filter) + 1) << 1);
	WriteRegBinary(hKey,L"boot time exclude filter",REG_SZ,settings.boot_ex_filter,
		(wcslen(settings.boot_ex_filter) + 1) << 1);
	WriteRegBinary(hKey,L"scheduled letters",REG_SZ,settings.sched_letters,
		(wcslen(settings.sched_letters) + 1) << 1);
	NtClose(hKey);
	/* native app should clean registry */
	if(__native_mode && !settings.every_boot)
		if(!RemoveAppFromBootExecute()) goto save_fail;
	if(!__native_mode)
	{
		if(settings.next_boot || settings.every_boot)
		{
			if(!AddAppToBootExecute()) goto save_fail;
		}
		else
		{
			if(!RemoveAppFromBootExecute()) goto save_fail;
		}
	}
	return NULL;
save_fail:
	return settings_msg;
}

/* important registry cleanup for uninstaller */
char * __stdcall i_udefrag_clean_registry(void)
{
	return RemoveAppFromBootExecute() ? NULL : settings_msg;
}

/* registry cleanup for native executable */
char * __stdcall i_udefrag_native_clean_registry(void)
{
	HANDLE hKey;

	udefrag_load_settings(0,NULL);
	if(settings.next_boot)
	{
		settings.next_boot = 0;
		/* set next boot parameter to zero */
		if(OpenKey(ud_key,&hKey))
		{
			WriteRegDWORD(hKey,L"next boot",settings.next_boot);
			NtClose(hKey);
		}
	}
	if(settings.every_boot) return NULL;
	return RemoveAppFromBootExecute() ? NULL : settings_msg;
}

BOOL OpenKey(short *key_name,PHANDLE phKey)
{
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS Status;

	RtlInitUnicodeString(&uStr,key_name);
	/* OBJ_CASE_INSENSITIVE must be specified on NT 4.0 !!! */
	InitializeObjectAttributes(&ObjectAttributes,&uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
	Status = NtOpenKey(phKey,KEY_QUERY_VALUE | KEY_SET_VALUE,&ObjectAttributes);
	if(!NT_SUCCESS(Status))
	{
		sprintf(settings_msg,"Can't open %ws key: %x!",key_name,Status);
		return FALSE;
	}
	return TRUE;
}

BOOL CreateKey(short *key_name,PHANDLE phKey)
{
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS Status;

	RtlInitUnicodeString(&uStr,key_name);
	/* OBJ_CASE_INSENSITIVE must be specified on NT 4.0 !!! */
	InitializeObjectAttributes(&ObjectAttributes,&uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
	Status = NtCreateKey(phKey,KEY_ALL_ACCESS,&ObjectAttributes,
				0,NULL,REG_OPTION_NON_VOLATILE,NULL);
	if(!NT_SUCCESS(Status))
	{
		sprintf(settings_msg,"Can't create %ws key: %x!",key_name,Status);
		return FALSE;
	}
	return TRUE;
}

BOOL ReadRegBinary(HANDLE hKey,short *value_name,DWORD type,void *buffer,DWORD size)
{
	UNICODE_STRING uStr;
	NTSTATUS Status;
	KEY_VALUE_PARTIAL_INFORMATION *pInfo = (KEY_VALUE_PARTIAL_INFORMATION *)reg_buffer;
	DWORD buffer_size = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + size;

	RtlInitUnicodeString(&uStr,value_name);
	Status = NtQueryValueKey(hKey,&uStr,KeyValuePartialInformation,
		pInfo,buffer_size,&buffer_size);
	if(NT_SUCCESS(Status))
	{
		if(pInfo->Type == type && pInfo->DataLength <= size)
		{
			if(buffer)
				RtlCopyMemory(buffer,pInfo->Data,pInfo->DataLength);
			return TRUE;
		}
		sprintf(settings_msg,"Invalid parameter %ws!",value_name);
		goto read_fail;
	}
	sprintf(settings_msg,"Can't read %ws value: %x!",value_name,Status);
read_fail:
	return FALSE;
}

BOOL WriteRegBinary(HANDLE hKey,short *value_name,DWORD type,void *buffer,DWORD size)
{
	UNICODE_STRING uStr;
	NTSTATUS Status;

	RtlInitUnicodeString(&uStr,value_name);
	Status = NtSetValueKey(hKey,&uStr,0,type,buffer,size);
	if(!NT_SUCCESS(Status))
	{
		sprintf(settings_msg,"Can't write %ws value: %x!",value_name,Status);
		return FALSE;
	}
	return TRUE;
}

BOOL AddAppToBootExecute(void)
{
	HANDLE hKey;
	KEY_VALUE_PARTIAL_INFORMATION *pInfo = (KEY_VALUE_PARTIAL_INFORMATION *)reg_buffer;
	short *data, *curr_pos;
	unsigned long length,curr_len,i,new_length = 0;


	/* add native program name to the BootExecute registry parameter */
	if(!OpenKey(smss_key,&hKey)) return FALSE;
	if(!ReadRegBinary(hKey,L"BootExecute",REG_MULTI_SZ,NULL,65536))
	{
		NtClose(hKey);
		return FALSE;
	}
	data = (short *)pInfo->Data;
	length = pInfo->DataLength;
	if(length > 32000)
	{
		sprintf(settings_msg,"BootExecute value is too long - %u!",length);
		NtClose(hKey);
		return FALSE;
	}
	/* if we have 'defrag_native' string in parameter then exit */
	/* convert length to number of wchars without the last one */
	length = (length >> 1) - 1;
	for(i = 0; i < length;)
	{
		curr_pos = data + i;
		curr_len = wcslen(curr_pos) + 1;
		if(!wcscmp(curr_pos,L"defrag_native")) /* if strings are equal */
			goto add_success;
		i +=  curr_len;
	}
	wcscpy(data + i,L"defrag_native");
	data[i + wcslen(L"defrag_native") + 1] = 0;
	length += wcslen(L"defrag_native") + 1 + 1;
	if(!WriteRegBinary(hKey,L"BootExecute",REG_MULTI_SZ,data,length << 1))
	{
		NtClose(hKey);
		return FALSE;
	}
add_success:
	NtClose(hKey);
	return TRUE;
}

BOOL RemoveAppFromBootExecute(void)
{
	HANDLE hKey;
	KEY_VALUE_PARTIAL_INFORMATION *pInfo = (KEY_VALUE_PARTIAL_INFORMATION *)reg_buffer;
	short *data, *curr_pos;
	short *new_data;
	unsigned long length,curr_len,i,new_length = 0;


	/* remove native program name from BootExecute registry parameter */
	if(!OpenKey(smss_key,&hKey)) return FALSE;
	if(!ReadRegBinary(hKey,L"BootExecute",REG_MULTI_SZ,NULL,65536))
	{
		NtClose(hKey);
		return FALSE;
	}
	data = (short *)pInfo->Data;
	length = pInfo->DataLength;
	if(length > 32000)
	{
		sprintf(settings_msg,"BootExecute value is too long - %u!",length);
		NtClose(hKey);
		return FALSE;
	}
	new_data = (short *)(reg_buffer + 32768);
	/* now we should copy all strings except 'defrag_native' 
	   to the destination buffer */
	memset((void *)new_data,0,length);
	/* convert length to number of wchars without the last one */
	length = (length >> 1) - 1;
	for(i = 0; i < length;)
	{
		curr_pos = data + i;
		curr_len = wcslen(curr_pos) + 1;
		if(wcscmp(curr_pos,L"defrag_native"))
		{ /* if strings are not equal */
			wcscpy(new_data + new_length,curr_pos);
			new_length += curr_len;
		}
		i +=  curr_len;
	}
	new_data[new_length] = 0;
	if(!WriteRegBinary(hKey,L"BootExecute",REG_MULTI_SZ,new_data,(new_length + 1) << 1))
	{
		NtClose(hKey);
		return FALSE;
	}
	NtClose(hKey);
	return TRUE;
}
