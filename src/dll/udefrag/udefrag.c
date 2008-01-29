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
* driver interaction functions.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> /* for toupper() function for mingw */
#include "../../include/ntndk.h"
#include "../../include/udefrag.h"
#include "../../include/ultradfg.h"
#include "../zenwinx/src/zenwinx.h"

/* global variables */
char result_msg[4096]; /* buffer for the default formatted result message */
char user_mode_buffer[65536]; /* for nt 4.0 */
int native_mode_flag = FALSE;
HANDLE init_event = NULL;
HANDLE io_event = NULL; /* for write requests */
HANDLE io2_event = NULL; /* for statistical data requests */
HANDLE stop_event = NULL; /* for stop request */
HANDLE map_event = NULL; /* for cluster map request */
short driver_key[] = \
  L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ultradfg";
HANDLE udefrag_device_handle = NULL;

extern ud_options settings;

unsigned char c, lett;
BOOL done_flag;
char error_message[ERR_MSG_SIZE];
short error_message_w[ERR_MSG_SIZE];

/* internal functions prototypes */
BOOL udefrag_send_command(unsigned char command,unsigned char letter);
BOOL udefrag_send_command_ex(unsigned char command,unsigned char letter,STATUPDATEPROC sproc);
BOOL n_create_event(HANDLE *pHandle,short *name);
BOOL n_ioctl(HANDLE handle,HANDLE event,ULONG code,
			PVOID in_buf,ULONG in_size,
			PVOID out_buf,ULONG out_size,
			char *err_format_string);
int __stdcall udefrag_load_settings(int argc, short **argv);
int __stdcall udefrag_save_settings(void);
int __stdcall udefrag_unload(BOOL save_options);

#define NtCloseSafe(h) if(h) { NtClose(h); h = NULL; }

/* functions */
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	/* here we have last chance to unload the driver */
	if(dwReason == DLL_PROCESS_DETACH)
		if(udefrag_unload(FALSE) < 0) winx_pop_error(NULL,0);
	return 1;
}

void __stdcall udefrag_pop_error(char *buffer, int size)
{
	winx_pop_error(buffer,size);
}

void __stdcall udefrag_pop_werror(short *buffer, int size)
{
	winx_pop_werror(buffer,size);
}

int __stdcall udefrag_init(int argc, short **argv,int native_mode,long map_size)
{
	UNICODE_STRING uStr;
	HANDLE UserToken = NULL;
	NTSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	char buf[ERR_MSG_SIZE];

	/* 0. only one instance of the program ! */
	native_mode_flag = native_mode;
	/* 1. Enable neccessary privileges */
	Status = NtOpenProcessToken(NtCurrentProcess(),MAXIMUM_ALLOWED,&UserToken);
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't open process token: %x!",(UINT)Status);
		UserToken = NULL;
		goto init_fail;
	}
	if(winx_enable_privilege(UserToken,SE_SHUTDOWN_PRIVILEGE) < 0) goto init_fail;
	/*if(!EnablePrivilege(UserToken,SE_MANAGE_VOLUME_PRIVILEGE)) goto init_fail;*/
	if(winx_enable_privilege(UserToken,SE_LOAD_DRIVER_PRIVILEGE) < 0) goto init_fail;
	NtCloseSafe(UserToken);
	/* create init_event - this must be after privileges enabling */
	RtlInitUnicodeString(&uStr,L"\\udefrag_init");
	InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
	Status = NtCreateEvent(&init_event,STANDARD_RIGHTS_ALL | 0x1ff,
				&ObjectAttributes,SynchronizationEvent,TRUE);
	if(!NT_SUCCESS(Status)){
		if(Status == STATUS_OBJECT_NAME_COLLISION)
			winx_push_error("You can run only one instance of UltraDefrag!");
		else
			winx_push_error("Can't create init_event: %x!",(UINT)Status);
		init_event = NULL;
		goto init_fail;
	}
	/* 2. Load the driver */
	RtlInitUnicodeString(&uStr,driver_key);
	Status = NtLoadDriver(&uStr);
	if(!NT_SUCCESS(Status) && Status != STATUS_IMAGE_ALREADY_LOADED){
		winx_push_error("Can't load ultradfg driver: %x!",(UINT)Status);
		goto init_fail;
	}
	/* 3. Open our device */
	RtlInitUnicodeString(&uStr,L"\\Device\\UltraDefrag");
	InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
	Status = NtCreateFile(&udefrag_device_handle,
				FILE_GENERIC_READ | FILE_GENERIC_WRITE,
			    &ObjectAttributes,&IoStatusBlock,NULL,0,
			    FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,NULL,0);
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't access ULTRADFG driver: %x!",(UINT)Status);
		udefrag_device_handle = NULL;
		goto init_fail;
	}
	/* 4. Create events */
	if(!n_create_event(&io_event,L"\\udefrag_io")) goto init_fail;
	if(!n_create_event(&io2_event,L"\\udefrag_io2")) goto init_fail;
	if(!n_create_event(&stop_event,L"\\udefrag_stop")) goto init_fail;
	if(!n_create_event(&map_event,L"\\udefrag_map")) goto init_fail;
	/* 5. Set user mode buffer - nt 4.0 specific */
	if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_USER_MODE_BUFFER,
		user_mode_buffer,0,NULL,0,
		"Can't set user mode buffer: %x!")) goto init_fail;
	/* 6. Set cluster map size */
	if(!n_ioctl(udefrag_device_handle,io_event,IOCTL_SET_CLUSTER_MAP_SIZE,
		&map_size,sizeof(long),NULL,0,
		"Can't setup cluster map buffer: %x!")) goto init_fail;
	/* 7. Load settings */
	udefrag_load_settings(argc,argv);
	if(udefrag_set_options(&settings)) goto init_fail;
	return 0;
