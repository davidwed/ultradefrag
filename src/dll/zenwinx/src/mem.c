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

/****f* zenwinx.memory/winx_virtual_alloc
 *  NAME
 *     winx_virtual_alloc
 *  SYNOPSIS
 *     addr = winx_virtual_alloc(size);
 *  FUNCTION
 *     Commit a region of pages in the virtual 
 *     address space of the calling process.
 *  INPUTS
 *     size - size of the region, in bytes.
 *  RESULT
 *     If the function succeeds, the return value is
 *     the base address of the allocated region of pages.
 *     If the function fails, the return value is NULL.
 *  EXAMPLE
 *     buffer = winx_virtual_alloc(1024);
 *     if(!buffer)
 *     {
 *         winx_printf("Can't allocate 1024 bytes of memory!\n");
 *         winx_exit(2); // exit with code 2
 *     }
 *     // ...
 *     winx_virtual_free(buffer, 1024);
 *  NOTES
 *     1. Memory allocated by this function
 *        is automatically initialized to zero.
 *     2. Memory protection for the region of
 *        pages to be allocated is PAGE_READWRITE.
 *  SEE ALSO
 *     winx_virtual_free
 ******/
void * __stdcall winx_virtual_alloc(unsigned long size)
{
	void *addr = NULL;
	NTSTATUS Status;

	Status = NtAllocateVirtualMemory(NtCurrentProcess(),
				&addr,0,&size,MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
	return (NT_SUCCESS(Status)) ? addr : NULL;
}

/****f* zenwinx.memory/winx_virtual_free
 *  NAME
 *     winx_virtual_free
 *  SYNOPSIS
 *     winx_virtual_free(addr, size);
 *  FUNCTION
 *     Release a region of pages within the
 *     virtual address space of the calling process.
 *  INPUTS
 *     addr - pointer to the base address
 *            of the region of pages to be freed.
 *     size - size of the region of memory
 *            to be freed, in bytes.
 *  RESULT
 *     This function does not return a value.
 *  EXAMPLE
 *     See an example for the winx_virtual_alloc() function.
 *  NOTES
 *     After this call you can never refer
 *     to the specified memory again.
 *  SEE ALSO
 *     winx_virtual_alloc
 ******/
void __stdcall winx_virtual_free(void *addr,unsigned long size)
{
	NtFreeVirtualMemory(NtCurrentProcess(),&addr,&size,MEM_RELEASE);
}
