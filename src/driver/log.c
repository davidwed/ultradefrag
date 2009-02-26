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
*  Procedures for logging.
*/

#include <stdarg.h>
#include <stdio.h>

#include "driver.h"

char buffer[1024];

#ifndef MICRO_EDITION
void DeleteLogFile(UDEFRAG_DEVICE_EXTENSION *dx)
{
	short p[] = L"\\??\\A:\\fraglist.luar";
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS status;

	p[4] = (short)dx->letter;
	RtlInitUnicodeString(&dx->log_path,p);
	InitializeObjectAttributes(&ObjectAttributes,
			&dx->log_path,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);
	status = ZwDeleteFile(&ObjectAttributes);
	DebugPrint1("-Ultradfg- Report was deleted with status %x\n",p,(UINT)status);
}

void Write(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,
		   PVOID buf,ULONG length/*,PLARGE_INTEGER pOffset*/)
{
	IO_STATUS_BLOCK ioStatus;
	NTSTATUS Status;

	Status = ZwWriteFile(hFile,NULL,NULL,NULL,&ioStatus,
			buf,length,/*pOffset*/NULL,NULL);
	//pOffset->QuadPart += length;
	if(Status == STATUS_PENDING){
		DebugPrint("-Ultradfg- Is waiting for write to logfile request completion.\n",NULL);
		Status = NtWaitForSingleObject(hFile,FALSE,NULL);
		if(NT_SUCCESS(Status)) Status = ioStatus.Status;
	}
}

void WriteLogBody(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,
				  BOOLEAN is_filtered)
{
	PFRAGMENTED pf;
	char *p;
	int i;
	char *comment;
	ANSI_STRING as;

	pf = dx->fragmfileslist; if(!pf) return;
	do {
	//for(pf = dx->fragmfileslist; pf != NULL; pf = pf->next_ptr){
		if(pf->pfn->is_filtered != is_filtered)
			goto next_item;//continue;
		if(pf->pfn->is_dir) comment = "[DIR]";
		else if(pf->pfn->is_overlimit) comment = "[OVR]";
		else if(pf->pfn->is_compressed) comment = "[CMP]";
		else comment = " - ";
		_snprintf(buffer, sizeof(buffer),
			"\t{fragments = %u,size = %I64u,filtered = %u,"
			"comment = \"%s\",name = [[",
			(UINT)pf->pfn->n_fragments,
			pf->pfn->clusters_total * dx->bytes_per_cluster,
			(UINT)(pf->pfn->is_filtered & 0x1),
			comment
			);
		buffer[sizeof(buffer) - 1] = 0; /* to be sure that the buffer is terminated by zero */
		Write(dx,hFile,buffer,strlen(buffer));

		if(RtlUnicodeStringToAnsiString(&as,&pf->pfn->name,TRUE) == STATUS_SUCCESS){
			/* replace square brackets with <> !!! */
			for(i = 0; i < as.Length; i++){
				if(as.Buffer[i] == '[') as.Buffer[i] = '<';
				else if(as.Buffer[i] == ']') as.Buffer[i] = '>';
			}
			Write(dx,hFile,as.Buffer,as.Length);
			RtlFreeAnsiString(&as);
		} else {
			DebugPrint("-Ultradfg- no enough memory for WriteLogBody()!\n",NULL);
		}
		strcpy(buffer,"]],uname = {");
		Write(dx,hFile,buffer,strlen(buffer));
		p = (char *)pf->pfn->name.Buffer;
		for(i = 0; i < pf->pfn->name.Length; i++){
			_snprintf(buffer,sizeof(buffer),/*"%03u,"*/"%u,",(UINT)p[i]);
			buffer[sizeof(buffer) - 1] = 0; /* to be sure that the buffer is terminated by zero */
			Write(dx,hFile,buffer,strlen(buffer));
		}
		strcpy(buffer,"}},\r\n");
		Write(dx,hFile,buffer,strlen(buffer));
	next_item:
		pf = pf->next_ptr;
	} while(pf != dx->fragmfileslist);
}

