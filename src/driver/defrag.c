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
 *  Defragmenter engine.
 */

#include "driver.h"

void CheckPendingBlocks(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PROCESS_BLOCK_STRUCT *ppbs,*new_ppbs;
	NTSTATUS status;
	IO_STATUS_BLOCK ioStatus;
	ULONGLONG startLcn,nextLcn,cluster,data_cluster;
	ULONGLONG *pnextLcn;
	PBITMAP_DESCRIPTOR bitMappings;
	ULONGLONG i;

	if(!dx->unprocessed_blocks)
		goto done;
	bitMappings = (PBITMAP_DESCRIPTOR)dx->BitMap;
#ifndef NT4_TARGET
	pnextLcn = &nextLcn;
#else
	if(dx->FileMap)
		pnextLcn = (ULONGLONG *)(dx->FileMap + FILEMAPSIZE * sizeof(ULONGLONG) + 2 * sizeof(ULONGLONG));
	else
	{
		DebugPrint("-Ultradfg- user mode buffer inaccessible!\n");
		pnextLcn = &nextLcn;
	}
#endif
	ppbs = dx->no_checked_blocks;
	while(ppbs)
	{
		if(ppbs->len)
		{
			*pnextLcn = ppbs->start; cluster = data_cluster = LLINVALID;
			startLcn = ppbs->start;
			i = 0;
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
					DebugPrint("-Ultradfg- Get Volume Bitmap Error: %x!\n",
						 (UINT)status);
					ppbs->len -= startLcn + i - ppbs->start;
					ppbs->start = startLcn + i;
					goto fail;
				}
				/* Scan through the returned bitmap info, 
				   looking for empty clusters */
				startLcn = bitMappings->StartLcn;
				for(i = 0; i < min(bitMappings->ClustersToEndOfVol,8*BITMAPBYTES); i++)
				{
					if(startLcn + i >= ppbs->start + ppbs->len)
						goto end_of_scan;
					if(!(bitMappings->Map[ i/8 ] & BitShift[ i % 8 ]))
					{
						/* Cluster is free */
						if(cluster == LLINVALID)
							cluster = startLcn + i;
						if(data_cluster != LLINVALID)
						{
							new_ppbs = (PROCESS_BLOCK_STRUCT *)InsertFirstItem((PLIST *) \
								&dx->no_checked_blocks,sizeof(PROCESS_BLOCK_STRUCT));
							if(new_ppbs)
							{
								new_ppbs->start = data_cluster;
								new_ppbs->len = startLcn + i - data_cluster;
								dx->unprocessed_blocks ++;
							}
							data_cluster = LLINVALID;
						}
					}
					else
					{
						/* Cluster isn't free */
						if(cluster != LLINVALID)
						{
							if(dbg_level > 1)
								DebugPrint("-Ultradfg- X start: %I64u len: %I64u\n",cluster,
									startLcn + i - cluster);
							InsertNewFreeBlock(dx,cluster,startLcn + i - cluster);
							cluster = LLINVALID;
						}
						if(data_cluster == LLINVALID)
							data_cluster = startLcn + i;
					}
				}
				/* Move to the next block */
				*pnextLcn = bitMappings->StartLcn + i;
			} while(status != STATUS_SUCCESS);
end_of_scan:
			/* mark current as free */
			ppbs->len = 0;
			dx->unprocessed_blocks --;
			/* if block is full */
			if(data_cluster == ppbs->start)
				goto next_item;
fail:
			if(cluster != LLINVALID)
			{
				if(dbg_level > 1)
					DebugPrint("-Ultradfg- X start: %I64u len: %I64u\n",cluster,
							startLcn + i - cluster);
				InsertNewFreeBlock(dx,cluster,startLcn + i - cluster);
			}
			if(data_cluster != LLINVALID)
			{
				new_ppbs = (PROCESS_BLOCK_STRUCT *)InsertFirstItem((PLIST *) \
					&dx->no_checked_blocks,sizeof(PROCESS_BLOCK_STRUCT));
				if(new_ppbs)
				{
					new_ppbs->start = data_cluster;
					new_ppbs->len = startLcn + i - data_cluster;
					dx->unprocessed_blocks ++;
				}
			}
		}
