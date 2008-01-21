/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
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
*  Procedures for logging.
*/

#include <stdarg.h>
#include <stdio.h>

#include "driver.h"

char buffer[1024];

void DeleteLogFile(UDEFRAG_DEVICE_EXTENSION *dx)
{
	short p[] = L"\\??\\A:\\FRAGLIST.LUAR";
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS status;

	p[4] = (short)dx->letter;
	RtlInitUnicodeString(&dx->log_path,p);
	InitializeObjectAttributes(&ObjectAttributes,&dx->log_path,0,NULL,NULL);
	status = ZwDeleteFile(&ObjectAttributes);
	DebugPrint1("-Ultradfg- Report %ws deleted with status %x\n",p,(UINT)status);
}

static __inline void Write(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,
		   PVOID buffer,ULONG length,PLARGE_INTEGER pOffset)
{
	IO_STATUS_BLOCK ioStatus;

	ZwWriteFile(hFile,NULL,NULL,NULL,&ioStatus,
			buffer,length,pOffset,NULL);
	pOffset->QuadPart += length;
}

void WriteLogBody(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,
				  PLARGE_INTEGER pOffset,BOOLEAN is_filtered)
{
	PFRAGMENTED pf;
	char *p;
	int i;
	char *comment;
	ANSI_STRING as;

	for(pf = dx->fragmfileslist; pf != NULL; pf = pf->next_ptr){
		if(pf->pfn->is_filtered != is_filtered)
			continue;
		if(pf->pfn->is_dir) comment = "[DIR]";
		else if(pf->pfn->is_overlimit) comment = "[OVR]";
		else if(pf->pfn->is_compressed) comment = "[CMP]";
		else comment = " - ";
		sprintf(buffer,
			"\t{fragments = %u,size = %I64u,filtered = %u,"
			"comment = \"%s\",name = [[",
			(UINT)pf->pfn->n_fragments,
			pf->pfn->clusters_total * dx->bytes_per_cluster,
			(UINT)(pf->pfn->is_filtered & 0x1),
			comment
			);
		Write(dx,hFile,buffer,strlen(buffer),pOffset);
		RtlUnicodeStringToAnsiString(&as,&pf->pfn->name,TRUE);
		/* replace square brackets with <> !!! */
		for(i = 0; i < as.Length; i++){
			if(as.Buffer[i] == '[') as.Buffer[i] = '<';
			else if(as.Buffer[i] == ']') as.Buffer[i] = '>';
		}
		Write(dx,hFile,as.Buffer,as.Length,pOffset);
		RtlFreeAnsiString(&as);
		strcpy(buffer,"]],uname = {");
		Write(dx,hFile,buffer,strlen(buffer),pOffset);
		p = (char *)pf->pfn->name.Buffer;
		for(i = 0; i < pf->pfn->name.Length; i++){
			sprintf(buffer,/*"%03u,"*/"%u,",(UINT)p[i]);
			Write(dx,hFile,buffer,strlen(buffer),pOffset);
		}
		strcpy(buffer,"}},\r\n");
		Write(dx,hFile,buffer,strlen(buffer),pOffset);
	}
}

BOOLEAN SaveFragmFilesListToDisk(UDEFRAG_DEVICE_EXTENSION *dx)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK ioStatus;
	LARGE_INTEGER offset;
	NTSTATUS Status;
	HANDLE hFile;

	short p[] = L"\\??\\A:\\FRAGLIST.LUAR";

	if(dx->report_type.type == NO_REPORT)
		goto done;
	/* Create the file */
	p[4] = (short)dx->letter;
	RtlInitUnicodeString(&dx->log_path,p);
	InitializeObjectAttributes(&ObjectAttributes,&dx->log_path,0,NULL,NULL);
	Status = ZwCreateFile(&hFile,FILE_GENERIC_WRITE,&ObjectAttributes,&ioStatus,
			  NULL,0,FILE_SHARE_READ,FILE_OVERWRITE_IF,
			  0,NULL,0);
	if(Status){
		DebugPrint("-Ultradfg- Can't create %ws\n",p);
		return FALSE;
	}
	offset.QuadPart = 0;
	sprintf(buffer,
		"-- UltraDefrag report for volume %c:\r\n\r\n"
		"volume_letter = \"%c\"\r\n\r\n"
		"files = {\r\n",
		dx->letter, dx->letter
		);
	Write(dx,hFile,buffer,strlen(buffer),&offset);
	WriteLogBody(dx,hFile,&offset,FALSE);
	WriteLogBody(dx,hFile,&offset,TRUE);
	strcpy(buffer,"}\r\n");
	Write(dx,hFile,buffer,strlen(buffer),&offset);
	ZwClose(hFile);
	DebugPrint("-Ultradfg- Report saved to %ws\n",p);
