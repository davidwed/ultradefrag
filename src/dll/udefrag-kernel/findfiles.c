/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
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
 * @file findfiles.c
 * @brief Universal file system scanning code.
 * @addtogroup UniversalFileSystemScan
 * @{
 */

#include "globals.h"

BOOLEAN AddFile(short *path,PFILE_BOTH_DIR_INFORMATION pFileInfo);
BOOLEAN UnwantedStuffOnFatOrUdfDetected(PFILE_BOTH_DIR_INFORMATION pFileInfo,PFILENAME pfn);
BOOLEAN ConsoleUnwantedStuffDetected(WCHAR *Path,ULONG *InsideFlag);

/**
 * @brief Searches recursively for files on the path.
 * @param[in] ParentDirectoryPath the full path of the
 *            directory which must be scanned.
 * @return Zero for success, negative value otherwise.
 * @note Here we are skipping as much files as
 * possible, because this dramatically descreases
 * the volume analysis time.
 */
int FindFiles(WCHAR *ParentDirectoryPath)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	PFILE_BOTH_DIR_INFORMATION pFileInfoFirst = NULL, pFileInfo;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING us;
	NTSTATUS Status;
	HANDLE DirectoryHandle;
	WCHAR *Path = NULL;
	#define PATH_BUFFER_LENGTH (MAX_PATH + 5)
	static WCHAR FileName[MAX_PATH]; /* static is used to allocate this variable in global space */
	USHORT FileNameLength;
	ULONG inside_flag = TRUE;

	/* allocate memory */
	pFileInfoFirst = (PFILE_BOTH_DIR_INFORMATION)winx_heap_alloc(
		FIND_DATA_SIZE + sizeof(PFILE_BOTH_DIR_INFORMATION));
	if(!pFileInfoFirst){
		DebugPrint("Cannot allocate memory for FILE_BOTH_DIR_INFORMATION structure!\n");
		return UDEFRAG_NO_MEM;
	}
	Path = (WCHAR *)winx_heap_alloc(PATH_BUFFER_LENGTH * sizeof(WCHAR));
	if(Path == NULL){
		DebugPrint("Cannot allocate memory for FILE_BOTH_DIR_INFORMATION structure!\n");
		winx_heap_free(pFileInfoFirst);
		return UDEFRAG_NO_MEM;
	}

	/* open directory */
	RtlInitUnicodeString(&us,ParentDirectoryPath);
	InitializeObjectAttributes(&ObjectAttributes,&us,0,NULL,NULL);
	Status = NtCreateFile(&DirectoryHandle,FILE_LIST_DIRECTORY | FILE_RESERVE_OPFILTER,
			    &ObjectAttributes,&IoStatusBlock,NULL,0,
			    FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,
				FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
				NULL,0);
	if(Status != STATUS_SUCCESS){
		DebugPrint1("Cannot open directory: %ws: %x\n",ParentDirectoryPath,(UINT)Status);
		DirectoryHandle = NULL;	winx_heap_free(pFileInfoFirst); winx_heap_free(Path);
		return (-1);
	}

	/* Query information about files */
	pFileInfo = pFileInfoFirst;
	pFileInfo->FileIndex = 0;
	pFileInfo->NextEntryOffset = 0;
	while(!CheckForStopEvent()){
		if (pFileInfo->NextEntryOffset != 0){
			pFileInfo = (PVOID)((char *)(ULONG_PTR)pFileInfo + pFileInfo->NextEntryOffset);
		} else {
			pFileInfo = (PVOID)((char *)(ULONG_PTR)pFileInfoFirst + sizeof(PFILE_BOTH_DIR_INFORMATION));
			pFileInfo->FileIndex = 0;
			RtlZeroMemory(pFileInfo,FIND_DATA_SIZE);
			Status = NtQueryDirectoryFile(DirectoryHandle,NULL,NULL,NULL,
									&IoStatusBlock,(PVOID)pFileInfo,
									FIND_DATA_SIZE,
									FileBothDirectoryInformation,
									FALSE, /* ReturnSingleEntry */
									NULL,
									FALSE); /* RestartScan */
			if(Status != STATUS_SUCCESS) break; /* no more items */
		}

		FileNameLength = (USHORT)(pFileInfo->FileNameLength / sizeof(WCHAR));
		if(FileNameLength > (MAX_PATH - 1)) continue; /* really it must be no longer than 255 characters */

		/* here we can use the FileName variable */
		(void)wcsncpy(FileName,pFileInfo->FileName,FileNameLength);
		FileName[FileNameLength] = 0;
		
		/* skip . and .. */
		if(wcscmp(FileName,L".") == 0 || wcscmp(FileName,L"..") == 0) continue;

		/* Path = ParentDirectoryPath + FileName */
		if(ParentDirectoryPath[wcslen(ParentDirectoryPath) - 1] == '\\'){
			/* rootdir contains closing backslash, other directories aren't enclosed */
			(void)_snwprintf(Path,PATH_BUFFER_LENGTH,L"%s%s",ParentDirectoryPath,FileName);
		} else {
			(void)_snwprintf(Path,PATH_BUFFER_LENGTH,L"%s\\%s",ParentDirectoryPath,FileName);
		}
		Path[PATH_BUFFER_LENGTH - 1] = 0;
		/* and here we cannot use the FileName variable! */

		/* VERY IMPORTANT: skip reparse points */
		/* FIXME: what is reparse point? How to detect these that represents another volumes? */
		if(IS_REPARSE_POINT(pFileInfo)){
			DebugPrint("Reparse point found %ws\n",Path);
			continue;
		}

		/*
		* Skip temporary files:
		* Not applicable for the volume optimization.
		*
		* Skip hard links:
		* It seems that we don't have any simple way to get information 
		* on hard links. So we couldn't skip them, although there is safe enough.
		*/

		/* UltraDefrag has a full support for sparse files! :D */
		if(IS_SPARSE_FILE(pFileInfo)){
			DebugPrint("Sparse file found %ws\n",Path);
			/* Let's defragment them! :) */
		}

		if(IS_ENCRYPTED_FILE(pFileInfo)){
			DebugPrint2("Encrypted file found %ws\n",Path);
		}

		if(ConsoleUnwantedStuffDetected(Path,&inside_flag)){
			//DbgPrint("excluded = %ws\n",dx->tmp_buf);
			continue;
		}

		/*if(IS_DIR(pFileInfo) && IS_COMPRESSED(pFileInfo))
			DebugPrint("-Ultradfg- Compressed directory found %ws\n",Path);
		*/

		if(IS_DIR(pFileInfo)) (void)FindFiles(Path);      /* directory found */
		else if(!pFileInfo->EndOfFile.QuadPart) continue; /* empty file found */
		
		/* skip parent directories in context menu handler */
		if(context_menu_handler && !inside_flag) continue;

		if(!AddFile(Path,pFileInfo)){
			DebugPrint("InsertFileName failed for %ws\n",Path);
			NtClose(DirectoryHandle);
			winx_heap_free(pFileInfoFirst);
			winx_heap_free(Path);
			return UDEFRAG_NO_MEM;
		}
	}

	NtClose(DirectoryHandle);
	winx_heap_free(pFileInfoFirst);
	winx_heap_free(Path);
    return 0;
}

