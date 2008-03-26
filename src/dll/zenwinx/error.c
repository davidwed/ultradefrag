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
* zenwinx.dll error handling routines.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#include "ntndk.h"
#include "zenwinx.h"

#define FormatMessageFound		0
#define FormatMessageNotFound	1
#define FormatMessageUndefined	2

long malloc_free_delta = 0;

const char no_mem[] = "Unknown state! No enough memory!";

int FormatMessageState = FormatMessageUndefined;
DWORD (WINAPI *func_FormatMessageA)(DWORD,PVOID,DWORD,DWORD,LPSTR,DWORD,va_list*);
DWORD (WINAPI *func_FormatMessageW)(DWORD,PVOID,DWORD,DWORD,LPWSTR,DWORD,va_list*);
int FindFormatMessage(void);

/****f* zenwinx.error/winx_get_error_description
* NAME
*    winx_get_error_description
* SYNOPSIS
*    winx_get_error_description(code);
* FUNCTION
*    Returns a text description for 
*    specified NTSTATUS code.
* INPUTS
*    code - NTSTATUS code.
* RESULT
*    Pointer to string with error description.
* EXAMPLE
*    str = winx_get_error_description(STATUS_ACCESS_VIOLATION);
* NOTES
*    This function returns messages only for well
*    known codes. Otherwise it returns predefined
*    "Unknown error; send report to the authors" message.
* SEE ALSO
*    winx_push_error,winx_pop_error,winx_pop_werror,
*    winx_save_error,winx_restore_error
******/
static char * __stdcall winx_get_error_description(unsigned long code)
{
	NTSTATUS Status;
	char *msg = "Unknown error; send report to the authors";
	
	Status = (NTSTATUS)code;
	switch(Status){
	case STATUS_SUCCESS:
		msg = "Operation was successful";
		break;
	case STATUS_OBJECT_NAME_INVALID:
	case STATUS_OBJECT_NAME_NOT_FOUND:
	case STATUS_OBJECT_NAME_COLLISION:
	case STATUS_OBJECT_PATH_INVALID:
	case STATUS_OBJECT_PATH_NOT_FOUND:
	case STATUS_OBJECT_PATH_SYNTAX_BAD:
		msg = "Path not found";
		break;
	case STATUS_BUFFER_TOO_SMALL:
		msg = "Buffer is too small";
		break;
	case STATUS_ACCESS_DENIED:
		msg = "Access denied";
		break;
	case STATUS_NO_MEMORY:
		msg = "No enough memory";
		break;
	case STATUS_UNSUCCESSFUL:
		msg = "Operation was unsuccessful";
		break;
	case STATUS_NOT_IMPLEMENTED:
		msg = "Operation is not implemented";
		break;
	case STATUS_INVALID_INFO_CLASS:
		msg = "Invalid info class";
		break;
	case STATUS_INFO_LENGTH_MISMATCH:
		msg = "Info length mismatch";
		break;
	case STATUS_ACCESS_VIOLATION:
		msg = "Access violation";
		break;
	case STATUS_INVALID_HANDLE:
		msg = "Invalid handle";
		break;
	case STATUS_INVALID_PARAMETER:
		msg = "Invalid parameter";
		break;
	case STATUS_NO_SUCH_DEVICE:
		msg = "Device not found";
		break;
	case STATUS_NO_SUCH_FILE:
		msg = "File not found";
		break;
	case STATUS_INVALID_DEVICE_REQUEST:
		msg = "Invalid device request";
		break;
	case STATUS_END_OF_FILE:
		msg = "End of file encountered";
		break;
	case STATUS_WRONG_VOLUME:
		msg = "Volume is wrong";
		break;
	case STATUS_NO_MEDIA_IN_DEVICE:
		msg = "No media in device";
		break;
	}
	return msg;
}

