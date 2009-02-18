/*
 *  ZenWINX - WIndows Native eXtended library.
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
* zenwinx.dll thread manipulation functions.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>

#include "ntndk.h"
#include "zenwinx.h"

/****f* zenwinx.thread/winx_create_thread
* NAME
*    winx_create_thread
* SYNOPSIS
*    error = winx_create_thread(start_addr,phandle);
* FUNCTION
*    Creates a thread to execute within the virtual
*    address space of the calling process.
* INPUTS
*    start_addr - starting address of the thread.
*    phandle    - address of memory to store a handle
*                 to the new thread.
* RESULT
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value.
* EXAMPLE
*    DWORD WINAPI thread_proc(LPVOID parameter)
*    {
*        // do something
*        winx_exit_thread();
*        return 0;
*    }
*
*    create_thread(thread_proc,param1);
* NOTES
*    Thread's function must have the following prototype:
*    DWORD WINAPI thread_proc(LPVOID parameter);
* SEE ALSO
*    winx_exit_thread
******/
int __stdcall winx_create_thread(PTHREAD_START_ROUTINE start_addr,HANDLE *phandle)
{
	NTSTATUS Status;
	HANDLE hThread;
	HANDLE *ph;

	if(!start_addr){
		winx_raise_error("E: winx_create_thread() invalid start_addr (NULL)!");
		return (-1);
	}

	/*
	* Implementation is very easy, because we have required call
	* on all of the supported versions of Windows.
	*/
	if(phandle) ph = phandle;
	else ph = &hThread;
	Status = RtlCreateUserThread(NtCurrentProcess(),NULL,FALSE,
					0,0,0,start_addr,NULL,ph,NULL);
	if(!NT_SUCCESS(Status)){
		winx_raise_error("E: Can't create thread: %x!",(UINT)Status);
		return (-1);
	}
	/*NtCloseSafe(*ph);*/
	return 0;
}

/****f* zenwinx.thread/winx_exit_thread
* NAME
*    winx_exit_thread
* SYNOPSIS
*    winx_exit_thread();
* FUNCTION
*    Ends the current thread with zero exit code.
* INPUTS
*    Nothing.
* RESULT
*    Nothing.
* EXAMPLE
*    See an example for the winx_create_thread() function.
* NOTES
*    If the function succeeds it will never return. (?)
* SEE ALSO
*    winx_create_thread
******/
void __stdcall winx_exit_thread(void)
{
	/* TODO: error handling and exit with specified status */
	NTSTATUS Status = ZwTerminateThread(NtCurrentThread(),STATUS_SUCCESS);
	if(!NT_SUCCESS(Status)){
		winx_raise_error("E: Can't terminate thread: %x!",(UINT)Status);
	}
}
