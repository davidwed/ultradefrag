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

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>

#include "ntndk.h"
#include "zenwinx.h"

#define MAX_BOOT_EXEC_VALUE_SIZE 8192

short smss_key[] = \
	L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager";

/****f* zenwinx.registry/winx_reg_create_key
* NAME
*    winx_reg_create_key
* SYNOPSIS
*    error = winx_reg_create_key(name,phkey);
* FUNCTION
*    Creates the specified key in registry.
* INPUTS
*    name - key name, p.a. L"\\REGISTRY\\MACHINE\\SYSTEM\\
*           CurrentControlSet\\Control\\YourNativeAppName";
* RESULT
*    If the function succeeds, the return value zero.
*    If the function fails, the return value is (-1).
* EXAMPLE
*    if(winx_reg_create_key(name,&hkey) < 0){
*        winx_pop_error(buffer,sizeof(buffer));
*        // handle error here
*    }
* NOTES
*    Names are case sensitive on NT 4.0!
* SEE ALSO
*    winx_reg_open_key, winx_reg_close_key
******/
int __stdcall winx_reg_create_key(short *key_name,PHANDLE phKey)
{
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS Status;

	RtlInitUnicodeString(&uStr,key_name);
	/* OBJ_CASE_INSENSITIVE must be specified on NT 4.0 !!! */
	InitializeObjectAttributes(&ObjectAttributes,&uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
	Status = NtCreateKey(phKey,KEY_ALL_ACCESS,&ObjectAttributes,
				0,NULL,REG_OPTION_NON_VOLATILE,NULL);
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't create %ls key: %x!",key_name,(UINT)Status);
		*phKey = NULL;
		return -1;
	}
	return 0;
}

/****f* zenwinx.registry/winx_reg_open_key
* NAME
*    winx_reg_open_key
* SYNOPSIS
*    error = winx_reg_open_key(name,phkey);
* FUNCTION
*    Opens the specified key in registry.
* INPUTS
*    name - key name, p.a. L"\\REGISTRY\\MACHINE\\SYSTEM\\
*           CurrentControlSet\\Control\\YourNativeAppName";
* RESULT
*    If the function succeeds, the return value zero.
*    If the function fails, the return value is (-1).
* EXAMPLE
*    if(winx_reg_open_key(name,&hkey) < 0){
*        winx_pop_error(buffer,sizeof(buffer));
*        // handle error here
*    }
* NOTES
*    Names are case sensitive on NT 4.0!
* SEE ALSO
*    winx_reg_create_key, winx_reg_close_key
******/
int __stdcall winx_reg_open_key(short *key_name,PHANDLE phKey)
{
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS Status;

	RtlInitUnicodeString(&uStr,key_name);
	/* OBJ_CASE_INSENSITIVE must be specified on NT 4.0 !!! */
	InitializeObjectAttributes(&ObjectAttributes,&uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
	Status = NtOpenKey(phKey,KEY_QUERY_VALUE | KEY_SET_VALUE,&ObjectAttributes);
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't open %ls key: %x!",key_name,(UINT)Status);
		*phKey = NULL;
		return -1;
	}
	return 0;
}

/****f* zenwinx.registry/winx_reg_query_value
* NAME
*    winx_reg_query_value
* SYNOPSIS
*    error = winx_reg_query_value(hkey,name,type,buffer,size);
* FUNCTION
*    Retrieves the data for a specified registry value.
* INPUTS
*    hKey   - handle of the registry key
*    name   - value name
*    type   - value type (REG_SZ, REG_DWORD etc.)
*    buffer - pointer to a buffer that receives the data
*    size   - pointer to a variable that holds a size of buffer
* RESULT
*    If the function succeeds, the return value zero.
*    If the function fails, the return value is (-1).
* EXAMPLE
*    if(winx_reg_query_value(hKey,L"YourProgramsFlag",
*      REG_DWORD,&flag,sizeof(DWORD)) < 0){
*        winx_pop_error(buffer,sizeof(buffer));
*        // handle error here
*    }
* SEE ALSO
*    winx_reg_set_value
******/
int __stdcall winx_reg_query_value(HANDLE hKey,short *value_name,DWORD type,void *buffer,DWORD *psize)
{
	UNICODE_STRING uStr;
	NTSTATUS Status;
	KEY_VALUE_PARTIAL_INFORMATION *pInfo;
	DWORD buffer_size, n;

	if(!hKey || !buffer || !psize){
		winx_push_error("Invalid parameter!");
		goto read_fail;
	}
	if(!*psize){
		winx_push_error("Invalid parameter!");
		goto read_fail;
	}
	buffer_size = n = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + *psize;
	pInfo = (KEY_VALUE_PARTIAL_INFORMATION *)winx_virtual_alloc(n);
	if(!pInfo){
		winx_push_error("No enough memory!");
		goto read_fail;
	}
	RtlInitUnicodeString(&uStr,value_name);
	Status = NtQueryValueKey(hKey,&uStr,KeyValuePartialInformation,
		pInfo,buffer_size,&buffer_size);
	if(NT_SUCCESS(Status)){
		if(pInfo->Type == type && pInfo->DataLength <= *psize){
			RtlCopyMemory(buffer,pInfo->Data,pInfo->DataLength);
			*psize = pInfo->DataLength;
			winx_virtual_free(pInfo,n);
			return 0;
		}
		winx_push_error("Invalid value %ls!",value_name);
		winx_virtual_free(pInfo,n);
		goto read_fail;
	}
	winx_push_error("Can't read %ls value: %x!",value_name,(UINT)Status);
	winx_virtual_free(pInfo,n);
read_fail:
	if(psize) *psize = 0;
	return -1;
}

