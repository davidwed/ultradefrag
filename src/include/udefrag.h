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

/*
* Udefrag.dll interface header.
*/

#ifndef _UDEFRAG_H_
#define _UDEFRAG_H_

/* debug print levels */
#define DBG_NORMAL     0
#define DBG_DETAILED   1
#define DBG_PARANOID   2

/* UltraDefrag error codes */
#define UDEFRAG_UNKNOWN_ERROR     (-1)
#define UDEFRAG_FAT_OPTIMIZATION  (-2)
#define UDEFRAG_W2K_4KB_CLUSTERS  (-3)
#define UDEFRAG_NO_MEM            (-4)
#define UDEFRAG_CDROM             (-5)
#define UDEFRAG_REMOTE            (-6)
#define UDEFRAG_ASSIGNED_BY_SUBST (-7)
#define UDEFRAG_REMOVABLE         (-8)
#define UDEFRAG_UDF_DEFRAG        (-9)

#define DEFAULT_REFRESH_INTERVAL 100

#define MAX_DOS_DRIVES 26
#define MAXFSNAME      32  /* I think, that's enough */

typedef struct _volume_info {
	char letter;
	char fsname[MAXFSNAME];
	wchar_t label[MAX_PATH + 1];
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
int __stdcall udefrag_get_volume_information(char volume_letter,volume_info *v);

int __stdcall udefrag_fbsize(ULONGLONG number, int digits, char *buffer, int length);
int __stdcall udefrag_dfbsize(char *string,ULONGLONG *pnumber);

typedef enum {
	ANALYSIS_JOB = 0,
	DEFRAGMENTATION_JOB,
	FULL_OPTIMIZATION_JOB,
	QUICK_OPTIMIZATION_JOB
} udefrag_job_type;

typedef enum {
	VOLUME_ANALYSIS = 0,
	VOLUME_DEFRAGMENTATION,
	VOLUME_OPTIMIZATION
} udefrag_operation_type;

/* flags triggering algorithm features on preview stage */
#define UD_PREVIEW_REPEAT    0x1
#define UD_PREVIEW_MATCHING  0x2
#define UD_PREVIEW_QUICK     0x4

enum {
    UNUSED_MAP_SPACE = 0,
	FREE_SPACE,
	SYSTEM_SPACE,
	SYSTEM_OVER_LIMIT_SPACE,
	FRAGM_SPACE,
	FRAGM_OVER_LIMIT_SPACE,
	UNFRAGM_SPACE,
	UNFRAGM_OVER_LIMIT_SPACE,
	MFT_ZONE_SPACE,           /* after free! */
	MFT_SPACE,                /* after mft zone! */
	DIR_SPACE,
	DIR_OVER_LIMIT_SPACE,
	COMPRESSED_SPACE,
	COMPRESSED_OVER_LIMIT_SPACE,
	TEMPORARY_SYSTEM_SPACE,
    NUM_OF_SPACE_STATES          /* this must always be the last */
};

#define UNKNOWN_SPACE FRAGM_SPACE

typedef struct _udefrag_progress_info {
	unsigned long files;              /* number of files */
	unsigned long directories;        /* number of directories */
	unsigned long compressed;         /* number of compressed files */
	unsigned long fragmented;         /* number of fragmented files */
	ULONGLONG fragments;              /* number of fragments */
	ULONGLONG total_space;            /* volume size, in bytes */
	ULONGLONG free_space;             /* free space amount, in bytes */
	ULONGLONG mft_size;               /* mft size, in bytes */
	udefrag_operation_type current_operation;  /* identifies currently running operation */
	unsigned long pass_number;        /* the current volume optimizer pass */
	ULONGLONG clusters_to_process;    /* number of clusters to process */
	ULONGLONG processed_clusters;     /* number of already processed clusters */
	double percentage;                /* used to deliver a job completion percentage to the caller */
	int completion_status;            /* zero for running job, positive value for succeeded, negative for failed */
	char *cluster_map;                /* pointer to the cluster map buffer */
	int cluster_map_size;             /* size of the cluster map buffer, in bytes */
	ULONGLONG moved_clusters;         /* number of moved clusters */
} udefrag_progress_info;

typedef void  (__stdcall *udefrag_progress_callback)(udefrag_progress_info *pi, void *p);
typedef int   (__stdcall *udefrag_terminator)(void *p);

int __stdcall udefrag_start_job(char volume_letter,udefrag_job_type job_type,int preview_flags,
		int cluster_map_size,udefrag_progress_callback cb,udefrag_terminator t,void *p);

char * __stdcall udefrag_get_default_formatted_results(udefrag_progress_info *pi);
void __stdcall udefrag_release_default_formatted_results(char *results);

char * __stdcall udefrag_get_error_description(int error_code);

/* reliable _toupper and _tolower analogs */
char __cdecl udefrag_toupper(char c);
char __cdecl udefrag_tolower(char c);

int __stdcall udefrag_set_log_file_path(void);

#endif /* _UDEFRAG_H_ */
