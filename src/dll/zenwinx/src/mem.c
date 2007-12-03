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
 *  zenwinx.dll functions to allocate and free memory.
 */

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ntndk.h"
#include "zenwinx.h"

void * __stdcall winx_virtual_alloc(unsigned long size)
{
	void *addr = NULL;
	NTSTATUS Status;

	Status = NtAllocateVirtualMemory(NtCurrentProcess(),
				&addr,0,&size,MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
	return (NT_SUCCESS(Status)) ? addr : NULL;
}

void __stdcall winx_virtual_free(void *addr,unsigned long size)
{
	NtFreeVirtualMemory(NtCurrentProcess(),&addr,&size,MEM_RELEASE);
}
