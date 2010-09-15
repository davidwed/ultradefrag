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
 * @file ftw.c
 * @brief File tree walk routines.
 * @addtogroup File
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

/**
 * @brief Size of the buffer dedicated
 * to list directory entries into.
 * @note Must be multiple of sizeof(void *).
 */
#define FILE_LISTING_SIZE (16*1024)

/**
 * @brief Size of the buffer dedicated
 * to list file fragments into.
 * @details It is large enough to hold
 * GET_RETRIEVAL_DESCRIPTOR structure 
 * as well as 512 MAPPING_PAIR structures.
 */
#define FILE_MAP_SIZE (sizeof(GET_RETRIEVAL_DESCRIPTOR) - sizeof(MAPPING_PAIR) + 512 * sizeof(MAPPING_PAIR))

/**
 * @brief LCN of virtual clusters.
 */
#define LLINVALID ((ULONGLONG) -1)

/* external functions prototypes */
winx_file_info * __stdcall ntfs_scan_disk(char volume_letter,
	int flags, ftw_filter_callback fcb, ftw_progress_callback pcb, 
	ftw_terminator t);

/**
 * @brief Checks whether the file
 * tree walk must be terminated or not.
 * @return Nonzero value indicates that
 * termination is requested.
 */
static int ftw_check_for_termination(ftw_terminator t)
{
	if(t == NULL)
		return 0;
	
	return t();
}

/**
 * @brief Opens the file for dumping.
 * @return Handle to the file, NULL
 * indicates failure.
 */
static HANDLE ftw_fopen(winx_file_info *f)
{
	ULONG flags = FILE_SYNCHRONOUS_IO_NONALERT;
	UNICODE_STRING us;
	OBJECT_ATTRIBUTES oa;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;
	HANDLE hFile;
	
	if(f == NULL)
		return NULL;
	
	RtlInitUnicodeString(&us,f->path);
	InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
	if(is_directory(f))
		flags |= FILE_OPEN_FOR_BACKUP_INTENT;
	else
		flags |= FILE_NO_INTERMEDIATE_BUFFERING;
	status = NtCreateFile(&hFile, FILE_GENERIC_READ | SYNCHRONIZE,
		&oa, &iosb, NULL, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN,
		flags, NULL, 0);
	if(status != STATUS_SUCCESS){
		DebugPrintEx(status,"Cannot open %ws",f->path);
		return NULL;
	}
	
	return hFile;
}

/**
 * @brief Retrieves information
 * about the file disposition.
 * @return Zero for success,
 * negative value otherwise.
 */
