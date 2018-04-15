/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2017 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @if INTERNAL
 * @file search.c
 * @brief File blocks and free space regions searching.
 * @addtogroup Search
 * @{
 */

/*
* Ideas by Dmitri Arkhangelski <dmitriar@gmail.com>
* and Stefan Pendl <stefanpe@users.sourceforge.net>.
*/

#include "udefrag-internals.h"

/************************************************************/
/*               Free space regions searching               */
/************************************************************/

/**
 * @internal
 * @brief Searches for the first suitable free space region.
 * @param[in] jp the job parameters.
 * @param[in] min_lcn the minimum LCN of the region.
 * @param[in] min_length the minimum length of the region, in clusters.
 * @param[out] max_length length of the biggest region found.
 * @note In case of termination request returns NULL immediately.
 */
winx_volume_region *find_first_free_region(udefrag_job_parameters *jp,
        ULONGLONG min_lcn,ULONGLONG min_length,ULONGLONG *max_length)
{
    winx_volume_region *rgn;
    ULONGLONG time = winx_xtime();

    if(max_length) *max_length = 0;
    for(rgn = jp->free_regions; rgn; rgn = rgn->next){
        if(jp->termination_router((void *)jp)) break;
        if(rgn->lcn >= min_lcn){
            if(max_length){
                if(rgn->length > *max_length)
                    *max_length = rgn->length;
            }
            if(rgn->length >= min_length){
                jp->p_counters.searching_time += winx_xtime() - time;
                return rgn;
            }
        }
        if(rgn->next == jp->free_regions) break;
    }
    jp->p_counters.searching_time += winx_xtime() - time;
    return NULL;
}

/**
 * @internal
 * @brief Searches for the last suitable free space region.
 * @param[in] jp the job parameters.
 * @param[in] min_lcn the minimum LCN of the region.
 * @param[in] min_length the minimum length of the region, in clusters.
 * @param[out] max_length length of the biggest region found.
 * @note In case of termination request returns NULL immediately.
 */
winx_volume_region *find_last_free_region(udefrag_job_parameters *jp,
        ULONGLONG min_lcn,ULONGLONG min_length,ULONGLONG *max_length)
{
    winx_volume_region *rgn;
    ULONGLONG time = winx_xtime();

    if(max_length) *max_length = 0;
    if(jp->free_regions){
        for(rgn = jp->free_regions->prev; rgn; rgn = rgn->prev){
            if(jp->termination_router((void *)jp)) break;
            if(rgn->lcn < min_lcn) break;
            if(max_length){
                if(rgn->length > *max_length)
                    *max_length = rgn->length;
            }
            if(rgn->length >= min_length){
                jp->p_counters.searching_time += winx_xtime() - time;
                return rgn;
            }
            if(rgn->prev == jp->free_regions->prev) break;
        }
    }
    jp->p_counters.searching_time += winx_xtime() - time;
    return NULL;
}

/**
 * @internal
 * @brief Searches for the largest free space region.
 * @note In case of termination request returns NULL
 * immediately.
 */
winx_volume_region *find_largest_free_region(udefrag_job_parameters *jp)
{
    winx_volume_region *rgn, *rgn_largest;
    ULONGLONG length;
    ULONGLONG time = winx_xtime();
    
    rgn_largest = NULL, length = 0;
    for(rgn = jp->free_regions; rgn; rgn = rgn->next){
        if(jp->termination_router((void *)jp)){
            jp->p_counters.searching_time += winx_xtime() - time;
            return NULL;
        }
        if(rgn->length > length){
            rgn_largest = rgn;
            length = rgn->length;
        }
        if(rgn->next == jp->free_regions) break;
    }
    jp->p_counters.searching_time += winx_xtime() - time;
    return rgn_largest;
}

/************************************************************/
/*                    Auxiliary routines                    */
/************************************************************/

/**
 * @internal
 * @brief An auxiliary structure used to save file blocks in a binary tree.
 */
struct file_block {
    winx_file_info *file;
    winx_blockmap *block;
};

/**
 * @internal
 * @brief An auxiliary routine used to sort file blocks in the binary tree.
 */
static int blocks_compare(const void *prb_a, const void *prb_b, void *prb_param)
{
    struct file_block *a, *b;
    
    a = (struct file_block *)prb_a;
    b = (struct file_block *)prb_b;
    
    if(a->block->lcn < b->block->lcn)
        return (-1);
    if(a->block->lcn == b->block->lcn)
        return 0;
    return 1;
}

/**
 * @internal
 * @brief An auxiliary routine used to free memory allocated for the tree items.
 */
static void free_item (void *prb_item, void *prb_param)
{
    struct file_block *item = (struct file_block *)prb_item;
    winx_free(item);
}

/**
 * @internal
 * @brief Creates and initializes a binary
 * tree to store all the file blocks into.
 * @return Zero for success, a negative value otherwise.
 * @note jp->file_blocks must be initialized by NULL
 * or point to a valid tree before this call.
 */
int create_file_blocks_tree(udefrag_job_parameters *jp)
{
    itrace("create_file_blocks_tree called");
    if(jp->file_blocks) destroy_file_blocks_tree(jp);
    jp->file_blocks = prb_create(blocks_compare,NULL,NULL);
    return 0;
}

/**
 * @internal
 * @brief Adds a file block to the binary tree.
 * @return Zero for success, a negative value otherwise.
 * @note Destroys the tree in case of errors.
 */
int add_block_to_file_blocks_tree(udefrag_job_parameters *jp, winx_file_info *file, winx_blockmap *block)
{
    struct file_block *fb;
    void **p;
    
    if(file == NULL || block == NULL)
        return (-1);
    
    if(jp->file_blocks == NULL)
        return (-1);

    fb = winx_malloc(sizeof *fb);
    fb->file = file;
    fb->block = block;
    p = prb_probe(jp->file_blocks,(void *)fb);
    /* if a duplicate item exists... */
    if(p && *p != fb){
        etrace("a duplicate found");
        winx_free(fb);
    }
    return 0;
}

/**
 * @internal
 * @brief Removes a file block from the binary tree.
 * @return Zero for success, a negative value otherwise.
 */
int remove_block_from_file_blocks_tree(udefrag_job_parameters *jp, winx_blockmap *block)
{
    struct file_block *fb;
    struct file_block b;
    
    if(block == NULL)
        return (-1);
    
    if(jp->file_blocks == NULL)
        return (-1);

    b.file = NULL;
    b.block = block;
    fb = prb_delete(jp->file_blocks,&b);
    if(fb == NULL){
        /* the following debugging output indicates either
           a bug, or file system inconsistency */
        etrace("failed for %p: VCN = %I64u, LCN = %I64u, LEN = %I64u",
            block, block->vcn, block->lcn, block->length);
        /* if block does not exist in the tree, we have nothing to cleanup */
        return 0;
    }
    winx_free(fb);
    return 0;
}

/**
 * @internal
 * @brief Destroys the binary tree containing all the file blocks.
 */
void destroy_file_blocks_tree(udefrag_job_parameters *jp)
{
    itrace("destroy_file_blocks_tree called");
    if(jp->file_blocks){
        prb_destroy(jp->file_blocks,free_item);
        jp->file_blocks = NULL;
    }
}

/************************************************************/
/*                  File blocks searching                   */
/************************************************************/

/**
 * @internal
 * @brief Searches for the first movable file block.
 * @param[in] jp the job parameters.
 * @param[in,out] min_lcn the minimum LCN accepted.
 * @param[in] flags one of the SKIP_xxx
 * flags defined in udefrag.h
 * @param[out] first_file pointer to
 * variable receiving information about
 * the file the found block belongs to.
 * @note In case of termination request
 * returns NULL immediately.
 */
winx_blockmap *find_first_block(udefrag_job_parameters *jp,
    ULONGLONG *min_lcn, int flags, winx_file_info **first_file)
{
    winx_file_info *found_file;
    winx_blockmap *first_block;
    winx_blockmap b;
    struct file_block fb, *item;
    struct prb_traverser t;
    int movable_file;
    ULONGLONG tm = winx_xtime();
    
    if(min_lcn == NULL || first_file == NULL)
        return NULL;
    
    found_file = NULL; first_block = NULL;
    b.lcn = *min_lcn; fb.block = &b;
    prb_t_init(&t,jp->file_blocks);
    item = prb_t_insert(&t,jp->file_blocks,&fb);
    if(item == &fb){
        /* block at min_lcn not found */
        item = prb_t_next(&t);
        if(prb_delete(jp->file_blocks,&fb) == NULL){
            etrace("cannot remove block from the tree");
            winx_flush_dbg_log(0); /* 'cause the error is critical */
        }
    }
    if(item){
        found_file = item->file;
        first_block = item->block;
    }
    while(!jp->termination_router((void *)jp)){
        if(found_file == NULL) break;
        if(flags & SKIP_PARTIALLY_MOVABLE_FILES){
            movable_file = can_move_entirely(found_file,jp);
        } else {
            movable_file = can_move(found_file,jp);
        }
        if(is_file_locked(found_file,jp)) movable_file = 0;
        if(movable_file){
            if(jp->is_fat && is_directory(found_file) && first_block == found_file->disp.blockmap){
                /* skip first fragments of FAT directories */
            } else {
                /* desired block found */
                *min_lcn = first_block->lcn + 1; /* the current block will be skipped later anyway in this case */
                *first_file = found_file;
                jp->p_counters.searching_time += winx_xtime() - tm;
                return first_block;
            }
        }
        
        /* skip the current block */
        *min_lcn = *min_lcn + 1;
        /* and go to the next one */
        item = prb_t_next(&t);
        if(item == NULL) break;
        found_file = item->file;
        first_block = item->block;
    }
    *first_file = NULL;
    jp->p_counters.searching_time += winx_xtime() - tm;
    return NULL;
}

/**
 * @}
 * @endif
 */
