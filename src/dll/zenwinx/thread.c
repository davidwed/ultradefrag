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

#include "ntndk.h"
#include "zenwinx.h"

#define CreateThreadFound		0
#define CreateThreadNotFound	1
#define CreateThreadUndefined	2

int CreateThreadState = CreateThreadUndefined;
HANDLE (WINAPI *func_CreateThread)(LPSECURITY_ATTRIBUTES,SIZE_T,LPTHREAD_START_ROUTINE,
	LPVOID,DWORD,LPDWORD);
int FindCreateThread(void);

#define GetLastErrorFound		0
#define GetLastErrorNotFound	1
#define GetLastErrorUndefined	2

int GetLastErrorState = GetLastErrorUndefined;
DWORD (WINAPI *func_GetLastError)(void);
int FindGetLastError(void);

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
	DWORD id;
	DWORD ErrorCode;

	if(!start_addr){
		winx_raise_error("E: winx_create_thread() invalid start_addr (NULL)!");
		return (-1);
	}

	/*
	* RtlCreateUserThread() call is incompatible with some 
	* antivirus or internet security stuff on x64 systems.
	* The following solution was suggested by 
	* Parvez Reza (parvez_ku_00@yahoo.com).
	*/
	if(FindCreateThread()){
		hThread = func_CreateThread(NULL,0,start_addr,NULL,0,&id);
		if(!hThread){ /* failure */
			if(FindGetLastError()){
				ErrorCode = func_GetLastError();
				winx_raise_error("E: CreateThread() failed, error code = 0x%x!",ErrorCode);
			} else {
				winx_raise_error("E: CreateThread() failed!");
			}
			return (-1);
		}
		if(phandle) *phandle = hThread; /* success */
	} else { /* we are in native mode or kernel32.dll is not loaded */
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

/* internal functions */
int FindCreateThread(void)
{
	ERRORHANDLERPROC eh;
	
	if(CreateThreadState == CreateThreadUndefined){
		eh = winx_set_error_handler(NULL);
		if(winx_get_proc_address(L"kernel32.dll","CreateThread",(void *)&func_CreateThread) == 0)
			CreateThreadState = CreateThreadFound;
		else
			CreateThreadState = CreateThreadNotFound;
		winx_set_error_handler(eh);
	}
	return (CreateThreadState == CreateThreadFound) ? TRUE : FALSE;
}

int FindGetLastError(void)
{
	ERRORHANDLERPROC eh;
	
	if(GetLastErrorState == GetLastErrorUndefined){
		eh = winx_set_error_handler(NULL);
		if(winx_get_proc_address(L"kernel32.dll","GetLastError",(void *)&func_GetLastError) == 0)
			GetLastErrorState = GetLastErrorFound;
		else
			GetLastErrorState = GetLastErrorNotFound;
		winx_set_error_handler(eh);
	}
	return (GetLastErrorState == GetLastErrorFound) ? TRUE : FALSE;
}
