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
* FindFiles() and related functions.
*/

#include "driver.h"

BOOLEAN InsertFileName(UDEFRAG_DEVICE_EXTENSION *dx,short *path,
					   PFILE_BOTH_DIR_INFORMATION pFileInfo);
BOOLEAN InsertFragmentedFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);

/* FindFiles() - recursive search of all files on specified path. */
BOOLEAN FindFiles(UDEFRAG_DEVICE_EXTENSION *dx,UNICODE_STRING *path)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	PFILE_BOTH_DIR_INFORMATION pFileInfoFirst = NULL, pFileInfo;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	UNICODE_STRING new_path, temp_path, temp_win32_path;
	HANDLE DirectoryHandle;
	unsigned int length;

	/* Allocate memory */
	pFileInfoFirst = (PFILE_BOTH_DIR_INFORMATION)AllocatePool(NonPagedPool,
		FIND_DATA_SIZE + sizeof(PFILE_BOTH_DIR_INFORMATION));
	if(!pFileInfoFirst){
		DebugPrint("-Ultradfg- cannot allocate memory for FILE_BOTH_DIR_INFORMATION structure!\n",NULL);
		return FALSE;
	}
	/* Open directory */
	InitializeObjectAttributes(&ObjectAttributes,path,0,NULL,NULL);
	Status = ZwCreateFile(&DirectoryHandle,FILE_LIST_DIRECTORY | FILE_RESERVE_OPFILTER,
			    &ObjectAttributes,&IoStatusBlock,NULL,0,
			    FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,
				FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
				NULL,0);
	if(Status != STATUS_SUCCESS){
		DebugPrint1("-Ultradfg- cannot open directory: %x\n",path->Buffer,(UINT)Status);
		DirectoryHandle = NULL;	ExFreePoolSafe(pFileInfoFirst);
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

		/* skip . and .. */
		if(!pFileInfo->FileNameLength) continue;
		if(pFileInfo->FileNameLength == sizeof(short) && pFileInfo->FileName[0] == 0x002E)
			continue;
		if(pFileInfo->FileName[0] == 0x002E && pFileInfo->FileName[1] == 0x002E)
			continue;

		length = min(path->Length >> 1,TEMP_BUFFER_CHARS - 2);
		if(length == (TEMP_BUFFER_CHARS - 2)){
			DebugPrint("-Ultradfg- path->Buffer is too long: %u bytes!\n",NULL,path->Length);
			continue;
		}
		wcsncpy(dx->tmp_buf,path->Buffer,length);
		dx->tmp_buf[length] = 0;

		/* rootdir contains closing backslash, other directories aren't enclosed */
		if(dx->tmp_buf[length - 1] != '\\'){
			wcscat(dx->tmp_buf,L"\\");
			length ++;
		}

		if((pFileInfo->FileNameLength >> 1) > (TEMP_BUFFER_CHARS - 1 - length)){
			DebugPrint("-Ultradfg- resulting path is too long: %u bytes!\n",NULL,
				length + (pFileInfo->FileNameLength >> 1));
			continue;
		}
		wcsncat(dx->tmp_buf,pFileInfo->FileName,(pFileInfo->FileNameLength) >> 1);

		/* VERY IMPORTANT: skip reparse points */
		/* FIXME: what is reparse point? How to detect these that represents another volumes? */
		if(IS_REPARSE_POINT(pFileInfo)){
			DebugPrint("-Ultradfg- Reparse point found\n",dx->tmp_buf);
			continue;
		}

		/* skip temporary files */
		if(IS_TEMPORARY_FILE(pFileInfo)){
			DebugPrint2("-Ultradfg- Temporary file found\n",dx->tmp_buf);
			continue;
		}

		/*
		* Skip hard links:
		* It seems that we don't have any simple way to get information 
		* on hard links. So we couldn't skip them, although there is safe enough.
		*/
		/*if(IS_HARD_LINK(pFileInfo)){
			DebugPrint("-Ultradfg- Hard link found\n",dx->tmp_buf);
		}
		if(wcsstr(dx->tmp_buf,L"HardLink")){
			DebugPrint("Attributes = %x\n",dx->tmp_buf,pFileInfo->FileAttributes);
		}*/

		/* UltraDefrag has a full support for sparse files! :D */
		if(IS_SPARSE_FILE(pFileInfo)){
			DebugPrint("-Ultradfg- Sparse file found\n",dx->tmp_buf);
			/* Let's defragment them! :) */
		}

		if(IS_ENCRYPTED_FILE(pFileInfo)){
			DebugPrint2("-Ultradfg- Encrypted file found\n",dx->tmp_buf);
		}

		/*
		* If we don't need to redraw a cluster map and the next operation
		* will not be the volume optimization, we can skip all filtered out files!
		*/
		if(!new_cluster_map && !dx->compact_flag){
			if(RtlCreateUnicodeString(&temp_path,dx->tmp_buf)){
				_wcslwr(temp_path.Buffer);
				/* skip all filtered out files */
				/* USE THE FOLLOWING CODE ONLY FOR CONTEXT MENU HANDLER: */
				if(context_menu_handler){
					if(RtlCreateUnicodeString(&temp_win32_path,dx->tmp_buf + 4)){
						_wcslwr(temp_win32_path.Buffer);
						/*
						* temp_win32_path contains the full path of the current file 
						* without leading '\??\' sequence
						*/
						/* is the current file placed in directory selected in context menu? */
						/* in other words: are we inside the selected folder? */
						if(!wcsstr(temp_win32_path.Buffer,
						  dx->in_filter.buffer + dx->in_filter.offsets->offset)){
							/* is current path a part of the path selected in context menu? */
							/* in other words: are we going in right direction? */
							if(!wcsstr(dx->in_filter.buffer + dx->in_filter.offsets->offset,
							  temp_win32_path.Buffer)){
								DebugPrint("-Ultradfg- Not included:\n",temp_path.Buffer);
								RtlFreeUnicodeString(&temp_win32_path);
								RtlFreeUnicodeString(&temp_path); continue;
							}
						}
						RtlFreeUnicodeString(&temp_win32_path);
					}else{
						DebugPrint2("-Ultradfg- cannot allocate memory for the temp_win32_path !\n",NULL);
					}
				}
				/*
				* To speed up the analysis in console/native apps 
				* set UD_EX_FILTER option, to speed up both analysis 
				* and defragmentation set UD_IN_FILTER and UD_EX_FILTER options.
				*/
				if(dx->ex_filter.buffer){
					if(IsStringInFilter(temp_path.Buffer,&dx->ex_filter)){
						DebugPrint("-Ultradfg- Excluded:\n",temp_path.Buffer);
						RtlFreeUnicodeString(&temp_path); continue;
					}
				}
				RtlFreeUnicodeString(&temp_path);
			} else DebugPrint2("-Ultradfg- cannot allocate memory for the temp_path !\n",NULL);
		}
		
		if(!RtlCreateUnicodeString(&new_path,dx->tmp_buf)){
			DebugPrint2("-Ultradfg- cannot allocate memory for the new_path!\n",NULL);
			ZwClose(DirectoryHandle); ExFreePoolSafe(pFileInfoFirst);
			return FALSE;
		}

		if(IS_DIR(pFileInfo)){
			FindFiles(dx,&new_path);
		} else {
			if(!pFileInfo->EndOfFile.QuadPart){ /* file is empty */
				RtlFreeUnicodeString(&new_path); continue;
			}
		}
		
		if(!InsertFileName(dx,new_path.Buffer,pFileInfo)){
			DebugPrint("-Ultradfg- InsertFileName failed for\n",new_path.Buffer);
			ZwClose(DirectoryHandle);
			RtlFreeUnicodeString(&new_path); ExFreePoolSafe(pFileInfoFirst);
			return FALSE;
		}
		RtlFreeUnicodeString(&new_path);
	}

	ZwClose(DirectoryHandle);
	Nt_ExFreePool(pFileInfoFirst);
    return TRUE;
}

