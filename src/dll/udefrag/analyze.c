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
 * @file analyze.c
 * @brief Volume analysis.
 * @addtogroup Analysis
 * @{
 */

/*
* NTFS formatted volumes cannot be safely accessed
* in NT4, as mentioned in program's handbook.
* On our test machines NTFS access caused blue
* screens regardless of whether we accessed it
* from kernel mode or user mode.
* Though, this fact should be tested again,
* because there was lots of improvements in code
* since time of the tests.
*/

#include "udefrag-internals.h"

/**
 * @brief Retrieves all information about the volume.
 * @return Zero for success, negative value otherwise.
 * @note Resets statistics and cluster map.
 */
int get_volume_information(udefrag_job_parameters *jp)
{
	/* reset coordinates of mft zones */
	memset(&jp->mft_zones,0,sizeof(struct _mft_zones));
	
	/* reset v_info structure */
	memset(&jp->v_info,0,sizeof(winx_volume_information));
	
	/* reset statistics */
	jp->pi.files = 0;
	jp->pi.directories = 0;
	jp->pi.compressed = 0;
	jp->pi.fragmented = 0;
	jp->pi.fragments = 0;
	jp->pi.total_space = 0;
	jp->pi.free_space = 0;
	jp->pi.mft_size = 0;
	jp->pi.clusters_to_process = 0;
	jp->pi.processed_clusters = 0;
	
	jp->fs_type = FS_UNKNOWN;
	
	/* reset file lists */
	destroy_lists(jp);
	
	/* update global variables holding drive geometry */
	if(winx_get_volume_information(jp->volume_letter,&jp->v_info) < 0){
		return (-1);
	} else {
		jp->pi.total_space = jp->v_info.total_bytes;
		jp->pi.free_space = jp->v_info.free_bytes;
		if(jp->v_info.bytes_per_cluster) jp->clusters_per_256k = _256K / jp->v_info.bytes_per_cluster;
		else jp->clusters_per_256k = 0;
		DebugPrint("total clusters: %I64u",jp->v_info.total_clusters);
		DebugPrint("cluster size: %I64u",jp->v_info.bytes_per_cluster);
		if(!jp->clusters_per_256k){
			DebugPrint("clusters are larger than 256 kbytes");
			jp->clusters_per_256k ++;
		}
		/* validate geometry */
		if(!jp->v_info.total_clusters || !jp->v_info.bytes_per_cluster){
			DebugPrint("wrong volume geometry detected");
			return (-1);
		}
		/* check partition type */
		DebugPrint("%s partition detected",jp->v_info.fs_name);
		if(!strcmp(jp->v_info.fs_name,"NTFS")){
			jp->fs_type = FS_NTFS;
		} else if(!strcmp(jp->v_info.fs_name,"FAT32")){
			jp->fs_type = FS_FAT32;
		} else if(strstr(jp->v_info.fs_name,"FAT")){
			/* no need to distinguish better */
			jp->fs_type = FS_FAT16;
		} else if(!strcmp(jp->v_info.fs_name,"UDF")){
			jp->fs_type = FS_UDF;
		} else {
//		} else if(!strcmp(jp->v_info.fs_name,"FAT12")){
//			jp->fs_type = FS_FAT12;
//		} else if(!strcmp(jp->v_info.fs_name,"FAT16")){
//			jp->fs_type = FS_FAT16;
//		} else if(!strcmp(jp->v_info.fs_name,"FAT32")){
//			/* check FAT32 version */
//			if(jp->v_info.fat32_mj_version > 0 || jp->v_info.fat32_mn_version > 0){
//				DebugPrint("cannot recognize FAT32 version %u.%u",
//					jp->v_info.fat32_mj_version,jp->v_info.fat32_mn_version);
//				/* for safe low level access in future releases */
//				jp->fs_type = FS_FAT32_UNRECOGNIZED;
//			} else {
//				jp->fs_type = FS_FAT32;
//			}
//		} else {
			DebugPrint("file system type is not recognized");
			DebugPrint("type independent routines will be used to defragment it");
			jp->fs_type = FS_UNKNOWN;
		}
	}

	jp->pi.clusters_to_process = jp->v_info.total_clusters;
	jp->pi.processed_clusters = 0;

	/* reset cluster map */
	reset_cluster_map(jp);
	if(jp->progress_router)
		jp->progress_router(jp); /* redraw progress */
	return 0;
}

/**
 * @brief get_free_space_layout helper.
 */
static int __stdcall process_free_region(winx_volume_region *rgn,void *user_defined_data)
{
	udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;
	
	if(jp->udo.dbgprint_level >= DBG_PARANOID)
		DebugPrint("start: %I64u len: %I64u",rgn->lcn,rgn->length);
	colorize_map_region(jp,rgn->lcn,rgn->length,FREE_SPACE,SYSTEM_SPACE);
	jp->pi.processed_clusters += rgn->length;
	if(jp->progress_router)
		jp->progress_router(jp); /* redraw progress */
	return 0; /* continue scan */
}

/**
 * @brief Retrieves free space layout.
 * @return Zero for success, negative value otherwise.
 */
static int get_free_space_layout(udefrag_job_parameters *jp)
{
	jp->free_regions = winx_get_free_volume_regions(jp->volume_letter,
		WINX_GVR_ALLOW_PARTIAL_SCAN,process_free_region,(void *)jp);
	
	return (jp->free_regions != NULL) ? 0 : (-1);
}

/**
 * @brief Checks whether specified 
 * region is inside processed volume.
 */
int check_region(udefrag_job_parameters *jp,ULONGLONG lcn,ULONGLONG length)
{
	if(lcn < jp->v_info.total_clusters \
	  && (lcn + length) <= jp->v_info.total_clusters)
		return 1;
	
	return 0;
}

/**
 * @brief Retrieves mft zones layout.
 * @note MFT space becomes excluded from the free space list.
 * Because Windows 2000 disallows to move files there.
 * And because on other systems this dirty technique 
 * causes MFT fragmentation.
 */
static void get_mft_zones_layout(udefrag_job_parameters *jp)
{
	ULONGLONG start,length,mirror_size;
	ULONGLONG mft_length = 0;

	if(jp->fs_type != FS_NTFS)
		return;
	
	/* 
	* Don't increment progress counters,
	* because mft zones are partially
	* inside already counted free space pool.
	*/
	DebugPrint("%-12s: %-20s: %-20s", "mft section", "start", "length");

	/* $MFT */
	start = jp->v_info.ntfs_data.MftStartLcn.QuadPart;
	if(jp->v_info.ntfs_data.BytesPerCluster)
		length = jp->v_info.ntfs_data.MftValidDataLength.QuadPart / jp->v_info.ntfs_data.BytesPerCluster;
	else
		length = 0;
	DebugPrint("%-12s: %-20I64u: %-20I64u", "mft", start, length);
	if(check_region(jp,start,length)){
		/* remark space as reserved */
		colorize_map_region(jp,start,length,MFT_ZONE_SPACE,SYSTEM_OR_FREE_SPACE);
		jp->free_regions = winx_sub_volume_region(jp->free_regions,start,length);
		jp->mft_zones.mft_start = start; jp->mft_zones.mft_end = start + length - 1;
		mft_length += length;
	}

	/* MFT Zone */
	start = jp->v_info.ntfs_data.MftZoneStart.QuadPart;
	length = jp->v_info.ntfs_data.MftZoneEnd.QuadPart - jp->v_info.ntfs_data.MftZoneStart.QuadPart + 1;
	DebugPrint("%-12s: %-20I64u: %-20I64u", "mft zone", start, length);
	if(check_region(jp,start,length)){
		/* remark space as reserved */
		colorize_map_region(jp,start,length,MFT_ZONE_SPACE,SYSTEM_OR_FREE_SPACE);
		jp->free_regions = winx_sub_volume_region(jp->free_regions,start,length);
		jp->mft_zones.mftzone_start = start; jp->mft_zones.mftzone_end = start + length - 1;
	}

	/* $MFT Mirror */
	start = jp->v_info.ntfs_data.Mft2StartLcn.QuadPart;
	length = 1;
	mirror_size = jp->v_info.ntfs_data.BytesPerFileRecordSegment * 4;
	if(jp->v_info.ntfs_data.BytesPerCluster && mirror_size > jp->v_info.ntfs_data.BytesPerCluster){
		length = mirror_size / jp->v_info.ntfs_data.BytesPerCluster;
		if(mirror_size - length * jp->v_info.ntfs_data.BytesPerCluster)
			length ++;
	}
	DebugPrint("%-12s: %-20I64u: %-20I64u", "mft mirror", start, length);
	if(check_region(jp,start,length)){
		/* remark space as reserved */
		colorize_map_region(jp,start,length,MFT_ZONE_SPACE,SYSTEM_OR_FREE_SPACE);
		jp->free_regions = winx_sub_volume_region(jp->free_regions,start,length);
		jp->mft_zones.mftmirr_start = start; jp->mft_zones.mftmirr_end = start + length - 1;
	}
	
	jp->pi.mft_size = mft_length * jp->v_info.bytes_per_cluster;
	DebugPrint("mft size = %I64u bytes",jp->pi.mft_size);
	if(jp->progress_router)
		jp->progress_router(jp); /* redraw progress */
}

/**
 * @brief Defines whether the file
 * must be excluded from the volume
 * processing or not.
 * @return Nonzero value indicates
 * that the file must be excluded.
 */
int exclude_by_path(winx_file_info *f,udefrag_job_parameters *jp)
{
	if(jp->udo.in_filter.count){
		if(!winx_patfind(f->path,&jp->udo.in_filter))
			return 1;
	}
	
	return winx_patfind(f->path,&jp->udo.ex_filter);
}

/**
 * @brief find_files helper.
 */
static int __stdcall filter(winx_file_info *f,void *user_defined_data)
{
	udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;
	int length;
	ULONGLONG filesize;
	
	/* skip entries with empty path */
	if(f->path == NULL) goto skip;
	if(f->path[0] == 0) goto skip;
	
	/*
	* Remove trailing dot from the root
	* directory path, otherwise we'll not
	* be able to defragment it.
	*/
	length = wcslen(f->path);
	if(f->path[length - 1] == '.'){
		DebugPrint("root directory detected, its trailing dot will be removed");
		f->path[length - 1] = 0;
	}
	
	/*
	* No files can be filtered out when
	* volume optimization job is requested.
	*/
	if(jp->job_type == OPTIMIZER_JOB)
		goto done;

	/* skip temporary files */
	if(is_temporary(f))	goto skip;

	/* filter files by their sizes */
	filesize = f->disp.clusters * jp->v_info.bytes_per_cluster;
	if(jp->udo.size_limit && filesize > jp->udo.size_limit)
		goto skip;
	
	/* filter files by their number of fragments */
	if(jp->udo.fragments_limit && f->disp.fragments < jp->udo.fragments_limit)
		goto skip;

	/* filter files by their paths */
	if(exclude_by_path(f,jp))
		goto skip;
	
done:
	/* very important: don't scan inside reparse points */
	if(is_reparse_point(f))
		goto skip; /* reparse points are always excluded */

	/* continue scan for ordinary not filtered out files */
	return 0;

skip:
	f->user_defined_flags |= UD_FILE_EXCLUDED;
	return 1;
}

/**
 * @brief find_files helper.
 */
static void __stdcall progress_callback(winx_file_info *f,void *user_defined_data)
{
	udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;
	ULONGLONG filesize;

	/* update progress counters */
	jp->pi.files ++;
	if(is_directory(f)) jp->pi.directories ++;
	if(is_compressed(f)) jp->pi.compressed ++;
	jp->pi.processed_clusters += f->disp.clusters;

	/* show debugging information about interesting cases */
	if(is_sparse(f) && f->path)
		DebugPrint("sparse file found: %ws",f->path);
	
	/*
	* Mark files of size larger than threshold
	* to be able to show them in dark colors on the map.
	*/
	filesize = f->disp.clusters * jp->v_info.bytes_per_cluster;
	if(jp->udo.size_limit && filesize > jp->udo.size_limit)
		f->user_defined_flags |= UD_FILE_OVER_LIMIT;

	/* FIXME: it shows approx. 1.6 Gb instead of 3.99 Gb */
	/*if(wcsstr(path,L"largefile"))
		DebugPrint("SIZE = %I64u\n", filesize);
	*/
	if(jp->progress_router)
		jp->progress_router(jp); /* redraw progress */
}

/**
 * @brief find_files helper.
 */
static int __stdcall terminator(void *user_defined_data)
{
	udefrag_job_parameters *jp = (udefrag_job_parameters *)user_defined_data;

	return jp->termination_router((void *)jp);
}

/**
 * @brief Guesses whether Explorer's 
 * context menu handler requested
 * defragmentation job or not.
 * @return Nonzero value indicates
 * that likely context menu handler
 * requested the job.
 * @note 
 * - get_options call must preceed
 *   this call.
 * - returns zero if entire disk
 *   processing is requested.
 */
int is_context_menu_handler(udefrag_job_parameters *jp)
{
	int length;
	
	/* single pattern must exist */
	if(jp->udo.in_filter.count != 1)
		return 0;
	
	/* at least x:\y must exist */
	length = wcslen(jp->udo.in_filter.array[0]);
	if(length < 4)
		return 0;
	
	/* check for x:\ */
	if(jp->udo.in_filter.array[0][1] != ':' || \
	  jp->udo.in_filter.array[0][2] != '\\')
		return 0;
	
	/* ok, we are in context menu handler */
	return 1;
}

/**
 * @brief Searches for all files on disk.
 * @return Zero for success, negative value otherwise.
 */
static int find_files(udefrag_job_parameters *jp)
{
	int context_menu_handler;
	short path[MAX_PATH + 1];
	short *p, *s_filter;
	winx_file_info *f;

	/*
	* FIXME: context menu handler will process files
	* with similar names: if c:\1 is requested, 
	* c:\1.txt, c:\1.html etc. will be processed too.
	*/

	/* check for context menu handler */
	context_menu_handler = is_context_menu_handler(jp);
	if(context_menu_handler)
		DebugPrint("context menu handler?");

	/* speed up context menu handler */
	if(jp->fs_type != FS_NTFS && context_menu_handler){
		DebugPrint("context menu handler on FAT?");
		s_filter = jp->udo.in_filter.array[0];
		/* let's assume that directory processing is requested */
		_snwprintf(path,MAX_PATH,L"\\??\\%ws",s_filter);
		jp->filelist = winx_ftw(path,
			WINX_FTW_RECURSIVE | WINX_FTW_DUMP_FILES | WINX_FTW_ALLOW_PARTIAL_SCAN,
			filter,progress_callback,terminator,(void *)jp);
		if(jp->filelist == NULL){
			/* it seems that single file processing is requested */
			DebugPrint("single file processing requested? %ws",s_filter);
			/* get parent directory path */
			p = wcsrchr(path,'\\');
			if(p == path + wcslen(L"\\??\\C:")){
				/* root directory, save trailing backslash */
				p[1] = 0;
			} else {
				p[0] = 0;
			}
			/* scan parent directory not recursively */
			jp->filelist = winx_ftw(path,
				WINX_FTW_DUMP_FILES | WINX_FTW_ALLOW_PARTIAL_SCAN,
				filter,progress_callback,terminator,(void *)jp);
		} else {
			DebugPrint("directory processing requested? %ws",s_filter);
		}
	} else {
		jp->filelist = winx_scan_disk(jp->volume_letter,
			WINX_FTW_DUMP_FILES | WINX_FTW_ALLOW_PARTIAL_SCAN,
			filter,progress_callback,terminator,(void *)jp);
	}
	if(jp->filelist == NULL && !jp->termination_router((void *)jp))
		return (-1);
	
	/* calculate number of fragmented files; redraw map */
	for(f = jp->filelist; f; f = f->next){
		/* skip excluded files and reparse points */
		if(!is_fragmented(f) || is_reparse_point(f) || is_excluded(f)){
			jp->pi.fragments ++;
		} else {
			jp->pi.fragmented ++;
			jp->pi.fragments += f->disp.fragments;
		}
		/* redraw cluster map */
		if(jp->fs_type == FS_NTFS)
			colorize_file(jp,f,SYSTEM_OR_MFT_ZONE_SPACE);
		else
			colorize_file(jp,f,SYSTEM_SPACE);
		//DebugPrint("%ws",f->path);
		if(jp->progress_router) /* need speedup? */
			jp->progress_router(jp); /* redraw progress */
		if(f->next == jp->filelist) break;
	}

	DebugPrint("files found: %u",jp->pi.files);
	DebugPrint("fragmented files: %u",jp->pi.fragmented);
	return 0;
}

/**
 * @brief Defines whether the file is from
 * well known locked files list or not.
 * @note Skips $Mft file, because it is
 * intended to be drawn in magenta color.
 */
static int is_well_known_locked_file(winx_file_info *f,udefrag_job_parameters *jp)
{
	int length;
	wchar_t mft_name[] = L"$Mft";
	wchar_t config_sign[] = L"\\system32\\config\\";
	
	length = wcslen(f->path);

	/* search for NTFS internal files */
	if(length >= 9){ /* to ensure that we have at least \??\X:\$x */
		if(f->path[7] == '$'){
			if(length == 11){
				if(winx_wcsistr(f->name,mft_name))
					return 0; /* skip $mft */
			}
			return 1;
		}
	}
	
	if(winx_wcsistr(f->name,L"pagefile.sys"))
		return 1;
	if(winx_wcsistr(f->name,L"hiberfil.sys"))
		return 1;
	if(winx_wcsistr(f->name,L"ntuser.dat"))
		return 1;

	if(winx_wcsistr(f->path,config_sign)){
		if(winx_wcsistr(f->name,L"sam"))
			return 1;
		if(winx_wcsistr(f->name,L"system"))
			return 1;
		if(winx_wcsistr(f->name,L"software"))
			return 1;
		if(winx_wcsistr(f->name,L"security"))
			return 1;
	}
	
	return 0;
}

/**
 * @brief Opens the file for moving.
 */
NTSTATUS udefrag_fopen(winx_file_info *f,HANDLE *phFile)
{
	UNICODE_STRING us;
	OBJECT_ATTRIBUTES oa;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;
	ULONG flags = FILE_SYNCHRONOUS_IO_NONALERT;

	if(f == NULL || phFile == NULL)
		return STATUS_INVALID_PARAMETER;
	
	if(is_directory(f))
		flags |= FILE_OPEN_FOR_BACKUP_INTENT;
	else
		flags |= FILE_NO_INTERMEDIATE_BUFFERING;
	
	RtlInitUnicodeString(&us,f->path);
	InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
	/*
	* Don't specify anything in addition
	* to a SYNCHRONIZE access right, otherwise
	* you will not be able to defragment $Mft
	* as well as other internal NTFS files
	* on Windows XP and Windows Server 2003 systems.
	* A single SYNCHRONIZE flag also enables internal
	* NTFS files defragmentation on Windows 2000.
	*/
	status = NtCreateFile(phFile,/*FILE_GENERIC_READ |*/ SYNCHRONIZE,
				&oa,&iosb,NULL,0,FILE_SHARE_READ | FILE_SHARE_WRITE,
				FILE_OPEN,flags,NULL,0);
	return status;
}

/**
 * @brief Defines whether the file
 * is locked by system or not.
 * @return Nonzero value indicates
 * that the file is locked.
 */
int is_file_locked(winx_file_info *f,udefrag_job_parameters *jp)
{
	NTSTATUS status;
	HANDLE hFile;

	status = udefrag_fopen(f,&hFile);
	if(status == STATUS_SUCCESS){
		NtCloseSafe(hFile);
		return 0;
	}

	/*DebugPrintEx(status,"cannot open %ws",f->path);*/
	/* redraw space */
	colorize_file_as_system(jp,f);
	/* file is locked by other application, so its state is unknown */
	/* don't reset its statistics though! */
	/*f->disp.clusters = 0;
	f->disp.fragments = 0;
	f->disp.flags = 0;*/
	winx_list_destroy((list_entry **)(void *)&f->disp.blockmap);
	f->user_defined_flags |= UD_FILE_LOCKED;
	if(jp->progress_router)
		jp->progress_router(jp); /* redraw progress */
	return 1;
}

/**
 * @brief Searches for well known locked files
 * and applies their dispositions to the map.
 * @details Resets f->disp structure of locked files.
 * @todo Speed up.
 */
static void redraw_well_known_locked_files(udefrag_job_parameters *jp)
{
	winx_file_info *f;
	ULONGLONG time;

	winx_dbg_print_header(0,0,"search for well known locked files...");
	time = winx_xtime();
	
	for(f = jp->filelist; f; f = f->next){
		if(f->disp.blockmap && f->path && f->name){ /* otherwise nothing to redraw */
			if(is_well_known_locked_file(f,jp)){
				if(!is_file_locked(f,jp)){
					/* possibility of this case must be reduced */
					DebugPrint("false detection: %ws",f->path);
				}
			}
		}
		if(f->next == jp->filelist) break;
	}

	winx_dbg_print_header(0,0,"well known locked files search completed in %I64u ms",
		winx_xtime() - time);
}

/**
 * @brief Produces list of fragmented files.
 */
static void produce_list_of_fragmented_files(udefrag_job_parameters *jp)
{
	winx_file_info *f;
	udefrag_fragmented_file *ff, *ffprev;
	
	for(f = jp->filelist; f; f = f->next){
		if(is_fragmented(f) && !is_excluded(f)){
			ffprev = NULL;
			/* add file to list of fragmented */
			for(ff = jp->fragmented_files; ff; ff = ff->next){
				if(ff->f->disp.fragments <= f->disp.fragments){
					if(ff != jp->fragmented_files)
						ffprev = ff->prev;
					break;
				}
				if(ff->next == jp->fragmented_files){
					ffprev = ff;
					break;
				}
			}
			
			ff = (udefrag_fragmented_file *)winx_list_insert_item((list_entry **)(void *)&jp->fragmented_files,
					(list_entry *)ffprev,sizeof(udefrag_fragmented_file));
			if(ff == NULL){
				DebugPrint("produce_list_of_fragmented_files: cannot allocate %u bytes of memory",
					sizeof(udefrag_fragmented_file));
			} else {
				ff->f = f;
			}
		}
		if(f->next == jp->filelist) break;
	}
}

/**
 * @brief Define whether some
 * actions are allowed or not.
 * @return Zero indicates that
 * requested operation is allowed,
 * negative value indicates contrary.
 */
static int define_allowed_actions(udefrag_job_parameters *jp)
{
	/*
	* NTFS volumes with cluster size greater than 4 kb
	* cannot be defragmented on Windows 2000.
	* This is a well known limitation of Windows Defrag API.
	*/
	if(jp->job_type != ANALYSIS_JOB \
	  && jp->fs_type == FS_NTFS \
	  && jp->v_info.bytes_per_cluster > 4096 \
	  && winx_get_os_version() == 50){
		DebugPrint("Cannot defragment NTFS volumes with\n"
			"cluster size greater than 4 kb\n"
			"on Windows 2000 (read docs for details).");
		return UDEFRAG_W2K_4KB_CLUSTERS;
	}
	  
	/*
	* UDF volumes cannot be either defragmented or optimized
	* because an appropriate system driver has poor support
	* of FSCTL_MOVE_FILE.
	*/
	if(jp->job_type != ANALYSIS_JOB && jp->fs_type == FS_UDF){
		DebugPrint("Cannot defragment/optimize UDF volumes\n");
		DebugPrint("because of poor support of FSCTL_MOVE_FILE\n");
		DebugPrint("by an appropriate system driver.\n");
		return UDEFRAG_UDF_DEFRAG;
	}
	
	/*
	* FAT volumes cannot be optimized.
	*/
	if(jp->job_type == OPTIMIZER_JOB \
	  && (jp->fs_type == FS_FAT12 \
	  || jp->fs_type == FS_FAT16 \
	  || jp->fs_type == FS_FAT32 \
	  || jp->fs_type == FS_FAT32_UNRECOGNIZED)){
		DebugPrint("Cannot optimize FAT volumes because of unmoveable directories\n");
		return UDEFRAG_FAT_OPTIMIZATION;
	}

	switch(jp->fs_type){
	case FS_FAT12:
	case FS_FAT16:
	case FS_FAT32:
	case FS_FAT32_UNRECOGNIZED:
		jp->actions.allow_dir_defrag = 0;
		jp->actions.allow_optimize = 0;
		break;
	case FS_NTFS:
		jp->actions.allow_dir_defrag = 1;
		jp->actions.allow_optimize = 1;
		break;
	default:
		jp->actions.allow_dir_defrag = 0;
		jp->actions.allow_optimize = 0;
		/*jp->actions.allow_dir_defrag = 1;
		jp->actions.allow_optimize = 1;*/
		break;
	}
	
	if(jp->actions.allow_dir_defrag)
		DebugPrint("directory defragmentation is allowed");
	else
		DebugPrint("directory defragmentation is denied (because not possible)");
	
	if(jp->actions.allow_optimize)
		DebugPrint("volume optimization is allowed");
	else
		DebugPrint("volume optimization is denied (because not possible)");
	
	return 0;
}

/**
 * @brief Performs a volume analysis.
 * @return Zero for success, negative value otherwise.
 */
int analyze(udefrag_job_parameters *jp)
{
	ULONGLONG time;
	int result;
	
	winx_dbg_print_header(0,0,"analysis of %c: started",jp->volume_letter);
	jp->pi.current_operation = 'A';
	time = winx_xtime();
	
	/* update volume information */
	if(get_volume_information(jp) < 0)
		return (-1);
	
	/* scan volume for free space areas */
	if(get_free_space_layout(jp) < 0)
		return (-1);
	
	/* apply information about mft zones */
	get_mft_zones_layout(jp);
	
	/* search for files */
	if(find_files(jp) < 0)
		return (-1);
	
	/* we'd like to behold well known locked files in green color */
	redraw_well_known_locked_files(jp);

	/* produce list of fragmented files */
	produce_list_of_fragmented_files(jp);

	result = define_allowed_actions(jp);
	if(result < 0)
		return result;
	
	winx_dbg_print_header(0,0,"analysis of %c: completed in %I64u ms",
		jp->volume_letter, winx_xtime() - time);
	return 0;
}

/** @} */
