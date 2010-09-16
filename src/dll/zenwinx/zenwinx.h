/*
 *  ZenWINX - WIndows Native eXtended library.
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
* zenwinx.dll interface definitions.
*/

#ifndef _ZENWINX_H_
#define _ZENWINX_H_

#define DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY "      Hit any key to display next page..."

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#define NtCloseSafe(h) { if(h) { NtClose(h); h = NULL; } }
#define DebugPrint winx_dbg_print
#define DebugPrintEx winx_dbg_print_ex

#define DbgCheck1(c,f,r) { \
	if(!(c)) {           \
		DebugPrint("The first parameter of " f " is invalid!"); \
		return (r);      \
	}                    \
}

#define DbgCheck2(c1,c2,f,r) { \
	if(!(c1)) {              \
		DebugPrint("The first parameter of " f " is invalid!"); \
		return (r);          \
	}                        \
	if(!(c2)) {              \
		DebugPrint("The second parameter of " f " is invalid!"); \
		return (r);          \
	}                        \
}

#define DbgCheck3(c1,c2,c3,f,r) { \
	if(!(c1)) {              \
		DebugPrint("The first parameter of " f " is invalid!"); \
		return (r);          \
	}                        \
	if(!(c2)) {              \
		DebugPrint("The second parameter of " f " is invalid!"); \
		return (r);          \
	}                        \
	if(!(c3)) {              \
		DebugPrint("The third parameter of " f " is invalid!"); \
		return (r);          \
	}                        \
}

typedef struct _WINX_FILE {
	HANDLE hFile;             /* file handle */
	LARGE_INTEGER roffset;    /* offset for read requests */
	LARGE_INTEGER woffset;    /* offset for write requests */
	void *io_buffer;          /* for buffered i/o */
	size_t io_buffer_size;    /* size of the buffer, in bytes */
	size_t io_buffer_offset;  /* current offset inside io_buffer */
	LARGE_INTEGER wboffset;   /* offset for write requests in buffered mode */
} WINX_FILE, *PWINX_FILE;

#define winx_fileno(f) ((f)->hFile)

/* zenwinx functions prototypes */
int  __stdcall winx_init(void *peb);
void __stdcall winx_exit(int exit_code);
void __stdcall winx_reboot(void);
void __stdcall winx_shutdown(void);

void __stdcall zenwinx_native_init(void);
void __stdcall zenwinx_native_unload(void);

void * __stdcall winx_virtual_alloc(SIZE_T size);
void   __stdcall winx_virtual_free(void *addr,SIZE_T size);
void * __stdcall winx_heap_alloc_ex(SIZE_T size,SIZE_T flags);
#define winx_heap_alloc(n)      winx_heap_alloc_ex(n,0)
#define winx_heap_alloc_zero(n) winx_heap_alloc_ex(n,HEAP_ZERO_MEMORY)
void   __stdcall winx_heap_free(void *addr);

int __cdecl winx_putch(int ch);
int __cdecl winx_puts(const char *string);
int __cdecl winx_printf(const char *format, ...);

int __cdecl winx_getch(void);
int __cdecl winx_getche(void);
int __cdecl winx_gets(char *string,int n);
int __cdecl winx_prompt(char *prompt,char *string,int n);

typedef struct _winx_history_entry {
	struct _winx_history_entry *next;
	struct _winx_history_entry *prev;
	char *string;
} winx_history_entry;

typedef struct _winx_history {
	winx_history_entry *head;
	winx_history_entry *current;
	int n_entries;
} winx_history;

void __cdecl winx_init_history(winx_history *h);
int  __cdecl winx_prompt_ex(char *prompt,char *string,int n,winx_history *h);
void __cdecl winx_destroy_history(winx_history *h);

int __cdecl winx_print_array_of_strings(char **strings,int line_width,int max_rows,char *prompt,int divide_to_pages);

/*
* Note: winx_scanf() can not be implemented;
* use winx_gets() and sscanf() instead.
*/
int __cdecl winx_kbhit(int msec);
int __cdecl winx_breakhit(int msec);
int __cdecl winx_kb_read(KBD_RECORD *kbd_rec,int msec);

void __stdcall winx_sleep(int msec);
int  __stdcall winx_get_os_version(void);
short * __stdcall winx_get_windows_boot_options(void);
int __stdcall winx_windows_in_safe_mode(void);
int  __stdcall winx_get_windows_directory(char *buffer, int length);
int  __stdcall winx_get_proc_address(short *libname,char *funcname,PVOID *proc_addr);

