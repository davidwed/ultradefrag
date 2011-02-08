/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file move.c
 * @brief File moving.
 * @addtogroup Move
 * @{
 */

#include "udefrag-internals.h"

/************************************************************/
/*                    Internal Routines                     */
/************************************************************/

/* TODO: remove it after testing of $Mft defrag on Vista/Win7 */
WINX_FILE * __stdcall new_winx_vopen(char volume_letter)
{
	char path[] = "\\??\\A:";
	ANSI_STRING as;
	UNICODE_STRING us;
	NTSTATUS status;
	HANDLE hFile;
	OBJECT_ATTRIBUTES oa;
	IO_STATUS_BLOCK iosb;
	ACCESS_MASK access_mask = FILE_GENERIC_READ /* | FILE_GENERIC_WRITE */;
	ULONG disposition = FILE_OPEN;
	ULONG flags = FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING | FILE_NON_DIRECTORY_FILE;
	WINX_FILE *f;

	path[4] = winx_toupper(volume_letter);
	RtlInitAnsiString(&as,path);
	if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) != STATUS_SUCCESS){
		DebugPrint("new_winx_vopen: cannot open %s: not enough memory",path);
		return NULL;
	}
	InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);

    if(winx_get_os_version() >= WINDOWS_7)
        flags |= FILE_DISALLOW_EXCLUSIVE;

	status = NtCreateFile(&hFile,
			access_mask,
			&oa,
			&iosb,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			disposition,
			flags,
			NULL,
			0
			);
	RtlFreeUnicodeString(&us);
	if(status != STATUS_SUCCESS){
		DebugPrintEx(status,"new_winx_vopen: cannot open %s",path);
		return NULL;
	}
	f = (WINX_FILE *)winx_heap_alloc(sizeof(WINX_FILE));
	if(!f){
		NtClose(hFile);
		DebugPrint("new_winx_vopen: cannot open %s: not enough memory",path);
		return NULL;
	}
	f->hFile = hFile;
	f->roffset.QuadPart = 0;
	f->woffset.QuadPart = 0;
	f->io_buffer = NULL;
	f->io_buffer_size = 0;
	f->io_buffer_offset = 0;
	f->wboffset.QuadPart = 0;
	return f;
}

/**
 * @brief Redraws freed space.
 */
static void redraw_freed_space(udefrag_job_parameters *jp,
		ULONGLONG lcn, ULONGLONG length, int old_color)
{
	/*
	* On FAT partitions after file moving filesystem driver marks
	* previously allocated clusters as free immediately.
	* But on NTFS they are always still allocated by system.
	* Only the next volume analysis frees them.
	*/
	if(jp->fs_type == FS_NTFS){
		/* mark clusters as temporarily allocated by system  */
		/* or as mft zone immediately since we're not using it */
		colorize_map_region(jp,lcn,length,TMP_SYSTEM_OR_MFT_ZONE_SPACE,old_color);
	} else {
		colorize_map_region(jp,lcn,length,FREE_SPACE,old_color);
		jp->free_regions = winx_add_volume_region(jp->free_regions,lcn,length);
	}
}

/**
 * @brief Moves file clusters.
 * @return Zero for success,
 * negative value otherwise.
 * @note 
 * - On NT4 this function can move
 * no more than 256 kilobytes once.
 * - Volume must be opened before this call,
 * jp->fVolume must contain a proper handle.
 */
static int move_file_clusters(HANDLE hFile,ULONGLONG startVcn,
	ULONGLONG targetLcn,ULONGLONG n_clusters,udefrag_job_parameters *jp)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK iosb;
	MOVEFILE_DESCRIPTOR mfd;

	DebugPrint("sVcn: %I64u,tLcn: %I64u,n: %u",
		 startVcn,targetLcn,n_clusters);

	if(jp->termination_router((void *)jp))
		return (-1);
	
	if(jp->udo.dry_run){
		jp->pi.moved_clusters += n_clusters;
		return 0;
	}

	/* setup movefile descriptor and make the call */
	mfd.FileHandle = hFile;
	mfd.StartVcn.QuadPart = startVcn;
	mfd.TargetLcn.QuadPart = targetLcn;
#ifdef _WIN64
	mfd.NumVcns = n_clusters;
#else
	mfd.NumVcns = (ULONG)n_clusters;