done:
	return TRUE;
}

/*************************************************************************************************/

/* This code is obsolete and we have them only as an alternative for something unexpected. */
/* special code for debug messages saving on nt 4.0 */

/* Currently (since v1.3.0) NT4_DBG is always undefined */
#ifdef NT4_DBG

void __stdcall OpenLog()
{
	char header[] = "[Begin]\r\n";

	dbg_ring_buffer = NULL; dbg_ring_buffer_offset = 0;
	dbg_ring_buffer = AllocatePool(NonPagedPool,RING_BUFFER_SIZE); /* 512 kb ring-buffer */
	if(!dbg_ring_buffer) DbgPrint("=Ultradfg= Can't allocate ring buffer!\n");
	else memset(dbg_ring_buffer,0,RING_BUFFER_SIZE);
	if(dbg_ring_buffer){
		strcpy(dbg_ring_buffer,header);
		dbg_ring_buffer_offset += sizeof(header) - 1;
	}
}

void __stdcall CloseLog()
{
	UNICODE_STRING log_path;
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS Status;
	IO_STATUS_BLOCK ioStatus;
	char error_msg[] = "No enough memory to save debug messages. It requires 512 kb.";
	char terminator[] = "[End]\r\n";

	/* create the file */
	RtlInitUnicodeString(&log_path,L"\\??\\C:\\DBGPRINT.LOG");
	InitializeObjectAttributes(&ObjectAttributes,&log_path,0,NULL,NULL);
	ZwDeleteFile(&ObjectAttributes);
	Status = ZwCreateFile(&hDbgLog,FILE_GENERIC_WRITE,&ObjectAttributes,&ioStatus,
			  NULL,0,FILE_SHARE_READ,FILE_OVERWRITE_IF,
			  0,NULL,0);
	if(Status){
		DbgPrint("-Ultradfg- Can't create C:\\DBGPRINT.LOG!\n");
		return;
	}
	dbg_log_offset.QuadPart = 0;
	/* write ring buffer to the file */
	if(!dbg_ring_buffer){
		ZwWriteFile(hDbgLog,NULL,NULL,NULL,&ioStatus,
			error_msg,sizeof(error_msg) - 1,&dbg_log_offset,NULL);
	} else {
		memcpy(dbg_ring_buffer + dbg_ring_buffer_offset,terminator,sizeof(terminator) - 1);
		ZwWriteFile(hDbgLog,NULL,NULL,NULL,&ioStatus,
			dbg_ring_buffer,strlen(dbg_ring_buffer),
			&dbg_log_offset,NULL);
	}
	/* close the file */
	ZwClose(hDbgLog);
	if(dbg_ring_buffer) ExFreePool(dbg_ring_buffer);
}

void __cdecl WriteLogMessage(char *format, ...)
{
	char buffer[1024]; /* 512 */
	va_list ap;
	ULONG length;
	char crlf[] = "\r\n";
	char terminator[] = "[End]\r\n";

	va_start(ap,format);
	memset(buffer,0,1024); /* required! */
	length = _vsnprintf(buffer,sizeof(buffer),format,ap);
	if(length == -1){
		buffer[sizeof(buffer) - 2] = '\n';
		length = sizeof(buffer) - 1;
	}
	buffer[sizeof(buffer) - 1] = 0;
	//if(KeGetCurrentIrql() == PASSIVE_LEVEL)
	//{
		if(length != 0)
			length --; /* remove the last new line character */
		if(dbg_ring_buffer_offset + length >= RING_BUFFER_SIZE - (sizeof(terminator) - 1))
			dbg_ring_buffer_offset = 0;
		memcpy(dbg_ring_buffer + dbg_ring_buffer_offset,buffer,length);
		dbg_ring_buffer_offset += length;
		if(dbg_ring_buffer_offset + 2 >= RING_BUFFER_SIZE - (sizeof(terminator) - 1))
			dbg_ring_buffer_offset = 0;
		memcpy(dbg_ring_buffer + dbg_ring_buffer_offset,crlf,2);
		dbg_ring_buffer_offset += 2;
	//}
	/* and send message to debugger */
	DbgPrint(buffer);
	va_end(ap);
}

#endif /* NT4_DBG */
