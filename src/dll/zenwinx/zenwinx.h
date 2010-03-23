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
	HANDLE hFile;
	LARGE_INTEGER roffset;
	LARGE_INTEGER woffset;
} WINX_FILE, *PWINX_FILE;

#define winx_fileno(f) ((f)->hFile)

/* zenwinx functions prototypes */
int  __stdcall winx_init(void *peb);
void __stdcall winx_exit(int exit_code);
void __stdcall winx_reboot(void);
void __stdcall winx_shutdown(void);

void * __stdcall winx_virtual_alloc(SIZE_T size);
void   __stdcall winx_virtual_free(void *addr,SIZE_T size);
void * __stdcall winx_heap_alloc(SIZE_T size);
void   __stdcall winx_heap_free(void *addr);

int __cdecl winx_putch(int ch);
int __cdecl winx_puts(const char *string);
int __cdecl winx_printf(const char *format, ...);

int __cdecl winx_getch(void);
int __cdecl winx_getche(void);
int __cdecl winx_gets(char *string,int n);
/*
* Note: winx_scanf() can not be implemented;
* use winx_gets() and sscanf() instead.
*/
int __cdecl winx_kbhit(int msec);
int __cdecl winx_breakhit(int msec);

void __stdcall winx_sleep(int msec);
int  __stdcall winx_get_os_version(void);
int __stdcall winx_windows_in_safe_mode(void);
int  __stdcall winx_get_windows_directory(char *buffer, int length);
int  __stdcall winx_get_proc_address(short *libname,char *funcname,PVOID *proc_addr);

/* process mode constants */
#define INTERNAL_SEM_FAILCRITICALERRORS 0
int  __stdcall winx_set_system_error_mode(unsigned int mode);

int __stdcall winx_load_driver(short *driver_name);
int __stdcall winx_unload_driver(short *driver_name);

int  __stdcall winx_create_thread(PTHREAD_START_ROUTINE start_addr,HANDLE *phandle);
void __stdcall winx_exit_thread(void);

int  __stdcall winx_enable_privilege(unsigned long luid);

#define DRIVE_ASSIGNED_BY_SUBST_COMMAND 1200
int __stdcall winx_get_drive_type(char letter);
int __stdcall winx_get_volume_size(char letter, LARGE_INTEGER *ptotal, LARGE_INTEGER *pfree);
int __stdcall winx_get_filesystem_name(char letter, char *buffer, int length);

WINX_FILE * __stdcall winx_fopen(const char *filename,const char *mode);
size_t __stdcall winx_fread(void *buffer,size_t size,size_t count,WINX_FILE *f);
size_t __stdcall winx_fwrite(const void *buffer,size_t size,size_t count,WINX_FILE *f);
void   __stdcall winx_fclose(WINX_FILE *f);
int __stdcall winx_ioctl(WINX_FILE *f,
                         int code,char *description,
                         void *in_buffer,int in_size,
                         void *out_buffer,int out_size,
                         int *pbytes_returned);
int __stdcall winx_fflush(WINX_FILE *f);
int __stdcall winx_create_directory(const char *path);
int __stdcall winx_delete_file(const char *filename);

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

/**
 * @brief Generic structure describing double linked list entry.
 */
typedef struct _list_entry {
	struct _list_entry *n; /* pointer to next entry */
	struct _list_entry *p; /* pointer to previous entry */
} list_entry;

list_entry * __stdcall winx_list_insert_item(list_entry **phead,list_entry *prev,long size);
void         __stdcall winx_list_remove_item(list_entry **phead,list_entry *item);
void         __stdcall winx_list_destroy    (list_entry **phead);

#endif /* _ZENWINX_H_ */
