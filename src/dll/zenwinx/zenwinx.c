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
 * @file zenwinx.c
 * @brief Startup and shutdown code.
 * @addtogroup StartupAndShutdown
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

int  __stdcall kb_open(void);
void __stdcall kb_close(void);

void winx_create_global_heap(void);
void winx_destroy_global_heap(void);
void winx_init_synch_objects(void);
void winx_destroy_synch_objects(void);

#ifndef STATIC_LIB
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH){
		winx_create_global_heap();
		winx_init_synch_objects();
	} else if(dwReason == DLL_PROCESS_DETACH){
		winx_destroy_global_heap();
		winx_destroy_synch_objects();
	}
	return 1;
}
#endif

/**
 * @brief Initializes the library.
 * @note Designed especially to replace DllMain
 * functionality in case of monolithic native application.
 */
void __stdcall zenwinx_native_init(void)
{
	winx_create_global_heap();
	winx_init_synch_objects();
}

/**
 * @brief Frees library resources.
 * @note Designed especially to replace DllMain
 * functionality in case of monolithic native application.
 */
void __stdcall zenwinx_native_unload(void)
{
	winx_destroy_global_heap();
	winx_destroy_synch_objects();
}

/**
 * @brief Initializes the native application.
 * @details This routine prepares keyboards for working 
 *          with user input related procedures.
 * @param[in] peb the Process Environment Block pointer.
 * @result Zero for success, negative value otherwise.
 * @par Example:
 * @code
 * // native entry point
 * void __stdcall NtProcessStartup(PPEB Peb)
 * {
 *     winx_init(Peb);
 *     // your program code here
 *     // ...
 *     winx_exit(0); // successful exit
 * }
 * @endcode
 */
int __stdcall winx_init(void *peb)
{
	PRTL_USER_PROCESS_PARAMETERS pp;
	int status;

	/* 1. Normalize and get the Process Parameters */
	pp = RtlNormalizeProcessParams(((PPEB)peb)->ProcessParameters);
	/* 2. Breakpoint if we were requested to do so */
	if(pp->DebugFlags) DbgBreakPoint();
	/* 3. Open keyboard */
	status = kb_open();
	return status;
}

/**
 * @brief Terminates the calling native process.
 * @details This routine releases all resources used
 *          by zenwinx library before the process termination.
 * @param[in] exit_code the exit status.
 */
void __stdcall winx_exit(int exit_code)
{
	kb_close();
	/*
	* The next call is undocumented, therefore
	* we are not checking its result.
	*/
	(void)NtTerminateProcess(NtCurrentProcess(),exit_code);
}

/**
 * @brief Reboots the computer.
 * @note If SE_SHUTDOWN privilege adjusting fails
 * then the computer will not be rebooted and the program 
 * will continue the execution after this call.
 */
void __stdcall winx_reboot(void)
{
	kb_close();
	(void)winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE);
	/*
	* The next call is undocumented, therefore
	* we are not checking its result.
	*/
	(void)NtShutdownSystem(ShutdownReboot);
}

/**
 * @brief Shuts down the computer.
 * @note If SE_SHUTDOWN privilege adjusting fails
 * then the computer will not be shut down and the program 
 * will continue the execution after this call.
 */
void __stdcall winx_shutdown(void)
{
	kb_close();
	(void)winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE);
	/*
	* The next call is undocumented, therefore
	* we are not checking its result.
	*/
	(void)NtShutdownSystem(ShutdownNoReboot);
}

/** @} */