BOOLEAN SaveFragmFilesListToDisk(UDEFRAG_DEVICE_EXTENSION *dx)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK ioStatus;
	NTSTATUS Status;
	HANDLE hFile;

	short p[] = L"\\??\\A:\\fraglist.luar";

	if(dx->disable_reports) return TRUE;
	/* Create the file */
	p[4] = (short)dx->letter;
	RtlInitUnicodeString(&dx->log_path,p);
	InitializeObjectAttributes(&ObjectAttributes,
			&dx->log_path,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);
	Status = ZwCreateFile(&hFile,
			/*FILE_GENERIC_WRITE*/FILE_APPEND_DATA | SYNCHRONIZE,
			&ObjectAttributes,
			&ioStatus,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ,
			FILE_OVERWRITE_IF,
			FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0
			);
	if(Status){
		DebugPrint("-Ultradfg- Can't create file: %x\n",p,(UINT)Status);
		hFile = NULL;
		return FALSE;
	}
	_snprintf(buffer,sizeof(buffer),
		"-- UltraDefrag report for volume %c:\r\n\r\n"
		"volume_letter = \"%c\"\r\n\r\n"
		"files = {\r\n",
		dx->letter, dx->letter
		);
	buffer[sizeof(buffer) - 1] = 0; /* to be sure that the buffer is terminated by zero */
	Write(dx,hFile,buffer,strlen(buffer));
	
	WriteLogBody(dx,hFile,FALSE);
	WriteLogBody(dx,hFile,TRUE);
	strcpy(buffer,"}\r\n");
	Write(dx,hFile,buffer,strlen(buffer));
	ZwClose(hFile);
	DebugPrint("-Ultradfg- Report saved to\n",p);
	return TRUE;
}
#else /* MICRO_EDITION */
/*
* This code was designed especially for Micro Edition.
* It saves report in plain text format.
*/
void DeleteLogFile(UDEFRAG_DEVICE_EXTENSION *dx)
{
	short p[] = L"\\??\\A:\\fraglist.txt";
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS status;

	p[4] = (short)dx->letter;
	RtlInitUnicodeString(&dx->log_path,p);
	InitializeObjectAttributes(&ObjectAttributes,
			&dx->log_path,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);
	status = ZwDeleteFile(&ObjectAttributes);
	DebugPrint1("-Ultradfg- Report was deleted with status %x\n",p,(UINT)status);
}

void Write(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,
		   PVOID buf,ULONG length/*,PLARGE_INTEGER pOffset*/)
{
	IO_STATUS_BLOCK ioStatus;
	NTSTATUS Status;

	Status = ZwWriteFile(hFile,NULL,NULL,NULL,&ioStatus,
			buf,length,/*pOffset*/NULL,NULL);
	//pOffset->QuadPart += length;
	if(Status == STATUS_PENDING){
		DebugPrint("-Ultradfg- Is waiting for write to logfile request completion.",NULL);
		Status = NtWaitForSingleObject(hFile,FALSE,NULL);
		if(NT_SUCCESS(Status)) Status = ioStatus.Status;
	}
}

void WriteLogBody(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,
				  BOOLEAN is_filtered)
{
	PFRAGMENTED pf;
	ANSI_STRING as;
	UNICODE_STRING us;
	short e1[] = L"\t";
	short e2[] = L"\r\n";

	pf = dx->fragmfileslist; if(!pf) return;
	do {
	//for(pf = dx->fragmfileslist; pf != NULL; pf = pf->next_ptr){
		if(pf->pfn->is_filtered != is_filtered)
			goto next_item;//continue;
		/* because on NT 4.0 we don't have _itow: */
		_itoa(pf->pfn->n_fragments,buffer,10);
		RtlInitAnsiString(&as,buffer);
		if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) != STATUS_SUCCESS){
			DebugPrint("-Ultradfg- no enough memory for WriteLogBody()!\n",NULL);
			return;
		}
		Write(dx,hFile,us.Buffer,us.Length);
		RtlFreeUnicodeString(&us);
		Write(dx,hFile,e1,sizeof(e1) - sizeof(short));
		Write(dx,hFile,pf->pfn->name.Buffer,pf->pfn->name.Length);
		Write(dx,hFile,e2,sizeof(e2) - sizeof(short));
	next_item:
		pf = pf->next_ptr;
	} while(pf != dx->fragmfileslist);
}

