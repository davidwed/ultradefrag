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
* zenwinx.dll - user privileges manipulation functions.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>

#include "ntndk.h"
#include "zenwinx.h"

/****f* zenwinx.privilege/winx_enable_privilege
* NAME
*    winx_enable_privilege
* SYNOPSIS
*    error = winx_enable_privilege(token,privilege_id);
* FUNCTION
*    Enables specified privilege for the specified token.
* INPUTS
*    token        - process / thread token.
*    privilege_id - one of the privilege codes from ntndk.h
* RESULT
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value.
* EXAMPLE
*    if(winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE) < 0){
*        winx_pop_error(NULL,0);
*        // handle error
*    }
******/
int __stdcall winx_enable_privilege(HANDLE hToken,DWORD dwLowPartOfLUID)
{
	NTSTATUS Status;
	TOKEN_PRIVILEGES tp;
	LUID luid;

	luid.HighPart = 0x0;
	luid.LowPart = dwLowPartOfLUID;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	Status = NtAdjustPrivilegesToken(hToken,FALSE,&tp,sizeof(TOKEN_PRIVILEGES),
									(PTOKEN_PRIVILEGES)NULL,(PDWORD)NULL);
	if(Status == STATUS_NOT_ALL_ASSIGNED || !NT_SUCCESS(Status)){
		winx_push_error("Can't enable privilege: %x : %x!",
				(UINT)dwLowPartOfLUID,(UINT)Status);
		return (-1);
	}
	return 0;
}