next_item:
		ppbs = ppbs->next_ptr;
	}
done:
	return;
}

void UpdateFragmentedFilesList(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFRAGMENTED pf,prev_pf;

	pf = dx->fragmfileslist;
	prev_pf = NULL;
	//DbgPrint("1: %x;%x\n",pf,prev_pf);
	while(pf)
	{
		if(!pf->pfn->is_fragm)
		{
			//DbgPrint("2: %x;%x\n",pf,prev_pf);
			pf = (PFRAGMENTED)RemoveItem((PLIST *)&dx->fragmfileslist,
				(PLIST *)(void *)&prev_pf,(PLIST *)(void *)&pf);
			//DbgPrint("3: %x;%x\n",pf,prev_pf);
		}
		else
		{
			//DbgPrint("4: %x;%x\n",pf,prev_pf);
			prev_pf = pf;
			pf = pf->next_ptr;
			//DbgPrint("5: %x;%x\n",pf,prev_pf);
		}
	}
}

/*
 * On FAT partitions after file moving filesystem driver marks
 * previously allocated clusters as free immediately.
 * But on NTFS we must wait until the volume has been checkpointed. 
 */
void ProcessFreeBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start, ULONGLONG len)
{
	PROCESS_BLOCK_STRUCT *ppbs;

	if(dx->partition_type == NTFS_PARTITION)
	{
		/* Mark clusters as no-checked */
		ProcessBlock(dx,start,len,NO_CHECKED_SPACE);

		ppbs = dx->no_checked_blocks;
		while(ppbs)
		{
			if(!ppbs->len)
			{
				ppbs->start = start;
				ppbs->len = len;
				break;
			}
			ppbs = ppbs->next_ptr;
		}
		if(!ppbs)
		{
			/* insert new item */
			ppbs = (PROCESS_BLOCK_STRUCT *)InsertFirstItem((PLIST *)&dx->no_checked_blocks,
				sizeof(PROCESS_BLOCK_STRUCT));
			if(!ppbs) goto direct_call;
			ppbs->len = len; ppbs->start = start;
		}
		dx->unprocessed_blocks ++;
	}
	else
	{
direct_call:
		InsertFreeSpaceBlock(dx,start,len);
	}
}

/* Kernel of defragmenter */
void Defragment(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFRAGMENTED pflist;
	PFILENAME curr_file;
	ULONG prev_fragmfilecounter;
	BOOLEAN result;
///PFREEBLOCKMAP block = dx->free_space_map;

	/* Clear the 'Stop' event. */
	KeClearEvent(&dx->stop_event);
	/* Delete temporary file. */
	DeleteLogFile(dx);

	/*while(block)
	{
		DebugPrint("%I64u - %I64u\n",block->lcn,block->length);
		block = block->next_ptr;
	}*/
	if(dx->compact_flag)
	{
		DefragmentFreeSpace(dx);
		goto exit_defrag;
	}

	dx->current_operation = 'D';
	dx->clusters_to_move_initial = dx->clusters_to_move;
	// signed !!!
	while(dx->fragmfilecounter > 0)
	{
		CheckPendingBlocks(dx);
		prev_fragmfilecounter = dx->fragmfilecounter;
		/* process fragmented files */
		pflist = dx->fragmfileslist;
		while(pflist)
		{
			curr_file = pflist->pfn;
			/* skip system and unfragmented files */
			if(/*(*/!curr_file->is_fragm/*) || \
				(curr_file->clusters_total > (ULONGLONG)MAXULONG)*/ /* file is too big */
				)
				goto next;
			/* skip directories on FAT part. */
			if(curr_file->is_dir && dx->partition_type != NTFS_PARTITION)
				goto next;
			/* skip files with size over limit */
			if(curr_file->is_overlimit)
				goto next;
			if(curr_file->is_filtered)
				goto next;
			if(KeReadStateEvent(&dx->stop_event) == 0x1)
				goto exit_defrag;
			result = DefragmentFile(dx,curr_file);
			if(result)
				DebugPrint("-Ultradfg- Defrag success.\n");
			else
				DebugPrint("-Ultradfg- Defrag error for %ws\n",curr_file->name.Buffer);
next:
			pflist = pflist->next_ptr;
		}
		if(dx->fragmfilecounter == prev_fragmfilecounter)
			break; /* can't defragment more [big] files */
		UpdateFragmentedFilesList(dx);
	}
exit_defrag:
	CheckPendingBlocks(dx);
	UpdateFragmentedFilesList(dx);
	SaveFragmFilesListToDisk(dx);
	if(KeReadStateEvent(&dx->stop_event) == 0x0)
		dx->status = STATUS_DEFRAGMENTED;
	else
		dx->status = STATUS_BEFORE_PROCESSING;
}

