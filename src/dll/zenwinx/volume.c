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
#include "partition.h"

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
 * @brief Opens a root directory of the volume.
 * @param[in] volume_letter the volume letter.
 * @return File handle, NULL indicates failure.
 * @note Internal use only.
 */
static HANDLE OpenRootDirectory(unsigned char volume_letter)
{
	unsigned short rootpath[] = L"\\??\\A:\\";
	HANDLE hRoot;
	NTSTATUS Status;
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;

	rootpath[4] = (short)volume_letter;
	RtlInitUnicodeString(&uStr,rootpath);
	InitializeObjectAttributes(&ObjectAttributes,&uStr,
				   FILE_READ_ATTRIBUTES,NULL,NULL); /* ?? */
	Status = NtCreateFile(&hRoot,FILE_GENERIC_READ,
				&ObjectAttributes,&IoStatusBlock,NULL,0,
				FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,
				NULL,0);
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"Cannot open %ls",rootpath);
		return NULL;
	}
	return hRoot;
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
	HANDLE hRoot;

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
	hRoot = OpenRootDirectory(letter);
	if(hRoot == NULL){
		winx_heap_free(ppdi);
		winx_heap_free(pffdi);
		return (-1);
	}
	RtlZeroMemory(pffdi,sizeof(FILE_FS_DEVICE_INFORMATION));
	Status = NtQueryVolumeInformationFile(hRoot,&iosb,
					pffdi,sizeof(FILE_FS_DEVICE_INFORMATION),
					FileFsDeviceInformation);
	NtClose(hRoot);
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
 * @brief Retrieves drive geometry.
 * @param[in] hRoot handle to the
 * root directory.
 * @param[out] pointer to the structure
 * receiving drive geometry.
 * @return Zero for success, negative
 * value otherwise.
 * @note Internal use only.
 */
static int get_drive_geometry(HANDLE hRoot,winx_volume_information *v)
{
	FILE_FS_SIZE_INFORMATION *pffs;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	
	/* allocate memory */
	pffs = winx_heap_alloc(sizeof(FILE_FS_SIZE_INFORMATION));
	if(pffs == NULL){
		DebugPrint("winx_get_volume_information(): cannot allocate %u bytes of memory!",
			sizeof(FILE_FS_SIZE_INFORMATION));
		return (-1);
	}
	RtlZeroMemory(pffs,sizeof(FILE_FS_SIZE_INFORMATION));

	/* get drive geometry */
	Status = NtQueryVolumeInformationFile(hRoot,&IoStatusBlock,pffs,
				sizeof(FILE_FS_SIZE_INFORMATION),FileFsSizeInformation);
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"winx_get_volume_information(): cannot get geometry of drive %c:",
			v->volume_letter);
		winx_heap_free(pffs);
		return (-1);
	}
	
	/* fill all geometry related fields of the output structure */
	v->total_bytes = (ULONGLONG)pffs->TotalAllocationUnits.QuadPart * \
		pffs->SectorsPerAllocationUnit * pffs->BytesPerSector;
	v->free_bytes = (ULONGLONG)pffs->AvailableAllocationUnits.QuadPart * \
		pffs->SectorsPerAllocationUnit * pffs->BytesPerSector;
	v->total_clusters = (ULONGLONG)pffs->TotalAllocationUnits.QuadPart;
	v->bytes_per_cluster = pffs->SectorsPerAllocationUnit * pffs->BytesPerSector;
	v->sectors_per_cluster = pffs->SectorsPerAllocationUnit;
	v->bytes_per_sector = pffs->BytesPerSector;

	/* cleanup */
	winx_heap_free(pffs);
	return 0;
}

/**
 * @brief Defines whether bios parameter block
 * belongs to the FAT-formatted partition or not.
 * Updates file system type related information
 * in structure pointed by the second parameter.
 * @return Zero if FAT is detected, negative value
 * otherwise.
 */