/* process mode constants */
#define INTERNAL_SEM_FAILCRITICALERRORS 0
int  __stdcall winx_set_system_error_mode(unsigned int mode);

int __stdcall winx_load_driver(short *driver_name);
int __stdcall winx_unload_driver(short *driver_name);

int  __stdcall winx_create_thread(PTHREAD_START_ROUTINE start_addr,PVOID parameter,HANDLE *phandle);
void __stdcall winx_exit_thread(void);

int  __stdcall winx_enable_privilege(unsigned long luid);

#define DRIVE_ASSIGNED_BY_SUBST_COMMAND 1200
int __stdcall winx_get_drive_type(char letter);

/*
* Maximal length of the file system name.
* Specified length is more than enough
* to hold all known names.
*/
#define MAX_FS_NAME_LENGTH 31

typedef struct _winx_volume_information {
	char volume_letter;                    /* must be set by caller! */
	char fs_name[MAX_FS_NAME_LENGTH + 1];  /* the name of the file system */
	ULONG fat32_mj_version;                /* major number of FAT32 version */
	ULONG fat32_mn_version;                /* minor number of FAT32 version */
	ULONGLONG total_bytes;                 /* total volume size, in bytes */
	ULONGLONG free_bytes;                  /* amount of free space, in bytes */
	ULONGLONG total_clusters;              /* total number of clusters */
	ULONGLONG bytes_per_cluster;           /* cluster size, in bytes */
	ULONG sectors_per_cluster;             /* number of sectors in each cluster */
	ULONG bytes_per_sector;                /* sector size, in bytes */
	NTFS_DATA ntfs_data;                   /* NTFS data, valid for NTFS formatted volumes only */
} winx_volume_information;

int __stdcall winx_get_volume_information(char volume_letter,winx_volume_information *v);

WINX_FILE * __stdcall winx_fopen(const char *filename,const char *mode);
WINX_FILE * __stdcall winx_fbopen(const char *filename,const char *mode,int buffer_size);
size_t __stdcall winx_fread(void *buffer,size_t size,size_t count,WINX_FILE *f);
size_t __stdcall winx_fwrite(const void *buffer,size_t size,size_t count,WINX_FILE *f);
ULONGLONG __stdcall winx_fsize(WINX_FILE *f);
void   __stdcall winx_fclose(WINX_FILE *f);
int __stdcall winx_ioctl(WINX_FILE *f,
                         int code,char *description,
                         void *in_buffer,int in_size,
                         void *out_buffer,int out_size,
                         int *pbytes_returned);
int __stdcall winx_fflush(WINX_FILE *f);
int __stdcall winx_create_directory(const char *path);
int __stdcall winx_delete_file(const char *filename);
void * __stdcall winx_get_file_contents(const char *filename,size_t *bytes_read);
void __stdcall winx_release_file_contents(void *contents);

/* winx_ftw flags */
#define WINX_FTW_RECURSIVE           0x1 /* forces to recursively scan all subdirectories */
#define WINX_FTW_DUMP_FILES          0x2 /* forces to fill winx_file_disposition structure */
#define WINX_FTW_ALLOW_PARTIAL_SCAN  0x4 /* allows information to be gathered partially */

#define is_readonly(f)            ((f)->flags & FILE_ATTRIBUTE_READONLY)
#define is_hidden(f)              ((f)->flags & FILE_ATTRIBUTE_HIDDEN)
#define is_system(f)              ((f)->flags & FILE_ATTRIBUTE_SYSTEM)
#define is_directory(f)           ((f)->flags & FILE_ATTRIBUTE_DIRECTORY)
#define is_archive(f)             ((f)->flags & FILE_ATTRIBUTE_ARCHIVE)
#define is_device(f)              ((f)->flags & FILE_ATTRIBUTE_DEVICE)
#define is_normal(f)              ((f)->flags & FILE_ATTRIBUTE_NORMAL)
#define is_temporary(f)           ((f)->flags & FILE_ATTRIBUTE_TEMPORARY)
#define is_sparse(f)              ((f)->flags & FILE_ATTRIBUTE_SPARSE_FILE)
#define is_reparse_point(f)       ((f)->flags & FILE_ATTRIBUTE_REPARSE_POINT)
#define is_compressed(f)          ((f)->flags & FILE_ATTRIBUTE_COMPRESSED)
#define is_offline(f)             ((f)->flags & FILE_ATTRIBUTE_OFFLINE)
#define is_not_content_indexed(f) ((f)->flags & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
#define is_encrypted(f)           ((f)->flags & FILE_ATTRIBUTE_ENCRYPTED)
#define are_valid_flags(f)        ((f)->flags & FILE_ATTRIBUTE_VALID_FLAGS)
#define are_valid_set_flags(f)    ((f)->flags & FILE_ATTRIBUTE_VALID_SET_FLAGS)