static int ftw_dump_file(winx_file_info *f,ftw_terminator t)
{
	GET_RETRIEVAL_DESCRIPTOR *filemap;
	HANDLE hFile;
	ULONGLONG startVcn;
	long counter; /* counts number of attempts to get information */
	#define MAX_COUNT 1000
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;
	int i;
	winx_blockmap *block = NULL;
	
	if(f == NULL)
		return (-1);
	
	/* reset disposition related fields */
	f->disp.clusters = 0;
	f->disp.fragments = 0;
	f->disp.flags = 0;
	f->disp.blockmap = NULL;
	
	/* open the file */
	hFile = ftw_fopen(f);
	if(hFile == NULL)
		return 0; /* file is locked by system */
	
	/* allocate memory */
	filemap = winx_heap_alloc(FILE_MAP_SIZE);
	if(filemap == NULL){
		DebugPrint("ftw_dump_file: cannot allocate %u bytes of memory",
			FILE_MAP_SIZE);
		NtClose(hFile);
		return (-1);
	}
	
	/* dump the file */
	startVcn = 0;
	counter = 0;
	do {
		memset(filemap,0,FILE_MAP_SIZE);
		status = NtFsControlFile(hFile,NULL,NULL,0,
			&iosb,FSCTL_GET_RETRIEVAL_POINTERS,
			&startVcn,sizeof(ULONGLONG),
			filemap,FILE_MAP_SIZE);
		counter ++;
		if(NT_SUCCESS(status)){
			NtWaitForSingleObject(hFile,FALSE,NULL);
			status = iosb.Status;
		}
		if(status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW){
			/* it always returns STATUS_END_OF_FILE for small files placed in MFT */
			if(status != STATUS_END_OF_FILE)
				DebugPrintEx(status,"Dump failed for %ws",f->path);
		cleanup:
			f->disp.clusters = 0;
			f->disp.fragments = 0;
			f->disp.flags = 0;
			winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
			winx_heap_free(filemap);
			NtClose(hFile);
			return 0; /* file is inside MFT */
		}

		if(ftw_check_for_termination(t)){
			if(counter > MAX_COUNT)
				DebugPrint("ftw_dump_file: %ws: infinite main loop?",f->path);
			goto cleanup;
		}
		
		/* check for an empty map */
		if(!filemap->NumberOfPairs && status != STATUS_SUCCESS){
			DebugPrint("%ws: empty map of file detected",f->path);
			goto cleanup;
		}
		
		/* loop through the buffer of number/cluster pairs */
		startVcn = filemap->StartVcn;
		for(i = 0; i < (ULONGLONG)filemap->NumberOfPairs; startVcn = filemap->Pair[i].Vcn, i++){
			/* compressed virtual runs (0-filled) are identified with LCN == -1	*/
			if(filemap->Pair[i].Lcn == LLINVALID)
				continue;
			
			/* The following code may cause an infinite main loop (bug #2053941?), */
			/* but for some 3.99 Gb files on FAT32 it works fine. */
			if(filemap->Pair[i].Vcn == 0){
				DebugPrint("%ws: wrong map of file detected",f->path);
				continue;
			}
			
			block = (winx_blockmap *)winx_list_insert_item((list_entry **)&f->disp.blockmap,
				(list_entry *)block,sizeof(winx_blockmap));
			if(block == NULL){
				DebugPrint("ftw_dump_file: cannot allocate %u bytes of memory",
					sizeof(winx_blockmap));
				/* cleanup */
				f->disp.clusters = 0;
				f->disp.fragments = 0;
				f->disp.flags = 0;
				winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
				winx_heap_free(filemap);
				NtClose(hFile);
				return (-1);
			}
			block->lcn = filemap->Pair[i].Lcn;
			block->length = filemap->Pair[i].Vcn - startVcn;
			block->vcn = startVcn;
			
			//DebugPrint("VCN = %I64u, LCN = %I64u, LENGTH = %I64u",
			//	block->vcn,block->lcn,block->length);

			f->disp.clusters += block->length;
			f->disp.fragments ++;
			/*
			* Sometimes normal file has more than one fragment, 
			* but is not fragmented yet! 8-) 
			*/
			if(block != f->disp.blockmap && \
			  block->lcn != (block->prev->lcn + block->prev->length))
				f->disp.flags |= WINX_FILE_DISP_FRAGMENTED;
		}
	} while(status != STATUS_SUCCESS);

	/* small directories placed inside MFT have empty list of fragments... */
	winx_heap_free(filemap);
	NtClose(hFile);
	return 0;
}

/**
 * @brief Adds directory entry to the file list.
 * @return Address of inserted file list entry,
 * NULL indicates failure.
 */
static winx_file_info * ftw_add_entry_to_filelist(short *path, int flags,
	ftw_filter_callback fcb, ftw_progress_callback pcb,
	ftw_terminator t, winx_file_info **filelist,
	FILE_BOTH_DIR_INFORMATION *file_entry)
{
	winx_file_info *f;
	int length;
	int is_rootdir = 0;
	
	if(path == NULL)
		return NULL;
	
	if(path[0] == 0){
		DebugPrint("ftw_add_entry_to_filelist: path is empty");
		return NULL;
	}
	
	/* insert new item to the file list */
	f = (winx_file_info *)winx_list_insert_item((list_entry **)(void *)filelist,
		NULL,sizeof(winx_file_info));
	if(f == NULL){
		DebugPrint("ftw_add_entry_to_filelist: cannot allocate %u bytes of memory",
			sizeof(winx_file_info));
		return NULL;
	}
	
	/* extract filename */
	f->name = winx_heap_alloc(file_entry->FileNameLength + sizeof(short));
	if(f->name == NULL){
		DebugPrint("ftw_add_entry_to_filelist: cannot allocate %u bytes of memory",
			file_entry->FileNameLength + sizeof(short));
		winx_list_remove_item((list_entry **)(void *)filelist,(list_entry *)f);
		return NULL;
	}
	memset(f->name,0,file_entry->FileNameLength + sizeof(short));
	memcpy(f->name,file_entry->FileName,file_entry->FileNameLength);
	
	/* detect whether we are in root directory or not */
	length = wcslen(path);
	if(path[length - 1] == '\\')
		is_rootdir = 1; /* only root directory contains trailing backslash */
	
	/* build path */
	length += wcslen(f->name) + 1;
	if(!is_rootdir)
		length ++;
	f->path = winx_heap_alloc(length * sizeof(short));
	if(f->path == NULL){
		DebugPrint("ftw_add_entry_to_filelist: cannot allocate %u bytes of memory",
			length * sizeof(short));
		winx_heap_free(f->name);
		winx_list_remove_item((list_entry **)(void *)filelist,(list_entry *)f);
		return NULL;
	}
	if(is_rootdir)
		(void)_snwprintf(f->path,length,L"%ws%ws",path,f->name);
	else
		(void)_snwprintf(f->path,length,L"%ws\\%ws",path,f->name);
	f->path[length - 1] = 0;
	
	/* save file attributes */
	f->flags = file_entry->FileAttributes;
	
	/* reset user defined flags */
	f->user_defined_flags = 0;
	
	//DebugPrint("%ws",f->path);
	
	/* reset internal data fields */
	memset(&f->internal,0,sizeof(winx_file_internal_info));
	
	/* reset file disposition */
	memset(&f->disp,0,sizeof(winx_file_disposition));

	/* get file disposition if requested */
	if(flags & WINX_FTW_DUMP_FILES){
		if(ftw_dump_file(f,t) < 0){
			winx_heap_free(f->name);
			winx_heap_free(f->path);
			winx_list_remove_item((list_entry **)(void *)filelist,(list_entry *)f);
			return NULL;
		}
	}	
	
	return f;
}

