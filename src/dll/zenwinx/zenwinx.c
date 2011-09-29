/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @brief Startup and shutdown.
 * @addtogroup StartupAndShutdown
 * @{
 */

#include "zenwinx.h"

int kb_open(void);
void kb_close(void);
int  winx_create_global_heap(void);
void winx_destroy_global_heap(void);
int  winx_init_synch_objects(void);
void winx_destroy_synch_objects(void);
void MarkWindowsBootAsSuccessful(void);
char *winx_get_error_description(unsigned long status);
void flush_dbg_log(int already_synchronized);

/**
 * @brief Internal variable indicating
 * whether library initialization failed
 * or not. Intended to be checked by
 * zenwinx_init_failed routine.
 */
int initialization_failed = 1;

#ifndef STATIC_LIB
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH){
		zenwinx_native_init();
	} else if(dwReason == DLL_PROCESS_DETACH){
		zenwinx_native_unload();
	}
	return 1;
}
#endif

/**
 * @brief Initializes the library.
 * @details Designed especially to replace DllMain
 * functionality in case of monolithic native application.
 * Call this routine in the beginning of the NtProcessStartup.
 * @return Zero for success, negative value otherwise.
 */
int zenwinx_native_init(void)
{
	if(winx_create_global_heap() < 0)
		return (-1);
	if(winx_init_synch_objects() < 0)
		return (-1);
	initialization_failed = 0;
	return 0;
}

/**
 * @brief Defines whether the library has 
 * been initialized successfully or not.
 * @return Boolean value.
 */
int zenwinx_init_failed(void)
{
	return initialization_failed;
}

/**
 * @brief Frees library resources.
 * @note Designed especially to replace DllMain
 * functionality in case of monolithic native application.
 * Don't call it before winx_shutdown() and winx_reboot(),
 * but call always before winx_exit().
 */
void zenwinx_native_unload(void)
{
	winx_destroy_synch_objects();
	winx_destroy_global_heap();
}

/**
 * @brief Initializes the native application.
 * @details This routine prepares keyboards
 * for work with user input related procedures.
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
int winx_init(void *peb)
{
	PRTL_USER_PROCESS_PARAMETERS pp;

	/*  normalize and get the process parameters */
	if(peb){
		pp = RtlNormalizeProcessParams(((PPEB)peb)->ProcessParameters);
		/* breakpoint if we were requested to do so */
		if(pp){
			if(pp->DebugFlags)
				DbgBreakPoint();
		}
	}

	/* open keyboard */
	return kb_open();
}

/**
 * @brief Displays error message when
 * either debug print or memory
 * allocation may be not available.
 * @param[in] msg the error message.
 * @param[in] Status the NT status code.
 * @note
 * - Intended to be used after winx_exit,
 *   winx_shutdown and winx_reboot failure.
 * - Internal use only.
 */
static void print_post_scriptum(char *msg,NTSTATUS Status)
{
	char buffer[256];

	_snprintf(buffer,sizeof(buffer),"\n%s: %x: %s\n\n",
		msg,(UINT)Status,winx_get_error_description(Status));
	buffer[sizeof(buffer) - 1] = 0;
	/* winx_printf cannot be used here */
	winx_print(buffer);
}

/**
 * @brief Terminates the calling native process.
 * @details This routine releases all resources
 * used by zenwinx library before the process termination.
 * @param[in] exit_code the exit status.
 */
void winx_exit(int exit_code)
{
	NTSTATUS Status;
	
	kb_close();
	flush_dbg_log(0);
	Status = NtTerminateProcess(NtCurrentProcess(),exit_code);
	if(!NT_SUCCESS(Status)){
		print_post_scriptum("winx_exit: cannot terminate process",Status);
	}
}

/**
 * @brief Reboots the computer.
 * @note If SE_SHUTDOWN privilege adjusting fails
 * then the computer will not be rebooted and the program 
 * will continue the execution after this call.
 */
void winx_reboot(void)
{
	NTSTATUS Status;
	
	kb_close();
	MarkWindowsBootAsSuccessful();
	(void)winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE);
	flush_dbg_log(0);
	Status = NtShutdownSystem(ShutdownReboot);
	if(!NT_SUCCESS(Status)){
		print_post_scriptum("winx_reboot: cannot reboot the computer",Status);
	}
}

/**
 * @brief Shuts down the computer.
 * @note If SE_SHUTDOWN privilege adjusting fails
 * then the computer will not be shut down and the program 
 * will continue the execution after this call.
 */
void winx_shutdown(void)
{
	NTSTATUS Status;
	
	kb_close();
	MarkWindowsBootAsSuccessful();
	(void)winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE);
	flush_dbg_log(0);
	Status = NtShutdownSystem(ShutdownPowerOff);
	if(!NT_SUCCESS(Status)){
		print_post_scriptum("winx_shutdown: cannot shut down the computer",Status);
	}
}

/** @} */
