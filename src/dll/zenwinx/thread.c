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
 * @file thread.c
 * @brief Threads code.
 * @addtogroup Threads
 * @{
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

/**
 * @brief Creates a thread and starts them.
 * @param[in] start_addr the starting address of the thread.
 * @param[out] phandle the thread handle pointer. May be NULL.
 * @return Zero for success, negative value otherwise.
 * @note Look at the following example for the 
 *       thread function prototype.
 * @par Example:
 * @code
 * DWORD WINAPI thread_proc(LPVOID parameter)
 * {
 *     // do something
 *     winx_exit_thread();
 *     return 0;
 * }
 * winx_create_thread(thread_proc,NULL);
 * @endcode
 */
int __stdcall winx_create_thread(PTHREAD_START_ROUTINE start_addr,HANDLE *phandle)
{
	NTSTATUS Status;
	HANDLE hThread;
	HANDLE *ph;
//	DWORD id;
//	DWORD ErrorCode;

	DbgCheck1(start_addr,"winx_create_thread",-1);

	/*
	* RtlCreateUserThread() call is incompatible with some 
	* antivirus or internet security stuff on x64 systems.
	* The following solution was suggested by 
	* Parvez Reza (parvez_ku_00@yahoo.com).
	*
	* 22 Dec 2009. More realistic cause - wrong prototype of
	* RtlCreateUserThread() with BOOLEAN keyword, now replaced 
	* by ULONG, therefore should work.
	*/
#if 0
	if(FindCreateThread()){
		hThread = func_CreateThread(NULL,0,start_addr,NULL,0,&id);
		if(!hThread){ /* failure */
			if(FindGetLastError()){
				ErrorCode = func_GetLastError();
				DebugPrint("CreateThread() failed, error code = 0x%x!",ErrorCode);
			} else {
				DebugPrint("CreateThread() failed!");
			}
			return (-1);
		}
		if(phandle) *phandle = hThread; /* success */
	} else
#endif
	 { /* we are in native mode or kernel32.dll is not loaded */
		/*
		* Implementation is very easy, because we have required call
		* on all of the supported versions of Windows.
		*/
		if(phandle) ph = phandle;
		else ph = &hThread;
		Status = RtlCreateUserThread(NtCurrentProcess(),NULL,
						0,0,0,0,start_addr,NULL,ph,NULL);
		if(!NT_SUCCESS(Status)){
			DebugPrintEx(Status,"Cannot create thread");
			//winx_printf("\nFUCKED 0x%x\n\n",(UINT)Status);
			return (-1);
		}
		/*NtCloseSafe(*ph);*/
	}
	return 0;
}

/**
 * @brief Terminates the current thread.
 * @details The exit code is always zero.
 * @todo Add exit code parameter.
 */
void __stdcall winx_exit_thread(void)
{
	NTSTATUS Status = ZwTerminateThread(NtCurrentThread(),STATUS_SUCCESS);
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"Cannot terminate thread");
	}
}

int FindCreateThread(void)
{
	if(CreateThreadState == CreateThreadUndefined){
		if(winx_get_proc_address(L"kernel32.dll","CreateThread",(void *)&func_CreateThread) == 0)
			CreateThreadState = CreateThreadFound;
		else
			CreateThreadState = CreateThreadNotFound;
	}
	return (CreateThreadState == CreateThreadFound) ? TRUE : FALSE;
}

int FindGetLastError(void)
{
	if(GetLastErrorState == GetLastErrorUndefined){
		if(winx_get_proc_address(L"kernel32.dll","GetLastError",(void *)&func_GetLastError) == 0)
			GetLastErrorState = GetLastErrorFound;
		else
			GetLastErrorState = GetLastErrorNotFound;
	}
	return (GetLastErrorState == GetLastErrorFound) ? TRUE : FALSE;
}

/** @} */