/**
 * @brief Adds information about
 * root directory to file list.
 */
static int ftw_add_root_directory(short *path, int flags,
	ftw_filter_callback fcb, ftw_progress_callback pcb, 
	ftw_terminator t,winx_file_info **filelist)
{
	winx_file_info *f;
	int length;
	FILE_BASIC_INFORMATION fbi;
	HANDLE hDir;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;
	
	if(path == NULL)
		return (-1);
	
	if(path[0] == 0){
		DebugPrint("ftw_add_root_directory: path is empty");
		return (-1);
	}
	
	/* insert new item to the file list */
	f = (winx_file_info *)winx_list_insert_item((list_entry **)(void *)filelist,
		NULL,sizeof(winx_file_info));
	if(f == NULL){
		DebugPrint("ftw_add_root_directory: cannot allocate %u bytes of memory",
			sizeof(winx_file_info));
		return (-1);
	}
	
	/* build path */
	length = wcslen(path) + 1;
	f->path = winx_heap_alloc(length * sizeof(short));
	if(f->path == NULL){
		DebugPrint("ftw_add_root_directory: cannot allocate %u bytes of memory",
			length * sizeof(short));
		winx_list_remove_item((list_entry **)(void *)filelist,(list_entry *)f);
		return (-1);
	}
	wcscpy(f->path,path);
	
	/* save . filename */
	f->name = winx_heap_alloc(2 * sizeof(short));
	if(f->name == NULL){
		DebugPrint("ftw_add_root_directory: cannot allocate %u bytes of memory",
			2 * sizeof(short));
		winx_heap_free(f->path);
		winx_list_remove_item((list_entry **)(void *)filelist,(list_entry *)f);
		return (-1);
	}
	f->name[0] = '.';
	f->name[1] = 0;
	
	/* get file attributes */
	f->flags |= FILE_ATTRIBUTE_DIRECTORY;
	hDir = ftw_fopen(f);
	if(hDir != NULL){
		memset(&fbi,0,sizeof(FILE_BASIC_INFORMATION));
		status = NtQueryInformationFile(hDir,&iosb,
			&fbi,sizeof(FILE_BASIC_INFORMATION),
			FileBasicInformation);
		if(!NT_SUCCESS(status)){
			DebugPrintEx(status,"NtQueryInformationFile(FileBasicInformation) failed");
		} else {
			f->flags = fbi.FileAttributes;
			DebugPrint("Root directory flags: %u",f->flags);
		}
		NtClose(hDir);
	}
	
	/* reset user defined flags */
	f->user_defined_flags = 0;
	
	/* reset internal data fields */
	memset(&f->internal,0,sizeof(winx_file_internal_info));
	
	/* reset file disposition */
	memset(&f->disp,0,sizeof(winx_file_disposition));

	/* get file disposition if requested */
	if(flags & WINX_FTW_DUMP_FILES){
		if(ftw_dump_file(f,t) < 0){
			winx_heap_free(f->name);
			winx_heap_free(f->path);
			winx_list_remove_item((list_entry **)(void *)filelist,(list_entry *)f);
			return (-1);
		}
	}
	
	/* call callbacks */
	if(pcb != NULL)
		pcb(f);
	if(fcb != NULL)
		fcb(f);
	
	return 0;
}

/**
 * @brief Opens directory for file listing.
 * @return Handle to the directory, NULL
 * indicates failure.
 */
