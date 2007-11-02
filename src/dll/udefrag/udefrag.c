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
#include <stdlib.h>
#include "../../include/ntndk.h"
#include "../../include/udefrag.h"
#include "../../include/ultradfg.h"

#define MAX_DOS_DRIVES 26
#define FS_ATTRIBUTE_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_ATTRIBUTE_INFORMATION))
#define INTERNAL_SEM_FAILCRITICALERRORS 0

/* global variables */
char msg[1024]; /* buffer for error messages */
char result_msg[4096]; /* buffer for the default formatted result message */
char user_mode_buffer[65536]; /* for nt 4.0 */
int __native_mode = FALSE;
HANDLE init_event = NULL;
HANDLE io_event = NULL; /* for write requests */
HANDLE io2_event = NULL; /* for statistical data requests */
HANDLE stop_event = NULL; /* for stop request */
HANDLE map_event = NULL; /* for cluster map request */
short driver_key[] = \
  L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ultradfg";
HANDLE udefrag_device_handle = NULL;

extern ud_settings settings;

volume_info v[MAX_DOS_DRIVES + 1];

BOOL nt4_system = FALSE; /* TRUE for nt 4.0 */

/* internal functions prototypes */
BOOL udefrag_send_command(unsigned char command,unsigned char letter);
BOOL get_drive_map(PROCESS_DEVICEMAP_INFORMATION *pProcessDeviceMapInfo);
BOOL internal_open_rootdir(unsigned char letter,HANDLE *phFile);
BOOL internal_validate_volume(unsigned char letter,int skip_removable,
							  char *fsname,
							  PROCESS_DEVICEMAP_INFORMATION *pProcessDeviceMapInfo,
							  FILE_FS_SIZE_INFORMATION *pFileFsSize);
BOOL EnablePrivilege(HANDLE hToken,DWORD dwLowPartOfLUID);
BOOL __CreateEvent(HANDLE *pHandle,short *name);
BOOL _ioctl(HANDLE handle,HANDLE event,ULONG code,
			PVOID in_buf,ULONG in_size,
			PVOID out_buf,ULONG out_size,
			char *err_format_string);
BOOL set_error_mode(UINT uMode);

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
	if(!__CreateEvent(&map_event,L"\\udefrag_map")) goto init_fail;
	/* 5. Set user mode buffer - nt 4.0 specific */
	if(!_ioctl(udefrag_device_handle,io_event,IOCTL_SET_USER_MODE_BUFFER,
		user_mode_buffer,0,NULL,0,
		"Can't set user mode buffer: %x!")) goto init_fail;
	return NULL;