NTSTATUS __MoveFile(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile, 
		    ULONGLONG startVcn, ULONGLONG targetLcn, ULONGLONG n_clusters)
{
	ULONG status;
	IO_STATUS_BLOCK ioStatus;
	MOVEFILE_DESCRIPTOR moveFile;
	MOVEFILE_DESCRIPTOR *pmoveFile;

	DebugPrint("-Ultradfg- sVcn: %I64u,tLcn: %I64u,n: %u\n",
		 startVcn,targetLcn,n_clusters);

	if(KeReadStateEvent(&dx->stop_event) == 0x1)
		return STATUS_UNSUCCESSFUL;

#ifndef NT4_TARGET
	pmoveFile = &moveFile;
#else
	if(dx->FileMap)
		pmoveFile = \
		 (MOVEFILE_DESCRIPTOR *)(dx->FileMap + FILEMAPSIZE * sizeof(ULONGLONG) + 3 * sizeof(ULONGLONG));
	else
	{
		DebugPrint("-Ultradfg- user mode buffer inaccessible!\n");
		pmoveFile = &moveFile;
	}
#endif
	/* Setup movefile descriptor and make the call */
	pmoveFile->FileHandle = hFile;
	pmoveFile->StartVcn.QuadPart = startVcn;
	pmoveFile->TargetLcn.QuadPart = targetLcn;
#ifdef _WIN64
	pmoveFile->NumVcns = n_clusters;
#else
	pmoveFile->NumVcns = (ULONG)n_clusters;
#endif
	status = ZwFsControlFile(dx->hVol,NULL,NULL,0,&ioStatus,
						FSCTL_MOVE_FILE,
						pmoveFile,sizeof(MOVEFILE_DESCRIPTOR),
						NULL,0);

	/* If the operation is pending, wait for it to finish */
	if(status == STATUS_PENDING)
	{
		NtWaitForSingleObject(hFile,FALSE,NULL);
		status = ioStatus.Status;
	}
	return status;
}