static HANDLE ftw_open_directory(short *path)
{
	UNICODE_STRING us;
	OBJECT_ATTRIBUTES oa;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;
	HANDLE hDir;
	
	RtlInitUnicodeString(&us,path);
	InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
	status = NtCreateFile(&hDir, FILE_LIST_DIRECTORY | FILE_RESERVE_OPFILTER,
		&oa, &iosb, NULL, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN,
		FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
		NULL, 0);
	if(status != STATUS_SUCCESS){
		DebugPrintEx(status,"Cannot open %ws",path);
		return NULL;
	}
	
	return hDir;
}

/**
 * @brief Scans directory and adde information
 * about files found to the passed file list.
 * @return Zero for success, -1 indicates
 * failure, -2 indicates termination requested
 * by caller.
 */
static int ftw_helper(short *path, int flags,
		ftw_filter_callback fcb, ftw_progress_callback pcb,
		ftw_terminator t,winx_file_info **filelist)
{
	FILE_BOTH_DIR_INFORMATION *file_listing, *file_entry;
	HANDLE hDir;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;
	winx_file_info *f;
	int skip_children, result;
	
	/* open directory */
	hDir = ftw_open_directory(path);
	if(hDir == NULL)
		return 0; /* directory is locked by system, skip it */
	
	/* allocate memory */
	file_listing = winx_heap_alloc(FILE_LISTING_SIZE);
	if(file_listing == NULL){
		DebugPrint("ftw_helper: cannot allocate %u bytes of memory",
			FILE_LISTING_SIZE);
		NtClose(hDir);
		return (-1);
	}
	
	/* reset buffer */
	memset((void *)file_listing,0,FILE_LISTING_SIZE);
	file_entry = file_listing;

	/* list directory entries */
	while(!ftw_check_for_termination(t)){
		/* get directory entry */
		if(file_entry->NextEntryOffset){
			/* go to the next directory entry */
			file_entry = (FILE_BOTH_DIR_INFORMATION *)((char *)file_entry + \
				file_entry->NextEntryOffset);
		} else {
			/* read next portion of directory entries */
			memset((void *)file_listing,0,FILE_LISTING_SIZE);
			status = NtQueryDirectoryFile(hDir,NULL,NULL,NULL,
				&iosb,(void *)file_listing,FILE_LISTING_SIZE,
				FileBothDirectoryInformation,
				FALSE /* return multiple entries */,
				NULL,
				FALSE /* do not restart scan */
				);
			if(status != STATUS_SUCCESS){
				/* no more entries to read */
				winx_heap_free(file_listing);
				NtClose(hDir);
				return 0;
			}
			file_entry = file_listing;
		}
		
		/* skip . and .. entries */
		if(file_entry->FileNameLength == sizeof(short)){
			if(file_entry->FileName[0] == '.')
				continue;
		}
		if(file_entry->FileNameLength == 2 * sizeof(short)){
			if(file_entry->FileName[0] == '.' && file_entry->FileName[1] == '.')
				continue;
		}

		/* validate entry */
		if(file_entry->FileNameLength == 0)
			continue;
		
		/* add entry to the file list */
		f = ftw_add_entry_to_filelist(path,flags,fcb,pcb,t,filelist,file_entry);
		if(f == NULL){
			winx_heap_free(file_listing);
			NtClose(hDir);
			return (-1);
		}
		
		//DebugPrint("%ws\n%ws",f->name,f->path);
		
		/* check for termination */
		if(ftw_check_for_termination(t)){
			DebugPrint("ftw_helper: terminated by user");
			winx_heap_free(file_listing);
			NtClose(hDir);
			return (-2);
		}
		
		/* call the callback routines */
		if(pcb != NULL)
			pcb(f);
		
		skip_children = 0;
		if(fcb != NULL)
			skip_children = fcb(f);

		/* scan subdirectories if requested */
		if(is_directory(f) && (flags & WINX_FTW_RECURSIVE) && !skip_children){
			result = ftw_helper(f->path,flags,fcb,pcb,t,filelist);
			if(result < 0){
				winx_heap_free(file_listing);
				NtClose(hDir);
				return result;
			}
		}
	}
	
	/* terminated */
	winx_heap_free(file_listing);
	NtClose(hDir);
	return (-2);
}

