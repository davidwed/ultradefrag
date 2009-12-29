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

void DeleteLogFile(UDEFRAG_DEVICE_EXTENSION *dx)
{
	short p[] = L"\\??\\A:\\fraglist.luar";
	short p1[] = L"\\??\\A:\\fraglist.txt";
	short p2[] = L"\\??\\A:\\fraglist.htm";
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS status;

	p[4] = (short)dx->letter;
	RtlInitUnicodeString(&dx->log_path,p);
	InitializeObjectAttributes(&ObjectAttributes,&dx->log_path,
			OBJ_CASE_INSENSITIVE,NULL,NULL);
	status = ZwDeleteFile(&ObjectAttributes);
	DebugPrint1("-Ultradfg- Report %ws was deleted with status %x\n",p,(UINT)status);

	p1[4] = (short)dx->letter;
	RtlInitUnicodeString(&dx->log_path,p1);
	InitializeObjectAttributes(&ObjectAttributes,&dx->log_path,
			OBJ_CASE_INSENSITIVE,NULL,NULL);
	status = ZwDeleteFile(&ObjectAttributes);
	DebugPrint1("-Ultradfg- Report %ws was deleted with status %x\n",p1,(UINT)status);

	p2[4] = (short)dx->letter;
	RtlInitUnicodeString(&dx->log_path,p2);
	InitializeObjectAttributes(&ObjectAttributes,&dx->log_path,
			OBJ_CASE_INSENSITIVE,NULL,NULL);
	status = ZwDeleteFile(&ObjectAttributes);
	DebugPrint1("-Ultradfg- Report %ws was deleted with status %x\n",p2,(UINT)status);
}

void Write(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,PVOID buf,ULONG length)
{
	IO_STATUS_BLOCK ioStatus;
	NTSTATUS Status;

	Status = ZwWriteFile(hFile,NULL,NULL,NULL,&ioStatus,buf,length,NULL,NULL);
	if(NT_SUCCESS(Status)/* == STATUS_PENDING*/){
		//DebugPrint("-Ultradfg- Is waiting for write to logfile request completion.\n");
		if(nt4_system)
			Status = NtWaitForSingleObject(hFile,FALSE,NULL);
		else
			Status = ZwWaitForSingleObject(hFile,FALSE,NULL);
		if(NT_SUCCESS(Status)) Status = ioStatus.Status;
	}
}

#ifndef MICRO_EDITION
void WriteLogBody(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,
				  BOOLEAN is_filtered)
{
	PFRAGMENTED pf;
	char *p;
	int i, offset;
	char *comment;
	ANSI_STRING as;

	pf = dx->fragmfileslist; if(!pf) return;
	do {
		if(pf->pfn->is_filtered != is_filtered)
			goto next_item;
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
			
			/* skip \??\ sequence in the beginning */
			if(as.Length > 0x4)
				Write(dx,hFile,(void *)((char *)(as.Buffer) + 0x4),as.Length - 0x4);
			else
				Write(dx,hFile,as.Buffer,as.Length);
			
			RtlFreeAnsiString(&as);
		} else {
			DebugPrint("-Ultradfg- no enough memory for WriteLogBody()!\n");
		}
		strcpy(buffer,"]],uname = {");
		Write(dx,hFile,buffer,strlen(buffer));
		p = (char *)pf->pfn->name.Buffer;
		/* skip \??\ sequence in the beginning */
		if(pf->pfn->name.Length > 0x8) offset = 0x8;
		else offset = 0x0;
		for(i = offset; i < (pf->pfn->name.Length - offset); i++){
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
	ULONG attr;

	short p[] = L"\\??\\A:\\fraglist.luar";

	if(dx->disable_reports) return TRUE;
	/* Create the file */
	p[4] = (short)dx->letter;
	RtlInitUnicodeString(&dx->log_path,p);
	attr = OBJ_CASE_INSENSITIVE;
	if(!nt4_system) attr |= OBJ_KERNEL_HANDLE; /* !!! */
	InitializeObjectAttributes(&ObjectAttributes,
			&dx->log_path,
			attr,
			NULL,
			NULL
			);
	Status = ZwCreateFile(&hFile,
			FILE_APPEND_DATA | SYNCHRONIZE,
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
		DebugPrint("-Ultradfg- Can't create %ws file: %x\n",p,(UINT)Status);
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

	/* this code returns UserMode!!! */
	/*if(ExGetPreviousMode() == KernelMode)
		DbgPrint("ExGetPreviousMode returned KernelMode\n");
	else
		DbgPrint("ExGetPreviousMode returned UserMode\n");*/
	
	ZwClose(hFile);
	
	DebugPrint("-Ultradfg- Report saved to %ws\n",p);
	return TRUE;
}
#else /* MICRO_EDITION */
/*
* This code was designed especially for Micro Edition.
* It saves report in plain text format.
*/
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
		if(pf->pfn->is_filtered != is_filtered)
			goto next_item;
		/* because on NT 4.0 we don't have _itow: */
		_itoa(pf->pfn->n_fragments,buffer,10);
		RtlInitAnsiString(&as,buffer);
		if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) != STATUS_SUCCESS){
			DebugPrint("-Ultradfg- no enough memory for WriteLogBody()!\n");
			return;
		}
		Write(dx,hFile,us.Buffer,us.Length);
		RtlFreeUnicodeString(&us);
		Write(dx,hFile,e1,sizeof(e1) - sizeof(short));
		/* skip \??\ sequence in the beginning */
		if(pf->pfn->name.Length > 0x8)
			Write(dx,hFile,(void *)((char *)(pf->pfn->name.Buffer) + 0x8),pf->pfn->name.Length - 0x8);
		else
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
	ULONG attr;

	short p[] = L"\\??\\A:\\fraglist.txt";
	short head[] = L"\r\nFragmented files on C:\r\n\r\n";
	short line[] = L"\r\n;-----------------------------------------------------------------\r\n\r\n";

	if(dx->disable_reports) return TRUE;
	/* Create the file */
	p[4] = (short)dx->letter;
	RtlInitUnicodeString(&dx->log_path,p);
	attr = OBJ_CASE_INSENSITIVE;
	if(!nt4_system) attr |= OBJ_KERNEL_HANDLE; /* !!! */
	InitializeObjectAttributes(&ObjectAttributes,
			&dx->log_path,
			attr,
			NULL,
			NULL
			);
	Status = ZwCreateFile(&hFile,
			FILE_APPEND_DATA | SYNCHRONIZE,
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
		DebugPrint("-Ultradfg- Can't create %ws file: %x\n",p,(UINT)Status);
		hFile = NULL;
		return FALSE;
	}

	head[22] = (short)dx->letter;
	Write(dx,hFile,head,sizeof(head) - sizeof(short));
	WriteLogBody(dx,hFile,FALSE);
	Write(dx,hFile,line,sizeof(line) - sizeof(short));
	WriteLogBody(dx,hFile,TRUE);
	ZwClose(hFile);
	DebugPrint("-Ultradfg- Report saved to %ws\n",p);
	return TRUE;
}
#endif /* MICRO_EDITION */
