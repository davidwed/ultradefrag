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
 *  zenwinx.dll interface definitions.
 */

#ifndef _ZENWINX_H_
#define _ZENWINX_H_

#define NtCloseSafe(h) if(h) { NtClose(h); h = NULL; }

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

unsigned long __stdcall winx_get_last_error(void);
void   __stdcall winx_set_last_error(unsigned long code);
char * __stdcall winx_get_error_description(unsigned long code);
char * __stdcall winx_get_error_message(int code);
int    __stdcall winx_get_error_message_ex(char *s, int n, signed int code);

/* zenwinx error codes */
#define WINX_INVALID_KEYBOARD    0x00000001
#define WINX_CREATE_EVENT_FAILED 0x00000002

#endif /* _ZENWINX_H_ */
