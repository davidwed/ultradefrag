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
* zenwinx.dll miscellaneous system functions.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>

#include "ntndk.h"
#include "zenwinx.h"

NTSTATUS (__stdcall *func_RtlGetVersion)(PRTL_OSVERSIONINFOW lpVersionInformation);

/****f* zenwinx.misc/winx_sleep
* NAME
*    winx_sleep
* SYNOPSIS
*    winx_sleep(msec);
* FUNCTION
*    Suspends the execution of the current thread
*    for a specified interval.
* INPUTS
*    msec - time interval, in milliseconds.
* RESULT
*    This function does not return a value.
* EXAMPLE
*    winx_sleep(1000); // wait one second
* NOTES
*    If msec is INFINITE, the function's
*    time-out interval never elapses.
******/
void __stdcall winx_sleep(int msec)
{
	LARGE_INTEGER Interval;

	if(msec != INFINITE){
		/* System time units are 100 nanoseconds. */
		Interval.QuadPart = -((signed long)msec * 10000);
	} else {
		/* Approximately 292000 years hence */
		Interval.QuadPart = MAX_WAIT_INTERVAL;
	}
	NtDelayExecution(FALSE,&Interval);
}

/****f* zenwinx.misc/winx_get_os_version
* NAME
*    winx_get_os_version
* SYNOPSIS
*    ver = winx_get_os_version();
* FUNCTION
*    Returns the version of Windows.
* INPUTS
*    Nothing.
* RESULT
*    40 - nt 4.0
*    50 - w2k
*    51 - XP
*    52 - server 2003
*    60 - Vista
* EXAMPLE
*    if(winx_get_os_version() >= 51){
*        // we have at least xp system; but
*        // unfortunately it may be incompatible
*        // with xp itself :(
*    }
* NOTES
*    For nt 4.0 and later systems only.
******/
int __stdcall winx_get_os_version(void)
{
	OSVERSIONINFOW ver;

	if(winx_get_proc_address(L"ntdll.dll","RtlGetVersion",
	  (void *)&func_RtlGetVersion) < 0){
		winx_pop_error(NULL,0);	
		return 40;
	}
	func_RtlGetVersion(&ver);
	return (ver.dwMajorVersion * 10 + ver.dwMinorVersion);
}

/****f* zenwinx.misc/winx_get_windows_directory
* NAME
*    winx_get_windows_directory
* SYNOPSIS
*    error = winx_get_windows_directory(buffer, length);
* FUNCTION
*    Returns %windir% path. In native form, as follows: \??\C:\WINDOWS
* INPUTS
*    buffer - pointer to the buffer to receive
*             the null-terminated path.
*    length - length of the buffer, usually equal to MAX_PATH.
* RESULT
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value.
* EXAMPLE
*    if(winx_get_windows_directory(buffer,sizeof(buffer)) < 0){
*        winx_pop_error(NULL,0);
*        // something is wrong...
*    }
* NOTES
*    Buffer should be at least MAX_PATH characters long.
******/
int __stdcall winx_get_windows_directory(char *buffer, int length)
{
	NTSTATUS Status;
	UNICODE_STRING name, us;
	ANSI_STRING as;
	short buf[MAX_PATH + 1];

	if(!buffer || length <= 0){
		winx_push_error("Invalid parameter!");
		return (-1);
	}

	RtlInitUnicodeString(&name,L"SystemRoot");
	us.Buffer = buf;
	us.Length = 0;
	us.MaximumLength = MAX_PATH * sizeof(short);
	Status = RtlQueryEnvironmentVariable_U(NULL,&name,&us);
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't query SystemRoot variable: %x!",(UINT)Status);
		return -1;
	}

	if(RtlUnicodeStringToAnsiString(&as,&us,TRUE) != STATUS_SUCCESS){
		winx_push_error("No enough memory!");
		return -1;
	}

	strcpy(buffer,"\\??\\");
	strncat(buffer,as.Buffer,length - strlen(buffer) - 1);
	buffer[length - 1] = 0; /* important ? */
	RtlFreeAnsiString(&as);
	return 0;
}

