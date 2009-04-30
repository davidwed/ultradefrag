/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
*  Procedures for debug logging.
*/

#include <stdarg.h>
#include <stdio.h>

#include "driver.h"

/*
* It must be always greater than PAGE_SIZE (8192 ?), because the 
* dbg_buffer needs to be aligned on a page boundary. This is a 
* special requirement of BugCheckSecondaryDumpDataCallback() 
* function described in DDK documentation.
*/
#define DBG_BUFFER_SIZE (32 * 1024) /* 32 kb - more than enough */

VOID __stdcall BugCheckCallback(PVOID Buffer,ULONG Length);
VOID __stdcall BugCheckSecondaryDumpDataCallback(
    KBUGCHECK_CALLBACK_REASON Reason,
    PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    PVOID ReasonSpecificData,
    ULONG ReasonSpecificDataLength);

KBUGCHECK_CALLBACK_RECORD bug_check_record;
KBUGCHECK_REASON_CALLBACK_RECORD bug_check_reason_record;

// {B7B5FCD6-FB0A-48f9-AEE5-02AF097C9504}
GUID ultradfg_guid = \
{ 0xb7b5fcd6, 0xfb0a, 0x48f9, { 0xae, 0xe5, 0x2, 0xaf, 0x9, 0x7c, 0x95, 0x4 } };

KEVENT dbgprint_event;
short *dbg_buffer;
unsigned int dbg_offset;

void __stdcall RegisterBugCheckCallbacks(void)
{
	KeInitializeCallbackRecord((&bug_check_record)); /* double brackets are required */
	KeInitializeCallbackRecord((&bug_check_reason_record));
	/*
	* Register callback on systems older than XP SP1 / Server 2003.
	* There is one significant restriction: bug data will be saved only 
	* in full kernel memory dump.
	*/
	if(!KeRegisterBugCheckCallback(&bug_check_record,(VOID *)BugCheckCallback,
		dbg_buffer,DBG_BUFFER_SIZE,"UltraDefrag"))
			DebugPrint("=Ultradfg= Cannot register bug check routine!\n",NULL);
	/*
	* Register another callback procedure on modern systems: XP SP1, Server 2003 etc.
	* Bug data will be available even in small memory dump, but just in theory :(
	*/
	if(ptrKeRegisterBugCheckReasonCallback){
		if(!ptrKeRegisterBugCheckReasonCallback(&bug_check_reason_record,
			(VOID *)BugCheckSecondaryDumpDataCallback,
			KbCallbackSecondaryDumpData,
			"UltraDefrag"))
				DebugPrint("=Ultradfg= Cannot register bug check dump data routine!\n",NULL);
	}
}

VOID __stdcall BugCheckCallback(PVOID Buffer,ULONG Length)
{
}

VOID __stdcall BugCheckSecondaryDumpDataCallback(
    KBUGCHECK_CALLBACK_REASON Reason,
    PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    PVOID ReasonSpecificData,
    ULONG ReasonSpecificDataLength)
{
	PKBUGCHECK_SECONDARY_DUMP_DATA p;
	
	p = (PKBUGCHECK_SECONDARY_DUMP_DATA)ReasonSpecificData;
	p->OutBuffer = dbg_buffer;
	p->OutBufferLength = DBG_BUFFER_SIZE;
	p->Guid = ultradfg_guid;
}

void __stdcall DeregisterBugCheckCallbacks(void)
{
	KeDeregisterBugCheckCallback(&bug_check_record);
	if(ptrKeDeregisterBugCheckReasonCallback)
		ptrKeDeregisterBugCheckReasonCallback(&bug_check_reason_record);
}

/*************************************************************************************************/
/* Since v2.1.0 this code is used to collect debugging messages. */

void __stdcall SaveLog();

BOOLEAN __stdcall OpenLog()
{
	BOOLEAN result = TRUE;

	/* This buffer must be in NonPaged pool to be accessible from BugCheckCallback's. */
	dbg_buffer = AllocatePool(NonPagedPool,DBG_BUFFER_SIZE); /* 32 kb ring-buffer */
	if(!dbg_buffer){
		result = FALSE;
		DbgPrint("=Ultradfg= Can't allocate dbg_buffer!\n");
	} else {
		memset((void *)dbg_buffer,0,DBG_BUFFER_SIZE);
		dbg_offset = 0;
	}
	KeInitializeEvent(&dbgprint_event,SynchronizationEvent,TRUE);
	return result;
}

void __stdcall CloseLog()
{
	if(dbg_buffer){
		if(KeGetCurrentIrql() == PASSIVE_LEVEL) SaveLog();
		Nt_ExFreePool(dbg_buffer);
		dbg_buffer = NULL;
	}
}

