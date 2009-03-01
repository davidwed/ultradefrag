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
* Defragmenter's engine / Kernel of the defragmenter /.
*/

#include "driver.h"

BOOLEAN MoveTheFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,ULONGLONG target);

/*
* Why analysis repeats before each defragmentation attempt (v2.1.2)?
*
* 1. On NTFS volumes we must do it, because otherwise Windows will not free
*    temporarily allocated space.
* 2. Modern FAT volumes aren't large enough to extremely slow down 
*    defragmentation by an additional analysis.
* 3. We cannot use NtNotifyChangeDirectoryFile() function in kernel mode.
*    It always returns 0xc0000005 error code (tested in XP 32-bit). It seems
*    that it's arguments must be in user memory space.
* 4. Filesystem data is cached by Windows, therefore second analysis will be
*    always much faster (at least on modern versions of Windows - tested on XP)
*    then the first attempt.
* 5. On very large volumes the second analysis may take few minutes. Well, the 
*    first one may take few hours... :)
*/

void Defragment(UDEFRAG_DEVICE_EXTENSION *dx)
{
	KSPIN_LOCK spin_lock;
	KIRQL oldIrql;
	PFRAGMENTED pf, plargest;
	PFREEBLOCKMAP block;
	ULONGLONG length;

	DebugPrint("-Ultradfg- ----- Defragmentation of %c: -----\n",NULL,dx->letter);
	DeleteLogFile(dx);

	/* Initialize progress counters. */
	KeInitializeSpinLock(&spin_lock);
	KeAcquireSpinLock(&spin_lock,&oldIrql);
	dx->clusters_to_process = dx->processed_clusters = 0;
	KeReleaseSpinLock(&spin_lock,oldIrql);

	for(pf = dx->fragmfileslist; pf != NULL; pf = pf->next_ptr){
		if(!pf->pfn->blockmap) goto next_item; /* skip fragmented files with unknown state */
		if(!dx->compact_flag){
			if(pf->pfn->is_overlimit) goto next_item; /* skip fragmented but filtered out files */
			if(pf->pfn->is_filtered) goto next_item;
			/* skip fragmented directories on FAT/UDF partitions */
			if(pf->pfn->is_dir && dx->partition_type != NTFS_PARTITION) goto next_item;
		}
		dx->clusters_to_process += pf->pfn->clusters_total;
	next_item:
		if(pf->next_ptr == dx->fragmfileslist) break;
	}

	dx->current_operation = 'D';
	
	/* Fill free space areas. */
	for(block = dx->free_space_map; block != NULL; block = block->next_ptr){
	L0:
		if(block->length <= 1) goto L1; /* skip 1 cluster blocks and zero length blocks */
		/* find largest fragmented file that can be stored here */
		plargest = NULL; length = 0;
		for(pf = dx->fragmfileslist; pf != NULL; pf = pf->next_ptr){
			if(!pf->pfn->blockmap) goto L2; /* skip fragmented files with unknown state */
			if(!dx->compact_flag){
				if(pf->pfn->is_overlimit) goto L2; /* skip fragmented but filtered out files */
				if(pf->pfn->is_filtered) goto L2;
				/* skip fragmented directories on FAT/UDF partitions */
				if(pf->pfn->is_dir && dx->partition_type != NTFS_PARTITION) goto L2;
			}
			if(pf->pfn->clusters_total <= block->length){
				if(pf->pfn->clusters_total > length){
					plargest = pf;
					length = pf->pfn->clusters_total;
				}
			}
		L2:
			if(pf->next_ptr == dx->fragmfileslist) break;
		}
		if(!plargest) goto L1; /* current block is too small */
		/* move file */
		if(MoveTheFile(dx,plargest->pfn,block->lcn))
			DebugPrint("-Ultradfg- Defrag success for\n",plargest->pfn->name.Buffer);
		else
			DebugPrint("-Ultradfg- Defrag error for\n",plargest->pfn->name.Buffer);
		dx->processed_clusters += plargest->pfn->clusters_total;
		UpdateFragmentedFilesList(dx);
		if(KeReadStateEvent(&stop_event)) break;
		/* after file moving continue from the first free space block */
		block = dx->free_space_map; if(!block) break; else goto L0;
	L1:
		if(block->next_ptr == dx->free_space_map) break;
	}
	SaveFragmFilesListToDisk(dx);
}