#endif
	Status = NtFsControlFile(winx_fileno(jp->fVolume),NULL,NULL,0,&iosb,
						FSCTL_MOVE_FILE,&mfd,sizeof(MOVEFILE_DESCRIPTOR),
						NULL,0);
	if(NT_SUCCESS(Status)){
		NtWaitForSingleObject(winx_fileno(jp->fVolume),FALSE,NULL);
		Status = iosb.Status;
	}
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"Cannot move file clusters");
		return (-1);
	}

	/*
	* Actually moving result is unknown here,
	* because API may return success, while
	* actually clusters may be moved partially.
	*/
	jp->pi.moved_clusters += n_clusters;
	return 0;
}

/**
 * @brief move_file helper.
 */
static void move_file_helper(HANDLE hFile,winx_file_info *f,
	ULONGLONG target,udefrag_job_parameters *jp)
{
	winx_blockmap *block;
	ULONGLONG curr_target, j, n, r;
	ULONGLONG clusters_to_process;
	int result;
	
	DebugPrint("%ws",f->path);
	DebugPrint("t: %I64u n: %I64u",target,f->disp.clusters);

	clusters_to_process = f->disp.clusters;
	curr_target = target;
	for(block = f->disp.blockmap; block; block = block->next){
		/* try to move current block */
		n = block->length / jp->clusters_per_256k;
		for(j = 0; j < n; j++){
			result = move_file_clusters(hFile,
				block->vcn + j * jp->clusters_per_256k,
				curr_target,jp->clusters_per_256k,jp);
			if(result < 0){
				jp->pi.processed_clusters += clusters_to_process;
				return;
			}
			jp->pi.processed_clusters += jp->clusters_per_256k;
			clusters_to_process -= jp->clusters_per_256k;
			curr_target += jp->clusters_per_256k;
		}
		/* try to move rest of block */
		r = block->length % jp->clusters_per_256k;
		if(r){
			result = move_file_clusters(hFile,
				block->vcn + j * jp->clusters_per_256k,
				curr_target,r,jp);
			if(result < 0){
				jp->pi.processed_clusters += clusters_to_process;
				return;
			}
			jp->pi.processed_clusters += r;
			clusters_to_process -= r;
			curr_target += r;
		}
		if(block->next == f->disp.blockmap) break;
	}
}

/**
 * @brief Prints list of file blocks.
 */
static void DbgPrintBlocksOfFile(winx_blockmap *blockmap)
{
	winx_blockmap *block;
	
	for(block = blockmap; block; block = block->next){
		DebugPrint("VCN: %I64u, LCN: %I64u, LENGTH: %u",
			block->vcn,block->lcn,block->length);
		if(block->next == blockmap) break;
	}
}

/**
 * @brief Compares two block maps.
 * @return Positive value indicates 
 * difference, zero indicates equality.
 * Negative value indicates that 
 * arguments are invalid.
 */
static int blockmap_compare(winx_file_disposition *disp1, winx_file_disposition *disp2)
{
	winx_blockmap *block1, *block2;

	/* validate arguments */
	if(disp1 == NULL || disp2 == NULL)
		return (-1);
	
	if(disp1->blockmap == disp2->blockmap)
		return 0;
	
	/* empty bitmap is not equal to non-empty one */
	if(disp1->blockmap == NULL || disp2->blockmap == NULL)
		return 1;

	/* ensure that both maps have non-negative number of elements */
	if(disp1->fragments <= 0 || disp2->fragments <= 0)
		return (-1);

	/* if number of elements differs, maps are different */
	if(disp1->fragments != disp2->fragments)
		return 1;

	/* comapare maps */	
	for(block1 = disp1->blockmap, block2 = disp2->blockmap;
	  block1 && block2; block1 = block1->next, block2 = block2->next){
		if((block1->vcn != block2->vcn) 
		  || (block1->lcn != block2->lcn)
		  || (block1->length != block2->length))
			return 1;
		if(block1->next == disp1->blockmap || block2->next == disp2->blockmap) break;
	}
	
	return 0;
}

static int __stdcall dump_terminator(void *user_defined_data)
{
	udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;

	return jp->termination_router((void *)jp);
}

/************************************************************/
/*                    move_file routine                     */
/************************************************************/

/**
 * @brief File moving results
 * intended for use in move_file only.
 */
typedef enum {
	UNDETERMINED_MOVING_SUCCESS,        /* file has been moved successfully, but its new block map isn't available */
	DETERMINED_MOVING_FAILURE,          /* nothing has been moved; new block map is available */
	DETERMINED_MOVING_PARTIAL_SUCCESS,  /* file has been moved partially; new block map is available */
	DETERMINED_MOVING_SUCCESS           /* file has been moved entirely; new block map is available */
} ud_file_moving_result;

