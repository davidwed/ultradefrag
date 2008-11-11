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
* zenwinx.dll crt's file functions analogs.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <stdio.h>

#include "ntndk.h"
#include "zenwinx.h"

/* only r, w, r+, w+ modes are supported */
WINX_FILE * __stdcall winx_fopen(const char *filename,const char *mode)
{
	ANSI_STRING as;
	UNICODE_STRING us;
	NTSTATUS status;
	HANDLE hFile;
	OBJECT_ATTRIBUTES oa;
	IO_STATUS_BLOCK iosb;
	ACCESS_MASK access_mask = FILE_GENERIC_READ;
	ULONG disposition = FILE_OPEN;
	WINX_FILE *f;

	if(!filename){
		winx_push_error("Can't open a file because name isn't specified!");
		return NULL;
	}
	RtlInitAnsiString(&as,filename);
	if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) != STATUS_SUCCESS){
		winx_push_error("Can't open %s! No enough memory!",filename);
		return NULL;
	}
	InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);

	if(!strcmp(mode,"r")){
		access_mask = FILE_GENERIC_READ;
		disposition = FILE_OPEN;
	} else if(!strcmp(mode,"w")){
		access_mask = FILE_GENERIC_WRITE;
		disposition = FILE_OVERWRITE_IF;
	} else if(!strcmp(mode,"r+")){
		access_mask = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
		disposition = FILE_OPEN;
	} else if(!strcmp(mode,"w+")){
		access_mask = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
		disposition = FILE_OVERWRITE_IF;
	}
	access_mask |= SYNCHRONIZE;

	status = NtCreateFile(&hFile,
			access_mask,
			&oa,
			&iosb,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			disposition,
			FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0
			);
	RtlFreeUnicodeString(&us);
	if(status != STATUS_SUCCESS){
		winx_push_error("Can't open %s: %x!",filename,(UINT)status);
		return NULL;
	}
	f = (WINX_FILE *)winx_virtual_alloc(sizeof(WINX_FILE));
	if(!f){
		NtClose(hFile);
		winx_push_error("Can't open %s! No enough memory!",filename);
		return NULL;
	}
	f->hFile = hFile;
	f->roffset.QuadPart = 0;
	f->woffset.QuadPart = 0;
	return f;
}

size_t __stdcall winx_fread(void *buffer,size_t size,size_t count,WINX_FILE *f)
{
	NTSTATUS status;
	IO_STATUS_BLOCK iosb;
	
	if(!buffer){
		winx_push_error("Can't read from a file because buffer isn't specified!");
		return 0;
	}
	status = NtReadFile(f->hFile,NULL,NULL,NULL,&iosb,
			 buffer,size * count,&f->roffset,NULL);
	if(status == STATUS_PENDING){
		status = NtWaitForSingleObject(f->hFile,FALSE,NULL);
		if(NT_SUCCESS(status)) status = iosb.Status;
	}
	if(status == STATUS_END_OF_FILE || (iosb.Information < size)){
		winx_push_error("EOF!");
		return 0;
	}
	if(status != STATUS_SUCCESS){
		winx_push_error("Can't read from a file: %x!",(UINT)status);
		return 0;
	}
	f->roffset.QuadPart += iosb.Information;//size * count;
	return (iosb.Information / size);//count;
}

size_t __stdcall winx_fwrite(const void *buffer,size_t size,size_t count,WINX_FILE *f)
{
	NTSTATUS status;
	IO_STATUS_BLOCK iosb;
	
	if(!buffer){
		winx_push_error("Can't write to a file because buffer isn't specified!");
		return 0;
	}
	status = NtWriteFile(f->hFile,NULL,NULL,NULL,&iosb,
			 (void *)buffer,size * count,&f->woffset,NULL);
	if(status == STATUS_PENDING){
		status = NtWaitForSingleObject(f->hFile,FALSE,NULL);
		if(NT_SUCCESS(status)) status = iosb.Status;
	}
	if(status != STATUS_SUCCESS/* || (iosb.Information < size)*/){
		winx_push_error("Can't write to a file: %x!",(UINT)status);
		return 0;
	}
	f->woffset.QuadPart += iosb.Information;//size * count;

	/* FIXME: iosb,Information may be zero though a write call was successful */
	if(iosb.Information < size) return count;

	return (iosb.Information / size);//count;
}

/* 0 on success, -1 otherwise */
int __stdcall winx_ioctl(WINX_FILE *f,
                         int code,char *description,
                         void *in_buffer,int in_size,
                         void *out_buffer,int out_size,
						 int *pbytes_returned)
{
	IO_STATUS_BLOCK iosb;
	NTSTATUS Status;

	if(!f){
		winx_push_error("Invalid parameter!");
		return (-1);
	}
	
	if(pbytes_returned) *pbytes_returned = 0;
	if((code >> 16) == FILE_DEVICE_FILE_SYSTEM){ /* on x64? */
		Status = NtFsControlFile(f->hFile,NULL,NULL,NULL,
			&iosb,code,in_buffer,in_size,out_buffer,out_size);
	} else {
		Status = NtDeviceIoControlFile(f->hFile,NULL,NULL,NULL,
			&iosb,code,in_buffer,in_size,out_buffer,out_size);
	}
	if(Status == STATUS_PENDING){
		Status = NtWaitForSingleObject(f->hFile,FALSE,NULL);
		if(NT_SUCCESS(Status)) Status = iosb.Status;
	}
	if(!NT_SUCCESS(Status) || Status == STATUS_PENDING){
		if(description)
			winx_push_error("%s failed: %x!",description,(UINT)Status);
		else
			winx_push_error("Ioctl %u failed: %x!",code,(UINT)Status);
		return (-1);
	}
	if(pbytes_returned) *pbytes_returned = iosb.Information;
	return 0;
}
						 
void __stdcall winx_fclose(WINX_FILE *f)
{
	if(!f) return;
	if(f->hFile) NtClose(f->hFile);
	winx_virtual_free(f,sizeof(WINX_FILE));
}
