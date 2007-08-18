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
 *  Console interface main code.
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winioctl.h>

#include "../include/misc.h"
#include "../include/ultradfg.h"
#include "../include/ultradfgver.h"

typedef LONG NTSTATUS;

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

NTSYSAPI VOID NTAPI RtlInitUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PCWSTR SourceString);

NTSTATUS NTAPI
NtLoadDriver(IN PUNICODE_STRING DriverServiceName);

NTSTATUS NTAPI
NtUnloadDriver(IN PUNICODE_STRING DriverServiceName);

HANDLE hOut;
WORD console_attr = 0x7;
HANDLE hUltraDefragDevice = INVALID_HANDLE_VALUE;
HANDLE hEvt = NULL;
OVERLAPPED ovrl;
short driver_key[] = \
	L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ultradfg";

void __cdecl print(const char *str)
{
	DWORD txd;
	WriteConsole(hOut,str,strlen(str),&txd,NULL);
}

void show_help(void)
{
	print("Usage: defrag [-a,-c,-?] driveletter:\n"
		 " -a\tanalyse only\n"
		 " -c\tcompact space\n"
		 " -?\tshow this help\n\n");
}

void display_error(char *msg)
{
	SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_INTENSITY);
	print(msg);
	SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
{
	DWORD txd;
	HANDLE hEvtStop = NULL;

	hEvtStop = CreateEvent(NULL,TRUE,TRUE,"UltraDefragStop");
	ovrl.hEvent = hEvtStop;
	ovrl.Offset = ovrl.OffsetHigh = 0;
	if(DeviceIoControl(hUltraDefragDevice,IOCTL_ULTRADEFRAG_STOP,
					NULL,0,NULL,0,&txd,&ovrl))
		WaitForSingleObject(hEvtStop,INFINITE);
	if(hEvtStop) CloseHandle(hEvtStop);
	return TRUE;
}

/* small function to exclude letters assigned by SUBST command */
BOOL is_virtual(char vol_letter)
{
	char dev_name[] = "A:";
	char target_path[512];

	dev_name[0] = vol_letter;
	QueryDosDevice(dev_name,target_path,512);
	return (strstr(target_path,"\\??\\") == target_path);
}

BOOL is_mounted(char vol_letter)
{
	char rootpath[] = "A:\\";
	ULARGE_INTEGER total, free, BytesAvailable;

	rootpath[0] = vol_letter;
	return (BOOL)GetDiskFreeSpaceEx(rootpath,&BytesAvailable,&total,&free);
}

