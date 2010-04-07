/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* User mode driver interface header.
*/

#ifndef _UDEFRAG_KERNEL_H_
#define _UDEFRAG_KERNEL_H_

typedef enum {
	ANALYSE_JOB,
	DEFRAG_JOB,
	OPTIMIZE_JOB
} UDEFRAG_JOB_TYPE;

typedef enum {
	FREE_SPACE = 0,
	SYSTEM_SPACE,
	SYSTEM_OVERLIMIT_SPACE,
	FRAGM_SPACE,
	FRAGM_OVERLIMIT_SPACE,
	UNFRAGM_SPACE,
	UNFRAGM_OVERLIMIT_SPACE,
	MFT_ZONE_SPACE,
	MFT_SPACE,
	DIR_SPACE,
	DIR_OVERLIMIT_SPACE,
	COMPRESSED_SPACE,
	COMPRESSED_OVERLIMIT_SPACE,
	TEMPORARY_SYSTEM_SPACE
} VOLUME_SPACE_STATE;

#define UNKNOWN_SPACE FRAGM_SPACE
#define NUM_OF_SPACE_STATES (TEMPORARY_SYSTEM_SPACE - FREE_SPACE + 1)

#define DBG_NORMAL     0
#define DBG_DETAILED   1
#define DBG_PARANOID   2

typedef struct _STATISTIC {
	ULONG		filecounter;
	ULONG		dircounter;
	ULONG		compressedcounter;
	ULONG		fragmfilecounter;
	ULONG		fragmcounter;
	ULONGLONG	total_space;
	ULONGLONG	free_space; /* in bytes */
	ULONGLONG	mft_size;
	UCHAR		current_operation;
	ULONGLONG	clusters_to_process;
	ULONGLONG	processed_clusters;
	ULONG		pass_number; /* for volume optimizer */
} STATISTIC, *PSTATISTIC;

void __stdcall udefrag_kernel_native_init(void);
void __stdcall udefrag_kernel_native_unload(void);
int __stdcall udefrag_kernel_start(char *volume_name, UDEFRAG_JOB_TYPE job_type, int cluster_map_size);
int __stdcall udefrag_kernel_get_statistic(STATISTIC *stat, char *map, int map_size);
int __stdcall udefrag_kernel_stop(void);

#endif /* _UDEFRAG_KERNEL_H_ */
