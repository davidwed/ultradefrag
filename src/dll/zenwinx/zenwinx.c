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
 * @brief Startup and shutdown.
 * @addtogroup StartupAndShutdown
 * @{
 */

#include "zenwinx.h"

int  __stdcall kb_open(void);
void __stdcall kb_close(void);

void winx_create_global_heap(void);
void winx_destroy_global_heap(void);
void winx_init_synch_objects(void);
void winx_destroy_synch_objects(void);
void __stdcall MarkWindowsBootAsSuccessful(void);
char * __stdcall winx_get_error_description(unsigned long status);
void winx_print(char *string);
void winx_flush_dbg_log(void);

void test(void)
{
	winx_file_info *f;
	
	//f = winx_ftw(L"\\??\\c:\\",WINX_FTW_RECURSIVE | WINX_FTW_DUMP_FILES,NULL,NULL);
	f = winx_scan_disk('L',WINX_FTW_DUMP_FILES,NULL,NULL,NULL,NULL);
	if(f == NULL)
		DebugPrint("$$$$ winx_scan_disk failed $$$");
	else
		winx_ftw_release(f);
}

#ifndef STATIC_LIB
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH){
		zenwinx_native_init();
		//test();
	} else if(dwReason == DLL_PROCESS_DETACH){
		zenwinx_native_unload();
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
int __stdcall winx_init(void *peb)
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
void __stdcall winx_exit(int exit_code)
{
	NTSTATUS Status;
	
	kb_close();
	winx_flush_dbg_log();
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
void __stdcall winx_reboot(void)
{
	NTSTATUS Status;
	
	kb_close();
	MarkWindowsBootAsSuccessful();
	(void)winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE);
	winx_flush_dbg_log();
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
void __stdcall winx_shutdown(void)
{
	NTSTATUS Status;
	
	kb_close();
	MarkWindowsBootAsSuccessful();
	(void)winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE);
	winx_flush_dbg_log();
	Status = NtShutdownSystem(ShutdownPowerOff);
	if(!NT_SUCCESS(Status)){
		print_post_scriptum("winx_shutdown: cannot shut down the computer",Status);
	}
}

/** @} */
