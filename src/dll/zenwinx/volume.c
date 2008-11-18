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
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "ntndk.h"
#include "zenwinx.h"

#define FS_ATTRIBUTE_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_ATTRIBUTE_INFORMATION))

BOOLEAN internal_open_rootdir(unsigned char letter,HANDLE *phFile);

/* TODO: move these two functions to a string.c file. */

/****f* zenwinx.volume/winx_fbsize
* NAME
*    winx_fbsize
* SYNOPSIS
*    winx_fbsize(number,digits,buffer,length);
* FUNCTION
*    Converts the 64-bit number of bytes 
*    to human readable format with the following suffixes:
*    Kb, Mb, Gb, Tb, Pb, Eb. With specified number of 
*    digits after a dot.
* INPUTS
*    number - number of bytes to be converted
*    digits - number of digits after a dot
*    buffer - pointer to the buffer for a resulting string
*    length - length of the buffer
* RESULT
*    The number of characters stored.
* EXAMPLE
*    winx_fbsize(1548,2,buffer,sizeof(buffer));
*    // buffer contains now "1.51 Kb"
* NOTES
*    The prototype of this function was StrFormatByteSize()
*    from shlwapi.dll implementation included in ReactOS.
* SEE ALSO
*    winx_dfbsize
******/
int __stdcall winx_fbsize(ULONGLONG number, int digits, char *buffer, int length)
{
	char symbols[] = {'K','M','G','T','P','E'};
	char spec[] = "%u.%00u %cb";
	double fn,n;
	unsigned int i,j,k;

	if(!buffer || !length) return (-1);
	if(number < 1024) return _snprintf(buffer,length - 1,"%u",(int)number);
	/* 
	* Because win ddk compiler doesn't have ULONGLONG -> double converter,
	* we need first divide an integer number and then convert in to double.
	*/
	if(number < 1024 * 1024){
		if(digits){
			/* we can convert a number to int here */
			fn = (double)(int)number;
			fn /= 1024.0;
			k = 0;
L0:
			/* example: fn = 128.45638, digits = 4 */
			/* %.2f doesn't work in native mode */
			n = 1.0;
			for(i = digits; i > 0; i--)
				n *= 10.0;
			fn *= n;
			/* example: n = 10000.0, fn = 1284563.8 */
			i = (int)fn;
			j = (int)n;
			/* example: i = 1284563, j = 10000 */
			spec[4] = '0' + digits / 10;
			spec[5] = '0' + digits % 10;
			return _snprintf(buffer,length - 1,spec,i/j,i%j,symbols[k]);
		} else {
			return _snprintf(buffer,length - 1,"%u Kb",(int)number / 1024);
		}
		return (-1); /* this point will never be reached */
	}
	/* convert integer number to kilobytes */
	number /= 1024;
	fn = (double)(signed __int64)number / 1024.00;
	for(k = 1; fn >= 1024.00; k++) fn /= 1024.00;
	if(k > sizeof(symbols) - 1) k = sizeof(symbols) - 1;
	goto L0;
	return (-1); /* this point will never be reached */
}