/**
 * @brief Moves the file entirely.
 * @details Can move any file
 * regardless of its fragmentation.
 * @param[in] f the file to be moved.
 * @param[in] target the LCN 
 * of the target free region.
 * @param[in] jp job parameters.
 * @return Zero for success, negative value otherwise.
 * @note 
 * - Volume must be opened before this call,
 * jp->fVolume must contain a proper handle.
 */
int move_file(winx_file_info *f,ULONGLONG target,udefrag_job_parameters *jp)
{
	NTSTATUS Status;
	HANDLE hFile;
	int old_color, new_color;
	int was_fragmented;
	winx_blockmap *block, *next_block;
	ULONGLONG lcn;
	winx_file_info new_file_info;
	ud_file_moving_result moving_result;
	
	/* save file properties */
	old_color = get_file_color(jp,f);
	was_fragmented = is_fragmented(f);

	/* open the file */
	Status = udefrag_fopen(f,&hFile);
	if(Status != STATUS_SUCCESS){
		DebugPrintEx(Status,"move_file: cannot open %ws",f->path);
		/* redraw space */
		if(old_color != MFT_SPACE)
			colorize_file_as_system(jp,f);
		/* file is locked by other application, so its state is unknown */
		/* don't reset its statistics though! */
		/*f->disp.clusters = 0;
		f->disp.fragments = 0;
		f->disp.flags = 0;*/
		winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
		f->user_defined_flags |= UD_FILE_LOCKED;
		jp->pi.processed_clusters += f->disp.clusters;
		if(jp->progress_router)
			jp->progress_router(jp); /* redraw progress */
		return (-1);
	}
	
	/* move the file */
	move_file_helper(hFile,f,target,jp);
	NtCloseSafe(hFile);

	/* get file moving result */
	if(jp->udo.dry_run){
		moving_result = UNDETERMINED_MOVING_SUCCESS;
	} else {
		/* redump the file to define precisely its new state */
		memcpy(&new_file_info,f,sizeof(winx_file_info));
		new_file_info.disp.blockmap = NULL;

		if(winx_ftw_dump_file(&new_file_info,dump_terminator,(void *)jp) < 0){
			DebugPrint("move_file: cannot redump the file");
			/* let's assume a successful move */
			moving_result = UNDETERMINED_MOVING_SUCCESS;
		} else {
			/* compare old and new block maps */
			if(blockmap_compare(&new_file_info.disp,&f->disp) == 0){
				DebugPrint("move_file: nothing has been moved");
				moving_result = DETERMINED_MOVING_FAILURE;
			} else if(is_fragmented(&new_file_info)){
				DebugPrint("move_file: the file has been moved just partially");
				DbgPrintBlocksOfFile(new_file_info.disp.blockmap);
				moving_result = DETERMINED_MOVING_PARTIAL_SUCCESS;
			} else {
				moving_result = DETERMINED_MOVING_SUCCESS;
			}
		}
	}

	/* handle a case when nothing has been moved */
	if(moving_result == DETERMINED_MOVING_FAILURE){
		winx_list_destroy((list_entry **)(void *)&new_file_info.disp.blockmap);
		f->user_defined_flags |= UD_FILE_MOVING_FAILED;
		return (-1);
	}

	/*
	* Something has been moved, therefore we need to redraw 
	* space, update free space pool and adjust statistics.
	*/

	/* refresh coordinates of mft zones if $mft or $mftmirr has been moved */
	if(old_color == MFT_SPACE || is_mft_mirror(f))
		update_mft_zones_layout(jp);
	
	/* adjust statistics and basic file properties */
	if(moving_result != DETERMINED_MOVING_PARTIAL_SUCCESS){
		/* file has been moved entirely */
		if(was_fragmented)
			jp->pi.fragmented --;
		f->disp.flags &= ~WINX_FILE_DISP_FRAGMENTED;
	} else {
		/* file has been moved partially */
		if(!was_fragmented)
			jp->pi.fragmented ++;
		f->disp.flags |= WINX_FILE_DISP_FRAGMENTED;
		f->user_defined_flags |= UD_FILE_MOVING_FAILED;
	}
	
	/* redraw target space */
	new_color = get_file_color(jp,f);
	colorize_map_region(jp,target,f->disp.clusters,new_color,FREE_SPACE);
	if(jp->progress_router)
		jp->progress_router(jp); /* redraw map */
			
	/* remove target space from free space pool */
	jp->free_regions = winx_sub_volume_region(jp->free_regions,target,f->disp.clusters);
		
	/* redraw source space; after winx_sub_volume_region()! */
	for(block = f->disp.blockmap; block; block = block->next){
		if(moving_result != DETERMINED_MOVING_PARTIAL_SUCCESS)
			redraw_freed_space(jp,block->lcn,block->length,old_color);
		else
			colorize_map_region(jp,block->lcn,block->length,FRAGM_SPACE,old_color);
		if(block->next == f->disp.blockmap) break;
	}
	if(jp->progress_router)
		jp->progress_router(jp); /* redraw map */
	
	/* adjust block map and statistics */
	if(moving_result == UNDETERMINED_MOVING_SUCCESS){
		if(!is_compressed(f) && !is_sparse(f)){
			f->disp.blockmap->lcn = target;
			f->disp.blockmap->length = f->disp.clusters;
			for(block = f->disp.blockmap->next; block != f->disp.blockmap; ){
				next_block = block->next;
				winx_list_remove_item((list_entry **)(void *)&f->disp.blockmap,(list_entry *)(void *)block);
				block = next_block;
			}
			jp->pi.fragments -= (f->disp.fragments - 1);
			f->disp.fragments = 1;
		} else {
			/* compressed and sparse files must have vcn's properly set */
			lcn = target;
			for(block = f->disp.blockmap; block; block = block->next){
				block->lcn = lcn;
				lcn += block->length;
				if(block->next == f->disp.blockmap) break;
			}
		}
	} else {
		/* new block map is available - use it */
		jp->pi.fragments -= (f->disp.fragments - new_file_info.disp.fragments);
		winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
		memcpy(&f->disp,&new_file_info.disp,sizeof(winx_file_disposition));
	}
	
	return (moving_result == DETERMINED_MOVING_PARTIAL_SUCCESS) ? (-1) : 0;
}

