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
 * @file misc.c
 * @brief Miscellaneous system functions code.
 * @addtogroup Miscellaneous
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

NTSTATUS (__stdcall *func_RtlGetVersion)(PRTL_OSVERSIONINFOW lpVersionInformation);

/**
 * @brief Suspends the execution of the current thread.
 * @param[in] msec the time interval, in milliseconds.
 *                 If an INFINITE constant is passed, the
 *                 time-out interval never elapses.
 */
void __stdcall winx_sleep(int msec)
{
	LARGE_INTEGER Interval;

	if(msec != INFINITE){
		/* System time units are 100 nanoseconds. */
		Interval.QuadPart = -((signed long)msec * 10000);
	} else {
		/* Approximately 292000 years hence */
		Interval.QuadPart = MAX_WAIT_INTERVAL;
	}
	/*
	* The next call is undocumented, therefore
	* we are not checking its result.
	*/
	(void)NtDelayExecution(0/*FALSE*/,&Interval);
}

/**
 * @brief Returns the version of Windows.
 * @return major_version_number * 100 + minor_version_number.
 * @note Works fine on NT 4.0 and later systems. Otherwise 
 *       always returns 400.
 * @par Example:
 * @code 
 * if(winx_get_os_version() >= 501){
 *     // we are running on XP or later system
 * }
 * @endcode
 */
int __stdcall winx_get_os_version(void)
{
	RTL_OSVERSIONINFOW ver;
	
	ver.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

	if(winx_get_proc_address(L"ntdll.dll","RtlGetVersion",
	  (void *)&func_RtlGetVersion) < 0) return 400;
	func_RtlGetVersion(&ver);
	return (ver.dwMajorVersion * 100 + ver.dwMinorVersion);
}

/**
 * @brief Retrieves the path of the Windows directory.
 * @param[out] buffer pointer to the buffer to receive
 *                    the null-terminated path.
 * @param[in]  length the length of the buffer, in characters.
 * @return Zero for success, negative value otherwise.
 * @note This function retrieves a native path, like this 
 *       \\??\\C:\\WINDOWS
 */
int __stdcall winx_get_windows_directory(char *buffer, int length)
{
	short buf[MAX_PATH + 1];

	DbgCheck2(buffer,(length > 0),"winx_get_windows_directory",-1);
	
	if(winx_query_env_variable(L"SystemRoot",buf,MAX_PATH) < 0) return (-1);
	(void)_snprintf(buffer,length - 1,"\\??\\%ws",buf);
	buffer[length - 1] = 0;
	return 0;
}

/**
 * @brief Queries a symbolic link.
 * @param[in]  name   the name of symbolic link.
 * @param[out] buffer pointer to the buffer to receive
 *                    the null-terminated target.
 * @param[in]  length of the buffer, in characters.
 * @return Zero for success, negative value otherwise.
 * @par Example:
 * @code
 * winx_query_symbolic_link(L"\\??\\C:",buffer,BUFFER_LENGTH);
 * // now the buffer may contain \Device\HarddiskVolume1 or something like that
 * @endcode
 */
int __stdcall winx_query_symbolic_link(short *name, short *buffer, int length)
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING uStr;
	NTSTATUS Status;
	HANDLE hLink;
	ULONG size;

	DbgCheck3(name,buffer,(length > 0),"winx_query_symbolic_link",-1);
	
	RtlInitUnicodeString(&uStr,name);
	InitializeObjectAttributes(&oa,&uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
	Status = NtOpenSymbolicLinkObject(&hLink,SYMBOLIC_LINK_QUERY,&oa);
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"Cannot open symbolic link %ls",name);
		return (-1);
	}
	uStr.Buffer = buffer;
	uStr.Length = 0;
	uStr.MaximumLength = length * sizeof(short);
	size = 0;
	Status = NtQuerySymbolicLinkObject(hLink,&uStr,&size);
	(void)NtClose(hLink);
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"Cannot query symbolic link %ls",name);
		return (-1);
	}
	buffer[length - 1] = 0;
	return 0;
}

/**
 * @brief Sets a system error mode.
 * @param[in] mode the process error mode.
 * @return Zero for success, negative value otherwise.
 * @note 
 * - Mode constants aren't the same as in Win32 SetErrorMode() call.
 * - Use INTERNAL_SEM_FAILCRITICALERRORS constant to 
 *   disable the critical-error-handler message box. After that
 *   you can for example try to read a missing floppy disk without 
 *   any popup windows displaying error messages.
 * - winx_set_system_error_mode(1) call is equal to SetErrorMode(0).
 * - Other mode constants can be found in ReactOS sources, but
 *   they needs to be tested meticulously because they were never
 *   officially documented.
 * @par Example:
 * @code
 * winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS);
 * @endcode
 */
int __stdcall winx_set_system_error_mode(unsigned int mode)
{
	NTSTATUS Status;

	Status = NtSetInformationProcess(NtCurrentProcess(),
					ProcessDefaultHardErrorMode,
					(PVOID)&mode,
					sizeof(int));
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"Cannot set system error mode %u",mode);
		return (-1);
	}
	return 0;
}

