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
* Debugging related routines.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#include "ntndk.h"
#include "zenwinx.h"

typedef struct _DBG_OUTPUT_DEBUG_STRING_BUFFER {
    ULONG ProcessId;
    UCHAR Msg[4096-sizeof(ULONG)];
} DBG_OUTPUT_DEBUG_STRING_BUFFER, *PDBG_OUTPUT_DEBUG_STRING_BUFFER;

#define BUFFER_SIZE (4096-sizeof(ULONG))

BOOL firstcall = TRUE;
BOOL err_flag = FALSE;

/****f* zenwinx.debug/winx_dbg_print
* NAME
*    winx_dbg_print
* FUNCTION
*    DbgPrint analog.
* SEE ALSO
*    winx_debug_print
******/
void __cdecl winx_dbg_print(char *format, ...)
{
	char *buffer = NULL;
	va_list arg;
	int length;
	ERRORHANDLERPROC eh = NULL;
	
	buffer = winx_virtual_alloc(BUFFER_SIZE);
	if(!buffer){
		if(!err_flag){
			winx_raise_error("W: Cannot allocate %u bytes "
				"of memory for winx_dbg_print()!",BUFFER_SIZE);
			err_flag = TRUE;
		}
		return;
	}

	/* 1a. store resulting ANSI string into buffer */
	va_start(arg,format);
	memset(buffer,0,BUFFER_SIZE);
	length = _vsnprintf(buffer,BUFFER_SIZE - 1,format,arg);
	buffer[BUFFER_SIZE - 1] = 0;
	va_end(arg);

	/* 1b. send resulting ANSI string to the debugger */
	if(!firstcall) eh = winx_set_error_handler(NULL);
	winx_debug_print(buffer);
	if(!firstcall) winx_set_error_handler(eh);
	
	winx_virtual_free(buffer,BUFFER_SIZE);
	firstcall = FALSE;
}

/****f* zenwinx.debug/winx_debug_print
* NAME
*    winx_debug_print
* SYNOPSIS
*    winx_debug_print(string);
* FUNCTION
*    Delivers message to the Debug Viewer program.
* INPUTS
*    string - message to send
* RESULT
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value.
* EXAMPLE
*    winx_debug_print("Hello!\n");
* SEE ALSO
*    winx_dbg_print
******/

int __stdcall winx_debug_print(char *string)
{
	HANDLE hEvtBufferReady = NULL;
	HANDLE hEvtDataReady = NULL;
	HANDLE hSection = NULL;
	LPVOID BaseAddress = NULL;
	LARGE_INTEGER SectionOffset;
	ULONG ViewSize = 0;

	UNICODE_STRING us;
	NTSTATUS Status;
	OBJECT_ATTRIBUTES oa;
	LARGE_INTEGER interval;
	
	DBG_OUTPUT_DEBUG_STRING_BUFFER *dbuffer;
	int length;
	
	/* FIXME: synchronize with other threads */
	
	/* 1. Open debugger's objects. */
	if(winx_open_event(L"\\BaseNamedObjects\\DBWIN_BUFFER_READY",SYNCHRONIZE,
		&hEvtBufferReady) < 0) goto failure;
	if(winx_open_event(L"\\BaseNamedObjects\\DBWIN_DATA_READY",EVENT_MODIFY_STATE,
		&hEvtDataReady) < 0) goto failure;
	RtlInitUnicodeString(&us,L"\\BaseNamedObjects\\DBWIN_BUFFER");
	InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
	Status = NtOpenSection(&hSection,SECTION_ALL_ACCESS,&oa);
	if(!NT_SUCCESS(Status)) goto failure;
	SectionOffset.QuadPart = 0;
	Status = NtMapViewOfSection(hSection,NtCurrentProcess(),
		&BaseAddress,0,0,&SectionOffset,&ViewSize,ViewShare,
		0,PAGE_READWRITE);
	if(!NT_SUCCESS(Status)) goto failure;
	
	/* 2. Send message. */
	/*
	* wait a maximum of 10 seconds for the debug monitor 
	* to finish processing the shared buffer
	*/
	interval.QuadPart = -(10000 * 10000);
	if(NtWaitForSingleObject(hEvtBufferReady,FALSE,&interval) != WAIT_OBJECT_0)
		goto failure;
	
	dbuffer = (DBG_OUTPUT_DEBUG_STRING_BUFFER *)BaseAddress;

	/* write the process id into the buffer */
	dbuffer->ProcessId = (DWORD)(NtCurrentTeb()->ClientId.UniqueProcess);

	strncpy(dbuffer->Msg,string,4096-sizeof(ULONG));
	dbuffer->Msg[4096-sizeof(ULONG)-1] = 0;
	/* ensure that buffer has new line character */
	length = strlen(dbuffer->Msg);
	if(dbuffer->Msg[length-1] != '\n'){
		if(length == (4096-sizeof(ULONG)-1)){
			dbuffer->Msg[length-1] = '\n';
		} else {
			dbuffer->Msg[length] = '\n';
			dbuffer->Msg[length+1] = 0;
		}
	}

	/* signal that the data contains meaningful data and can be read */
	NtSetEvent(hEvtDataReady,NULL);
	
	NtCloseSafe(hEvtBufferReady);
	NtCloseSafe(hEvtDataReady);
	if(BaseAddress)
		NtUnmapViewOfSection(NtCurrentProcess(),BaseAddress);
	NtCloseSafe(hSection);
	return 0;

failure:
	NtCloseSafe(hEvtBufferReady);
	NtCloseSafe(hEvtDataReady);
	if(BaseAddress)
		NtUnmapViewOfSection(NtCurrentProcess(),BaseAddress);
	NtCloseSafe(hSection);
	return (-1);
}
