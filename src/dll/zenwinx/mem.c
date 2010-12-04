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
 * @brief Memory.
 * @addtogroup Memory
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

void winx_print(char *string);

HANDLE hGlobalHeap = NULL; /* for winx_heap_alloc_ex call */

/*
* NOTE: Never call winx_dbg_print_ex() and
* winx_dbg_print() from these functions!
*/

/**
 * @brief Allocates a block of virtual memory.
 * @param[in] size the size of the block to be allocated, in bytes.
 * Note that the allocated block may be bigger than the requested size.
 * @return A pointer to the allocated block. NULL indicates failure.
 * @note
 * - Allocated memory is automatically initialized to zero.
 * - Memory protection for the allocated pages is PAGE_READWRITE.
 */
void * __stdcall winx_virtual_alloc(SIZE_T size)
{
	void *addr = NULL;
	NTSTATUS Status;

	Status = NtAllocateVirtualMemory(NtCurrentProcess(),&addr,0,
		&size,MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
	return (NT_SUCCESS(Status)) ? addr : NULL;
}

/**
 * @brief Releases a block of virtual memory.
 * @param[in] addr the memory block pointer.
 * @param[in] size the size of the block to be released, in bytes.
 */
void __stdcall winx_virtual_free(void *addr,SIZE_T size)
{
	(void)NtFreeVirtualMemory(NtCurrentProcess(),&addr,&size,MEM_RELEASE);
}

/**
 * @brief Allocates a block of memory from the global growable heap.
 * @param size the size of the block to be allocated, in bytes.
 * Note that the allocated block may be bigger than the requested size.
 * @param flags HEAP_ZERO_MEMORY to initialize memory to zero, zero otherwise.
 * @return A pointer to the allocated block. NULL indicates failure.
 */
void * __stdcall winx_heap_alloc_ex(SIZE_T size,SIZE_T flags)
{
	if(hGlobalHeap == NULL) return NULL;
	return RtlAllocateHeap(hGlobalHeap,flags,size); 
}

/**
 * @brief Frees a previously allocated memory block. 
 * @param[in] addr the memory block pointer.
 */
void __stdcall winx_heap_free(void *addr)
{
	if(hGlobalHeap && addr) (void)RtlFreeHeap(hGlobalHeap,0,addr);
}

/* internal code */
void winx_create_global_heap(void)
{
	/* create growable heap with initial size of 100 kb */
	if(hGlobalHeap == NULL){
		hGlobalHeap = RtlCreateHeap(HEAP_GROWABLE,NULL,0,100 * 1024,NULL,NULL);
	}
	if(hGlobalHeap == NULL){
		DebugPrint("Cannot create global memory heap!");
		/* winx_printf cannot be used here */
		winx_print("\nCannot create global memory heap!\n");
	}
}

void winx_destroy_global_heap(void)
{
	if(hGlobalHeap) (void)RtlDestroyHeap(hGlobalHeap);
}

/** @} */