static int IsFatPartition(BPB *bpb,winx_volume_information *v)
{
	char signature[9];
	BOOL fat_found = FALSE;
	ULONG RootDirSectors; /* USHORT ? */
	ULONG FatSectors;
	ULONG TotalSectors;
	ULONG DataSectors;
	ULONG CountOfClusters;

	/* search for FAT signatures */
	signature[8] = 0;
	memcpy((void *)signature,(void *)bpb->Fat1x.BS_FilSysType,8);
	if(strstr(signature,"FAT")) fat_found = TRUE;
	else {
		memcpy((void *)signature,(void *)bpb->Fat32.BS_FilSysType,8);
		if(strstr(signature,"FAT")) fat_found = TRUE;
	}
	if(!fat_found)
		return (-1);

	/* determine which type of FAT we have */
	RootDirSectors = ((bpb->RootDirEnts * 32) + (bpb->BytesPerSec - 1)) / bpb->BytesPerSec;

	if(bpb->FAT16sectors) FatSectors = (ULONG)(bpb->FAT16sectors);
	else FatSectors = bpb->Fat32.FAT32sectors;
	
	if(bpb->FAT16totalsectors) TotalSectors = bpb->FAT16totalsectors;
	else TotalSectors = bpb->FAT32totalsectors;
	
	DataSectors = TotalSectors - (bpb->ReservedSectors + (bpb->NumFATs * FatSectors) + RootDirSectors);
	CountOfClusters = DataSectors / bpb->SecPerCluster;
	
	if(CountOfClusters < 4085) {
		/* Volume is FAT12 */
		strcpy(v->fs_name,"FAT12");
		return 0;
	} else if(CountOfClusters < 65525) {
		/* Volume is FAT16 */
		strcpy(v->fs_name,"FAT16");
		return 0;
	} else {
		/* Volume is FAT32 */
		strcpy(v->fs_name,"FAT32");
	}
	
	/* save FAT32 version */
	v->fat32_mj_version = (ULONG)((bpb->Fat32.BPB_FSVer >> 8) & 0xFF);
	v->fat32_mn_version = (ULONG)(bpb->Fat32.BPB_FSVer & 0xFF);
	return 0;
}

/**
 * @brief Defines whether bios parameter block
 * belongs to the NTFS-formatted partition or not.
 * Updates file system type related information
 * in structure pointed by the second parameter.
 * @return Zero if NTFS is detected, negative value
 * otherwise.
 */
static int IsNtfsPartition(BPB *bpb,winx_volume_information *v)
{
	char signature[9];

	/* search for NTFS signature */
	signature[8] = 0;
	memcpy((void *)signature,(void *)bpb->OemName,8);
	if(strcmp(signature,"NTFS    ") == 0){
		strcpy(v->fs_name,"NTFS");
		return 0;
	}
	
	return (-1);
}

/**
 * @brief Opens the volume.
 * @note Internal use only.
 */
static WINX_FILE * open_volume(char volume_letter)
{
	char path[64];
	char flags[2];
	#define FLAG 'r'

	(void)_snprintf(path,64,"\\??\\%c:",volume_letter);
	path[63] = 0;
#if FLAG != 'r'
#error Volume must be opened for read access!
#endif
	flags[0] = FLAG; flags[1] = 0;
	return winx_fopen(path,flags);
}

/**
 * @brief Reads the first sector
 * of the volume into memory.
 * @param[out] buffer pointer
 * to the output buffer.
 * The size of the buffer must be
 * no less than sector size.
 * @param[in] v pointer to structure
 * containing drive geometry.
 * @return Zero for sucsess, negative
 * value otherwise.
 * @note Internal use only.
 */
static int read_first_sector(void *buffer,winx_volume_information *v)
{
	WINX_FILE *f;
	size_t n_read;

	/* open the volume */
	f = open_volume(v->volume_letter);
	if(f == NULL)
		return (-1);

	/* read the first sector */
	n_read = winx_fread(buffer,1,(size_t)v->bytes_per_sector,f);
	if(n_read == 0 || n_read > (size_t)v->bytes_per_sector){
		winx_fclose(f);
		return (-1);
	}
	
	/* cleanup */
	winx_fclose(f);
	return 0;
}

/**
 * @brief Retrieves the name of file system.
 * @param[in] hRoot handle to the
 * root directory.
 * @param[out] pointer to the structure
 * receiving the filesystem name and version
 * of FAT32 if an appropriate filesystem
 * is detected.
 * @return Zero for success, negative
 * value otherwise.
 * @note Call it after get_drive_geometry.
 * Internal use only.
 */
