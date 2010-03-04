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
 * @file freespace.c
 * @brief Volume free space dumping code.
 * @addtogroup FreeSpace
 * @{
 */

#include "globals.h"

static FREEBLOCKMAP *AddLastFreeBlock(ULONGLONG start,ULONGLONG length);

/**
 * @brief Dumps the free space on the volume.
 * @return An appropriate NTSTATUS code.
 * @todo This function should return common error
 * codes instead of the status codes.
 */
NTSTATUS FillFreeSpaceMap(void)
{
	char *BitMap;
	NTSTATUS status;
	PBITMAP_DESCRIPTOR bitMappings;
	ULONGLONG cluster,startLcn;
	IO_STATUS_BLOCK ioStatus;
	ULONGLONG i;
	ULONGLONG len;
	/* Bit shifting array for efficient processing of the bitmap */
	UCHAR BitShift[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
	ULONGLONG nextLcn;
	
	/* allocate memory */
	BitMap = winx_heap_alloc(BITMAPSIZE * sizeof(UCHAR));
	if(BitMap == NULL){
		DebugPrint("Cannot allocate memory for FillFreeSpaceMap()!\n");
		out_of_memory_condition_counter ++;
		return STATUS_NO_MEMORY;
	}

	/* start scanning */
	bitMappings = (PBITMAP_DESCRIPTOR)BitMap;
	nextLcn = 0; cluster = LLINVALID;
	do {
		RtlZeroMemory(bitMappings,BITMAPSIZE);
		status = NtFsControlFile(winx_fileno(fVolume),NULL,NULL,0,&ioStatus,
			FSCTL_GET_VOLUME_BITMAP,&nextLcn,sizeof(cluster),bitMappings,BITMAPSIZE);
		if(NT_SUCCESS(status)/* == STATUS_PENDING*/){
			NtWaitForSingleObject(winx_fileno(fVolume),FALSE,NULL);
			status = ioStatus.Status;
		}
		if(status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW){
			DebugPrint("Get Volume Bitmap Error: %x!\n",(UINT)status);
			winx_heap_free(BitMap);
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
					DebugPrint2("start: %I64u len: %I64u\n",cluster,len);
					if(!AddLastFreeBlock(cluster,len)){
						winx_heap_free(BitMap);
						return STATUS_NO_MEMORY;
					}
					Stat.processed_clusters += len;
					cluster = LLINVALID;
				}
			}
		}
		/* Move to the next block */
		nextLcn = bitMappings->StartLcn + i;
	} while(status != STATUS_SUCCESS);

	if(cluster != LLINVALID){
		len = startLcn + i - cluster;
		DebugPrint2("start: %I64u len: %I64u\n",cluster,len);
		if(!AddLastFreeBlock(cluster,len)){
			winx_heap_free(BitMap);
			return STATUS_NO_MEMORY;
		}
		Stat.processed_clusters += len;
	}
	winx_heap_free(BitMap);
	return STATUS_SUCCESS;
}

/**
 * @brief Processes a space block freed by file moving routines.
 * @details Remarks the cluster map and adds the block to the
 * free space map if it is really free.
 * @param[in] start the starting cluster of the block.
 * @param[in] len the length of the block, in clusters.
 * @param[in] old_space_state the state of the block which it has
 *                            before the freedom.
 * @note On NTFS volumes the freed block is always marked
 * as temporarily allocated by system because on NTFS the volume
 * checkpoints mechanism exists which really frees the space only during
 * the next volume analysis.
 */
void ProcessFreedBlock(ULONGLONG start,ULONGLONG len,UCHAR old_space_state)
{
	/*
	* On FAT partitions after file moving filesystem driver marks
	* previously allocated clusters as free immediately.
	* But on NTFS they are always allocated by system, not free, never free.
	* Only the next volume analysis frees them.
	*/
	if(partition_type == NTFS_PARTITION){
		/* mark clusters as temporarily allocated by system  */
		RemarkBlock(start,len,TEMPORARY_SYSTEM_SPACE,old_space_state);
	} else {
		RemarkBlock(start,len,FREE_SPACE,old_space_state);
		AddFreeSpaceBlock(start,len);
	}
}