/* inserts the new FILENAME structure to filelist */
/* Returns TRUE on success and FALSE if no enough memory */
BOOLEAN InsertFileName(UDEFRAG_DEVICE_EXTENSION *dx,short *path,
					   PFILE_BOTH_DIR_INFORMATION pFileInfo)
{
	PFILENAME pfn;

	/* Add file name with path to filelist */
	pfn = (PFILENAME)InsertFirstItem((PLIST *)&dx->filelist,sizeof(FILENAME));
	if(!pfn) return FALSE;
	if(!RtlCreateUnicodeString(&pfn->name,path)){
		DebugPrint2("-Ultradfg- no enough memory for pfn->name initialization!\n",NULL);
		dx->filelist = pfn->next_ptr;
		Nt_ExFreePool(pfn);
		return FALSE;
	}
	pfn->is_dir = IS_DIR(pFileInfo);
	pfn->is_compressed = IS_COMPRESSED(pFileInfo);
	if(dx->sizelimit && \
		(unsigned __int64)(pFileInfo->AllocationSize.QuadPart) > dx->sizelimit)
		pfn->is_overlimit = TRUE;
	else
		pfn->is_overlimit = FALSE;
	if(DumpFile(dx,pfn)){
		if(pfn->is_fragm){
			if(!InsertFragmentedFile(dx,pfn)){
				dx->fragmfilecounter --;
				dx->fragmcounter -= pfn->n_fragments;
				goto fail;
			}
		}
		dx->filecounter ++;
		if(pfn->is_dir) dx->dircounter ++;
		if(pfn->is_compressed) dx->compressedcounter ++;
	}
	return TRUE;

fail:
	dx->filelist = pfn->next_ptr;
	RtlFreeUnicodeString(&pfn->name);
	Nt_ExFreePool(pfn);
	return FALSE;
}

