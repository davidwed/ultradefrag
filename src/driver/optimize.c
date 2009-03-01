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

void MovePartOfFileBlock(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,ULONGLONG startVcn,
		ULONGLONG targetLcn,ULONGLONG n_clusters);

/*
* NOTES:
* 1. On FAT it's bad idea, because dirs aren't moveable.
* 2. On NTFS cycle of attempts is a bad solution,
* because it increases processing time. Also on NTFS all 
* space freed during the defragmentation is still temporarily
* allocated by system for a long time.
* 3. It makes an analysis after each volume optimization to 
* update list of fragmented files.
* 4. Usually this function increases a number of fragmented files.
* 5. We cannot update a number of fragmented files during the 
* optimization process.
*/
void DefragmentFreeSpace(UDEFRAG_DEVICE_EXTENSION *dx)
{
	KSPIN_LOCK spin_lock;
	KIRQL oldIrql;
	PFILENAME pfn, lastpfn;
	PBLOCKMAP block, lastblock;
	PFREEBLOCKMAP freeblock;
	ULONGLONG maxlcn;
	ULONGLONG vcn, length;
	ULONGLONG movings;
	
	DebugPrint("-Ultradfg- ----- Optimization of %c: -----\n",NULL,dx->letter);
	DeleteLogFile(dx);

	/* Initialize progress counters. */
	KeInitializeSpinLock(&spin_lock);
	KeAcquireSpinLock(&spin_lock,&oldIrql);
	dx->clusters_to_process = dx->processed_clusters = 0;
	KeReleaseSpinLock(&spin_lock,oldIrql);
	for(freeblock = dx->free_space_map; freeblock != NULL; freeblock = freeblock->next_ptr){
		if(freeblock->next_ptr == dx->free_space_map) break;
		dx->clusters_to_process += freeblock->length;
	}
	dx->current_operation = 'C';
	
	/* On FAT volumes it increase distance between dir & files inside it. */
	if(dx->partition_type != NTFS_PARTITION) return;

	while(1){
		/* 1. Find the latest file block on the volume. */
		lastblock = NULL; lastpfn = NULL; maxlcn = 0;
		for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
			for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
				if(block->lcn > maxlcn){
					lastblock = block;
					lastpfn = pfn;
					maxlcn = block->lcn;
				}
				if(block->next_ptr == pfn->blockmap) break;
			}
			if(pfn->next_ptr == dx->filelist) break;
		}
		if(!lastblock) break;
		DebugPrint("-Ultradfg- Last block = Lcn:%I64u Length:%I64u\n",lastpfn->name.Buffer,
			lastblock->lcn,lastblock->length);
		if(KeReadStateEvent(&stop_event)) break;
		/* 2. Fill free space areas in the beginning of the volume with lastblock contents. */
		movings = 0;
		for(freeblock = dx->free_space_map; freeblock != NULL; freeblock = freeblock->next_ptr){
			if(freeblock->lcn < lastblock->lcn && freeblock->length){
				/* fill block with lastblock contents */
				length = min(freeblock->length,lastblock->length);
				vcn = lastblock->vcn + (lastblock->length - length);
				MovePartOfFileBlock(dx,lastpfn,vcn,freeblock->lcn,length);
				movings ++;
				dx->processed_clusters += length;
				ProcessBlock(dx,freeblock->lcn,length,UNKNOWN_SPACE,FREE_SPACE);
				ProcessBlock(dx,lastblock->lcn + (lastblock->length - length),length,
					FREE_SPACE,GetSpaceState(lastpfn));
				freeblock->length -= length;
				freeblock->lcn += length;
				lastblock->length -= length;
				if(!lastblock->length){
					RemoveItem((PLIST *)&lastpfn->blockmap,(PLIST)lastblock);
					break;
				}
			}
			if(KeReadStateEvent(&stop_event)) break;
			if(freeblock->next_ptr == dx->free_space_map) break;
		}
		if(!movings) break;
	}
	
	/* Analyse volume again to update fragmented files list. */
	KeClearEvent(&stop_event);
	Analyse(dx);
}

void MovePartOfFileBlock(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,ULONGLONG startVcn,
		ULONGLONG targetLcn,ULONGLONG n_clusters)
{
	HANDLE hFile;
	ULONGLONG target;
	ULONGLONG j,n,r;
	NTSTATUS Status;

	Status = OpenTheFile(pfn,&hFile);
	if(Status) return;

	target = targetLcn;
	n = n_clusters / dx->clusters_per_256k;
	for(j = 0; j < n; j++){
		Status = MovePartOfFile(dx,hFile,startVcn + j * dx->clusters_per_256k, \
			target,dx->clusters_per_256k);
		if(Status){ ZwClose(hFile); return; }
		target += dx->clusters_per_256k;
	}
	r = n_clusters % dx->clusters_per_256k;
	if(r){
		Status = MovePartOfFile(dx,hFile,startVcn + j * dx->clusters_per_256k, \
			target,r);
		if(Status){ ZwClose(hFile); return; }
	}

	ZwClose(hFile);
}
