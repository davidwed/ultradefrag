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
* zenwinx.dll registry functions.
*/

/* Use standard RtlXxx functions instead of writing specialized versions of them. */

#include "ntndk.h"
#include "zenwinx.h"

static int __stdcall open_smss_key(HANDLE *pKey);
static int __stdcall read_boot_exec_value(HANDLE hKey,void **data,DWORD *size);
static int __stdcall write_boot_exec_value(HANDLE hKey,void *data,DWORD size);
static void __stdcall flush_smss_key(HANDLE hKey);

/* The following two functions replaces bootexctrl in native mode. */

/****f* zenwinx.registry/winx_register_boot_exec_command
* NAME
*    winx_register_boot_exec_command
* SYNOPSIS
*    error = winx_register_boot_exec_command(command);
* FUNCTION
*    Registers a specified command to be executed during 
*    Windows boot process.
* INPUTS
*    command    - command name, without extension
* RESULT
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value.
* EXAMPLE
*    (void)winx_register_boot_exec_command(L"defrag_native");
* NOTES
*    Specified command's executable must be placed inside 
*    a system32 directory to be executed successfully.
* SEE ALSO
*    winx_unregister_boot_exec_command
******/
int __stdcall winx_register_boot_exec_command(short *command)
{
	HANDLE hKey;
	KEY_VALUE_PARTIAL_INFORMATION *data;
	DWORD size, value_size;
	short *value, *pos;
	DWORD length, i, len;
	
	if(open_smss_key(&hKey) < 0) return (-1);
	size = (wcslen(command) + 1) * sizeof(short);
	if(read_boot_exec_value(hKey,(void **)(void *)&data,&size) < 0){
		NtCloseSafe(hKey);
		return (-1);
	}
	
	if(data->Type != REG_MULTI_SZ){
		winx_dbg_print("BootExecute value has wrong type 0x%x!",
				data->Type);
		winx_virtual_free((void *)data,size);
		NtCloseSafe(hKey);
		return (-1);
	}
	
	value = (short *)(data->Data);
	length = (data->DataLength >> 1) - 1;
	for(i = 0; i < length;){
		pos = value + i;
		//winx_dbg_print("%ws",pos);
		len = wcslen(pos) + 1;
		if(!wcscmp(pos,command)) goto done;
		i += len;
	}
	wcscpy(value + i,command);
	value[i + wcslen(command) + 1] = 0;

	value_size = (i + wcslen(command) + 1 + 1) * sizeof(short);
	if(write_boot_exec_value(hKey,(void *)(data->Data),value_size) < 0){
		winx_virtual_free((void *)data,size);
		NtCloseSafe(hKey);
		return (-1);
	}

done:	
	winx_virtual_free((void *)data,size);
	flush_smss_key(hKey); /* required by native app before shutdown */
	NtCloseSafe(hKey);
	return 0;
}

