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
 * @brief Performs a volume defragmentation.
 * @return Zero for success, negative value otherwise.
 */
int defragment(udefrag_job_parameters *jp)
{
	udefrag_fragmented_file *f;
	int result;
	
	/* analyze volume */
	result = analyze(jp);
	if(result < 0)
		return result;
	
	/* reset statistics */
	jp->pi.clusters_to_process = 0;
	jp->pi.processed_clusters = 0;
	jp->pi.moved_clusters = 0;
	jp->pi.current_operation = VOLUME_DEFRAGMENTATION;
	for(f = jp->fragmented_files; f; f = f->next){
		if(jp->termination_router((void *)jp)) return 0;
		/*
		* Count all fragmented files which 
		* can be processed at least partially.
		*/
		if(can_defragment_partially(f->f,jp))
			jp->pi.clusters_to_process += f->f->disp.clusters;
		if(f->next == jp->fragmented_files) break;
	}
	
	/* open the volume */
	// fVolume = new_winx_vopen(winx_toupper(jp->volume_letter));
	jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
	if(jp->fVolume == NULL)
		return (-1);
	
	if(jp->udo.preview_mask & 4)
        defragment_small_files(jp);
    else
        defragment_small_files_respect_best_matching(jp);

	if(jp->termination_router((void *)jp) == 0)
		defragment_big_files(jp);
	
	redraw_all_temporary_system_space_as_free(jp);
	winx_fclose(jp->fVolume);
	jp->fVolume = NULL;
	return 0;
}

/**
 * @brief Performs a volume defragmentation.
 * @return Zero for success, negative value otherwise.
 */
int defragment_ex(udefrag_job_parameters *jp)
{
	udefrag_fragmented_file *f;
	
	/* reset statistics */
	jp->pi.clusters_to_process = 0;
	jp->pi.processed_clusters = 0;
	jp->pi.moved_clusters = 0;
	jp->pi.current_operation = VOLUME_DEFRAGMENTATION;
	for(f = jp->fragmented_files; f; f = f->next){
		if(jp->termination_router((void *)jp)) return 0;
		/*
		* Count all fragmented files which 
		* can be processed at least partially.
		*/
		if(can_defragment_partially(f->f,jp))
			jp->pi.clusters_to_process += f->f->disp.clusters;
		if(f->next == jp->fragmented_files) break;
	}
	
	/* open the volume */
	// fVolume = new_winx_vopen(winx_toupper(jp->volume_letter));
	jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
	if(jp->fVolume == NULL)
		return (-1);
	
	if(jp->udo.preview_mask & 4)
        defragment_small_files(jp);
    else
        defragment_small_files_respect_best_matching(jp);

	redraw_all_temporary_system_space_as_free(jp);
	winx_fclose(jp->fVolume);
	jp->fVolume = NULL;
	return 0;
}

/**
 * @brief Performs a partial volume defragmentation.
 * @return Zero for success, negative value otherwise.
 */
int defragment_partial(udefrag_job_parameters *jp)
{
	udefrag_fragmented_file *f;
	
	/* reset statistics */
	jp->pi.clusters_to_process = 0;
	jp->pi.processed_clusters = 0;
	jp->pi.moved_clusters = 0;
	jp->pi.current_operation = VOLUME_DEFRAGMENTATION;
	for(f = jp->fragmented_files; f; f = f->next){
		if(jp->termination_router((void *)jp)) return 0;
		/*
		* Count all fragmented files which 
		* can be processed at least partially.
		*/
		if(can_defragment_partially(f->f,jp))
			jp->pi.clusters_to_process += f->f->disp.clusters;
		if(f->next == jp->fragmented_files) break;
	}
	
	/* open the volume */
	// fVolume = new_winx_vopen(winx_toupper(jp->volume_letter));
	jp->fVolume = winx_vopen(winx_toupper(jp->volume_letter));
	if(jp->fVolume == NULL)
		return (-1);
	
	defragment_big_files(jp);
	
	redraw_all_temporary_system_space_as_free(jp);
	winx_fclose(jp->fVolume);
	jp->fVolume = NULL;
	return 0;
}

/** @} */
