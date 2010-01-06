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
 * @file mem.c
 * @brief Memory allocation code.
 * @addtogroup Memory
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

HANDLE hGlobalHeap = NULL; /* for winx_heap_alloc call */

/* never call winx_dbg_print_ex() and winx_dbg_print() from these functions! */

/*
* winx_virtual_alloc, winx_virtual_free - ULONGLONG is 
* equal to SIZE_T on x64 systems.
*/

/**
 * @brief Allocates a block of virtual memory.
 * @param[in] size the size of the block to be allocated, in bytes.
 *                 Note that the allocated block may be bigger than
 *                 the requested size.
 * @return A pointer to the allocated block. NULL indicates failure.
 * @note
 * - Allocated memory is automatically initialized to zero.
 * - Memory protection for the allocated pages is PAGE_READWRITE.
 */
void * __stdcall winx_virtual_alloc(ULONGLONG size)
{
	void *addr = NULL;
	NTSTATUS Status;

	Status = NtAllocateVirtualMemory(NtCurrentProcess(),&addr,0,
		(SIZE_T *)(PVOID)&size,MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
	return (NT_SUCCESS(Status)) ? addr : NULL;
}

/**
 * @brief Releases a block of virtual memory.
 * @param[in] addr the memory block pointer.
 * @param[in] size the size of the block to be released, in bytes.
 */
void __stdcall winx_virtual_free(void *addr,ULONGLONG size)
{
	NtFreeVirtualMemory(NtCurrentProcess(),&addr,
		(SIZE_T *)(PVOID)&size,MEM_RELEASE);
}

/**
 * @brief Allocates a block of memory from the global growable heap.
 * @param size the size of the block to be allocated, in bytes.
 * @return A pointer to the allocated block. NULL indicates failure.
 */
void * __stdcall winx_heap_alloc(ULONGLONG size)
{
	if(hGlobalHeap == NULL) return NULL;
	return RtlAllocateHeap(hGlobalHeap,0/*HEAP_ZERO_MEMORY*/,(ULONG)size);
}

/**
 * @brief Frees a previously allocated memory block. 
 * @param[in] addr the memory block pointer.
 */
void __stdcall winx_heap_free(void *addr)
{
	if(hGlobalHeap && addr) RtlFreeHeap(hGlobalHeap,0,addr);
}

/* internal code */
void winx_create_global_heap(void)
{
	/* create growable heap with initial size of 100 kb */
	hGlobalHeap = RtlCreateHeap(HEAP_GROWABLE,NULL,0,100 * 1024,NULL,NULL);
}

void winx_destroy_global_heap(void)
{
	if(hGlobalHeap) RtlDestroyHeap(hGlobalHeap);
}

/** @} */
