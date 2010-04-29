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
 * @file map.c
 * @brief Cluster map code.
 * @addtogroup ClusterMap
 * @{
 */

#include "globals.h"

/*
* Buffer to store the number of clusters of each kind.
* More details at http://www.thescripts.com/forum/thread617704.html
* ('Dynamically-allocated Multi-dimensional Arrays - C').
*/
ULONGLONG (*new_cluster_map)[NUM_OF_SPACE_STATES] = NULL;
ULONG map_size = 0;

ULONGLONG clusters_per_cell = 0;
ULONGLONG clusters_per_last_cell = 0; /* last block can represent more clusters */
BOOLEAN opposite_order = FALSE; /* if true then number of clusters is less than number of blocks */
ULONGLONG cells_per_cluster = 0;
ULONGLONG cells_per_last_cluster = 0;

/**
 * @brief Allocates the cluster map.
 * @param[in] size the number of map cells.
 * @return Zero for success, negative value otherwise.
 */
int AllocateMap(int size)
{
	LARGE_INTEGER interval;
	NTSTATUS Status;
	int buffer_size;
	
	/* synchronize with GetMap requests */
	interval.QuadPart = (-1 * 10000 * 1000); /* 1 sec */
	Status = NtWaitForSingleObject(hMapEvent,FALSE,&interval);
	if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status)){
		DebugPrint("NtWaitForSingleObject(hMapEvent,...) failed: %x!",
			(UINT)Status);
		return (-1);
	}

	DebugPrint("Map size = %u\n",size);
	if(new_cluster_map) FreeMap();
	if(!size){
		(void)NtSetEvent(hMapEvent,NULL);
		return 0; /* console/native apps may work without cluster map */
	}
	buffer_size = NUM_OF_SPACE_STATES * size * sizeof(ULONGLONG);
	new_cluster_map = winx_heap_alloc(buffer_size);
	if(!new_cluster_map){
		DebugPrint("Cannot allocate %u bytes of memory for cluster map!",
			buffer_size);
		out_of_memory_condition_counter ++;
		(void)NtSetEvent(hMapEvent,NULL);
		return (-1);
	}
	map_size = size;
	memset(new_cluster_map,0,buffer_size);
	/* reset specific variables */
	clusters_per_cell = 0;
	clusters_per_last_cell = 0;
	opposite_order = FALSE;
	cells_per_cluster = 0;
	cells_per_last_cluster = 0;
	(void)NtSetEvent(hMapEvent,NULL);
	return 0;
}

/**
 * @brief Retrieves the cluster map.
 * @param[out] dest pointer to buffer
 *                  receiving the map.
 * @param[in] cluster_map_size the 
 *                  number of map cells.
 * @return Zero for success, negative value otherwise.
 */
int GetMap(char *dest,int cluster_map_size)
{
	LARGE_INTEGER interval;
	NTSTATUS Status;
	ULONG i, k, index;
	ULONGLONG maximum, n;
	
	/* synchronize with map reallocation */
	interval.QuadPart = (-1); /* 100 nsec */
	Status = NtWaitForSingleObject(hMapEvent,FALSE,&interval);
	if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status)) return (-1);
	
	/* copy data */
	if(!new_cluster_map){
		(void)NtSetEvent(hMapEvent,NULL);
		return (-1);
	}
	if(cluster_map_size != map_size){
		DebugPrint("Map size is wrong: %u != %u!\n",
			cluster_map_size,map_size);
		(void)NtSetEvent(hMapEvent,NULL);
		return (-1);
	}
	for(i = 0; i < map_size; i++){
		maximum = new_cluster_map[i][0];
		index = 0;
		for(k = 1; k < NUM_OF_SPACE_STATES; k++){
			n = new_cluster_map[i][k];
			if(n >= maximum){ /* support of colors precedence  */
				maximum = n;
				index = k;
			}
		}
		dest[i] = (char)index;
	}
	(void)NtSetEvent(hMapEvent,NULL);
	return 0;
}

/**
 * @brief Marks all space in cluster map as free.
 * @details All cells of the map are marked as
 * containing a single cluster.
 */
void MarkAllSpaceAsFree0(void)
{
	ULONG i;
	
	if(!new_cluster_map) return;
	memset(new_cluster_map,0,NUM_OF_SPACE_STATES * map_size * sizeof(ULONGLONG));
	for(i = 0; i < map_size; i++)
		new_cluster_map[i][FREE_SPACE] = 1;
}

