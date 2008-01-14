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
 *  Save to logfile procedure.
 */

#include <stdarg.h>
#include <stdio.h>

#include "driver.h"

void DeleteLogFile(UDEFRAG_DEVICE_EXTENSION *dx)
{
	short p[] = L"\\??\\A:\\FRAGLIST.HTM";
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS status;

	p[4] = (short)dx->letter;
	RtlInitUnicodeString(&dx->log_path,p);
	InitializeObjectAttributes(&ObjectAttributes,&dx->log_path,0,NULL,NULL);
	status = ZwDeleteFile(&ObjectAttributes);
	DebugPrint1("-Ultradfg- Delete %ws status %x\n",p,(UINT)status);
}

void Write(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,
		   PVOID buffer,ULONG length,PLARGE_INTEGER pOffset)
{
	IO_STATUS_BLOCK ioStatus;
	UNICODE_STRING uStr;
	ANSI_STRING aStr;

	if(dx->report_type.format == ASCII_FORMAT)
	{
		RtlInitUnicodeString(&uStr,buffer);
		if(RtlUnicodeStringToAnsiString(&aStr,&uStr,TRUE) == \
			STATUS_SUCCESS)
		{
			ZwWriteFile(hFile,NULL,NULL,NULL,&ioStatus,
				aStr.Buffer,aStr.Length,pOffset,NULL);
			pOffset->QuadPart += aStr.Length;
			RtlFreeAnsiString(&aStr);
		}
	}
	else
	{
		ZwWriteFile(hFile,NULL,NULL,NULL,&ioStatus,
			buffer,length,pOffset,NULL);
		pOffset->QuadPart += length;
	}
}

void WriteLogBody(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,
				  PLARGE_INTEGER pOffset,BOOLEAN is_filtered)
{
	PFRAGMENTED pf;
	char buffer[32];
	ANSI_STRING aStr;
	UNICODE_STRING uStr;
	int length;
	short e1[] = L"<tr class=\"u\"><td class=\"c\">";
	short e2[] = L"</td><td>";
	short e3[] = L"</td></tr>\n";
	short dir[] = L"</td><td class=\"c\">[DIR]";
	short compr[] = L"</td><td class=\"c\">[CMP]";
	short over[] = L"</td><td class=\"c\">[OVR]";
	short normal[] = L"</td><td class=\"c\"> - ";

	pf = dx->fragmfileslist;
	while(pf)
	{
		if(pf->pfn->is_filtered != is_filtered)
			goto next;
		e1[11] = pf->pfn->is_filtered ? (short)'f': (short)'u';
#if 0
		Write(dx,hFile,pf->pfn->name.Buffer,pf->pfn->name.Length,pOffset);
		Write(dx,hFile,crlf,4,pOffset);
#endif
		Write(dx,hFile,e1,sizeof(e1) - 2,pOffset);
		/* because on NT 4.0 we don't have _itow: */
		_itoa(pf->pfn->n_fragments,buffer,10);
		RtlInitAnsiString(&aStr,buffer);
		RtlAnsiStringToUnicodeString(&uStr,&aStr,TRUE);
		length = wcslen(uStr.Buffer) << 1;
		Write(dx,hFile,uStr.Buffer,length,pOffset);
		RtlFreeUnicodeString(&uStr);
		Write(dx,hFile,e2,sizeof(e2) - 2,pOffset);
		Write(dx,hFile,pf->pfn->name.Buffer,pf->pfn->name.Length,pOffset);
		if(pf->pfn->is_dir)
			Write(dx,hFile,dir,sizeof(dir) - 2,pOffset);
		else if(pf->pfn->is_overlimit)
			Write(dx,hFile,over,sizeof(over) - 2,pOffset);
		else if(pf->pfn->is_compressed)
			Write(dx,hFile,compr,sizeof(compr) - 2,pOffset);
		else
			Write(dx,hFile,normal,sizeof(normal) - 2,pOffset);
		Write(dx,hFile,e3,sizeof(e3) - 2,pOffset);
next:
		pf = pf->next_ptr;
	}
}

BOOLEAN SaveFragmFilesListToDisk(UDEFRAG_DEVICE_EXTENSION *dx)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS Status;
	HANDLE hFile;
	IO_STATUS_BLOCK ioStatus;
