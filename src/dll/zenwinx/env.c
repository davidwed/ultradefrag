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
* zenwinx.dll functions for Environment manipulations.
*/

#include "ntndk.h"
#include "zenwinx.h"

/****f* zenwinx.env/winx_query_env_variable
* NAME
*    winx_query_env_variable
* SYNOPSIS
*    error = winx_query_env_variable(name, buffer, length);
* FUNCTION
*    Returns the value of specified environment variable.
* INPUTS
*    name   - variable name
*    buffer - pointer to the buffer to receive
*             the null-terminated value.
*    length - length of the buffer (in characters)
* RESULT
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value.
* EXAMPLE
*    winx_query_env_variable(L"SystemRoot",buffer,sizeof(buffer));
* SEE ALSO
*    winx_set_env_variable
******/
int __stdcall winx_query_env_variable(short *name, short *buffer, int length)
{
	UNICODE_STRING n, v;
	NTSTATUS Status;
	
	if(!name){
		winx_debug_print("winx_query_env_variable() invalid name!");
		return (-1);
	}
	if(!buffer){
		winx_debug_print("winx_query_env_variable() invalid buffer!");
		return (-1);
	}

	RtlInitUnicodeString(&n,name);
	v.Buffer = buffer;
	v.Length = 0;
	v.MaximumLength = length * sizeof(short);
	Status = RtlQueryEnvironmentVariable_U(NULL,&n,&v);
	if(!NT_SUCCESS(Status)){
		winx_dbg_print_ex("Can't query %ws environment variable: %x!",
				name,(UINT)Status);
		return (-1);
	}
	return 0;
}

/****f* zenwinx.env/winx_set_env_variable
* NAME
*    winx_set_env_variable
* SYNOPSIS
*    error = winx_set_env_variable(name, value);
* FUNCTION
*    Sets the specified environment variable 
*    for the current process.
* INPUTS
*    name  - variable name
*    value - value string (NULL if you wish to delete 
*            the variable)
* RESULT
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value.
* EXAMPLE
*    winx_set_env_variable(L"XYZ",L"abc");
* SEE ALSO
*    winx_query_env_variable
******/
int __stdcall winx_set_env_variable(short *name, short *value)
{
	UNICODE_STRING n, v;
	NTSTATUS status;

	if(!name){
		winx_debug_print("winx_set_env_variable() invalid name!");
		return (-1);
	}

	RtlInitUnicodeString(&n,name);
	if(value){
		RtlInitUnicodeString(&v,value);
		status = RtlSetEnvironmentVariable(NULL,&n,&v);
	} else {
		status = RtlSetEnvironmentVariable(NULL,&n,NULL);
	}
	if(!NT_SUCCESS(status)){
		winx_dbg_print_ex("Can't set %ws environment variable: %x!",
				name,(UINT)status);
		return (-1);
	}
	return 0;
}
