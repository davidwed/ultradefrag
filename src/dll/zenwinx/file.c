/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

/**
 * @file file.c
 * @brief File I/O code.
 * @addtogroup File
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

/**
 * @brief fopen() native equivalent.
 * @note Only r, w, a, r+, w+, a+ modes are supported.
 */
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

	DbgCheck2(filename,mode,"winx_fopen",NULL);

	RtlInitAnsiString(&as,filename);
	if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) != STATUS_SUCCESS){
		DebugPrint("Cannot open %s! Not enough memory!",filename);
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
	} else if(!strcmp(mode,"a")){
		access_mask = FILE_APPEND_DATA;
		disposition = FILE_OPEN_IF;
	} else if(!strcmp(mode,"a+")){
		access_mask = FILE_GENERIC_READ | FILE_APPEND_DATA;
		disposition = FILE_OPEN_IF;
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
		DebugPrintEx(status,"Cannot open %s",filename);
		return NULL;
	}
	f = (WINX_FILE *)winx_heap_alloc(sizeof(WINX_FILE));
	if(!f){
		NtClose(hFile);
		DebugPrint("Cannot open %s! Not enough memory!",filename);
		return NULL;
	}
	f->hFile = hFile;
	f->roffset.QuadPart = 0;
	f->woffset.QuadPart = 0;
	return f;
}

/**
 * @brief fread() native equivalent.
 */
size_t __stdcall winx_fread(void *buffer,size_t size,size_t count,WINX_FILE *f)
{
	NTSTATUS status;
	IO_STATUS_BLOCK iosb;
	
	DbgCheck2(buffer,f,"winx_fread",0);

	status = NtReadFile(f->hFile,NULL,NULL,NULL,&iosb,
			 buffer,size * count,&f->roffset,NULL);
	if(NT_SUCCESS(status)){
		status = NtWaitForSingleObject(f->hFile,FALSE,NULL);
		if(NT_SUCCESS(status)) status = iosb.Status;
	}
	if(status != STATUS_SUCCESS){
		DebugPrintEx(status,"Cannot read from a file");
		return 0;
	}
	if(iosb.Information == 0){ /* encountered on x64 XP */
		f->roffset.QuadPart += size * count;
		return count;
	}
	f->roffset.QuadPart += iosb.Information;
	return (iosb.Information / size);
}

/**
 * @brief fwrite() native equivalent.
 */
size_t __stdcall winx_fwrite(const void *buffer,size_t size,size_t count,WINX_FILE *f)
{
	NTSTATUS status;
	IO_STATUS_BLOCK iosb;
	
	DbgCheck2(buffer,f,"winx_fwrite",0);

	status = NtWriteFile(f->hFile,NULL,NULL,NULL,&iosb,
			 (void *)buffer,size * count,&f->woffset,NULL);
	if(NT_SUCCESS(status)){
		status = NtWaitForSingleObject(f->hFile,FALSE,NULL);
		if(NT_SUCCESS(status)) status = iosb.Status;
	}
	if(status != STATUS_SUCCESS){
		DebugPrintEx(status,"Cannot write to a file");
		return 0;
	}
	if(iosb.Information == 0){ /* encountered on x64 XP */
		f->woffset.QuadPart += size * count;
		return count;
	}
	f->woffset.QuadPart += iosb.Information;
	return (iosb.Information / size);
}

/**
 * @brief Sends an I/O control code to the specified device.
 * @param[in] f the file handle.
 * @param[in] code the IOCTL code.
 * @param[in] description the string explaining the meaning of the
 *                        request, used by error handling code.
 * @param[in] in_buffer the input buffer pointer.
 * @param[in] in_size the input buffer size, in bytes.
 * @param[out] out_buffer the output buffer pointer.
 * @param[in] out_size the output buffer size, in bytes.
 * @param[out] pbytes_returned pointer to the variable receiving
 *                             the number of bytes written to the
 *                             output buffer.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall winx_ioctl(WINX_FILE *f,
                         int code,char *description,
                         void *in_buffer,int in_size,
                         void *out_buffer,int out_size,
						 int *pbytes_returned)
{
	IO_STATUS_BLOCK iosb;
	NTSTATUS Status;

	DbgCheck1(f,"winx_ioctl",-1);
	
	if(out_buffer) RtlZeroMemory(out_buffer,out_size);
	
	if(pbytes_returned) *pbytes_returned = 0;
	if((code >> 16) == FILE_DEVICE_FILE_SYSTEM){ /* on x64? */
		Status = NtFsControlFile(f->hFile,NULL,NULL,NULL,
			&iosb,code,in_buffer,in_size,out_buffer,out_size);
	} else {
		Status = NtDeviceIoControlFile(f->hFile,NULL,NULL,NULL,
			&iosb,code,in_buffer,in_size,out_buffer,out_size);
	}
	if(NT_SUCCESS(Status)){
		Status = NtWaitForSingleObject(f->hFile,FALSE,NULL);
		if(NT_SUCCESS(Status)) Status = iosb.Status;
	}
	if(!NT_SUCCESS(Status)/* || Status == STATUS_PENDING*/){
		if(description)
			DebugPrintEx(Status,"%s failed",description);
		else
			DebugPrintEx(Status,"Ioctl %u failed",code);
		return (-1);
	}
	if(pbytes_returned) *pbytes_returned = iosb.Information;
	return 0;
}

