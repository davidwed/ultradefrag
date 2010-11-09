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
 * @file privilege.c
 * @brief User privileges.
 * @addtogroup Privileges
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

/**
 * @brief Enables a user privilege for the current process.
 * @param[in] luid identifier of the requested privilege, 
 * ntndk.h file contains definitions of various privileges.
 * @return Zero for success, negative value otherwise.
 * @par Example:
 * @code
 * winx_enable_privilege(SE_SHUTDOWN_PRIVILEGE);
 * @endcode
 */
int __stdcall winx_enable_privilege(unsigned long luid)
{
	HANDLE hToken = NULL;
	NTSTATUS Status;
	TOKEN_PRIVILEGES tp;
	LUID luid_struct;
	
	/* TODO: investigate whether it is possible to use simpler RtlAdjustPrivilege API or not */

	Status = NtOpenProcessToken(NtCurrentProcess(),MAXIMUM_ALLOWED,&hToken);
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"winx_enable_privilege: cannot enable privilege %x: open token failure",(UINT)luid);
		return (-1);
	}

	luid_struct.HighPart = 0x0;
	luid_struct.LowPart = luid;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid_struct;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	Status = NtAdjustPrivilegesToken(hToken,0/*FALSE*/,&tp,sizeof(TOKEN_PRIVILEGES),
									(PTOKEN_PRIVILEGES)NULL,(PDWORD)NULL);
	NtCloseSafe(hToken);
	if(Status == STATUS_NOT_ALL_ASSIGNED || !NT_SUCCESS(Status)){
		DebugPrintEx(Status,"winx_enable_privilege: cannot enable privilege 0x%x",(UINT)luid);
		return (-1);
	}
	return 0;
}

/** @} */
