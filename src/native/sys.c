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
 *  Additional system calls.
 */

#include "defrag_native.h"
#include "../include/ultradfgver.h"

BOOL EnablePrivilege(DWORD dwLowPartOfLUID)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;
	NTSTATUS status;
	HANDLE UserToken;

	NtOpenProcessToken(NtCurrentProcess(),MAXIMUM_ALLOWED/*0x2000000*/,&UserToken);

	luid.HighPart = 0x0;
	luid.LowPart = dwLowPartOfLUID;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	status = NtAdjustPrivilegesToken(UserToken,FALSE,&tp,sizeof(TOKEN_PRIVILEGES),
									(PTOKEN_PRIVILEGES)NULL,(PDWORD)NULL);
	return NT_SUCCESS(status);
}

void IntSleep(DWORD dwMilliseconds)
{
	LARGE_INTEGER Interval;

	if (dwMilliseconds != INFINITE)
	{
		/* System time units are 100 nanoseconds. */
		Interval.QuadPart = -((signed long)dwMilliseconds * 10000);
	}
	else
	{
		/* Approximately 292000 years hence */
		Interval.QuadPart = -0x7FFFFFFFFFFFFFFF;
	}
	NtDelayExecution (FALSE, &Interval);
}