/****f* zenwinx.volume/winx_dfbsize
* NAME
*    winx_dfbsize
* SYNOPSIS
*    winx_dfbsize(string,pnumber);
* FUNCTION
*    Converts string generated by winx_fbsize() to 
*    an integer number of bytes.
* INPUTS
*    string  - string to be converted
*    pnumber - pointer to ULONGLONG variable
*              that receives the result
* RESULT
*    Zero for success, -1 otherwise.
* EXAMPLE
*    winx_dfbsize("128.3 Kb", &size);
*    // size contains now 131072 // not 131379! (see below)
* MOTES
*    All digits after a dot will be ignored, 
*    so 34.5687 will be treated as 34.
* SEE ALSO
*    winx_fbsize
******/
int __stdcall winx_dfbsize(char *string,ULONGLONG *pnumber)
{
	char symbols[] = {'K','M','G','T','P','E'};
	char t[64];
	signed int i;
	ULONGLONG m = 1;

	if(strlen(string) > 63) return (-1);
	strcpy(t,string);
	_strupr(t);
	for(i = 0; i < sizeof(symbols); i++)
		if(strchr(t,symbols[i])) break;
	if(i < sizeof(symbols)) /* suffix found */
		for(; i >= 0; i--) m *= 1024;
	*pnumber = m * _atoi64(string);
	/* TODO: decode digits after a dot */
	return 0;
}

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
* SEE ALSO
*    winx_get_disk_free_space, winx_get_filesystem_name
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
		winx_raise_error("E: winx_get_drive_type() invalid letter %c!",letter);
		return (-1);
	}
	/* this call is applicable only for w2k and later versions */
	Status = NtQueryInformationProcess(NtCurrentProcess(),
					ProcessDeviceMap,
					&pdi,sizeof(PROCESS_DEVICEMAP_INFORMATION),
					NULL);
	if(!NT_SUCCESS(Status)){
		if(Status != STATUS_INVALID_INFO_CLASS){
			winx_raise_error("E: winx_get_drive_type(): can't get drive map: %x!",(UINT)Status);
			return (-1);
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
		winx_raise_error("N: winx_get_drive_type(): can't open symbolic link %ls: %x!",
			link_name,(UINT)Status);
		return (-1);
	}
	uStr.Buffer = link_target;
	uStr.Length = 0;
	uStr.MaximumLength = MAX_TARGET_LENGTH;
	size = 0;
	Status = NtQuerySymbolicLinkObject(hLink,&uStr,&size);
	NtClose(hLink);
	if(!NT_SUCCESS(Status)){
		winx_raise_error("N: winx_get_drive_type(): can't query symbolic link %ls: %x!",
			link_name,(UINT)Status);
		return (-1);
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
		return (-1);
	Status = NtQueryVolumeInformationFile(hFile,&iosb,
					&ffdi,sizeof(FILE_FS_DEVICE_INFORMATION),
					FileFsDeviceInformation);
	NtClose(hFile);
	if(!NT_SUCCESS(Status)){
		winx_raise_error("E: winx_get_drive_type(): can't get volume type for \'%c\': %x!",
			letter,(UINT)Status);
		return (-1);
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

/****f* zenwinx.volume/winx_get_volume_size
* NAME
*    winx_get_volume_size
* SYNOPSIS
*    error = winx_get_volume_size(letter, ptotal, pfree);
* FUNCTION
*    Retrieves total size of the specified volume
*    and free space on them in bytes.
* INPUTS
*    letter - volume letter
*    ptotal - pointer to a variable that receives 
*             the total number of bytes on the disk
*    pfree  - Pointer to a variable that receives 
*             the total number of free bytes on the disk 
* RESULT
*    If the function fails, the return value is (-1).
*    Otherwise it returns zero.
* EXAMPLE
*    winx_get_volume_size('C',&total,&free);
* SEE ALSO
*    winx_get_drive_type, winx_get_filesystem_name
******/
int __stdcall winx_get_volume_size(char letter, LARGE_INTEGER *ptotal, LARGE_INTEGER *pfree)
{
	HANDLE hRoot;
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_FS_SIZE_INFORMATION ffs;
	
	letter = (char)toupper((int)letter);
	if(letter < 'A' || letter > 'Z'){
		winx_raise_error("E: winx_get_volume_size() invalid letter %c!",letter);
		return (-1);
	}
	if(!ptotal){
		winx_raise_error("E: winx_get_volume_size() invalid ptotal (NULL)!");
		return (-1);
	}
	if(!pfree){
		winx_raise_error("E: winx_get_volume_size() invalid pfree (NULL)!");
		return (-1);
	}

	if(!internal_open_rootdir(letter,&hRoot)) return (-1);
	Status = NtQueryVolumeInformationFile(hRoot,&IoStatusBlock,&ffs,
				sizeof(FILE_FS_SIZE_INFORMATION),FileFsSizeInformation);
	NtClose(hRoot);
	if(!NT_SUCCESS(Status)){
		winx_raise_error("E: winx_get_volume_size(): can't get size of volume \'%c\': %x!",
			letter,(UINT)Status);
		return (-1);
	}
	
	ptotal->QuadPart = ffs.TotalAllocationUnits.QuadPart * \
				ffs.SectorsPerAllocationUnit * ffs.BytesPerSector;
	pfree->QuadPart = ffs.AvailableAllocationUnits.QuadPart * \
				ffs.SectorsPerAllocationUnit * ffs.BytesPerSector;
	return 0;
}

/****f* zenwinx.volume/winx_get_filesystem_name
* NAME
*    winx_get_filesystem_name
* SYNOPSIS
*    error = winx_get_filesystem_name(letter, buffer, length);
* FUNCTION
*    Retrieves the name of file system containing
*    on the specified volume.
* INPUTS
*    letter - volume letter
*    buffer - pointer to the buffer to receive
*             the name of filesystem
*    length - maximum size of the buffer in characters.
* RESULT
*    If the function fails, the return value is (-1).
*    Otherwise it returns zero.
* EXAMPLE
*    winx_get_filesystem_name('C',fsname,sizeof(fsname));
* SEE ALSO
*    winx_get_drive_type, winx_get_disk_free_space
******/
int __stdcall winx_get_filesystem_name(char letter, char *buffer, int length)
{
	HANDLE hRoot;
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;
	UCHAR buf[FS_ATTRIBUTE_BUFFER_SIZE];
	PFILE_FS_ATTRIBUTE_INFORMATION pFileFsAttribute;
	UNICODE_STRING us;
	ANSI_STRING as;
	ULONG len;
	
	letter = (char)toupper((int)letter);
	if(letter < 'A' || letter > 'Z'){
		winx_raise_error("E: winx_get_filesystem_name() invalid letter %c!",letter);
		return (-1);
	}
	if(!buffer){
		winx_raise_error("E: winx_get_filesystem_name() invalid buffer!");
		return (-1);
	}
	if(length <= 0){
		winx_raise_error("E: winx_get_filesystem_name() invalid length == %i!",length);
		return (-1);
	}

	buffer[0] = 0;
	if(!internal_open_rootdir(letter,&hRoot)) return (-1);
	pFileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)buf;
	Status = NtQueryVolumeInformationFile(hRoot,&IoStatusBlock,pFileFsAttribute,
				FS_ATTRIBUTE_BUFFER_SIZE,FileFsAttributeInformation);
	NtClose(hRoot);
	if(!NT_SUCCESS(Status)){
		winx_raise_error("W: Can't get file system name for \'%c\': %x!",letter,(UINT)Status);
		return (-1);
	}

	us.Buffer = pFileFsAttribute->FileSystemName;
	len = pFileFsAttribute->FileSystemNameLength;
	us.Length = us.MaximumLength = (USHORT)len;
	
	as.Buffer = buffer;
	as.Length = 0;
	as.MaximumLength = (USHORT)length;
	if(RtlUnicodeStringToAnsiString(&as,&us,FALSE) != STATUS_SUCCESS){
		if(length >= 2)
			strcpy(buffer,"?");
		else
			buffer[0] = 0;
	}
	return 0;
}

/* internal function */
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
		winx_raise_error("E: Can't open %ls: %x!",rootpath,(UINT)Status);
		return FALSE;
	}
	return TRUE;
}