/**
 * @brief Marks all space in cluster map as system.
 * @details All cells of the map are marked as
 * containing a real number of clusters per cell.
 */
void MarkAllSpaceAsSystem1(void)
{
	ULONG i;
	
	if(!new_cluster_map) return;
	/* calculate map parameters */
	clusters_per_cell = clusters_total / map_size;
	if(clusters_per_cell){
		opposite_order = FALSE;
		clusters_per_last_cell = clusters_per_cell + \
			(clusters_total - clusters_per_cell * map_size);
		for(i = 0; i < map_size - 1; i++)
			new_cluster_map[i][SYSTEM_SPACE] = clusters_per_cell;
		new_cluster_map[i][SYSTEM_SPACE] = clusters_per_last_cell;
	} else {
		opposite_order = TRUE;
		cells_per_cluster = map_size / clusters_total;
		cells_per_last_cluster = cells_per_cluster + \
			(map_size - cells_per_cluster * clusters_total);
		DebugPrint("Opposite order %I64u:%I64u:%I64u\n", \
			clusters_total,cells_per_cluster,cells_per_last_cluster);
	}
}

/**
 * @brief Retrieves a space state of the file.
 * @param[in] pfn pointer to the FILENAME structure
 * containing information about the file.
 * @return A space state of the file.
 */
unsigned char GetFileSpaceState(PFILENAME pfn)
{
	UCHAR space_states[] = {UNFRAGM_SPACE,UNFRAGM_OVERLIMIT_SPACE, \
			      COMPRESSED_SPACE,COMPRESSED_OVERLIMIT_SPACE, \
			      DIR_SPACE,DIR_OVERLIMIT_SPACE,DIR_SPACE, \
			      DIR_OVERLIMIT_SPACE};
	int d,c,o;
	unsigned char state;
	PBLOCKMAP block;
	int i;

	/* show $MFT file in dark magenta color */
	if(IsMft(pfn)){
		i = 0;
		for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
			DebugPrint("MFT part #%u start: %I64u, length: %I64u\n",
				i,block->lcn,block->length);
			i ++;
			if(block->next_ptr == pfn->blockmap) break;
		}
		return MFT_SPACE;
	}
	
	/*
	* Show filtered out stuff and files above size threshold
	* and reparse points as unfragmented.
	*/
	if(pfn->is_fragm && !pfn->is_filtered && !pfn->is_overlimit && !pfn->is_reparse_point)
		state = pfn->is_overlimit ? FRAGM_OVERLIMIT_SPACE : FRAGM_SPACE;
	else{
		d = (int)(pfn->is_dir) & 0x1;
		c = (int)(pfn->is_compressed) & 0x1;
		o = (int)(pfn->is_overlimit) & 0x1;
		state = space_states[(d << 2) + (c << 1) + o];
	}
	return state;
}

/**
 * @brief Checks whether a range of clusters is outside MFT or not.
 */
static BOOLEAN IsOutsideMft(ULONGLONG start,ULONGLONG len)
{
	if(mft_end == 0 || (start + len) <= mft_start || start > mft_end) return TRUE;
	return FALSE;
}

/**
 * @brief Checks whether a range of clusters is outside MFT Zone or not.
 */
static BOOLEAN IsOutsideMftZone(ULONGLONG start,ULONGLONG len)
{
	if(mftzone_end == 0 || (start + len) <= mftzone_start || start > mftzone_end) return TRUE;
	return FALSE;
}

/**
 * @brief Checks whether a range of clusters is outside MFT Mirror or not.
 */
static BOOLEAN IsOutsideMftMirr(ULONGLONG start,ULONGLONG len)
{
	if(mftmirr_end == 0 || (start + len) <= mftmirr_start || start > mftmirr_end) return TRUE;
	return FALSE;
}

/**
 * @brief Checks whether a range of clusters is inside MFT or not.
 */
static BOOLEAN IsInsideMft(ULONGLONG start,ULONGLONG len)
{
	if(mft_end != 0 && start >= mft_start && (start + len) <= (mft_end + 1)) return TRUE;
	return FALSE;
}

/**
 * @brief Checks whether a range of clusters is inside MFT Zone or not.
 */
static BOOLEAN IsInsideMftZone(ULONGLONG start,ULONGLONG len)
{
	if(mftzone_end != 0 && start >= mftzone_start && (start + len) <= (mftzone_end + 1)) return TRUE;
	return FALSE;
}

