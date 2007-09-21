/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
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
 *  Registry functions for native interface.
 */

#include "defrag_native.h"
#include "../include/ultradfgver.h"

extern HANDLE hUltraDefragDevice;
extern DWORD next_boot, every_boot;
extern short *letters;

short smss_key[] = L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager";
short ultradefrag_key[] = L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\UltraDefrag";
char reg_buffer[65536]; /* instead malloc calls */

NTSTATUS OpenKey(short *key_name,PHANDLE phKey);
DWORD ReadRegDword(HANDLE hKey,short *value_name);
void WriteRegDword(HANDLE hKey,short *value_name,DWORD value);
void SetFilter(HANDLE hKey,short *value_name,DWORD ioctl_code);
short *ReadLetters(HANDLE hKey);
int __cdecl print(const char *str);

/* read settings from registry and send to driver special commands */
void ReadSettings()
{
	HANDLE hKey;
	IO_STATUS_BLOCK IoStatusBlock;
	DWORD dbg_level;

	/* open the UltraDefrag key */
	if(OpenKey(ultradefrag_key,&hKey) != STATUS_SUCCESS)
		return;
	/* read parameters */
	next_boot = ReadRegDword(hKey,L"next boot");
	every_boot = ReadRegDword(hKey,L"every boot");
	dbg_level = ReadRegDword(hKey,L"dbgprint level");
	NtDeviceIoControlFile(hUltraDefragDevice,NULL,NULL,NULL,&IoStatusBlock, \
		IOCTL_SET_DBGPRINT_LEVEL,&dbg_level,sizeof(DWORD),NULL,0);
	SetFilter(hKey,L"boot time include filter",IOCTL_SET_INCLUDE_FILTER);
	SetFilter(hKey,L"boot time exclude filter",IOCTL_SET_EXCLUDE_FILTER);
	letters = ReadLetters(hKey);
	/* close key */
	NtClose(hKey);
}

NTSTATUS OpenKey(short *key_name,PHANDLE phKey)
{
	NTSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING __uStr;
	char st[32];

	RtlInitUnicodeString(&__uStr,key_name);
	/* OBJ_CASE_INSENSITIVE must be specified on NT 4.0 !!! */
	InitializeObjectAttributes(&ObjectAttributes,&__uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
	Status = NtOpenKey(phKey,KEY_QUERY_VALUE | KEY_SET_VALUE,&ObjectAttributes);
	if(!NT_SUCCESS(Status))
	{
		_itoa(Status,st,16);
		print("\nCan't open registry key! ");
		print(st);
		print("\n");
	}
	return Status;
}

DWORD ReadRegDword(HANDLE hKey,short *value_name)
{
	NTSTATUS Status;
	UNICODE_STRING __uStr;
	KEY_VALUE_PARTIAL_INFORMATION *pInfo = (KEY_VALUE_PARTIAL_INFORMATION *)reg_buffer;
	DWORD size = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD);
	DWORD value = 0;

	RtlInitUnicodeString(&__uStr,value_name);
	Status = NtQueryValueKey(hKey,&__uStr,KeyValuePartialInformation,
		pInfo,size,&size);
	if(NT_SUCCESS(Status))
	{
		if(pInfo->Type == REG_DWORD && pInfo->DataLength == sizeof(DWORD))
			RtlCopyMemory(&value,pInfo->Data,sizeof(DWORD));
	}
	return value;
}

void WriteRegDword(HANDLE hKey,short *value_name,DWORD value)
{
	UNICODE_STRING __uStr;

	RtlInitUnicodeString(&__uStr,value_name);
	NtSetValueKey(hKey,&__uStr,0,REG_DWORD,&value,sizeof(DWORD));
}

void SetFilter(HANDLE hKey,short *value_name,DWORD ioctl_code)
{
	NTSTATUS Status;
	UNICODE_STRING __uStr;
	KEY_VALUE_PARTIAL_INFORMATION *pInfo = (KEY_VALUE_PARTIAL_INFORMATION *)reg_buffer;
	DWORD size = 65536;
	IO_STATUS_BLOCK IoStatusBlock;

	RtlInitUnicodeString(&__uStr,value_name);
	Status = NtQueryValueKey(hKey,&__uStr,KeyValuePartialInformation,
		pInfo,size,&size);
	if(NT_SUCCESS(Status))
	{
		Status = NtDeviceIoControlFile(hUltraDefragDevice,NULL,NULL,NULL,&IoStatusBlock, \
			ioctl_code,&pInfo->Data,pInfo->DataLength,NULL,0);
	}
	return;
}

short *ReadLetters(HANDLE hKey)
{
	NTSTATUS Status;
	UNICODE_STRING __uStr;
	KEY_VALUE_PARTIAL_INFORMATION *pInfo = (KEY_VALUE_PARTIAL_INFORMATION *)reg_buffer;
	DWORD size = 65536;

	RtlInitUnicodeString(&__uStr,L"scheduled letters");
	Status = NtQueryValueKey(hKey,&__uStr,KeyValuePartialInformation,
		pInfo,size,&size);
	if(NT_SUCCESS(Status))
		return (short *)&pInfo->Data;
	return NULL;
}

void CleanRegistry()
{
	UNICODE_STRING __uStr;
	HANDLE hKey;
	DWORD _len,length,len2;
	char *boot_exec;
	char new_boot_exec[512] = "";
	KEY_VALUE_PARTIAL_INFORMATION *pInfo;
	NTSTATUS Status;

	if(OpenKey(ultradefrag_key,&hKey) == STATUS_SUCCESS)
	{
		WriteRegDword(hKey,L"next boot",0);
		NtClose(hKey);
	}
	if(!every_boot)
	{
		///print("preparing...\n");
		/* remove native program name from BootExecute registry parameter */
		if(OpenKey(smss_key,&hKey) == STATUS_SUCCESS)
		{
			///print("opened\n");
			_len = 510;
			pInfo = (KEY_VALUE_PARTIAL_INFORMATION *)reg_buffer;
			RtlInitUnicodeString(&__uStr,L"BootExecute");
			Status = NtQueryValueKey(hKey,&__uStr,KeyValuePartialInformation,
				pInfo,_len,&_len);
			if(NT_SUCCESS(Status) && pInfo->Type == REG_MULTI_SZ)
			{
				///print("query ok\n");
				boot_exec = pInfo->Data;
				_len = pInfo->DataLength;
				len2 = 0;
				memset((void *)new_boot_exec,0,512);
				for(length = 0;length < _len - 2;)
				{
					if(wcscmp((short *)(boot_exec + length),L"defrag_native"))
					{ /* if strings are not equal */
						///print(boot_exec + length);
						///print("\n");
						wcscpy((short *)(new_boot_exec + len2),(short *)(boot_exec + length));
						len2 += (wcslen((short *)(boot_exec + length)) + 1) << 1;
					}
					length += (wcslen((short *)(boot_exec + length)) + 1) << 1;
				}
				new_boot_exec[len2] = new_boot_exec[len2 + 1] = 0;
				RtlInitUnicodeString(&__uStr,L"BootExecute");
				NtSetValueKey(hKey,&__uStr,0,REG_MULTI_SZ,new_boot_exec,len2 + 2);
			}
			NtClose(hKey);
		}
	}
}