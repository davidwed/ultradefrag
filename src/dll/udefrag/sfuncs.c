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
 *  udefrag.dll - middle layer between driver and user interfaces:
 *  simple interface functions set for scripting languages.
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

#define FormatMessageFound		0
#define FormatMessageNotFound	1
#define FormatMessageUndefined	2

#define CFUNCBUFFERSIZE 2048

char msg_buffer_a[CFUNCBUFFERSIZE+2];
char map_msg_buffer_a[CFUNCBUFFERSIZE+2];
char vol_msg_buffer_a[CFUNCBUFFERSIZE+2];

char s_letters[MAX_DOS_DRIVES + 1];
char s_msg[4096];
char s_map_msg[4096];
char map[32768];

char unk_code_a[] = "Unknown error code.";
extern int FormatMessageState;

extern DWORD (WINAPI *func_FormatMessageA)(DWORD,PVOID,DWORD,DWORD,LPSTR,DWORD,va_list*);
extern DWORD (WINAPI *func_FormatMessageW)(DWORD,PVOID,DWORD,DWORD,LPWSTR,DWORD,va_list*);

/* internal functions prototypes */
char * __stdcall i_udefrag_init(int argc, short **argv,int native_mode,long map_size);
char * __stdcall i_udefrag_unload(BOOL save_options);
char * __stdcall i_udefrag_analyse(unsigned char letter);
char * __stdcall i_udefrag_defragment(unsigned char letter);
char * __stdcall i_udefrag_optimize(unsigned char letter);
char * __stdcall i_udefrag_analyse_ex(unsigned char letter,STATUPDATEPROC sproc);
char * __stdcall i_udefrag_defragment_ex(unsigned char letter,STATUPDATEPROC sproc);
char * __stdcall i_udefrag_optimize_ex(unsigned char letter,STATUPDATEPROC sproc);
char * __stdcall i_udefrag_get_ex_command_result(void);
char * __stdcall i_udefrag_stop(void);
char * __stdcall i_udefrag_get_progress(STATISTIC *pstat, double *percentage);
char * __stdcall i_udefrag_get_map(char *buffer,int size);
char * __stdcall i_udefrag_get_avail_volumes(volume_info **vol_info,int skip_removable);
char * __stdcall i_udefrag_validate_volume(unsigned char letter,int skip_removable);
char * __stdcall i_scheduler_get_avail_letters(char *letters);
char * __stdcall i_udefrag_set_options(ud_options *ud_opts);
char * __stdcall i_udefrag_save_settings(void);
char * __stdcall i_udefrag_clean_registry(void);
char * __stdcall i_udefrag_native_clean_registry(void);

BOOL get_proc_address(short *libname,char *funcname,PVOID *proc_addr);

char *format_message_a(char *string,char *buffer)
{
	char *p;
	UINT iStatus;
	NTSTATUS Status;
	ULONG err_code;
	int length;

	/* check string */
	if(!string) return NULL;
	if(!string[0])
	{
		buffer[0] = 0;
		return buffer;
	}
	/* copy string to buffer */
	strcpy(buffer,string);
	/* if status code is specified, add corresponding text */
	p = strrchr(string,':');
	if(p)
	{
		if(FormatMessageState == FormatMessageUndefined)
		{
			if(get_proc_address(L"kernel32.dll","FormatMessageA",(void *)&func_FormatMessageA))
				FormatMessageState = FormatMessageFound;
			else
				FormatMessageState = FormatMessageNotFound;
			if(!get_proc_address(L"kernel32.dll","FormatMessageW",(void *)&func_FormatMessageW))
				FormatMessageState = FormatMessageNotFound;
		}
		if(FormatMessageState == FormatMessageFound)
		{
			strcat(buffer,"\n");
			/* skip ':' and spaces */
			p ++;
			while(*p == 0x20) p++;
			sscanf(p,"%x",&iStatus);
			/* very important for x64 targets */
			Status = (NTSTATUS)(signed int)iStatus;
			err_code = RtlNtStatusToDosError(Status);
			length = strlen(buffer);
			if(!func_FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err_code,
				MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
				buffer + length,CFUNCBUFFERSIZE - length - 1,NULL))
			{
				strcat(buffer,unk_code_a);
			}
		}
		else
		{
			/*
			 * FIXME: handle this error
			 */
		}
	}
	return buffer;
}

char * __stdcall udefrag_s_init(long map_size)
{
	char *result;

	result = format_message_a(i_udefrag_init(0,NULL,FALSE,map_size),msg_buffer_a);
	return result ? result : "";
}

char * __stdcall udefrag_s_unload(BOOL save_options)
{
	char *result;

	result = format_message_a(i_udefrag_unload(save_options),msg_buffer_a);
	return result ? result : "";
}