/**
 * @brief Adds a file to the file list.
 * @param[in] path the full path of the file.
 * @param[in] pFileInfo pointer to the structure
 * containing a file attributes.
 * @return Boolean value: TRUE for success, FALSE otherwise.
 * @note Adds a file only if its information needs to be cached.
 */
BOOLEAN AddFile(short *path,PFILE_BOTH_DIR_INFORMATION pFileInfo)
{
	PFILENAME pfn;
	ULONGLONG filesize;

	pfn = (PFILENAME)winx_list_insert_item((list_entry **)(void *)&filelist,NULL,sizeof(FILENAME));
	if(pfn == NULL) return FALSE;
	
	/* Initialize pfn->name field. */
	if(!RtlCreateUnicodeString(&pfn->name,path)){
		DebugPrint2("No enough memory for pfn->name initialization!\n");
		winx_list_remove_item((list_entry **)(void *)&filelist,(list_entry *)pfn);
		return FALSE;
	}

	/* Set flags. */
	pfn->is_dir = IS_DIR(pFileInfo);
	pfn->is_compressed = IS_COMPRESSED(pFileInfo);
	pfn->is_reparse_point = IS_REPARSE_POINT(pFileInfo);

	/*
	* This code fails for 3.99 Gb files on FAT32 volumes with 32k cluster size (tested on 32-bit XP),
	* because pFileInfo->AllocationSize member holds zero value in this case.
	*/
	/*if(dx->sizelimit && \
		(ULONGLONG)(pFileInfo->AllocationSize.QuadPart) > dx->sizelimit)
		pfn->is_overlimit = TRUE;
	else
		pfn->is_overlimit = FALSE;
	*/

	if(UnwantedStuffOnFatOrUdfDetected(pFileInfo,pfn)) pfn->is_filtered = TRUE;
	else pfn->is_filtered = FALSE;

	/* Dump the file. */
	if(!DumpFile(pfn)){
		/* skip files with unknown state */
		RtlFreeUnicodeString(&pfn->name);
		winx_list_remove_item((list_entry **)(void *)&filelist,(list_entry *)pfn);
		return TRUE;
	}

	filesize = pfn->clusters_total * bytes_per_cluster;
	if(sizelimit && filesize > sizelimit) pfn->is_overlimit = TRUE;
	else pfn->is_overlimit = FALSE;
	
	/* mark some files as filtered out */
	CHECK_FOR_FRAGLIMIT(pfn);

	/* FIXME: it shows approx. 1.6 Gb instead of 3.99 Gb */
	if(wcsstr(path,L"largefile"))
		DebugPrint("SIZE = %I64u\n", filesize);

	/* Update statistics and cluster map. */
	Stat.filecounter ++;
	if(pfn->is_dir) Stat.dircounter ++;
	if(pfn->is_compressed) Stat.compressedcounter ++;
	/* skip here filtered out and big files and reparse points */
	if(pfn->is_fragm && !pfn->is_reparse_point &&
	  ((!pfn->is_filtered && !pfn->is_overlimit) || optimize_flag)
	  ){
		Stat.fragmfilecounter ++;
		Stat.fragmcounter += pfn->n_fragments;
	} else {
		Stat.fragmcounter ++;
	}
	Stat.processed_clusters += pfn->clusters_total;
	MarkFileSpace(pfn,SYSTEM_SPACE);

	if(optimize_flag || pfn->is_fragm) return TRUE;

	/* 7. Destroy useless data. */
	DeleteBlockmap(pfn);
	RtlFreeUnicodeString(&pfn->name);
	winx_list_remove_item((list_entry **)(void *)&filelist,(list_entry *)pfn);
	return TRUE;
}