/****f* zenwinx.misc/winx_set_system_error_mode
* NAME
*    winx_set_system_error_mode
* SYNOPSIS
*    error = winx_set_system_error_mode(mode);
* FUNCTION
*    SetErrorMode() native analog.
* INPUTS
*    mode - process error mode.
* RESULT
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value.
* EXAMPLE
*    if(winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS) < 0){
*        winx_pop_error(buffer,sizeof(buffer));
*        // handle error here
*    }
* NOTES
*    1. Mode constants aren't the same as in SetErrorMode() call.
*    2. Use INTERNAL_SEM_FAILCRITICALERRORS constant to 
*    disable the critical-error-handler message box. After that
*    you can p.a. try to read a missing floppy disk without any 
*    popup windows displaying error messages.
*    3. winx_set_system_error_mode(1) call is equal
*    to SetErrorMode(0).
*    4. Other mode constants can be found in ReactOS source 
*    code. They needs to be tested before including into this 
*    documentation to ensure that they are doing what expected.
******/
int __stdcall winx_set_system_error_mode(unsigned int mode)
{
	NTSTATUS Status;

	Status = NtSetInformationProcess(NtCurrentProcess(),
					ProcessDefaultHardErrorMode,
					(PVOID)&mode,
					sizeof(int));
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't set error mode %u: %x!",mode,(UINT)Status);
		return (-1);
	}
	return 0;
}

/****f* zenwinx.misc/winx_load_driver
* NAME
*    winx_load_driver
* SYNOPSIS
*    error = winx_load_driver(driver_name);
* FUNCTION
*    Loads the specified driver.
* INPUTS
*    driver_name - name of the driver
* RESULT
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value.
* EXAMPLE
*    if(winx_load_driver(L"ultradfg") < 0){
*        winx_pop_error(buffer,sizeof(buffer));
*        // the UltraDefrag driver cannot be loaded,
*        // handle error here
*    }
* NOTES
*    If the specified driver is already loaded,
*    this function returns success.
* SEE ALSO
*    winx_unload_driver
******/
int __stdcall winx_load_driver(short *driver_name)
{
	UNICODE_STRING us;
	short driver_key[128]; /* enough for any driver registry path */
	NTSTATUS Status;

	wcscpy(driver_key,L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
	if(wcslen(driver_name) > (128 - wcslen(driver_key) - 1)){
		winx_push_error("Driver name %ws is too long!", driver_name);
		return (-1);
	}
	wcscat(driver_key,driver_name);
	RtlInitUnicodeString(&us,driver_key);
	Status = NtLoadDriver(&us);
	if(!NT_SUCCESS(Status) && Status != STATUS_IMAGE_ALREADY_LOADED){
		winx_push_error("Can't load %ws driver: %x!",driver_name,(UINT)Status);
		return (-1);
	}
	return 0;
}

/****f* zenwinx.misc/winx_unload_driver
* NAME
*    winx_unload_driver
* SYNOPSIS
*    error = winx_unload_driver(driver_name);
* FUNCTION
*    Unloads the specified driver.
* INPUTS
*    driver_name - name of the driver
* RESULT
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value.
* EXAMPLE
*    if(winx_unload_driver(L"ultradfg") < 0){
*        winx_pop_error(buffer,sizeof(buffer));
*        // the UltraDefrag driver cannot be unloaded,
*        // handle error here
*    }
* SEE ALSO
*    winx_load_driver
******/
int __stdcall winx_unload_driver(short *driver_name)
{
	UNICODE_STRING us;
	short driver_key[128]; /* enough for any driver registry path */
	NTSTATUS Status;

	wcscpy(driver_key,L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
	if(wcslen(driver_name) > (128 - wcslen(driver_key) - 1)){
		winx_push_error("Driver name %ws is too long!", driver_name);
		return (-1);
	}
	wcscat(driver_key,driver_name);
	RtlInitUnicodeString(&us,driver_key);
	Status = NtUnloadDriver(&us);
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't unload %ws driver: %x!",driver_name,(UINT)Status);
		return (-1);
	}
	return 0;
}
