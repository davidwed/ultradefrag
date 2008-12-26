/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
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

/* ldr.c - KernelGetProcAddress() implementation */

#include "driver.h"

/*
* Many thanks to Alexander Telyatnikov:
* http://alter.org.ua/en/docs/nt_kernel/procaddr/
*/

/* NT 4.0 and higher */

PVOID KernelGetModuleBase(PCHAR pModuleName)
{
    PVOID pModuleBase = NULL;
    PULONG pSystemInfoBuffer = NULL;
	NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
	ULONG SystemInfoBufferSize = 0;
	PSYSTEM_MODULE_INFORMATION_ENTRY pSysModuleEntry;
	ULONG i;

	status = ZwQuerySystemInformation(SystemModuleInformation,
		&SystemInfoBufferSize,0,&SystemInfoBufferSize);
	if(!SystemInfoBufferSize) return NULL;

	pSystemInfoBuffer = (PULONG)ExAllocatePool(NonPagedPool, SystemInfoBufferSize*2);
	if(!pSystemInfoBuffer) return NULL;

	memset(pSystemInfoBuffer, 0, SystemInfoBufferSize*2);
	status = ZwQuerySystemInformation(SystemModuleInformation,
		pSystemInfoBuffer,SystemInfoBufferSize*2,&SystemInfoBufferSize);
	if(NT_SUCCESS(status)){
		pSysModuleEntry = ((PSYSTEM_MODULE_INFORMATION)(pSystemInfoBuffer))->Module;
		for (i = 0; i <((PSYSTEM_MODULE_INFORMATION)(pSystemInfoBuffer))->Count; i++){
			if (_stricmp(pSysModuleEntry[i]./*ModuleName*/ImageName + 
			  pSysModuleEntry[i]./*ModuleNameOffset*/PathLength,pModuleName) == 0){
				pModuleBase = pSysModuleEntry[i].Base;
				break;
			}
		}
	}

    if(pSystemInfoBuffer) ExFreePool(pSystemInfoBuffer);
    return pModuleBase;
}

PVOID
RtlImageDirectoryEntryToData(
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size
    );

PVOID KernelGetProcAddress(PVOID ModuleBase,PCHAR pFunctionName)
{
	PVOID pFunctionAddress = NULL;
	ULONG size = 0;
	PIMAGE_DOS_HEADER dos;
	PIMAGE_NT_HEADERS nt;
	PIMAGE_DATA_DIRECTORY expdir;
	PIMAGE_EXPORT_DIRECTORY exports;
	ULONG addr;
	PULONG functions;
	PSHORT ordinals;
	PULONG names;
	ULONG max_name;
	ULONG max_func;
	ULONG i;
	ULONG ord;

#if 0 /* #ifndef WIN9X_SUPPORT */
	/* this function is missing on nt4, therefore we cannot use them */
	exports = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(ModuleBase,
		TRUE,IMAGE_DIRECTORY_ENTRY_EXPORT,&size);
	addr = (PUCHAR)((ULONG)exports-(ULONG)ModuleBase);
#else
	dos = (PIMAGE_DOS_HEADER) ModuleBase;
	nt = (PIMAGE_NT_HEADERS)((ULONG)ModuleBase + dos->e_lfanew);
	expdir = (PIMAGE_DATA_DIRECTORY)(nt->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_EXPORT);
	addr = expdir->VirtualAddress;
	exports = (PIMAGE_EXPORT_DIRECTORY)((ULONG)ModuleBase + addr);
#endif

	functions = (PULONG)((ULONG)ModuleBase + exports->AddressOfFunctions);
	ordinals = (PSHORT)((ULONG)ModuleBase + exports->AddressOfNameOrdinals);
	names = (PULONG)((ULONG)ModuleBase + exports->AddressOfNames);
	max_name = exports->NumberOfNames;
	max_func = exports->NumberOfFunctions;

	for (i = 0; i < max_name; i++){
		ord = ordinals[i];
		if(i >= max_name || ord >= max_func) return NULL;

		if(functions[ord] < addr || functions[ord] >= addr + size){
			if(strcmp((PCHAR)ModuleBase + names[i], pFunctionName) == 0){
				pFunctionAddress =(PVOID)((PCHAR)ModuleBase + functions[ord]);
				break;
			}
		}
	}

	return pFunctionAddress;
}