/****f* zenwinx.error/winx_push_error
* NAME
*    winx_push_error
* SYNOPSIS
*    winx_push_error(format, ...);
* FUNCTION
*    Saves formatted error message.
* INPUTS
*    format - format string
*    ...    - parameters
* RESULT
*    This function does not return a value.
* EXAMPLE
*    winx_push_error("Can't open the file %s: %x!",
*            filename, (UINT)Status);
* NOTES
*    1. If some winx function returns negative value,
*       such function must call winx_push_error() before.
*    2. After each unsuccessful call of such functions,
*       winx_pop_error() must be called before any other
*       system call in the current thread (win32, 
*       native, crt, mfc ...).
*    3. Each winx_push_error() call must be like this:
*       winx_push_error("error message with parameters: %x!",
*           parameters,(UINT)Status);
*       Status parameter is optional.
*    4. Don't use ": %x!" substring to purposes other than
*       operation status definition.
* SEE ALSO
*    winx_get_error_description,winx_pop_error,winx_pop_werror,
*    winx_save_error,winx_restore_error
******/
void __cdecl winx_push_error(char *format, ...)
{
	char *buffer;
	va_list arg;
	int length;
	
	buffer = winx_virtual_alloc(ERR_MSG_SIZE);
	if(!buffer){
		NtCurrentTeb()->LastErrorValue = (ULONG)(LONG_PTR)no_mem;
		return;
	}
	//InterlockedIncrement(&malloc_free_delta);
	malloc_free_delta ++;
	va_start(arg,format);
	memset(buffer,0,ERR_MSG_SIZE);
	length = _vsnprintf(buffer,ERR_MSG_SIZE - 1,format,arg);
	buffer[ERR_MSG_SIZE - 1] = 0;
	va_end(arg);
	NtCurrentTeb()->LastErrorValue = (ULONG)(LONG_PTR)buffer;
}

/****f* zenwinx.error/winx_pop_error
* NAME
*    winx_pop_error
* SYNOPSIS
*    winx_pop_error(buffer, size);
* FUNCTION
*    Retrieves formatted error message
*    and removes it from stack.
* INPUTS
*    buffer - memory block to store message into
*    size   - maximum number of characters to store,
*             including terminal zero
* RESULT
*    This function does not return a value.
* EXAMPLE
*    if(winx_xxx() < 0){
*        winx_pop_error(buffer,sizeof(buffer));
*    }
* NOTES
*    1. See notes for winx_push_error() function.
*    2. The first parameter may be NULL if you don't
*       need error message.
* SEE ALSO
*    winx_push_error,winx_pop_werror,winx_get_error_description,
*    winx_save_error,winx_restore_error
******/
void __stdcall winx_pop_error(char *buffer, int size)
{
	int length, length2;
	char *source;
	char src[ERR_MSG_SIZE];
	char desc[256];
	char *p;
	UINT iStatus;
	NTSTATUS Status;
	ULONG err_code;
	
	source = (char *)(LONG_PTR)(NtCurrentTeb()->LastErrorValue);
	strcpy(src,source);
	if(strcmp(source,no_mem) != 0){
		winx_virtual_free(source,ERR_MSG_SIZE);
		//InterlockedDecrement(&malloc_free_delta);
		malloc_free_delta --;
	}

	if(buffer && size > 0){
		length = min(strlen(src),(unsigned int)size - 1);
		strncpy(buffer,src,length);
		buffer[length] = 0;
		/* append error description */
		p = strrchr(buffer,':');
		if(p){
			/* skip ':' and spaces */
			p ++;
			while(*p == 0x20) p++;
			sscanf(p,"%x",&iStatus);
			/* very important for x64 targets */
			Status = (NTSTATUS)(signed int)iStatus;
			strcpy(desc,"\r\n");
			/* try to get FormatMessage function address */
			if(FindFormatMessage()){
				/* append localized error description from kernel32.dll */
				err_code = RtlNtStatusToDosError(Status);
				if(!func_FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err_code,
					MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
					desc + 2,sizeof(desc) - 10,NULL))
				{
					goto get_predefined_msg;
				}
			} else {
			get_predefined_msg:
				strcpy(desc + 2,winx_get_error_description((unsigned long)Status));
			}
			length2 = min(strlen(desc),(unsigned int)size - length - 1);
			strncpy(buffer + length,desc,length2);
			buffer[length + length2] = 0;
		}
	}
}