void InsertFreeSpaceBlock(UDEFRAG_DEVICE_EXTENSION *dx,
			  ULONGLONG start,ULONGLONG length)
{
	PFREEBLOCKMAP block, prev_block, new_block;

	ProcessBlock(dx,start,length,FREE_SPACE);

	block = dx->free_space_map;
	prev_block = NULL;
	while(block)
	{
		if(block->lcn > start)
		{
			if(!prev_block)
			{
				if(block->lcn == start + length)
				{
					block->lcn = start;
					block->length += length;
				}
				else
				{
					//insert new head
					//InsertBlock(dx,NULL,block->next_ptr,start,length);
					new_block = (PFREEBLOCKMAP)InsertFirstItem((PLIST *)&dx->free_space_map,
						sizeof(FREEBLOCKMAP));
					if(new_block)
					{
						new_block->lcn = start;
						new_block->length = length;
					}
				}
			}
			else
			{
				// insert block between prev and current
				if(block->lcn == start + length)
				{
					if(block->lcn == prev_block->lcn + prev_block->length)
					{
						prev_block->length += (length + block->length);
						prev_block->next_ptr = block->next_ptr;
						ExFreePool((void *)block);
					}
					else
					{
						block->lcn = start;
						block->length += length;
					}
					break;
				}
				if(block->lcn == prev_block->lcn + prev_block->length)
				{
					prev_block->length += length;
					break;
				}
				//InsertBlock(dx,prev_block,block,start,length);
				new_block = (PFREEBLOCKMAP)InsertMiddleItem((PLIST *)(void *)&prev_block,
					(PLIST *)(void *)&block,sizeof(FREEBLOCKMAP));
				if(new_block)
				{
					new_block->lcn = start;
					new_block->length = length;
				}
			}
			break;
		}
		prev_block = block;
		block = block->next_ptr;
	}
	if(!block)
	{
		// check if new block hit previous
		if(prev_block)
		{
			if(start == prev_block->lcn + prev_block->length)
			{
				prev_block->length += length;
				return;
			}
		}
		//dx->lastfreeblock = prev_block;
		//InsertNewFreeBlock(dx,start,length);
		new_block = (PFREEBLOCKMAP)InsertLastItem((PLIST *)&dx->free_space_map,
			(PLIST *)(void *)&prev_block,sizeof(FREEBLOCKMAP));
		if(new_block)
		{
			new_block->lcn = start;
			new_block->length = length;
		}
	}
}

void RemoveFreeSpaceBlocks(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	PFREEBLOCKMAP block, prev_block, new_block;
	PBLOCKMAP file_block;
	ULONG cnt;

	/* find block and truncate it */
	block = dx->free_space_map;
	prev_block = NULL;
	cnt = pfn->n_fragments;
	while(block && cnt)
	{
		file_block = pfn->blockmap;
		while(file_block)
		{
			if(file_block->lcn >= block->lcn && \
				(file_block->lcn + file_block->length) <= (block->lcn + block->length))
			{
				cnt --;
				if(file_block->lcn == block->lcn)
				{
					block->lcn += file_block->length;
					block->length -= file_block->length;
					goto next;
				}
				if((file_block->lcn + file_block->length) == (block->lcn + block->length))
				{
					block->length -= file_block->length;
					goto next;
				}
				/* If we are in the middle of the free block: */
				//InsertBlock(dx,block,block->next_ptr,
				//	file_block->lcn + file_block->length,
				//	block->lcn + block->length - file_block->lcn - file_block->length);
				new_block = (PFREEBLOCKMAP)InsertMiddleItem((PLIST *)(void *)&block,
					(PLIST *)&block->next_ptr,sizeof(FREEBLOCKMAP));
				if(new_block)
				{
					new_block->lcn = file_block->lcn + file_block->length;
					new_block->length = block->lcn + block->length - \
						file_block->lcn - file_block->length;
				}
				block->length = file_block->lcn - block->lcn;
			}
next:
			file_block = file_block->next_ptr;
		}
		if(!block->length)
		{
			block = (PFREEBLOCKMAP)RemoveItem((PLIST *)&dx->free_space_map,
				(PLIST *)(void *)&prev_block,(PLIST *)(void *)&block);
		}
		else
		{
			prev_block = block;
			block = block->next_ptr;
		}
	}
}

NTSTATUS MoveBlocksOfFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,
			  HANDLE hFile,ULONGLONG targetLcn,
			  ULONGLONG clusters_per_block)
{
	PBLOCKMAP curr_block;
	ULONGLONG j,n,r;
	ULONGLONG curr_target;
	NTSTATUS Status = STATUS_SUCCESS;

	curr_target = targetLcn;
	curr_block = pfn->blockmap;
	while(curr_block)
	{
		/* try to move current block */
		n = curr_block->length / clusters_per_block;
		for(j = 0; j < n; j++)
		{
			Status = __MoveFile(dx,hFile,curr_block->vcn + j * clusters_per_block, \
				curr_target,clusters_per_block);
			if(Status) goto exit;
			curr_target += clusters_per_block;
		}
		/* try to move rest of block */
		r = curr_block->length % clusters_per_block;
		if(r)
		{
			Status = __MoveFile(dx,hFile,curr_block->vcn + j * clusters_per_block, \
				curr_target,r);
			if(Status) goto exit;
			curr_target += r;
		}
		curr_block = curr_block->next_ptr;
	}
exit:
	return Status;
}