/**
 * @brief Adds a file to the list of fragmented files.
 * @param[in] pfn pointer to the structure describing the file.
 * @return Boolean value: TRUE for success, FALSE otherwise.
 */
BOOLEAN AddFileToFragmented(PFILENAME pfn)
{
	PFRAGMENTED pf, prev_pf = NULL;
	
	for(pf = fragmfileslist; pf != NULL; pf = pf->next_ptr){
		if(pf->pfn->n_fragments <= pfn->n_fragments){
			if(pf != fragmfileslist) prev_pf = pf->prev_ptr;
			break;
		}
		if(pf->next_ptr == fragmfileslist){
			prev_pf = pf;
			break;
		}
	}

	pf = (PFRAGMENTED)winx_list_insert_item((list_entry **)(void *)&fragmfileslist,(list_entry *)prev_pf,sizeof(FRAGMENTED));
	if(!pf){
		DebugPrint2("Cannot allocate memory for InsertFragmentedFile()!\n");
		return FALSE;
	}
	pf->pfn = pfn;
	return TRUE;
}

/**
 * @brief Removes all unfragmented files from the list of fragmented files.
 */
void UpdateFragmentedFilesList(void)
{
	PFRAGMENTED pf, next_pf, head;
	
	head = fragmfileslist;
	for(pf = fragmfileslist; pf != NULL;){
		next_pf = pf->next_ptr;
		if(!pf->pfn->is_fragm){
			winx_list_remove_item((list_entry **)(void *)&fragmfileslist,(list_entry *)pf);
			if(fragmfileslist == NULL) break;
			if(fragmfileslist != head){
				head = fragmfileslist;
				pf = next_pf;
				continue;
			}
		}
		pf = next_pf;
		if(pf == head) break;
	}
}

