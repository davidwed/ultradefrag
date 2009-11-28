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

#include "ntndk.h"
#include "zenwinx.h"

HANDLE hSynchEvent = NULL;

/* internal calls */
void __stdcall InitSynchObjectsErrorHandler(short *msg)
{
	winx_dbg_print("------------------------------------------------------------\n");
	winx_dbg_print("%ws\n",msg);
	winx_dbg_print("------------------------------------------------------------\n");
}

void winx_init_synch_objects(void)
{
	winx_create_event(L"\\winx_dbgprint_synch_event",
		SynchronizationEvent,&hSynchEvent);
	if(hSynchEvent) NtSetEvent(hSynchEvent,NULL);
}

void winx_destroy_synch_objects(void)
{
	winx_destroy_event(hSynchEvent);
}

typedef struct _DBG_OUTPUT_DEBUG_STRING_BUFFER {
    ULONG ProcessId;
    UCHAR Msg[4096-sizeof(ULONG)];
} DBG_OUTPUT_DEBUG_STRING_BUFFER, *PDBG_OUTPUT_DEBUG_STRING_BUFFER;

#define BUFFER_SIZE (4096-sizeof(ULONG))

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
	
	/* never call winx_raise_error() here! */
	
	buffer = winx_virtual_alloc(BUFFER_SIZE);
	if(!buffer){
		if(!err_flag){
			winx_debug_print(">UltraDefrag< Cannot allocate memory "
							 "for winx_dbg_print()!");
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
	winx_debug_print(buffer);
	
	winx_virtual_free(buffer,BUFFER_SIZE);
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

	/* never call winx_raise_error() here! */

	/* 0. synchronize with other threads */
	if(hSynchEvent){
		interval.QuadPart = -(11000 * 10000); /* 11 sec */
		Status = NtWaitForSingleObject(hSynchEvent,FALSE,&interval);
		if(Status != WAIT_OBJECT_0) return (-1);
	}
	
	/* 1. Open debugger's objects. */
	RtlInitUnicodeString(&us,L"\\BaseNamedObjects\\DBWIN_BUFFER_READY");
	InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
	Status = NtOpenEvent(&hEvtBufferReady,SYNCHRONIZE,&oa);
	if(!NT_SUCCESS(Status)) goto failure;
	
	RtlInitUnicodeString(&us,L"\\BaseNamedObjects\\DBWIN_DATA_READY");
	InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
	Status = NtOpenEvent(&hEvtDataReady,EVENT_MODIFY_STATE,&oa);
	if(!NT_SUCCESS(Status)) goto failure;

	RtlInitUnicodeString(&us,L"\\BaseNamedObjects\\DBWIN_BUFFER");
	InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
	Status = NtOpenSection(&hSection,SECTION_ALL_ACCESS,&oa);
	if(!NT_SUCCESS(Status)) goto failure;
	SectionOffset.QuadPart = 0;
	Status = NtMapViewOfSection(hSection,NtCurrentProcess(),
		&BaseAddress,0,0,&SectionOffset,(SIZE_T *)&ViewSize,ViewShare,
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
	#if defined(__GNUC__)
	#ifndef _WIN64
	dbuffer->ProcessId = (DWORD)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess);
	#else
	dbuffer->ProcessId = 0xFFFF; /* eliminates need of NtCurrentTeb() */
	#endif
	#else /* not defined(__GNUC__) */
	dbuffer->ProcessId = (DWORD)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess);
	#endif

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
	if(hSynchEvent) NtSetEvent(hSynchEvent,NULL);
	return 0;

failure:
	NtCloseSafe(hEvtBufferReady);
	NtCloseSafe(hEvtDataReady);
	if(BaseAddress)
		NtUnmapViewOfSection(NtCurrentProcess(),BaseAddress);
	NtCloseSafe(hSection);
	if(hSynchEvent) NtSetEvent(hSynchEvent,NULL);
	return (-1);
}
