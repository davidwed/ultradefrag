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
* zenwinx.dll GetProcAddress() and other related stuff.
*/

#include "ntndk.h"
#include "zenwinx.h"

/****f* zenwinx.loader/winx_get_proc_address
* NAME
*    winx_get_proc_address
* SYNOPSIS
*    error = winx_get_proc_address(libname,funcname,paddr);
* FUNCTION
*    Retrieves the address of an exported function or variable
*    from the specified dynamic-link library.
* INPUTS
*    libname  - name of the library.
*    funcname - name of the function or variable.
*    paddr    - address of memory to store retrieved address into.
* RESULT
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value.
* EXAMPLE
*    if(winx_get_proc_address(L"ntdll.dll","RtlGetVersion",
*            (void *)&func_RtlGetVersion) < 0){
*        // it seems that we have nt 4.0 system without RtlGetVersion()
*    }
* NOTES
*    Specified dynamic-link library must be loaded before this call.
******/
int __stdcall winx_get_proc_address(short *libname,char *funcname,PVOID *proc_addr)
{
	UNICODE_STRING uStr;
	ANSI_STRING aStr;
	NTSTATUS Status;
	HMODULE base_addr;

	if(!libname){
		winx_raise_error("E: winx_get_proc_address() invalid libname!");
		return (-1);
	}
	if(!funcname){
		winx_raise_error("E: winx_get_proc_address() invalid funcname!");
		return (-1);
	}
	if(!proc_addr){
		winx_raise_error("E: winx_get_proc_address() invalid proc_addr!");
		return (-1);
	}

	RtlInitUnicodeString(&uStr,libname);
	Status = LdrGetDllHandle(0,0,&uStr,(HMODULE *)&base_addr);
	if(!NT_SUCCESS(Status)){
		winx_raise_error("N: Can't get %ls handle: %x!",libname,(UINT)Status);
		return (-1);
	}
	RtlInitAnsiString(&aStr,funcname);
	Status = LdrGetProcedureAddress(base_addr,&aStr,0,proc_addr);
	if(!NT_SUCCESS(Status)){
		winx_raise_error("N: Can't get address for %s: %x!",funcname,(UINT)Status);
		*proc_addr = NULL;
		return (-1);
	}
	return 0;
}