/**
 * @brief Loads a driver.
 * @param[in] driver_name the name of the driver exactly
 *                        as written in system registry.
 * @return Zero for success, negative value otherwise.
 * @note When the driver is already loaded this function returns
 *       success.
 */
int __stdcall winx_load_driver(short *driver_name)
{
	UNICODE_STRING us;
	short driver_key[128]; /* enough for any driver registry path */
	NTSTATUS Status;

	DbgCheck1(driver_name,"winx_load_driver",-1);

	(void)_snwprintf(driver_key,127,L"%ls%ls",
			L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\",driver_name);
	driver_key[127] = 0;
	RtlInitUnicodeString(&us,driver_key);
	Status = NtLoadDriver(&us);
	if(!NT_SUCCESS(Status) && Status != STATUS_IMAGE_ALREADY_LOADED){
		DebugPrintEx(Status,"Cannot load %ws driver",driver_name);
		return (-1);
	}
	return 0;
}

/**
 * @brief Unloads a driver.
 * @param[in] driver_name the name of the driver exactly
 *                        as written in system registry.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall winx_unload_driver(short *driver_name)
{
	UNICODE_STRING us;
	short driver_key[128]; /* enough for any driver registry path */
	NTSTATUS Status;

	DbgCheck1(driver_name,"winx_unload_driver",-1);

	(void)_snwprintf(driver_key,127,L"%ls%ls",
			L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\",driver_name);
	driver_key[127] = 0;
	RtlInitUnicodeString(&us,driver_key);
	Status = NtUnloadDriver(&us);
	if(!NT_SUCCESS(Status)){
		DebugPrintEx(Status,"Cannot unload %ws driver",driver_name);
		return (-1);
	}
	return 0;
}

/**
 * @brief Determines whether WIndows is in Safe Mode or not.
 * @return Positive value indicates the presence of the Safe Mode.
 * Zero value indicates a normal boot. Negative value indicates
 * indeterminism caused by impossibility of an appropriate check.
 */
int __stdcall winx_windows_in_safe_mode(void)
{
	UNICODE_STRING us;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS status;
	HANDLE hKey;
	KEY_VALUE_PARTIAL_INFORMATION *data;
	short *data_buffer = NULL;
	DWORD data_size = 0;
	DWORD data_size2 = 0;
	DWORD data_length;
	int safe_boot = 0;
	
	/* 1. open HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control registry key */
	RtlInitUnicodeString(&us,L"\\Registry\\Machine\\SYSTEM\\"
							 L"CurrentControlSet\\Control");
	InitializeObjectAttributes(&oa,&us,OBJ_CASE_INSENSITIVE,NULL,NULL);
	status = NtOpenKey(&hKey,KEY_QUERY_VALUE,&oa);
	if(status != STATUS_SUCCESS){
		DebugPrintEx(status,"Cannot open %ws",us.Buffer);
		winx_printf("Cannot open %ls: %x!\n\n",us.Buffer,(UINT)status);
		return (-1);
	}
	
	/* 2. read SystemStartOptions value */
	RtlInitUnicodeString(&us,L"SystemStartOptions");
	status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
			NULL,0,&data_size);
	if(status != STATUS_BUFFER_TOO_SMALL){
		DebugPrintEx(status,"Cannot query SystemStartOptions value size");
		winx_printf("Cannot query SystemStartOptions value size: %x!\n\n",(UINT)status);
		return (-1);
	}
	data_size += sizeof(short);
	data = winx_heap_alloc(data_size);
	if(data == NULL){
		DebugPrint("Cannot allocate %u bytes of memory for winx_windows_in_safe_mode()!",
				data_size);
		winx_printf("Cannot allocate %u bytes of memory for winx_windows_in_safe_mode()!\n\n",
				data_size);
		return (-1);
	}
	
	RtlZeroMemory(data,data_size);
	status = NtQueryValueKey(hKey,&us,KeyValuePartialInformation,
			data,data_size,&data_size2);
	if(status != STATUS_SUCCESS){
		DebugPrintEx(status,"Cannot query SystemStartOptions value");
		winx_printf("Cannot query SystemStartOptions value: %x!\n\n",(UINT)status);
		winx_heap_free(data);
		return (-1);
	}
	data_buffer = (short *)(data->Data);
	data_length = data->DataLength >> 1;
	if(data_length == 0){ /* value is empty */
		winx_heap_free(data);
		return 0;
	}
	data_buffer[data_length - 1] = 0;
	
	/* 3. search for SAFEBOOT */
	_wcsupr(data_buffer);
	if(wcsstr(data_buffer,L"SAFEBOOT")) safe_boot = 1;
	DebugPrint("%ls - %u\n\n",data_buffer,data_size);
	//winx_printf("%ls - %u\n\n",data_buffer,data_size);
	winx_heap_free(data);

	return safe_boot;
}
/** @} */