int __cdecl wmain(int argc, short **argv)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	int n;
	int a_flag = 0,c_flag = 0,f_flag = 0;
	short *in_filter = NULL, *ex_filter = NULL;
	int length;
	HANDLE hEventIsRunning = NULL;
	char letter = 0;
	char rootdir[] = "A:\\";
	DWORD txd;
	char s[16];
	char *str;
	STATISTIC stat;
	int err_code = 0;
	ULONGLONG sizelimit = 0;

	ULTRADFG_COMMAND cmd;
	UNICODE_STRING uStr;

	HANDLE hToken;
	TOKEN_PRIVILEGES tp;

	/* display copyright */
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if(GetConsoleScreenBufferInfo(hOut,&csbi))
		console_attr = csbi.wAttributes;
	SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	print(VERSIONINTITLE " console interface\n"
	      "Copyright (c) Dmitri Arkhangelski, 2007.\n\n");
	/* only one instance of program ! */
	hEventIsRunning = OpenEvent(EVENT_MODIFY_STATE,FALSE, \
	                            "UltraDefragIsRunning");
	if(hEventIsRunning)
	{
		display_error("You can run only one instance of UltraDefrag!\n");
		err_code = 1; goto cleanup;
	}
	hEventIsRunning = CreateEvent(NULL,TRUE,TRUE,"UltraDefragIsRunning");
	/* analyse command line */
	if(argc < 2)
	{
		show_help(); goto cleanup;
	}
	for(n = 1; n < argc; n++)
	{
		if(argv[n][0] && argv[n][1] == ':')
			letter = (char)argv[n][0];
		if(argv[n][0] == '-')
		{
			switch(argv[n][1])
			{
			case 'a':
			case 'A':
				a_flag = 1;
				break;
			case 'c':
			case 'C':
				c_flag = 1;
				break;
			case 'f':
			case 'F':
				f_flag = 1;
				break;
			case '?':
			case 'h':
			case 'H':
				show_help();
				goto cleanup;
			case 's':
			case 'S':
				sizelimit = _wtoi64(argv[n] + 2);
				break;
			case 'i':
			case 'I':
				in_filter = (argv[n] + 2);
				break;
			case 'e':
			case 'E':
				ex_filter = (argv[n] + 2);
				break;
			default:
				display_error("Unknown option: ");
				///WriteConsole(hOut,str,strlen(str),&txd,NULL);
				display_error("\n");
				goto cleanup;
			}
		}
	}
	/* validate driveletter */
	if(!((letter >= 'A' && letter <= 'Z') || \
	     (letter >= 'a' && letter <= 'z')))
	{
		display_error("Incorrect or not specified drive letter\n");
		err_code = 2; goto cleanup;
	}
	SetErrorMode(SEM_FAILCRITICALERRORS);
	rootdir[0] = letter;
	switch(GetDriveType(rootdir))
	{
	case DRIVE_FIXED:
	case DRIVE_REMOVABLE:
	case DRIVE_RAMDISK:
		if(!is_virtual(letter) && is_mounted(letter))
			break; /* OK */
	default:
		SetErrorMode(0);
		display_error("Volume must be on non-cdrom local drive\n");
		err_code = 2; goto cleanup;
	}
	SetErrorMode(0);
	hEvt = CreateEvent(NULL,TRUE,TRUE,"UltraDefragIoComplete");
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,TRUE);
	/* to do our job */
	if(OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&hToken))
	{
		tp.PrivilegeCount = 1;
		tp.Privileges->Attributes = SE_PRIVILEGE_ENABLED;
		tp.Privileges->Luid.HighPart = 0;
		tp.Privileges->Luid.LowPart = 0xA; /*SE_LOAD_DRIVER_PRIVILEGE;*/
		AdjustTokenPrivileges(hToken,FALSE,&tp,0,NULL,NULL);
		CloseHandle(hToken);
	}
	RtlInitUnicodeString(&uStr,driver_key);
	NtLoadDriver(&uStr);
	hUltraDefragDevice = CreateFileW(L"\\\\.\\ultradfg",GENERIC_ALL, \
			FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING, \
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,0);
	if(hUltraDefragDevice == INVALID_HANDLE_VALUE)
	{
		display_error("Can't access ultradfg driver!\n");
		err_code = 3; goto cleanup;
	}

	length = in_filter ? ((wcslen(in_filter) + 1) << 1) : 0;
	ovrl.hEvent = hEvt;
	ovrl.Offset = ovrl.OffsetHigh = 0;
	if(DeviceIoControl(hUltraDefragDevice,IOCTL_SET_INCLUDE_FILTER,
		in_filter,length,NULL,0,&txd,&ovrl))
		WaitForSingleObject(hEvt,INFINITE);
	ovrl.hEvent = hEvt;
	ovrl.Offset = ovrl.OffsetHigh = 0;
	length = ex_filter ? ((wcslen(ex_filter) + 1) << 1) : 0;
	if(DeviceIoControl(hUltraDefragDevice,IOCTL_SET_EXCLUDE_FILTER,
		ex_filter,length,NULL,0,&txd,&ovrl))
		WaitForSingleObject(hEvt,INFINITE);

	cmd.command = a_flag ? 'a' : 'd';
	if(c_flag) cmd.command = 'c';
	cmd.letter = letter;
	cmd.sizelimit = sizelimit;

	ovrl.hEvent = hEvt;
	ovrl.Offset = ovrl.OffsetHigh = 0;
	if(!WriteFile(hUltraDefragDevice,&cmd,sizeof(ULTRADFG_COMMAND), \
		      &txd,&ovrl))
	{
		display_error("I/O error!\n");
		err_code = 4; goto cleanup;
	}
	else
	{
		WaitForSingleObject(hEvt,INFINITE);
	}

	/* display results */
	ovrl.hEvent = hEvt;
	ovrl.Offset = ovrl.OffsetHigh = 0;
	if(DeviceIoControl(hUltraDefragDevice,IOCTL_GET_STATISTIC,NULL,0, \
		&stat,sizeof(STATISTIC),&txd,&ovrl))
	{
		WaitForSingleObject(hEvt,INFINITE);
		print("\nVolume information:\n");
		str = FormatBySize(stat.total_space,s,LEFT_ALIGNED);
		print("\n  Volume size                  = ");
		print(str);
		str = FormatBySize(stat.free_space,s,LEFT_ALIGNED);
		print("\n  Free space                   = ");
		print(str);
		print("\n\n  Total number of files        = ");
		_itoa(stat.filecounter,s,10);
		print(s);
		print("\n  Number of fragmented files   = ");
		_itoa(stat.fragmfilecounter,s,10);
		print(s);
		Format2(((double)(stat.fragmcounter)/(double)(stat.filecounter)),s);
		print("\n  Fragments per file           = ");
		print(s);
		print("\n");
	}

cleanup:
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
	if(hUltraDefragDevice != INVALID_HANDLE_VALUE)
		CloseHandle(hUltraDefragDevice);
	if(hEventIsRunning) CloseHandle(hEventIsRunning);
	if(hEvt) CloseHandle(hEvt);
	NtUnloadDriver(&uStr);
	SetConsoleTextAttribute(hOut, console_attr);
	ExitProcess(err_code);
}
 