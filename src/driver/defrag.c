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
* Defragmenter engine.
*/

#include "driver.h"

/*
* Since the 2.0.1 version the state of files 
* after defragmentation will be unknown.
* So we need analyse() call before 
* each next defragmentation.
* We can simply do it, because the OS
* (at least XP) has disk requests caching system.
* The first analysis will be still slow enough,
* but each next one will be very fast 
* (no more than few seconds).
*
* Therefore the code that corrects file maps
* after each defragmentation will be removed.
*
* Due to imperfectness of the NTFS file system
* the CheckPendingBlocks() call always returns
* failure, and we should remove them.
*
* We can destroy block map after defragmentation
* because on NTFS we need analysis due to 
* file system imperfectness. On the other hand,
* FAT formatted volumes usually aren't big enough 
* to slow down the process.
*/


/* Kernel of defragmenter */
void Defragment(UDEFRAG_DEVICE_EXTENSION *dx)
{
	KSPIN_LOCK spin_lock;
	KIRQL oldIrql;
	PFRAGMENTED pflist;
	PFILENAME curr_file;

	KeClearEvent(&dx->stop_event);
	DeleteLogFile(dx);

	/* Initialize progress counters. */
	KeInitializeSpinLock(&spin_lock);
	KeAcquireSpinLock(&spin_lock,&oldIrql);
	dx->clusters_to_process = dx->processed_clusters = 0;
	KeReleaseSpinLock(&spin_lock,oldIrql);
	for(pflist = dx->fragmfileslist; pflist != NULL; pflist = pflist->next_ptr){
		if(!pflist->pfn->blockmap) continue;
		if(pflist->pfn->is_overlimit) continue;
		if(pflist->pfn->is_filtered) continue;
		if(pflist->pfn->is_dir && dx->partition_type != NTFS_PARTITION) continue;
		dx->clusters_to_process += pflist->pfn->clusters_total;
	}
	dx->current_operation = 'D';

	/* process fragmented files */
	dx->invalid_movings = 0;
	for(pflist = dx->fragmfileslist; pflist != NULL; pflist = pflist->next_ptr){
		curr_file = pflist->pfn;
		/* skip system and unfragmented files */
		if(!curr_file->blockmap) continue;
		if(!curr_file->is_fragm) continue;
		/* skip directories on FAT part. */
		if(curr_file->is_dir && dx->partition_type != NTFS_PARTITION)
			continue;
		/* skip filtered out files */
		if(curr_file->is_overlimit) continue;
		if(curr_file->is_filtered) continue;
		if(KeReadStateEvent(&dx->stop_event) == 0x1)
			goto exit_defrag;
		if(DefragmentFile(dx,curr_file))
			DebugPrint("-Ultradfg- Defrag success.\n");
		else
			DebugPrint("-Ultradfg- Defrag error for %ws\n",curr_file->name.Buffer);
		dx->processed_clusters += curr_file->clusters_total;
	}

exit_defrag:
	UpdateFragmentedFilesList(dx);
	SaveFragmFilesListToDisk(dx);
	/* The state of some processed files maybe unknown... */
	if(!dx->invalid_movings && KeReadStateEvent(&dx->stop_event) == 0x0 && \
		dx->partition_type != NTFS_PARTITION) dx->status = STATUS_DEFRAGMENTED;
	else dx->status = STATUS_BEFORE_PROCESSING;
}

/*
 * NOTES:
 * 1. On FAT it's bad idea, because dirs isn't moveable.
 * 2. On NTFS cycle of attempts is bad solution,
 *    because it increase processing time and can be as while(1) {}.
 */
void DefragmentFreeSpace(UDEFRAG_DEVICE_EXTENSION *dx)
{
	KSPIN_LOCK spin_lock;
	KIRQL oldIrql;
	PFILENAME curr_file;

	KeClearEvent(&dx->stop_event);
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
	dx->invalid_movings = 0;
	for(curr_file = dx->filelist; curr_file != NULL; curr_file = curr_file->next_ptr){
		/* skip system files */
		if(!curr_file->blockmap) continue;
		/* skip files with size over limit */
		/*if(curr_file->is_overlimit)
		{
			curr_file = curr_file->next_ptr;
			continue;
		}*/
		if(KeReadStateEvent(&dx->stop_event) == 0x1)
			goto exit_defrag_space;
		DefragmentFile(dx,curr_file);
		dx->processed_clusters += curr_file->clusters_total;
	}

exit_defrag_space:
	UpdateFragmentedFilesList(dx);
	SaveFragmFilesListToDisk(dx);
	/* The state of some processed files maybe unknown... */
	if(!dx->invalid_movings && KeReadStateEvent(&dx->stop_event) == 0x0 && \
		dx->partition_type != NTFS_PARTITION) dx->status = STATUS_DEFRAGMENTED;
	else dx->status = STATUS_BEFORE_PROCESSING;
}
 
NTSTATUS MovePartOfFile(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile, 
		    ULONGLONG startVcn, ULONGLONG targetLcn, ULONGLONG n_clusters)
{
	ULONG status;
	IO_STATUS_BLOCK ioStatus;

	DebugPrint("-Ultradfg- sVcn: %I64u,tLcn: %I64u,n: %u\n",
		 startVcn,targetLcn,n_clusters);

	if(KeReadStateEvent(&dx->stop_event) == 0x1)
		return STATUS_UNSUCCESSFUL;

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
	/* Check target space to get moving status: */
	return CheckFreeSpace(dx,targetLcn,n_clusters) ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}