#define WINX_FILE_DISP_FRAGMENTED 0x1

#define is_fragmented(f)          ((f)->disp.flags & WINX_FILE_DISP_FRAGMENTED)

typedef struct _winx_blockmap {
	struct _winx_blockmap *next; /* pointer to the next fragment */
	struct _winx_blockmap *prev; /* pointer to the previous fragment */
	ULONGLONG vcn;               /* virtual cluster number */
	ULONGLONG lcn;               /* logical cluster number */
	ULONGLONG length;            /* size of the fragment, in bytes */
} winx_blockmap;

typedef struct _winx_file_disposition {
	ULONGLONG clusters;                /* total number of clusters belonging to the file */
	unsigned long fragments;           /* total number of file fragments */
	unsigned long flags;               /* combination of WINX_FILE_DISP_xxx flags */
	winx_blockmap *blockmap;           /* map of blocks <=> list of fragments */
} winx_file_disposition;

typedef struct _winx_file_internal_info {
	ULONGLONG BaseMftId;
	ULONGLONG ParentDirectoryMftId;
} winx_file_internal_info;

typedef struct _winx_file_info {
	struct _winx_file_info *next;      /* pointer to the next item */
	struct _winx_file_info *prev;      /* pointer to the previous item */
	short *name;                       /* name of the file */
	short *path;                       /* full native path */
	unsigned long flags;               /* combination of FILE_ATTRIBUTE_xxx flags defined in winnt.h */
	winx_file_disposition disp;        /* information about file fragments and their disposition */
	unsigned long user_defined_flags;  /* combination of flags defined by the caller */
	winx_file_internal_info internal;  /* internal information used by ftw_scan_disk support routines */
} winx_file_info;

typedef int  (__stdcall *ftw_filter_callback)(winx_file_info *f);
typedef void (__stdcall *ftw_progress_callback)(winx_file_info *f);
typedef int  (__stdcall *ftw_terminator)(void);

winx_file_info * __stdcall winx_ftw(short *path, int flags,
		ftw_filter_callback fcb, ftw_progress_callback pcb, ftw_terminator t);

winx_file_info * __stdcall winx_scan_disk(char volume_letter, int flags,
		ftw_filter_callback fcb,ftw_progress_callback pcb, ftw_terminator t);

void __stdcall winx_ftw_release(winx_file_info *filelist);
#define winx_scan_disk_release(f) winx_ftw_release(f)

int __stdcall winx_query_env_variable(short *name, short *buffer, int length);
int __stdcall winx_set_env_variable(short *name, short *value);

int __stdcall winx_query_symbolic_link(short *name, short *buffer, int length);

int __stdcall winx_fbsize(ULONGLONG number, int digits, char *buffer, int length);
int __stdcall winx_dfbsize(char *string,ULONGLONG *pnumber);

int __stdcall winx_create_event(short *name,int type,HANDLE *phandle);
int __stdcall winx_open_event(short *name,int flags,HANDLE *phandle);
void __stdcall winx_destroy_event(HANDLE h);

int __stdcall winx_register_boot_exec_command(short *command);
int __stdcall winx_unregister_boot_exec_command(short *command);

void __cdecl winx_dbg_print(char *format, ...);
void __cdecl winx_dbg_print_ex(unsigned long status,char *format, ...);

ULONGLONG __stdcall winx_str2time(char *string);
int __stdcall winx_time2str(ULONGLONG time,char *buffer,int size);
ULONGLONG __stdcall winx_xtime(void);
#define winx_xtime_nsec() (winx_xtime() * 1000 * 1000)

/**
 * @brief Generic structure describing double linked list entry.
 */
typedef struct _list_entry {
	struct _list_entry *next; /* pointer to next entry */
	struct _list_entry *prev; /* pointer to previous entry */
} list_entry;

list_entry * __stdcall winx_list_insert_item(list_entry **phead,list_entry *prev,long size);
void         __stdcall winx_list_remove_item(list_entry **phead,list_entry *item);
void         __stdcall winx_list_destroy    (list_entry **phead);

wchar_t * __cdecl winx_wcsistr(const wchar_t * wcs1,const wchar_t * wcs2);

#endif /* _ZENWINX_H_ */
