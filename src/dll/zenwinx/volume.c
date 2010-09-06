/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

/**
 * @file volume.c
 * @brief Disk volumes managing code.
 * @addtogroup Disks
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

static BOOLEAN internal_open_rootdir(unsigned char letter,HANDLE *phFile);

/**
 * @brief Converts the 64-bit number
 * of bytes to a human readable format.
 * @details This function supports
 * the following suffixes: Kb, Mb, Gb, Tb, Pb, Eb.
 * @param[in] number a number to be converted.
 * @param[in] digits a number of digits after a dot.
 * @param[out] buffer pointer to the buffer for a resulting string.
 * @param[in] length the length of the buffer, in characters.
 * @return A number of characters stored, not counting the 
 * terminating null character. If the number of characters
 * required to store the data exceeds length, then length 
 * characters are stored in buffer and a negative value is returned.
 * @note The prototype of this function was StrFormatByteSize()
 * from shlwapi.dll implementation included in ReactOS.
 */
int __stdcall winx_fbsize(ULONGLONG number, int digits, char *buffer, int length)
{
	char symbols[] = {'K','M','G','T','P','E'};
	char spec[] = "%u.%00u %cb";
	double fn,n;
	unsigned int i,j,k;
	int result;

	if(!buffer || !length) return (-1);
	if(number < 1024){
		result = _snprintf(buffer,length - 1,"%u",(int)number);
		buffer[length - 1] = 0;
		return result;
	}
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
			result = _snprintf(buffer,length - 1,spec,i/j,i%j,symbols[k]);
			buffer[length - 1] = 0;
			return result;
		} else {
			result = _snprintf(buffer,length - 1,"%u Kb",(int)number / 1024);
			buffer[length - 1] = 0;
			return result;
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

/**
 * @brief Decodes a formatted string produced by winx_fbsize().
 * @param[in] string the string to be converted.
 * @param[out] pnumber pointer to the variable receiving the result.
 * @result Zero for success, negative value otherwise.
 * @bug All digits after a dot are ignored.
 */
int __stdcall winx_dfbsize(char *string,ULONGLONG *pnumber)
{
	char symbols[] = {'K','M','G','T','P','E'};
	char t[64];
	signed int i;
	ULONGLONG m = 1;

	if(strlen(string) > 63) return (-1);
	(void)strcpy(t,string);
	(void)_strupr(t);
	for(i = 0; i < sizeof(symbols); i++)
		if(strchr(t,symbols[i])) break;
	if(i < sizeof(symbols)) /* suffix found */
		for(; i >= 0; i--) m *= 1024;
	*pnumber = m * _atoi64(string);
	/* TODO: decode digits after a dot */
	return 0;
}

/**
 * @brief Win32 GetDriveType() native equivalent.
 * @param[in] letter the volume letter
 * @return A drive type, negative value indicates failure.
 */
int __stdcall winx_get_drive_type(char letter)
{
	short link_name[] = L"\\??\\A:";
	#define MAX_TARGET_LENGTH 256
	short link_target[MAX_TARGET_LENGTH];
	PROCESS_DEVICEMAP_INFORMATION *ppdi;
	FILE_FS_DEVICE_INFORMATION *pffdi;
	IO_STATUS_BLOCK iosb;
	NTSTATUS Status;
	int drive_type;
	HANDLE hFile;

	/* An additional checks for DFS were suggested by Stefan Pendl (pendl2megabit@yahoo.de). */
	/* DFS shares have DRIVE_NO_ROOT_DIR type though they are actually remote. */

	letter = (unsigned char)toupper((int)letter);
	if(letter < 'A' || letter > 'Z'){
		DebugPrint("winx_get_drive_type() invalid letter %c!",letter);
		return (-1);
	}
	
	/* check for the drive existence */
	link_name[4] = (short)letter;
	if(winx_query_symbolic_link(link_name,link_target,MAX_TARGET_LENGTH) < 0)
		return (-1);
	
	/* check for an assignment made by subst command */
	if(wcsstr(link_target,L"\\??\\") == (wchar_t *)link_target)
		return DRIVE_ASSIGNED_BY_SUBST_COMMAND;

	/* check for classical floppies */
	if(wcsstr(link_target,L"Floppy"))
		return DRIVE_REMOVABLE;
	
	/* allocate memory */
	ppdi = winx_heap_alloc(sizeof(PROCESS_DEVICEMAP_INFORMATION));
	if(!ppdi){
		DebugPrint("Cannot allocate memory for winx_get_drive_type()!\n");
		return (-1);
	}
	pffdi = winx_heap_alloc(sizeof(FILE_FS_DEVICE_INFORMATION));
	if(!pffdi){
		winx_heap_free(ppdi);
		DebugPrint("Cannot allocate memory for winx_get_drive_type()!\n");
		return (-1);
	}
	
	/* try to define exactly which type has a specified drive (w2k+) */
	RtlZeroMemory(ppdi,sizeof(PROCESS_DEVICEMAP_INFORMATION));
	Status = NtQueryInformationProcess(NtCurrentProcess(),
					ProcessDeviceMap,ppdi,
					sizeof(PROCESS_DEVICEMAP_INFORMATION),
					NULL);
	if(NT_SUCCESS(Status)){
		drive_type = (int)ppdi->Query.DriveType[letter - 'A'];
		/*
		* Type DRIVE_NO_ROOT_DIR have the following drives:
		* 1. assigned by subst command
		* 2. SCSI external drives
		* 3. RAID volumes
		* 4. DFS shares
		* We need additional checks to know exactly.
		*/
		if(drive_type != DRIVE_NO_ROOT_DIR){
			winx_heap_free(ppdi);
			winx_heap_free(pffdi);
			return drive_type;
		}
	} else {
		if(Status != STATUS_INVALID_INFO_CLASS){ /* on NT4 this is always false */
			DebugPrintEx(Status,"winx_get_drive_type(): cannot get device map");
			winx_heap_free(ppdi);
			winx_heap_free(pffdi);
			return (-1);
		}
	}
	
	/* try to define exactly again which type has a specified drive (nt4+) */
	/* note that the drive motor can be powered on during this check */
	if(!internal_open_rootdir(letter,&hFile)){
		winx_heap_free(ppdi);
		winx_heap_free(pffdi);
		return (-1);
	}
	RtlZeroMemory(pffdi,sizeof(FILE_FS_DEVICE_INFORMATION));
	Status = NtQueryVolumeInformationFile(hFile,&iosb,
					pffdi,sizeof(FILE_FS_DEVICE_INFORMATION),
					FileFsDeviceInformation);
	NtClose(hFile);
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"winx_get_drive_type(): cannot get volume type for \'%c\'",letter);
		winx_heap_free(ppdi);
		winx_heap_free(pffdi);
		return (-1);
	}

	/* separate remote and removable drives */
	if(pffdi->Characteristics & FILE_REMOTE_DEVICE){
		winx_heap_free(ppdi);
		winx_heap_free(pffdi);
		return DRIVE_REMOTE;
	}
	if(pffdi->Characteristics & FILE_REMOVABLE_MEDIA){
		winx_heap_free(ppdi);
		winx_heap_free(pffdi);
		return DRIVE_REMOVABLE;
	}

	/* finally define drive type exactly */
	switch(pffdi->DeviceType){
	case FILE_DEVICE_CD_ROM:
	case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
	case FILE_DEVICE_DVD: /* ? */
		winx_heap_free(ppdi);
		winx_heap_free(pffdi);
		return DRIVE_CDROM;
	case FILE_DEVICE_NETWORK_FILE_SYSTEM:
	case FILE_DEVICE_NETWORK: /* ? */
	case FILE_DEVICE_NETWORK_BROWSER: /* ? */
	case FILE_DEVICE_DFS_FILE_SYSTEM:
	case FILE_DEVICE_DFS_VOLUME:
	case FILE_DEVICE_DFS:
		winx_heap_free(ppdi);
		winx_heap_free(pffdi);
		return DRIVE_REMOTE;
	case FILE_DEVICE_DISK:
	case FILE_DEVICE_FILE_SYSTEM: /* ? */
	/*case FILE_DEVICE_VIRTUAL_DISK:*/
	/*case FILE_DEVICE_MASS_STORAGE:*/
	case FILE_DEVICE_DISK_FILE_SYSTEM:
		winx_heap_free(ppdi);
		winx_heap_free(pffdi);
		return DRIVE_FIXED;
	case FILE_DEVICE_UNKNOWN:
		winx_heap_free(ppdi);
		winx_heap_free(pffdi);
		return DRIVE_UNKNOWN;
	}
	winx_heap_free(ppdi);
	winx_heap_free(pffdi);
	return DRIVE_UNKNOWN;
}

