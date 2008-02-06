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

/* Kernel of defragmenter */
void Defragment(UDEFRAG_DEVICE_EXTENSION *dx)
{
	KSPIN_LOCK spin_lock;
	KIRQL oldIrql;
	PFRAGMENTED pflist;
	PFILENAME curr_file;
	int x = 0; // temporary replacement for RedumpSpace.

	KeClearEvent(&dx->stop_event);
	DeleteLogFile(dx);

	/* Initialize progress counters. */
	KeInitializeSpinLock(&spin_lock);
	KeAcquireSpinLock(&spin_lock,&oldIrql);
	dx->clusters_to_move = dx->clusters_to_move_initial = 0;
	KeReleaseSpinLock(&spin_lock,oldIrql);
	for(pflist = dx->fragmfileslist; pflist != NULL; pflist = pflist->next_ptr){
		if(!pflist->pfn->blockmap) continue;
		if(pflist->pfn->is_overlimit) continue;
		if(pflist->pfn->is_filtered) continue;
		if(pflist->pfn->is_dir && dx->partition_type != NTFS_PARTITION) continue;
		dx->clusters_to_move_initial += pflist->pfn->clusters_total;
	}
	dx->clusters_to_move = dx->clusters_to_move_initial;
	dx->current_operation = 'D';

	/* process fragmented files */
	dx->invalid_movings = 0;
	CheckPendingBlocks(dx);
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
		dx->clusters_to_move -= curr_file->clusters_total;
	}
	/*
	* Here dx->clusters_to_move should be zero.
	* We have reached 100% progress point.
	*/

	if(dx->invalid_movings)
		x = 1; //RedumpSpace(dx);

exit_defrag:
	CheckPendingBlocks(dx);
	UpdateFragmentedFilesList(dx);
	SaveFragmFilesListToDisk(dx);
	/*
	* If defragmentation was stopped then we need
	* repeat analyse before the next defragmentation.
	*/
	if(KeReadStateEvent(&dx->stop_event) == 0x0)
		dx->status = STATUS_DEFRAGMENTED;
	else
		dx->status = STATUS_BEFORE_PROCESSING;
	if(x) dx->status = STATUS_BEFORE_PROCESSING;
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
	int x = 0; // temporary replacement for RedumpSpace.

	KeClearEvent(&dx->stop_event);
	DeleteLogFile(dx);

	/* Initialize progress counters. */
	KeInitializeSpinLock(&spin_lock);
	KeAcquireSpinLock(&spin_lock,&oldIrql);
	dx->clusters_to_move = dx->clusters_to_move_initial = 0;
	KeReleaseSpinLock(&spin_lock,oldIrql);
	for(curr_file = dx->filelist; curr_file != NULL; curr_file = curr_file->next_ptr){
		if(!curr_file->blockmap) continue;
		dx->clusters_to_move_initial += curr_file->clusters_total;
	}
	dx->clusters_to_move = dx->clusters_to_move_initial;
	dx->current_operation = 'C';

	/* On FAT volumes it increase distance between dir & files inside it. */
	if(dx->partition_type != NTFS_PARTITION){
		dx->clusters_to_move = 0;
		return;
	}

	/* process all files */
	dx->invalid_movings = 0;
	CheckPendingBlocks(dx);
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
		dx->clusters_to_move -= curr_file->clusters_total;
	}
	/*
	* Here dx->clusters_to_move should be zero.
	* We have reached 100% progress point.
	*/

	if(dx->invalid_movings)
		x = 1; //RedumpSpace(dx);

exit_defrag_space:
	CheckPendingBlocks(dx);
	UpdateFragmentedFilesList(dx);
	SaveFragmFilesListToDisk(dx);
	/*
	* If defragmentation was stopped then we need
	* repeat analyse before the next defragmentation.
	*/
	if(KeReadStateEvent(&dx->stop_event) == 0x0)
		dx->status = STATUS_DEFRAGMENTED;
	else
		dx->status = STATUS_BEFORE_PROCESSING;
	if(x) dx->status = STATUS_BEFORE_PROCESSING;
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