init_fail:
	NtCloseSafe(UserToken);
	if(init_event){
		/* save error message */
		winx_save_error(buf,ERR_MSG_SIZE);
		if(udefrag_unload(FALSE) < 0) winx_pop_error(NULL,0);
		/* restore error message */
		winx_restore_error(buf);
	}
	return (-1);
}

int __stdcall udefrag_unload(BOOL save_options)
{
	UNICODE_STRING uStr;

	/* unload only if init_event was created */
	if(!init_event){
		winx_push_error("Udefrag.dll unload without initialization!");
		goto unload_fail;
	}
	/* close events */
	NtClose(init_event); init_event = NULL;
	NtCloseSafe(io_event);
	NtCloseSafe(io2_event);
	NtCloseSafe(stop_event);
	NtCloseSafe(map_event);
	/* close device handle */
	NtCloseSafe(udefrag_device_handle);
	/* unload driver */
	RtlInitUnicodeString(&uStr,driver_key);
	NtUnloadDriver(&uStr);
	/* save settings */
	if(save_options)
		if(udefrag_save_settings() < 0) goto unload_fail;
	return 0;
unload_fail:
	return (-1);
}

/* you can send only one command at the same time */
DWORD WINAPI send_command(LPVOID unused)
{
	ANSI_STRING aStr;
	UNICODE_STRING uStr;
	char fmt_error[] = "Can't format message!";

	if(!udefrag_send_command(c,lett))
		winx_pop_werror(error_message_w,ERR_MSG_SIZE);
	else
		wcscpy(error_message_w,L"?");

	/* convert string to ansi */
	RtlInitUnicodeString(&uStr,error_message_w);
	aStr.Buffer = error_message;
	aStr.Length = 0;
	aStr.MaximumLength = ERR_MSG_SIZE - 1;
	if(RtlUnicodeStringToAnsiString(&aStr,&uStr,FALSE) != STATUS_SUCCESS)
		strcpy(error_message,fmt_error);
	done_flag = TRUE;
	if(winx_exit_thread() < 0) winx_pop_error(NULL,0);
	return 0;
}

BOOL udefrag_send_command_ex(unsigned char command,unsigned char letter,STATUPDATEPROC sproc)
{
	if(!sproc){
		/* send command directly and return */
		return udefrag_send_command(command,letter) ? TRUE : FALSE;
	}
	done_flag = FALSE; strcpy(error_message,"?"); wcscpy(error_message_w,L"?");
	/* create a thread for driver command processing */
	c = command; lett = letter;
	if(winx_create_thread(send_command,NULL) < 0)
		return FALSE;
	/*
	* Call specified callback 
	* every (settings.update_interval) milliseconds.
	*/
	do {
		winx_sleep(settings.update_interval);
		sproc(FALSE);
	} while(!done_flag);
	sproc(TRUE);
	return TRUE;
}

int __stdcall udefrag_analyse(unsigned char letter,STATUPDATEPROC sproc)
{
	return udefrag_send_command_ex('a',letter,sproc) ? 0 : (-1);
}

int __stdcall udefrag_defragment(unsigned char letter,STATUPDATEPROC sproc)
{
	return udefrag_send_command_ex('d',letter,sproc) ? 0 : (-1);
}

int __stdcall udefrag_optimize(unsigned char letter,STATUPDATEPROC sproc)
{
	return udefrag_send_command_ex('c',letter,sproc) ? 0 : (-1);
}

char * __stdcall udefrag_get_command_result(void)
{
	return error_message;
}

short * __stdcall udefrag_get_command_result_w(void)
{
	return error_message_w;
}

int __stdcall udefrag_stop(void)
{
	if(!init_event){
		winx_push_error("Udefrag.dll stop call without initialization!");
		return (-1);
	}
	return n_ioctl(udefrag_device_handle,stop_event,
		IOCTL_ULTRADEFRAG_STOP,NULL,0,NULL,0,
		"Can't stop driver command: %x!") ? 0 : (-1);
}

