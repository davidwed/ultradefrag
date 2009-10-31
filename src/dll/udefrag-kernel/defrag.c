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
* User mode driver - defragmenter's engine / Kernel of the defragmenter /.
*/

#include "globals.h"

BOOLEAN MoveTheFile(PFILENAME pfn,ULONGLONG target);

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
*    than the first attempt.
* 5. On very large FAT volumes the second analysis may take few minutes. Well, the 
*    first one may take few hours... :)
*/

/* returns -1 if no files were defragmented, zero otherwise */
int Defragment(char *volume_name)
{
	PFRAGMENTED pf, plargest;
	PFREEBLOCKMAP block;
	ULONGLONG length;
	ULONGLONG defragmented = 0;

	DebugPrint("----- Defragmentation of %s: -----\n",volume_name);

	/* Initialize progress counters. */
	Stat.clusters_to_process = Stat.processed_clusters = 0;

	for(pf = fragmfileslist; pf != NULL; pf = pf->next_ptr){
		if(!pf->pfn->blockmap) goto next_item; /* skip fragmented files with unknown state */
		if(!optimize_flag){
			if(pf->pfn->is_overlimit) goto next_item; /* skip fragmented but filtered out files */
			//if(pf->pfn->is_filtered) goto next_item; /* since v3.2.0 fragmfileslist never contains such entries */
			/* skip fragmented directories on FAT/UDF partitions */
			if(pf->pfn->is_dir && partition_type != NTFS_PARTITION) goto next_item;
		}
		Stat.clusters_to_process += pf->pfn->clusters_total;
	next_item:
		if(pf->next_ptr == fragmfileslist) break;
	}

	Stat.current_operation = 'D';
	
	/* Fill free space areas. */
	for(block = free_space_map; block != NULL; block = block->next_ptr){
	L0:
		if(block->length <= 1) goto L1; /* skip 1 cluster blocks and zero length blocks */
		/* find largest fragmented file that can be stored here */
		plargest = NULL; length = 0;
		for(pf = fragmfileslist; pf != NULL; pf = pf->next_ptr){
			if(!pf->pfn->blockmap) goto L2; /* skip fragmented files with unknown state */
			if(!optimize_flag){
				if(pf->pfn->is_overlimit) goto L2; /* skip fragmented but filtered out files */
				//if(pf->pfn->is_filtered) goto L2; /* since v3.2.0 fragmfileslist never contains such entries */
				/* skip fragmented directories on FAT/UDF partitions */
				if(pf->pfn->is_dir && partition_type != NTFS_PARTITION) goto L2;
			}
			if(pf->pfn->clusters_total <= block->length){
				if(pf->pfn->clusters_total > length){
					/* skip locked files here to prevent skipping the current free space block */
					/* an appropriate check was moved to Analyze() function */
					plargest = pf;
					length = pf->pfn->clusters_total;
				}
			}
		L2:
			if(pf->next_ptr == fragmfileslist) break;
		}
		if(!plargest) goto L1; /* current block is too small */
		/* move file */
		if(MoveTheFile(plargest->pfn,block->lcn)){
			DebugPrint("Defrag success for %ws\n",plargest->pfn->name.Buffer);
			defragmented++;
		} else {
			DebugPrint("Defrag error for %ws\n",plargest->pfn->name.Buffer);
		}
		Stat.processed_clusters += plargest->pfn->clusters_total;
		UpdateFragmentedFilesList();
		if(CheckForStopEvent()) break;
		/* after file moving continue from the first free space block */
		block = free_space_map; if(!block) break; else goto L0;
	L1:
		if(block->next_ptr == free_space_map) break;
	}
	return (defragmented == 0) ? (-1) : (0);
}

NTSTATUS MovePartOfFile(HANDLE hFile,ULONGLONG startVcn, ULONGLONG targetLcn, ULONGLONG n_clusters)
{
	ULONG status;
	IO_STATUS_BLOCK ioStatus;
	MOVEFILE_DESCRIPTOR moveFile;

	DebugPrint("sVcn: %I64u,tLcn: %I64u,n: %u\n",
		 startVcn,targetLcn,n_clusters);

	if(CheckForStopEvent()) return STATUS_UNSUCCESSFUL;

	/* Setup movefile descriptor and make the call */
	moveFile.FileHandle = hFile;
	moveFile.StartVcn.QuadPart = startVcn;
	moveFile.TargetLcn.QuadPart = targetLcn;
#ifdef _WIN64
	moveFile.NumVcns = n_clusters;
#else
	moveFile.NumVcns = (ULONG)n_clusters;
#endif
	/* On NT 4.0 it can move no more than 256 kbytes once. */
	status = NtFsControlFile(winx_fileno(fVolume),NULL,NULL,0,&ioStatus,
						FSCTL_MOVE_FILE,
						&moveFile,sizeof(MOVEFILE_DESCRIPTOR),
						NULL,0);

	/* If the operation is pending, wait for it to finish */
	if(status == STATUS_PENDING){
		/* FIXME: winx_fileno(fVolume) ??? */
		NtWaitForSingleObject(winx_fileno(fVolume)/*hFile*/,FALSE,NULL);
		status = ioStatus.Status;
	}
	if(!NT_SUCCESS(status)) return status;
	return STATUS_SUCCESS; /* it means: the result is unknown */
}

