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

#include "ntndk.h"
#include "zenwinx.h"

#define FormatMessageFound		0
#define FormatMessageNotFound	1
#define FormatMessageUndefined	2

int FormatMessageState = FormatMessageUndefined;
DWORD (WINAPI *func_FormatMessageW)(DWORD,PVOID,DWORD,DWORD,LPWSTR,DWORD,va_list*);
int FindFormatMessage(void);

void (__stdcall *ErrorHandler)(short *msg) = NULL;

/****f* zenwinx.error/winx_set_error_handler
* NAME
*    winx_set_error_handler
* SYNOPSIS
*    winx_set_error_handler(handler);
* FUNCTION
*    Delivers error message to the ErrorHandler.
* INPUTS
*    handler - address of ErrorHandler callback,
*              if NULL then process will work
*              without any handler
* RESULT
*    Returns previously set handler.
* EXAMPLE
*    winx_set_error_handler(ErrorHandlerFunction);
* SEE ALSO
*    winx_raise_error, winx_get_error_description
******/
ERRORHANDLERPROC __stdcall winx_set_error_handler(ERRORHANDLERPROC handler)
{
	ERRORHANDLERPROC h = ErrorHandler;
	ErrorHandler = handler;
	return h;
}

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
*    winx_raise_error, winx_set_error_handler
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

/****f* zenwinx.error/winx_raise_error
* NAME
*    winx_raise_error
* SYNOPSIS
*    winx_raise_error(format, ...);
* FUNCTION
*    Delivers error message to the ErrorHandler.
* INPUTS
*    format - format string
*    ...    - parameters
* RESULT
*    This function does not return a value.
* EXAMPLE
*    winx_raise_error("W: Can't open %s file: %x!",
*            filename, (UINT)Status);
* SEE ALSO
*    winx_get_error_description, winx_set_error_handler
******/
void __cdecl winx_raise_error(char *format, ...)
{
	char buffer[ERR_MSG_SIZE];
	short desc[256];
	short resulting_message[ERR_MSG_SIZE];
	va_list arg;
	int length;
	char *p;
	UINT iStatus = 0;
	NTSTATUS Status;
	ULONG err_code;

	/* 0. if ErrorHandler is not set return */
	if(!ErrorHandler) return;

	/* 1. store resulting ANSI string into buffer */
	va_start(arg,format);
	memset(buffer,0,ERR_MSG_SIZE);
	length = _vsnprintf(buffer,ERR_MSG_SIZE - 1,format,arg);
	buffer[ERR_MSG_SIZE - 1] = 0;
	va_end(arg);

	/* 2. get error description */
	/* a). try to find status code */
	p = strrchr(buffer,':');
	if(!p || !strstr(format,": %x!")){
status_code_missing:
		_snwprintf(resulting_message,ERR_MSG_SIZE - 1,L"%hs",buffer);
		resulting_message[ERR_MSG_SIZE - 1] = 0;
		/* call ErrorHandler and return */
		ErrorHandler(resulting_message);
		return;
	}
	/* b). skip ':' and spaces */
	p ++;
	while(*p == 0x20) p++;
	/* c). get status code */
	sscanf(p,"%x",&iStatus);
	if(!iStatus) goto status_code_missing;
	Status = (NTSTATUS)(signed int)iStatus; /* very important for x64 targets */
	/* d). try to get FormatMessage function address */
	if(FindFormatMessage()){
		/* get localized error description from kernel32.dll */
		err_code = RtlNtStatusToDosError(Status);
		if(!func_FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,NULL,err_code,
			MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
			desc,sizeof(desc) / sizeof(short)  - 1,NULL)) goto get_predefined_msg;
	} else {
	get_predefined_msg:
		_snwprintf(desc,sizeof(desc) / sizeof(short) - 1,L"%hs",
			winx_get_error_description((unsigned long)Status));
	}
	desc[sizeof(desc) / sizeof(short) - 1] = 0;
	/* 3. append the description to resulting string */
	_snwprintf(resulting_message,ERR_MSG_SIZE - 1,L"%hs\r\n%ws",buffer,desc);
	resulting_message[ERR_MSG_SIZE - 1] = 0;

	/* 4. call ErrorHandler and return */
	ErrorHandler(resulting_message);
	return;
}

int FindFormatMessage(void)
{
	ERRORHANDLERPROC eh;
	
	if(FormatMessageState == FormatMessageUndefined){
		eh = winx_set_error_handler(NULL);
		if(winx_get_proc_address(L"kernel32.dll","FormatMessageW",(void *)&func_FormatMessageW) == 0)
			FormatMessageState = FormatMessageFound;
		else
			FormatMessageState = FormatMessageNotFound;
		winx_set_error_handler(eh);
	}
	return (FormatMessageState == FormatMessageFound) ? TRUE : FALSE;
}
