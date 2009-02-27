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
* Volume optimization routines.
*/

#include "driver.h"

#if 0
/*
* NOTES:
* 1. On FAT it's bad idea, because dirs aren't moveable.
* 2. On NTFS cycle of attempts is a bad solution,
* because it increases processing time. Also on NTFS all 
* space freed during the defragmentation is still temporarily
* allocated by system for a long time.
*/
void DefragmentFreeSpace(UDEFRAG_DEVICE_EXTENSION *dx)
{
	KSPIN_LOCK spin_lock;
	KIRQL oldIrql;
	PFILENAME curr_file;

	DebugPrint("-Ultradfg- ----- Optimization of %c: -----\n",NULL,dx->letter);
	DeleteLogFile(dx);

	/* Initialize progress counters. */
	KeInitializeSpinLock(&spin_lock);
	KeAcquireSpinLock(&spin_lock,&oldIrql);
	dx->clusters_to_process = dx->processed_clusters = 0;
	KeReleaseSpinLock(&spin_lock,oldIrql);
	for(curr_file = dx->filelist; curr_file != NULL; curr_file = curr_file->next_ptr){
		if(!curr_file->blockmap) continue;
		dx->clusters_to_process += curr_file->clusters_total;
	}
	dx->current_operation = 'C';

	/* On FAT volumes it increase distance between dir & files inside it. */
	if(dx->partition_type != NTFS_PARTITION){
		dx->processed_clusters = dx->clusters_to_process;
		return;
	}

	/* process all files */
	for(curr_file = dx->filelist; curr_file != NULL; curr_file = curr_file->next_ptr){
		/* skip system files */
		if(!curr_file->blockmap) continue;
		if(KeReadStateEvent(&stop_event)) break;
		DefragmentFile(dx,curr_file);
		dx->processed_clusters += curr_file->clusters_total;
	}
	UpdateFragmentedFilesList(dx);
	SaveFragmFilesListToDisk(dx);
}
#endif

void DefragmentFreeSpace(UDEFRAG_DEVICE_EXTENSION *dx)
{
}