/**
 * @brief Adds a free space block to the free space map.
 * @param[in] start the starting cluster of the block.
 * @param[in] length the length of the block, in clusters.
 */
void AddFreeSpaceBlock(ULONGLONG start,ULONGLONG length)
{
	PFREEBLOCKMAP block, prev_block = NULL, next_block;
	
	for(block = free_space_map; block != NULL; block = block->next_ptr){
		if(block->lcn > start){
			if(block != free_space_map) prev_block = block->prev_ptr;
			break;
		}
		if(block->next_ptr == free_space_map){
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
	if(free_space_map){
		if(prev_block == NULL) next_block = free_space_map;
		else next_block = prev_block->next_ptr;
		if(start + length == next_block->lcn){
			next_block->lcn = start;
			next_block->length += length;
			return;
		}
	}
	
	block = (PFREEBLOCKMAP)winx_list_insert_item((list_entry **)(void *)&free_space_map,
		(list_entry *)prev_block,sizeof(FREEBLOCKMAP));
	if(!block) return;
	
	block->lcn = start;
	block->length = length;
}

/**
 * @brief Inserts the last free space block to the free space map.
 * @param[in] start the starting cluster of the block.
 * @param[in] length the length of the block, in clusters.
 * @return Pointer to the inserted block. NULL indicates failure.
 * @note Internal use only.
 */
static FREEBLOCKMAP *AddLastFreeBlock(ULONGLONG start,ULONGLONG length)
{
	PFREEBLOCKMAP block, lastblock = NULL;
	
	if(free_space_map) lastblock = free_space_map->prev_ptr;
	block = (PFREEBLOCKMAP)winx_list_insert_item((list_entry **)(void *)&free_space_map,
		(list_entry *)lastblock,sizeof(FREEBLOCKMAP));
	if(block){
		block->lcn = start;
		block->length = length;
	}

	/* mark space */
	RemarkBlock(start,length,FREE_SPACE,SYSTEM_SPACE);
	return block;
}

/**
 * @brief Cuts the left side of the free space block.
 * @param[in] start the starting cluster of the block.
 * @param[in] length the length of the cluster chain
 *                   to be cut, in clusters.
 * @note If length parameter is equal to the free space block
 * length this function marks the block as zero length block.
 */
void TruncateFreeSpaceBlock(ULONGLONG start,ULONGLONG length)
{
	PFREEBLOCKMAP block;
	ULONGLONG n;

	for(block = free_space_map; block != NULL; block = block->next_ptr){
		if(block->lcn == start){
			n = min(block->length,length);
			block->lcn += n;
			block->length -= n;
			return;
		}
		if(block->next_ptr == free_space_map) break;
	}
	DebugPrint("TruncateFreeSpaceBlock() failed: Lcn=%I64u!\n",start);
}

/**
 * @brief Removes a range of clusters from the free space list.
 * @param[in] start the starting cluster of the block.
 * @param[in] len the length of the block, in clusters.
 */
void RemoveFreeSpaceBlock(ULONGLONG start,ULONGLONG len)
{
	PFREEBLOCKMAP block;
	ULONGLONG new_lcn, new_length;
	
	for(block = free_space_map; block != NULL; block = block->next_ptr){
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
			AddFreeSpaceBlock(new_lcn,new_length);
			goto next_block;
		}

next_block:
		if(block->next_ptr == free_space_map) break;
	}
}

/**
 * @brief Prints sequenly all the entries of the free space list.
 */
void DbgPrintFreeSpaceList(void)
{
	PFREEBLOCKMAP block;
	
	for(block = free_space_map; block != NULL; block = block->next_ptr){
		DebugPrint2("Free Block start: %I64u len: %I64u\n",block->lcn,block->length);
		if(block->next_ptr == free_space_map) break;
	}
}

/** @} */
