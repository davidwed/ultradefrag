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
 *  zenwinx.dll startup code and some basic procedures.
 */

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>

#include "ntndk.h"
#include "zenwinx.h"

int  __stdcall kb_open(short *kb_device_name);
void __stdcall kb_close(void);

BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	return 1;
}

int __stdcall winx_init(void *peb)
{
	PRTL_USER_PROCESS_PARAMETERS pp;
	int status;

	/* 1. Normalize and get the Process Parameters */
	pp = RtlNormalizeProcessParams(((PPEB)peb)->ProcessParameters);
	/* 2. Breakpoint if we were requested to do so */
	if(pp->DebugFlags) DbgBreakPoint();
	/* 3. Open the keyboard */
	status = kb_open(L"\\Device\\KeyboardClass0");
	return status;
}

void __stdcall winx_exit(int exit_code)
{
	kb_close();
	NtTerminateProcess(NtCurrentProcess(),exit_code);
}

void __stdcall winx_reboot(void)
{
	kb_close();
	NtShutdownSystem(ShutdownReboot);
}

void __stdcall winx_shutdown(void)
{
	kb_close();
	NtShutdownSystem(ShutdownNoReboot);
}