BOOLEAN SaveFragmFilesListToDisk(UDEFRAG_DEVICE_EXTENSION *dx)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK ioStatus;
	NTSTATUS Status;
	HANDLE hFile;

	short p[] = L"\\??\\A:\\fraglist.txt";
	short head[] = L"\r\nFragmented files on C:\r\n\r\n";
	short line[] = L"\r\n;-----------------------------------------------------------------\r\n\r\n";

	if(dx->disable_reports) return TRUE;
	/* Create the file */
	p[4] = (short)dx->letter;
	RtlInitUnicodeString(&dx->log_path,p);
	InitializeObjectAttributes(&ObjectAttributes,
			&dx->log_path,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);
	Status = ZwCreateFile(&hFile,
			/*FILE_GENERIC_WRITE*/FILE_APPEND_DATA | SYNCHRONIZE,
			&ObjectAttributes,
			&ioStatus,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ,
			FILE_OVERWRITE_IF,
			FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0
			);
	if(Status){
		DebugPrint("-Ultradfg- Can't create file: %x\n",p,(UINT)Status);
		hFile = NULL;
		return FALSE;
	}

	head[22] = (short)dx->letter;
	Write(dx,hFile,head,sizeof(head) - sizeof(short));
	WriteLogBody(dx,hFile,FALSE);
	Write(dx,hFile,line,sizeof(line) - sizeof(short));
	WriteLogBody(dx,hFile,TRUE);
	ZwClose(hFile);
	DebugPrint("-Ultradfg- Report saved to\n",p);
	return TRUE;
}
#endif /* MICRO_EDITION */

/*************************************************************************************************/
/* Since v2.1.0 this code is used to collect debugging messages. */

void __stdcall OpenLog()
{
	dbg_buffer = AllocatePool(NonPagedPool,DBG_BUFFER_SIZE); /* 32 kb ring-buffer */
	if(!dbg_buffer) DbgPrint("=Ultradfg= Can't allocate dbg_buffer!\n");
	else{
		memset((void *)dbg_buffer,0,DBG_BUFFER_SIZE);
		dbg_offset = 0;
	}
	KeInitializeEvent(&dbgprint_event,SynchronizationEvent,TRUE);
}

void __stdcall CloseLog()
{
	if(dbg_buffer){
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
	InitializeObjectAttributes(&ObjectAttributes,&us_log_path,0,NULL,NULL);
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

	if(!strcmp(format,"FLUSH_DBG_CACHE\n")) goto save_log;
	if(dbg_offset + (length << 1) >= DBG_BUFFER_SIZE - (2 << 1) - sizeof(short)){
	save_log:
		/* Save buffer contents to disc and reinitialize the ring buffer */
		if(KeGetCurrentIrql() == PASSIVE_LEVEL) SaveLog();
		memset((void *)dbg_buffer,0,DBG_BUFFER_SIZE);
		dbg_offset = 0;
	}
	memcpy((void *)(dbg_buffer + (dbg_offset >> 1)),(void *)bf,length << 1);
	dbg_offset += (length << 1);
	memcpy((void *)(dbg_buffer + (dbg_offset >> 1)),(void *)crlf,2 * sizeof(short));
	dbg_offset += (2 << 1);
cleanup:
	if(KeGetCurrentIrql() > DISPATCH_LEVEL) return;
	KeSetEvent(&dbgprint_event,IO_NO_INCREMENT,FALSE);
}
