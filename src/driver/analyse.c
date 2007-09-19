/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 *  Fragmentation analyse engine.
 */

#include "driver.h"
#if 0
ULONGLONG _rdtsc(void)
{
	ULONGLONG retval;

	__asm {
		rdtsc
		lea ebx, retval
		mov [ebx], eax
		mov [ebx+4], edx
	};
	return retval;
}
#endif
void PrepareDataFields(PEXAMPLE_DEVICE_EXTENSION dx)
{
	dx->free_space_map = NULL;
	dx->filelist = NULL;
	dx->fragmfileslist = NULL;
	dx->filecounter = 0;
	dx->dircounter = 0;
	dx->compressedcounter = 0;
	dx->free_space_map = NULL;
	dx->lastfreeblock = NULL;
	dx->fragmfilecounter = 0;
	dx->fragmcounter = 0;
	dx->clusters_total = 0;
	dx->cluster_map = 0;
	dx->total_space = 0; dx->free_space = 0;
	dx->hVol = NULL;
	dx->partition_type = 0x0;
	dx->mft_size = 0;
	dx->processed_clusters = 0;
	dx->unprocessed_blocks = 0;
	dx->no_checked_blocks = NULL;
	dx->status = STATUS_BEFORE_PROCESSING;
	dx->current_operation = 'A';
	dx->clusters_to_move = dx->clusters_to_move_initial = 0;
	dx->clusters_to_compact = dx->clusters_to_compact_initial = 0;
	dx->clusters_to_move_tmp = 0;
	dx->clusters_to_compact_tmp = 0;
}

/* Init() - initialize filename list and map of free space */
BOOLEAN Analyse(PEXAMPLE_DEVICE_EXTENSION dx)
{
	short path[50] = L"\\??\\A:";
	///short p[] = L"\\??\\A:\\FRAGLIST.HTM";
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	UNICODE_STRING pathU;
	//ULONGLONG tm;

	/* TODO: optimize FindFiles() */
	/* data initialization */
	FreeAllBuffers(dx);
	PrepareDataFields(dx);
	/* Clear the 'Stop' event. */
	KeClearEvent(&(dx->stop_event));

	/* 0. open the volume */
	DeleteLogFile(dx);
	path[4] = (short)(dx->letter);
	RtlInitUnicodeString(&pathU,path);
	InitializeObjectAttributes(&ObjectAttributes,&pathU,0,NULL,NULL);
	Status = ZwCreateFile(&(dx->hVol),FILE_GENERIC_READ | FILE_WRITE_DATA,
		&ObjectAttributes,&IoStatusBlock,
		NULL,0,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,
		NULL,0);
	if(!NT_SUCCESS(Status))
	{
		DebugPrint("-Ultradfg- Can't open volume %x\n",Status);
		goto fail;
	}

	/* 1. get number of clusters: free and total */
	if(!GetTotalClusters(dx)) goto fail;
	DebugPrint("-Ultradfg- total clusters: %I64u\n", dx->clusters_total);

	/* Synchronize drive.
	 *
	 * On NT 4.0 (at least under MS Virtual PC) it will crash system:
	 */
#ifndef NT4_TARGET 
	Status = __NtFlushBuffersFile(dx->hVol);
	if(Status)
		DebugPrint("-Ultradfg- Can't flush volume buffers %x\n",Status);
#endif
///tm = _rdtsc();
	if(!FillFreeClusterMap(dx)) goto fail;
///DebugPrint("%I64u\n", _rdtsc() - tm);
	/* 2. initialize file list and put their clusters to map */
	//tm = _rdtsc();
	wcscat(path,L"\\");
	if(!RtlCreateUnicodeString(&pathU,path))
	{
		DbgPrintNoMem();
		goto fail;
	}
	if(!FindFiles(dx,&pathU,TRUE))
	{
		RtlFreeUnicodeString(&pathU);
		goto fail;
	}
	RtlFreeUnicodeString(&pathU);
	if(!dx->filelist) goto fail;
	DebugPrint("-Ultradfg- Files found: %u\n",dx->filecounter);
	DebugPrint("-Ultradfg- Fragmented files: %u\n",dx->fragmfilecounter);
	//DebugPrint("Time: %I64u",_rdtsc() - tm);
	/* 3. If it's NTFS volume, we can locate MFT and put its clusters to map */
	ProcessMFT(dx);
	SaveFragmFilesListToDisk(dx);
	if(KeReadStateEvent(&(dx->stop_event)) == 0x0)
		dx->status = STATUS_ANALYSED;
	else
		dx->status = STATUS_BEFORE_PROCESSING;
	dx->clusters_to_move = dx->clusters_to_move_initial = dx->clusters_to_move_tmp;
	dx->clusters_to_compact_initial = dx->clusters_to_compact_tmp;
	return TRUE;
fail:
	return FALSE;
}

