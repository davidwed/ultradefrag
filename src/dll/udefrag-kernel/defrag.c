/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

/**
 * @file defrag.c
 * @brief Volume defragmentation code.
 * @addtogroup Defrag
 * @{
 */

#include "globals.h"

/*
* How to move directories on FAT.
* Suggested by Vasily Smirnov http://www.bacek.ru/
*
* 1. Detect FAT formatted volumes before attempting to
*    defragment them.
* 2. Lock the volume to prevent modification of the
*    currently moving directory.
* 3. Handle a special case of directories on FAT in
*    MovePartOfFile() procedure:
* A. Create a temporary file on the volume with size
*    equal to directory size.
* B. Move the newly created file to desired clusters.
* C. Copy directory contents to the file.
* D. Copy directory information to the FAT entry of the file.
* E. Mark old FAT entry as free.
* 4. Unlock the volume.
*
* 5. The best practice would be to get free space in the
*    beginning of the volume to hold all directories together.
*    Then we'll move all directories and then - all files thereafter.
* 6. DOS boot files must be skipped to prevent breaking the system.
*    These files are: IO.SYS, MSDOS.SYS, IBMBIO.COM, IBMDOS.COM
* 7. Also many old programs rely on positions of their files on disk
*    (especially security software, boot loaders and similar stuff),
*    so such files should be skipped. Anyway, customers must be forewarned
*    before the FAT-formatted volume optimization.
*/

extern ULONGLONG StartingPoint; /* it's a part of the volume optimizer */

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
*    that its arguments must be in the user memory space.
* 4. Filesystem data is cached by Windows, therefore the second analysis will be
*    always much faster (at least on modern versions of Windows - tested on XP)
*    than the first attempt.
* 5. On a very large FAT volumes the second analysis may take few minutes. Well, the 
*    first one may take few hours... :)
*/

/**
 * @brief Performs a volume defragmentation.
 * @param[in] volume_name the name of the volume.
 * @return Zero if at least one file has been
 * defragmented, negative value otherwise.
 */
int Defragment(char *volume_name)
{
	PFRAGMENTED pf, plargest;
	PFREEBLOCKMAP block;
	ULONGLONG length;
	ULONGLONG locked_clusters;
	ULONGLONG defragmented = 0;

	DebugPrint("----- Defragmentation of %s: -----\n",volume_name);
	
	/* Initialize progress counters. */
	Stat.clusters_to_process = Stat.processed_clusters = 0;
	Stat.current_operation = 'D';
	
	/* skip all locked files */
	//CheckAllFragmentedFiles();

	for(pf = fragmfileslist; pf != NULL; pf = pf->next_ptr){
		if(!pf->pfn->blockmap) goto next_item; /* skip fragmented files with unknown state */
		if(!optimize_flag){
			if(pf->pfn->is_overlimit) goto next_item; /* skip fragmented but filtered out files */
			//if(pf->pfn->is_filtered) goto next_item; /* since v3.2.0 fragmfileslist never contains such entries */
		}
		/* skip fragmented directories on FAT/UDF partitions */
		if(pf->pfn->is_dir && !AllowDirDefrag) goto next_item;
		Stat.clusters_to_process += pf->pfn->clusters_total;
	next_item:
		if(pf->next_ptr == fragmfileslist) break;
	}

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
				if(pf->pfn->is_filtered) goto L2; /* in v3.2.0 fragmfileslist never contained such entries */
			}
			/* skip fragmented directories on FAT/UDF partitions */
			if(pf->pfn->is_dir && !AllowDirDefrag) goto L2;
			if(pf->pfn->clusters_total <= block->length){
				if(pf->pfn->clusters_total > length){
					/* skip locked files here to prevent skipping the current free space block */
					plargest = pf;
					length = pf->pfn->clusters_total;
				}
			}
		L2:
			if(pf->next_ptr == fragmfileslist) break;
		}
		if(!plargest) goto L1; /* current block is too small */
		locked_clusters = 0;
		if(plargest->pfn->blockmap) locked_clusters = plargest->pfn->clusters_total;
		if(IsFileLocked(plargest->pfn)){
			Stat.processed_clusters += locked_clusters;
			goto L0;
		}
		/* move file */
		if(MoveTheFile(plargest->pfn,block->lcn)){
			DebugPrint("Defrag success for %ws\n",plargest->pfn->name.Buffer);
			defragmented++;
		} else {
			DebugPrint("Defrag error for %ws\n",plargest->pfn->name.Buffer);
		}
		/*Stat.processed_clusters += plargest->pfn->clusters_total;*/
		UpdateFragmentedFilesList();
		if(CheckForStopEvent()) break;
		/* after file moving continue from the first free space block */
		block = free_space_map; if(!block) break; else goto L0;
	L1:
		if(block->next_ptr == free_space_map) break;
	}
	return (defragmented == 0) ? (-1) : (0);
}

