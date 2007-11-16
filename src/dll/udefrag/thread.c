/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
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
 *  udefrag.dll - middle layer between driver and user interfaces:
 *  functions to create and exit threads.
 */

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../include/ntndk.h"
#include "../../include/udefrag.h"
#include "../../include/ultradfg.h"

#define ExitThreadFound		0
#define ExitThreadNotFound	1
#define ExitThreadUndefined	2

/* global variables */
char th_msg[1024];
char exit_th_msg[1024];
int ExitThreadState = ExitThreadUndefined;
VOID (__stdcall *_RtlExitUserThread)(NTSTATUS Status);
NTSTATUS (__stdcall *_RtlGetVersion)(PRTL_OSVERSIONINFOW lpVersionInformation);

/* internal functions prototypes */
BOOL get_proc_address(char *name,PVOID *proc_addr);

char * __stdcall create_thread(PTHREAD_START_ROUTINE start_addr,HANDLE *phandle)
{
	NTSTATUS Status;
	HANDLE hThread;
	HANDLE *ph;

	/* implementation is very easy, because we have required call
	 * on all of the supported versions of Windows
	 */
	if(phandle) ph = phandle;
	else ph = &hThread;
	Status = RtlCreateUserThread(NtCurrentProcess(),NULL,FALSE,
					0,0,0,start_addr,NULL,ph,NULL);
	if(!NT_SUCCESS(Status))
	{
		sprintf(th_msg,"Can't create thread: %x!",Status);
		return th_msg;
	}
	return NULL;
}

void __stdcall exit_thread(void)
{
	/* On NT 4.0 and W2K we should do nothing, on XP SP1 both variants are acceptable,
	   on XP x64 and Vista RtlExitUserThread() MUST be called. */
	if(ExitThreadState == ExitThreadUndefined)
	{ /* try to find it */
		if(get_proc_address("RtlExitUserThread",(void *)&_RtlExitUserThread))
			ExitThreadState = ExitThreadFound;
		else
			ExitThreadState = ExitThreadNotFound;
	}
	if(ExitThreadState == ExitThreadFound)
		_RtlExitUserThread(STATUS_SUCCESS);
}

BOOL get_proc_address(char *name,PVOID *proc_addr)
{
	UNICODE_STRING uStr;
	ANSI_STRING aStr;
	NTSTATUS Status;
	PVOID base_addr;

	RtlInitUnicodeString(&uStr,L"ntdll.dll");
	Status = LdrGetDllHandle(0,0,&uStr,(HMODULE *)&base_addr);
	if(!NT_SUCCESS(Status))
	{
		sprintf(exit_th_msg,"Can't get ntdll handle: %x!",Status);
		return FALSE;
	}
	RtlInitAnsiString(&aStr,name);
	Status = LdrGetProcedureAddress(base_addr,&aStr,0,proc_addr);
	if(!NT_SUCCESS(Status))
	{
		sprintf(exit_th_msg,"Can't get address for %s: %x!",name,Status);
		return FALSE;
	}
	return TRUE;
}

/* return values: 40 for nt 4.0; 50 for w2k etc. */
int __stdcall get_os_version(void)
{
	OSVERSIONINFOW ver;

	if(!get_proc_address("RtlGetVersion",(void *)&_RtlGetVersion))
		return 40;
	_RtlGetVersion(&ver);
	return (ver.dwMajorVersion * 10 + ver.dwMinorVersion);
}