NTSTATUS MoveCompressedFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,
			    HANDLE hFile,ULONGLONG targetLcn)
{
	PBLOCKMAP curr_block;
	ULONGLONG curr_target;
	NTSTATUS Status = STATUS_SUCCESS;

	curr_target = targetLcn;
	curr_block = pfn->blockmap;
	while(curr_block)
	{
		Status = __MoveFile(dx,hFile,curr_block->vcn, \
			curr_target,curr_block->length);
		if(Status) break;
		curr_target += curr_block->length;
		curr_block = curr_block->next_ptr;
	}
	return Status;
}

ULONGLONG FindTarget(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	ULONGLONG t_before = LLINVALID, t_after = LLINVALID;
	ULONGLONG target;
	PFREEBLOCKMAP block = dx->free_space_map;

	while(block)
	{
		if(block->length >= pfn->clusters_total)
		{
			if(block->lcn < pfn->blockmap->lcn)
			{
				if(dx->compact_flag && t_before != LLINVALID)
					goto next;
				t_before = dx->compact_flag ? block->lcn : \
					(block->lcn + block->length - pfn->clusters_total);
			}
			else
			{
				t_after = block->lcn;
				break;
			}
		}
next:
		block = block->next_ptr;
	}
	if(dx->compact_flag)
	{
		target = t_before;
		if(pfn->is_fragm && t_before == LLINVALID)
			target = t_after;
	}
	else
	{
		if(t_after == LLINVALID) { target = t_before; goto exit; }
		if(t_before == LLINVALID) { target = t_after;  goto exit; }
		target = \
			((pfn->blockmap->lcn - t_before) < (t_after - pfn->blockmap->lcn)) ? \
			t_before : t_after;

	}
exit:
	return target;
}

BOOLEAN DefragmentFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	HANDLE hFile;
	ULONGLONG target,curr_target;

	ULONGLONG clusters_per_block;
	PBLOCKMAP curr_block;

	BOOLEAN was_fragmented;

	/* Open the file */
	InitializeObjectAttributes(&ObjectAttributes,&pfn->name,0,NULL,NULL);
	Status = ZwCreateFile(&hFile,FILE_GENERIC_READ,&ObjectAttributes,&IoStatusBlock,
			  NULL,0,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,
			  pfn->is_dir ? FILE_OPEN_FOR_BACKUP_INTENT : FILE_NO_INTERMEDIATE_BUFFERING,
			  NULL,0);
	if(Status)
	{
		if(dbg_level > 0)
			DebugPrint("-Ultradfg- %ws open: %x\n", pfn->name.Buffer,(UINT)Status);
		return FALSE;
	}
	/* Find free space */
	clusters_per_block = _256K / dx->bytes_per_cluster;
	target = FindTarget(dx,pfn);
	if(target == LLINVALID)
	{
		if(dbg_level > 1)
			DebugPrint("-Ultradfg- no enough continuous free space on volume\n");
		ZwClose(hFile);
		return FALSE;
	}

	DebugPrint("-Ultradfg- %ws\n",pfn->name.Buffer);
	DebugPrint("-Ultradfg- t: %I64u n: %I64u\n",target,pfn->clusters_total);

	was_fragmented = pfn->is_fragm;
	/* If file is compressed then we must move only non-virtual clusters. */
	/* If OS version is 4.x and file is greater then 256k ... */
	if(/*!dx->xp_compatible && */pfn->clusters_total > clusters_per_block)
		Status = MoveBlocksOfFile(dx,pfn,hFile,target,clusters_per_block);
	else if(pfn->is_compressed)
		Status = MoveCompressedFile(dx,pfn,hFile,target);
	else
		Status = __MoveFile(dx,hFile,0,target,pfn->clusters_total);
	ZwClose(hFile);
	if(Status != STATUS_SUCCESS)
		DebugPrint("MoveFile error: %x\n",(UINT)Status);
	/* mark space as free */
	curr_block = pfn->blockmap;
	while(curr_block)
	{
		ProcessFreeBlock(dx,curr_block->lcn,curr_block->length);
		curr_block = curr_block->next_ptr;
	}
	if(dx->partition_type == NTFS_PARTITION && Status == STATUS_SUCCESS)
	{
		if(pfn->is_compressed)
		{
			curr_block = pfn->blockmap;
			curr_target = target;
			while(curr_block)
			{
				curr_block->lcn = curr_target;
				curr_target += curr_block->length;
				curr_block = curr_block->next_ptr;
			}
		}
		else
		{
			pfn->blockmap->lcn = target;
			pfn->blockmap->length = pfn->clusters_total;
			DestroyList((PLIST *)&pfn->blockmap->next_ptr);
			dx->fragmcounter -= (pfn->n_fragments - 1);
			pfn->n_fragments = 1;
		}
		if(pfn->is_fragm)
		{
			dx->fragmfilecounter --;
			pfn->is_fragm = FALSE;
		}
		MarkSpace(dx,pfn);
	}
	else
	{
		/* Because on FAT part. we always have STATUS_SUCCESS. */
		RedumpFile(dx,pfn);
	}
	RemoveFreeSpaceBlocks(dx,pfn);
	if(was_fragmented && !pfn->is_fragm)
		dx->clusters_to_move -= pfn->clusters_total;
	if(!was_fragmented && pfn->is_fragm)
		dx->clusters_to_move += pfn->clusters_total;
	return (!pfn->is_fragm);
}