/**
 * @brief Checks whether a range of clusters is inside MFT Mirror or not.
 */
static BOOLEAN IsInsideMftMirr(ULONGLONG start,ULONGLONG len)
{
	if(mftmirr_end != 0 && start >= mftmirr_start && (start + len) <= (mftmirr_end + 1)) return TRUE;
	return FALSE;
}

/**
 * @brief Remarks a range of clusters, respecting
 * whether they are inside mft zones or not.
 * @param[in] start the starting cluster of the block.
 * @param[in] len the length of the block, in clusters.
 * @param[in] new_space_state the new state of the marked space.
 * @param[in] old_space_state the previous state of the marked space.
 * @param[in] adjust_new_space_state boolean value indicating whether
 * the procedure must adjust the new space state or the old one.
 * @note The parameter representing space state to be adjusted
 * must contain default space state to be used when clusters are
 * outside mft zones.
 */
static void RemarkBlockRespectToMftZones(ULONGLONG start,ULONGLONG len,
		int new_space_state,int old_space_state,
		BOOLEAN adjust_new_space_state)
{
	BOOLEAN outside_mft, outside_mftzone, outside_mftmirr;
	BOOLEAN inside_mft, inside_mftzone, inside_mftmirr;
	ULONGLONG i;
	int default_space_state,adjusted_space_state;
	
	if(adjust_new_space_state) default_space_state = new_space_state;
	else default_space_state = old_space_state;

	/* check whether the block is outside mft zones */
	outside_mft = IsOutsideMft(start,len);
	outside_mftzone = IsOutsideMftZone(start,len);
	outside_mftmirr = IsOutsideMftMirr(start,len);
	if(outside_mft && outside_mftzone && outside_mftmirr){
		if(adjust_new_space_state)
			RemarkBlock(start,len,default_space_state,old_space_state);
		else
			RemarkBlock(start,len,new_space_state,default_space_state);
	} else {
		/* check whether the block is completely inside mft zone */
		inside_mft = IsInsideMft(start,len);
		inside_mftzone = IsInsideMftZone(start,len);
		inside_mftmirr = IsInsideMftMirr(start,len);
		if(inside_mft || inside_mftzone || inside_mftmirr){
			if(adjust_new_space_state)
				RemarkBlock(start,len,MFT_ZONE_SPACE,old_space_state);
			else
				RemarkBlock(start,len,new_space_state,MFT_ZONE_SPACE);
		} else {
			/* block is partially inside mft zone */
			/* this case is rare, it may encounter mo more than 6 times */
			/* therefore we can handle it easy, without optimizing the code for speed */
			for(i = start; i < start + len; i++){
				if((i < mft_start || i > mft_end) && (i < mftzone_start || i > mftzone_end) && 
				  (i < mftmirr_start || i > mftmirr_end))
					adjusted_space_state = default_space_state;
				else
					adjusted_space_state = MFT_ZONE_SPACE;
				if(adjust_new_space_state)
					RemarkBlock(i,1,adjusted_space_state,old_space_state);
				else
					RemarkBlock(i,1,new_space_state,adjusted_space_state);
			}
		}
	}
}

/**
 * @brief Remarks a range of clusters belonging to the file in the cluster map.
 * @param[in] pfn pointer to the FILENAME structure containing information about the file.
 * @param[in] old_space_state the previous state of the marked space.
 */
void MarkFileSpace(PFILENAME pfn,int old_space_state)
{
	PBLOCKMAP block;
	UCHAR new_space_state;

	new_space_state = GetFileSpaceState(pfn);
	for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
		RemarkBlock(block->lcn,block->length,new_space_state,old_space_state);
		if(block->next_ptr == pfn->blockmap) break;
	}
}

/**
 * @brief Remarks a range of clusters belonging to the file as system clusters in the cluster map.
 * @param[in] pfn pointer to the FILENAME structure containing information about the file.
 */
void RemarkFileSpaceAsSystem(PFILENAME pfn)
{
	PBLOCKMAP block;
	UCHAR state, new_state;
	
	state = GetFileSpaceState(pfn);
	new_state = pfn->is_overlimit ? SYSTEM_OVERLIMIT_SPACE : SYSTEM_SPACE;
	for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
		RemarkBlock(block->lcn,block->length,new_state,state);
		if(block->next_ptr == pfn->blockmap) break;
	}
}

