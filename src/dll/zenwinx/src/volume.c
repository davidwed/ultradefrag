/*
 *  ZenWINX - WIndows Native eXtended library.
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
* zenwinx.dll functions to get information about disk volumes.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#include <ctype.h>

#include "ntndk.h"
#include "zenwinx.h"

BOOLEAN internal_open_rootdir(unsigned char letter,HANDLE *phFile);

/****f* zenwinx.volume/winx_get_drive_type
* NAME
*    winx_get_drive_type
* SYNOPSIS
*    type = winx_get_drive_type(letter);
* FUNCTION
*    Win32 GetDriveType() native analog.
* INPUTS
*    letter - volume letter
* RESULT
*    If the function fails, the return value is (-1).
*    Otherwise it returns the same values 
*    that GetDriveType() returns.
* EXAMPLE
*    type = winx_get_drive_type('C');
*    if(type < 0){
*        winx_pop_error(buffer,sizeof(buffer));
*        // handle error here
*    }
* SEE ALSO
******/
int __stdcall winx_get_drive_type(char letter)
{
	PROCESS_DEVICEMAP_INFORMATION pdi;
	NTSTATUS Status;
	int nt4_system = FALSE;
	UNICODE_STRING uStr;
	HANDLE hFile;
	HANDLE hLink;
	short link_name[] = L"\\??\\A:";
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK iosb;
	FILE_FS_DEVICE_INFORMATION ffdi;
	#define MAX_TARGET_LENGTH 256
	short link_target[MAX_TARGET_LENGTH];
	ULONG size;

	letter = (unsigned char)toupper((int)letter);
	if(letter < 'A' || letter > 'Z'){
		winx_push_error("Invalid letter %c!",letter);
		return -1;
	}
	/* this call is applicable only for w2k and later versions */
	Status = NtQueryInformationProcess(NtCurrentProcess(),
					ProcessDeviceMap,
					&pdi,sizeof(PROCESS_DEVICEMAP_INFORMATION),
					NULL);
	if(!NT_SUCCESS(Status)){
		if(Status != STATUS_INVALID_INFO_CLASS){
			winx_push_error("Can't get drive map: %x!",(UINT)Status);
			return -1;
		}
		nt4_system = TRUE;
	}
	/*
	* Type DRIVE_NO_ROOT_DIR have following drives:
	* 1. assigned by subst command
	* 2. SCSI external drives
	* 3. RAID volumes
	* And we need additional check to know more about
	* volume type.
	*/
	link_name[4] = (short)letter;
	RtlInitUnicodeString(&uStr,link_name);
	InitializeObjectAttributes(&ObjectAttributes,&uStr,
		OBJ_CASE_INSENSITIVE,NULL,NULL);
	Status = NtOpenSymbolicLinkObject(&hLink,SYMBOLIC_LINK_QUERY,
		&ObjectAttributes);
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't open symbolic link %ls: %x!",
			link_name,(UINT)Status);
		return -1;
	}
	uStr.Buffer = link_target;
	uStr.Length = 0;
	uStr.MaximumLength = MAX_TARGET_LENGTH * sizeof(short);
	size = 0;
	Status = NtQuerySymbolicLinkObject(hLink,&uStr,&size);
	NtClose(hLink);
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't query symbolic link %ls: %x!",
			link_name,(UINT)Status);
		return -1;
	}
	link_target[MAX_TARGET_LENGTH - 1] = 0; /* terminate the buffer */
	if(wcsstr(link_target,L"\\??\\") == (wchar_t *)link_target)
		return DRIVE_ASSIGNED_BY_SUBST_COMMAND;
	/*
	* Floppies have "Floppy" substring in their names.
	*/
	if(wcsstr(link_target,L"Floppy"))
		return DRIVE_REMOVABLE;
	/* Now we have fine knowledge, but only on w2k and later systems. */
	if(!nt4_system)	return (int)pdi.Query.DriveType[letter - 'A'];
	/*
	* NT 4.0 "feature":
	* To exclude other unwanted drives we need to query their types.
	* Note that the drive motor can be powered on during this check.
	*/
	if(!internal_open_rootdir(letter,&hFile))
		return -1;
	Status = NtQueryVolumeInformationFile(hFile,&iosb,
					&ffdi,sizeof(FILE_FS_DEVICE_INFORMATION),
					FileFsDeviceInformation);
	NtClose(hFile);
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't get volume type for \'%c\': %x!",
			letter,(UINT)Status);
		return -1;
	}
	switch(ffdi.DeviceType){
	case FILE_DEVICE_CD_ROM:
	case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
		return DRIVE_CDROM;
	case FILE_DEVICE_NETWORK_FILE_SYSTEM:
		return DRIVE_REMOTE;
	case FILE_DEVICE_DISK:
	case FILE_DEVICE_DISK_FILE_SYSTEM:
		if(ffdi.Characteristics & FILE_REMOTE_DEVICE)
			return DRIVE_REMOTE;
		if(ffdi.Characteristics & FILE_REMOVABLE_MEDIA)
			return DRIVE_REMOVABLE;
		return DRIVE_FIXED;
	case FILE_DEVICE_UNKNOWN:
		return DRIVE_UNKNOWN;
	}
	return DRIVE_UNKNOWN;
}

BOOLEAN internal_open_rootdir(unsigned char letter,HANDLE *phFile)
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
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't open %ls: %x!",rootpath,(UINT)Status);
		return FALSE;
	}
	return TRUE;
}
