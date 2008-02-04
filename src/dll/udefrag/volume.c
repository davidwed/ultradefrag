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
* Volume validation routines.
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

#define FS_ATTRIBUTE_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_ATTRIBUTE_INFORMATION))
#define INTERNAL_SEM_FAILCRITICALERRORS 0

volume_info v[MAX_DOS_DRIVES + 1];
BOOL nt4_system = FALSE; /* TRUE for nt 4.0 */

BOOL get_drive_map(PROCESS_DEVICEMAP_INFORMATION *pProcessDeviceMapInfo);
BOOL internal_open_rootdir(unsigned char letter,HANDLE *phFile);
BOOL internal_validate_volume(unsigned char letter,int skip_removable,
							  char *fsname,
							  PROCESS_DEVICEMAP_INFORMATION *pProcessDeviceMapInfo,
							  FILE_FS_SIZE_INFORMATION *pFileFsSize);
BOOL set_error_mode(UINT uMode);

/****f* udefrag.volume/udefrag_get_avail_volumes
* NAME
*    udefrag_get_avail_volumes
* SYNOPSIS
*    error = udefrag_get_avail_volumes(ppvol_info, skip_removable);
* FUNCTION
*    Retrieves the list of available volumes.
* INPUTS
*    ppvol_info     - pointer to variable of volume_info* type.
*    skip_removable - true if we need to skip removable drives,
*                     false otherwise
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    volume_info *v;
*    char buffer[ERR_MSG_SIZE];
*    int i;
*
*    if(udefrag_get_avail_volumes(&v,TRUE) < 0){
*        udefrag_pop_error(buffer,sizeof(buffer));
*    } else {
*        for(i = 0;;i++){
*            if(!v[i].letter) break;
*            // ...
*        }
*    }
* NOTES
*    if(skip_removable == FALSE && you have 
*      floppy drive without floppy disk)
*       then you will hear noise :))
* SEE ALSO
*    udefrag_validate_volume, scheduler_get_avail_letters
******/
int __stdcall udefrag_get_avail_volumes(volume_info **vol_info,int skip_removable)
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
	for(i = 0; i < MAX_DOS_DRIVES; i++){
		if((drive_map & (1 << i)) || nt4_system){
			letter = 'A' + (char)i;
			if(!internal_validate_volume(letter,skip_removable,v[index].fsname,
				&ProcessDeviceMapInfo,&FileFsSize)){
					winx_pop_error(NULL,0);
					continue;
			}
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
	if(!set_error_mode(1)){ /* equal to SetErrorMode(0) */
		winx_pop_error(NULL,0);
	}
	return 0;
get_volumes_fail:
	return (-1);
}

/* NOTE: this is something like udefrag_get_avail_volumes()
 *       but without noise.
 */
int __stdcall udefrag_validate_volume(unsigned char letter,int skip_removable)
{
	PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;
	FILE_FS_SIZE_INFORMATION FileFsSize;

	letter = (unsigned char)toupper((int)letter);
	if(letter < 'A' || letter > 'Z'){
		winx_push_error("Invalid volume letter %c!",letter);
		goto validate_fail;
	}
	if(!nt4_system)
		if(!get_drive_map(&ProcessDeviceMapInfo)) goto validate_fail;
	/* set error mode to ignore missing removable drives */
	if(!set_error_mode(INTERNAL_SEM_FAILCRITICALERRORS)) goto validate_fail;
	if(!internal_validate_volume(letter,skip_removable,NULL,
		&ProcessDeviceMapInfo,&FileFsSize)) goto validate_fail;
	/* try to restore error mode to default state */
	if(!set_error_mode(1)){ /* equal to SetErrorMode(0) */
		winx_pop_error(NULL,0);
	}
	return 0;
validate_fail:
	return (-1);
}

int __stdcall scheduler_get_avail_letters(char *letters)
{
	volume_info *v;
	int i;

	if(udefrag_get_avail_volumes(&v,TRUE)) return (-1);
	for(i = 0;;i++){
		letters[i] = v[i].letter;
		if(v[i].letter == 0) break;
	}
	return 0;
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
	if(!NT_SUCCESS(Status)){
		if(Status == STATUS_INVALID_INFO_CLASS)
			nt4_system = TRUE;
		else {
			winx_push_error("Can't get drive map: %x!",(UINT)Status);
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
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't open %ls: %x!",rootpath,(UINT)Status);
		*phFile = NULL;
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

	if(nt4_system){
		/*
		* Exclude letters assigned by 'subst' command - 
		* on w2k and later systems such drives have type
		* DRIVE_NO_ROOT_DIR, but on nt4.0 ...
		*/
		link_name[4] = (short)letter;
		RtlInitUnicodeString(&uStr,link_name);
		InitializeObjectAttributes(&ObjectAttributes,&uStr,
			OBJ_CASE_INSENSITIVE,NULL,NULL);
		Status = NtOpenSymbolicLinkObject(&hLink,SYMBOLIC_LINK_QUERY,
			&ObjectAttributes);
		if(!NT_SUCCESS(Status)){
			winx_push_error("Can't open symbolic link %ls: %x!",link_name,(UINT)Status);
			hLink = NULL;
			goto invalid_volume;
		}
		uStr.Buffer = link_target;
		uStr.Length = 0;
		uStr.MaximumLength = MAX_TARGET_LENGTH * sizeof(short);
		size = 0;
		Status = NtQuerySymbolicLinkObject(hLink,&uStr,&size);
		NtClose(hLink);
		if(!NT_SUCCESS(Status)){
			winx_push_error("Can't query symbolic link %ls: %x!",link_name,(UINT)Status);
			goto invalid_volume;
		}
		link_target[MAX_TARGET_LENGTH - 1] = 0; /* terminate the buffer */
		if(wcsstr(link_target,L"\\??\\") == (wchar_t *)link_target){
			winx_push_error("Volume letter is assigned by \'subst\' command!");
			goto invalid_volume;
		}
		/*
		* Floppies have "Floppy" substring in their names.
		*/
		if(wcsstr(link_target,L"Floppy")){
			if(skip_removable){
				winx_push_error("It's removable volume!");
				goto invalid_volume;
			}
			goto get_vol_info;
		}
		/*
		* To exclude other unwanted drives we need to query their types.
		* Note that the drive motor can be powered on during this check.
		*/
		if(!internal_open_rootdir(letter,&hFile)) goto invalid_volume;
		Status = NtQueryVolumeInformationFile(hFile,&IoStatusBlock,
						&FileFsDevice,sizeof(FILE_FS_DEVICE_INFORMATION),
						FileFsDeviceInformation);
		NtClose(hFile);
		if(!NT_SUCCESS(Status)){
			winx_push_error("Can't get volume type for \'%c\': %x!",letter,(UINT)Status);
			goto invalid_volume;
		}
		switch(FileFsDevice.DeviceType){
		case FILE_DEVICE_CD_ROM:
		case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
		case FILE_DEVICE_NETWORK_FILE_SYSTEM:
			winx_push_error("Volume must be on non-cdrom local drive!");
			goto invalid_volume;
		case FILE_DEVICE_DISK:
		case FILE_DEVICE_DISK_FILE_SYSTEM:
			if(FileFsDevice.Characteristics & FILE_REMOTE_DEVICE){
				winx_push_error("Volume must be on non-cdrom local drive!");
				goto invalid_volume;
			}
			if((FileFsDevice.Characteristics & FILE_REMOVABLE_MEDIA) &&
				skip_removable){
					winx_push_error("It's removable volume!");
					goto invalid_volume;
			}
			break;
		case FILE_DEVICE_UNKNOWN:
			winx_push_error("Unknown volume type!");
			goto invalid_volume;
		}
	} else { /* not nt4_system */
		type = (UINT)pProcessDeviceMapInfo->Query.DriveType[letter - 'A'];
		if(type == DRIVE_CDROM || type == DRIVE_REMOTE){
			winx_push_error("Volume must be on non-cdrom local drive, but it's %u!",type);
			goto invalid_volume;
		}
		if(type == DRIVE_NO_ROOT_DIR){
			winx_push_error("It seems that volume letter is assigned by \'subst\' command!");
			goto invalid_volume;
		}
		if(type == DRIVE_REMOVABLE && skip_removable){
			winx_push_error("It's removable volume!");
			goto invalid_volume;
		}
	}
get_vol_info:
	/* get volume information */
	if(!internal_open_rootdir(letter,&hFile)) goto invalid_volume;
	Status = NtQueryVolumeInformationFile(hFile,&IoStatusBlock,pFileFsSize,
				sizeof(FILE_FS_SIZE_INFORMATION),FileFsSizeInformation);
	if(!NT_SUCCESS(Status)){
		NtClose(hFile);
		winx_push_error("Can't get size of volume \'%c\': %x!",letter,(UINT)Status);
		goto invalid_volume;
	}
	if(fsname){
		fsname[0] = 0;
		pFileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)buffer;
		Status = NtQueryVolumeInformationFile(hFile,&IoStatusBlock,pFileFsAttribute,
					FS_ATTRIBUTE_BUFFER_SIZE,FileFsAttributeInformation);
		if(!NT_SUCCESS(Status)){
			NtClose(hFile);
			winx_push_error("Can't get file system name for \'%c\': %x!",letter,(UINT)Status);
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

BOOL set_error_mode(UINT uMode)
{
	NTSTATUS Status;

	Status = NtSetInformationProcess(NtCurrentProcess(),
					ProcessDefaultHardErrorMode,
					(PVOID)&uMode,
					sizeof(uMode));
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't set error mode %u: %x!",uMode,(UINT)Status);
		return FALSE;
	}
	return TRUE;
}