BOOLEAN GetTotalClusters(PEXAMPLE_DEVICE_EXTENSION dx)
{
	FILE_FS_SIZE_INFORMATION FileFsSize;
	IO_STATUS_BLOCK IoStatusBlock;
	unsigned short path[] = L"\\??\\A:\\";
	UNICODE_STRING NtPathU;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE hFile;
	NTSTATUS errCode;
	ULONGLONG bpc; /* bytes per cluster */

	path[4] = (unsigned short)dx->letter;
	RtlInitUnicodeString(&NtPathU,path);
	InitializeObjectAttributes(&ObjectAttributes,&NtPathU,
			       FILE_READ_ATTRIBUTES,NULL,NULL);

	errCode = ZwCreateFile(&hFile,FILE_GENERIC_READ,
			    &ObjectAttributes,&IoStatusBlock,NULL,0,
			    FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,
			    NULL,0);

	if(errCode) goto exit;

	errCode = ZwQueryVolumeInformationFile(hFile,&IoStatusBlock,&FileFsSize,
			  sizeof(FILE_FS_SIZE_INFORMATION),FileFsSizeInformation);
	ZwClose(hFile);
	if(errCode) goto exit;

	bpc = FileFsSize.SectorsPerAllocationUnit * FileFsSize.BytesPerSector;
	dx->bytes_per_cluster = bpc;
	dx->total_space = FileFsSize.TotalAllocationUnits.QuadPart * bpc;
	dx->free_space = FileFsSize.AvailableAllocationUnits.QuadPart * bpc;

	dx->clusters_total = (ULONGLONG)(FileFsSize.TotalAllocationUnits.QuadPart);
exit:
	return (errCode == STATUS_SUCCESS);
}

void ProcessMFT(PEXAMPLE_DEVICE_EXTENSION dx)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;
	PARTITION_INFORMATION part_info;
	NTFS_DATA ntfs_data;
	ULONGLONG start,len,/*i,*/mft_len = 0;

	Status = ZwDeviceIoControlFile(dx->hVol,NULL,NULL,NULL,&IoStatusBlock, \
		IOCTL_DISK_GET_PARTITION_INFO,NULL,0, \
		&part_info, sizeof(PARTITION_INFORMATION));
	if(Status == STATUS_PENDING)
	{
		NtWaitForSingleObject(dx->hVol,FALSE,NULL);
		Status = IoStatusBlock.Status;
	}
	if(Status == STATUS_SUCCESS)
		dx->partition_type = part_info.PartitionType;
	if(Status == STATUS_SUCCESS && part_info.PartitionType == NTFS_PARTITION)
	{
		Status = ZwFsControlFile(dx->hVol,NULL,NULL,NULL,&IoStatusBlock, \
				FSCTL_GET_NTFS_VOLUME_DATA,NULL,0, \
				&ntfs_data, sizeof(NTFS_DATA));
		if(Status == STATUS_PENDING)
		{
			NtWaitForSingleObject(dx->hVol,FALSE,NULL);
			Status = IoStatusBlock.Status;
		}
		if(Status == STATUS_SUCCESS)
		{
			/* not increment dx->processed_clusters here, because some part of MFT marked as free */
			DebugPrint("-Ultradfg- MFT_file   : start : length\n");
			/* $MFT */
			start = ntfs_data.MftStartLcn.QuadPart;
			len = ntfs_data.MftValidDataLength.QuadPart / ntfs_data.BytesPerCluster;
			DebugPrint("-Ultradfg- $MFT       :%I64u :%I64u\n",start,len);
			///dx->processed_clusters += len;
			_int64_memset(dx->cluster_map + start,MFT_SPACE,len);
			mft_len += len;
			/* $MFT2 */
			start = ntfs_data.MftZoneStart.QuadPart;
			len = ntfs_data.MftZoneEnd.QuadPart - ntfs_data.MftZoneStart.QuadPart;
			DebugPrint("-Ultradfg- $MFT2      :%I64u :%I64u\n",start,len);
			///dx->processed_clusters += len;
			_int64_memset(dx->cluster_map + start,MFT_SPACE,len);
			mft_len += len;
			/* $MFTMirror */
			start = ntfs_data.Mft2StartLcn.QuadPart;
			DebugPrint("-Ultradfg- $MFTMirror :%I64u :1\n",start);
			dx->cluster_map[start] = MFT_SPACE;
			///dx->processed_clusters ++;
			mft_len ++;
			dx->mft_size = (ULONG)(mft_len * dx->bytes_per_cluster);
		}
	}
}