NTSTATUS MovePartOfFile(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile, 
		    ULONGLONG startVcn, ULONGLONG targetLcn, ULONGLONG n_clusters)
{
	ULONG status;
	IO_STATUS_BLOCK ioStatus;

	DebugPrint("-Ultradfg- sVcn: %I64u,tLcn: %I64u,n: %u\n",NULL,
		 startVcn,targetLcn,n_clusters);

	if(KeReadStateEvent(&stop_event)) return STATUS_UNSUCCESSFUL;

	/* Setup movefile descriptor and make the call */
	dx->pmoveFile->FileHandle = hFile;
	dx->pmoveFile->StartVcn.QuadPart = startVcn;
	dx->pmoveFile->TargetLcn.QuadPart = targetLcn;
#ifdef _WIN64
	dx->pmoveFile->NumVcns = n_clusters;
#else
	dx->pmoveFile->NumVcns = (ULONG)n_clusters;
#endif
	/* On NT 4.0 it can move no more than 256 kbytes once. */
	status = ZwFsControlFile(dx->hVol,NULL,NULL,0,&ioStatus,
						FSCTL_MOVE_FILE,
						dx->pmoveFile,sizeof(MOVEFILE_DESCRIPTOR),
						NULL,0);

	/* If the operation is pending, wait for it to finish */
	if(status == STATUS_PENDING){
		NtWaitForSingleObject(hFile,FALSE,NULL);
		status = ioStatus.Status;
	}
	if(!NT_SUCCESS(status)) return status;
	return STATUS_SUCCESS; /* it means: the result is unknown */
}

/* Tries to move the file entirely. */
NTSTATUS MoveBlocksOfFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,
			  HANDLE hFile,ULONGLONG targetLcn)
{
	PBLOCKMAP block;
	ULONGLONG curr_target;
	ULONGLONG j,n,r;
	NTSTATUS Status;

	curr_target = targetLcn;
	for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
		/* try to move current block */
		n = block->length / dx->clusters_per_256k;
		for(j = 0; j < n; j++){
			Status = MovePartOfFile(dx,hFile,block->vcn + j * dx->clusters_per_256k, \
				curr_target,dx->clusters_per_256k);
			if(Status) return Status;
			curr_target += dx->clusters_per_256k;
		}
		/* try to move rest of block */
		r = block->length % dx->clusters_per_256k;
		if(r){
			Status = MovePartOfFile(dx,hFile,block->vcn + j * dx->clusters_per_256k, \
				curr_target,r);
			if(Status) return Status;
			curr_target += r;
		}
		if(block->next_ptr == pfn->blockmap) break;
	}
	return STATUS_SUCCESS;
}

/* For defragmenter only, not for optimizer! */
BOOLEAN MoveTheFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,ULONGLONG target)
{
	NTSTATUS Status;
	HANDLE hFile;
	PBLOCKMAP block;
	FILENAME fn;

	/* Open the file */
	Status = OpenTheFile(pfn,&hFile);
	if(Status){
		DebugPrint("-Ultradfg- Can't open file: %x\n",pfn->name.Buffer,(UINT)Status);
		/* we need to destroy the block map to avoid infinite loops */
		DeleteBlockmap(pfn); /* file is locked by other application, so its state is unknown */
		return FALSE;
	}

	DebugPrint("-Ultradfg-\n",pfn->name.Buffer);
	DebugPrint("-Ultradfg- t: %I64u n: %I64u\n",NULL,target,pfn->clusters_total);

	Status = MoveBlocksOfFile(dx,pfn,hFile,target);
	ZwClose(hFile);
	
	if(Status == STATUS_SUCCESS){ /* we have undefined result */
		/* Check new file blocks to get moving status. */
		fn.name = pfn->name;
		if(DumpFile(dx,&fn)){ /* otherwise let's assume the successful moving result? */
			if(fn.is_fragm) Status = STATUS_UNSUCCESSFUL;
			if(fn.blockmap){
				if(fn.blockmap->lcn != target) Status = STATUS_UNSUCCESSFUL;
			}
			DeleteBlockmap(&fn);
		}
	}

	/* first of all: remove target space from free space pool */
	if(Status == STATUS_SUCCESS){
		dx->fragmfilecounter --;
		dx->fragmcounter -= (pfn->n_fragments - 1);
		pfn->is_fragm = FALSE; /* before GetSpaceState() call */
	}
	ProcessBlock(dx,target,pfn->clusters_total,GetSpaceState(pfn),FREE_SPACE);
	TruncateFreeSpaceBlock(dx,target,pfn->clusters_total);

	if(Status == STATUS_SUCCESS){
		/* free previously allocated space (after TruncateFreeSpaceBlock() call!) */
		for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
			ProcessFreeBlock(dx,block->lcn,block->length,FRAGM_SPACE/*old_state*/);
			if(block->next_ptr == pfn->blockmap) break;
		}
	} else {
		DebugPrint("-Ultradfg- MoveFile error: %x\n",NULL,(UINT)Status);
	}
	DeleteBlockmap(pfn); /* because we don't need this info after file moving */
	return (!pfn->is_fragm);
}
