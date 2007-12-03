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
 *  interface for classic programming languages.
 */

/*
 *  NOTE: this library isn't absolutely thread-safe. Be careful!
 */

/*
 *  Name i_xxx() means Internal xxx(); n_yyy() - Native yyy();
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

short msg_buffer[CFUNCBUFFERSIZE+2];
short stop_msg_buffer[CFUNCBUFFERSIZE+2];
short progress_msg_buffer[CFUNCBUFFERSIZE+2];
short map_msg_buffer[CFUNCBUFFERSIZE+2];
short vol_msg_buffer[CFUNCBUFFERSIZE+2];
short settings_msg_buffer[CFUNCBUFFERSIZE+2];

short fmt_error[] = L"Can't format message!";
short unk_code[] =  L"Unknown error code.";
int FormatMessageState = FormatMessageUndefined;

DWORD (WINAPI *func_FormatMessageA)(DWORD,PVOID,DWORD,DWORD,LPSTR,DWORD,va_list*);
DWORD (WINAPI *func_FormatMessageW)(DWORD,PVOID,DWORD,DWORD,LPWSTR,DWORD,va_list*);

/* internal functions prototypes */
char * __stdcall i_udefrag_init(int argc, short **argv,int native_mode);
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

short *format_message_w(char *string,short *buffer)
{
	char *p;
	ANSI_STRING aStr;
	UNICODE_STRING uStr;
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
	/* convert string to unicode */
	RtlInitAnsiString(&aStr,string);
	uStr.Buffer = buffer;
	uStr.Length = 0;
	uStr.MaximumLength = CFUNCBUFFERSIZE;
	if(RtlAnsiStringToUnicodeString(&uStr,&aStr,FALSE) != STATUS_SUCCESS)
		wcscpy(buffer,fmt_error);
	/* if status code is specified, add corresponding text */
	p = strrchr(string,':');
	if(p)
	{
		/* skip ':' and spaces */
		p ++;
		while(*p == 0x20) p++;
		sscanf(p,"%x",&Status);
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
			wcscat(buffer,L"\n");
			err_code = RtlNtStatusToDosError(Status);
			length = wcslen(buffer);
			if(!func_FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err_code,
				MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
				buffer + length,CFUNCBUFFERSIZE - length - 1,NULL))
			{
				wcscat(buffer,unk_code);
			}
		}
		else
		{
			/* in native applicatons we should display 
			 * detailed info for common errors
			 */
		}
	}
	return buffer;
}

short * __stdcall udefrag_init(int argc, short **argv,int native_mode)
{
	return format_message_w(i_udefrag_init(argc,argv,native_mode),msg_buffer);
}

short * __stdcall udefrag_unload(BOOL save_options)
{
	return format_message_w(i_udefrag_unload(save_options),msg_buffer);
}

/* you can send only one command at the same time */
short * __stdcall udefrag_analyse(unsigned char letter)
{
	return format_message_w(i_udefrag_analyse(letter),msg_buffer);
}

short * __stdcall udefrag_defragment(unsigned char letter)
{
	return format_message_w(i_udefrag_defragment(letter),msg_buffer);
}

short * __stdcall udefrag_optimize(unsigned char letter)
{
	return format_message_w(i_udefrag_optimize(letter),msg_buffer);
}

/*
 * NOTE: don't use create_thread call before command_ex returns!
 */
short * __stdcall udefrag_analyse_ex(unsigned char letter,STATUPDATEPROC sproc)
{
	return format_message_w(i_udefrag_analyse_ex(letter,sproc),msg_buffer);
}

short * __stdcall udefrag_defragment_ex(unsigned char letter,STATUPDATEPROC sproc)
{
	return format_message_w(i_udefrag_defragment_ex(letter,sproc),msg_buffer);
}

short * __stdcall udefrag_optimize_ex(unsigned char letter,STATUPDATEPROC sproc)
{
	return format_message_w(i_udefrag_optimize_ex(letter,sproc),msg_buffer);
}

short * __stdcall udefrag_get_ex_command_result(void)
{
	return format_message_w(i_udefrag_get_ex_command_result(),msg_buffer);
}

short * __stdcall udefrag_stop(void)
{
	return format_message_w(i_udefrag_stop(),stop_msg_buffer);
}

short * __stdcall udefrag_get_progress(STATISTIC *pstat, double *percentage)
{
	return format_message_w(i_udefrag_get_progress(pstat,percentage),progress_msg_buffer);
}

short * __stdcall udefrag_get_map(char *buffer,int size)
{
	return format_message_w(i_udefrag_get_map(buffer,size),map_msg_buffer);
}

/* following functions set isn't thread-safe: you should call only one at same time */

/* NOTE: if(skip_removable == FALSE && you have floppy drive without floppy disk)
 *       then you will hear noise :))
 */
short * __stdcall udefrag_get_avail_volumes(volume_info **vol_info,int skip_removable)
{
	return format_message_w(i_udefrag_get_avail_volumes(vol_info,skip_removable),vol_msg_buffer);
}

/* NOTE: this is something like udefrag_get_avail_volumes()
 *       but without noise.
 */
short * __stdcall udefrag_validate_volume(unsigned char letter,int skip_removable)
{
	return format_message_w(i_udefrag_validate_volume(letter,skip_removable),vol_msg_buffer);
}

short * __stdcall scheduler_get_avail_letters(char *letters)
{
	return format_message_w(i_scheduler_get_avail_letters(letters),vol_msg_buffer);
}

short * __stdcall udefrag_set_options(ud_options *ud_opts)
{
	return format_message_w(i_udefrag_set_options(ud_opts),settings_msg_buffer);
}

/* important registry cleanup for uninstaller */
short * __stdcall udefrag_clean_registry(void)
{
	return format_message_w(i_udefrag_clean_registry(),settings_msg_buffer);
}

/* registry cleanup for native executable */
short * __stdcall udefrag_native_clean_registry(void)
{
	return format_message_w(i_udefrag_native_clean_registry(),settings_msg_buffer);
}