/*
 * NOTES:
 * 1. On FAT it's bad idea, because dirs isn't moveable.
 * 2. On NTFS cycle of attempts is bad solution,
 *    because it increase processing time and can be as while(1) {}.
 */
void DefragmentFreeSpace(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFILENAME curr_file;

	dx->current_operation = 'C';
	dx->clusters_to_compact = dx->clusters_to_compact_initial;
	/* On FAT volumes it increase distance between dir & files inside it. */
	if(dx->partition_type != NTFS_PARTITION)
		return;
	CheckPendingBlocks(dx);
	/* process all files */
	curr_file = dx->filelist;
	while(curr_file)
	{
		/* skip system files */
		if(/*(*/!curr_file->blockmap/*) || \
			(curr_file->clusters_total > (ULONGLONG)MAXULONG)*/ /* file is too big */
			)
		{
			curr_file = curr_file->next_ptr;
			continue;
		}
		/* skip directories on FAT part. */
		/* skip files with size over limit */
/*			if(curr_file->is_overlimit)
		{
			curr_file = curr_file->next_ptr;
			continue;
		}
*/
		if(KeReadStateEvent(&dx->stop_event) == 0x1)
			return;
		DefragmentFile(dx,curr_file);
		dx->clusters_to_compact -= curr_file->clusters_total;
		curr_file = curr_file->next_ptr;
	}
}

void FreeAllBuffers(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFILENAME ptr,next_ptr;

	/* Set 'Stop' event. */
	KeSetEvent(&dx->stop_event,IO_NO_INCREMENT,FALSE);
	/* Now we can free buffers */
	DestroyList((PLIST *)&dx->no_checked_blocks);
	dx->unprocessed_blocks = 0;
	ptr = dx->filelist;
	while(ptr)
	{
		next_ptr = ptr->next_ptr;
		DestroyList((PLIST *)&ptr->blockmap);
		RtlFreeUnicodeString(&ptr->name);
		ExFreePool((void *)ptr);
		ptr = next_ptr;
	}
	DestroyList((PLIST *)&dx->free_space_map);
	DestroyList((PLIST *)&dx->fragmfileslist);
	dx->filelist = NULL;
	CloseVolume(dx);
}
