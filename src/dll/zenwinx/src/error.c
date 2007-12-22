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

/****f* zenwinx.error/winx_get_error_message
 *  NAME
 *     winx_get_error_message
 *  SYNOPSIS
 *     winx_get_error_message(code);
 *  FUNCTION
 *     Returns a text description for specified
 *     zenwinx error code.
 *  INPUTS
 *     code - one of the zenwinx error codes,
 *            returned by one of the zenwinx functions.
 *  RESULT
 *     Pointer to string with error description;
 *     NULL for invalid code.
 *  EXAMPLE
 *     signed int code;
 *     unsigned long last_error;
 *
 *     code = winx_xxx();
 *     if(code < 0)
 *     {
 *         winx_printf("%s\n",winx_get_error_message(code));
 *         last_error = winx_get_last_error();
 *         winx_printf(winx_get_error_description(last_error);
 *         // error handling ...
 *     }
 *  NOTES
 *     ZenWINX functions usually returns negative value 
 *     as error code: return -ZENWINX_ERROR_CODE;
 *     Therefore use returned value,
 *     not the code from zenwinx.h.
 *  SEE ALSO
 *     winx_get_error_message_ex
 ******/
char * __stdcall winx_get_error_message(int code)
{
	unsigned int index;
	
	index = (unsigned int)(-code);
	if(index >= sizeof(emsg) / sizeof(short *))
		return "";
	return emsg[index];
}

/****f* zenwinx.error/winx_get_error_description
 *  NAME
 *     winx_get_error_description
 *  SYNOPSIS
 *     winx_get_error_description(code);
 *  FUNCTION
 *     Returns a text description for 
 *     specified extended error code.
 *  INPUTS
 *     code - extended error code, 
 *            returned by winx_get_last_error.
 *  RESULT
 *     Pointer to string with extended error description.
 *  EXAMPLE
 *     See an example for the winx_get_error_message function.
 *  NOTES
 *     If you use winx_set_last_error for purposes 
 *     other than NTSTATUS code saving,
 *     don't use this function to decode error.
 *  SEE ALSO
 *     winx_get_last_error,winx_set_last_error
 ******/
char * __stdcall winx_get_error_description(unsigned long code)
{
	NTSTATUS Status;
	char *msg = "Unknown error; send report to the authors";
	
	Status = (NTSTATUS)code;
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

/****f* zenwinx.error/winx_get_error_message_ex
 *  NAME
 *     winx_get_error_message_ex
 *  SYNOPSIS
 *     winx_get_error_message_ex(s,n,code);
 *  FUNCTION
 *     Returns a full text description for specified
 *     zenwinx error code, including information about
 *     the last-error extended code.
 *  INPUTS
 *     s    - buffer to store description.
 *     n    - maximum number of characters to store.
 *     code - one of the zenwinx error codes,
 *            specified in zenwinx.h.
 *  RESULT
 *     Zero for success;
 *     -1 for error (buffer is too small).
 *  EXAMPLE
 *     NTSTATUS Status = STATUS_SUCCESS;
 *     Status = NtReadFile(...);
 *     if(!NT_SUCCESS(Status))
 *     {
 *         winx_set_last_error((ULONG)Status);
 *         // error handling ...
 *     }
 *  NOTES
 *     1. Use this function to retrieve detailed 
 *        information about the last error.
 *     2. ZenWINX functions usually returns 
 *        negative value as error code:
 *        return -ZENWINX_ERROR_CODE;
 *        Therefore use returned value, 
 *        not the code from zenwinx.h.
 *  SEE ALSO
 *     winx_get_error_message
 ******/
int __stdcall winx_get_error_message_ex(char *s, int n, signed int code)
{
	char msg[256];
	int len;
	unsigned long last_error;
	
	if(!s) return -1;
	memset(msg,0,sizeof(msg));
	s[0] = 0;
	last_error = winx_get_last_error();
	sprintf(msg,"%s: %x!\n%s.",winx_get_error_message(code),
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
 *     winx_printf("Last extended error code = %u",
 *                 (UINT)winx_get_last_error());
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