/****f* zenwinx.registry/winx_unregister_boot_exec_command
* NAME
*    winx_unregister_boot_exec_command
* SYNOPSIS
*    error = winx_unregister_boot_exec_command(command);
* FUNCTION
*    Deregisters a specified command from being executed during 
*    Windows boot process.
* INPUTS
*    command    - command name, without extension
* RESULT
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value.
* EXAMPLE
*    (void)winx_unregister_boot_exec_command(L"defrag_native");
* SEE ALSO
*    winx_register_boot_exec_command
******/
int __stdcall winx_unregister_boot_exec_command(short *command)
{
	HANDLE hKey;
	KEY_VALUE_PARTIAL_INFORMATION *data;
	DWORD size;
	short *value, *pos;
	DWORD length, i, len;
	short *new_value;
	DWORD new_value_size;
	DWORD new_length;
	
	if(open_smss_key(&hKey) < 0) return (-1);
	size = (wcslen(command) + 1) * sizeof(short);
	if(read_boot_exec_value(hKey,(void **)(void *)&data,&size) < 0){
		NtCloseSafe(hKey);
		return (-1);
	}
	
	if(data->Type != REG_MULTI_SZ){
		winx_dbg_print("BootExecute value has wrong type 0x%x!",
				data->Type);
		winx_virtual_free((void *)data,size);
		NtCloseSafe(hKey);
		return (-1);
	}
	
	value = (short *)(data->Data);
	length = (data->DataLength >> 1) - 1;
	
	new_value_size = (length + 1) << 1;
	new_value = winx_virtual_alloc(new_value_size);
	if(!new_value){
		winx_dbg_print("Cannot allocate %u bytes of memory"
						 "for new BootExecute value!",new_value_size);
		winx_virtual_free((void *)data,size);
		NtCloseSafe(hKey);
		return (-1);
	}

	memset((void *)new_value,0,new_value_size);
	new_length = 0;
	for(i = 0; i < length;){
		pos = value + i;
		//winx_dbg_print("%ws",pos);
		len = wcslen(pos) + 1;
		if(wcscmp(pos,command)){
			wcscpy(new_value + new_length,pos);
			new_length += len;
		}
		i += len;
	}
	new_value[new_length] = 0;
	
	if(write_boot_exec_value(hKey,(void *)new_value,
	  (new_length + 1) * sizeof(short)) < 0){
		winx_virtual_free((void *)new_value,new_value_size);
		winx_virtual_free((void *)data,size);
		NtCloseSafe(hKey);
		return (-1);
	}

	winx_virtual_free((void *)new_value,new_value_size);
	winx_virtual_free((void *)data,size);
	flush_smss_key(hKey); /* required by native app before shutdown */
	NtCloseSafe(hKey);
	return 0;
}

/* internal functions */
static int __stdcall open_smss_key(HANDLE *pKey)
{
	UNICODE_STRING us;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS status;
	
	RtlInitUnicodeString(&us,L"\\Registry\\Machine\\SYSTEM\\"
							 L"CurrentControlSet\\Control\\Session Manager");
	InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);
	status = NtOpenKey(pKey,KEY_QUERY_VALUE | KEY_SET_VALUE,&oa);
	if(status != STATUS_SUCCESS){
		winx_dbg_print_ex("Can't open %ws: %x!",
			us.Buffer,(UINT)status);
		return (-1);
	}
	return 0;
}

/* third parameter - in bytes */
static int __stdcall read_boot_exec_value(HANDLE hKey,void **data,DWORD *size)
{
	void *data_buffer = NULL;
	DWORD data_size = 0;
	DWORD data_size2 = 0;
	DWORD additional_space_size = *size;
	UNICODE_STRING us;
	NTSTATUS status;
	
	RtlInitUnicodeString(&us,L"BootExecute");
	status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
			NULL,0,&data_size);
	if(status != STATUS_BUFFER_TOO_SMALL){
		winx_dbg_print_ex("Cannot query BootExecute value size: %x!",
				(UINT)status);
		return (-1);
	}
	data_size += additional_space_size;
	data_buffer = winx_virtual_alloc(data_size); /* allocates zero filled buffer */
	status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
			data_buffer,data_size,&data_size2);
	if(status != STATUS_SUCCESS){
		winx_dbg_print_ex("Cannot query BootExecute value: %x!",
				(UINT)status);
		winx_virtual_free(data_buffer,data_size);
		return (-1);
	}
	
	*data = data_buffer;
	*size = data_size;
	return 0;
}

static int __stdcall write_boot_exec_value(HANDLE hKey,void *data,DWORD size)
{
	UNICODE_STRING us;
	NTSTATUS status;
	
	RtlInitUnicodeString(&us,L"BootExecute");
	status = NtSetValueKey(hKey,&us,0,REG_MULTI_SZ,data,size);
	if(status != STATUS_SUCCESS){
		winx_dbg_print_ex("Cannot set BootExecute value: %x!",
				(UINT)status);
		return (-1);
	}
	
	return 0;
}

static void __stdcall flush_smss_key(HANDLE hKey)
{
	NTSTATUS status;
	
	status = NtFlushKey(hKey);
	if(status != STATUS_SUCCESS)
		winx_dbg_print_ex("Cannot update Session Manager "
				"registry key on disk: %x!",(UINT)status);
}