//	short crlf[] = L"\r\n";
	LARGE_INTEGER offset;

	short p[] = L"\\??\\A:\\FRAGLIST.HTM";

	short head[] = L"<html><head><title>Fragmented files on C:</title>\n"
					L"<style>\ntd {font-family: monospace; font-size: 10pt}\n"
					L".c {text-align: center}\n.f {background-color:"
					L" #000000; color: #FFFFFF}\n</style></head>\n<body>\n"
					L"<center>\n<pre><h3>Fragmented files on C:</h3>\n"
					L"</pre><table border=\"1\" color=\"#FFAA55\" "
					L"cellspacing=\"0\" width=\"100%\">\n"
					L"<tr><td class=\"c\"># fragments</td>\n"
					L"<td class=\"c\">filename</td>\n"
					L"<td class=\"c\">comment</td></tr>\n";
	short end[] = L"</table>\n</center>\n</body></html>";

	if(dx->report_type.type == NO_REPORT)
		goto done;
	/* Create the file */
	p[4] = (short)dx->letter;
	RtlInitUnicodeString(&dx->log_path,p);
	InitializeObjectAttributes(&ObjectAttributes,&dx->log_path,0,NULL,NULL);
	Status = ZwCreateFile(&hFile,FILE_GENERIC_WRITE,&ObjectAttributes,&ioStatus,
			  NULL,0,FILE_SHARE_READ,FILE_OVERWRITE_IF,
			  0,NULL,0);
	if(Status)
	{
		DebugPrint("-Ultradfg- Can't create %ws\n",p);
		return FALSE;
	}
	offset.QuadPart = 0;
	head[39] = head[235] = (short)dx->letter;
	Write(dx,hFile,head,sizeof(head) - 2,&offset);
	WriteLogBody(dx,hFile,&offset,FALSE);
	WriteLogBody(dx,hFile,&offset,TRUE);
	Write(dx,hFile,end,sizeof(end) - 2,&offset);
	ZwClose(hFile);
done:
	return TRUE;
}

/*************************************************************************************************/

/* TODO: DbgPrint logger or this code? */

/* special code for debug messages saving on nt 4.0 */

#ifdef NT4_DBG

void __stdcall OpenLog()
{
	char header[] = "[Begin]\r\n";

	dbg_ring_buffer = NULL; dbg_ring_buffer_offset = 0;
	dbg_ring_buffer = AllocatePool(NonPagedPool,RING_BUFFER_SIZE); /* 512 kb ring-buffer */
	if(!dbg_ring_buffer) DbgPrint("=Ultradfg= Can't allocate ring buffer!\n");
	else memset(dbg_ring_buffer,0,RING_BUFFER_SIZE);
	if(dbg_ring_buffer)
	{
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

	/* Create the file */
	RtlInitUnicodeString(&log_path,L"\\??\\C:\\DBGPRINT.LOG");
	InitializeObjectAttributes(&ObjectAttributes,&log_path,0,NULL,NULL);
	ZwDeleteFile(&ObjectAttributes);
	Status = ZwCreateFile(&hDbgLog,FILE_GENERIC_WRITE,&ObjectAttributes,&ioStatus,
			  NULL,0,FILE_SHARE_READ,FILE_OVERWRITE_IF,
			  0,NULL,0);
	if(Status)
	{
		DbgPrint("-Ultradfg- Can't create C:\\DBGPRINT.LOG!\n");
		return;
	}
	dbg_log_offset.QuadPart = 0;
	/* write ring buffer to file */
	if(!dbg_ring_buffer)
	{
		ZwWriteFile(hDbgLog,NULL,NULL,NULL,&ioStatus,
			error_msg,sizeof(error_msg) - 1,&dbg_log_offset,NULL);
	}
	else
	{
		memcpy(dbg_ring_buffer + dbg_ring_buffer_offset,terminator,sizeof(terminator) - 1);
		ZwWriteFile(hDbgLog,NULL,NULL,NULL,&ioStatus,
			dbg_ring_buffer,strlen(dbg_ring_buffer),
			&dbg_log_offset,NULL);
	}
	/* close file */
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
	if(length == -1)
	{
		buffer[sizeof(buffer) - 2] = '\n';
		length = sizeof(buffer) - 1;
	}
	buffer[sizeof(buffer) - 1] = 0;
	//if(KeGetCurrentIrql() == PASSIVE_LEVEL)
	//{
		if(length != 0)
			length --; /* remove last new line character */
		//ZwWriteFile(hDbgLog,NULL,NULL,NULL,&ioStatus,
		//	buffer,length,&dbg_log_offset,NULL);
		//dbg_log_offset.QuadPart += length;
		//ZwWriteFile(hDbgLog,NULL,NULL,NULL,&ioStatus,
		//	crlf,2,&dbg_log_offset,NULL);
		//dbg_log_offset.QuadPart += 2;
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
