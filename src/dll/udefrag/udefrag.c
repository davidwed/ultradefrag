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
 *  functions to interact with driver.
 */

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include "../../include/ntndk.h"
#include "../../include/udefrag.h"
#include "../../include/ultradfg.h"

/* global variables */
char msg[1024]; /* buffer for error messages */
char user_mode_buffer[65536]; /* for nt 4.0 */
int __native_mode = FALSE;
HANDLE init_event = NULL;
HANDLE io_event = NULL; /* for write requests */
HANDLE io2_event = NULL; /* for statistical data requests */
HANDLE stop_event = NULL; /* for stop request */
short driver_key[] = \
  L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ultradfg";
HANDLE udefrag_device_handle = NULL;

extern ud_settings settings;

/* internal functions prototypes */
BOOL udefrag_send_command(unsigned char command,unsigned char letter);
BOOL EnablePrivilege(HANDLE hToken,DWORD dwLowPartOfLUID);
BOOL __CreateEvent(HANDLE *pHandle,short *name);
BOOL _ioctl(HANDLE handle,HANDLE event,ULONG code,
			PVOID in_buf,ULONG in_size,
			PVOID out_buf,ULONG out_size,
			char *err_format_string);

/* inline functions */
#define SafeNtClose(h) if(h) { NtClose(h); h = NULL; }

/* functions */
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	return 1;
}

char * __cdecl udefrag_init(int native_mode)
{
	UNICODE_STRING uStr;
	HANDLE UserToken = NULL;
	NTSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;

	/* 0. only one instance of program ! */
	__native_mode = native_mode;
	RtlInitUnicodeString(&uStr,L"\\udefrag_init");
	InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
	Status = NtCreateEvent(&init_event,STANDARD_RIGHTS_ALL | 0x1ff,
				&ObjectAttributes,SynchronizationEvent,TRUE);
	if(!NT_SUCCESS(Status))
	{
		if(Status == STATUS_OBJECT_NAME_COLLISION)
			strcpy(msg,"You can run only one instance of UltraDefrag!");
		else
			sprintf(msg,"Can't create init_event: %x!",Status);
		goto init_fail;
	}
	/* 1. Enable neccessary privileges */
	Status = NtOpenProcessToken(NtCurrentProcess(),MAXIMUM_ALLOWED,&UserToken);
	if(!NT_SUCCESS(Status))
	{
		sprintf(msg,"Can't open process token: %x!",Status);
		goto init_fail;
	}
	if(!EnablePrivilege(UserToken,SE_SHUTDOWN_PRIVILEGE)) goto init_fail;
	/*if(!EnablePrivilege(UserToken,SE_MANAGE_VOLUME_PRIVILEGE)) goto init_fail;*/
	if(!EnablePrivilege(UserToken,SE_LOAD_DRIVER_PRIVILEGE)) goto init_fail;
	SafeNtClose(UserToken);
	/* 2. Load driver */
	RtlInitUnicodeString(&uStr,driver_key);
	Status = NtLoadDriver(&uStr);
	if(!NT_SUCCESS(Status) && Status != STATUS_IMAGE_ALREADY_LOADED)
	{
		sprintf(msg,"Can't load ultradfg driver: %x!",Status);
		goto init_fail;
	}
	/* 3. Open device */
	RtlInitUnicodeString(&uStr,L"\\Device\\UltraDefrag");
	InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
	Status = NtCreateFile(&udefrag_device_handle,
				FILE_GENERIC_READ | FILE_GENERIC_WRITE,
			    &ObjectAttributes,&IoStatusBlock,NULL,0,
			    FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,NULL,0);
	if(!NT_SUCCESS(Status))
	{
		sprintf(msg,"Can't access ULTRADFG driver: %x!",Status);
		goto init_fail;
	}
	/* 4. Create events */
	if(!__CreateEvent(&io_event,L"\\udefrag_io")) goto init_fail;
	if(!__CreateEvent(&io2_event,L"\\udefrag_io2")) goto init_fail;
	if(!__CreateEvent(&stop_event,L"\\udefrag_stop")) goto init_fail;
	/* 5. Set user mode buffer - nt 4.0 specific */
	if(!_ioctl(udefrag_device_handle,io_event,IOCTL_SET_USER_MODE_BUFFER,
		user_mode_buffer,0,NULL,0,
		"Can't set user mode buffer: %x!")) goto init_fail;
	return NULL;
init_fail:
	SafeNtClose(UserToken);
	return msg;
}

/* temporary proc. */
HANDLE get_device_handle()
{
	return udefrag_device_handle;
}

char * __cdecl udefrag_unload(void)
{
	UNICODE_STRING uStr;

	/* unload only if init_event was created */
	if(!init_event)
	{
		strcpy(msg,"Udefrag.dll unload without initialization!");
		goto unload_fail;
	}
	/* close events */
	NtClose(init_event); init_event = NULL;
	SafeNtClose(io_event);
	SafeNtClose(io2_event);
	SafeNtClose(stop_event);
	/* close device handle */
	SafeNtClose(udefrag_device_handle);
	/* unload driver */
	RtlInitUnicodeString(&uStr,driver_key);
	NtUnloadDriver(&uStr);
	return NULL;
unload_fail:
	return msg;
}

char * __cdecl udefrag_analyse(unsigned char letter)
{
	return udefrag_send_command('a',letter) ? NULL : msg;
}

char * __cdecl udefrag_defragment(unsigned char letter)
{
	return udefrag_send_command('d',letter) ? NULL : msg;
}

char * __cdecl udefrag_optimize(unsigned char letter)
{
	return udefrag_send_command('c',letter) ? NULL : msg;
}

