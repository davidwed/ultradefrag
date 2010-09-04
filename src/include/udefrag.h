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

/*
* Udefrag.dll interface header.
*/

#ifndef _UDEFRAG_H_
#define _UDEFRAG_H_

#include "udefrag-kernel.h"

#if defined(__POCC__)
#pragma ftol(inlined)
#endif

/* UltraDefrag error codes */
#define UDEFRAG_UNKNOWN_ERROR     (-1)
#define UDEFRAG_ALREADY_RUNNING   (-2)
#define UDEFRAG_W2K_4KB_CLUSTERS  (-3)
#define UDEFRAG_NO_MEM            (-4)
#define UDEFRAG_CDROM             (-5)
#define UDEFRAG_REMOTE            (-6)
#define UDEFRAG_ASSIGNED_BY_SUBST (-7)
#define UDEFRAG_REMOVABLE         (-8)

#define DEFAULT_REFRESH_INTERVAL 100

#define MAX_DOS_DRIVES 26
#define MAXFSNAME      32  /* I think that's enough. */

typedef struct _volume_info {
	char letter;
	char fsname[MAXFSNAME];
	LARGE_INTEGER total_space;
	LARGE_INTEGER free_space;
	int is_removable;
} volume_info;

void __stdcall udefrag_monolithic_native_app_init(void);
void __stdcall udefrag_monolithic_native_app_unload(void);

int __stdcall udefrag_init(void);
int __stdcall udefrag_unload(void);

/*
* This callback procedure was designed especially 
* to refresh progress indicator during the defragmentation process.
*/
typedef int (__stdcall *STATUPDATEPROC)(int done_flag);
int __stdcall udefrag_start(char *volume_name, UDEFRAG_JOB_TYPE job_type, int cluster_map_size, STATUPDATEPROC sproc);
int __stdcall udefrag_stop(void);
int __stdcall udefrag_get_progress(STATISTIC *pstat, double *percentage);
int __stdcall udefrag_get_map(char *buffer,int size);
char *  __stdcall udefrag_get_default_formatted_results(STATISTIC *pstat);
void __stdcall udefrag_release_default_formatted_results(char *results);
char * __stdcall udefrag_get_error_description(int error_code);

int __stdcall udefrag_get_avail_volumes(volume_info **vol_info,int skip_removable);
int __stdcall udefrag_validate_volume(unsigned char letter,int skip_removable);

int __stdcall udefrag_fbsize(ULONGLONG number, int digits, char *buffer, int length);
int __stdcall udefrag_dfbsize(char *string,ULONGLONG *pnumber);

#endif /* _UDEFRAG_H_ */