/* TODO: */

static int move_file_blocks_helper(HANDLE hFile,winx_file_info *f,
	winx_blockmap *first_block,ULONGLONG n_blocks,ULONGLONG target,
	udefrag_job_parameters *jp);


/**
 * @brief Moves blocks of the file.
 * @param[in] f the file to be moved.
 * @param[in] first_block the first block.
 * @param[in] n_blocks number of blocks to move.
 * @param[in] target the LCN of the target free region.
 * @param[in] jp job parameters.
 * @return Zero for success, negative value otherwise.
 * @note 
 * - Volume must be opened before this call,
 * jp->fVolume must contain a proper handle.
 */
int move_file_blocks(winx_file_info *f,winx_blockmap *first_block,
	ULONGLONG n_blocks,ULONGLONG target,udefrag_job_parameters *jp)
{
	winx_blockmap *block, *next_block;
	ULONGLONG n, length;
	NTSTATUS Status;
	HANDLE hFile;
	int result;
	int old_color, new_color;
	
	length = 0;
	for(block = first_block, n = 0; block; block = block->next, n++){
		length += block->length;
		if(block->next == f->disp.blockmap || n == (n_blocks - 1)) break;
	}
	
	old_color = get_file_color(jp,f);

	/* open the file */
	Status = udefrag_fopen(f,&hFile);
	if(Status != STATUS_SUCCESS){
		DebugPrintEx(Status,"Cannot open %ws",f->path);
		/* redraw space */
		if(old_color != MFT_SPACE)
			colorize_file_as_system(jp,f);
		/* file is locked by other application, so its state is unknown */
		/* don't reset its statistics though! */
		/*f->disp.clusters = 0;
		f->disp.fragments = 0;
		f->disp.flags = 0;*/
		winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
		f->user_defined_flags |= UD_FILE_LOCKED;
		jp->pi.processed_clusters += length;
		if(jp->progress_router)
			jp->progress_router(jp); /* redraw progress */
		return (-1);
	}
	
	/* move the file */
	result = move_file_blocks_helper(hFile,f,first_block,n_blocks,target,jp);
	/* don't set UD_FILE_MOVING_FAILED here */
	NtCloseSafe(hFile);
	
	/* update statistics */
	if(result >= 0){
		jp->pi.fragments -= n_blocks;
	}
	
	/* refresh coordinates of mft zones if $mft or $mftmirr has been moved */
	if(old_color == MFT_SPACE || is_mft_mirror(f))
		update_mft_zones_layout(jp);
	
	/* redraw target space */
	new_color = get_file_color(jp,f);
	colorize_map_region(jp,target,length,new_color,FREE_SPACE);
	if(jp->progress_router)
		jp->progress_router(jp); /* redraw map */
			
	/* remove target space from free space pool */
	jp->free_regions = winx_sub_volume_region(jp->free_regions,target,length);
	
	/* redraw source space; after winx_sub_volume_region()! */
	if(result >= 0){
		for(block = first_block, n = 0; block; block = block->next, n++){
			if(old_color == MFT_SPACE)
				redraw_freed_space(jp,block->lcn,block->length,MFT_SPACE);
			else
				redraw_freed_space(jp,block->lcn,block->length,FRAGM_SPACE);
			if(block->next == f->disp.blockmap || n == (n_blocks - 1)) break;
		}
		if(jp->progress_router)
			jp->progress_router(jp); /* redraw map */
	}
	
	/* adjust blockmap */
	first_block->lcn = target;
	first_block->length = length;
	for(block = first_block->next, n = 1; block; n++){
		next_block = block->next;
		winx_list_remove_item((list_entry **)(void *)&f->disp.blockmap,(list_entry *)(void *)block);
		if(next_block == f->disp.blockmap || n == (n_blocks - 1)) break;
		block = next_block;
	}
	f->disp.fragments -= (n_blocks - 1);
	return result;
}