/**
 * @brief fflush() native equivalent.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall winx_fflush(WINX_FILE *f)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK iosb;
	
	DbgCheck1(f,"winx_fflush",-1);

	Status = NtFlushBuffersFile(f->hFile,&iosb);
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"NtFlushBuffersFile() failed");
		return (-1);
	}
	return 0;
}

/**
 * @brief Retrieves the size of the file.
 * @param[in] f pointer to structure returned
 * by winx_fopen() call.
 * @return The size of the file, in bytes.
 */
ULONGLONG __stdcall winx_fsize(WINX_FILE *f)
{
	NTSTATUS status;
	IO_STATUS_BLOCK iosb;
	FILE_STANDARD_INFORMATION fsi;

	DbgCheck1(f,"winx_fsize",0);

	status = NtQueryInformationFile(f->hFile,&iosb,
		&fsi,sizeof(FILE_STANDARD_INFORMATION),
		FileStandardInformation);
	if(!NT_SUCCESS(status)){
		DebugPrintEx(status,"NtQueryInformationFile(FileStandardInformation) failed");
		return 0;
	}
	return fsi.EndOfFile.QuadPart;
}

/**
 * @brief fclose() native equivalent.
 */
void __stdcall winx_fclose(WINX_FILE *f)
{
	if(!f) return;
	if(f->hFile) NtClose(f->hFile);
	winx_heap_free(f);
}

/**
 * @brief Creates a directory.
 * @param[in] path the native path to the directory.
 * @return Zero for success, negative value otherwise.
 * @note If the requested directory already exists this function
 *       returns success.
 */
int __stdcall winx_create_directory(const char *path)
{
	ANSI_STRING as;
	UNICODE_STRING us;
	NTSTATUS status;
	HANDLE hFile;
	OBJECT_ATTRIBUTES oa;
	IO_STATUS_BLOCK iosb;

	DbgCheck1(path,"winx_create_directory",-1);

	RtlInitAnsiString(&as,path);
	if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) != STATUS_SUCCESS){
		DebugPrint("Cannot create %s! Not enough memory!",path);
		return (-1);
	}
	InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);

	status = NtCreateFile(&hFile,
			FILE_LIST_DIRECTORY | SYNCHRONIZE | FILE_OPEN_FOR_BACKUP_INTENT,
			&oa,
			&iosb,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			FILE_CREATE,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE,
			NULL,
			0
			);
	RtlFreeUnicodeString(&us);
	if(NT_SUCCESS(status)){
		NtClose(hFile);
		return 0;
	}
	/* if it already exists then return success */
	if(status == STATUS_OBJECT_NAME_COLLISION) return 0;
	DebugPrintEx(status,"Cannot create %s",path);
	return (-1);
}

/**
 * @brief Deletes a file.
 * @param[in] filename the native path to the file.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall winx_delete_file(const char *filename)
{
	ANSI_STRING as;
	UNICODE_STRING us;
	NTSTATUS status;
	OBJECT_ATTRIBUTES oa;

	DbgCheck1(filename,"winx_delete_file",-1);

	RtlInitAnsiString(&as,filename);
	if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) != STATUS_SUCCESS){
		DebugPrint("Cannot delete %s! Not enough memory!",filename);
		return (-1);
	}

	InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);
	status = NtDeleteFile(&oa);
	RtlFreeUnicodeString(&us);
	if(!NT_SUCCESS(status)){
		DebugPrintEx(status,"Cannot delete %s",filename);
		return (-1);
	}
	return 0;
}

/** @} */