/**
 * @brief Returns list of files contained
 * in directory, and all its subdirectories
 * if WINX_FTW_RECURSIVE flag is passed.
 * @param[in] path the native path of the
 * directory to be scanned.
 * @param[in] flags combination
 * of WINX_FTW_xxx flags, defined in zenwinx.h
 * @param[in] fcb address of callback routine
 * to be called for each file; if it returns
 * nonzero value, all file's children will be
 * skipped. Zero value forces to continue
 * subdirectory scan.
 * @param[in] pcb address of callback routine
 * to be called for each file to update progress
 * information specific for the caller.
 * @param[in] t address of procedure to be called
 * each time when winx_ftw would like to know
 * whether it must be terminated or not.
 * Nonzero value, returned by terminator,
 * forces file tree walk to be terminated.
 * @return List of files, NULL indicates failure.
 * @note 
 * - Optimized for little directories scan.
 * - To scan root directory, add trailing backslash
 *   to the path.
 * - fcb parameter may be equal to NULL if no
 *   filtering is needed.
 * - pcb parameter may be equal to NULL.
 * @par Example:
 * @code
 * int __stdcall process_file(winx_file_info *f)
 * {
 *     if(skip_directory(f))
 *         return 1;    // skip current directory
 *
 *     return 0; // continue walk
 * }
 *
 * void __stdcall update_progress(winx_file_info *f)
 * {
 *     if(is_directory(f))
 *         dir_count ++;
 *     // etc.
 * }
 *
 * int __stdcall terminator(void)
 * {
 *     if(stop_event)
 *         return 1; // terminate walk
 *
 *     return 0;     // continue walk
 * }
 *
 * // list all files on disk c:
 * filelist = winx_ftw(L"\\??\\c:\\",0,process_file,update_progress,terminator);
 * // ...
 * // process list of files
 * // ...
 * winx_ftw_release(filelist);
 * @endcode
 */
winx_file_info * __stdcall winx_ftw(short *path, int flags,
		ftw_filter_callback fcb, ftw_progress_callback pcb, ftw_terminator t)
{
	winx_file_info *filelist = NULL;
	
	if(ftw_helper(path,flags,fcb,pcb,t,&filelist) == (-1) && \
	  !(flags & WINX_FTW_ALLOW_PARTIAL_SCAN)){
		/* destroy list */
		winx_ftw_release(filelist);
		return NULL;
	}
		
	return filelist;
}

/**
 * @brief winx_ftw analog, but optimized
 * for the entire disk scan.
 * @note NTFS is scanned directly through reading
 * MFT records, because this highly (25 times)
 * speeds up the scan. For FAT we have noticed
 * no speedup (even slowdown) while trying
 * to walk trough FAT entries. This is because
 * Windows file cache makes access even faster.
 * UDF has been never tested in direct mode
 * because of its highly complicated standard.
 */
winx_file_info * __stdcall winx_scan_disk(char volume_letter, int flags,
		ftw_filter_callback fcb,ftw_progress_callback pcb, ftw_terminator t)
{
	winx_file_info *filelist = NULL;
	short rootpath[] = L"\\??\\A:\\";
	winx_volume_information v;
	
	if(winx_get_volume_information(volume_letter,&v) >= 0){
		DebugPrint("winx_scan_disk: file system is %s",v.fs_name);
		if(!strcmp(v.fs_name,"NTFS"))
			return ntfs_scan_disk(volume_letter,flags,fcb,pcb,t);
	}
	
	/* collect information about root directory */
	rootpath[4] = (short)volume_letter;
	if(ftw_add_root_directory(rootpath,flags,fcb,pcb,t,&filelist) == (-1) && \
	  !(flags & WINX_FTW_ALLOW_PARTIAL_SCAN)){
		/* destroy list */
		winx_ftw_release(filelist);
		return NULL;
	}

	/* collect information about entire directory tree */
	flags |= WINX_FTW_RECURSIVE;
	if(ftw_helper(rootpath,flags,fcb,pcb,t,&filelist) == (-1) && \
	  !(flags & WINX_FTW_ALLOW_PARTIAL_SCAN)){
		/* destroy list */
		winx_ftw_release(filelist);
		return NULL;
	}

	return filelist;
}

/**
 * @brief Releases resources
 * allocated by winx_ftw
 * or winx_scan_disk.
 * @param[in] filelist pointer
 * to list of files.
 */
void __stdcall winx_ftw_release(winx_file_info *filelist)
{
	winx_file_info *f;

	/* walk through list of files and free allocated memory */
	for(f = filelist; f != NULL; f = f->next){
		if(f->name)
			winx_heap_free(f->name);
		if(f->path)
			winx_heap_free(f->path);
		winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
		if(f->next == filelist) break;
	}
	winx_list_destroy((list_entry **)(void *)&filelist);
}

/** @} */