BOOL udefrag_send_command(unsigned char command,unsigned char letter)
{
	ULTRADFG_COMMAND cmd;
	LARGE_INTEGER offset;
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;

	if(!init_event){
		winx_push_error("Udefrag.dll \'%c\' call without initialization!",command);
		return FALSE;
	}
	cmd.command = command;
	cmd.letter = letter;
	cmd.sizelimit = settings.sizelimit;
	offset.QuadPart = 0;
	Status = NtWriteFile(udefrag_device_handle,/*NULL*/io_event,
		NULL,NULL,&IoStatusBlock,
		&cmd,sizeof(ULTRADFG_COMMAND),&offset,0);
#if 0
	if(Status == STATUS_PENDING){
		Status = NtWaitForSingleObject(udefrag_device_handle/*io_event*/,FALSE,NULL);
		if(NT_SUCCESS(Status)) Status = IoStatusBlock.Status;
	}
#endif
	if(!NT_SUCCESS(Status) || Status == STATUS_PENDING){
		winx_push_error("Can't execute driver command \'%c\' for volume %c: %x!",
			command,letter,(UINT)Status);
		return FALSE;
	}
	return TRUE;
}

int __stdcall udefrag_get_progress(STATISTIC *pstat, double *percentage)
{
	if(!init_event){
		winx_push_error("Udefrag.dll get_progress call without initialization!");
		goto get_progress_fail;
	}
	if(!n_ioctl(udefrag_device_handle,io2_event,IOCTL_GET_STATISTIC,
		NULL,0,pstat,sizeof(STATISTIC),
		"Statistical data unavailable: %x!")) goto get_progress_fail;
	if(percentage){ /* calculate percentage only if we have such request */
		switch(pstat->current_operation){
		case 'A':
			*percentage = (double)(LONGLONG)pstat->processed_clusters *
					(double)(LONGLONG)pstat->bytes_per_cluster / 
					((double)(LONGLONG)pstat->total_space + 0.1);
			break;
		case 'D':
			if(pstat->clusters_to_move_initial == 0)
				*percentage = 1.00;
			else
				*percentage = 1.00 - (double)(LONGLONG)pstat->clusters_to_move / 
						((double)(LONGLONG)pstat->clusters_to_move_initial + 0.1);
			break;
		case 'C':
			if(pstat->clusters_to_compact_initial == 0)
				*percentage = 1.00;
			else
				*percentage = 1.00 - (double)(LONGLONG)pstat->clusters_to_compact / 
						((double)(LONGLONG)pstat->clusters_to_compact_initial + 0.1);
		}
		*percentage = (*percentage) * 100.00;
	}
	return 0;
get_progress_fail:
	return (-1);
}

int __stdcall udefrag_get_map(char *buffer,int size)
{
	if(!init_event){
		winx_push_error("Udefrag.dll get_map call without initialization!");
		return (-1);
	}
	return n_ioctl(udefrag_device_handle,map_event,IOCTL_GET_CLUSTER_MAP,
		NULL,0,buffer,(ULONG)size,"Cluster map unavailable: %x!") ? 0 : (-1);
}

/* useful for native and console applications */
char * __stdcall udefrag_get_default_formatted_results(STATISTIC *pstat)
{
	char s[68];
	double p;
	unsigned int ip;

	strcpy(result_msg,"Volume information:\n");
	strcat(result_msg,"\n  Volume size                  = ");
	fbsize(s,pstat->total_space);
	strcat(result_msg,s);
	strcat(result_msg,"\n  Free space                   = ");
	fbsize(s,pstat->free_space);
	strcat(result_msg,s);
	strcat(result_msg,"\n\n  Total number of files        = ");
	_itoa(pstat->filecounter,s,10);
	strcat(result_msg,s);
	strcat(result_msg,"\n  Number of fragmented files   = ");
	_itoa(pstat->fragmfilecounter,s,10);
	strcat(result_msg,s);
	p = (double)(pstat->fragmcounter)/((double)(pstat->filecounter) + 0.1);
	ip = (unsigned int)(p * 100.00);
	strcat(result_msg,"\n  Fragments per file           = ");
	sprintf(s,"%u.%02u",ip / 100,ip % 100);
	strcat(result_msg,s);
	return result_msg;
}

BOOL n_create_event(HANDLE *pHandle,short *name)
{
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS Status;

	RtlInitUnicodeString(&uStr,name);
	InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
	Status = NtCreateEvent(pHandle,STANDARD_RIGHTS_ALL | 0x1ff,
				&ObjectAttributes,SynchronizationEvent,FALSE);
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't create %ls event: %x!",name,(UINT)Status);
		*pHandle = NULL;
		return FALSE;
	}
	return TRUE;
}

BOOL n_ioctl(HANDLE handle,HANDLE event,ULONG code,
			PVOID in_buf,ULONG in_size,
			PVOID out_buf,ULONG out_size,
			char *err_format_string)
{
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	Status = NtDeviceIoControlFile(handle,event,
				NULL,NULL,&IoStatusBlock,code,
				in_buf,in_size,out_buf,out_size);
/*	if(Status == STATUS_PENDING)
	{
		Status = NtWaitForSingleObject(event,FALSE,NULL);
		if(NT_SUCCESS(Status)) Status = IoStatusBlock.Status;
	}
*/	if(!NT_SUCCESS(Status) || Status == STATUS_PENDING){
		winx_push_error(err_format_string,(UINT)Status);
		return FALSE;
	}
	return TRUE;
}
