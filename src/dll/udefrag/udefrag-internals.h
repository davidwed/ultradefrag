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

#ifndef _UDEFRAG_INTERNALS_H_
#define _UDEFRAG_INTERNALS_H_

#include "../../include/ntndk.h"
#include "../../include/udefrag.h"
#include "../zenwinx/zenwinx.h"
#include "../../include/ultradfgver.h"

#ifndef DebugPrint
#define DebugPrint winx_dbg_print
#endif

/* flags for user_defined_flags member of filelist entries */
#define UD_FILE_EXCLUDED       0x1
#define UD_FILE_OVER_LIMIT     0x2

/* file status flags */
#define UD_FILE_LOCKED           0x4   /* file is locked by system */
#define UD_FILE_TOO_LARGE        0x8   /* file is larger than the biggest free space region */
#define UD_FILE_MOVING_FAILED    0x10  /* file moving completed with failure */
#define UD_FILE_IMPROPER_STATE   0x20  /* file is in improper state (chkdsk run needed) or some bug encountered */

/* this flag is used to skip files already processed with errors, */
/* it is intended for use in individual volume processing tasks */
#define UD_FILE_CURRENTLY_EXCLUDED	 0x40

#define is_excluded(f)           ((f)->user_defined_flags & UD_FILE_EXCLUDED)
#define is_over_limit(f)         ((f)->user_defined_flags & UD_FILE_OVER_LIMIT)

#define is_locked(f)             ((f)->user_defined_flags & UD_FILE_LOCKED)
#define is_too_large(f)          ((f)->user_defined_flags & UD_FILE_TOO_LARGE)
#define is_moving_failed(f)      ((f)->user_defined_flags & UD_FILE_MOVING_FAILED)
#define is_in_improper_state(f)  ((f)->user_defined_flags & UD_FILE_IMPROPER_STATE)

#define is_currently_excluded(f) ((f)->user_defined_flags & UD_FILE_CURRENTLY_EXCLUDED)

#define is_block_excluded(b)  ((b)->length == 0)

/* named constant for 256k */
#define _256K (256 * 1024)

/*
* MSDN states that environment variables
* are limited by 32767 characters,
* including terminal zero.
*/
#define ENV_BUFFER_SIZE 32767

typedef struct _udefrag_options {
	winx_patlist in_filter;     /* patterns for file inclusion */
	winx_patlist ex_filter;     /* patterns for file exclusion */
	ULONGLONG size_limit;       /* file size threshold */
	ULONGLONG fragments_limit;  /* file fragments threshold */
	ULONGLONG time_limit;       /* processing time limit, in seconds */
	int refresh_interval;       /* progress refresh interval, in milliseconds */
	int disable_reports;        /* nonzero value forces fragmentation reports to be disabled */
	int dbgprint_level;         /* controls amount of debugging information */
	int dry_run;                /* set %UD_DRY_RUN% variable to avoid actual data moving in tests */
	int preview_flags;          /* flags triggering algorithm features on preview stage */
} udefrag_options;

typedef struct _udefrag_fragmented_file {
	struct _udefrag_fragmented_file *next;
	struct _udefrag_fragmented_file *prev;
	winx_file_info *f;
} udefrag_fragmented_file;

struct _mft_zones {
	ULONGLONG mft_start;
	ULONGLONG mft_end;
	ULONGLONG mftzone_start;
	ULONGLONG mftzone_end;
	ULONGLONG mftmirr_start;
	ULONGLONG mftmirr_end;
};

typedef enum {
	FS_UNKNOWN = 0, /* including UDF and Ext2 */
	FS_FAT12,
	FS_FAT16,
	FS_FAT32,
	FS_FAT32_UNRECOGNIZED,
	FS_NTFS,
	FS_UDF
} file_system_type;

typedef struct _udefrag_allowed_actions {
	int allow_dir_defrag;      /* on FAT directories aren't moveable */
	int allow_optimize;        /* zero on FAT, because of unmoveable directories */
} udefrag_allowed_actions;

/*
* More details at http://www.thescripts.com/forum/thread617704.html
* ('Dynamically-allocated Multi-dimensional Arrays - C').
*/
typedef struct _cmap {
	ULONGLONG (*array)[NUM_OF_SPACE_STATES];
	ULONGLONG field_size;
	int map_size;
	int n_colors; /* TODO: remove it */
	int default_color;
	ULONGLONG clusters_per_cell;
	ULONGLONG clusters_per_last_cell;
	BOOLEAN opposite_order; /* clusters < cells */
	ULONGLONG cells_per_cluster;
	ULONGLONG cells_per_last_cluster;
} cmap;

typedef void (__stdcall *udefrag_progress_router)(void /*udefrag_job_parameters*/ *p);
typedef int  (__stdcall *udefrag_termination_router)(void /*udefrag_job_parameters*/ *p);