/**
 * @brief Remarks a range of clusters in the cluster map.
 * @param[in] start the starting cluster of the block.
 * @param[in] len the length of the block, in clusters.
 * @param[in] space_state the new state of the marked space.
 * @param[in] old_space_state the previous state of the marked space.
 */
void RemarkBlock(ULONGLONG start,ULONGLONG len,int space_state,int old_space_state)
{
	ULONGLONG cell, offset, n;
	ULONGLONG ncells, i, j;
	ULONGLONG *c;
	PFREEBLOCKMAP block;
	ULONGLONG current_cluster, clusters_to_process, length;

	/* return if we don't need cluster map */
	if(!new_cluster_map) return;
	
	/* check parameters */
	if(space_state < 0 || (space_state >= NUM_OF_SPACE_STATES && space_state != FREE_OR_MFT_ZONE_SPACE)) return;
	if(old_space_state < 0 || (old_space_state >= NUM_OF_SPACE_STATES && 
		old_space_state != SYSTEM_OR_FREE_SPACE && old_space_state != SYSTEM_OR_MFT_ZONE_SPACE)) return;
	if(start + len > clusters_total) return;

	if(old_space_state == SYSTEM_OR_FREE_SPACE){
		current_cluster = start;
		clusters_to_process = len;
		for(block = free_space_map; block != NULL; block = block->next_ptr){
			/* break if current block follows the target */
			if(block->lcn >= start + len){
				if(clusters_to_process)
					RemarkBlock(current_cluster,clusters_to_process,space_state,SYSTEM_SPACE);
				break;
			}
			/* skip preceding blocks */
			if(block->lcn >= current_cluster){
				if(block->lcn > current_cluster){
					length = block->lcn - current_cluster;
					RemarkBlock(current_cluster,length,space_state,SYSTEM_SPACE);
					current_cluster += length;
					clusters_to_process -= length;
				}
				/* now block->lcn is equal to current_block always */
				length = min(block->length,clusters_to_process);
				RemarkBlock(current_cluster,length,space_state,FREE_SPACE);
				current_cluster += length;
				clusters_to_process -= length;
			}
			if(block->next_ptr == free_space_map) break;
		}
		return;
	}
	
	if(old_space_state == SYSTEM_OR_MFT_ZONE_SPACE){
		RemarkBlockRespectToMftZones(start,len,space_state,SYSTEM_SPACE,FALSE);
		return;
	}
	
	if(space_state == FREE_OR_MFT_ZONE_SPACE){
		RemarkBlockRespectToMftZones(start,len,FREE_SPACE,old_space_state,TRUE);
		return;
	}

	if(!opposite_order){
		if(!clusters_per_cell) return;
		/*
		* The following code uses _aulldvrm() function:
		* cell = start / dx->clusters_per_cell;
		* offset = start % dx->clusters_per_cell;
		* But we don't have this function on NT 4.0!!!
		*/
		cell = start / clusters_per_cell;
		offset = start - cell * clusters_per_cell;
		while((cell < (map_size - 1)) && len){
			n = min(len,clusters_per_cell - offset);
			new_cluster_map[cell][space_state] += n;
			c = &new_cluster_map[cell][old_space_state];
			if(*c >= n) *c -= n; else *c = 0;
			len -= n;
			cell ++;
			offset = 0;
		}
		
		/* ckeck cell correctness */
		if(cell >= map_size) return; /* important !!! */
		
		if(len){
			n = min(len,clusters_per_last_cell - offset);
			new_cluster_map[cell][space_state] += n;
			c = &new_cluster_map[cell][old_space_state];
			if(*c >= n) *c -= n; else *c = 0;
		}
	} else { /* dx->opposite_order */
		cell = start * cells_per_cluster;
		ncells = len * cells_per_cluster;
		/*if(start + len > dx->clusters_total) KeBugCheck();*/
		if(start + len == clusters_total)
			ncells += (cells_per_last_cluster - cells_per_cluster);
		for(i = 0; i < ncells; i++){
			for(j = 0; j < NUM_OF_SPACE_STATES; j++)
				new_cluster_map[cell + i][j] = 0;
			new_cluster_map[cell + i][space_state] = 1;
		}
	}
}

/**
 * @brief Frees the cluster map.
 */
void FreeMap(void)
{
	if(new_cluster_map){
		winx_heap_free(new_cluster_map);
		new_cluster_map = NULL;
	}
	map_size = 0;
}

/** @} */
