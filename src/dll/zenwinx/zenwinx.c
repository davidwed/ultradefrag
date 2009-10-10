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
* zenwinx.dll startup code and some basic procedures.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <stdio.h>

#include "ntndk.h"
#include "zenwinx.h"

//extern long malloc_free_delta;

//unsigned long __cdecl DbgPrint(char *format, ...);

int  __stdcall kb_open(short *kb_device_name);
void __stdcall kb_close(void);

BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	return 1;
}

/****f* zenwinx.startup/winx_init
* NAME
*    winx_init
* SYNOPSIS
*    error = winx_init(peb);
* FUNCTION
*    Initialize a native application and ZenWINX library.
* INPUTS
*    peb   - pointer for the Process Environment Block, that is 
*            the first parameter of NtProcessStartup() function.
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    // native entry point
*    void __stdcall NtProcessStartup(PPEB Peb)
*    {
*        winx_init(Peb);
*        // your program code here
*        // ...
*        winx_exit(0); // successful exit
*    }
* NOTES
*    You should call this function before any attempt
*    to use keyboard related functions.
* BUGS
*    It seems that USB keyboards are unsupported.
* SEE ALSO
*    winx_exit,winx_reboot,winx_shutdown,winx_pop_error
******/
int __stdcall winx_init(void *peb)
{
	PRTL_USER_PROCESS_PARAMETERS pp;
	int status;
	int i, j, k;
	short kb_device_name[32];

	/* 1. Normalize and get the Process Parameters */
	pp = RtlNormalizeProcessParams(((PPEB)peb)->ProcessParameters);
	/* 2. Breakpoint if we were requested to do so */
	if(pp->DebugFlags) DbgBreakPoint();
	/* 3. Open the PS/2 or USB keyboard */
	for(i = 0; i < 20; i++){
		_snwprintf(kb_device_name,32,L"\\Device\\KeyboardClass%u",i);
		status = kb_open(kb_device_name);
		if(status >= 0){
			winx_printf("Keyboard device found: %ws.\n",kb_device_name);
			winx_printf("Hit any key to ensure that it is physically connected...  ");
			for(j = 0; j < 10; j++){
				if(winx_kbhit(1000) >= 0) {
					winx_printf("\n\n");
					winx_printf("Keyboard initialized, wait 3 seconds...\n\n");
					for(k = 0; k < 60; k++)
						winx_kbhit(50); /* cleanup keyboard buffer */
					return status;
				}
				winx_printf("%u ",10 - j);
			}
			winx_printf("\n\n");
			/* no key has been pressed, try another keyboard */
			kb_close();
			continue;
		}
	}
	
	winx_printf("\nWait 15 seconds for USB keyboard initialization...\n\n");
	winx_sleep(15000);
	
	for(i = 0; i < 100; i++){
		_snwprintf(kb_device_name,32,L"\\Device\\KeyboardClass%u",i);
		status = kb_open(kb_device_name);
		if(status >= 0){
			winx_printf("Keyboard device found: %ws.\n",kb_device_name);
			winx_printf("Hit any key to ensure that it is physically connected...  ");
			for(j = 0; j < 10; j++){
				if(winx_kbhit(1000) >= 0){
					winx_printf("\n\n");
					winx_printf("Keyboard initialized, wait 3 seconds...\n\n");
					for(k = 0; k < 60; k++)
						winx_kbhit(50); /* cleanup keyboard buffer */
					return status;
				}
				winx_printf("%u ",10 - j);
			}
			winx_printf("\n\n");
			/* no key has been pressed, try another keyboard */
			kb_close();
			continue;
		}
	}
	
	winx_printf("Preparing for automatic keyboard detection...\n\n");

	for(i = 0; i < 100; i++){
		_snwprintf(kb_device_name,32,L"\\Device\\KeyboardClass%u",i);
		status = kb_open(kb_device_name);
		if(status >= 0){
			winx_printf("Keyboard device found: %ws.\n\n",kb_device_name);
			winx_printf("Keyboard initialized, wait 3 seconds...\n\n");
			for(k = 0; k < 60; k++)
				winx_kbhit(50); /* cleanup keyboard buffer */
			return status;
		}
	}
	return status;
}

/****f* zenwinx.startup/winx_exit
* NAME
*    winx_exit
* SYNOPSIS
*    winx_exit(status);
* FUNCTION
*    Terminate the calling native process after ZenWINX cleanup.
* INPUTS
*    status - exit status.
* RESULT
*    This function does not return a value.
* EXAMPLE
*    See an example for the winx_init() function.
* NOTES
*    This function should be used instead of 
*    NtTerminateProcess() to free resources
*    allocated by winx_init() function.
* SEE ALSO
*    winx_init,winx_reboot,winx_shutdown
******/
void __stdcall winx_exit(int exit_code)
{
	kb_close();
	//DbgPrint("-ZenWINX- malloc_free_delta = %li\n",malloc_free_delta);
	NtTerminateProcess(NtCurrentProcess(),exit_code);
}

/****f* zenwinx.startup/winx_reboot
* NAME
*    winx_reboot
* SYNOPSIS
*    winx_reboot();
* FUNCTION
*     Reboot the computer after ZenWINX cleanup.
* INPUTS
*    Nothing.
* RESULT
*    This function does not return a value.
* EXAMPLE
*    if(command == RebootCommand){
*        winx_printf("Reboot ...");
*        winx_reboot();
*        winx_printf("You don't have a special privilege\n"
*                    "to reboot the computer!\n");
*    }
* NOTES
*    If a SE_SHUTDOWN privilege can not be enabled,
*    then the computer will not be rebooted
*    and the program execution will continue with
*    the first instruction after this call.
* SEE ALSO
*    winx_init,winx_exit,winx_shutdown
******/
void __stdcall winx_reboot(void)
{
	kb_close();
	winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE);
	NtShutdownSystem(ShutdownReboot);
}

/****f* zenwinx.startup/winx_shutdown
* NAME
*    winx_shutdown
* SYNOPSIS
*    winx_shutdown();
* FUNCTION
*     Shutdown the computer after ZenWINX cleanup.
* INPUTS
*    Nothing.
* RESULT
*    This function does not return a value.
* EXAMPLE
*    if(command == ShutdownCommand){
*        winx_printf("Shutdown ...");
*        winx_shutdown();
*        winx_printf("You don't have a special privilege\n"
*                    "to shutdown the computer!\n");
*    }
* NOTES
*    If a SE_SHUTDOWN privilege can not be enabled,
*    then the computer will not be shutdown
*    and the program execution will continue with
*    the first instruction after this call.
* SEE ALSO
*    winx_init,winx_exit,winx_reboot
******/
void __stdcall winx_shutdown(void)
{
	kb_close();
	winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE);
	NtShutdownSystem(ShutdownNoReboot);
}