/* Tries to move the file entirely. */
NTSTATUS MoveBlocksOfFile(PFILENAME pfn,HANDLE hFile,ULONGLONG targetLcn)
{
	PBLOCKMAP block;
	ULONGLONG curr_target;
	ULONGLONG j,n,r;
	NTSTATUS Status;

	curr_target = targetLcn;
	for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
		/* try to move current block */
		n = block->length / clusters_per_256k;
		for(j = 0; j < n; j++){
			Status = MovePartOfFile(hFile,block->vcn + j * clusters_per_256k, \
				curr_target,clusters_per_256k);
			if(Status) return Status;
			curr_target += clusters_per_256k;
		}
		/* try to move rest of block */
		r = block->length % clusters_per_256k;
		if(r){
			Status = MovePartOfFile(hFile,block->vcn + j * clusters_per_256k, \
				curr_target,r);
			if(Status) return Status;
			curr_target += r;
		}
		if(block->next_ptr == pfn->blockmap) break;
	}
	return STATUS_SUCCESS;
}

void DbgPrintBlocksOfFile(PBLOCKMAP blockmap)
{
	PBLOCKMAP block;
	
	for(block = blockmap; block != NULL; block = block->next_ptr){
		DebugPrint("VCN: %I64u, LCN: %I64u, LENGTH: %u\n",
			block->vcn,block->lcn,block->length);
		if(block->next_ptr == blockmap) break;
	}
}

/* For defragmenter only, not for optimizer! */
BOOLEAN MoveTheFile(PFILENAME pfn,ULONGLONG target)
{
	NTSTATUS Status;
	HANDLE hFile;
	PBLOCKMAP block;
	FILENAME fn;

	/* Open the file */
	Status = OpenTheFile(pfn,&hFile);
	if(Status){
		DebugPrint("Can't open %ws file: %x\n",pfn->name.Buffer,(UINT)Status);
		/* we need to destroy the block map to avoid infinite loops */
		DeleteBlockmap(pfn); /* file is locked by other application, so its state is unknown */
		return FALSE;
	}

	DebugPrint("%ws\n",pfn->name.Buffer);
	DebugPrint("t: %I64u n: %I64u\n",target,pfn->clusters_total);

	Status = MoveBlocksOfFile(pfn,hFile,target);
	NtClose(hFile);
	
	if(Status == STATUS_SUCCESS){ /* we have undefined result */
		/* Check new file blocks to get moving status. */
		fn.name = pfn->name;
		if(DumpFile(&fn)){ /* otherwise let's assume the successful moving result? */
			if(fn.is_fragm){
				DebugPrint("MoveBlocksOfFile failed: "
						   "still fragmented.\n");
				DbgPrintBlocksOfFile(fn.blockmap);
				Status = STATUS_UNSUCCESSFUL;
			}
			if(fn.blockmap){
				if(fn.blockmap->lcn != target){
					DebugPrint("MoveBlocksOfFile failed: "
							   "first block not found on target space.\n");
					DbgPrintBlocksOfFile(fn.blockmap);
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			DeleteBlockmap(&fn);
		}
	}

	/* first of all: remove target space from free space pool */
	if(Status == STATUS_SUCCESS){
		Stat.fragmfilecounter --;
		Stat.fragmcounter -= (pfn->n_fragments - 1);
		pfn->is_fragm = FALSE; /* before GetSpaceState() call */
	}
	ProcessBlock(target,pfn->clusters_total,GetSpaceState(pfn),FREE_SPACE);
	TruncateFreeSpaceBlock(target,pfn->clusters_total);

	if(Status == STATUS_SUCCESS){
		/* free previously allocated space (after TruncateFreeSpaceBlock() call!) */
		for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
			ProcessFreeBlock(block->lcn,block->length,FRAGM_SPACE/*old_state*/);
			if(block->next_ptr == pfn->blockmap) break;
		}
	} else {
		DebugPrint("MoveFile error: %x\n",(UINT)Status);
	}
	DeleteBlockmap(pfn); /* because we don't need this info after file moving */
	return (!pfn->is_fragm);
}
