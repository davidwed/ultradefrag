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

#if defined(__POCC__)
#pragma ftol(inlined)
#endif

/* debug print levels */
#define DBG_NORMAL     0
#define DBG_DETAILED   1
#define DBG_PARANOID   2

/* UltraDefrag error codes */
#define UDEFRAG_UNKNOWN_ERROR     (-1)
#define UDEFRAG_W2K_4KB_CLUSTERS  (-3)
#define UDEFRAG_NO_MEM            (-4)
#define UDEFRAG_CDROM             (-5)
#define UDEFRAG_REMOTE            (-6)
#define UDEFRAG_ASSIGNED_BY_SUBST (-7)
#define UDEFRAG_REMOVABLE         (-8)

#define DEFAULT_REFRESH_INTERVAL 100

#define MAX_DOS_DRIVES 26
#define MAXFSNAME      32  /* I think, that's enough */

typedef struct _volume_info {
	char letter;
	char fsname[MAXFSNAME];
	LARGE_INTEGER total_space;
	LARGE_INTEGER free_space;
	int is_removable;
} volume_info;

/**
 * @brief Initializes all libraries required 
 * for the native application.
 * @note Designed especially to replace DllMain
 * functionality in case of monolithic native application.
 * Call this routine in the beginning of NtProcessStartup() code.
 */
#define udefrag_monolithic_native_app_init() zenwinx_native_init()

/**
 * @brief Frees resources of all libraries required 
 * for the native application.
 * @note Designed especially to replace DllMain
 * functionality in case of monolithic native application.
 * Don't call it before winx_shutdown() and winx_reboot(),
 * but call always before winx_exit().
 */
#define udefrag_monolithic_native_app_unload() zenwinx_native_unload()

volume_info * __stdcall udefrag_get_vollist(int skip_removable);
void __stdcall udefrag_release_vollist(volume_info *v);
int __stdcall udefrag_validate_volume(char volume_letter,int skip_removable);

int __stdcall udefrag_fbsize(ULONGLONG number, int digits, char *buffer, int length);
int __stdcall udefrag_dfbsize(char *string,ULONGLONG *pnumber);

typedef enum {
	ANALYSIS_JOB = 0,
	DEFRAG_JOB,
	OPTIMIZER_JOB
} udefrag_job_type;

enum {
	FREE_SPACE = 0,
	SYSTEM_SPACE,
	SYSTEM_OVER_LIMIT_SPACE,
	FRAGM_SPACE,
	FRAGM_OVER_LIMIT_SPACE,
	UNFRAGM_SPACE,
	UNFRAGM_OVER_LIMIT_SPACE,
	MFT_ZONE_SPACE, /* after free! */
	MFT_SPACE, /* after mft zone! */
	DIR_SPACE,
	DIR_OVER_LIMIT_SPACE,
	COMPRESSED_SPACE,
	COMPRESSED_OVER_LIMIT_SPACE,
	TEMPORARY_SYSTEM_SPACE
};

#define UNKNOWN_SPACE FRAGM_SPACE
#define NUM_OF_SPACE_STATES (TEMPORARY_SYSTEM_SPACE - FREE_SPACE + 1)

typedef struct _udefrag_progress_info {
	unsigned long files;              /* number of files */
	unsigned long directories;        /* number of directories */
	unsigned long compressed;         /* number of compressed files */
	unsigned long fragmented;         /* number of fragmented files */
	unsigned long fragments;          /* number of fragments */
	ULONGLONG total_space;            /* volume size, in bytes */
	ULONGLONG free_space;             /* free space amount, in bytes */
	ULONGLONG mft_size;               /* mft size, in bytes */
	unsigned char current_operation;  /* identifier of the currently running job */
	unsigned long pass_number;        /* the current volume optimizer pass */
	ULONGLONG clusters_to_process;    /* number of clusters to process */
	ULONGLONG processed_clusters;     /* number of already processed clusters */
	double percentage;                /* used to deliver a job completion percentage to the caller */
	int completion_status;            /* zero for running job, positive value for succeeded, negative for failed */
	char *cluster_map;                /* pointer to the cluster map buffer */
	int cluster_map_size;             /* size of the cluster map buffer, in bytes */
} udefrag_progress_info;

typedef void  (__stdcall *udefrag_progress_callback)(udefrag_progress_info *pi, void *p);
typedef int   (__stdcall *udefrag_terminator)(void *p);

int __stdcall udefrag_start_job(char volume_letter,udefrag_job_type job_type,
		int cluster_map_size,udefrag_progress_callback cb,udefrag_terminator t,void *p);

char * __stdcall udefrag_get_default_formatted_results(udefrag_progress_info *pi);
void __stdcall udefrag_release_default_formatted_results(char *results);

char * __stdcall udefrag_get_error_description(int error_code);

/* reliable _toupper and _tolower analogs */
char __cdecl udefrag_toupper(char c);
char __cdecl udefrag_tolower(char c);

#endif /* _UDEFRAG_H_ */
