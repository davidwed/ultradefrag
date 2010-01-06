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
 * @file env.c
 * @brief Process environment managing code.
 * @addtogroup Environment
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

/**
 * @brief Queries an environment variable.
 * @param[in]  name   the name of environment variable.
 * @param[out] buffer pointer to the buffer to receive
 *                    the null-terminated value string.
 * @param[in]  length the length of the buffer, in characters.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall winx_query_env_variable(short *name, short *buffer, int length)
{
	UNICODE_STRING n, v;
	NTSTATUS Status;
	
	DbgCheck3(name,buffer,(length > 0),"winx_query_env_variable",-1);

	RtlInitUnicodeString(&n,name);
	v.Buffer = buffer;
	v.Length = 0;
	v.MaximumLength = length * sizeof(short);
	Status = RtlQueryEnvironmentVariable_U(NULL,&n,&v);
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"Cannot query %ws environment variable",name);
		return (-1);
	}
	return 0;
}

/**
 * @brief Sets an environment variable.
 * @param[in] name  the name of the environment variable.
 * @param[in] value the null-terminated value string. Note that
 *                  passing NULL here causes a variable deletion.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall winx_set_env_variable(short *name, short *value)
{
	UNICODE_STRING n, v;
	NTSTATUS status;

	DbgCheck1(name,"winx_set_env_variable",-1);

	RtlInitUnicodeString(&n,name);
	if(value){
		RtlInitUnicodeString(&v,value);
		status = RtlSetEnvironmentVariable(NULL,&n,&v);
	} else {
		status = RtlSetEnvironmentVariable(NULL,&n,NULL);
	}
	if(!NT_SUCCESS(status)){
		DebugPrintEx(status,"Cannot set %ws environment variable",name);
		return (-1);
	}
	return 0;
}

/** @} */
