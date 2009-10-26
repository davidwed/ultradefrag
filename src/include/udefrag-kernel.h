/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

#if defined(__POCC__)
#pragma ftol(inlined)
#endif

typedef enum {
	ANALYSE_JOB,
	DEFRAG_JOB,
	OPTIMIZE_JOB
} UDEFRAG_JOB_TYPE;

int __stdcall udefrag_kernel_start(char *volume_name, UDEFRAG_JOB_TYPE job_type, int cluster_map_size);
int __stdcall udefrag_kernel_get_statistic(STATISTIC *stat, char *map, int map_size);
int __stdcall udefrag_kernel_stop(void);

#endif /* _UDEFRAG_KERNEL_H_ */