FREEBLOCKMAP *InsertNewFreeBlock(PEXAMPLE_DEVICE_EXTENSION dx,
				 ULONGLONG start,ULONGLONG length)
{
	PFREEBLOCKMAP block;

	block = (PFREEBLOCKMAP)InsertLastItem((PLIST *)&dx->free_space_map,
		(PLIST *)&dx->lastfreeblock,sizeof(FREEBLOCKMAP));
	if(block)
	{
		/* set fields */
		block->lcn = start;
		block->length = length;
	}

	/* mark space */
	ProcessBlock(dx,start,length,FREE_SPACE);
	return block;
}

/* FillFreeClusterMap() */
/* Dumps all the free clusters on the volume */
BOOLEAN FillFreeClusterMap(PEXAMPLE_DEVICE_EXTENSION dx)
{
	ULONG status;
	PBITMAP_DESCRIPTOR bitMappings;
	ULONGLONG cluster,startLcn,nextLcn;
	IO_STATUS_BLOCK ioStatus;
	ULONGLONG i;
	ULONGLONG len;
	/* Bit shifting array for efficient processing of the bitmap */
	UCHAR BitShift[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
//ULONGLONG t;
	ULONGLONG *pnextLcn;
	/* Allocate memory */
	dx->cluster_map = (UCHAR *)_int64_malloc(dx->clusters_total);
	if(!dx->cluster_map)
	{
		DbgPrintNoMem();
		goto fail;
	}
	_int64_memset((void *)(dx->cluster_map),SYSTEM_SPACE,dx->clusters_total);
//	t = _rdtsc();
	/* Start scanning */
	bitMappings = (PBITMAP_DESCRIPTOR)(dx->BitMap);
#ifndef NT4_TARGET
	pnextLcn = &nextLcn;
#else
	if(dx->FileMap)
		pnextLcn = (ULONGLONG *)(dx->FileMap + FILEMAPSIZE * sizeof(ULONGLONG));
	else
	{
		DebugPrint("-Ultradfg- user mode buffer inaccessible!\n");
		pnextLcn = &nextLcn;
	}
#endif
	*pnextLcn = 0; cluster = LLINVALID;
	do
	{
		status = ZwFsControlFile(dx->hVol,NULL,NULL,0,&ioStatus,
			FSCTL_GET_VOLUME_BITMAP,
			pnextLcn,sizeof(cluster),bitMappings,BITMAPSIZE);
		if(status == STATUS_PENDING)
		{
			NtWaitForSingleObject(dx->hVol,FALSE,NULL);
			status = ioStatus.Status;
		}
		if(status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW)
		{
			DebugPrint("-Ultradfg- Get Volume Bitmap Error: %x!\n",status);
			goto fail;
		}
		/* Scan through the returned bitmap info, 
		   looking for empty clusters */
		startLcn = bitMappings->StartLcn;
		for(i = 0; i < min(bitMappings->ClustersToEndOfVol,8*BITMAPBYTES); i++)
		{
			if(!(bitMappings->Map[ i/8 ] & BitShift[ i % 8 ]))
			{
				/* Cluster is free */
				if(cluster == LLINVALID)
					cluster = startLcn + i;
			}
			else
			{
				/* Cluster isn't free */
				if(cluster != LLINVALID)
				{
					len = startLcn + i - cluster;
					if(dbg_level > 1)
						DebugPrint("-Ultradfg- start: %I64u len: %I64u\n",cluster,len);
					if(!InsertNewFreeBlock(dx,cluster,len))
						goto fail;
					dx->processed_clusters += len;
					cluster = LLINVALID;
				} 
			}
		}
		/* Move to the next block */
		*pnextLcn = bitMappings->StartLcn + i;
	} while(status != STATUS_SUCCESS);

	if(cluster != LLINVALID)
	{
		len = startLcn + i - cluster;
		if(dbg_level > 1)
			DebugPrint("-Ultradfg- start: %I64u len: %I64u\n",cluster,len);
		if(!InsertNewFreeBlock(dx,cluster,len))
			goto fail;
		dx->processed_clusters += len;
	}
//	DebugPrint("time: %I64u\n",_rdtsc() - t);
	return TRUE;
fail:
	return FALSE;
}

BOOLEAN InsertFileName(PEXAMPLE_DEVICE_EXTENSION dx,short *path,
					PFILE_BOTH_DIR_INFORMATION pFileInfo,BOOLEAN is_root)
{
	PFILENAME pfn;

	/* Add file name with path to filelist */
	pfn = (PFILENAME)InsertFirstItem((PLIST *)&dx->filelist,sizeof(FILENAME));
	if(!pfn) return FALSE; /* No enough memory! */
	if(!RtlCreateUnicodeString(&(pfn->name),path))
	{
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
	if(!DumpFile(dx,pfn))
	{
no_mem:
		dx->filelist = pfn->next_ptr;
		RtlFreeUnicodeString(&(pfn->name));
		ExFreePool(pfn);
	}
	else
	{
		if(pfn->is_fragm)
		{
			if(!InsertFragmFileBlock(dx,pfn))
			{
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

/* FindFiles() - recursive search of all files on specified path
   and put their clusters to map */
BOOLEAN FindFiles(PEXAMPLE_DEVICE_EXTENSION dx,UNICODE_STRING *path,BOOLEAN is_root)
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
	if(!pFileInfoFirst)
	{
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
	if(Status != STATUS_SUCCESS)
	{
		if(dbg_level > 0)
			DebugPrint("-Ultradfg- Can't open %ws %x\n",path->Buffer,Status);
		goto fail;
	}

	/* Query information about files */
	pFileInfo = pFileInfoFirst;
	pFileInfo->FileIndex = 0;
	pFileInfo->NextEntryOffset = 0;
	while(1)
	{
		if(KeReadStateEvent(&(dx->stop_event)) == 0x1)
			goto no_more_items;
		if (pFileInfo->NextEntryOffset != 0)
		{
			pFileInfo = (PVOID)((ULONG_PTR)pFileInfo + pFileInfo->NextEntryOffset);
		}
		else
		{
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
		wcsncpy(dx->tmp_buf,path->Buffer,(path->Length) >> 1);
		dx->tmp_buf[(path->Length) >> 1] = 0;
		if(!is_root)
			wcscat(dx->tmp_buf,L"\\");
		wcsncat(dx->tmp_buf,pFileInfo->FileName,(pFileInfo->FileNameLength) >> 1);
		if(!RtlCreateUnicodeString(&new_path,dx->tmp_buf))
		{
			DbgPrintNoMem();
			ZwClose(DirectoryHandle);
			goto fail;
		}

		if(IS_DIR(pFileInfo))
		{
			/*if(!*/FindFiles(dx,&new_path,FALSE)/*) goto fail*/;
			if(KeReadStateEvent(&(dx->stop_event)) == 0x1)
				goto no_more_items;
		}
		else
		{
			if(!pFileInfo->EndOfFile.QuadPart) goto next; /* file is empty */
		}
		if(!InsertFileName(dx,new_path.Buffer,pFileInfo,is_root))
		{
			if(dbg_level > 0)
				DebugPrint("-Ultradfg- InsertFileName failed for %ws\n",new_path.Buffer);
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
	if(pFileInfoFirst) ExFreePool(pFileInfoFirst);
	return FALSE;
}

BOOLEAN IsStringInFilter(short *str,PFILTER pf)
{
	POFFSET po;

	po = pf->offsets;
	while(po)
	{
		if((pf->buffer + po->offset)[0])
		{
			if(wcsstr(str,pf->buffer + po->offset))
				return TRUE;
		}
		po = po->next_ptr;
	}
	return FALSE;
}

void ApplyFilter(PEXAMPLE_DEVICE_EXTENSION dx,PFILENAME pfn)
{
	UNICODE_STRING str;

	if(!RtlCreateUnicodeString(&str,pfn->name.Buffer))
	{
		DbgPrintNoMem();
		pfn->is_filtered = FALSE;
		return;
	}
	_wcslwr(str.Buffer);
	if(dx->in_filter.buffer)
		if(!IsStringInFilter(str.Buffer,&dx->in_filter))
			goto excl;
	if(dx->ex_filter.buffer)
		if(IsStringInFilter(str.Buffer,&dx->ex_filter))
			goto excl;
	RtlFreeUnicodeString(&str);
	pfn->is_filtered = FALSE;
	return;
excl:
	RtlFreeUnicodeString(&str);
	pfn->is_filtered = TRUE;
	return;
}

BOOLEAN InsertFragmFileBlock(PEXAMPLE_DEVICE_EXTENSION dx,PFILENAME pfn)
{
	PFRAGMENTED pf,plist;

	pf = (PFRAGMENTED)AllocatePool(NonPagedPool,sizeof(FRAGMENTED));
	if(!pf) return FALSE;
	pf->pfn = pfn;
	plist = dx->fragmfileslist;
	while(plist)
	{
		if(plist->pfn->n_fragments > pfn->n_fragments)
		{
			if(!plist->next_ptr)
			{
				plist->next_ptr = pf;
				pf->next_ptr = NULL;
				break;
			}
			if(plist->next_ptr->pfn->n_fragments <= pfn->n_fragments)
			{
				pf->next_ptr = plist->next_ptr;
				plist->next_ptr = pf;
				break;
			}
		}
		plist = plist->next_ptr;
	}
	if(!plist)
	{
		pf->next_ptr = dx->fragmfileslist;
		dx->fragmfileslist = pf;
	}
	ApplyFilter(dx,pfn);
	return TRUE;
}

void DeleteBlockmap(PFILENAME pfn)
{
	DestroyList((PLIST *)&pfn->blockmap);
	pfn->clusters_total = pfn->n_fragments = 0;
	pfn->is_fragm = FALSE;
}

BLOCKMAP *InsertNewBlock(PEXAMPLE_DEVICE_EXTENSION dx,PFILENAME pfn,
					ULONGLONG startVcn,ULONGLONG startLcn,ULONGLONG length)
{
	PBLOCKMAP block;

	/* for compressed files it's neccessary: */
	if(dx->lastblock)
	{
		if(startLcn != dx->lastblock->lcn + dx->lastblock->length)
			pfn->is_fragm = TRUE;
	}
	block = (PBLOCKMAP)InsertLastItem((PLIST *)&pfn->blockmap,
		(PLIST *)&dx->lastblock,sizeof(BLOCKMAP));
	if(block)
	{
		/* set fields */
		block->lcn = startLcn;
		block->length = length;
		block->vcn = startVcn;
		/* change file statistic */
		pfn->clusters_total += length;
	}
	return block;
}

void MarkSpace(PEXAMPLE_DEVICE_EXTENSION dx,PFILENAME pfn)
{
	PBLOCKMAP block;
	UCHAR space_state;

	if(!pfn->is_fragm)
		ProcessUnfragmentedBlock(dx,pfn,pfn->blockmap->lcn,pfn->clusters_total);
	else
	{
		space_state = pfn->is_overlimit ? FRAGM_OVERLIMIT_SPACE : FRAGM_SPACE;
		block = pfn->blockmap;
		while(block)
		{
			ProcessBlock(dx,block->lcn,block->length,space_state);
			///DbgPrint("lcn %I64u:len %I64u %ws",block->lcn,block->length,pfn->name.Buffer);
			block = block->next_ptr;
		}
	}
}

/* Dump File()
 * Dumps the clusters belonging to the specified file until the
 * end of the file or the user stops the dump.
 *
 * NOTES: 
 * 1. On NTFS volumes files smaller then 1 kb are placed in MFT.
 *    And we exclude their from defragmenting process.
 * 2. On NTFS we skip 0-filled virtual clusters of compressed files.
 */

BOOLEAN DumpFile(PEXAMPLE_DEVICE_EXTENSION dx,PFILENAME pfn)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS Status;
	HANDLE hFile;
	IO_STATUS_BLOCK ioStatus;
	PGET_RETRIEVAL_DESCRIPTOR fileMappings;

	ULONGLONG startVcn,startLcn,length;
	ULONGLONG *pstartVcn;
	int i,cnt = 0;

	/* Data initialization */
	pfn->clusters_total = pfn->n_fragments = 0;
	pfn->is_fragm = FALSE;
	pfn->blockmap = NULL;
	dx->lastblock = NULL;
	/* Open the file */
	InitializeObjectAttributes(&ObjectAttributes,&(pfn->name),0,NULL,NULL);
	Status = ZwCreateFile(&hFile,FILE_GENERIC_READ,&ObjectAttributes,&ioStatus,
			  NULL,0,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,
			  pfn->is_dir ? FILE_OPEN_FOR_BACKUP_INTENT : FILE_NO_INTERMEDIATE_BUFFERING,
			  NULL,0);
	if(Status)
	{
		if(dbg_level > 0)
			DebugPrint("-Ultradfg- System file found: %ws %x\n",pfn->name.Buffer,Status);
		goto dump_success; /* System file! */
	}

	/* Start dumping the mapping information. Go until we hit the end of the file. */
#ifndef NT4_TARGET
	pstartVcn = &startVcn;
#else
	if(dx->FileMap)
		pstartVcn = (ULONGLONG *)(dx->FileMap + FILEMAPSIZE * sizeof(ULONGLONG) + sizeof(ULONGLONG));
	else
	{
		DebugPrint("-Ultradfg- user mode buffer inaccessible!\n");
		pstartVcn = &startVcn;
	}
#endif
	*pstartVcn = 0;
	fileMappings = (PGET_RETRIEVAL_DESCRIPTOR)(dx->FileMap);
	do
	{
		Status = ZwFsControlFile(hFile, NULL, NULL, 0, &ioStatus, \
						FSCTL_GET_RETRIEVAL_POINTERS, \
						pstartVcn, sizeof(ULONGLONG), \
						fileMappings, FILEMAPSIZE * sizeof(LARGE_INTEGER));
		if(Status == STATUS_PENDING)
		{
			NtWaitForSingleObject(hFile,FALSE,NULL);
			Status = ioStatus.Status;
		}
		if(Status != STATUS_SUCCESS && Status != STATUS_BUFFER_OVERFLOW)
		{
dump_fail:
			/*#if DBG
			if(dbg_level > 0)
				DbgPrint("-Ultradfg- Dump failed for %ws %x\n",pfn->name.Buffer,Status);
			#endif
			*/
			DeleteBlockmap(pfn);
			ZwClose(hFile);
			return FALSE;
		}
		/* Loop through the buffer of number/cluster pairs. */
		*pstartVcn = fileMappings->StartVcn;
		for(i = 0; i < (ULONGLONG) fileMappings->NumberOfPairs; i++)
		{
			/* On NT 4.0, a compressed virtual run (0-filled) is  */
			/* identified with a cluster offset of -1 */
			if(fileMappings->Pair[i].Lcn == LLINVALID)
				goto next_run;
			if(fileMappings->Pair[i].Vcn == 0)
				goto next_run; /* only for some 3.99 Gb files on FAT32 */
			startLcn = fileMappings->Pair[i].Lcn;
			length = fileMappings->Pair[i].Vcn - *pstartVcn;
			dx->processed_clusters += length;
			if(!InsertNewBlock(dx,pfn,*pstartVcn,startLcn,length))
				goto dump_fail;
			cnt ++;	/* block counter */
next_run:
			*pstartVcn = fileMappings->Pair[i].Vcn;
		}
	} while(Status != STATUS_SUCCESS);

	if(!cnt) goto dump_fail; /* file is placed in MFT or some error */
	ZwClose(hFile);

	MarkSpace(dx,pfn);
	pfn->n_fragments = cnt;
	if(pfn->is_fragm)
	{
		dx->fragmfilecounter ++; /* file is true fragmented */
		if(!pfn->is_dir && !pfn->is_overlimit)
			dx->clusters_to_move_tmp += pfn->clusters_total;
	}
	dx->fragmcounter += cnt;
	dx->clusters_to_compact_tmp += pfn->clusters_total;
dump_success:
	return TRUE; /* success */
}

void RedumpFile(PEXAMPLE_DEVICE_EXTENSION dx,PFILENAME pfn)
{
	BOOLEAN is_fragm;

	is_fragm = pfn->is_fragm;
	/* change statistic */
	dx->fragmcounter -= pfn->n_fragments;
	/* clean data */
	DeleteBlockmap(pfn);
	/* dump the file */
	if(is_fragm)
	{
		dx->fragmfilecounter --;
		DumpFile(dx,pfn);
	}
	else
	{
		DumpFile(dx,pfn);
		if(pfn->is_fragm)
			InsertFragmFileBlock(dx,pfn);
	}
}