void __stdcall SaveLog()
{
	UNICODE_STRING us_log_path;
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS Status;
	IO_STATUS_BLOCK ioStatus;
	short error_msg[] = L"No enough memory to save debug messages. It requires 32 kb.";
	HANDLE hDbgLog;

	/* create the file */
	RtlInitUnicodeString(&us_log_path,log_path);
	/* OBJ_KERNEL_HANDLE is very important for Vista and later systems!!! */
	if(nt4_system){
		InitializeObjectAttributes(&ObjectAttributes,&us_log_path,0,NULL,NULL);
	} else {
		InitializeObjectAttributes(&ObjectAttributes,&us_log_path,OBJ_KERNEL_HANDLE,NULL,NULL);
	}
	Status = ZwCreateFile(&hDbgLog,FILE_APPEND_DATA | SYNCHRONIZE,&ObjectAttributes,&ioStatus,
			  NULL,FILE_ATTRIBUTE_NORMAL,FILE_SHARE_READ,FILE_OPEN_IF,
			  FILE_SYNCHRONOUS_IO_NONALERT,NULL,0);
	if(Status){
		DbgPrint("-Ultradfg- Can't create %ws: %x!\n",log_path,(UINT)Status);
		hDbgLog = NULL;
		return;
	}
	/* write ring buffer to the file */
	if(!dbg_buffer)	ZwWriteFile(hDbgLog,NULL,NULL,NULL,&ioStatus,
		error_msg,sizeof(error_msg) - sizeof(short),NULL,NULL);
	else ZwWriteFile(hDbgLog,NULL,NULL,NULL,&ioStatus,
		dbg_buffer,wcslen(dbg_buffer) << 1,NULL,NULL);
	/* close the file */
	ZwClose(hDbgLog);
}

BOOLEAN CheckForSystemVolume(void)
{
	char ch, ch2;
	
	ch = (UCHAR)toupper((int)volume_letter);
	ch2 = (UCHAR)toupper((int)log_path[4]);
	if(ch == ch2) return TRUE;
	return FALSE;
}

/*
* NOTE: Never call this function with parameters 
* from paged memory pool when IRQL >= DISPATCH_LEVEL!
*/
void __cdecl DebugPrint(char *format, short *ustring, ...)
{
	va_list ap;
	/* this buffer must be less than dbg_buffer */
	#define BUFFER_SIZE 1024 /* 512 */
	char buffer[BUFFER_SIZE];
	short bf[BUFFER_SIZE];
	short crlf[] = L"\r\n";
	signed long length; /* Signed!!! */
	NTSTATUS Status;
	int timeout_flag = 0;
	
	/* Wait for another DebugPrint calls to be finished */
	if(KeGetCurrentIrql() > DISPATCH_LEVEL) timeout_flag = 1;
	else {
		Status = KeWaitForSingleObject(&dbgprint_event,Executive,KernelMode,FALSE,NULL);
		if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status)){
			timeout_flag = 1;
			DbgPrint("-Ultradfg- dbgprint_event waiting failure!\n");
		}
	}

	/* Fucked nt 4.0 & w2k kernels don't contain _vsnwprintf() call! */
	va_start(ap,ustring);
	memset((void *)buffer,0,BUFFER_SIZE); /* required! */
	length = _vsnprintf(buffer,BUFFER_SIZE - 1,format,ap);
	va_end(ap);
	if(length < 0){
		buffer[BUFFER_SIZE - 2] = '\n';
		length = BUFFER_SIZE - 1;
	}
	buffer[BUFFER_SIZE - 1] = 0;
	/* remove the last new line character */
	if(length > 0){ buffer[length - 1] = 0; length--; }
	/* convert the string to unicode and append the ustring parameter */
	if(ustring)
		length = _snwprintf(bf,BUFFER_SIZE - 1,L"%S %s",buffer,ustring);
	else
		length = _snwprintf(bf,BUFFER_SIZE - 1,L"%S",buffer);
	if(length < 0) length = BUFFER_SIZE - 1;
	bf[BUFFER_SIZE - 1] = 0;

	/* send message to debugger */
	DbgPrint("%ws\n",bf);

	/*
	* We cannot write to a file if the previous 
	* operation is not completed.
	*/
	if(timeout_flag) goto cleanup;
	
	if(!dbg_buffer) goto cleanup;

	if(CheckForSystemVolume() == FALSE){
		if(!strcmp(format,"FLUSH_DBG_CACHE\n")) goto save_log;
		if(dbg_offset + (length << 1) >= DBG_BUFFER_SIZE - (2 << 1) - sizeof(short)){
		save_log:
			/* Save buffer contents to disc and reinitialize the ring buffer */
			if(KeGetCurrentIrql() == PASSIVE_LEVEL) SaveLog();
			memset((void *)dbg_buffer,0,DBG_BUFFER_SIZE);
			dbg_offset = 0;
		}
	} else {
		/*
		* We have defragmented the system volume. 
		* Never flush log data to disk to prevent file moving failure.
		* Write the new message to the beginning of the buffer instead.
		*/
		if(dbg_offset + (length << 1) >= DBG_BUFFER_SIZE - (2 << 1) - sizeof(short)){
			dbg_offset = 0;
		}
	}

	memcpy((void *)(dbg_buffer + (dbg_offset >> 1)),(void *)bf,length << 1);
	dbg_offset += (length << 1);
	memcpy((void *)(dbg_buffer + (dbg_offset >> 1)),(void *)crlf,2 * sizeof(short));
	dbg_offset += (2 << 1);
cleanup:
	if(KeGetCurrentIrql() > DISPATCH_LEVEL) return;
	KeSetEvent(&dbgprint_event,IO_NO_INCREMENT,FALSE);
}
