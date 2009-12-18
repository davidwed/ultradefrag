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
* Functions for free space manipulations.
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

	/* Start scanning */
	bitMappings = (PBITMAP_DESCRIPTOR)(dx->BitMap);
	*dx->pnextLcn = 0; cluster = LLINVALID;
	do {
		RtlZeroMemory(bitMappings,BITMAPSIZE);
		status = ZwFsControlFile(dx->hVol,NULL,NULL,0,&ioStatus,
			FSCTL_GET_VOLUME_BITMAP,
			dx->pnextLcn,sizeof(cluster),bitMappings,BITMAPSIZE);
		if(status == STATUS_PENDING){
			if(nt4_system)
				NtWaitForSingleObject(dx->hVol,FALSE,NULL);
			else
				ZwWaitForSingleObject(dx->hVol,FALSE,NULL);
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
	return STATUS_SUCCESS;
}

/*
* On FAT partitions after file moving filesystem driver marks
* previously allocated clusters as free immediately.
* But on NTFS they are always allocated by system, not free, never free.
* Only the next analyse frees them.
*/
void ProcessFreeBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,
					  ULONGLONG len,UCHAR old_space_state)
{
	if(dx->partition_type == NTFS_PARTITION) /* Mark clusters as no-checked */
		ProcessBlock(dx,start,len,NO_CHECKED_SPACE,old_space_state);
	else InsertFreeSpaceBlock(dx,start,len,old_space_state);
}

void InsertFreeSpaceBlockInternal(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG length);

void InsertFreeSpaceBlock(UDEFRAG_DEVICE_EXTENSION *dx,
			  ULONGLONG start,ULONGLONG length,UCHAR old_space_state)
{
	ProcessBlock(dx,start,length,FREE_SPACE,old_space_state);
	InsertFreeSpaceBlockInternal(dx,start,length);
}
			  
/* inserts any block in free space map */
void InsertFreeSpaceBlockInternal(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG length)
{
	PFREEBLOCKMAP block, prev_block = NULL, next_block;
	
	for(block = dx->free_space_map; block != NULL; block = block->next_ptr){
		if(block->lcn > start){
			if(block != dx->free_space_map) prev_block = block->prev_ptr;
			break;
		}
		if(block->next_ptr == dx->free_space_map){
			prev_block = block;
			break;
		}
	}

	/* hits the new block previous? */
	if(prev_block){
		if(prev_block->lcn + prev_block->length == start){
			prev_block->length += length;
			if(prev_block->lcn + prev_block->length == prev_block->next_ptr->lcn){
				prev_block->length += prev_block->next_ptr->length;
				prev_block->next_ptr->length = 0;
			}
			return;
		}
	}
	
	/* hits the new block the next one? */
	if(dx->free_space_map){
		if(prev_block == NULL) next_block = dx->free_space_map;
		else next_block = prev_block->next_ptr;
		if(start + length == next_block->lcn){
			next_block->lcn = start;
			next_block->length += length;
			return;
		}
	}
	
	block = (PFREEBLOCKMAP)InsertItem((PLIST *)&dx->free_space_map,
		(PLIST)prev_block,sizeof(FREEBLOCKMAP),PagedPool);
	if(!block) return;
	
	block->lcn = start;
	block->length = length;
}

/* inserts last block in free space map: used in analysis process only */
FREEBLOCKMAP *InsertLastFreeBlock(UDEFRAG_DEVICE_EXTENSION *dx,
				 ULONGLONG start,ULONGLONG length)
{
	PFREEBLOCKMAP block, lastblock = NULL;
	
	if(dx->free_space_map) lastblock = dx->free_space_map->prev_ptr;
	block = (PFREEBLOCKMAP)InsertItem((PLIST *)&dx->free_space_map,
		(PLIST)lastblock,sizeof(FREEBLOCKMAP),PagedPool);
	if(block){
		block->lcn = start;
		block->length = length;
	}

	/* mark space */
	ProcessBlock(dx,start,length,FREE_SPACE,SYSTEM_SPACE);
	return block;
}

/*
* Cuts the left side of the free block. If length is equal 
* to free block length it marks them as zero length block.
*/
void TruncateFreeSpaceBlock(UDEFRAG_DEVICE_EXTENSION *dx,
							ULONGLONG start,ULONGLONG length)
{
	PFREEBLOCKMAP block;
	ULONGLONG n;

	for(block = dx->free_space_map; block != NULL; block = block->next_ptr){
		if(block->lcn == start){
			n = min(block->length,length);
			block->lcn += n;
			block->length -= n;
			return;
		}
		if(block->next_ptr == dx->free_space_map) break;
	}
	DebugPrint("-Ultradfg- TruncateFreeSpaceBlock() failed: Lcn=%I64u!\n",start);
}

/* Removes specified range of clusters from the free space list. */
void CleanupFreeSpaceList(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG len)
{
	PFREEBLOCKMAP block;
	ULONGLONG new_lcn, new_length;
	
	for(block = dx->free_space_map; block != NULL; block = block->next_ptr){
		if(block->lcn >= start && (block->lcn + block->length) <= (start + len)){
			/*
			* block is inside a specified range
			* |--------------------|
			*        |block|
			*/
			block->length = 0;
			goto next_block;
		}
		if(block->lcn < start && (block->lcn + block->length) > start && \
		  (block->lcn + block->length) <= (start + len)){
			/*
			* cut the right side of block
			*     |--------------------|
			* |block|
			*/
			block->length = start - block->lcn;
			goto next_block;
		}
		if(block->lcn >= start && block->lcn < (start + len)){
			/*
			* cut the left side of block
			* |--------------------|
			*                    |block|
			*/
			block->length = block->lcn + block->length - (start + len);
			block->lcn = start + len;
			goto next_block;
		}
		if(block->lcn < start && (block->lcn + block->length) > (start + len)){
			/*
			* specified range is inside a block
			*   |----|
			* |-------block--------|
			*/
			new_lcn = start + len;
			new_length = block->lcn + block->length - (start + len);
			block->length = start - block->lcn;
			InsertFreeSpaceBlockInternal(dx,new_lcn,new_length);
			goto next_block;
		}

next_block:
		if(block->next_ptr == dx->free_space_map) break;
	}
}

void DbgPrintFreeSpaceList(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFREEBLOCKMAP block;
	
	for(block = dx->free_space_map; block != NULL; block = block->next_ptr){
		DebugPrint2("-Ultradfg- Free Block start: %I64u len: %I64u\n",block->lcn,block->length);
		if(block->next_ptr == dx->free_space_map) break;
	}
}