/* ------------------------------------------------------------------------- */
/*                   The following function moves clusters.                  */
/* ------------------------------------------------------------------------- */

/**
 * @brief Moves a range of clusters belonging to the file.
 * @param[in] hFile handle of the file.
 * @param[in] startVcn the starting virtual cluster number
 *                     defining position inside the file.
 * @param[in] targetLcn the starting logical cluster number
 *                      defining position of target space
 *                      on the volume.
 * @param[in] n_clusters the number of clusters to move.
 * @return An appropriate NTSTATUS code.
 * @note On NT 4.0 this function can move
 * no more than 256 kilobytes once.
 */
NTSTATUS MovePartOfFile(HANDLE hFile,ULONGLONG startVcn, ULONGLONG targetLcn, ULONGLONG n_clusters)
{
	NTSTATUS status;
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
	if(NT_SUCCESS(status)/* == STATUS_PENDING*/){
		/* FIXME: winx_fileno(fVolume) ??? */
		NtWaitForSingleObject(winx_fileno(fVolume)/*hFile*/,FALSE,NULL);
		status = ioStatus.Status;
	}
	if(!NT_SUCCESS(status)) return status;
	return STATUS_SUCCESS; /* it means: the result is unknown */
}

/* ------------------------------------------------------------------------- */
/*        The following 2 functions are drivers for MovePartOfFile().        */
/*        They must update Stat.processed_clusters correctly.                */
/* ------------------------------------------------------------------------- */

/**
 * @brief Moves a file entirely.
 * @param[in] pfn pointer to the structure
 *                describing the file.
 * @param[in] hFile handle of the file.
 * @param[in] targetLcn the starting logical cluster number
 * defining position of target space on the volume.
 * @return An appropriate NTSTATUS code.
 */
NTSTATUS MoveBlocksOfFile(PFILENAME pfn,HANDLE hFile,ULONGLONG targetLcn)
{
	PBLOCKMAP block;
	ULONGLONG curr_target;
	ULONGLONG j,n,r;
	NTSTATUS Status;
	ULONGLONG clusters_to_process;

	clusters_to_process = pfn->clusters_total;
	curr_target = targetLcn;
	for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
		/* try to move current block */
		n = block->length / clusters_per_256k;
		for(j = 0; j < n; j++){
			Status = MovePartOfFile(hFile,block->vcn + j * clusters_per_256k, \
				curr_target,clusters_per_256k);
			if(Status != STATUS_SUCCESS){ /* failure */
				Stat.processed_clusters += clusters_to_process;
				return Status;
			}
			Stat.processed_clusters += clusters_per_256k;
			clusters_to_process -= clusters_per_256k;
			curr_target += clusters_per_256k;
		}
		/* try to move rest of block */
		r = block->length % clusters_per_256k;
		if(r){
			Status = MovePartOfFile(hFile,block->vcn + j * clusters_per_256k, \
				curr_target,r);
			if(Status != STATUS_SUCCESS){ /* failure */
				Stat.processed_clusters += clusters_to_process;
				return Status;
			}
			Stat.processed_clusters += r;
			clusters_to_process -= r;
			curr_target += r;
		}
		if(block->next_ptr == pfn->blockmap) break;
	}
	return STATUS_SUCCESS;
}