/**
 * @brief Retrieves a size and a free space amount of the volume.
 * @param[in] letter the volume letter.
 * @param[out] ptotal pointer to a variable
 * receiving a size of the volume.
 * @param[out] pfree pointer to a variable
 * receiving the amount of free space.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall winx_get_volume_size(char letter, LARGE_INTEGER *ptotal, LARGE_INTEGER *pfree)
{
	HANDLE hRoot;
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_FS_SIZE_INFORMATION *pffs;
	
	letter = (char)toupper((int)letter);
	if(letter < 'A' || letter > 'Z'){
		DebugPrint("winx_get_volume_size() invalid letter %c!",letter);
		return (-1);
	}
	if(!ptotal){
		DebugPrint("winx_get_volume_size() invalid ptotal (NULL)!");
		return (-1);
	}
	if(!pfree){
		DebugPrint("winx_get_volume_size() invalid pfree (NULL)!");
		return (-1);
	}

	/* allocate memory */
	pffs = winx_heap_alloc(sizeof(FILE_FS_SIZE_INFORMATION));
	if(!pffs){
		DebugPrint("Cannot allocate memory for winx_get_volume_size()!\n");
		return (-1);
	}

	if(!internal_open_rootdir(letter,&hRoot)){
		winx_heap_free(pffs);
		return (-1);
	}
	RtlZeroMemory(pffs,sizeof(FILE_FS_SIZE_INFORMATION));
	Status = NtQueryVolumeInformationFile(hRoot,&IoStatusBlock,pffs,
				sizeof(FILE_FS_SIZE_INFORMATION),FileFsSizeInformation);
	NtClose(hRoot);
	if(!NT_SUCCESS(Status)){
		winx_heap_free(pffs);
		DebugPrintEx(Status,"winx_get_volume_size(): cannot get size of volume \'%c\'",letter);
		return (-1);
	}
	
	ptotal->QuadPart = pffs->TotalAllocationUnits.QuadPart * \
				pffs->SectorsPerAllocationUnit * pffs->BytesPerSector;
	pfree->QuadPart = pffs->AvailableAllocationUnits.QuadPart * \
				pffs->SectorsPerAllocationUnit * pffs->BytesPerSector;
	winx_heap_free(pffs);
	return 0;
}

