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

/* zero for success, -1 otherwise */
int AllocateMap(int size)
{
	int buffer_size;
	
	DebugPrint("Map size = %u\n",size);
	if(new_cluster_map) FreeMap();
	if(!size) return (-1);
	buffer_size = NUM_OF_SPACE_STATES * size * sizeof(ULONGLONG);
	new_cluster_map = winx_virtual_alloc(buffer_size);
	if(!new_cluster_map){
		winx_dbg_print("Cannot allocate %u bytes for cluster map\n",
			buffer_size);
		return (-1);
	}
	map_size = size;
	memset(new_cluster_map,0,buffer_size);
	return 0;
}

void GetMap(char *dest,int cluster_map_size)
{
	ULONG i, k, index;
	ULONGLONG maximum, n;
	
	if(!new_cluster_map) return;
	if(cluster_map_size != map_size){
		DebugPrint2("Map size is wrong: %u != %u!\n",
			cluster_map_size,map_size);
		return;
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