init_fail:
	SafeNtClose(UserToken);
	if(init_event) udefrag_unload();
	return msg;
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
	SafeNtClose(map_event);
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
		if(NT_SUCCESS(Status)) Status = IoStatusBlock.Status;
	}
	if(!NT_SUCCESS(Status))
	{
		sprintf(msg,"Can't execute driver command \'%c\' for volume %c: %x!",
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

char * __cdecl udefrag_get_map(char *buffer,int size)
{
	if(!init_event)
	{
		strcpy(msg,"Udefrag.dll get_map call without initialization!");
		return msg;
	}
	return _ioctl(udefrag_device_handle,map_event,IOCTL_GET_CLUSTER_MAP,
		NULL,0,buffer,(ULONG)size,"Cluster map unavailable: %x!") ? NULL : msg;
}

/* useful for native and console applications */
char * __cdecl get_default_formatted_results(STATISTIC *pstat)
{
	char s[64];
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
	p = (double)(pstat->fragmcounter)/(double)(pstat->filecounter);
	ip = (unsigned int)(p * 100.00);
	strcat(result_msg,"\n  Fragments per file           = ");
	sprintf(s,"%u.%02u",ip / 100,ip % 100);
	strcat(result_msg,s);
	return result_msg;
}

/* NOTE: if(skip_removable == FALSE && you have floppy drive without floppy disk)
 *       then you will hear noise :))
 */
char * __cdecl udefrag_get_avail_volumes(volume_info **vol_info,int skip_removable)
{
	PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;
	ULONG drive_map;
	ULONG i, index;
	char letter;
	FILE_FS_SIZE_INFORMATION FileFsSize;

	/* get full list of volumes */
	*vol_info = v;
	if(!nt4_system)
		if(!get_drive_map(&ProcessDeviceMapInfo)) goto get_volumes_fail;
	/* set error mode to ignore missing removable drives */
	if(!set_error_mode(INTERNAL_SEM_FAILCRITICALERRORS)) goto get_volumes_fail;
	drive_map = ProcessDeviceMapInfo.Query.DriveMap;
	index = 0;
	for(i = 0; i < MAX_DOS_DRIVES; i++)
	{
		if((drive_map & (1 << i)) || nt4_system)
		{
			letter = 'A' + (char)i;
			if(!internal_validate_volume(letter,skip_removable,v[index].fsname,
				&ProcessDeviceMapInfo,&FileFsSize)) continue;
			v[index].total_space.QuadPart = FileFsSize.TotalAllocationUnits.QuadPart * \
				FileFsSize.SectorsPerAllocationUnit * FileFsSize.BytesPerSector;
			v[index].free_space.QuadPart = FileFsSize.AvailableAllocationUnits.QuadPart * \
				FileFsSize.SectorsPerAllocationUnit * FileFsSize.BytesPerSector;
			v[index].letter = letter;
			index ++;
		}
	}
	v[index].letter = 0;
	/* try to restore error mode to default state */
	set_error_mode(1); /* equal to SetErrorMode(0) */
	return NULL;
get_volumes_fail:
	return msg;
}

/* NOTE: this is something like udefrag_get_avail_volumes()
 *       but without noise.
 */
char * __cdecl udefrag_validate_volume(unsigned char letter,int skip_removable)
{
	PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;
	FILE_FS_SIZE_INFORMATION FileFsSize;

	letter = (unsigned char)toupper((int)letter);
	if(letter < 'A' || letter > 'Z')
	{
		sprintf(msg,"Invalid volume letter %c!",letter);
		goto validate_fail;
	}
	if(!nt4_system)
		if(!get_drive_map(&ProcessDeviceMapInfo)) goto validate_fail;
	/* set error mode to ignore missing removable drives */
	if(!set_error_mode(INTERNAL_SEM_FAILCRITICALERRORS)) goto validate_fail;
	if(!internal_validate_volume(letter,skip_removable,NULL,
		&ProcessDeviceMapInfo,&FileFsSize)) goto validate_fail;
	/* try to restore error mode to default state */
	set_error_mode(1); /* equal to SetErrorMode(0) */
	return NULL;
validate_fail:
	return msg;
}

BOOL get_drive_map(PROCESS_DEVICEMAP_INFORMATION *pProcessDeviceMapInfo)
{
	NTSTATUS Status;

	/* this call is applicable only for w2k and later versions */
	Status = NtQueryInformationProcess(NtCurrentProcess(),
					ProcessDeviceMap,
					pProcessDeviceMapInfo,
					sizeof(PROCESS_DEVICEMAP_INFORMATION),
					NULL);
	if(!NT_SUCCESS(Status))
	{
		if(Status == STATUS_INVALID_INFO_CLASS)
			nt4_system = TRUE;
		else
		{
			sprintf(msg,"Can't get drive map: %x!",Status);
			return FALSE;
		}
	}
	return TRUE;
}

BOOL internal_open_rootdir(unsigned char letter,HANDLE *phFile)
{
	NTSTATUS Status;
	short rootpath[] = L"\\??\\A:\\";
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;

	rootpath[4] = (short)letter;
	RtlInitUnicodeString(&uStr,rootpath);
	InitializeObjectAttributes(&ObjectAttributes,&uStr,
				   FILE_READ_ATTRIBUTES,NULL,NULL);
	Status = NtCreateFile(phFile,FILE_GENERIC_READ,
				&ObjectAttributes,&IoStatusBlock,NULL,0,
				FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,
				NULL,0);
	if(!NT_SUCCESS(Status))
	{
		sprintf(msg,"Can't open %ws: %x!",rootpath,Status);
		return FALSE;
	}
	return TRUE;
}

/* NOTE: letter must be between 'A' and 'Z' */
BOOL internal_validate_volume(unsigned char letter,int skip_removable,
							  char *fsname,
							  PROCESS_DEVICEMAP_INFORMATION *pProcessDeviceMapInfo,
							  FILE_FS_SIZE_INFORMATION *pFileFsSize)
{
	NTSTATUS Status;
	UINT type;
	HANDLE hFile;
	HANDLE hLink;
	short link_name[] = L"\\??\\A:";
	#define MAX_TARGET_LENGTH 256
	short link_target[MAX_TARGET_LENGTH];
	ULONG size;
	UNICODE_STRING uStr;
	ANSI_STRING aStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_FS_DEVICE_INFORMATION FileFsDevice;
	PFILE_FS_ATTRIBUTE_INFORMATION pFileFsAttribute;
	UCHAR buffer[FS_ATTRIBUTE_BUFFER_SIZE];
	short str[MAXFSNAME];
	ULONG length;

	if(nt4_system)
	{
		/* let us assume that A and B volumes are located on floppies 
		 * to reduce noise */
		if(letter == 'A' || letter == 'B')
		{
			type = DRIVE_REMOVABLE;
			goto validate_type;
		}
		if(!internal_open_rootdir(letter,&hFile)) goto invalid_volume;
		Status = NtQueryVolumeInformationFile(hFile,&IoStatusBlock,
						&FileFsDevice,sizeof(FILE_FS_DEVICE_INFORMATION),
						FileFsDeviceInformation);
		NtClose(hFile);
		if(!NT_SUCCESS(Status))
		{
			sprintf(msg,"Can't get volume type for \'%c\': %x!",letter,Status);
			goto invalid_volume;
		}
		switch(FileFsDevice.DeviceType)
		{
		case FILE_DEVICE_CD_ROM:
		case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
			type = DRIVE_CDROM;
			break;
		case FILE_DEVICE_VIRTUAL_DISK:
			type = DRIVE_RAMDISK;
			break;
		case FILE_DEVICE_NETWORK_FILE_SYSTEM:
			type = DRIVE_REMOTE;
			break;
		case FILE_DEVICE_DISK:
		case FILE_DEVICE_DISK_FILE_SYSTEM:
			if(FileFsDevice.Characteristics & FILE_REMOTE_DEVICE)
			{
				type = DRIVE_REMOTE;
				break;
			}
			if(FileFsDevice.Characteristics & FILE_REMOVABLE_MEDIA)
				type = DRIVE_REMOVABLE;
			else
				type = DRIVE_FIXED;
			break;
		default:
			type = DRIVE_UNKNOWN;
		}
	}
	else
	{
		type = (UINT)pProcessDeviceMapInfo->Query.DriveType[letter - 'A'];
	}
validate_type:
	/* exclude remote and read-only volumes */
	if(type != DRIVE_FIXED && type != DRIVE_REMOVABLE && type != DRIVE_RAMDISK)
	{
		sprintf(msg,"Volume must be on non-cdrom local drive, but it's %u!",type);
		goto invalid_volume;
	}
	/* exclude removable media if requested */
	if(type == DRIVE_REMOVABLE && skip_removable)
	{
		strcpy(msg,"It's removable volume!");
		goto invalid_volume;
	}
	/* exclude letters assigned by 'subst' command - 
	   on w2k and later systems such drives have type DRIVE_NO_ROOT_DIR, but on nt4.0 ... */
	if(nt4_system)
	{
		link_name[4] = (short)letter;
		RtlInitUnicodeString(&uStr,link_name);
		InitializeObjectAttributes(&ObjectAttributes,&uStr,
			OBJ_CASE_INSENSITIVE,NULL,NULL);
		Status = NtOpenSymbolicLinkObject(&hLink,SYMBOLIC_LINK_QUERY,
			&ObjectAttributes);
		if(!NT_SUCCESS(Status))
		{
			sprintf(msg,"Can't open symbolic link %ws: %x!",link_name,Status);
			goto invalid_volume;
		}
		uStr.Buffer = link_target;
		uStr.Length = 0;
		uStr.MaximumLength = MAX_TARGET_LENGTH * sizeof(short);
		size = 0;
		Status = NtQuerySymbolicLinkObject(hLink,&uStr,&size);
		NtClose(hLink);
		if(!NT_SUCCESS(Status))
		{
			sprintf(msg,"Can't query symbolic link %ws: %x!",link_name,Status);
			goto invalid_volume;
		}
		link_target[4] = 0; /* terminate the buffer */
		if(!wcscmp(link_target,L"\\??\\"))
		{
			strcpy(msg,"Volume letter is assigned by \'subst\' command!");
			goto invalid_volume;
		}
	}
	/* get volume information */
	if(!internal_open_rootdir(letter,&hFile)) goto invalid_volume;
	Status = NtQueryVolumeInformationFile(hFile,&IoStatusBlock,pFileFsSize,
				sizeof(FILE_FS_SIZE_INFORMATION),FileFsSizeInformation);
	if(!NT_SUCCESS(Status))
	{
		NtClose(hFile);
		sprintf(msg,"Can't get size of volume \'%c\': %x!",letter,Status);
		goto invalid_volume;
	}
	if(fsname)
	{
		fsname[0] = 0;
		pFileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)buffer;
		Status = NtQueryVolumeInformationFile(hFile,&IoStatusBlock,pFileFsAttribute,
					FS_ATTRIBUTE_BUFFER_SIZE,FileFsAttributeInformation);
		if(!NT_SUCCESS(Status))
		{
			NtClose(hFile);
			sprintf(msg,"Can't get file system name for \'%c\': %x!",letter,Status);
			goto invalid_volume;
		}
		memcpy(str,pFileFsAttribute->FileSystemName,(MAXFSNAME - 1) * sizeof(short));
		length = pFileFsAttribute->FileSystemNameLength >> 1;
		if(length < MAXFSNAME) str[length] = 0;
		str[MAXFSNAME - 1] = 0;
		RtlInitUnicodeString(&uStr,str);
		aStr.Buffer = fsname;
		aStr.Length = 0;
		aStr.MaximumLength = MAXFSNAME - 1;
		if(RtlUnicodeStringToAnsiString(&aStr,&uStr,FALSE) != STATUS_SUCCESS)
			strcpy(fsname,"????");
	}
	NtClose(hFile);
	return TRUE;
invalid_volume:
	return FALSE;
}

char * __stdcall scheduler_get_avail_letters(char *letters)
{
	volume_info *v;
	int i;

	if(udefrag_get_avail_volumes(&v,TRUE)) return msg;
	for(i = 0;;i++)
	{
		letters[i] = v[i].letter;
		if(v[i].letter == 0) break;
	}
	return NULL;
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
		if(NT_SUCCESS(Status)) Status = IoStatusBlock.Status;
	}
	if(!NT_SUCCESS(Status))
	{
		sprintf(msg,err_format_string,Status);
		return FALSE;
	}
	return TRUE;
}