/**
 * @brief Retrieves a name of filesystem containing on the volume.
 * @param[in] letter the volume letter.
 * @param[out] buffer pointer to the buffer receiving the result.
 * @param[in] length the length of the buffer, in characters.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall winx_get_filesystem_name(char letter, char *buffer, int length)
{
	#define FS_ATTRIBUTE_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_ATTRIBUTE_INFORMATION))
	UCHAR *buf;
	PFILE_FS_ATTRIBUTE_INFORMATION pFileFsAttribute;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	HANDLE hRoot;
	UNICODE_STRING us;
	ANSI_STRING as;
	ULONG len;
	
	letter = (char)toupper((int)letter);
	if(letter < 'A' || letter > 'Z'){
		DebugPrint("winx_get_filesystem_name() invalid letter %c!",letter);
		return (-1);
	}
	if(!buffer){
		DebugPrint("winx_get_filesystem_name() invalid buffer!");
		return (-1);
	}
	if(length <= 0){
		DebugPrint("winx_get_filesystem_name() invalid length == %i!",length);
		return (-1);
	}

	buffer[0] = 0;
	
	/* allocate memory */
	buf = winx_heap_alloc(FS_ATTRIBUTE_BUFFER_SIZE);
	if(!buf){
		DebugPrint("Cannot allocate memory for winx_get_filesystem_name()!\n");
		return(-1);
	}
	
	if(!internal_open_rootdir(letter,&hRoot)){
		winx_heap_free(buf);
		return (-1);
	}
	pFileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)buf;
	RtlZeroMemory(pFileFsAttribute,FS_ATTRIBUTE_BUFFER_SIZE);
	Status = NtQueryVolumeInformationFile(hRoot,&IoStatusBlock,pFileFsAttribute,
				FS_ATTRIBUTE_BUFFER_SIZE,FileFsAttributeInformation);
	NtClose(hRoot);
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"Cannot get file system name for \'%c\'",letter);
		winx_heap_free(buf);
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
	winx_heap_free(buf);
	return 0;
}

/**
 * @brief Opens a root directory of the volume.
 * @param[in] letter the volume letter.
 * @param[out] phFile pointer to the file handle.
 * @return TRUE for success, FALSE indicates failure.
 * @note Internal use only.
 */
static BOOLEAN internal_open_rootdir(unsigned char letter,HANDLE *phFile)
{
	NTSTATUS Status;
	unsigned short rootpath[] = L"\\??\\A:\\";
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;

	rootpath[4] = (short)letter;
	RtlInitUnicodeString(&uStr,rootpath);
	InitializeObjectAttributes(&ObjectAttributes,&uStr,
				   FILE_READ_ATTRIBUTES,NULL,NULL); /* ?? */
	Status = NtCreateFile(phFile,FILE_GENERIC_READ,
				&ObjectAttributes,&IoStatusBlock,NULL,0,
				FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,
				NULL,0);
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"Cannot open %ls",rootpath);
		return FALSE;
	}
	return TRUE;
}

/** @} */
