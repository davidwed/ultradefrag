/*
 *  WINX - WIndows Native eXtended library.
 *  Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 *  winx.dll interface definitions.
 */

typedef LONGLONG WINX_STATUS;

#define MAKE_WINX_STATUS(code,status) \
		((WINX_STATUS)(((LONG)(status)) | ((WINX_STATUS)((LONG)(code))) << 32))

#define NTSTATUS(ws)  ((LONG)(ws))
#define ERRCODE(ws)   ((LONG)(((WINX_STATUS)(ws) >> 32) & 0xFFFFFFFF))

/* winx error codes */
#define WINX_SUCCESS                  0x00000000L
#define WINX_KEYBOARD_INACCESSIBLE    0x00000001L
#define WINX_CREATE_KB_EVENT_ERROR    0x00000002L


#define SafeNtClose(h) if(h) { NtClose(h); h = NULL; }

/* winx functions prototypes */
WINX_STATUS __stdcall winx_init(PVOID Peb);
WINX_STATUS __stdcall winx_exit(void);
