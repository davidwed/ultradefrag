/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* FindFiles() and related functions.
*/

#include "driver.h"

BOOLEAN InsertFileName(UDEFRAG_DEVICE_EXTENSION *dx,short *path,
					   PFILE_BOTH_DIR_INFORMATION pFileInfo);
BOOLEAN UnwantedStuffOnFatOrUdfDetected(UDEFRAG_DEVICE_EXTENSION *dx,
		PFILE_BOTH_DIR_INFORMATION pFileInfo,PFILENAME pfn);

/* FindFiles() - recursive search of all files on specified path. */
BOOLEAN FindFiles(UDEFRAG_DEVICE_EXTENSION *dx,WCHAR *ParentDirectoryPath)
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
	pFileInfoFirst = (PFILE_BOTH_DIR_INFORMATION)AllocatePool(PagedPool,
		FIND_DATA_SIZE + sizeof(PFILE_BOTH_DIR_INFORMATION));
	if(!pFileInfoFirst){
		DebugPrint("-Ultradfg- cannot allocate memory for FILE_BOTH_DIR_INFORMATION structure!\n",NULL);
		return FALSE;
	}
	Path = (WCHAR *)AllocatePool(PagedPool,PATH_BUFFER_LENGTH * sizeof(WCHAR));
	if(Path == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for FILE_BOTH_DIR_INFORMATION structure!\n",NULL);
		Nt_ExFreePool(pFileInfoFirst);
		return FALSE;
	}

	/* open directory */
	RtlInitUnicodeString(&us,ParentDirectoryPath);
	if(nt4_system){
		InitializeObjectAttributes(&ObjectAttributes,&us,0,NULL,NULL);
	} else {
		InitializeObjectAttributes(&ObjectAttributes,&us,OBJ_KERNEL_HANDLE,NULL,NULL);
	}
	Status = ZwCreateFile(&DirectoryHandle,FILE_LIST_DIRECTORY | FILE_RESERVE_OPFILTER,
			    &ObjectAttributes,&IoStatusBlock,NULL,0,
			    FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,
				FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
				NULL,0);
	if(Status != STATUS_SUCCESS){
		DebugPrint1("-Ultradfg- cannot open directory: %x\n",ParentDirectoryPath,(UINT)Status);
		DirectoryHandle = NULL;	ExFreePoolSafe(pFileInfoFirst); ExFreePoolSafe(Path);
		return FALSE;
	}

	/* Query information about files */
	pFileInfo = pFileInfoFirst;
	pFileInfo->FileIndex = 0;
	pFileInfo->NextEntryOffset = 0;
	while(!KeReadStateEvent(&stop_event)){
		if (pFileInfo->NextEntryOffset != 0){
			pFileInfo = (PVOID)((ULONG_PTR)pFileInfo + pFileInfo->NextEntryOffset);
		} else {
			pFileInfo = (PVOID)((ULONG_PTR)pFileInfoFirst + sizeof(PFILE_BOTH_DIR_INFORMATION));
			pFileInfo->FileIndex = 0;
			Status = ZwQueryDirectoryFile(DirectoryHandle,NULL,NULL,NULL,
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

		/* here we can use FileName variable */
		wcsncpy(FileName,pFileInfo->FileName,FileNameLength);
		FileName[FileNameLength] = 0;
		
		/* skip . and .. */
		if(wcscmp(FileName,L".") == 0 || wcscmp(FileName,L"..") == 0) continue;

		/* store into Path buffer retrieved file name appended to ParentDirectoryPath */
		if(ParentDirectoryPath[wcslen(ParentDirectoryPath) - 1] == '\\'){
			/* rootdir contains closing backslash, other directories aren't enclosed */
			_snwprintf(Path,PATH_BUFFER_LENGTH,L"%s%s",ParentDirectoryPath,FileName);
		} else {
			_snwprintf(Path,PATH_BUFFER_LENGTH,L"%s\\%s",ParentDirectoryPath,FileName);
		}
		Path[PATH_BUFFER_LENGTH - 1] = 0;
		/* and here we cannot use FileName variable! */

		/* VERY IMPORTANT: skip reparse points */
		/* FIXME: what is reparse point? How to detect these that represents another volumes? */
		if(IS_REPARSE_POINT(pFileInfo)){
			DebugPrint("-Ultradfg- Reparse point found\n",Path);
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
			DebugPrint("-Ultradfg- Sparse file found\n",Path);
			/* Let's defragment them! :) */
		}

		if(IS_ENCRYPTED_FILE(pFileInfo)){
			DebugPrint2("-Ultradfg- Encrypted file found\n",Path);
		}

		if(ConsoleUnwantedStuffDetected(dx,Path,&inside_flag)){
			///DbgPrint("excluded = %ws\n",dx->tmp_buf);
			continue;
		}

		/*if(IS_DIR(pFileInfo) && IS_COMPRESSED(pFileInfo))
			DebugPrint("-Ultradfg- Compressed directory found\n",Path);
		*/

		if(IS_DIR(pFileInfo)) FindFiles(dx,Path);
		else if(!pFileInfo->EndOfFile.QuadPart) continue; /* file is empty */
		
		/* skip parent directories in context menu handler */
		if(context_menu_handler && !inside_flag) continue;

		if(!InsertFileName(dx,Path,pFileInfo)){
			DebugPrint("-Ultradfg- InsertFileName failed for\n",Path);
			ZwClose(DirectoryHandle);
			ExFreePoolSafe(pFileInfoFirst);
			ExFreePoolSafe(Path);
			return FALSE;
		}
	}

	ZwClose(DirectoryHandle);
	Nt_ExFreePool(pFileInfoFirst);
	Nt_ExFreePool(Path);
    return TRUE;
}

/* inserts the new FILENAME structure to filelist */
/* Returns TRUE on success and FALSE if no enough memory */
BOOLEAN InsertFileName(UDEFRAG_DEVICE_EXTENSION *dx,short *path,
					   PFILE_BOTH_DIR_INFORMATION pFileInfo)
{
	PFILENAME pfn;
	ULONGLONG filesize;

	/* Add a file only if we need to have its information cached. */
	pfn = (PFILENAME)InsertItem((PLIST *)&dx->filelist,NULL,sizeof(FILENAME),PagedPool);
	if(pfn == NULL) return FALSE;
	
	/* Initialize pfn->name field. */
	if(!RtlCreateUnicodeString(&pfn->name,path)){
		DebugPrint2("-Ultradfg- no enough memory for pfn->name initialization!\n",NULL);
		RemoveItem((PLIST *)&dx->filelist,(LIST *)pfn);
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

	if(UnwantedStuffOnFatOrUdfDetected(dx,pFileInfo,pfn)) pfn->is_filtered = TRUE;
	else pfn->is_filtered = FALSE;

	/* Dump the file. */
	if(!DumpFile(dx,pfn)){
		/* skip files with unknown state */
		RtlFreeUnicodeString(&pfn->name);
		RemoveItem((PLIST *)&dx->filelist,(LIST *)pfn);
		return TRUE;
	}

	filesize = pfn->clusters_total * dx->bytes_per_cluster;
	if(dx->sizelimit && filesize > dx->sizelimit) pfn->is_overlimit = TRUE;
	else pfn->is_overlimit = FALSE;

	if(wcsstr(path,L"largefile"))
		DbgPrint("SIZE = %I64u\n", filesize); /* shows approx. 1.6 Gb instead of 3.99 Gb */

	/* Update statistics and cluster map. */
	dx->filecounter ++;
	if(pfn->is_dir) dx->dircounter ++;
	if(pfn->is_compressed) dx->compressedcounter ++;
	/* skip here filtered out and big files and reparse points */
	if(pfn->is_fragm && !pfn->is_filtered && !pfn->is_overlimit && !pfn->is_reparse_point){
		dx->fragmfilecounter ++;
		dx->fragmcounter += pfn->n_fragments;
	} else {
		dx->fragmcounter ++;
	}
	dx->processed_clusters += pfn->clusters_total;
	MarkSpace(dx,pfn,SYSTEM_SPACE);

	if(dx->compact_flag || pfn->is_fragm) return TRUE;

	/* 7. Destroy useless data. */
	DeleteBlockmap(pfn);
	RtlFreeUnicodeString(&pfn->name);
	RemoveItem((PLIST *)&dx->filelist,(LIST *)pfn);
	return TRUE;
}

/* inserts the new structure to list of fragmented files */
/* Returns TRUE on success and FALSE if no enough memory */
BOOLEAN InsertFragmentedFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	PFRAGMENTED pf, prev_pf = NULL;
	
	for(pf = dx->fragmfileslist; pf != NULL; pf = pf->next_ptr){
		if(pf->pfn->n_fragments <= pfn->n_fragments){
			if(pf != dx->fragmfileslist) prev_pf = pf->prev_ptr;
			break;
		}
		if(pf->next_ptr == dx->fragmfileslist){
			prev_pf = pf;
			break;
		}
	}

	pf = (PFRAGMENTED)InsertItem((PLIST *)&dx->fragmfileslist,(PLIST)prev_pf,sizeof(FRAGMENTED),PagedPool);
	if(!pf){
		DebugPrint2("-Ultradfg- cannot allocate memory for InsertFragmentedFile()!\n",NULL);
		return FALSE;
	}
	pf->pfn = pfn;
	return TRUE;
}

/* removes unfragmented files from the list of fragmented files */
void UpdateFragmentedFilesList(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFRAGMENTED pf, next_pf, head;
	
	head = dx->fragmfileslist;
	for(pf = dx->fragmfileslist; pf != NULL;){
		next_pf = pf->next_ptr;
		if(!pf->pfn->is_fragm){
			RemoveItem((PLIST *)&dx->fragmfileslist,(PLIST)pf);
			if(dx->fragmfileslist == NULL) break;
			if(dx->fragmfileslist != head){
				head = dx->fragmfileslist;
				pf = next_pf;
				continue;
			}
		}
		pf = next_pf;
		if(pf == head) break;
	}
}

BOOLEAN UnwantedStuffOnFatOrUdfDetected(UDEFRAG_DEVICE_EXTENSION *dx,
		PFILE_BOTH_DIR_INFORMATION pFileInfo,PFILENAME pfn)
{
	UNICODE_STRING us;

	/* skip temporary files ;-) */
	if(IS_TEMPORARY_FILE(pFileInfo)){
		DebugPrint2("-Ultradfg- Temporary file found\n",pfn->name.Buffer);
		return TRUE;
	}
	
	/* skip all unwanted files by user defined patterns */
	if(!RtlCreateUnicodeString(&us,pfn->name.Buffer)){
		DebugPrint2("-Ultradfg- cannot allocate memory for UnwantedStuffDetected()!\n",NULL);
		return FALSE;
	}
	_wcslwr(us.Buffer);

	if(dx->in_filter.buffer){
		if(!IsStringInFilter(us.Buffer,&dx->in_filter)){
			RtlFreeUnicodeString(&us); return TRUE; /* not included */
		}
	}

	if(dx->ex_filter.buffer){
		if(IsStringInFilter(us.Buffer,&dx->ex_filter)){
			RtlFreeUnicodeString(&us); return TRUE; /* excluded */
		}
	}
	RtlFreeUnicodeString(&us);

	return FALSE;
}

/*
* If we don't need to redraw a cluster map and the next operation
* will not be the volume optimization, we can skip all filtered out files!
*/
/*
* InsideFlag is applicable only for context menu handler.
* TRUE indicates that we are inside the selected directory
* and we need to save information about specified file.
* FALSE indicates that we are going in right direction
* and we must scan specified parent directory for files,
* but we don't need to save information about 
* this parent directory itself.
*/
BOOLEAN ConsoleUnwantedStuffDetected(UDEFRAG_DEVICE_EXTENSION *dx,WCHAR *Path,ULONG *InsideFlag)
{
	WCHAR *lpath = NULL;
	ULONG path_length;

	/* let's assume that we are inside directory selected in context menu handler */
	if(InsideFlag) *InsideFlag = TRUE;
	
	if(new_cluster_map || dx->compact_flag) return FALSE;
	
	/* allocate memory for Path converted to lowercase */
	path_length = wcslen(Path);
	if(path_length < 4) return FALSE; /* because must contain \??\ sequence */
	lpath = (WCHAR *)AllocatePool(PagedPool,(path_length + 1) * sizeof(WCHAR));
	if(lpath == NULL){
		DebugPrint("-Ultradfg- cannot allocate memory for ConsoleUnwantedStuffDetected()!\n",NULL);
		return FALSE;
	}
	wcscpy(lpath,Path);
	_wcslwr(lpath);
	
	/* USE THE FOLLOWING CODE ONLY FOR CONTEXT MENU HANDLER: */
	if(context_menu_handler){
		/* is the current file placed in directory selected in context menu? */
		/* in other words: are we inside the selected folder? */
		if(!wcsstr(lpath,dx->in_filter.buffer + dx->in_filter.offsets->offset)){
			if(InsideFlag) *InsideFlag = FALSE;
			/* is current path a part of the path selected in context menu? */
			/* in other words: are we going in right direction? */
			if(!wcsstr(dx->in_filter.buffer + dx->in_filter.offsets->offset,lpath + 4)){
				DebugPrint1("-Ultradfg- Not included:\n",Path);
				ExFreePoolSafe(lpath);
				return TRUE;
			}
		}
	}

	/*
	* To speed up the analysis in console/native apps 
	* set UD_EX_FILTER option, to speed up both analysis 
	* and defragmentation set UD_IN_FILTER and UD_EX_FILTER options.
	*/
	if(dx->ex_filter.buffer){
		if(IsStringInFilter(lpath,&dx->ex_filter)){
			DebugPrint1("-Ultradfg- Excluded:\n",Path);
			ExFreePoolSafe(lpath);
			return TRUE;
		}
	}
	ExFreePoolSafe(lpath);
	return FALSE;
}
