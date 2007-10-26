/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
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
 *  GUI - common definitions.
 */

#ifndef _DFRG_MAIN_H_
#define _DFRG_MAIN_H_

#include "../include/udefrag.h"
/*
typedef LONG NTSTATUS;

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

NTSYSAPI VOID NTAPI RtlInitUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PCWSTR SourceString);

NTSTATUS NTAPI
NtLoadDriver(IN PUNICODE_STRING DriverServiceName);

NTSTATUS NTAPI
NtUnloadDriver(IN PUNICODE_STRING DriverServiceName);

#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000)
#define STATUS_IMAGE_ALREADY_LOADED      ((NTSTATUS)0xC000010E)
*/
#ifndef USE_WINDDK
#define SetWindowLongPtr SetWindowLong
#define LONG_PTR LONG
#define GWLP_WNDPROC GWL_WNDPROC
#endif

#define BLOCKS_PER_HLINE  52
#define BLOCKS_PER_VLINE  14
#define N_BLOCKS          BLOCKS_PER_HLINE * BLOCKS_PER_VLINE

#endif /* _DFRG_MAIN_H_ */