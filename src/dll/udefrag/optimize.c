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

#if 0
/************************************************************/
/*                    Full Optimization                     */
/************************************************************/

/**
 * @brief Performs a full volume optimization.
 * @details Full optimization moves all files
 * to the end of the volume and then moves them
 * back to get as much continuous free space as possible.
 * This algorithm has been used in UltraDefrag 4.4 and
 * previous releases.
 * @return Zero for success, negative value otherwise.
 */
int full_optimize(udefrag_job_parameters *jp)
{
    /*
        1) find first free region
        2) find first fragmented file
        3) if lcn.first.free.region > lcn.first.fragmented.file
               lcn.start.cluster = lcn.first.fragmented.file
           else
               if lcn.last.processed.cluster = 0
                   lcn.start.cluster = lcn.first.free.region.end
               else
                   lcn.start.cluster = lcn.last.processed.cluster
        4) move all clusters after lcn.start.cluster to the end of the volume
    */
	return 0;
}


/************************************************************/
/*                   Quick Optimization                     */
/************************************************************/

/**
 * @brief Performs a quick volume optimization.
 * @details Quick optimization moves all files
 * from the end of the volume to the beginning
 * to get as much continuous free space as possible.
 * This algorithm has been introduced in UltraDefrag 5.0.
 * @return Zero for success, negative value otherwise.
 */
int quick_optimize(udefrag_job_parameters *jp)
{
    /*
        1) find middle of the volume
        2) move all fragmented clusters from the first half to the end
           of the volume
        3) update cluster map (free temporary space)
        4) move all not fragmented files from the second half to the
           beginning of the volume
        5) if no file has been moved, move not fragmented files from
           the end of the first half to the beginning of the volume
    */
	return 0;
}
#endif

/** @} */