NTSTATUS MoveCompressedFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,
			    HANDLE hFile,ULONGLONG targetLcn)
{
	PBLOCKMAP block;
	ULONGLONG curr_target;
	NTSTATUS Status = STATUS_SUCCESS;

	curr_target = targetLcn;
	for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
		Status = MovePartOfFile(dx,hFile,block->vcn, \
			curr_target,block->length);
		if(Status) break;
		curr_target += block->length;
	}
	return Status;
}

ULONGLONG FindTarget(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	ULONGLONG t_before = LLINVALID, t_after = LLINVALID;
	ULONGLONG target;
	PFREEBLOCKMAP block = dx->free_space_map;

	while(block)
	{
		if(block->length >= pfn->clusters_total)
		{
			if(block->lcn < pfn->blockmap->lcn)
			{
				if(dx->compact_flag && t_before != LLINVALID)
					goto next;
				t_before = dx->compact_flag ? block->lcn : \
					(block->lcn + block->length - pfn->clusters_total);
			}
			else
			{
				t_after = block->lcn;
				break;
			}
		}
next:
		block = block->next_ptr;
	}
	if(dx->compact_flag)
	{
		target = t_before;
		if(pfn->is_fragm && t_before == LLINVALID)
			target = t_after;
	}
	else
	{
		if(t_after == LLINVALID) { target = t_before; goto exit; }
		if(t_before == LLINVALID) { target = t_after;  goto exit; }
		target = \
			((pfn->blockmap->lcn - t_before) < (t_after - pfn->blockmap->lcn)) ? \
			t_before : t_after;

	}
exit:
	return target;
}

/* Returns true when file was moved, otherwise false and file in undefined state. */
BOOLEAN DefragmentFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	HANDLE hFile;
	ULONGLONG target,curr_target;
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
	/* If file is compressed then we must move only non-virtual clusters. */
	/* If OS version is 4.x and file is greater then 256k ... */
	if(/*!dx->xp_compatible && */pfn->clusters_total > dx->clusters_per_256k)
		Status = MoveBlocksOfFile(dx,pfn,hFile,target);
	else if(pfn->is_compressed)
		Status = MoveCompressedFile(dx,pfn,hFile,target);
	else
		Status = MovePartOfFile(dx,hFile,0,target,pfn->clusters_total);
	ZwClose(hFile);
	/* if Status is successful then 
	 *		mark target space using MarkSpace()
	 *      remove target space from free space map
	 *		mark source space as free (on fat/udf) or pending (on ntfs)
	 *      add source space to free space map
	 *		rebuild file's blockmap
	 * else
	 *		mark target space as system (because state is unknown)
	 *      remove target space from free space map
	 *      mark source space as system (see above)
	 *		destroy file's blockmap and mark file as system
	 *      increase invalid movings counter
	 * endif
	 */
	if(Status == STATUS_SUCCESS){
		for(block = pfn->blockmap; block != NULL; block = block->next_ptr)
			ProcessFreeBlock(dx,block->lcn,block->length,old_state);
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
		MarkSpace(dx,pfn,FREE_SPACE);
		TruncateFreeSpaceBlock(dx,target,pfn->clusters_total);
	} else {
		DebugPrint("MoveFile error: %x\n",(UINT)Status);
		ProcessBlock(dx,target,pfn->clusters_total,
				SYSTEM_SPACE,FREE_SPACE);
		TruncateFreeSpaceBlock(dx,target,pfn->clusters_total);
		for(block = pfn->blockmap; block != NULL; block = block->next_ptr)
			ProcessBlock(dx,block->lcn,block->length,SYSTEM_SPACE,old_state);
		DeleteBlockmap(pfn);
		if(!pfn->is_fragm){
			dx->fragmfilecounter ++;
			pfn->is_fragm = TRUE;
		}
		dx->invalid_movings ++;
	}
	return (!pfn->is_fragm);
}
