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