/**
 * @brief move_file_blocks helper.
 */
static int move_file_blocks_helper(HANDLE hFile,winx_file_info *f,
	winx_blockmap *first_block,ULONGLONG n_blocks,ULONGLONG target,
	udefrag_job_parameters *jp)
{
	winx_blockmap *block;
	ULONGLONG curr_target, i, j, n, r;
	ULONGLONG clusters_to_process;
	winx_file_info f_cp;
	int result;
	int block_found;
	
	DebugPrint("%ws",f->path);
	DebugPrint("t: %I64u vcn: %I64u n_blocks: %I64u",target,first_block->vcn,n_blocks);

	clusters_to_process = 0;
	for(block = first_block, i = 0; block; block = block->next, i++){
		clusters_to_process += block->length;
		if(block->next == f->disp.blockmap || i == (n_blocks - 1)) break;
	}

	curr_target = target;
	for(block = first_block, i = 0; block; block = block->next, i++){
		/* try to move current block */
		n = block->length / jp->clusters_per_256k;
		for(j = 0; j < n; j++){
			result = move_file_clusters(hFile,
				block->vcn + j * jp->clusters_per_256k,
				curr_target,jp->clusters_per_256k,jp);
			if(result < 0){
				jp->pi.processed_clusters += clusters_to_process;
				return result;
			}
			jp->pi.processed_clusters += jp->clusters_per_256k;
			clusters_to_process -= jp->clusters_per_256k;
			curr_target += jp->clusters_per_256k;
		}
		/* try to move rest of block */
		r = block->length % jp->clusters_per_256k;
		if(r){
			result = move_file_clusters(hFile,
				block->vcn + j * jp->clusters_per_256k,
				curr_target,r,jp);
			if(result < 0){
				jp->pi.processed_clusters += clusters_to_process;
				return result;
			}
			jp->pi.processed_clusters += r;
			clusters_to_process -= r;
			curr_target += r;
		}
		if(block->next == f->disp.blockmap || i == (n_blocks - 1)) break;
	}

	if(jp->udo.dry_run)
		return 0;

	/* check whether the file was actually moved or not */
	memcpy(&f_cp,f,sizeof(winx_file_info));
	f_cp.disp.blockmap = NULL;
	if(winx_ftw_dump_file(&f_cp,dump_terminator,(void *)jp) >= 0){
		if(f_cp.disp.blockmap){
			block_found = 0;
			for(block = f_cp.disp.blockmap; block; block = block->next){
				if(block->lcn == target){
					block_found = 1;
					break;
				}
				if(block->next == f_cp.disp.blockmap) break;
			}
			if(!block_found){
				DebugPrint("File moving failed: file block is not found on target space");
				DbgPrintBlocksOfFile(f_cp.disp.blockmap);
				winx_list_destroy((list_entry **)(void *)&f_cp.disp.blockmap);
				return (-1);
			}
		}
		winx_list_destroy((list_entry **)(void *)&f_cp.disp.blockmap);
	}

	return 0;
}


/** @} */