/* inserts the new structure to list of fragmented files */
/* Returns TRUE on success and FALSE if no enough memory */
BOOLEAN InsertFragmentedFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	PFRAGMENTED pf,plist;

	pf = (PFRAGMENTED)AllocatePool(NonPagedPool,sizeof(FRAGMENTED));
	if(!pf){
		DebugPrint2("-Ultradfg- cannot allocate memory for InsertFragmentedFile()!\n",NULL);
		return FALSE;
	}
	pf->pfn = pfn;
	plist = dx->fragmfileslist;
	while(plist){
		if(plist->pfn->n_fragments > pfn->n_fragments){
			if(!plist->next_ptr){
				plist->next_ptr = pf;
				pf->next_ptr = NULL;
				break;
			}
			if(plist->next_ptr->pfn->n_fragments <= pfn->n_fragments){
				pf->next_ptr = plist->next_ptr;
				plist->next_ptr = pf;
				break;
			}
		}
		plist = plist->next_ptr;
	}
	if(!plist){
		pf->next_ptr = dx->fragmfileslist;
		dx->fragmfileslist = pf;
	}
	ApplyFilter(dx,pfn);
	return TRUE;
}

/* removes unfragmented files from the list of fragmented files */
void UpdateFragmentedFilesList(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFRAGMENTED pf,prev_pf;

	pf = dx->fragmfileslist;
	prev_pf = NULL;
	while(pf){
		if(!pf->pfn->is_fragm){
			pf = (PFRAGMENTED)RemoveItem((PLIST *)&dx->fragmfileslist,
				(PLIST *)(void *)&prev_pf,(PLIST *)(void *)&pf);
		} else {
			prev_pf = pf;
			pf = pf->next_ptr;
		}
	}
}