char * __stdcall udefrag_s_analyse(unsigned char letter)
{
	char *result;

	result = format_message_a(i_udefrag_analyse(letter),msg_buffer_a);
	return result ? result : "";
}

char * __stdcall udefrag_s_defragment(unsigned char letter)
{
	char *result;

	result = format_message_a(i_udefrag_defragment(letter),msg_buffer_a);
	return result ? result : "";
}

char * __stdcall udefrag_s_optimize(unsigned char letter)
{
	char *result;

	result = format_message_a(i_udefrag_optimize(letter),msg_buffer_a);
	return result ? result : "";
}

char * __stdcall udefrag_s_stop(void)
{
	char *result;

	result = format_message_a(i_udefrag_stop(),msg_buffer_a);
	return result ? result : "";
}

char * __stdcall udefrag_s_get_progress(void)
{
	return "";
}

char * __stdcall udefrag_s_get_map(int size)
{
	char *result;
	int i;

	if(size > sizeof(map) - 1)
		return "ERROR: Map resolution is too high!";
	result = format_message_a(i_udefrag_get_map(map,size - 1),map_msg_buffer_a);
	if(result)
	{
		strcpy(s_map_msg,"ERROR: ");
		strcat(s_map_msg,result);
		return s_map_msg;
	}
	/* remove zeroes from the map and terminate string */
	for(i = 0; i < size - 1; i++)
		map[i] ++;
	map[i] = 0;
	return map;
}

char * __stdcall udefrag_s_get_options(void)
{
	return "";
}

char * __stdcall udefrag_s_set_options(char *string)
{
	return "";
}

char * __stdcall udefrag_s_get_avail_volumes(int skip_removable)
{
	char *result;
	volume_info *v;
	int i;
	char chr;
	char t[256];
	int p;
	double d;

	result = format_message_a(i_udefrag_get_avail_volumes(&v,skip_removable),vol_msg_buffer_a);
	if(result)
	{
		strcpy(s_msg,"ERROR: ");
		strcat(s_msg,result);
		return s_msg;
	}
	strcpy(s_msg,"");
	for(i = 0;;i++)
	{
		chr = v[i].letter;
		if(!chr) break;
		sprintf(t,"%c:%s:",chr,v[i].fsname);
		strcat(s_msg,t);
		fbsize(t,(ULONGLONG)(v[i].total_space.QuadPart));
		strcat(s_msg,t);
		strcat(s_msg,":");
		fbsize(t,(ULONGLONG)(v[i].free_space.QuadPart));
		strcat(s_msg,t);
		strcat(s_msg,":");
		d = (double)(signed __int64)(v[i].free_space.QuadPart);
		d /= ((double)(signed __int64)(v[i].total_space.QuadPart) + 0.1);
		p = (int)(100 * d);
		sprintf(t,"%u %%:",p);
		strcat(s_msg,t);
	}
	/* remove last ':' */
	if(strlen(s_msg) > 0)
		s_msg[strlen(s_msg) - 1] = 0;
	return s_msg;
}

char * __stdcall udefrag_s_validate_volume(unsigned char letter,int skip_removable)
{
	char *result;

	result = format_message_a(i_udefrag_validate_volume(letter,skip_removable),vol_msg_buffer_a);
	return result ? result : "";
}

char * __stdcall scheduler_s_get_avail_letters(void)
{
	char *result;

	result = format_message_a(i_scheduler_get_avail_letters(s_letters),vol_msg_buffer_a);
	if(!result) return s_letters;
	strcpy(s_msg,"ERROR: ");
	strcat(s_msg,result);
	return s_msg;
}

char * __stdcall udefrag_s_analyse_ex(unsigned char letter,STATUPDATEPROC sproc)
{
	char *result;

	result = format_message_a(i_udefrag_analyse_ex(letter,sproc),msg_buffer_a);
	return result ? result : "";
}

char * __stdcall udefrag_s_defragment_ex(unsigned char letter,STATUPDATEPROC sproc)
{
	char *result;

	result = format_message_a(i_udefrag_defragment_ex(letter,sproc),msg_buffer_a);
	return result ? result : "";
}

char * __stdcall udefrag_s_optimize_ex(unsigned char letter,STATUPDATEPROC sproc)
{
	char *result;

	result = format_message_a(i_udefrag_optimize_ex(letter,sproc),msg_buffer_a);
	return result ? result : "";
}

char * __stdcall udefrag_s_get_ex_command_result(void)
{
	char *result;

	result = format_message_a(i_udefrag_get_ex_command_result(),msg_buffer_a);
	return result ? result : "";
}