/**
 * @brief Checks, must file be skipped or not.
 * @param[in] pFileInfo pointer to the structure
 * containing a file attributes.
 * @param[in] pfn pointer to the structure
 * describing the file.
 * @return Boolean value. TRUE indicates that
 * the file must be skipped, FALSE indicates contrary.
 */
BOOLEAN UnwantedStuffOnFatOrUdfDetected(PFILE_BOTH_DIR_INFORMATION pFileInfo,PFILENAME pfn)
{
	UNICODE_STRING us;

	/* skip temporary files ;-) */
	if(IS_TEMPORARY_FILE(pFileInfo)){
		DebugPrint2("Temporary file found %ws\n",pfn->name.Buffer);
		return TRUE;
	}
	
	/* skip all unwanted files by user defined patterns */
	if(!RtlCreateUnicodeString(&us,pfn->name.Buffer)){
		DebugPrint2("Cannot allocate memory for UnwantedStuffDetected()!\n");
		return FALSE;
	}
	(void)_wcslwr(us.Buffer);

	if(in_filter.buffer){
		if(!IsStringInFilter(us.Buffer,&in_filter)){
			RtlFreeUnicodeString(&us); return TRUE; /* not included */
		}
	}

	if(ex_filter.buffer){
		if(IsStringInFilter(us.Buffer,&ex_filter)){
			RtlFreeUnicodeString(&us); return TRUE; /* excluded */
		}
	}
	RtlFreeUnicodeString(&us);

	return FALSE;
}

/**
 * @brief Checks, must file be skipped or not
 *        in case when cluster map is not used.
 * @param[in] Path the full path of the file.
 * @param[out] InsideFlag a boolean value
 * indicating are we inside the directory 
 * the context menu handler is running for, or not.
 * @return Boolean value. TRUE indicates that
 * the file must be skipped, FALSE indicates contrary.
 * @note InsideFlag is applicable only for context menu handler.
 * TRUE indicates that we are inside the selected directory
 * and we need to save information about the file.
 * FALSE indicates that we are going in right direction
 * and we must scan specified parent directory for files,
 * but we don't need to save information about 
 * this parent directory itself.
 */
BOOLEAN ConsoleUnwantedStuffDetected(WCHAR *Path,ULONG *InsideFlag)
{
	WCHAR *lpath = NULL;
	ULONG path_length;

	/* let's assume that we are inside directory selected in context menu handler */
	if(InsideFlag) *InsideFlag = TRUE;
	
	if(new_cluster_map || optimize_flag) return FALSE;
	
	/* allocate memory for Path converted to lowercase */
	path_length = wcslen(Path);
	if(path_length < 4) return FALSE; /* because must contain \??\ sequence */
	lpath = (WCHAR *)winx_heap_alloc((path_length + 1) * sizeof(WCHAR));
	if(lpath == NULL){
		DebugPrint("Cannot allocate memory for ConsoleUnwantedStuffDetected()!\n");
		return FALSE;
	}
	(void)wcscpy(lpath,Path);
	(void)_wcslwr(lpath);
	
	/* USE THE FOLLOWING CODE ONLY FOR THE CONTEXT MENU HANDLER: */
	if(context_menu_handler){
		/* is the current file placed in directory selected in context menu? */
		/* in other words: are we inside the selected folder? */
		if(!wcsstr(lpath,in_filter.buffer + in_filter.offsets->offset)){
			if(InsideFlag) *InsideFlag = FALSE;
			/* is current path a part of the path selected in context menu? */
			/* in other words: are we going in right direction? */
			if(!wcsstr(in_filter.buffer + in_filter.offsets->offset,lpath + 4)){
				DebugPrint1("Not included: %ws\n",Path);
				winx_heap_free(lpath);
				return TRUE;
			}
		}
	}

	/*
	* To speed up the analysis in console/native apps 
	* set UD_EX_FILTER option, to speed up both analysis 
	* and defragmentation set UD_IN_FILTER and UD_EX_FILTER options.
	*/
	if(ex_filter.buffer){
		if(IsStringInFilter(lpath,&ex_filter)){
			DebugPrint1("Excluded: %ws\n",Path);
			winx_heap_free(lpath);
			return TRUE;
		}
	}
	winx_heap_free(lpath);
	return FALSE;
}

/** @} */
