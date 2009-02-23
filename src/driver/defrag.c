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
* Defragmenter's engine.
*/

#include "driver.h"

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

/* Kernel of the defragmenter */

/**************************** OLD ALGORITHM ***************************************/
#if 0
void Defragment(UDEFRAG_DEVICE_EXTENSION *dx)
{
	KSPIN_LOCK spin_lock;
	KIRQL oldIrql;
	PFRAGMENTED pflist;
	PFILENAME curr_file;

	DebugPrint("-Ultradfg- ----- Defragmentation of %c: -----\n",NULL,dx->letter);
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
		if(KeReadStateEvent(&stop_event)) break;
		if(DefragmentFile(dx,curr_file))
			DebugPrint("-Ultradfg- Defrag success.\n",NULL);
		else
			DebugPrint("-Ultradfg- Defrag error for\n",curr_file->name.Buffer);
		dx->processed_clusters += curr_file->clusters_total;
	}
	UpdateFragmentedFilesList(dx);
	SaveFragmFilesListToDisk(dx);
}

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
 
NTSTATUS MovePartOfFile(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile, 
		    ULONGLONG startVcn, ULONGLONG targetLcn, ULONGLONG n_clusters)
{
	ULONG status;
	IO_STATUS_BLOCK ioStatus;

	DebugPrint("-Ultradfg- sVcn: %I64u,tLcn: %I64u,n: %u\n",NULL,
		 startVcn,targetLcn,n_clusters);

	if(KeReadStateEvent(&stop_event) == 0x1)
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
	}
	return STATUS_SUCCESS;
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
	NTSTATUS Status;
	HANDLE hFile;
	ULONGLONG target, curr_target;
	PBLOCKMAP block;
	UCHAR old_state;

	/* Open the file */
	Status = OpenTheFile(pfn,&hFile);
	if(Status){
		DebugPrint1("-Ultradfg- Can't open file: %x\n",pfn->name.Buffer,(UINT)Status);
		return FALSE;
	}
	/* Find free space */
	target = FindTarget(dx,pfn);
	if(target == LLINVALID){
		DebugPrint2("-Ultradfg- no enough continuous free space on volume\n",NULL);
		ZwClose(hFile);
		return FALSE;
	}

	DebugPrint("-Ultradfg-\n",pfn->name.Buffer);
	DebugPrint("-Ultradfg- t: %I64u n: %I64u\n",NULL,target,pfn->clusters_total);

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
		DebugPrint("MoveFile error: %x\n",NULL,(UINT)Status);
		/* mark space allocated by file as fragmented */
		for(block = pfn->blockmap; block != NULL; block = block->next_ptr)
			ProcessBlock(dx,block->lcn,block->length,FRAGM_SPACE,old_state);
		/* destroy blockmap */
		DeleteBlockmap(pfn);
		if(!pfn->is_fragm){
			dx->fragmfilecounter ++;
			pfn->is_fragm = TRUE;
		}
	}
	/* remove target space from free space pool */
	ProcessBlock(dx,target,pfn->clusters_total,GetSpaceState(pfn),FREE_SPACE);
	TruncateFreeSpaceBlock(dx,target,pfn->clusters_total);
	return (!pfn->is_fragm);
}
#endif
/**************************** NEW ALGORITHM (SINCE v2.2.0) ******************************/

BOOLEAN MoveTheFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,ULONGLONG target);

