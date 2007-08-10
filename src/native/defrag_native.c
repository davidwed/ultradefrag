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
 *  Main source of native interface.
 */

#include "defrag_native.h"
#include "../include/ultradfgver.h"

HANDLE hUltraDefragDevice = NULL;
HANDLE hStdInput = NULL;
HANDLE hKbEvent = NULL;
HANDLE hDeviceIoEvent;
UNICODE_STRING uStr;
char letter;

#define USAGE  "Supported commands:\n" \
		"  analyse X:\n" \
		"  defrag X:\n" \
		"  compact X:\n" \
		"  batch\n" \
		"  drives\n" \
		"  exit\n" \
		"  reboot\n" \
		"  shutdown\n" \
		"  help\n"

int __cdecl putch(int ch)
{
	short str[2];
	UNICODE_STRING strU;

	str[0] = (short)ch;
	str[1] = 0;

	RtlInitUnicodeString(&strU,str);
	NtDisplayString(&strU);

	return 0;
}
int __cdecl print(const char *str)
{
	int i,len;

	len = strlen(str);
	for(i = 0; i < len; i ++)
	{
		putch(str[i]);
	}
	return 0;
}

BOOL EnablePrivilege(DWORD dwLowPartOfLUID)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;
	NTSTATUS status;
	HANDLE UserToken;

	NtOpenProcessToken((HANDLE)(size_t)0x0FFFFFFFF,0x2000000,&UserToken);

	luid.HighPart = 0x0;
	luid.LowPart = dwLowPartOfLUID;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	status = NtAdjustPrivilegesToken(UserToken,FALSE,&tp,sizeof(TOKEN_PRIVILEGES),
									(PTOKEN_PRIVILEGES)NULL,(PDWORD)NULL);
	return NT_SUCCESS(status);
}

void IntSleep(DWORD dwMilliseconds)
{
	LARGE_INTEGER Interval;

	if (dwMilliseconds != INFINITE)
	{
		/* System time units are 100 nanoseconds. */
		Interval.QuadPart = -((signed long)dwMilliseconds * 10000);
	}
	else
	{
		/* Approximately 292000 years hence */
		Interval.QuadPart = -0x7FFFFFFFFFFFFFFF;
	}
	NtDelayExecution (FALSE, &Interval);
}

NTSTATUS ConReadConsoleInput(PKEYBOARD_INPUT_DATA InputData)
{
	IO_STATUS_BLOCK Iosb;
	NTSTATUS Status;
	LARGE_INTEGER ByteOffset;

	ByteOffset.QuadPart = 0;
	Status = NtReadFile(hStdInput,hKbEvent,NULL,NULL,
			&Iosb,InputData,sizeof(KEYBOARD_INPUT_DATA),&ByteOffset,0);
	/* wait in case operation is pending */
	if(Status == STATUS_PENDING)
	{
		Status = NtWaitForSingleObject(hKbEvent,FALSE,NULL);
		if(!Status) Status = Iosb.Status;
	}
	return Status;
}

BOOL con_scanf(char *buffer)
{
	int max_chars = 60;
	int index = 0;
	KEYBOARD_INPUT_DATA kbd;
	NTSTATUS status;
	KBD_RECORD kbd_rec;
	char ch;

	while(index <= max_chars)
	{
		status = ConReadConsoleInput(&kbd);
		if(!status)
		{
			IntTranslateKey(&kbd, &kbd_rec);
			if(!kbd_rec.bKeyDown) continue;
			ch = kbd_rec.AsciiChar;
			if(!ch) continue; /* for control keys */
			if(ch == 13)
			{
				putch('\r');
				putch('\n');
				break;
			}
			putch(ch);
			buffer[index] = ch;
			index ++;
		}
		else return FALSE;
	}
	buffer[index] = 0;
	return TRUE;
}

void Cleanup()
{
	IO_STATUS_BLOCK IoStatusBlock;

	NtDeviceIoControlFile(hUltraDefragDevice,NULL,NULL,NULL,&IoStatusBlock, \
				IOCTL_ULTRADEFRAG_STOP,NULL,0,NULL,0);
	if(hUltraDefragDevice) NtClose(hUltraDefragDevice);
	if(hKbEvent) NtClose(hKbEvent);
	if(hDeviceIoEvent) NtClose(hDeviceIoEvent);
	if(hStdInput) NtClose(hStdInput);
	NtUnloadDriver(&uStr);
}