/****f* zenwinx.error/winx_pop_werror
* NAME
*    winx_pop_werror
* SYNOPSIS
*    winx_pop_werror(buffer, size);
* FUNCTION
*    Unicode version of winx_pop_error().
* SEE ALSO
*    winx_push_error,winx_pop_error,winx_get_error_description,
*    winx_save_error,winx_restore_error
******/
void __stdcall winx_pop_werror(short *buffer, int size)
{
	int length, length2;
	char *source;
	char src[ERR_MSG_SIZE];
	char c_buffer[ERR_MSG_SIZE];
	short desc[256];
	char *p;
	UINT iStatus;
	NTSTATUS Status;
	ULONG err_code;
	ANSI_STRING aStr;
	UNICODE_STRING uStr;
	short fmt_error[] = L"Can't format message!";

	source = (char *)(LONG_PTR)(NtCurrentTeb()->LastErrorValue);
	strcpy(src,source);
	if(strcmp(source,no_mem) != 0){
		winx_virtual_free(source,ERR_MSG_SIZE);
		//InterlockedDecrement(&malloc_free_delta);
	}

	if(buffer && size > 0){
		length = min(strlen(src),ERR_MSG_SIZE - 1);
		strncpy(c_buffer,src,length);
		c_buffer[length] = 0;
		/* convert string to unicode */
		RtlInitAnsiString(&aStr,c_buffer);
		uStr.Buffer = buffer;
		uStr.Length = 0;
		uStr.MaximumLength = size - 1;
		if(RtlAnsiStringToUnicodeString(&uStr,&aStr,FALSE) != STATUS_SUCCESS)
			wcscpy(buffer,fmt_error);
		/* append error description */
		p = strrchr(c_buffer,':');
		if(p){
			/* skip ':' and spaces */
			p ++;
			while(*p == 0x20) p++;
			sscanf(p,"%x",&iStatus);
			/* very important for x64 targets */
			Status = (NTSTATUS)(signed int)iStatus;
			wcscpy(desc,L"\r\n");
			/* try to get FormatMessage function address */
			if(FindFormatMessage()){
				/* append localized error description from kernel32.dll */
				err_code = RtlNtStatusToDosError(Status);
				if(!func_FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err_code,
					MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
					desc + 2,sizeof(desc) / sizeof(short)  - 10,NULL))
				{
					goto get_predefined_msg;
				}
			} else {
			get_predefined_msg:
				RtlInitAnsiString(&aStr,winx_get_error_description((unsigned long)Status));
				uStr.Buffer = desc + 2;
				uStr.Length = 0;
				uStr.MaximumLength = sizeof(desc) / sizeof(short) - 10;
				if(RtlAnsiStringToUnicodeString(&uStr,&aStr,FALSE) != STATUS_SUCCESS)
					wcscpy(desc + 2,fmt_error);
			}
			length = wcslen(buffer);
			length2 = min(wcslen(desc),(unsigned int)size - length - 1);
			wcsncpy(buffer + length,desc,length2);
			buffer[length + length2] = 0;
		}
	}
}

int FindFormatMessage(void)
{
	if(FormatMessageState == FormatMessageUndefined){
		if(winx_get_proc_address(L"kernel32.dll","FormatMessageA",(void *)&func_FormatMessageA) == 0)
			FormatMessageState = FormatMessageFound;
		else {
			FormatMessageState = FormatMessageNotFound; /* before pop_error !!! */
			winx_pop_error(NULL,0);
		}
		if(winx_get_proc_address(L"kernel32.dll","FormatMessageW",(void *)&func_FormatMessageW) < 0){
			FormatMessageState = FormatMessageNotFound; /* before pop_error !!! */
			winx_pop_error(NULL,0);
		}
	}
	return (FormatMessageState == FormatMessageFound) ? TRUE : FALSE;
}

/****f* zenwinx.error/winx_save_error
* NAME
*    winx_save_error
* SYNOPSIS
*    winx_save_error(buffer, size);
* FUNCTION
*    Retrieves unformatted error message
*    and removes it from stack.
* INPUTS
*    buffer - memory block to store message into
*    size   - maximum number of characters to store,
*             including terminal zero
* RESULT
*    This function does not return a value.
* EXAMPLE
*    if(winx_xxx() < 0){
*        winx_save_error(buffer,sizeof(buffer));
*        // other winx calls
*        winx_restore_error(buffer); // for winx_xxx()
*    }
* NOTES
*    See notes for winx_push_error() function.
* SEE ALSO
*    winx_push_error,winx_pop_error,winx_pop_werror,
*    winx_get_error_description,winx_restore_error
******/
void __stdcall winx_save_error(char *buffer, int size)
{
	int length;
	char *source;
	
	source = (char *)(LONG_PTR)(NtCurrentTeb()->LastErrorValue);

	if(buffer && size > 0){
		length = min(strlen(source),(unsigned int)size - 1);
		strncpy(buffer,source,length);
		buffer[length] = 0;
	}

	if(strcmp(source,no_mem) != 0){
		winx_virtual_free(source,ERR_MSG_SIZE);
		//InterlockedDecrement(&malloc_free_delta);
	}
}

/****f* zenwinx.error/winx_restore_error
* NAME
*    winx_restore_error
* SYNOPSIS
*    winx_restore_error(buffer);
* FUNCTION
*    winx_push_error() analog.
* SEE ALSO
*    winx_push_error,winx_pop_error,winx_pop_werror,
*    winx_get_error_description,winx_restore_error
******/
void __stdcall winx_restore_error(char *buffer)
{
	winx_push_error(buffer);
}