char * __cdecl udefrag_stop(void)
{
	if(!init_event)
	{
		strcpy(msg,"Udefrag.dll stop call without initialization!");
		return msg;
	}
	return _ioctl(udefrag_device_handle,stop_event,
		IOCTL_ULTRADEFRAG_STOP,NULL,0,NULL,0,
		"Can't stop driver command: %x!") ? NULL : msg;
}

BOOL udefrag_send_command(unsigned char command,unsigned char letter)
{
	ULTRADFG_COMMAND cmd;
	LARGE_INTEGER offset;
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;

	if(!init_event)
	{
		sprintf(msg,"Udefrag.dll \'%c\' call without initialization!",command);
		return FALSE;
	}
	cmd.command = command;
	cmd.letter = letter;
	cmd.sizelimit = settings.sizelimit;
	cmd.mode = __UserMode;
	offset.QuadPart = 0;
	Status = NtWriteFile(udefrag_device_handle,io_event,
		NULL,NULL,&IoStatusBlock,
		&cmd,sizeof(ULTRADFG_COMMAND),&offset,0);
	if(Status == STATUS_PENDING)
	{
		Status = NtWaitForSingleObject(io_event,FALSE,NULL);
		if(!Status)	Status = IoStatusBlock.Status;
	}
	if(!NT_SUCCESS(Status))
	{
		sprintf(msg,"Can't execute driver command %c for volume %c: %x!",
			command,letter,Status);
		return FALSE;
	}
	return TRUE;
}

char * __cdecl udefrag_get_progress(STATISTIC *pstat, double *percentage)
{
	if(!init_event)
	{
		strcpy(msg,"Udefrag.dll get_progress call without initialization!");
		goto get_progress_fail;
	}
	if(!_ioctl(udefrag_device_handle,io2_event,IOCTL_GET_STATISTIC,
		NULL,0,pstat,sizeof(STATISTIC),
		"Statistical data unavailable: %x!")) goto get_progress_fail;
	if(percentage) /* calculate percentage only if we have such request */
	{
		switch(pstat->current_operation)
		{
		case 'A':
			*percentage = (double)(LONGLONG)pstat->processed_clusters *
					(double)(LONGLONG)pstat->bytes_per_cluster / 
					(double)(LONGLONG)pstat->total_space;
			break;
		case 'D':
			if(pstat->clusters_to_move_initial == 0)
				*percentage = 1.00;
			else
				*percentage = 1.00 - (double)(LONGLONG)pstat->clusters_to_move / 
						(double)(LONGLONG)pstat->clusters_to_move_initial;
			break;
		case 'C':
			if(pstat->clusters_to_compact_initial == 0)
				*percentage = 1.00;
			else
				*percentage = 1.00 - (double)(LONGLONG)pstat->clusters_to_compact / 
						(double)(LONGLONG)pstat->clusters_to_compact_initial;
		}
		*percentage = (*percentage) * 100.00;
	}
	return NULL;
get_progress_fail:
	return msg;
}

BOOL EnablePrivilege(HANDLE hToken,DWORD dwLowPartOfLUID)
{
	NTSTATUS Status;
	TOKEN_PRIVILEGES tp;
	LUID luid;

	luid.HighPart = 0x0;
	luid.LowPart = dwLowPartOfLUID;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	Status = NtAdjustPrivilegesToken(hToken,FALSE,&tp,sizeof(TOKEN_PRIVILEGES),
									(PTOKEN_PRIVILEGES)NULL,(PDWORD)NULL);
	if(Status == STATUS_NOT_ALL_ASSIGNED || !NT_SUCCESS(Status))
	{
		sprintf(msg,"Can't enable privilege: %x : %x!",dwLowPartOfLUID,Status);
		return FALSE;
	}
	return TRUE;
}

BOOL __CreateEvent(HANDLE *pHandle,short *name)
{
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS Status;

	RtlInitUnicodeString(&uStr,name);
	InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
	Status = NtCreateEvent(pHandle,STANDARD_RIGHTS_ALL | 0x1ff,
				&ObjectAttributes,SynchronizationEvent,FALSE);
	if(!NT_SUCCESS(Status))
	{
		sprintf(msg,"Can't create %ws event: %x!",name,Status);
		return FALSE;
	}
	return TRUE;
}

BOOL _ioctl(HANDLE handle,HANDLE event,ULONG code,
			PVOID in_buf,ULONG in_size,
			PVOID out_buf,ULONG out_size,
			char *err_format_string)
{
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	Status = NtDeviceIoControlFile(handle,event,
				NULL,NULL,&IoStatusBlock,code,
				in_buf,in_size,out_buf,out_size);
	if(Status == STATUS_PENDING)
	{
		Status = NtWaitForSingleObject(event,FALSE,NULL);
		if(!Status)	Status = IoStatusBlock.Status;
	}
	if(!NT_SUCCESS(Status))
	{
		sprintf(msg,err_format_string,Status);
		return FALSE;
	}
	return TRUE;
}

int __cdecl fbsize(char *s,ULONGLONG n)
{
	char symbols[] = {'K','M','G','T','P','E'};
	double fn;
	unsigned int i;
	unsigned int in;

	/* convert n to Kb - enough for ~8 mln. Tb volumes */
	fn = (double)(signed __int64)n / 1024.00;
	for(i = 0; fn >= 1024.00; i++) fn /= 1024.00;
	if(i > sizeof(symbols) - 1) i = sizeof(symbols) - 1;
	/* %.2f don't work in native mode */
	in = (unsigned int)(fn * 100.00);
	return sprintf(s,"%u.%02u %cb",in / 100,in % 100,symbols[i]);
}
