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
					   PFILE_BOTH_DIR_INFORMATION pFileInfo,BOOLEAN is_root);
BOOLEAN InsertFragmentedFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);

/* FindFiles() - recursive search of all files on specified path. */
BOOLEAN FindFiles(UDEFRAG_DEVICE_EXTENSION *dx,UNICODE_STRING *path,BOOLEAN is_root)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	PFILE_BOTH_DIR_INFORMATION pFileInfoFirst = NULL, pFileInfo;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	UNICODE_STRING new_path;
	HANDLE DirectoryHandle;

	/* Allocate memory */
	pFileInfoFirst = (PFILE_BOTH_DIR_INFORMATION)AllocatePool(NonPagedPool,
		FIND_DATA_SIZE + sizeof(PFILE_BOTH_DIR_INFORMATION));
	if(!pFileInfoFirst){
		DbgPrintNoMem();
		goto fail;
	}
	/* Open directory */
	InitializeObjectAttributes(&ObjectAttributes,path,0,NULL,NULL);
	Status = ZwCreateFile(&DirectoryHandle,FILE_LIST_DIRECTORY | FILE_RESERVE_OPFILTER,
			    &ObjectAttributes,&IoStatusBlock,NULL,0,
			    FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,
				FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
				NULL,0);
	if(Status != STATUS_SUCCESS){
		DebugPrint1("-Ultradfg- Can't open %ws %x\n",path->Buffer,(UINT)Status);
		DirectoryHandle = NULL;
		goto fail;
	}

	/* Query information about files */
	pFileInfo = pFileInfoFirst;
	pFileInfo->FileIndex = 0;
	pFileInfo->NextEntryOffset = 0;
	while(!KeReadStateEvent(&dx->stop_event)){
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
			if(Status != STATUS_SUCCESS) goto no_more_items;
		}
		/* Interpret information */
		if(pFileInfo->FileName[0] == 0x002E)
			if(pFileInfo->FileName[1] == 0x002E || pFileInfo->FileNameLength == sizeof(short))
				continue;	/* for . and .. */
		/* VERY IMPORTANT: skip reparse points */
		if(IS_REPARSE_POINT(pFileInfo)) continue;
		/* FIXME: hard links? */
		/* FIXME: sparse files? */
		wcsncpy(dx->tmp_buf,path->Buffer,(path->Length) >> 1);
		dx->tmp_buf[(path->Length) >> 1] = 0;
		if(!is_root)
			wcscat(dx->tmp_buf,L"\\");
		wcsncat(dx->tmp_buf,pFileInfo->FileName,(pFileInfo->FileNameLength) >> 1);
		if(!RtlCreateUnicodeString(&new_path,dx->tmp_buf)){
			DbgPrintNoMem();
			ZwClose(DirectoryHandle);
			goto fail;
		}

		if(IS_DIR(pFileInfo)){
			/*if(!*/FindFiles(dx,&new_path,FALSE)/*) goto fail*/;
			if(KeReadStateEvent(&dx->stop_event) == 0x1)
				goto no_more_items;
		} else {
			if(!pFileInfo->EndOfFile.QuadPart) goto next; /* file is empty */
		}
		if(!InsertFileName(dx,new_path.Buffer,pFileInfo,is_root)){
			DebugPrint1("-Ultradfg- InsertFileName failed for %ws\n",new_path.Buffer);
			ZwClose(DirectoryHandle);
			RtlFreeUnicodeString(&new_path);
			goto fail;
		}
next:
		RtlFreeUnicodeString(&new_path);
	}
no_more_items:
	ZwClose(DirectoryHandle);
	ExFreePool(pFileInfoFirst);
    return TRUE;
fail:
	ExFreePoolSafe(pFileInfoFirst);
	return FALSE;
}

/* inserts the new FILENAME structure to filelist */
/* Returns TRUE on success and FALSE if no enough memory */
BOOLEAN InsertFileName(UDEFRAG_DEVICE_EXTENSION *dx,short *path,
					   PFILE_BOTH_DIR_INFORMATION pFileInfo,BOOLEAN is_root)
{
	PFILENAME pfn;

	/* Add file name with path to filelist */
	pfn = (PFILENAME)InsertFirstItem((PLIST *)&dx->filelist,sizeof(FILENAME));
	if(!pfn){
		DbgPrintNoMem();
		return FALSE;
	}
	if(!RtlCreateUnicodeString(&pfn->name,path)){
		DbgPrintNoMem();
		dx->filelist = pfn->next_ptr;
		ExFreePool(pfn);
		return TRUE;
	}
	pfn->is_dir = IS_DIR(pFileInfo);
	pfn->is_compressed = IS_COMPRESSED(pFileInfo);
	if(dx->sizelimit && \
		(unsigned __int64)(pFileInfo->AllocationSize.QuadPart) > dx->sizelimit)
		pfn->is_overlimit = TRUE;
	else
		pfn->is_overlimit = FALSE;
	if(!DumpFile(dx,pfn)){
no_mem:
		dx->filelist = pfn->next_ptr;
		RtlFreeUnicodeString(&pfn->name);
		ExFreePool(pfn);
	} else {
		if(pfn->is_fragm) {
			if(!InsertFragmentedFile(dx,pfn)) {
				dx->fragmfilecounter --;
				dx->fragmcounter -= pfn->n_fragments;
				goto no_mem;
			}
		}
		dx->filecounter ++;
		if(pfn->is_dir) dx->dircounter ++;
		if(pfn->is_compressed) dx->compressedcounter ++;
	}
	return TRUE;
}

/* inserts the new structure to list of fragmented files */
/* Returns TRUE on success and FALSE if no enough memory */
BOOLEAN InsertFragmentedFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	PFRAGMENTED pf,plist;

	pf = (PFRAGMENTED)AllocatePool(NonPagedPool,sizeof(FRAGMENTED));
	if(!pf){
		DbgPrintNoMem();
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

