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
* User mode driver - cluster map related code.
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

/* zero for success, -1 otherwise */
int AllocateMap(int size)
{
	LARGE_INTEGER interval;
	NTSTATUS Status;
	int buffer_size;
	
	/* synchronize with GetMap requests */
	interval.QuadPart = (-1 * 10000 * 1000); /* 1 sec */
	Status = NtWaitForSingleObject(hMapEvent,FALSE,&interval);
	if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status)){
		winx_raise_error("E: NtWaitForSingleObject(hMapEvent,...) failed: %x!",
			(UINT)Status);
		return (-1);
	}

	DebugPrint("Map size = %u\n",size);
	if(new_cluster_map) FreeMap();
	if(!size){
		NtSetEvent(hMapEvent,NULL);
		return 0; /* console/native apps may work without cluster map */
	}
	buffer_size = NUM_OF_SPACE_STATES * size * sizeof(ULONGLONG);
	new_cluster_map = winx_virtual_alloc(buffer_size);
	if(!new_cluster_map){
		winx_raise_error("E: Cannot allocate %u bytes of memory for cluster map!",
			buffer_size);
		NtSetEvent(hMapEvent,NULL);
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
	NtSetEvent(hMapEvent,NULL);
	return 0;
}

void GetMap(char *dest,int cluster_map_size)
{
	LARGE_INTEGER interval;
	NTSTATUS Status;
	ULONG i, k, index;
	ULONGLONG maximum, n;
	
	/* synchronize with map reallocation */
	interval.QuadPart = (-1); /* 100 nsec */
	Status = NtWaitForSingleObject(hMapEvent,FALSE,&interval);
	if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status)) return;
	
	/* copy data */
	if(!new_cluster_map) goto cleanup;
	if(cluster_map_size != map_size){
		DebugPrint2("Map size is wrong: %u != %u!\n",
			cluster_map_size,map_size);
		goto cleanup;
	}
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
cleanup:
	NtSetEvent(hMapEvent,NULL);
	return;
}

/* marks all space as system with 1 cluster per cell */
void MarkAllSpaceAsSystem0(void)
{
	ULONG i;
	
	if(!new_cluster_map) return;
	memset(new_cluster_map,0,NUM_OF_SPACE_STATES * map_size * sizeof(ULONGLONG));
	for(i = 0; i < map_size; i++)
		new_cluster_map[i][SYSTEM_SPACE] = 1;
}

/* corrects number of clusters per cell set by MarkAllSpaceAsSystem0() */
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
		DebugPrint("-Ultradfg- opposite order %I64u:%I64u:%I64u\n", \
			clusters_total,cells_per_cluster,cells_per_last_cluster);
	}
}

/* returns current space state for the specified file */
unsigned char GetSpaceState(PFILENAME pfn)
{
	UCHAR space_states[] = {UNFRAGM_SPACE,UNFRAGM_OVERLIMIT_SPACE, \
			      COMPRESSED_SPACE,COMPRESSED_OVERLIMIT_SPACE, \
			      DIR_SPACE,DIR_OVERLIMIT_SPACE,DIR_SPACE, \
			      DIR_OVERLIMIT_SPACE};
	int d,c,o;
	unsigned char state;

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

/* marks space allocated by specified file */
void MarkSpace(PFILENAME pfn,int old_space_state)
{
	PBLOCKMAP block;
	UCHAR state;
	
	state = GetSpaceState(pfn);
	for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
		ProcessBlock(block->lcn,block->length,state,old_space_state);
		if(block->next_ptr == pfn->blockmap) break;
	}
}

/* applies clusters block data to cluster map */
void ProcessBlock(ULONGLONG start,ULONGLONG len,int space_state,int old_space_state)
{
	ULONGLONG cell, offset, n;
	ULONGLONG ncells, i, j;

	/* return if we don't need cluster map */
	if(!new_cluster_map) return;
	
	/* check parameters */
	if(space_state < 0 || space_state >= NUM_OF_SPACE_STATES) return;
	if(old_space_state < 0 || old_space_state >= NUM_OF_SPACE_STATES) return;
	if(start + len > clusters_total) return;

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
			if(new_cluster_map[cell][old_space_state] >= n)
				new_cluster_map[cell][old_space_state] -= n;
			else
				new_cluster_map[cell][old_space_state] = 0;
			len -= n;
			cell ++;
			offset = 0;
		}
		
		/* ckeck cell correctness */
		if(cell >= map_size) return; /* important !!! */
		
		if(len){
			n = min(len,clusters_per_last_cell - offset);
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

void FreeMap(void)
{
	int buffer_size;
	
	if(new_cluster_map){
		buffer_size = NUM_OF_SPACE_STATES * map_size * sizeof(ULONGLONG);
		winx_virtual_free(new_cluster_map,buffer_size);
		new_cluster_map = NULL;
	}
	map_size = 0;
}
