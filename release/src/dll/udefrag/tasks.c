/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file tasks.c
 * @brief Volume processing tasks.
 * @details Set of atomic volume processing tasks 
 * defined here is built over the move_file routine
 * and is intended to build defragmentation and 
 * optimization routines over it.
 * @addtogroup Tasks
 * @{
 */

/*
* Ideas by Dmitri Arkhangelski <dmitriar@gmail.com>
* and Stefan Pendl <stefanpe@users.sourceforge.net>.
*/

#include "udefrag-internals.h"

/************************************************************/
/*                    Internal Routines                     */
/************************************************************/

/**
 * @brief Defines whether the file can be moved or not.
 */
int can_move(winx_file_info *f,udefrag_job_parameters *jp)
{
    wchar_t *boot_files[] = {
        L"*\\safeboot.fs",   /* http://www.safeboot.com/ */
        L"*\\Gobackio.bin",  /* Symantec GoBack */
        L"*\\PGPWDE0*",      /* PGP Whole Disk Encryption */
        L"*\\bootwiz*",      /* Acronis OS Selector */
        L"*\\BootAuth?.sys", /* DriveCrypt (http://www.securstar.com/) */
        L"*\\$dcsys$",       /* DiskCryptor (Diskencryption Software) */
        L"*\\bootstat.dat",  /* part of Windows */
        NULL
    };
    int i;

    /* skip files with empty path */
    if(f->path == NULL)
        return 0;
    if(f->path[0] == 0)
        return 0;
    
    /* skip files already moved to front in optimization */
    if(is_moved_to_front(f))
        return 0;
    
    /* skip files already excluded by the current task */
    if(is_currently_excluded(f))
        return 0;
    
    /* skip files with undefined cluster map and locked files */
    if(f->disp.blockmap == NULL || is_locked(f))
        return 0;
    
    /* skip files of zero length */
    if(f->disp.clusters == 0 || \
      (f->disp.blockmap->next == f->disp.blockmap && \
      f->disp.blockmap->length == 0)){
        f->user_defined_flags |= UD_FILE_IMPROPER_STATE;
        return 0;
    }

    /* skip FAT directories */
    if(is_directory(f) && !jp->actions.allow_dir_defrag)
        return 0;
    
    /* skip file in case of improper state detected */
    if(is_in_improper_state(f))
        return 0;
    
    /* avoid infinite loops */
    if(is_moving_failed(f))
        return 0;
    
    /* keep the computer bootable */
    if(is_not_essential_file(f)) return 1;
    if(is_essential_boot_file(f)) return 0;
    for(i = 0; boot_files[i]; i++){
        if(winx_wcsmatch(f->path,boot_files[i],WINX_PAT_ICASE)){
            DebugPrint("can_move: essential boot file detected: %ws",f->path);
            f->user_defined_flags |= UD_FILE_ESSENTIAL_BOOT_FILE;
            return 0;
        }
    }
    f->user_defined_flags |= UD_FILE_NOT_ESSENTIAL_FILE;
    return 1;
}

/**
 * @brief Defines whether the file can be defragmented or not.
 */
int can_defragment(winx_file_info *f,udefrag_job_parameters *jp)
{
    if(!can_move(f,jp))
        return 0;

    /* skip files with less than 2 fragments */
    if(f->disp.blockmap->next == f->disp.blockmap \
      || f->disp.fragments < 2 || !is_fragmented(f))
        return 0;
        
    /* in MFT optimization defragment marked files only */
    if(jp->job_type == MFT_OPTIMIZATION_JOB \
      && !is_fragmented_by_mft_opt(f))
        return 0;
    
    return 1;
}

/**
 * @brief Defines whether MFT can be optimized or not.
 */
static int can_optimize_mft(udefrag_job_parameters *jp,winx_file_info **mft_file)
{
    winx_file_info *f, *file;
    
    if(mft_file)
        *mft_file = NULL;

    /* MFT can be optimized on NTFS formatted volumes only */
    if(jp->fs_type != FS_NTFS)
        return 0;
    
    /* MFT is not movable on systems prior to xp */
    if(winx_get_os_version() < WINDOWS_XP)
        return 0;
    
    /*
    * In defragmentation this task is impossible because
    * of lack of information needed to cleanup space.
    */
    if(jp->job_type == DEFRAGMENTATION_JOB){
        DebugPrint("optimize_mft: MFT optimization is impossible in defragmentation task");
        return 0;
    }
    
    /* search for $mft file */
    file = NULL;
    for(f = jp->filelist; f; f = f->next){
        if(is_mft(f,jp)){
            file = f;
            break;
        }
        if(f->next == jp->filelist) break;
    }
    if(file == NULL){
        DebugPrint("optimize_mft: cannot find $mft file");
        return 0;
    }
    
    /* is $mft file locked? */
    if(is_file_locked(file,jp)){
        DebugPrint("optimize_mft: $mft file is locked");
        return 0;
    }
    
    /* can we move something? */
    if(!file->disp.blockmap \
      || !file->disp.clusters \
      || is_in_improper_state(file)){
        DebugPrint("optimize_mft: no movable data detected");
        return 0;
    }
      
    /* is MFT already optimized? */
    if(!is_fragmented(file) || file->disp.blockmap->next == file->disp.blockmap){
        DebugPrint("optimize_mft: $mft file is already optimized");
        return 0;
    }

    if(mft_file)
        *mft_file = file;
    return 1;
}

/**
 * @brief Sends list of $mft blocks to the debugger.
 */
static void list_mft_blocks(winx_file_info *mft_file)
{
    winx_blockmap *block;
    ULONGLONG i;

    for(block = mft_file->disp.blockmap, i = 0; block; block = block->next, i++){
        DebugPrint("mft part #%I64u start: %I64u, length: %I64u",
            i,block->lcn,block->length);
        if(block->next == mft_file->disp.blockmap) break;
    }
}

/************************************************************/
/*                      Atomic Tasks                        */
/************************************************************/

/**
 * @brief Optimizes MFT by placing its parts 
 * after the first one as close as possible.
 * @details We cannot adjust MFT zone, but
 * usually it should follow optimized MFT 
 * automatically. Also we have no need to move
 * MFT to a different location than the one 
 * initially chosen by the O/S, since on big 
 * volumes it makes sense to keep it near 
 * the middle of the disk, so the disk head
 * does not have to move back and forth across
 * the whole disk to access files at the back
 * of the volume.
 * @note As a side effect it may increase
 * number of fragmented files.
 */
int optimize_mft_helper(udefrag_job_parameters *jp)
{
    ULONGLONG clusters_to_process; /* the number of $mft clusters not processed yet */
    ULONGLONG start_vcn;           /* VCN of the portion of $mft not processed yet */
    ULONGLONG clusters_to_move;    /* the number of $mft clusters intended for the current move */
    ULONGLONG start_lcn;           /* address of space not processed yet */
    ULONGLONG time, tm;
    winx_volume_region *rlist = NULL, *rgn, *r, *target_rgn;
    winx_file_info *mft_file, *first_file;
    winx_blockmap *block, *first_block;
    ULONGLONG end_lcn, min_lcn, next_vcn;
    ULONGLONG current_vcn, remaining_clusters, n, lcn;
    winx_volume_region region = {0};
    ULONGLONG clusters_to_cleanup;
    ULONGLONG target;
    int block_cleaned_up;
    char buffer[32];
    
    jp->pi.current_operation = VOLUME_OPTIMIZATION;
    jp->pi.moved_clusters = 0;
    jp->pi.total_moves = 0;
    
    if(!can_optimize_mft(jp,&mft_file))
        return 0;
    
    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return (-1);

    DebugPrint("optimize_mft: initial $mft map:");
    list_mft_blocks(mft_file);
    
    time = start_timing("mft optimization",jp);
    
    /* get list of free regions - without exclusion of MFT zone */
    tm = winx_xtime();
    rlist = winx_get_free_volume_regions(jp->volume_letter,
        WINX_GVR_ALLOW_PARTIAL_SCAN,NULL,(void *)jp);
    jp->p_counters.temp_space_releasing_time += winx_xtime() - tm;

    clusters_to_process = mft_file->disp.clusters - mft_file->disp.blockmap->length;
    start_lcn = mft_file->disp.blockmap->lcn + mft_file->disp.blockmap->length;
    start_vcn = mft_file->disp.blockmap->next->vcn;

try_again:
    while(clusters_to_process > 0){
        if(jp->termination_router((void *)jp)) break;
        if(rlist == NULL) break;

        /* release temporary allocated space */
        release_temp_space_regions(jp);
        
        /* search for the first free region after start_lcn */
        target_rgn = NULL; tm = winx_xtime();
        for(rgn = rlist; rgn; rgn = rgn->next){
            if(rgn->lcn >= start_lcn && rgn->length){
                target_rgn = rgn;
                break;
            }
            if(rgn->next == rlist) break;
        }
        jp->p_counters.searching_time += winx_xtime() - tm;
        
        /* process file blocks between start_lcn and target_rgn */
        if(target_rgn) end_lcn = target_rgn->lcn;
        else end_lcn = jp->v_info.total_clusters;
        clusters_to_cleanup = clusters_to_process;
        block_cleaned_up = 0;
        region.length = 0;
        while(clusters_to_cleanup > 0){
            if(jp->termination_router((void *)jp)) goto done;
            min_lcn = start_lcn;
            first_block = find_first_block(jp, &min_lcn, MOVE_ALL, 0, &first_file);
            if(first_block == NULL) break;
            if(first_block->lcn >= end_lcn) break;
            
            /* does the first block follow a previously moved one? */
            if(block_cleaned_up){
                if(first_block->lcn != region.lcn + region.length)
                    break;
                if(first_file == mft_file)
                    break;
            }
            
            /* don't move already optimized parts of $mft */
            if(first_file == mft_file && first_block->vcn == start_vcn){
                if(clusters_to_process <= first_block->length \
                  || first_block->next == first_file->disp.blockmap){
                    clusters_to_process = 0;
                    goto done;
                } else {
                    clusters_to_process -= first_block->length;
                    clusters_to_cleanup -= first_block->length;
                    start_vcn = first_block->next->vcn;
                    start_lcn = first_block->lcn + first_block->length;
                    continue;
                }
            }
            
            /* cleanup space */
            if(rlist == NULL) goto done;
            lcn = first_block->lcn;
            current_vcn = first_block->vcn;
            clusters_to_move = remaining_clusters = min(clusters_to_cleanup, first_block->length);
            while(remaining_clusters){
                /* use last free region */
                if(rlist == NULL) goto done;
                rgn = NULL;
                for(r = rlist->prev; r; r = r->prev){
                    if(r->length > 0){
                        rgn = r;
                        break;
                    }
                    if(r->prev == rlist->prev) break;
                }
                if(rgn == NULL) goto done;
                
                n = min(rgn->length,remaining_clusters);
                target = rgn->lcn + rgn->length - n;
                /* subtract target clusters from auxiliary list of free regions */
                rlist = winx_sub_volume_region(rlist,target,n);
                if(first_file != mft_file)
                    first_file->user_defined_flags |= UD_FILE_FRAGMENTED_BY_MFT_OPT;
                if(move_file(first_file,current_vcn,n,target,0,jp) < 0){
                    if(!block_cleaned_up){
                        start_lcn = lcn + clusters_to_move;
                        goto try_again;
                    } else {
                        goto move_mft;
                    }
                }
                current_vcn += n;
                remaining_clusters -= n;
            }
            /* space cleaned up successfully */
            region.next = region.prev = &region;
            if(!block_cleaned_up)
                region.lcn = lcn;
            region.length += clusters_to_move;
            target_rgn = &region;
            start_lcn = region.lcn + region.length;
            clusters_to_cleanup -= clusters_to_move;
            block_cleaned_up = 1;
        }
    
move_mft:        
        /* release temporary allocated space ! */
        release_temp_space_regions(jp);
        
        /* target_rgn points to the target free region, so let's move the next portion of $mft */
        if(target_rgn == NULL) break;
        n = clusters_to_move = min(clusters_to_process,target_rgn->length);
        next_vcn = 0; current_vcn = start_vcn;
        for(block = mft_file->disp.blockmap; block && n; block = block->next){
            if(block->vcn + block->length > start_vcn){
                if(n > block->length - (current_vcn - block->vcn)){
                    n -= block->length - (current_vcn - block->vcn);
                    current_vcn = block->next->vcn;
                } else {
                    if(n == block->length - (current_vcn - block->vcn)){
                        if(block->next == mft_file->disp.blockmap)
                            next_vcn = block->vcn + block->length;
                        else
                            next_vcn = block->next->vcn;
                    } else {
                        next_vcn = current_vcn + n;
                    }
                    break;
                }
            }
            if(block->next == mft_file->disp.blockmap) break;
        }
        if(next_vcn == 0){
            DebugPrint("optimize_mft: next_vcn calculation failed");
            break;
        }
        target = target_rgn->lcn;
        /* subtract target clusters from auxiliary list of free regions */
        rlist = winx_sub_volume_region(rlist,target,clusters_to_move);
        if(move_file(mft_file,start_vcn,clusters_to_move,target,0,jp) < 0){
            if(jp->last_move_status != STATUS_ALREADY_COMMITTED){
                /* on unrecoverable failures exit */
                break;
            }
            /* go forward and try to cleanup next blocks */
            mft_file->user_defined_flags &= ~UD_FILE_MOVING_FAILED;
            start_lcn = target + clusters_to_move;
            continue;
        }
        /* $mft part moved successfully */
        clusters_to_process -= clusters_to_move;
        start_lcn = target + clusters_to_move;
        start_vcn = next_vcn;
        jp->pi.total_moves ++;
    }

done:
    DebugPrint("optimize_mft: final $mft map:");
    list_mft_blocks(mft_file);
    
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    stop_timing("mft optimization",time,jp);
    winx_release_free_volume_regions(rlist);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    return 0;
}

/**
 * @brief Calculates total number of fragmented clusters.
 */
static ULONGLONG fragmented_clusters(udefrag_job_parameters *jp)
{
    udefrag_fragmented_file *f;
    ULONGLONG n = 0;
    
    for(f = jp->fragmented_files; f; f = f->next){
        if(jp->termination_router((void *)jp)) break;
        /*
        * Count all fragmented files which can be processed.
        */
        if(can_defragment(f->f,jp))
            n += f->f->disp.clusters;
        if(f->next == jp->fragmented_files) break;
    }
    return n;
}

/**
 * @brief Defragments all fragmented files entirely, if possible.
 * @details 
 * - This routine fills free space areas from the beginning of the
 * volume regardless of the best matching rules.
 */
int defragment_small_files_walk_free_regions(udefrag_job_parameters *jp)
{
    ULONGLONG time, tm;
    ULONGLONG defragmented_files;
    winx_volume_region *rgn;
    udefrag_fragmented_file *f, *f_largest;
    ULONGLONG length;
    char buffer[32];
    ULONGLONG clusters_to_move;
    winx_file_info *file;
    ULONGLONG file_length;
    
    jp->pi.current_operation = VOLUME_DEFRAGMENTATION;
    jp->pi.moved_clusters = 0;

    /* free as much temporarily allocated space as possible */
    release_temp_space_regions(jp);
    
    /* no files are excluded by this task currently */
    for(f = jp->fragmented_files; f; f = f->next){
        f->f->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
        if(f->next == jp->fragmented_files) break;
    }

    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return (-1);

    time = start_timing("defragmentation",jp);
    
    jp->pi.clusters_to_process = \
        jp->pi.processed_clusters + fragmented_clusters(jp);

    /* fill free regions in the beginning of the volume */
    defragmented_files = 0;
    for(rgn = jp->free_regions; rgn; rgn = rgn->next){
        if(jp->termination_router((void *)jp)) break;
        
        /* skip micro regions */
        if(rgn->length < 2) goto next_rgn;

        /* find largest fragmented file that fits in the current region */
        do {
            if(jp->termination_router((void *)jp)) goto done;
            f_largest = NULL, length = 0; tm = winx_xtime();
            for(f = jp->fragmented_files; f; f = f->next){
                file_length = f->f->disp.clusters;
                if(file_length > length && file_length <= rgn->length){
                    if(can_defragment(f->f,jp) && !is_mft(f->f,jp)){
                        f_largest = f;
                        length = file_length;
                    }
                }
                if(f->next == jp->fragmented_files) break;
            }
            jp->p_counters.searching_time += winx_xtime() - tm;
            if(f_largest == NULL) goto next_rgn;
            file = f_largest->f; /* f_largest may be destroyed by move_file */
            
            /* move the file */
            clusters_to_move = file->disp.clusters;
            if(move_file(file,file->disp.blockmap->vcn,
             clusters_to_move,rgn->lcn,0,jp) >= 0){
                if(jp->udo.dbgprint_level >= DBG_DETAILED)
                    DebugPrint("Defrag success for %ws",file->path);
                defragmented_files ++;
            } else {
                DebugPrint("Defrag failure for %ws",file->path);
                /* exclude file from the current task */
                file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
            }
            
            /* skip locked files here to prevent skipping the current free region */
        } while(is_locked(file));
        
        /* after file moving continue from the first free region */
        if(jp->free_regions == NULL) break;
        rgn = jp->free_regions->prev;
        continue;
        
    next_rgn:
        if(rgn->next == jp->free_regions) break;
    }

done:
    /* display amount of moved data and number of defragmented files */
    DebugPrint("%I64u files defragmented",defragmented_files);
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    stop_timing("defragmentation",time,jp);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    return 0;
}

/**
 * @brief Defragments all fragmented files entirely, if possible.
 * @details This routine fills free space areas respect to the best matching rules.
 */
int defragment_small_files_walk_fragmented_files(udefrag_job_parameters *jp)
{
    ULONGLONG time, tm;
    ULONGLONG defragmented_files;
    winx_volume_region *rgn;
    udefrag_fragmented_file *f, *f_largest;
    ULONGLONG length;
    char buffer[32];
    ULONGLONG clusters_to_move;
    winx_file_info *file;
    ULONGLONG file_length;

    jp->pi.current_operation = VOLUME_DEFRAGMENTATION;
    jp->pi.moved_clusters = 0;

    /* free as much temporarily allocated space as possible */
    release_temp_space_regions(jp);

    /* no files are excluded by this task currently */
    for(f = jp->fragmented_files; f; f = f->next){
        f->f->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
        if(f->next == jp->fragmented_files) break;
    }

    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return (-1);

    time = start_timing("defragmentation",jp);

    jp->pi.clusters_to_process = \
        jp->pi.processed_clusters + fragmented_clusters(jp);

    /* find best matching free region for each fragmented file */
    defragmented_files = 0;
    while(jp->termination_router((void *)jp) == 0){
        f_largest = NULL, length = 0; tm = winx_xtime();
        for(f = jp->fragmented_files; f; f = f->next){
            file_length = f->f->disp.clusters;
            if(file_length > length){
                if(can_defragment(f->f,jp) && !is_mft(f->f,jp)){
                    f_largest = f;
                    length = file_length;
                }
            }
            if(f->next == jp->fragmented_files) break;
        }
        jp->p_counters.searching_time += winx_xtime() - tm;
        if(f_largest == NULL) break;
        file = f_largest->f; /* f_largest may be destroyed by move_file */

        rgn = find_matching_free_region(jp,file->disp.blockmap->lcn,file->disp.clusters,FIND_MATCHING_RGN_ANY);
        if(jp->termination_router((void *)jp)) break;
        if(rgn == NULL){
            /* exclude file from the current task */
            file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
        } else {
            /* move the file */
            clusters_to_move = file->disp.clusters;
            if(move_file(file,file->disp.blockmap->vcn,
             clusters_to_move,rgn->lcn,0,jp) >= 0){
                if(jp->udo.dbgprint_level >= DBG_DETAILED)
                    DebugPrint("Defrag success for %ws",file->path);
                defragmented_files ++;
            } else {
                DebugPrint("Defrag failure for %ws",file->path);
                /* exclude file from the current task */
                file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
            }
        }
    }
    
    /* display amount of moved data and number of defragmented files */
    DebugPrint("%I64u files defragmented",defragmented_files);
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    stop_timing("defragmentation",time,jp);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    return 0;
}

/**
 * @brief Defragments all fragmented files at least 
 * partially, when possible.
 * @details
 * - If some file cannot be defragmented due to its size,
 * this routine marks it by UD_FILE_TOO_LARGE flag.
 * - This routine fills free space areas starting
 * from the biggest one to concatenate as much fragments
 * as possible.
 * - This routine uses UD_MOVE_FILE_CUT_OFF_MOVED_CLUSTERS
 * flag to avoid infinite loops, therefore processed file
 * maps become cut. This is not a problem while we use
 * a single defragment_big_files call after all other
 * volume processing steps.
 */
int defragment_big_files(udefrag_job_parameters *jp)
{
    ULONGLONG time, tm;
    ULONGLONG defragmented_files;
    winx_volume_region *rgn_largest;
    udefrag_fragmented_file *f, *f_largest;
    ULONGLONG target, length;
    winx_blockmap *block, *first_block, *longest_sequence;
    ULONGLONG n_blocks, max_n_blocks, longest_sequence_length;
    ULONGLONG remaining_clusters;
    char buffer[32];
    winx_file_info *file;
    ULONGLONG file_length;

    jp->pi.current_operation = VOLUME_DEFRAGMENTATION;
    jp->pi.moved_clusters = 0;

    /* free as much temporarily allocated space as possible */
    release_temp_space_regions(jp);

    /* no files are excluded by this task currently */
    for(f = jp->fragmented_files; f; f = f->next){
        f->f->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
        if(f->next == jp->fragmented_files) break;
    }

    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return (-1);

    time = start_timing("partial defragmentation",jp);
    
    jp->pi.clusters_to_process = \
        jp->pi.processed_clusters + fragmented_clusters(jp);

    /* fill largest free region first */
    defragmented_files = 0;
    
    if(winx_get_os_version() <= WINDOWS_2K && jp->fs_type == FS_NTFS){
        DebugPrint("Windows NT 4.0 and Windows 2000 have stupid limitations in defrag API");
        DebugPrint("a partial file defragmentation is almost impossible on these systems");
        goto done;
    }
    
    while(jp->termination_router((void *)jp) == 0){
        /* find largest free space region */
        rgn_largest = find_largest_free_region(jp);
        if(rgn_largest == NULL) break;
        if(rgn_largest->length < 2) break;
        
        /* find largest fragmented file which fragments can be joined there */
        do {
try_again:
            if(jp->termination_router((void *)jp)) goto done;
            f_largest = NULL, length = 0; tm = winx_xtime();
            for(f = jp->fragmented_files; f; f = f->next){
                file_length = f->f->disp.clusters;
                if(file_length > length && can_defragment(f->f,jp) && !is_mft(f->f,jp)){
                    f_largest = f;
                    length = file_length;
                }
                if(f->next == jp->fragmented_files) break;
            }
            if(f_largest == NULL){
                jp->p_counters.searching_time += winx_xtime() - tm;
                goto part_defrag_done;
            }
            if(is_file_locked(f_largest->f,jp)){
                jp->pi.processed_clusters += f_largest->f->disp.clusters;
                jp->p_counters.searching_time += winx_xtime() - tm;
                goto try_again;
            }
            jp->p_counters.searching_time += winx_xtime() - tm;
            
            /* find longest sequence of file blocks which fits in the current free region */
            longest_sequence = NULL, max_n_blocks = 0, longest_sequence_length = 0;
            for(first_block = f_largest->f->disp.blockmap; first_block; first_block = first_block->next){
                if(!is_block_excluded(first_block)){
                    n_blocks = 0, remaining_clusters = rgn_largest->length;
                    for(block = first_block; block; block = block->next){
                        if(jp->termination_router((void *)jp)) goto done;
                        if(is_block_excluded(block)) break;
                        if(block->length <= remaining_clusters){
                            n_blocks ++;
                            remaining_clusters -= block->length;
                        } else {
                            break;
                        }
                        if(block->next == f_largest->f->disp.blockmap) break;
                    }
                    if(n_blocks > 1 && n_blocks > max_n_blocks){
                        longest_sequence = first_block;
                        max_n_blocks = n_blocks;
                        longest_sequence_length = rgn_largest->length - remaining_clusters;
                    }
                }
                if(first_block->next == f_largest->f->disp.blockmap) break;
            }
            
            if(longest_sequence == NULL){
                /* fragments of the current file cannot be joined */
                f_largest->f->user_defined_flags |= UD_FILE_TOO_LARGE;
                /* exclude file from the current task */
                f_largest->f->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
            }
        } while(is_too_large(f_largest->f));

        /* join fragments */
        file = f_largest->f; /* f_largest may be destroyed by move_file */
        target = rgn_largest->lcn; 
        if(move_file(file,longest_sequence->vcn,longest_sequence_length,
          target,UD_MOVE_FILE_CUT_OFF_MOVED_CLUSTERS,jp) >= 0){
            if(jp->udo.dbgprint_level >= DBG_DETAILED)
                DebugPrint("Partial defrag success for %ws",file->path);
            defragmented_files ++;
        } else {
            DebugPrint("Partial defrag failure for %ws",file->path);
            /* exclude file from the current task */
            file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
        }
        
        /* remove target space from the free space pool anyway */
        jp->free_regions = winx_sub_volume_region(jp->free_regions,target,longest_sequence_length);
    }

part_defrag_done:
    /* mark all files not processed yet as too large */
    for(f = jp->fragmented_files; f; f = f->next){
        if(jp->termination_router((void *)jp)) break;
        if(can_defragment(f->f,jp) && !is_mft(f->f,jp))
            f->f->user_defined_flags |= UD_FILE_TOO_LARGE;
        if(f->next == jp->fragmented_files) break;
    }

done:
    /* display amount of moved data and number of partially defragmented files */
    DebugPrint("%I64u files partially defragmented",defragmented_files);
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    stop_timing("partial defragmentation",time,jp);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    return 0;
}

/**
 * @brief Auxiliary routine used to sort files in binary tree.
 */
static int files_compare(const void *prb_a, const void *prb_b, void *prb_param)
{
    winx_file_info *a, *b;
    
    a = (winx_file_info *)prb_a;
    b = (winx_file_info *)prb_b;
    
    if(a->disp.blockmap == NULL || b->disp.blockmap == NULL){
        DebugPrint("files_compare: unexpected condition");
        return 0;
    }
    
    if(a->disp.blockmap->lcn < b->disp.blockmap->lcn)
        return (-1);
    if(a->disp.blockmap->lcn == b->disp.blockmap->lcn)
        return 0;
    return 1;
}

/**
 * @brief move_files_to_front helper.
 */
static winx_file_info *find_largest_file(udefrag_job_parameters *jp,
        struct prb_table **pt,ULONGLONG min_lcn,int flags,int *files_found,
        ULONGLONG *length,ULONGLONG max_length)
{
    winx_file_info *file, *largest_file = NULL;
    struct prb_traverser t;
    winx_file_info f;
    winx_blockmap b;
    ULONGLONG file_length;
    ULONGLONG tm;
    
    tm = winx_xtime();
    if(*pt == NULL) goto slow_search;
    largest_file = NULL; *length = 0; *files_found = 0;
    b.lcn = min_lcn;
    f.disp.blockmap = &b;
    prb_t_init(&t,*pt);
    file = prb_t_insert(&t,*pt,&f);
    if(file == NULL){
        DebugPrint("find_largest_file: cannot insert file to the tree");
        prb_destroy(*pt,NULL);
        *pt = NULL;
        goto slow_search;
    }
    if(file == &f){
        file = prb_t_next(&t);
        if(prb_delete(*pt,&f) == NULL){
            DebugPrint("find_largest_file: cannot remove file from the tree");
            prb_destroy(*pt,NULL);
            *pt = NULL;
            goto slow_search;
        }
    }
    while(file){
        /*DebugPrint("XX: %ws",file->path);*/
        if(!can_move(file,jp) || is_mft(file,jp)){
        } else if((flags == MOVE_FRAGMENTED) && !is_fragmented(file)){
        } else if((flags == MOVE_NOT_FRAGMENTED) && is_fragmented(file)){
        } else {
            *files_found = 1;
            file_length = file->disp.clusters;
            if(file_length > *length \
              && file_length <= max_length){
                largest_file = file;
                *length = file_length;
                if(file_length == max_length) break;
            }
        }
        file = prb_t_next(&t);
    }
    goto done;

slow_search:
    largest_file = NULL; *length = 0; *files_found = 0;
    for(file = jp->filelist; file; file = file->next){
        if(can_move(file,jp) && !is_mft(file,jp)){
            if((flags == MOVE_FRAGMENTED) && !is_fragmented(file)){
            } else if((flags == MOVE_NOT_FRAGMENTED) && is_fragmented(file)){
            } else {
                if(file->disp.blockmap->lcn >= min_lcn){
                    *files_found = 1;
                    file_length = file->disp.clusters;
                    if(file_length > *length \
                      && file_length <= max_length){
                        largest_file = file;
                        *length = file_length;
                    }
                }
            }
        }
        if(file->next == jp->filelist) break;
    }

done:
    jp->p_counters.searching_time += winx_xtime() - tm;
    return largest_file;
}

/**
 * @brief Moves selected group of files
 * to the beginning of the volume to free its end.
 * @details This routine tries to move each file entirely
 * to avoid increasing fragmentation.
 * @param[in] jp job parameters.
 * @param[in] start_lcn the first cluster of an area intended 
 * to be cleaned up - all clusters before it should be skipped.
 * @param[in] flags one of MOVE_xxx flags defined in udefrag.h
 * @return Zero for success, negative value otherwise.
 */
int move_files_to_front(udefrag_job_parameters *jp, ULONGLONG start_lcn, int flags)
{
    ULONGLONG time;
    ULONGLONG moves, pass;
    winx_file_info *file,/* *last_file, */*largest_file;
    ULONGLONG length, rgn_size_threshold;
    int files_found;
    ULONGLONG rgn_lcn, min_file_lcn, min_rgn_lcn, max_rgn_lcn;
    /*ULONGLONG end_lcn, last_lcn;*/
    winx_volume_region *rgn, *r;
    ULONGLONG clusters_to_move;
    ULONGLONG file_lcn;
    winx_blockmap *block;
    ULONGLONG new_sp = 0;
    int completed;
    char buffer[32];

    struct prb_table *pt;
    
    jp->pi.current_operation = VOLUME_OPTIMIZATION;
    jp->pi.moved_clusters = 0;
    
    pt = prb_create(files_compare,NULL,NULL);
    if(pt == NULL){
        DebugPrint("move_files_to_front: cannot create files tree");
        DebugPrint("move_files_to_front: slow file search will be used");
    }
    
    /* no files are excluded by this task currently */
    for(file = jp->filelist; file; file = file->next){
        file->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
        if(pt != NULL){
            if(can_move(file,jp) && !is_mft(file,jp)){
                if(file->disp.blockmap->lcn >= start_lcn){
                    if(prb_probe(pt,(void *)file) == NULL){
                        DebugPrint("move_files_to_front: cannot add file to the tree");
                        prb_destroy(pt,NULL);
                        pt = NULL;
                    }
                }
            }
        }
        if(file->next == jp->filelist) break;
    }
    
    /*DebugPrint("start LCN = %I64u",start_lcn);
    file = prb_t_first(&t,pt);
    while(file){
        DebugPrint("file %ws at %I64u",file->path,file->disp.blockmap->lcn);
        file = prb_t_next(&t);
    }*/

    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return (-1);

    time = start_timing("file moving to front",jp);
    
    /* do the job */
    /* strategy 1: the most effective one */
    rgn_size_threshold = 1; pass = 0; jp->pi.total_moves = 0;
    while(!jp->termination_router((void *)jp)){
        moves = 0;
    
        /* free as much temporarily allocated space as possible */
        release_temp_space_regions(jp);
        
        min_rgn_lcn = 0; max_rgn_lcn = jp->v_info.total_clusters - 1;
        while(!jp->termination_router((void *)jp)){
            if(jp->udo.job_flags & UD_JOB_REPEAT)
                new_sp = calculate_starting_point(jp,start_lcn);
            rgn = NULL;
            for(r = jp->free_regions; r; r = r->next){
                if(r->lcn >= min_rgn_lcn && r->lcn <= max_rgn_lcn){
                    if(r->length > rgn_size_threshold){
                        rgn = r;
                        break;
                    }
                }
                if(r->next == jp->free_regions) break;
            }
            if(rgn == NULL) break;
            /* break if on the next pass the region will be moved to the end */
            if(jp->udo.job_flags & UD_JOB_REPEAT){
                if(rgn->lcn > new_sp){
                    completed = 1;
                    if(jp->job_type == QUICK_OPTIMIZATION_JOB){
                        if(get_number_of_fragmented_clusters(jp,new_sp,rgn->lcn) == 0)
                            completed = 0;
                    }
                    if(completed){
                        DebugPrint("rgn->lcn = %I64u, rgn->length = %I64u",rgn->lcn,rgn->length);
                        DebugPrint("move_files_to_front: heavily fragmented space begins at %I64u cluster",new_sp);
                        goto done;
                    }
                }
            }

            largest_file = NULL;
            do {
                if(largest_file){
                    /* count clusters of locked files */
                    jp->pi.processed_clusters += largest_file->disp.clusters;
                }
                largest_file = find_largest_file(jp,&pt,max(start_lcn, rgn->lcn + 1),flags,&files_found,&length,rgn->length);
                if(files_found == 0){
                    /* no more files can be moved on the current pass */
                    goto next_pass;
                }
                if(largest_file == NULL) break;
            } while(is_file_locked(largest_file,jp));

            if(largest_file == NULL){
                /* current free region is too small, let's try next one */
                rgn_size_threshold = rgn->length;
            } else {
                /* move the file */
                min_file_lcn = jp->v_info.total_clusters - 1;
                for(block = largest_file->disp.blockmap; block; block = block->next){
                    if(block->length && block->lcn < min_file_lcn)
                        min_file_lcn = block->lcn;
                    if(block->next == largest_file->disp.blockmap) break;
                }
                rgn_lcn = rgn->lcn;
                clusters_to_move = largest_file->disp.clusters;
                file_lcn = largest_file->disp.blockmap->lcn;
                /* move_file may destroy map of the file, so remove it from the tree */
                if(pt){
                    if(prb_delete(pt,largest_file) == NULL){
                        DebugPrint("move_files_to_front: cannot remove file from the tree");
                        prb_destroy(pt,NULL);
                        pt = NULL;
                    }
                }
                if(move_file(largest_file,largest_file->disp.blockmap->vcn,clusters_to_move,rgn->lcn,0,jp) >= 0){
                    moves ++;
                    jp->pi.total_moves ++;
                }
                /* regardless of result, exclude the file */
                largest_file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
                largest_file->user_defined_flags |= UD_FILE_MOVED_TO_FRONT;
                /* continue from the first free region after used one */
                min_rgn_lcn = rgn_lcn + 1;
                if(max_rgn_lcn > min_file_lcn - 1)
                    max_rgn_lcn = min_file_lcn - 1;
            }
        }

next_pass:    
        if(moves == 0) break;
        DebugPrint("move_files_to_front: pass %I64u completed, %I64u files moved",pass,moves);
        pass ++;
    }

done:
    /* display amount of moved data */
    DebugPrint("%I64u files moved totally",jp->pi.total_moves);
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    if(pt != NULL) prb_destroy(pt,NULL);
    stop_timing("file moving to front",time,jp);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    return 0;
}

/**
 * @brief Moves selected group of files
 * to the end of the volume to free its
 * beginning.
 * @brief This routine moves individual clusters
 * and never tries to keep the file not fragmented.
 * @param[in] jp job parameters.
 * @param[in] start_lcn the first cluster of an area intended 
 * to be cleaned up - all clusters before it should be skipped.
 * @param[in] flags one of MOVE_xxx flags defined in udefrag.h
 * @return Zero for success, negative value otherwise.
 */
int move_files_to_back(udefrag_job_parameters *jp, ULONGLONG start_lcn, int flags)
{
    ULONGLONG time;
    int nt4w2k_limitations = 0;
    winx_file_info *file, *first_file;
    winx_blockmap *first_block;
    ULONGLONG block_lcn, block_length, n;
    ULONGLONG current_vcn, remaining_clusters;
    winx_volume_region *rgn, *last_rgn;
    ULONGLONG clusters_to_move;
    char buffer[32];
    
    jp->pi.current_operation = VOLUME_OPTIMIZATION;
    jp->pi.moved_clusters = 0;
    jp->pi.total_moves = 0;
    
    /* free as much temporarily allocated space as possible */
    release_temp_space_regions(jp);

    /* no files are excluded by this task currently */
    for(file = jp->filelist; file; file = file->next){
        file->user_defined_flags &= ~UD_FILE_CURRENTLY_EXCLUDED;
        if(file->next == jp->filelist) break;
    }

    /* open the volume */
    jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
    if(jp->fVolume == NULL)
        return (-1);

    time = start_timing("file moving to end",jp);

    if(winx_get_os_version() <= WINDOWS_2K && jp->fs_type == FS_NTFS){
        DebugPrint("Windows NT 4.0 and Windows 2000 have stupid limitations in defrag API");
        DebugPrint("volume optimization is not so much effective on these systems");
        nt4w2k_limitations = 1;
    }
    
    /* do the job */
    while(!jp->termination_router((void *)jp)){
        /* find the first block after start_lcn */
        first_block = find_first_block(jp,&start_lcn,flags,1,&first_file);
        if(first_block == NULL) break;
        /* move the first block to the last free regions */
        block_lcn = first_block->lcn; block_length = first_block->length;
        if(!nt4w2k_limitations){
            /* sure, we can fill by the first block's data the last free regions */
            if(jp->free_regions == NULL) goto done;
            current_vcn = first_block->vcn;
            remaining_clusters = first_block->length;
            while(remaining_clusters){
                /* find last suitable free region */
                if(jp->free_regions == NULL) goto done;
                last_rgn = NULL;
                for(rgn = jp->free_regions->prev; rgn; rgn = rgn->prev){
                    if(rgn->length > 0){
                        last_rgn = rgn;
                        break;
                    }
                    if(rgn->prev == jp->free_regions->prev) break;
                }
                if(last_rgn == NULL) goto done;
                if(last_rgn->lcn < block_lcn + block_length){
                    /* no more moves to the end of disk are possible */
                    goto done;
                }
                n = min(last_rgn->length,remaining_clusters);
                if(move_file(first_file,current_vcn,n,last_rgn->lcn + last_rgn->length - n,0,jp) < 0){
                    /* exclude file from the current task to avoid infinite loops */
                    first_file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
                }
                current_vcn += n;
                remaining_clusters -= n;
                jp->pi.total_moves ++;
            }
        } else {
            /* it is safe to move entire files and entire blocks of compressed/sparse files */
            if(is_compressed(first_file) || is_sparse(first_file)){
                /* move entire block */
                current_vcn = first_block->vcn;
                clusters_to_move = first_block->length;
            } else {
                /* move entire file */
                current_vcn = first_file->disp.blockmap->vcn;
                clusters_to_move = first_file->disp.clusters;
            }
            rgn = find_matching_free_region(jp,first_block->lcn,clusters_to_move,FIND_MATCHING_RGN_FORWARD);
            //rgn = find_last_free_region(jp,clusters_to_move);
            if(rgn != NULL){
                if(rgn->lcn > first_block->lcn){
                    move_file(first_file,current_vcn,clusters_to_move,
                        rgn->lcn + rgn->length - clusters_to_move,0,jp);
                    jp->pi.total_moves ++;
                }
            }
            /* otherwise the volume processing is extremely slow and may even go to an infinite loop */
            first_file->user_defined_flags |= UD_FILE_CURRENTLY_EXCLUDED;
        }
    }
    
done:
    /* display amount of moved data */
    DebugPrint("%I64u clusters moved",jp->pi.moved_clusters);
    winx_bytes_to_hr(jp->pi.moved_clusters * jp->v_info.bytes_per_cluster,1,buffer,sizeof(buffer));
    DebugPrint("%s moved",buffer);
    stop_timing("file moving to end",time,jp);
    winx_fclose(jp->fVolume);
    jp->fVolume = NULL;
    return 0;
}

/** @} */
