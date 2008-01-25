/*
 *  ZenWINX - WIndows Native eXtended library.
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
* zenwinx.dll interface definitions.
*/

#ifndef _ZENWINX_H_
#define _ZENWINX_H_

/*
* IMPORTANT NOTES:
*   1. If some winx function returns negative value,
*   such function must call winx_push_error() before.
*   2. After each unsuccessful call of such functions,
*   winx_pop_error() must be called before any other
*   system call in the current thread (win32, 
*   native, crt, mfc ...).
*   3. Each winx_push_error() call must be like this:
*       winx_push_error("error message with parameters: %x!",
*           parameters,(UINT)Status);
*      Status parameter is optional.
*   4. List of functions that needs winx_pop_error:
*		winx_breakhit
*		winx_create_thread
*		winx_enable_privilege
*		winx_exit_thread
*		winx_get_proc_address
*		winx_getch
*		winx_getche
*		winx_gets
*		winx_init
*		winx_kbhit
*/

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#define NtCloseSafe(h) if(h) { NtClose(h); h = NULL; }

#define ERR_MSG_SIZE 1024

/* zenwinx functions prototypes */
int  __stdcall winx_init(void *peb);
void __stdcall winx_exit(int exit_code);
void __stdcall winx_reboot(void);
void __stdcall winx_shutdown(void);

void * __stdcall winx_virtual_alloc(unsigned long size);
void   __stdcall winx_virtual_free(void *addr,unsigned long size);

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

void __cdecl winx_push_error(char *format, ...);
void __stdcall winx_pop_error(char *buffer,int size);
void __stdcall winx_pop_werror(short *buffer, int size);
void __stdcall winx_save_error(char *buffer, int size);
void __stdcall winx_restore_error(char *buffer);

void __stdcall winx_sleep(int msec);
int  __stdcall winx_get_os_version(void);
int  __stdcall winx_get_proc_address(short *libname,char *funcname,PVOID *proc_addr);

int  __stdcall winx_create_thread(PTHREAD_START_ROUTINE start_addr,HANDLE *phandle);
int  __stdcall winx_exit_thread(void);

int  __stdcall winx_enable_privilege(HANDLE hToken,DWORD dwLowPartOfLUID);

#endif /* _ZENWINX_H_ */