/**
 * @brief Moves a part of file.
 * @param[in] pfn pointer to the structure describing the file.
 * @param[in] startVcn the starting virtual cluster number
 *                     defining position inside the file.
 * @param[in] targetLcn the starting logical cluster number
 *                      defining position of target space
 *                      on the volume.
 * @param[in] n_clusters the number of clusters to move.
 * @note On NT 4.0 this function has no size limit
 * for the part of file to be moved, unlike the MovePartOfFile().
 */
void MovePartOfFileBlock(PFILENAME pfn,ULONGLONG startVcn,
		ULONGLONG targetLcn,ULONGLONG n_clusters)
{
	HANDLE hFile;
	ULONGLONG target;
	ULONGLONG j,n,r;
	NTSTATUS Status;
	ULONGLONG clusters_to_process;

	clusters_to_process = n_clusters;
	Status = OpenTheFile(pfn,&hFile);
	if(Status != STATUS_SUCCESS){
		Stat.processed_clusters += clusters_to_process;
		return;
	}

	target = targetLcn;
	n = n_clusters / clusters_per_256k;
	for(j = 0; j < n; j++){
		Status = MovePartOfFile(hFile,startVcn + j * clusters_per_256k, \
			target,clusters_per_256k);
		if(Status != STATUS_SUCCESS){ /* failure */
			Stat.processed_clusters += clusters_to_process;
			NtClose(hFile);
			return;
		}
		Stat.processed_clusters += clusters_per_256k;
		clusters_to_process -= clusters_per_256k;
		target += clusters_per_256k;
	}
	r = n_clusters % clusters_per_256k;
	if(r){
		Status = MovePartOfFile(hFile,startVcn + j * clusters_per_256k, \
			target,r);
		if(Status != STATUS_SUCCESS){ /* failure */
			Stat.processed_clusters += clusters_to_process;
			NtClose(hFile);
			return;
		}
		Stat.processed_clusters += r;
		clusters_to_process -= r;
	}

	NtClose(hFile);
}

/**
 * @brief Prints sequently information about all blocks of the file.
 * @param[in] blockmap pointer to the list representing blocks of file.
 */
void DbgPrintBlocksOfFile(PBLOCKMAP blockmap)
{
	PBLOCKMAP block;
	
	for(block = blockmap; block != NULL; block = block->next_ptr){
		DebugPrint("VCN: %I64u, LCN: %I64u, LENGTH: %u\n",
			block->vcn,block->lcn,block->length);
		if(block->next_ptr == blockmap) break;
	}
}

/**
 * @brief Moves a file and updates the global statistics and map.
 * @param[in] pfn pointer to the structure describing the file.
 * @param[in] target the starting logical cluster number
 * defining position of target space on the volume.
 * @return Boolean value. TRUE indicates success,
 * FALSE indicates failure.
 * @note For defragmenter only, not for optimizer!
 */
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

	/* first of all: remove target space from the free space list */
	if(Status == STATUS_SUCCESS){
		Stat.fragmfilecounter --;
		Stat.fragmcounter -= (pfn->n_fragments - 1);
		pfn->is_fragm = FALSE; /* before GetSpaceState() call */
	}
	RemarkBlock(target,pfn->clusters_total,GetFileSpaceState(pfn),FREE_SPACE);
	TruncateFreeSpaceBlock(target,pfn->clusters_total);

	if(Status == STATUS_SUCCESS){
		/* free previously allocated space (after TruncateFreeSpaceBlock() call!) */
		for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
			ProcessFreedBlock(block->lcn,block->length,FRAGM_SPACE/*old_state*/);
			if(block->next_ptr == pfn->blockmap) break;
		}
	} else {
		DebugPrint("MoveFile error: %x\n",(UINT)Status);
	}
	DeleteBlockmap(pfn); /* because we don't need this info after file moving */
	
	/* move starting point to avoid unwanted additional optimization passes */
	if(target + pfn->clusters_total - 1 > StartingPoint)
		StartingPoint = target + pfn->clusters_total - 1;
	return (!pfn->is_fragm);
}

/** @} */
