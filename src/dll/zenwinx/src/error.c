/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 *  zenwinx.dll error handling routines.
 */

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <stdio.h>

#include "ntndk.h"
#include "zenwinx.h"

char *emsg[] = \
{
	"",
	"Can't open the keyboard",
	"Can't create event"
};

char * __stdcall winx_get_error_message(int code)
{
	if(code < 0 || code >= sizeof(emsg) / sizeof(short *))
		return "";
	return emsg[code];
}

char * __stdcall winx_get_error_description(unsigned long code)
{
	NTSTATUS Status;
	char *msg = "Unknown error; send report to the authors";
	
	Status = -code;
	switch(Status)
	{
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

int __stdcall winx_get_error_message_ex(char *s, int n, signed int code)
{
	char msg[256];
	int len;
	unsigned long last_error;
	
	if(!s) return -1;
	memset(msg,0,sizeof(msg));
	s[0] = 0;
	last_error = winx_get_last_error();
	sprintf(msg,"%s: %x!\n%s.",winx_get_error_message(-code),
		(UINT)last_error,winx_get_error_description(last_error));
	len = strlen(msg);
	if(len >= n) return -1;
	strcpy(s,msg);
	return 0;
}

/****f* zenwinx.error/winx_set_last_error
 *  NAME
 *     winx_set_last_error
 *  SYNOPSIS
 *     winx_set_last_error(code);
 *  FUNCTION
 *     Sets the last-error code for the calling thread.
 *  INPUTS
 *     code - unsigned long value to be used as last-error code.
 *  RESULT
 *     This function does not return a value.
 *  EXAMPLE
 *     NTSTATUS Status = STATUS_SUCCESS;
 *     Status = NtReadFile(...);
 *     if(!NT_SUCCESS(Status))
 *     {
 *         winx_set_last_error((ULONG)Status);
 *         // error handling ...
 *     }
 *  NOTES
 *     ZenWINX library utilize this function to save NTSTATUS
 *     values threated as extented error codes only.
 *  SEE ALSO
 *     winx_get_last_error
 ******/
void __stdcall winx_set_last_error(unsigned long code)
{
	NtCurrentTeb()->LastErrorValue = code;
}

/****f* zenwinx.error/winx_get_last_error
 *  NAME
 *     winx_get_last_error
 *  SYNOPSIS
 *     code = winx_get_last_error();
 *  FUNCTION
 *     Retrieves the calling thread's last-error 
 *     extended code value.
 *  INPUTS
 *     Nothing.
 *  RESULT
 *     code - calling thread's last-error
 *            extended code.
 *  EXAMPLE
 *     See an example for the winx_init() function.
 *  NOTES
 *     Many of ZenWINX functions (as described in this manual)
 *     returns negative error codes defined in zenwinx.h.
 *     In addition, they stores extended error code using
 *     winx_set_last_error function. To get this stored value
 *     use this function.
 *  SEE ALSO
 *     winx_set_last_error
 ******/
unsigned long __stdcall winx_get_last_error(void)
{
	return NtCurrentTeb()->LastErrorValue;
}