/****f* zenwinx.registry/winx_reg_set_value
* NAME
*    winx_reg_set_value
* SYNOPSIS
*    error = winx_reg_set_value(hkey,name,type,buffer,size);
* FUNCTION
*    Sets the data for a specified registry value.
* INPUTS
*    hKey   - handle of the registry key
*    name   - value name
*    type   - value type (REG_SZ, REG_DWORD etc.)
*    buffer - pointer to a buffer that contains the data
*    size   - size of buffer
* RESULT
*    If the function succeeds, the return value zero.
*    If the function fails, the return value is (-1).
* EXAMPLE
*    if(winx_reg_set_value(hKey,L"YourProgramsFlag",
*      REG_DWORD,&flag,sizeof(DWORD)) < 0){
*        winx_pop_error(buffer,sizeof(buffer));
*        // handle error here
*    }
* SEE ALSO
*    winx_reg_query_value
******/
int __stdcall winx_reg_set_value(HANDLE hKey,short *value_name,DWORD type,void *buffer,DWORD size)
{
	UNICODE_STRING uStr;
	NTSTATUS Status;

	RtlInitUnicodeString(&uStr,value_name);
	Status = NtSetValueKey(hKey,&uStr,0,type,buffer,size);
	if(!NT_SUCCESS(Status)){
		winx_push_error("Can't write %ls value: %x!",value_name,(UINT)Status);
		return -1;
	}
	return 0;
}

/****f* zenwinx.registry/winx_reg_add_to_boot_execute
* NAME
*    winx_reg_add_to_boot_execute
* SYNOPSIS
*    error = winx_reg_add_to_boot_execute(command);
* FUNCTION
*    Adds the specified command to the BootExecute list.
* INPUTS
*    command - native application name
* RESULT
*    If the function succeeds, the return value zero.
*    If the function fails, the return value is (-1).
* EXAMPLE
*    if(winx_reg_add_to_boot_execute(L"YourProgramName") < 0){
*        winx_pop_error(buffer,sizeof(buffer));
*        // handle error here
*    }
* NOTES
*    If BootExecute value is too long (above 8192 bytes),
*    this function fails.
* SEE ALSO
*    winx_reg_remove_from_boot_execute
******/
int __stdcall winx_reg_add_to_boot_execute(short *command)
{
	HANDLE hKey;
	DWORD buffer_size, data_size;
	short *data, *curr_pos;
	unsigned long length,curr_len,i;

	if(!command){
		winx_push_error("Invalid parameter!");
		return -1;
	}
	buffer_size = MAX_BOOT_EXEC_VALUE_SIZE + wcslen(command) + 10;
	buffer_size <<= 1; /* bytes = words << 1 */
	data = (short *)winx_virtual_alloc(buffer_size);
	if(!data){
		winx_push_error("No enough memory!");
		return -1;
	}
	/* add command to the BootExecute registry parameter */
	if(winx_reg_open_key(smss_key,&hKey) < 0){
		winx_virtual_free(data,buffer_size);
		return -1;
	}
	data_size = MAX_BOOT_EXEC_VALUE_SIZE << 1;
	if(winx_reg_query_value(hKey,L"BootExecute",REG_MULTI_SZ,
		data,&data_size) < 0)
	{
		NtClose(hKey);
		winx_virtual_free(data,buffer_size);
		return -1;
	}
	/* length = number of wchars without the last one */
	length = (data_size >> 1) - 1;
	for(i = 0; i < length;){
		curr_pos = data + i;
		curr_len = wcslen(curr_pos) + 1;
		if(!wcscmp(curr_pos,command)) /* if strings are equal */
			goto add_success;
		i += curr_len;
	}
	wcscpy(data + i,command);
	data[i + wcslen(command) + 1] = 0;
	length += wcslen(command) + 1 + 1;
	if(winx_reg_set_value(hKey,L"BootExecute",REG_MULTI_SZ,
		data,length << 1) < 0)
	{
		NtClose(hKey);
		winx_virtual_free(data,buffer_size);
		return -1;
	}
add_success:
	NtClose(hKey);
	winx_virtual_free(data,buffer_size);
	return 0;
}