typedef struct _udefrag_job_parameters {
	unsigned char volume_letter;                /* volume letter */
	udefrag_job_type job_type;                  /* type of requested job */
	udefrag_progress_callback cb;               /* progress update callback */
	udefrag_terminator t;                       /* termination callback */
	void *p;                                    /* pointer to user defined data to be passed to both callbacks */
	udefrag_progress_router progress_router;        /* address of procedure delivering progress info to the caller */
	udefrag_termination_router termination_router;  /* address of procedure triggering job termination */
	ULONGLONG start_time;                       /* time of the job launch */
	ULONGLONG progress_refresh_time;            /* time of the last progress refresh */
	udefrag_options udo;                        /* job options */
	udefrag_progress_info pi;                   /* progress counters */
	winx_volume_information v_info;             /* basic volume information */
	file_system_type fs_type;                   /* type of volume file system */
	winx_file_info *filelist;                   /* list of files */
	udefrag_fragmented_file *fragmented_files;  /* list of fragmented files */
	winx_volume_region *free_regions;           /* list of free space regions */
	struct _mft_zones mft_zones;                /* coordinates of mft zones */
	ULONGLONG clusters_per_256k;                /* number of clusters in 256k block */
	udefrag_allowed_actions actions;            /* actions allowed for selected file system */
	cmap cluster_map;                           /* cluster map internal data */
	WINX_FILE *fVolume;                         /* handle of the volume, used by file moving routines */
	winx_volume_region *temp_space_list;        /* list of regions of space temporarily allocated by system */
	ULONGLONG moveable_mft_clusters;            /* number of clusters in moveable parts of $mft */
	ULONGLONG free_rgn_size_threshold;          /* free region size threshold used in volume optimization */
} udefrag_job_parameters;

int  get_options(udefrag_job_parameters *jp);
void release_options(udefrag_job_parameters *jp);
int save_fragmentation_reports(udefrag_job_parameters *jp);
void remove_fragmentation_reports(udefrag_job_parameters *jp);

/* some volume space states, for internal use only */
#define SYSTEM_OR_MFT_ZONE_SPACE      100
#define SYSTEM_OR_FREE_SPACE          101
#define FREE_OR_MFT_ZONE_SPACE        102
#define TMP_SYSTEM_OR_MFT_ZONE_SPACE  103

int allocate_map(int map_size,udefrag_job_parameters *jp);
void free_map(udefrag_job_parameters *jp);
void reset_cluster_map(udefrag_job_parameters *jp);
void colorize_map_region(udefrag_job_parameters *jp,
		ULONGLONG lcn, ULONGLONG length, int new_color, int old_color);
void colorize_file(udefrag_job_parameters *jp, winx_file_info *f, int old_color);
void colorize_file_as_system(udefrag_job_parameters *jp, winx_file_info *f);
int get_file_color(udefrag_job_parameters *jp, winx_file_info *f);
void release_temp_space_regions(udefrag_job_parameters *jp);
void redraw_all_temporary_system_space_as_free(udefrag_job_parameters *jp);

int analyze(udefrag_job_parameters *jp);
int defragment(udefrag_job_parameters *jp);
int defragment_partial(udefrag_job_parameters *jp);
int full_optimize(udefrag_job_parameters *jp);
int quick_optimize(udefrag_job_parameters *jp);
void destroy_lists(udefrag_job_parameters *jp);

ULONGLONG start_timing(char *operation_name,udefrag_job_parameters *jp);
void stop_timing(char *operation_name,ULONGLONG start_time,udefrag_job_parameters *jp);

WINX_FILE * __stdcall new_winx_vopen(char volume_letter);
int check_region(udefrag_job_parameters *jp,ULONGLONG lcn,ULONGLONG length);
void update_mft_zones_layout(udefrag_job_parameters *jp);
void adjust_mft_file(winx_file_info *f,udefrag_job_parameters *jp);
winx_blockmap *get_first_block_of_cluster_chain(winx_file_info *f,ULONGLONG vcn);

NTSTATUS udefrag_fopen(winx_file_info *f,HANDLE *phFile);
int is_file_locked(winx_file_info *f,udefrag_job_parameters *jp);
int is_mft(winx_file_info *f,udefrag_job_parameters *jp);
int is_mft_mirror(winx_file_info *f,udefrag_job_parameters *jp);

int expand_fragmented_files_list(winx_file_info *f,udefrag_job_parameters *jp);
void truncate_fragmented_files_list(winx_file_info *f,udefrag_job_parameters *jp);

/*
* move_file prototype and flags
*/

/*
* If this flag is set, the move_file
* routine will cut off moved range
* of clusters from the file's map passed in.
* Thus, after each move file's map will become
* shorter and shorter preventing subsequent
* moves of the same data in a partial
* defragmentation and in volume optimization.
*/
#define UD_MOVE_FILE_CUT_OFF_MOVED_CLUSTERS 0x1

int move_file(winx_file_info *f,
              ULONGLONG vcn,
			  ULONGLONG length,
			  ULONGLONG target,
			  int flags,
			  udefrag_job_parameters *jp
			  );

ULONGLONG get_file_length(udefrag_job_parameters *jp, winx_file_info *f);
int can_defragment(winx_file_info *f,udefrag_job_parameters *jp);
int can_move(winx_file_info *f,udefrag_job_parameters *jp);
int optimize_mft(udefrag_job_parameters *jp);
int defragment_small_files(udefrag_job_parameters *jp);
int defragment_big_files(udefrag_job_parameters *jp);

/* flags used in move_files_to_xxx routines */
enum {
	MOVE_FRAGMENTED,
	MOVE_NOT_FRAGMENTED,
	MOVE_ALL
};

int move_files_to_front(udefrag_job_parameters *jp, ULONGLONG start_lcn, int flags);
int move_files_to_back(udefrag_job_parameters *jp, ULONGLONG start_lcn, int flags);

#endif /* _UDEFRAG_INTERNALS_H_ */