NTSTATUS MoveBlocksOfFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,
			  HANDLE hFile,ULONGLONG targetLcn)
{
	PBLOCKMAP block;
	ULONGLONG curr_target;
	ULONGLONG j,n,r;
	NTSTATUS Status = STATUS_SUCCESS;

	curr_target = targetLcn;
	for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
		/* try to move current block */
		n = block->length / dx->clusters_per_256k;
		for(j = 0; j < n; j++){
			Status = MovePartOfFile(dx,hFile,block->vcn + j * dx->clusters_per_256k, \
				curr_target,dx->clusters_per_256k);
			if(Status) goto exit;
			curr_target += dx->clusters_per_256k;
		}
		/* try to move rest of block */
		r = block->length % dx->clusters_per_256k;
		if(r){
			Status = MovePartOfFile(dx,hFile,block->vcn + j * dx->clusters_per_256k, \
				curr_target,r);
			if(Status) goto exit;
			curr_target += r;
		}
	}
exit:
	return Status;
}

ULONGLONG FindTarget(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	ULONGLONG t_before = LLINVALID, t_after = LLINVALID;
	ULONGLONG target;
	PFREEBLOCKMAP block;

	for(block = dx->free_space_map; block != NULL; block = block->next_ptr){
		if(block->length >= pfn->clusters_total){
			if(block->lcn < pfn->blockmap->lcn){
				if(dx->compact_flag && t_before != LLINVALID)
					continue;
				t_before = dx->compact_flag ? block->lcn : \
					(block->lcn + block->length - pfn->clusters_total);
			} else {
				t_after = block->lcn;
				break;
			}
		}
	}

	if(dx->compact_flag){
		if(pfn->is_fragm && t_before == LLINVALID) target = t_after;
		else target = t_before;
	} else {
		if(t_after == LLINVALID) return t_before;
		if(t_before == LLINVALID) return t_after;
		target = \
			((pfn->blockmap->lcn - t_before) < (t_after - pfn->blockmap->lcn)) ? \
			t_before : t_after;
	}
	return target;
}

/* Returns true when file was moved, otherwise false and file in undefined state. */
BOOLEAN DefragmentFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	HANDLE hFile;
	ULONGLONG target, curr_target;
	PBLOCKMAP block;
	UCHAR old_state;

	/* Open the file */
	InitializeObjectAttributes(&ObjectAttributes,&pfn->name,0,NULL,NULL);
	Status = ZwCreateFile(&hFile,FILE_GENERIC_READ,&ObjectAttributes,&IoStatusBlock,
			  NULL,0,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,
			  pfn->is_dir ? FILE_OPEN_FOR_BACKUP_INTENT : FILE_NO_INTERMEDIATE_BUFFERING,
			  NULL,0);
	if(Status){
		DebugPrint1("-Ultradfg- Can't open %ws: %x\n", pfn->name.Buffer,(UINT)Status);
		return FALSE;
	}
	/* Find free space */
	target = FindTarget(dx,pfn);
	if(target == LLINVALID){
		DebugPrint2("-Ultradfg- no enough continuous free space on volume\n");
		ZwClose(hFile);
		return FALSE;
	}

	DebugPrint("-Ultradfg- %ws\n",pfn->name.Buffer);
	DebugPrint("-Ultradfg- t: %I64u n: %I64u\n",target,pfn->clusters_total);

	old_state = GetSpaceState(pfn);
	Status = MoveBlocksOfFile(dx,pfn,hFile,target);
	ZwClose(hFile);

	if(Status == STATUS_SUCCESS){
		/* free previously allocated space */
		for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
			ProcessFreeBlock(dx,block->lcn,block->length,old_state);
		}
		/* correct file information */
		if(pfn->is_compressed){
			curr_target = target;
			for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
				block->lcn = curr_target;
				curr_target += block->length;
			}
		} else {
			pfn->blockmap->lcn = target;
			pfn->blockmap->length = pfn->clusters_total;
			DestroyList((PLIST *)&pfn->blockmap->next_ptr);
			dx->fragmcounter -= (pfn->n_fragments - 1);
			pfn->n_fragments = 1;
		}
		if(pfn->is_fragm){
			dx->fragmfilecounter --;
			pfn->is_fragm = FALSE;
		}
	} else {
		DebugPrint("MoveFile error: %x\n",(UINT)Status);
		/* mark space allocated by file as fragmented */
		for(block = pfn->blockmap; block != NULL; block = block->next_ptr)
			ProcessBlock(dx,block->lcn,block->length,FRAGM_SPACE,old_state);
		/* destroy blockmap */
		DeleteBlockmap(pfn);
		if(!pfn->is_fragm){
			dx->fragmfilecounter ++;
			pfn->is_fragm = TRUE;
		}
		dx->invalid_movings ++;
	}
	/* remove target space from free space pool */
	ProcessBlock(dx,target,pfn->clusters_total,GetSpaceState(pfn),FREE_SPACE);
	TruncateFreeSpaceBlock(dx,target,pfn->clusters_total);
	return (!pfn->is_fragm);
}