/****f* zenwinx.registry/winx_reg_remove_from_boot_execute
* NAME
*    winx_reg_remove_from_boot_execute
* SYNOPSIS
*    error = winx_reg_remove_from_boot_execute(command);
* FUNCTION
*    Removes the specified command from the BootExecute list.
* INPUTS
*    command - native application name
* RESULT
*    If the function succeeds, the return value zero.
*    If the function fails, the return value is (-1).
* EXAMPLE
*    if(winx_reg_remove_from_boot_execute(L"YourProgramName") < 0){
*        winx_pop_error(buffer,sizeof(buffer));
*        // handle error here
*    }
* NOTES
*    If BootExecute value is too long (above 8192 bytes),
*    this function fails.
* SEE ALSO
*    winx_reg_add_to_boot_execute
******/
int __stdcall winx_reg_remove_from_boot_execute(short *command)
{
	HANDLE hKey;
	DWORD buffer_size, data_size;
	short *data, *curr_pos;
	short *new_data;
	unsigned long length,curr_len,i,new_length;


	if(!command){
		winx_push_error("Invalid parameter!");
		return -1;
	}
	buffer_size = MAX_BOOT_EXEC_VALUE_SIZE + wcslen(command) + 10;
	buffer_size <<= 1; /* bytes = words << 1 */
	data = (short *)winx_virtual_alloc(buffer_size);
	if(!data){
		winx_push_error("No enough memory!");
		return -1;
	}
	/* remove command from BootExecute registry parameter */
	if(winx_reg_open_key(smss_key,&hKey) < 0){
		winx_virtual_free(data,buffer_size);
		return -1;
	}
	data_size = MAX_BOOT_EXEC_VALUE_SIZE << 1;
	if(winx_reg_query_value(hKey,L"BootExecute",REG_MULTI_SZ,
		data,&data_size) < 0)
	{
		NtClose(hKey);
		winx_virtual_free(data,buffer_size);
		return -1;
	}

	new_data = (short *)winx_virtual_alloc(buffer_size);
	if(!new_data){
		winx_push_error("No enough memory!");
		NtClose(hKey);
		winx_virtual_free(data,buffer_size);
		return -1;
	}

	/*
	* Now we should copy all strings except specified command
	* to the destination buffer.
	*/
	memset((void *)new_data,0,buffer_size);
	/* length = number of wchars without the last one */
	length = (data_size >> 1) - 1;
	new_length = 0;
	for(i = 0; i < length;){
		curr_pos = data + i;
		curr_len = wcslen(curr_pos) + 1;
		if(wcscmp(curr_pos,command))
		{ /* if strings are not equal */
			wcscpy(new_data + new_length,curr_pos);
			new_length += curr_len;
		}
		i += curr_len;
	}
	new_data[new_length] = 0;
	if(winx_reg_set_value(hKey,L"BootExecute",REG_MULTI_SZ,
		new_data,(new_length + 1) << 1) < 0)
	{
		NtClose(hKey);
		winx_virtual_free(data,buffer_size);
		winx_virtual_free(new_data,buffer_size);
		return -1;
	}
	NtClose(hKey);
	winx_virtual_free(data,buffer_size);
	winx_virtual_free(new_data,buffer_size);
	return 0;
}

/****f* zenwinx.registry/winx_reg_close_key
* NAME
*    winx_reg_close_key
* SYNOPSIS
*    winx_reg_close_key(hkey);
* FUNCTION
*    Releases the specified registry key handle.
* INPUTS
*    hKey - registry key handle.
* RESULT
*    Nothing.
* EXAMPLE
*    if(winx_reg_open_key(name,&hkey) < 0){
*        winx_pop_error(buffer,sizeof(buffer));
*        // handle error here
*    } else {
*        // ...
*        winx_reg_close_key(hKey);
*    }
* NOTES
*    If hKey is NULL, this function does nothing.
* SEE ALSO
*    winx_reg_create_key, winx_reg_open_key
******/
void __stdcall winx_reg_close_key(HANDLE hKey)
{
	if(hKey) NtClose(hKey);
}