BOOL set_error_mode(UINT uMode)
{
	NTSTATUS Status;

	Status = NtSetInformationProcess(NtCurrentProcess(),
					ProcessDefaultHardErrorMode,
					(PVOID)&uMode,
					sizeof(uMode));
	if(!NT_SUCCESS(Status))
	{
		sprintf(msg,"Can't set error mode %u: %x!",uMode,Status);
		return FALSE;
	}
	return TRUE;
}

void __cdecl nsleep(int msec)
{
	LARGE_INTEGER Interval;

	if(msec != INFINITE)
	{
		/* System time units are 100 nanoseconds. */
		Interval.QuadPart = -((signed long)msec * 10000);
	}
	else
	{
		/* Approximately 292000 years hence */
		Interval.QuadPart = -0x7FFFFFFFFFFFFFFF;
	}
	NtDelayExecution(FALSE,&Interval);
}

/* original name was StrFormatByteSize */
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

/* fbsize variant for other purposes */
int __cdecl fbsize2(char *s,ULONGLONG n)
{
	char symbols[] = {'K','M','G','T','P','E'};
	unsigned int i;

	if(n < 1024) return sprintf(s,"%u",(int)n);
	n /= 1024;
	for(i = 0; n >= 1024; i++) n /= 1024;
	if(i > sizeof(symbols) - 1) i = sizeof(symbols) - 1;
	return sprintf(s,"%u %cb",(int)n,symbols[i]);
}

/* decode fbsize2 formatted string */
int __cdecl dfbsize2(char *s,ULONGLONG *pn)
{
	char symbols[] = {'K','M','G','T','P','E'};
	char t[64];
	signed int i;
	ULONGLONG m = 1;

	if(strlen(s) > 63) return (-1);
	strcpy(t,s);
	_strupr(t);
	for(i = 0; i < sizeof(symbols); i++)
		if(strchr(t,symbols[i])) break;
	if(i < sizeof(symbols)) /* suffix found */
		for(; i >= 0; i--) m *= 1024;
	*pn = m * _atoi64(s);
	return 0;
}