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
 * @file optimize.c
 * @brief Volume optimization.
 * @addtogroup Optimizer
 * @{
 */

#include "udefrag-internals.h"

/**
 * @brief Searches for free space region starting at the beginning of the volume.
 * @param[in] start_cluster lcn to start searching from.
 * @param[in] min_length minimum length of free space in clusters.
 * @note In case of termination request returns NULL immediately.
 */
winx_volume_region *find_free_region_forward(udefrag_job_parameters *jp,
    ULONGLONG start_cluster, ULONGLONG min_length)
{
	winx_volume_region *rgn, *rgn_found;
	ULONGLONG length, clcn;
	
	rgn_found = NULL, length = 0, clcn = 0;
	for(rgn = jp->free_regions; rgn; rgn = rgn->next){
		if(jp->termination_router((void *)jp))
			return NULL;
		if(rgn->length >= min_length){
            if( length == 0 || rgn->length < length){
                rgn_found = rgn;
                length = rgn->length;
                clcn = rgn->lcn;
            }
		}
        if(rgn_found != NULL){
            if(clcn >= start_cluster){
                break;
            } else {
                if(length == min_length) break;
            }
        }
		if(rgn->next == jp->free_regions) break;
	}
	return rgn_found;
}

/**
 * @brief Searches for free space region starting at the end of the volume.
 * @param[in] start_cluster lcn to start searching from.
 * @param[in] min_length minimum length of free space in clusters.
 * @note In case of termination request returns NULL immediately.
 */
winx_volume_region *find_free_region_backward(udefrag_job_parameters *jp,
    ULONGLONG start_cluster, ULONGLONG min_length)
{
	winx_volume_region *rgn, *rgn_found;
	ULONGLONG length, clcn;
	
	rgn_found = NULL, length = 0, clcn = jp->v_info.total_clusters;
	for(rgn = jp->free_regions->prev; rgn; rgn = rgn->prev){
		if(jp->termination_router((void *)jp))
			return NULL;
		if(rgn->length >= min_length){
            if( length == 0 || rgn->length < length){
                rgn_found = rgn;
                length = rgn->length;
                clcn = rgn->lcn;
            }
		}
        if(rgn_found != NULL){
            if(clcn <= start_cluster){
                break;
            } else {
                if(length == min_length) break;
            }
        }
		if(rgn->prev == jp->free_regions->prev) break;
	}
	return rgn_found;
}

/************************************************************/
/*                    Full Optimization                     */
/************************************************************/

/**
 * @brief Performs a full volume optimization.
 * @details Full optimization moves all files
 * to the terminal part of the volume and then moves them
 * back to get as much continuous free space as possible.
 * This algorithm has been used in UltraDefrag 4.4 and
 * previous releases.
 * @return Zero for success, negative value otherwise.
 */
int full_optimize(udefrag_job_parameters *jp)
{
	return 0;
}


/************************************************************/
/*                   Quick Optimization                     */
/************************************************************/

/**
 * @brief Performs a quick volume optimization.
 * @details Quick optimization moves all files
 * from the terminal part of the volume to the beginning
 * to get as much continuous free space as possible.
 * This algorithm has been introduced in UltraDefrag 5.0.
 * @return Zero for success, negative value otherwise.
 */
int quick_optimize(udefrag_job_parameters *jp)
{
	return 0;
}

/** @} */