void __stdcall NtProcessStartup(PPEB Peb)
{
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	UNICODE_STRING strU;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	char buf[64];
	DWORD Action = ShutdownReboot;
	char *pos;
	unsigned short path[] = L"\\??\\A:\\";
	HANDLE hFile;
	FILE_FS_DEVICE_INFORMATION FileFsDevice;
	int a_flag, d_flag, c_flag;
	ULTRADFG_COMMAND cmd;
	STATISTIC stat;
	char s[11];
	char *str;
	LARGE_INTEGER offset;
	short driver_key[] = \
	  L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ultradfg";

	/* 1. Normalize and get the Process Parameters */
	ProcessParameters = RtlNormalizeProcessParams(Peb->ProcessParameters);

	/* 2. Breakpoint if we were requested to do so */
	if (ProcessParameters->DebugFlags) DbgBreakPoint();

	/* 3. Initialization */
#if USE_INSTEAD_SMSS
	NtInitializeRegistry(FALSE);
#endif
	offset.QuadPart = 0;
	/* 4. Display Copyright */
	RtlInitUnicodeString(&strU,
		VERSIONINTITLE_U L" native interface\n"
		L"Copyright (c) Dmitri Arkhangelski, 2007.\n\n"
		L"UltraDefrag comes with ABSOLUTELY NO WARRANTY.\n\n");
	NtDisplayString(&strU);
	/* 5. Enable neccessary privileges */
	if(!EnablePrivilege(SE_SHUTDOWN_PRIVILEGE))
		print("Can't enable SE_SHUTDOWN\n");
	if(!EnablePrivilege(SE_MANAGE_VOLUME_PRIVILEGE))
		print("Can't enable SE_MANAGE_VOLUME\n");
	if(!EnablePrivilege(SE_LOAD_DRIVER_PRIVILEGE))
		print("Can't enable SE_LOAD_DRIVER\n");
	/* 6. Open the keyboard */
	RtlInitUnicodeString(&strU,L"\\Device\\KeyboardClass0");
	InitializeObjectAttributes(&ObjectAttributes,&strU,0,NULL,NULL);
	Status = NtCreateFile(&hStdInput,0x80100080,
			    &ObjectAttributes,&IoStatusBlock,NULL,0x80,
			    0,1,1,NULL,0);
	if(Status)
	{
		print("\nERROR: No keyboard available! Wait 10 seconds.\n");
		IntSleep(10000);
		NtShutdownSystem(ShutdownReboot);
	}
	RtlInitUnicodeString(&strU,L"\\KbEvent");
	InitializeObjectAttributes(&ObjectAttributes,&strU,0,NULL,NULL);
	Status = NtCreateEvent(&hKbEvent,0x1f01ff,&ObjectAttributes,1,0);
	if(Status)
	{
		print("\nERROR: Can't create \\KbEvent! Wait 10 seconds.\n");
		goto abort;
	}
	/* 7. Open the ultradfg device */
	RtlInitUnicodeString(&uStr,driver_key);
	NtLoadDriver(&uStr);
	RtlInitUnicodeString(&strU,L"\\Device\\UltraDefrag");
	InitializeObjectAttributes(&ObjectAttributes,&strU,0,NULL,NULL);
	Status = NtCreateFile(&hUltraDefragDevice,FILE_GENERIC_READ | FILE_GENERIC_WRITE,
			    &ObjectAttributes,&IoStatusBlock,NULL,0,
			    FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,NULL,0);
	if(Status)
	{
		print("\nERROR: Can't access ULTRADFG driver! Wait 10 seconds.\n");
		goto abort;
	}
	RtlInitUnicodeString(&strU,L"\\UltraDefragIoEvent");
	InitializeObjectAttributes(&ObjectAttributes,&strU,0,NULL,NULL);
	Status = NtCreateEvent(&hDeviceIoEvent,0x1f01ff,&ObjectAttributes,1,0);
	if(Status)
	{
		print("\nERROR: Can't create \\UltraDefragIoEvent! Wait 10 seconds.\n");
		goto abort;
	}

	/* 8. Command Loop */
	while(1)
	{
		print("# ");
		if(!con_scanf(buf))
		{
			print("\nERROR: Can't read \\Device\\KeyboardClass0! Wait 10 seconds.\n");
			Action = ShutdownReboot;
			goto abort;
		}
		if(strstr(buf,"shutdown"))
		{
			print("Shutdown ...");
			Action = ShutdownNoReboot;
			goto exit;
		}
		if(strstr(buf,"reboot"))
		{
			print("Reboot ...");
			goto exit;
		}
		if(strstr(buf,"batch"))
		{
			print("Prepare to batch processing ...\n");
			continue;
		}
		if(strstr(buf,"exit"))
		{
			print("Good bye ...\n");
			Cleanup();
			NtTerminateProcess(NtCurrentProcess(), 0);
			return;
		}

		if(strstr(buf,"analyse")) a_flag = 1;
		else a_flag = 0;

		if(strstr(buf,"defrag")) d_flag = 1;
		else d_flag = 0;

		if(strstr(buf,"compact")) c_flag = 1;
		else c_flag = 0;

		if(a_flag || d_flag || c_flag)
		{
			pos = strchr(buf,':');
			if(!pos || pos == buf)
				continue;
			if(a_flag)	cmd.command = 'a';
			else if(d_flag)	cmd.command = 'd';
			else cmd.command = 'c';
			cmd.letter = pos[-1];
			Status = NtWriteFile(hUltraDefragDevice,hKbEvent,NULL,NULL,&IoStatusBlock, \
				&cmd,sizeof(ULTRADFG_COMMAND),&offset,0);
			if(Status == STATUS_PENDING)
			{
				Status = NtWaitForSingleObject(hKbEvent,FALSE,NULL);
				if(!Status)
					Status = IoStatusBlock.Status;
			}
			if(Status)
			{
				print("\nIncorrect drive letter or internal driver error!\n");
				continue;
			}
			Status = NtDeviceIoControlFile(hUltraDefragDevice,NULL,NULL,NULL,&IoStatusBlock, \
				IOCTL_GET_STATISTIC,NULL,0,&stat,sizeof(STATISTIC));
			if(Status)
			{
				print("\nNo statistic available.\n");
			}
			else
			{
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
				print("\r\n");
			}
			continue;
		}
		if(strstr(buf,"drives"))
		{
			print("Available drive letters:   ");
			for(letter = 'A'; letter <= 'Z'; letter ++)
			{
				path[4] = (unsigned short)letter;
				RtlInitUnicodeString(&strU,path);
				InitializeObjectAttributes(&ObjectAttributes,&strU,
							   FILE_READ_ATTRIBUTES,NULL,NULL);

				Status = NtCreateFile (&hFile,FILE_GENERIC_READ,
							&ObjectAttributes,&IoStatusBlock,NULL,0,
							FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,0,
							NULL,0);

				if(Status) continue;

				Status = NtQueryVolumeInformationFile(hFile,&IoStatusBlock,&FileFsDevice,
						  sizeof(FILE_FS_DEVICE_INFORMATION),FileFsDeviceInformation);
				NtClose(hFile);
				if(Status) continue;
				if((FileFsDevice.DeviceType != FILE_DEVICE_DISK) && 
					(FileFsDevice.DeviceType != FILE_DEVICE_DISK_FILE_SYSTEM))
					continue;
				if((FileFsDevice.Characteristics & FILE_REMOTE_DEVICE) ||
					(FileFsDevice.Characteristics & FILE_REMOVABLE_MEDIA))
					continue;
				putch(letter);
				print("   ");
			}
			print("\r\n");
			continue;
		}
		if(strstr(buf,"help"))
		{
			print(USAGE);
			continue;
		}
		print("Unknown command!\n");
	}
abort:
	IntSleep(10000);
exit:
	Cleanup();
	NtShutdownSystem(Action);
	/* 8. We're done here */
	NtTerminateProcess(NtCurrentProcess(), 0);
}
