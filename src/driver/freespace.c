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
 *  Functions for free space manipulations.
 */

#include "driver.h"

/* Dumps all the free clusters on the volume */
NTSTATUS FillFreeSpaceMap(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ULONG status;
	PBITMAP_DESCRIPTOR bitMappings;
	ULONGLONG cluster,startLcn;
	IO_STATUS_BLOCK ioStatus;
	ULONGLONG i;
	ULONGLONG len;
	/* Bit shifting array for efficient processing of the bitmap */
	UCHAR BitShift[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
//ULONGLONG t;
	//	t = _rdtsc();
	/* Start scanning */
	bitMappings = (PBITMAP_DESCRIPTOR)(dx->BitMap);
	*dx->pnextLcn = 0; cluster = LLINVALID;
	do {
		status = ZwFsControlFile(dx->hVol,NULL,NULL,0,&ioStatus,
			FSCTL_GET_VOLUME_BITMAP,
			dx->pnextLcn,sizeof(cluster),bitMappings,BITMAPSIZE);
		if(status == STATUS_PENDING){
			NtWaitForSingleObject(dx->hVol,FALSE,NULL);
			status = ioStatus.Status;
		}
		if(status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW){
			DebugPrint("-Ultradfg- Get Volume Bitmap Error: %x!\n",(UINT)status);
			return status;
		}
		/* Scan through the returned bitmap info. */
		startLcn = bitMappings->StartLcn;
		for(i = 0; i < min(bitMappings->ClustersToEndOfVol,8*BITMAPBYTES); i++){
			if(!(bitMappings->Map[ i/8 ] & BitShift[ i % 8 ])){
				/* Cluster is free */
				if(cluster == LLINVALID)
					cluster = startLcn + i;
			} else {
				/* Cluster isn't free */
				if(cluster != LLINVALID){
					len = startLcn + i - cluster;
					DebugPrint2("-Ultradfg- start: %I64u len: %I64u\n",cluster,len);
					if(!InsertLastFreeBlock(dx,cluster,len))
						return STATUS_NO_MEMORY;
					dx->processed_clusters += len;
					cluster = LLINVALID;
				} 
			}
		}
		/* Move to the next block */
		*dx->pnextLcn = bitMappings->StartLcn + i;
	} while(status != STATUS_SUCCESS);

	if(cluster != LLINVALID){
		len = startLcn + i - cluster;
		DebugPrint2("-Ultradfg- start: %I64u len: %I64u\n",cluster,len);
		if(!InsertLastFreeBlock(dx,cluster,len))
			return STATUS_NO_MEMORY;
		dx->processed_clusters += len;
	}
//	DebugPrint("time: %I64u\n",_rdtsc() - t);
	return STATUS_SUCCESS;
}

/* returns TRUE if all requested space is really free, FALSE otherwise */
BOOLEAN CheckFreeSpace(UDEFRAG_DEVICE_EXTENSION *dx,
					   ULONGLONG start, ULONGLONG len)
{
	NTSTATUS status;
	IO_STATUS_BLOCK ioStatus;
	ULONGLONG startLcn;
	PBITMAP_DESCRIPTOR bitMappings;
	ULONGLONG i;

	bitMappings = (PBITMAP_DESCRIPTOR)dx->BitMap;
	*dx->pnextLcn = start; startLcn = start; i = 0;
	do {
		status = ZwFsControlFile(dx->hVol,NULL,NULL,0,&ioStatus,
			FSCTL_GET_VOLUME_BITMAP,
			dx->pnextLcn,sizeof(ULONGLONG),bitMappings,BITMAPSIZE);
		if(status == STATUS_PENDING){
			NtWaitForSingleObject(dx->hVol,FALSE,NULL);
			status = ioStatus.Status;
		}
		if(status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW){
			DebugPrint("-Ultradfg- Get Volume Bitmap Error: %x!\n",
				 (UINT)status);
			return FALSE;
		}
		/* Scan, looking for non-empty clusters. */
		startLcn = bitMappings->StartLcn;
		for(i = 0; i < min(bitMappings->ClustersToEndOfVol,8*BITMAPBYTES); i++){
			if(startLcn + i >= start + len) return TRUE;
			if(bitMappings->Map[ i/8 ] & BitShift[ i % 8 ]){
				return FALSE; /* Cluster isn't free */
			}
		}
		/* Move to the next block */
		*dx->pnextLcn = bitMappings->StartLcn + i;
	} while(status != STATUS_SUCCESS);

	return TRUE;
}

/*
 * FIXME: what is about memory usage growing because
 * pending blocks may be never freed.
 */
/*
 * FIXME: this function is really not useful, at least on XP, 
 * because all blocks are still not checked by system and 
 * CheckFreeSpace() always returns FALSE here.
 */
void CheckPendingBlocks(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PROCESS_BLOCK_STRUCT *ppbs;

	if(!dx->unprocessed_blocks) return;
	for(ppbs = dx->no_checked_blocks; ppbs != NULL; ppbs = ppbs->next_ptr){
		if(ppbs->len){
			if(CheckFreeSpace(dx,ppbs->start,ppbs->len)){
				DebugPrint2("-Ultradfg- Pending block was freed start: %I64u len: %I64u\n",
						ppbs->start,ppbs->len);
				InsertFreeSpaceBlock(dx,ppbs->start,ppbs->len);
				ppbs->len = 0;
				dx->unprocessed_blocks --;
			}
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

	if(dx->partition_type == NTFS_PARTITION){
		/* Mark clusters as no-checked */
		ProcessBlock(dx,start,len,NO_CHECKED_SPACE,SYSTEM_SPACE); /* FIXME!!! */

		for(ppbs = dx->no_checked_blocks; ppbs != NULL; ppbs = ppbs->next_ptr){
			if(!ppbs->len){
				ppbs->start = start;
				ppbs->len = len;
				break;
			}
		}
		if(!ppbs){
			/* insert new item */
			ppbs = (PROCESS_BLOCK_STRUCT *)InsertFirstItem((PLIST *)&dx->no_checked_blocks,
				sizeof(PROCESS_BLOCK_STRUCT));
			if(!ppbs) goto direct_call; /* ??? */
			ppbs->len = len; ppbs->start = start;
		}
		dx->unprocessed_blocks ++;
	} else {
direct_call:
		InsertFreeSpaceBlock(dx,start,len);
	}
}

/* insert any block in free space map */
/* TODO: this function should be optimized */
void InsertFreeSpaceBlock(UDEFRAG_DEVICE_EXTENSION *dx,
			  ULONGLONG start,ULONGLONG length)
{
	PFREEBLOCKMAP block, prev_block = NULL, new_block;

	ProcessBlock(dx,start,length,FREE_SPACE,SYSTEM_SPACE); /* FIXME!!! */

	for(block = dx->free_space_map; block != NULL; prev_block = block, block = block->next_ptr){
		if(block->lcn > start){
			if(!prev_block){
				if(block->lcn == start + length){
					block->lcn = start;
					block->length += length;
				} else {
					//insert new head
					//InsertBlock(dx,NULL,block->next_ptr,start,length);
					new_block = (PFREEBLOCKMAP)InsertFirstItem((PLIST *)&dx->free_space_map,
						sizeof(FREEBLOCKMAP));
					if(new_block){
						new_block->lcn = start;
						new_block->length = length;
					}
				}
			} else {
				// insert block between prev and current
				if(block->lcn == start + length){
					if(block->lcn == prev_block->lcn + prev_block->length){
						prev_block->length += (length + block->length);
						prev_block->next_ptr = block->next_ptr;
						ExFreePool((void *)block);
					} else {
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
				if(new_block){
					new_block->lcn = start;
					new_block->length = length;
				}
			}
			break;
		}
	}
	if(!block){
		// check if new block hit previous
		if(prev_block){
			if(start == prev_block->lcn + prev_block->length){
				prev_block->length += length;
				return;
			}
		}
		//dx->lastfreeblock = prev_block;
		//InsertNewFreeBlock(dx,start,length);
		new_block = (PFREEBLOCKMAP)InsertLastItem((PLIST *)&dx->free_space_map,
			(PLIST *)(void *)&prev_block,sizeof(FREEBLOCKMAP));
		if(new_block){
			new_block->lcn = start;
			new_block->length = length;
		}
	}
}

/* insert last block in free space map: used in analysis process only */
FREEBLOCKMAP *InsertLastFreeBlock(UDEFRAG_DEVICE_EXTENSION *dx,
				 ULONGLONG start,ULONGLONG length)
{
	PFREEBLOCKMAP block;

	block = (PFREEBLOCKMAP)InsertLastItem((PLIST *)&dx->free_space_map,
		(PLIST *)&dx->lastfreeblock,sizeof(FREEBLOCKMAP));
	if(block){
		block->lcn = start;
		block->length = length;
	}

	/* mark space */
	ProcessBlock(dx,start,length,FREE_SPACE,SYSTEM_SPACE);
	return block;
}

/* TODO: remove for() cycle */
void TruncateFreeSpaceBlock(UDEFRAG_DEVICE_EXTENSION *dx,
							ULONGLONG start,ULONGLONG length)
{
	PFREEBLOCKMAP block, prev_block = NULL;
	ULONGLONG n;

	for(block = dx->free_space_map; block != NULL; prev_block = block, block = block->next_ptr){
		if(block->lcn == start){
			n = min(block->length,length);
			block->lcn += n;
			block->length -= n;
			goto found;
		}
		if(block->lcn + block->length == start + length){
			n = min(block->length,length);
			block->length -= n;
			goto found;
		}
	}
	return;
found:
	if(!block->length)
		RemoveItem((PLIST *)&dx->free_space_map,
			(PLIST *)(void *)&prev_block,(PLIST *)(void *)&block);
}