void Defragment(UDEFRAG_DEVICE_EXTENSION *dx)
{
	KSPIN_LOCK spin_lock;
	KIRQL oldIrql;
	PFRAGMENTED pflist, plargest;
	PFREEBLOCKMAP block;
	ULONGLONG length;

	DebugPrint("-Ultradfg- ----- Defragmentation of %c: -----\n",NULL,dx->letter);
	DeleteLogFile(dx);

	/* Initialize progress counters. */
	KeInitializeSpinLock(&spin_lock);
	KeAcquireSpinLock(&spin_lock,&oldIrql);
	dx->clusters_to_process = dx->processed_clusters = 0;
	KeReleaseSpinLock(&spin_lock,oldIrql);
	for(pflist = dx->fragmfileslist; pflist != NULL; pflist = pflist->next_ptr){
		if(!pflist->pfn->blockmap) continue; /* skip fragmented files with unknown state */
		if(pflist->pfn->is_overlimit) continue; /* skip fragmented but filtered out files */
		if(pflist->pfn->is_filtered) continue;
		/* skip fragmented directories on FAT/UDF partitions */
		if(pflist->pfn->is_dir && dx->partition_type != NTFS_PARTITION) continue;
		dx->clusters_to_process += pflist->pfn->clusters_total;
	}
	dx->current_operation = 'D';
	
	/* Fill free space areas. */
	for(block = dx->free_space_map; block != NULL; block = block->next_ptr){
	L0:
		if(block->length <= 1) continue; /* skip 1 cluster blocks and zero length blocks */
		/* find largest fragmented file that can be stored here */
		plargest = NULL; length = 0;
		for(pflist = dx->fragmfileslist; pflist != NULL; pflist = pflist->next_ptr){
			if(!pflist->pfn->blockmap) continue; /* skip fragmented files with unknown state */
			if(pflist->pfn->is_overlimit) continue; /* skip fragmented but filtered out files */
			if(pflist->pfn->is_filtered) continue;
			/* skip fragmented directories on FAT/UDF partitions */
			if(pflist->pfn->is_dir && dx->partition_type != NTFS_PARTITION) continue;
			if(pflist->pfn->clusters_total <= block->length){
				if(pflist->pfn->clusters_total > length){
					plargest = pflist;
					length = pflist->pfn->clusters_total;
				}
			}
		}
		if(!plargest) continue; /* current block is too small */
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
	/* Check target space to get moving status: */
	/*
	* This check is invalid, because it returns success when target space is allocated 
	* by another file.
	*/
	//return CheckFreeSpace(dx,targetLcn,n_clusters) ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
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
	}
	/* Check new file blocks to get moving status. */
	return CheckFilePosition(dx,hFile,targetLcn,pfn->clusters_total) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

/* For defragmenter only, not for optimizer! */
BOOLEAN MoveTheFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,ULONGLONG target)
{
	NTSTATUS Status;
	HANDLE hFile;
//	ULONGLONG curr_target;
	PBLOCKMAP block;
//	UCHAR old_state;

	/* Open the file */
	Status = OpenTheFile(pfn,&hFile);
	if(Status){
		DebugPrint1("-Ultradfg- Can't open file: %x\n",pfn->name.Buffer,(UINT)Status);
		/* we need to destroy the block map to avoid infinite loops */
		DeleteBlockmap(pfn); /* file is locked by other application, so its state is unknown */
		return FALSE;
	}

	DebugPrint("-Ultradfg-\n",pfn->name.Buffer);
	DebugPrint("-Ultradfg- t: %I64u n: %I64u\n",NULL,target,pfn->clusters_total);

//	old_state = FRAGM_SPACE; //GetSpaceState(pfn);
	Status = MoveBlocksOfFile(dx,pfn,hFile,target);
	ZwClose(hFile);

	/* first of all: remove target space from free space pool */
	if(Status == STATUS_SUCCESS){
		dx->fragmfilecounter --;
		pfn->is_fragm = FALSE; /* before GetSpaceState() call */
	}
	ProcessBlock(dx,target,pfn->clusters_total,GetSpaceState(pfn),FREE_SPACE);
	TruncateFreeSpaceBlock(dx,target,pfn->clusters_total);

	if(Status == STATUS_SUCCESS){
		/* free previously allocated space (after TruncateFreeSpaceBlock() call!) */
		for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
			ProcessFreeBlock(dx,block->lcn,block->length,FRAGM_SPACE/*old_state*/);
		}
		/* correct file information */
/*		if(pfn->is_compressed){
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
*/		//if(pfn->is_fragm){
		//}
	} else {
		DebugPrint("-Ultradfg- MoveFile error: %x\n",NULL,(UINT)Status);
		/* mark space allocated by file as fragmented */
//		for(block = pfn->blockmap; block != NULL; block = block->next_ptr)
//			ProcessBlock(dx,block->lcn,block->length,FRAGM_SPACE,old_state);
		/* destroy blockmap */
//		DeleteBlockmap(pfn);
//		if(!pfn->is_fragm){
//			dx->fragmfilecounter ++;
//			pfn->is_fragm = TRUE;
//		}
	}
	DeleteBlockmap(pfn); /* because we don't need this info after file moving */
	return (!pfn->is_fragm);
}
