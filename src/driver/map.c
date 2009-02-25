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
* Cluster map (required by gui apps) processing.
*/

#include "driver.h"

/* allocates buffer for cluster map of specified size */
NTSTATUS AllocateMap(ULONG size)
{
#ifndef MICRO_EDITION
	/* map reallocation doesn't supported yet */
	if(new_cluster_map){
		DebugPrint("-Ultradfg- map reallocation doesn't supported yet!\n",NULL);
		return STATUS_INVALID_DEVICE_REQUEST;
	}
	map_size = size;
	DebugPrint("-Ultradfg- Map size = %u\n",NULL,map_size);
	if(!size) return STATUS_SUCCESS;
	new_cluster_map = AllocatePool(NonPagedPool,
			NUM_OF_SPACE_STATES * size * sizeof(ULONGLONG));
	if(!new_cluster_map){
		DebugPrint("-Ultradfg- cannot allocate memory for cluster map!\n",NULL);
		map_size = 0;
		return STATUS_NO_MEMORY;
	}
#endif
	return STATUS_SUCCESS;
}

/* returns current space state for the specified file */
unsigned char GetSpaceState(PFILENAME pfn)
{
#ifndef MICRO_EDITION
	UCHAR space_states[] = {UNFRAGM_SPACE,UNFRAGM_OVERLIMIT_SPACE, \
			      COMPRESSED_SPACE,COMPRESSED_OVERLIMIT_SPACE, \
			      DIR_SPACE,DIR_OVERLIMIT_SPACE,DIR_SPACE, \
			      DIR_OVERLIMIT_SPACE};
	int d,c,o;
	unsigned char state;

	if(pfn->is_fragm)
		state = pfn->is_overlimit ? FRAGM_OVERLIMIT_SPACE : FRAGM_SPACE;
	else{
		d = (int)(pfn->is_dir) & 0x1;
		c = (int)(pfn->is_compressed) & 0x1;
		o = (int)(pfn->is_overlimit) & 0x1;
		state = space_states[(d << 2) + (c << 1) + o];
	}
	return state;
#else
	return 0;
#endif
}

/* marks space allocated by specified file */
void MarkSpace(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,int old_space_state)
{
#ifndef MICRO_EDITION
	PBLOCKMAP block;
	UCHAR state;
	
	state = GetSpaceState(pfn);
//	for(block = pfn->blockmap; block != NULL; block = block->next_ptr)
//		ProcessBlock(dx,block->lcn,block->length,state,old_space_state);
	block = pfn->blockmap;
	if(!block) return;
	do {
		ProcessBlock(dx,block->lcn,block->length,state,old_space_state);
		block = block->next_ptr;
	} while (block != pfn->blockmap);
#endif
}

/* applies clusters block data to cluster map */
void ProcessBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,
				  ULONGLONG len,int space_state,int old_space_state)
{
#ifndef MICRO_EDITION
	ULONGLONG cell, offset, n;
	ULONGLONG ncells, i, j;

	/* return if we don't need cluster map */
	if(!new_cluster_map) return;

	if(!dx->opposite_order){
		if(!dx->clusters_per_cell) return;
		/*
		* The following code uses _aulldvrm() function:
		* cell = start / dx->clusters_per_cell;
		* offset = start % dx->clusters_per_cell;
		* But we don't have this function on NT 4.0!!!
		*/
		cell = start / dx->clusters_per_cell;
		offset = start - cell * dx->clusters_per_cell;
		while((cell < (map_size - 1)) && len){
			n = min(len,dx->clusters_per_cell - offset);
			new_cluster_map[cell][space_state] += n;
			if(new_cluster_map[cell][old_space_state] >= n)
				new_cluster_map[cell][old_space_state] -= n;
			else
				new_cluster_map[cell][old_space_state] = 0;
			len -= n;
			cell ++;
			offset = 0;
		}
		if(len){
			n = min(len,dx->clusters_per_last_cell - offset);
			new_cluster_map[cell][space_state] += n;
			/*
			* Some space is identified as free and mft allocated at the same time;
			* therefore this check is required;
			* Because in space states enum mft has number above free space number,
			* these blocks will be displayed as mft blocks.
			*/
			if(new_cluster_map[cell][old_space_state] >= n)
				new_cluster_map[cell][old_space_state] -= n;
			else
				new_cluster_map[cell][old_space_state] = 0;
		}
	} else { /* dx->opposite_order */
		cell = start * dx->cells_per_cluster;
		ncells = len * dx->cells_per_cluster;
		/*if(start + len > dx->clusters_total) KeBugCheck();*/
		if(start + len == dx->clusters_total)
			ncells += (dx->cells_per_last_cluster - dx->cells_per_cluster);
		for(i = 0; i < ncells; i++){
			for(j = 0; j < NUM_OF_SPACE_STATES; j++)
				new_cluster_map[cell + i][j] = 0;
			new_cluster_map[cell + i][space_state] = 1;
		}
	}
#endif
}

/* marks all space as system with 1 cluster per cell */
void MarkAllSpaceAsSystem0(UDEFRAG_DEVICE_EXTENSION *dx)
{
#ifndef MICRO_EDITION
	ULONG i;
	
	if(!new_cluster_map) return;
	memset(new_cluster_map,0,NUM_OF_SPACE_STATES * map_size * sizeof(ULONGLONG));
	for(i = 0; i < map_size; i++)
		new_cluster_map[i][SYSTEM_SPACE] = 1;
#endif
}

/* corrects number of clusters per cell set by MarkAllSpaceAsSystem0() */
void MarkAllSpaceAsSystem1(UDEFRAG_DEVICE_EXTENSION *dx)
{
#ifndef MICRO_EDITION
	ULONG i;
	
	if(!new_cluster_map) return;
	/* calculate map parameters */
	dx->clusters_per_cell = dx->clusters_total / map_size;
	if(dx->clusters_per_cell){
		dx->opposite_order = FALSE;
		dx->clusters_per_last_cell = dx->clusters_per_cell + \
			(dx->clusters_total - dx->clusters_per_cell * map_size);
		for(i = 0; i < map_size - 1; i++)
			new_cluster_map[i][SYSTEM_SPACE] = dx->clusters_per_cell;
		new_cluster_map[i][SYSTEM_SPACE] = dx->clusters_per_last_cell;
	} else {
		dx->opposite_order = TRUE;
		dx->cells_per_cluster = map_size / dx->clusters_total;
		dx->cells_per_last_cluster = dx->cells_per_cluster + \
			(map_size - dx->cells_per_cluster * dx->clusters_total);
		DebugPrint("-Ultradfg- opposite order %I64u:%I64u:%I64u\n",NULL, \
			dx->clusters_total,dx->cells_per_cluster,dx->cells_per_last_cluster);
	}
#endif
}

void GetMap(char *dest)
{
#ifndef MICRO_EDITION
	ULONG i, k, index;
	ULONGLONG maximum, n;
	
	if(!new_cluster_map) return;
	for(i = 0; i < map_size; i++){
		maximum = new_cluster_map[i][0];
		index = 0;
		for(k = 1; k < NUM_OF_SPACE_STATES; k++){
			n = new_cluster_map[i][k];
			/* >= is very important: mft and free */
			if(n > maximum || (n == maximum && k != NO_CHECKED_SPACE)){
				maximum = n;
				index = k;
			}
		}
		dest[i] = (char)index;
	}
#endif
}
