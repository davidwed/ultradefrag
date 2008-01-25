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
* zenwinx.dll thread manipulation functions.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>

#include "ntndk.h"
#include "zenwinx.h"

#define ExitThreadFound		0
#define ExitThreadNotFound	1
#define ExitThreadUndefined	2

int ExitThreadState = ExitThreadUndefined;
VOID (__stdcall *func_RtlExitUserThread)(NTSTATUS Status);

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
*        if(winx_exit_thread() < 0))
*            winx_pop_error(NULL,0);
*        return 0;
*    }
*
*    if(create_thread(thread_proc,param1) < 0){
*        winx_pop_error(buffer,sizeof(buffer));
*        // handle error
*    }
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

	/*
	* Implementation is very easy, because we have required call
	* on all of the supported versions of Windows.
	*/
	if(phandle) ph = phandle;
	else ph = &hThread;
	Status = RtlCreateUserThread(NtCurrentProcess(),NULL,FALSE,
					0,0,0,start_addr,NULL,ph,NULL);
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't create thread: %x!",(UINT)Status);
		return (-1);
	}
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
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value.
* EXAMPLE
*    See an example for the winx_create_thread() function.
* NOTES
*    If the function succeeds on xp and later systems,
*    it will never return.
* SEE ALSO
*    winx_create_thread
******/
int __stdcall winx_exit_thread(void)
{
	/*
	* On NT 4.0 and W2K we should do nothing, on XP SP1 both variants are acceptable,
	* on XP x64 and Vista RtlExitUserThread() MUST be called.
	*/
	if(ExitThreadState == ExitThreadUndefined){ /* try to find it */
		if(winx_get_proc_address(L"ntdll.dll","RtlExitUserThread",
				(void *)&func_RtlExitUserThread) == 0)
			ExitThreadState = ExitThreadFound;
		else {
			winx_pop_error(NULL,0);
			ExitThreadState = ExitThreadNotFound;
		}
	}
	if(ExitThreadState == ExitThreadFound){
		func_RtlExitUserThread(STATUS_SUCCESS);
		return 0;
	}
	if(winx_get_os_version() >= 51){
		winx_push_error("RtlExitThread function not found!");
		return (-1);
	}
	return 0;
}
