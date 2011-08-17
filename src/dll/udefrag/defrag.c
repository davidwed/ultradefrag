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
 * @file defrag.c
 * @brief Volume defragmentation.
 * @addtogroup Defrag
 * @{
 */

#include "udefrag-internals.h"

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
			n += get_file_length(jp,f->f);
		if(f->next == jp->fragmented_files) break;
	}
	return n;
}

/**
 * @brief Performs a volume defragmentation.
 * @return Zero for success, negative value otherwise.
 */
int defragment(udefrag_job_parameters *jp)
{
	disk_processing_routine defrag_routine = NULL;
	int result, overall_result = -1;
	
	/* perform volume analysis */
	result = analyze(jp); /* we need to call it once, here */
	if(result < 0) return result;
	
	/* choose defragmentation strategy */
	if(jp->pi.fragmented >= jp->free_regions_count || \
       jp->udo.job_flags & UD_PREVIEW_MATCHING){
        DebugPrint("defragment: walking fragmented files list");
		defrag_routine = defragment_small_files_walk_fragmented_files;
	} else {
        DebugPrint("defragment: walking free regions list");
        defrag_routine = defragment_small_files_walk_free_regions;
    }
	
	/* do the job */
	jp->pi.pass_number = 0;
	while(!jp->termination_router((void *)jp)){
		/* reset counters */
		jp->pi.processed_clusters = 0;
		jp->pi.clusters_to_process = fragmented_clusters(jp);
		
		result = defrag_routine(jp);
		if(result == 0){
			/* defragmentation succeeded at least once */
			overall_result = 0;
		}
		
		/* break if nothing moved */
		if(result < 0 || jp->pi.moved_clusters == 0) break;
		
		/* break if no repeat */
		if(!(jp->udo.job_flags & UD_JOB_REPEAT)) break;
		
		/* go to the next pass */
		jp->pi.pass_number ++;
	}
	
	if(jp->termination_router((void *)jp)) return 0;
	
	/* perform partial defragmentation */
	jp->pi.processed_clusters = 0;
	jp->pi.clusters_to_process = fragmented_clusters(jp);
	result = defragment_big_files(jp);
	if(result == 0){
		/* at least partial defragmentation succeeded */
		overall_result = 0;
	}

	if(jp->termination_router((void *)jp)) return 0;
	
	return overall_result;
}

/** @} */