static int get_filesystem_name(HANDLE hRoot,winx_volume_information *v)
{
	FILE_FS_ATTRIBUTE_INFORMATION *pfa;
	int fs_attr_info_size;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	short fs_name[MAX_FS_NAME_LENGTH + 1];
	int length;
	void *first_sector;
	BPB *bpb;

	/*
	* We're using a low level analysis
	* of the first sector firstly, because
	* this method is able to detect exactly
	* the type of the file system. Also,
	* it well distinguishes different
	* editions of FAT.
	*/
	if(v->bytes_per_sector >= sizeof(BPB)){
		/* allocate memory */
		first_sector = winx_heap_alloc(v->bytes_per_sector);
		if(first_sector == NULL){
			DebugPrint("winx_get_volume_information(): cannot allocate %u bytes of memory!",
				v->bytes_per_sector);
		} else {
			if(read_first_sector(first_sector,v) < 0){
				winx_heap_free(first_sector);
			} else {
				bpb = (BPB *)first_sector;
				if(IsNtfsPartition(bpb,v) >= 0){
					winx_heap_free(first_sector);
					return 0;
				}
				if(IsFatPartition(bpb,v) >= 0){
					winx_heap_free(first_sector);
					return 0;
				}
				winx_heap_free(first_sector);
			}
		}
	}
	
	/*
	* If direct sector analysis failed,
	* then get file system name through
	* FILE_FS_ATTRIBUTE_INFORMATION.
	*/
	fs_attr_info_size = MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_ATTRIBUTE_INFORMATION);
	pfa = winx_heap_alloc(fs_attr_info_size);
	if(pfa == NULL){
		DebugPrint("winx_get_volume_information(): cannot allocate %u bytes of memory!",
			fs_attr_info_size);
		return(-1);
	}
	
	RtlZeroMemory(pfa,fs_attr_info_size);
	Status = NtQueryVolumeInformationFile(hRoot,&IoStatusBlock,pfa,
				fs_attr_info_size,FileFsAttributeInformation);
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"winx_get_volume_information(): cannot get file system name of drive %c:",
			v->volume_letter);
		winx_heap_free(pfa);
		return (-1);
	}
	
	/*
	* pfa->FileSystemName.Buffer may be not NULL terminated
	* (theoretically), so name extraction is more tricky
	* than it should be.
	*/
	length = min(MAX_FS_NAME_LENGTH,pfa->FileSystemNameLength / sizeof(short));
	wcsncpy(fs_name,pfa->FileSystemName,length);
	fs_name[length] = 0;
	_snprintf(v->fs_name,MAX_FS_NAME_LENGTH,"%ws",fs_name);
	v->fs_name[MAX_FS_NAME_LENGTH] = 0;

	/* cleanup */
	winx_heap_free(pfa);
	return 0;
}

/**
 * @brief Retrieves NTFS data for the filesystem.
 * @param[out] pointer to the structure
 * receiving the information.
 * @return Zero for success, negative value otherwise.
 * @note Internal use only.
 */
static int get_ntfs_data(winx_volume_information *v)
{
	WINX_FILE *f;
	
	/* open the volume */
	f = open_volume(v->volume_letter);
	if(f == NULL)
		return (-1);
	
	/* get ntfs data */
	if(winx_ioctl(f,FSCTL_GET_NTFS_VOLUME_DATA,
	  "get_ntfs_data: ntfs data request",
	  NULL,0,&v->ntfs_data,sizeof(NTFS_DATA),NULL) < 0){
		winx_fclose(f);
		return (-1);
	}
	
	/* cleanup */
	winx_fclose(f);
	return 0;
}

/**
 * @brief Retrieves detailed information
 * about disk volume.
 * @param[in] volume_letter the volume letter.
 * @param[in,out] v pointer to structure
 * receiving the volume information.
 * @return Zero for success, negative
 * value otherwise.
 */
int __stdcall winx_get_volume_information(char volume_letter,winx_volume_information *v)
{
	char c;
	HANDLE hRoot;
	
	/* check input data correctness */
	if(v == NULL)
		return (-1);

	/* reset all fields of the structure, except of volume_letter */
	memset(v,0,sizeof(winx_volume_information));
	v->volume_letter = volume_letter;

	c = (char)toupper((int)volume_letter);
	if(c == 0 || c < 'A' || c > 'Z')
		return (-1);
	
	/* open root directory */
	hRoot = OpenRootDirectory(volume_letter);
	if(hRoot == NULL)
		return (-1);
	
	/* get drive geometry */
	if(get_drive_geometry(hRoot,v) < 0){
		NtClose(hRoot);
		return (-1);
	}
	
	/* get the name of contained file system */
	if(get_filesystem_name(hRoot,v) < 0){
		NtClose(hRoot);
		return (-1);
	}
	
	/* get NTFS data */
	memset(&v->ntfs_data,0,sizeof(NTFS_DATA));
	if(!strcmp(v->fs_name,"NTFS")){
		if(get_ntfs_data(v) < 0){
			DebugPrint("NTFS data is unavailable for %c:",
				volume_letter);
		}
	}
	
	return 0;
}

/**
 * @brief fflush equivalent for entire volume.
 */
int __stdcall winx_vflush(char volume_letter)
{
	char path[] = "\\??\\A:";
	WINX_FILE *f;
	int result = -1;
	
	path[4] = volume_letter;
	f = winx_fopen(path,"r+");
	if(f){
		result = winx_fflush(f);
		winx_fclose(f);
	}

	return result;
}

/**
 * @brief Retrieves list of free regions on the volume.
 * @param[in] volume_name the name of the volume.
 * @param[in] flags currently not used.
 * @return List of free regions, NULL indicates that
 * either disk is full (unlikely) or some error occured.
 * @todo Add flag disallowing partial results.
 */
winx_volume_region * __stdcall winx_get_free_volume_regions(char volume_letter,int flags)
{
	winx_volume_region *rlist = NULL, *rgn = NULL;
	BITMAP_DESCRIPTOR *bitmap;
	#define LLINVALID   ((ULONGLONG) -1)
	#define BITMAPBYTES 4096
	#define BITMAPSIZE  (BITMAPBYTES + 2 * sizeof(ULONGLONG))
	/* bit shifting array for efficient processing of the bitmap */
	unsigned char bitshift[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
	WINX_FILE *f;
	ULONGLONG i, start, next, free_rgn_start;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;
	
	/* allocate memory */
	bitmap = winx_heap_alloc(BITMAPSIZE);
	if(bitmap == NULL){
		DebugPrint("winx_get_free_volume_regions: cannot allocate %u bytes of memory",
			BITMAPSIZE);
		return NULL;
	}
	
	/* open volume */
	f = open_volume(volume_letter);
	if(f == NULL){
		winx_heap_free(bitmap);
		return NULL;
	}
	
	/* get volume bitmap */
	next = 0, free_rgn_start = LLINVALID;
	do {
		/* get next portion of the bitmap */
		memset(bitmap,0,BITMAPSIZE);
		status = NtFsControlFile(winx_fileno(f),NULL,NULL,0,&iosb,
			FSCTL_GET_VOLUME_BITMAP,&next,sizeof(ULONGLONG),
			bitmap,BITMAPSIZE);
		if(NT_SUCCESS(status)){
			NtWaitForSingleObject(winx_fileno(f),FALSE,NULL);
			status = iosb.Status;
		}
		if(status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW){
			DebugPrintEx(status,"winx_get_free_volume_regions: cannot get volume bitmap");
			winx_fclose(f);
			winx_heap_free(bitmap);
			//winx_list_destroy((list_entry **)(void *)&rlist);
			return NULL;
		}
		
		/* scan through the returned bitmap info */
		start = bitmap->StartLcn;
		for(i = 0; i < min(bitmap->ClustersToEndOfVol, 8 * BITMAPBYTES); i++){
			if(!(bitmap->Map[ i/8 ] & bitshift[ i % 8 ])){
				/* cluster is free */
				if(free_rgn_start == LLINVALID)
					free_rgn_start = start + i;
			} else {
				/* cluster isn't free */
				if(free_rgn_start != LLINVALID){
					/* add free region to the list */
					rgn = (winx_volume_region *)winx_list_insert_item((list_entry **)(void *)&rlist,
						(list_entry *)rgn,sizeof(winx_volume_region));
					if(rgn == NULL){
						DebugPrint("winx_get_free_volume_regions: cannot allocate %u bytes of memory",
							sizeof(winx_volume_region));
						/* TODO: return if partial results aren't allowed */
					} else {
						rgn->lcn = free_rgn_start;
						rgn->length = start + i - free_rgn_start;
					}
					free_rgn_start = LLINVALID;
				}
			}
		}
		
		/* go to the next portion of data */
		next = bitmap->StartLcn + i;
	} while(status != STATUS_SUCCESS);

	if(free_rgn_start != LLINVALID){
		/* add free region to the list */
		rgn = (winx_volume_region *)winx_list_insert_item((list_entry **)(void *)&rlist,
			(list_entry *)rgn,sizeof(winx_volume_region));
		if(rgn == NULL){
			DebugPrint("winx_get_free_volume_regions: cannot allocate %u bytes of memory",
				sizeof(winx_volume_region));
			/* TODO: return if partial results aren't allowed */
		} else {
			rgn->lcn = free_rgn_start;
			rgn->length = start + i - free_rgn_start;
		}
		free_rgn_start = LLINVALID;
	}
	
	/* cleanup */
	winx_fclose(f);
	winx_heap_free(bitmap);
	return rlist;
}

/**
 * @brief Adds a range of clusters to the list of regions.
 * @param[in,out] rlist the list of volume regions.
 * @param[in] lcn the logical cluster number of the region to be added.
 * @param[in] length the size of the region to be added, in clusters.
 * @return Pointer to updated list of regions.
 */
winx_volume_region * __stdcall winx_add_volume_region(winx_volume_region *rlist,
		ULONGLONG lcn,ULONGLONG length)
{
	winx_volume_region *r, *rnext, *rprev = NULL;
	
	for(r = rlist; r; r = r->next){
		if(r->lcn > lcn){
			if(r != rlist) rprev = r->prev;
			break;
		}
		if(r->next == rlist){
			rprev = r;
			break;
		}
	}

	/* hits the new region previous one? */
	if(rprev){
		if(rprev->lcn + rprev->length == lcn){
			rprev->length += length;
			if(rprev->lcn + rprev->length == rprev->next->lcn){
				rprev->length += rprev->next->length;
				rprev->next->length = 0;
			}
			return rlist;
		}
	}
	
	/* hits the new region the next one? */
	if(rlist){
		if(rprev == NULL) rnext = rlist;
		else rnext = rprev->next;
		if(lcn + length == rnext->lcn){
			rnext->lcn = lcn;
			rnext->length += length;
			return rlist;
		}
	}
	
	r = (winx_volume_region *)winx_list_insert_item((list_entry **)(void *)&rlist,
		(list_entry *)rprev,sizeof(winx_volume_region));
	if(r == NULL){
		/* TODO: better error handling */
		return rlist;
	}
	
	r->lcn = lcn;
	r->length = length;
	return rlist;
}

/**
 * @brief Subtracts a range of clusters from the list of regions.
 * @param[in,out] rlist the list of volume regions.
 * @param[in] lcn the logical cluster number of the region to be subtracted.
 * @param[in] length the size of the region to be subtracted, in clusters.
 * @return Pointer to updated list of regions.
 */
winx_volume_region * __stdcall winx_sub_volume_region(winx_volume_region *rlist,
		ULONGLONG lcn,ULONGLONG length)
{
	winx_volume_region *r;
	ULONGLONG new_lcn, new_length;
	
	for(r = rlist; r; r = r->next){
		if(r->lcn >= lcn && (r->lcn + r->length) <= (lcn + length)){
			/*
			* list entry is inside a specified range
			* |--------------------|
			*        |-r-|
			*/
			r->length = 0;
			goto next_region;
		}
		if(r->lcn < lcn && (r->lcn + r->length) > lcn && \
		  (r->lcn + r->length) <= (lcn + length)){
			/*
			* cut the right side of the list entry
			*     |--------------------|
			* |----r----|
			*/
			r->length = lcn - r->lcn;
			goto next_region;
		}
		if(r->lcn >= lcn && r->lcn < (lcn + length)){
			/*
			* cut the left side of the list entry
			* |--------------------|
			*                   |----r----|
			*/
			r->length = r->lcn + r->length - (lcn + length);
			r->lcn = lcn + length;
			goto next_region;
		}
		if(r->lcn < lcn && (r->lcn + r->length) > (lcn + length)){
			/*
			* specified range is inside list entry
			*   |----|
			* |-------r--------|
			*/
			new_lcn = lcn + length;
			new_length = r->lcn + r->length - (lcn + length);
			r->length = lcn - r->lcn;
			rlist = winx_add_volume_region(rlist,new_lcn,new_length);
			goto next_region;
		}
		/* if specified range is outside of the region, there is nothing to subtract */
next_region:
		if(r->next == rlist) break;
	}
	return rlist;
}

/**
 * @brief Frees memory allocated
 * by winx_get_free_volume_regions.
 */
void __stdcall winx_release_free_volume_regions(winx_volume_region *rlist)
{
	winx_list_destroy((list_entry **)(void *)&rlist);
}

/** @} */
