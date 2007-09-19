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
HANDLE hStopEvent = NULL;
HANDLE hDeviceIoEvent;
UNICODE_STRING uStr;
char letter;

DWORD next_boot, every_boot;
ULONGLONG sizelimit = 0;

int abort_flag = 0,done_flag = 0,wait_flag = 0;
char last_op = 0;
int i = 0,j = 0,k; /* number of '-' */

short ultradefrag_key[] = L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\UltraDefrag";

char buffer[65536]; /* instead malloc calls */

char user_mode_buffer[65536]; /* for nt 4.0 support */

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
	//char b[32];

	NtOpenProcessToken(NtCurrentProcess(),0x2000000,&UserToken);

	luid.HighPart = 0x0;
	luid.LowPart = dwLowPartOfLUID;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	status = NtAdjustPrivilegesToken(UserToken,FALSE,&tp,sizeof(TOKEN_PRIVILEGES),
									(PTOKEN_PRIVILEGES)NULL,(PDWORD)NULL);
	/*if(status)
	{
		_itoa(status,b,16);
		print(b);
		print("\n");
	}*/
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

NTSTATUS WaitKbHit(DWORD msec,KEYBOARD_INPUT_DATA *pkbd)
{
	IO_STATUS_BLOCK Iosb;
	NTSTATUS Status;
	LARGE_INTEGER ByteOffset;
	LARGE_INTEGER interval;

	ByteOffset.QuadPart = 0;
	if (msec != INFINITE)
		interval.QuadPart = -((signed long)msec * 10000);
	else
		interval.QuadPart = -0x7FFFFFFFFFFFFFFF;
	Status = NtReadFile(hStdInput,hKbEvent,NULL,NULL,
			&Iosb,pkbd,sizeof(KEYBOARD_INPUT_DATA),&ByteOffset,0);
	/* wait in case operation is pending */
	if(Status == STATUS_PENDING)
	{
		Status = NtWaitForSingleObject(hKbEvent,FALSE,&interval);
		if(!Status) Status = Iosb.Status;
	}
	return Status;
}

DWORD WINAPI wait_kb_proc(LPVOID unused)
{
	KEYBOARD_INPUT_DATA kbd;

	if(WaitKbHit(3000,&kbd) == STATUS_SUCCESS)
		abort_flag = 1;
	return 0;
}

void UpdateProgress()
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;
	STATISTIC stat;
	double percentage;

	Status = NtDeviceIoControlFile(hUltraDefragDevice,hStopEvent,NULL,NULL,&IoStatusBlock, \
		IOCTL_GET_STATISTIC,NULL,0,&stat,sizeof(STATISTIC));
	if(Status == STATUS_PENDING)
	{
		Status = NtWaitForSingleObject(hStopEvent,FALSE,NULL);
		if(!Status)	Status = IoStatusBlock.Status;
	}
	if(Status == STATUS_SUCCESS)
	{
		switch(stat.current_operation)
		{
		case 'A':
			percentage = (double)(LONGLONG)stat.processed_clusters *
					(double)(LONGLONG)stat.bytes_per_cluster / 
					(double)(LONGLONG)stat.total_space;
			break;
		case 'D':
			if(stat.clusters_to_move_initial == 0)
				percentage = 1.00;
			else
				percentage = 1.00 - (double)(LONGLONG)stat.clusters_to_move / 
						(double)(LONGLONG)stat.clusters_to_move_initial;
			break;
		case 'C':
			if(stat.clusters_to_compact_initial == 0)
				percentage = 1.00;
			else
				percentage = 1.00 - (double)(LONGLONG)stat.clusters_to_compact / 
						(double)(LONGLONG)stat.clusters_to_compact_initial;
		}
		percentage *= 100.00;
		if(stat.current_operation != last_op)
		{
			if(last_op)
				for(k = i; k < 50; k++)
					putch('-'); /* 100 % of previous operation */
			i = 0; /* new operation */
			print("\n");
			putch(stat.current_operation);
			print(": ");
			last_op = stat.current_operation;
		}
		j = (int)percentage / 2;
		for(k = i; k < j; k++)
		{
			putch('-');
		}
		i = j;
	}
}

DWORD WINAPI wait_break_proc(LPVOID unused)
{
	KEYBOARD_INPUT_DATA kbd;
	NTSTATUS status;
	IO_STATUS_BLOCK IoStatusBlock;
	///char b[32];

	wait_flag = 1;
	i = j = 0;
	last_op = 0;
	do
	{
		status = WaitKbHit(100,&kbd);
		if(!status)
		{
			if((kbd.Flags & KEY_E1) && (kbd.MakeCode == 0x1d))
			{
				status = NtDeviceIoControlFile(hUltraDefragDevice,hStopEvent,NULL,NULL,&IoStatusBlock, \
					IOCTL_ULTRADEFRAG_STOP,NULL,0,NULL,0);
				if(status == STATUS_PENDING)
					status = NtWaitForSingleObject(hStopEvent,FALSE,NULL);
				abort_flag = 1;
			}
		}
		UpdateProgress();
	} while(!done_flag);
	if(!abort_flag) /* set progress to 100 % */
	{
		///_itoa(i,b,10);
		///print(b);
		///print("\n");
		for(k = i; k < 50; k++)
			putch('-');
	}
	///UpdateProgress();

	wait_flag = 0;
	return 0;
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

DWORD ReadRegDword(short *value_name)
{
	NTSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING __uStr;
	HANDLE hKey;
	KEY_VALUE_PARTIAL_INFORMATION pInfo[2]; /* second for dword value placing */
	DWORD size = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD);
	DWORD value = 0;
	char st[32];

	RtlInitUnicodeString(&__uStr,ultradefrag_key);
	/* OBJ_CASE_INSENSITIVE must be specified on NT 4.0 !!! */
	InitializeObjectAttributes(&ObjectAttributes,&__uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
	Status = NtOpenKey(&hKey,KEY_QUERY_VALUE,&ObjectAttributes);
	if(!NT_SUCCESS(Status))
	{
		_itoa(Status,st,16);
		print("\nCan't open registry key! ");
		print(st);
		print("\n");
		return 0;
	}
	RtlInitUnicodeString(&__uStr,value_name);
	Status = NtQueryValueKey(hKey,&__uStr,KeyValuePartialInformation,
		&pInfo[0],size,&size);
	if(NT_SUCCESS(Status))
	{
		if(pInfo[0].Type == REG_DWORD && pInfo[0].DataLength == sizeof(DWORD))
			RtlCopyMemory(&value,pInfo[0].Data,sizeof(DWORD));
	}
	NtClose(hKey);
	return value;
}

void WriteRegDword(short *value_name,DWORD value)
{
	NTSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING __uStr;
	HANDLE hKey;
	char st[32];

	RtlInitUnicodeString(&__uStr,ultradefrag_key);
	InitializeObjectAttributes(&ObjectAttributes,&__uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
	Status = NtOpenKey(&hKey,KEY_SET_VALUE,&ObjectAttributes);
	if(!NT_SUCCESS(Status))
	{
		_itoa(Status,st,16);
		print("\nCan't open registry key! ");
		print(st);
		print("\n");
		return;
	}
	RtlInitUnicodeString(&__uStr,value_name);
	NtSetValueKey(hKey,&__uStr,0,REG_DWORD,&value,sizeof(DWORD));
	NtClose(hKey);
}

void SetFilter(short *value_name,DWORD ioctl_code)
{
	NTSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING __uStr;
	HANDLE hKey;
	KEY_VALUE_PARTIAL_INFORMATION *pInfo = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
	DWORD size = 65536;
	char st[32];
	IO_STATUS_BLOCK IoStatusBlock;

	RtlInitUnicodeString(&__uStr,ultradefrag_key);
	InitializeObjectAttributes(&ObjectAttributes,&__uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
	Status = NtOpenKey(&hKey,KEY_QUERY_VALUE,&ObjectAttributes);
	if(!NT_SUCCESS(Status))
	{
		_itoa(Status,st,16);
		print("\nCan't open registry key! ");
		print(st);
		print("\n");
		return;
	}
	RtlInitUnicodeString(&__uStr,value_name);
	Status = NtQueryValueKey(hKey,&__uStr,KeyValuePartialInformation,
		pInfo,size,&size);
	if(NT_SUCCESS(Status))
	{
		Status = NtDeviceIoControlFile(hUltraDefragDevice,NULL,NULL,NULL,&IoStatusBlock, \
			ioctl_code,&pInfo->Data,pInfo->DataLength,NULL,0);
	}
	NtClose(hKey);
	return;
}

short *ReadLetters()
{
	NTSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING __uStr;
	HANDLE hKey;
	KEY_VALUE_PARTIAL_INFORMATION *pInfo = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
	DWORD size = 65536;
	char st[32];

	RtlInitUnicodeString(&__uStr,ultradefrag_key);
	InitializeObjectAttributes(&ObjectAttributes,&__uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
	Status = NtOpenKey(&hKey,KEY_QUERY_VALUE,&ObjectAttributes);
	if(!NT_SUCCESS(Status))
	{
		_itoa(Status,st,16);
		print("\nCan't open registry key! ");
		print(st);
		print("\n");
		return NULL;
	}
	RtlInitUnicodeString(&__uStr,L"scheduled letters");
	Status = NtQueryValueKey(hKey,&__uStr,KeyValuePartialInformation,
		pInfo,size,&size);
	if(NT_SUCCESS(Status))
	{
		NtClose(hKey);
		return (short *)&pInfo->Data;
	}
	NtClose(hKey);
	return NULL;
}

void Cleanup()
{
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING __uStr;
	HANDLE hKey;
	DWORD _len,length,len2;
	char *boot_exec;
	char new_boot_exec[512] = "";
	KEY_VALUE_PARTIAL_INFORMATION *pInfo;
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS Status;

	/* stop device */
	NtDeviceIoControlFile(hUltraDefragDevice,NULL,NULL,NULL,&IoStatusBlock, \
				IOCTL_ULTRADEFRAG_STOP,NULL,0,NULL,0);
	/* close handles */
	if(hUltraDefragDevice) NtClose(hUltraDefragDevice);
	if(hKbEvent) NtClose(hKbEvent);
	if(hStopEvent) NtClose(hStopEvent);
	if(hDeviceIoEvent) NtClose(hDeviceIoEvent);
	if(hStdInput) NtClose(hStdInput);
	/* unload driver */
	NtUnloadDriver(&uStr);

	/* registry cleanup */
	WriteRegDword(L"next boot",0);
	if(!every_boot)
	{
		///print("preparing...\n");
		/* remove native program name from BootExecute registry parameter */
		RtlInitUnicodeString(&__uStr,
			L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager");
		InitializeObjectAttributes(&ObjectAttributes,&__uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
		Status = NtOpenKey(&hKey,KEY_QUERY_VALUE | KEY_SET_VALUE,&ObjectAttributes);
		if(NT_SUCCESS(Status))
		{
			///print("opened\n");
			_len = 510;
			pInfo = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
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

void ProcessVolume(char letter,char command)
{
	ULTRADFG_COMMAND cmd;
	STATISTIC stat;
	char s[16];
	char *str;
	LARGE_INTEGER offset;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	HANDLE hThread;

	print("\nPreparing to ");
	switch(command)
	{
	case 'a':
	case 'A':
		print("analyse ");
		break;
	case 'd':
	case 'D':
		print("defragment ");
		break;
	case 'c':
	case 'C':
		print("compact ");
		break;
	}
	putch(letter);
	print(": ...\n");

	done_flag = 0;
	Status = RtlCreateUserThread(NtCurrentProcess(),NULL,FALSE,
		0,0,0,wait_break_proc,NULL,&hThread,NULL);
	if(Status)
	{
		print("\nCan't create thread: "); 
		_itoa(Status,buffer,16);
		print(buffer);
	}

	cmd.command = command;
	cmd.letter = letter;
	cmd.sizelimit = sizelimit;
	cmd.mode = __UserMode;///__KernelMode;
	offset.QuadPart = 0;

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
		done_flag = 1;
		while(wait_flag)
			IntSleep(10);
		return;
	}
	done_flag = 1;
	while(wait_flag)
		IntSleep(10);
	Status = NtDeviceIoControlFile(hUltraDefragDevice,NULL,NULL,NULL,&IoStatusBlock, \
		IOCTL_GET_STATISTIC,NULL,0,&stat,sizeof(STATISTIC));
	if(Status)
	{
		print("\nNo statistic available.\n");
	}
	else
	{
		print("\nVolume information:");
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
		print("\r\n\r\n");
	}
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
	short driver_key[] = \
	  L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ultradfg";
	char command;
	LARGE_INTEGER offset;
	short *letters;
	HANDLE hThread = NULL;
	DWORD dbg_level = 0;
	DWORD i,length;

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
	//if(nt_4)
	//	print("\n\n");
	RtlInitUnicodeString(&strU,
		VERSIONINTITLE_U L" native interface\n"
		L"Copyright (c) Dmitri Arkhangelski, 2007.\n\n"
		L"UltraDefrag comes with ABSOLUTELY NO WARRANTY.\n\n"
		L"If something is wrong, hit F8 on startup\n"
		L"and select 'Last Known Good Configuration'.\n"
		L"Use Break key to abort defragmentation.\n\n");
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
		print("\nERROR: No keyboard available! Wait 5 seconds.\n");
		IntSleep(5000);
		goto continue_boot;
		///NtShutdownSystem(ShutdownReboot);
	}
	RtlInitUnicodeString(&strU,L"\\KbEvent");
	InitializeObjectAttributes(&ObjectAttributes,&strU,0,NULL,NULL);
	Status = NtCreateEvent(&hKbEvent,0x1f01ff,&ObjectAttributes,1,0);
	if(Status)
	{
		print("\nERROR: Can't create \\KbEvent! Wait 5 seconds.\n");
		IntSleep(5000);
		goto continue_boot;///abort;
	}
	RtlInitUnicodeString(&strU,L"\\UDefragStopEvent");
	InitializeObjectAttributes(&ObjectAttributes,&strU,0,NULL,NULL);
	Status = NtCreateEvent(&hStopEvent,0x1f01ff,&ObjectAttributes,1,0);
	if(Status)
	{
		print("\nERROR: Can't create \\UDefragStopEvent! Wait 5 seconds.\n");
		IntSleep(5000);
		goto continue_boot;///abort;
	}
	/* read parameters from registry */
	next_boot = ReadRegDword(L"next boot");
	every_boot = ReadRegDword(L"every boot");
	/* 6b. Prompt to exit */
	RtlInitUnicodeString(&uStr,L"Press any key to exit...  ");
	NtDisplayString(&uStr);
	Status = RtlCreateUserThread(NtCurrentProcess(),NULL,FALSE,
		0,0,0,wait_kb_proc,NULL,&hThread,NULL);
	if(Status)
	{
		print("\nCan't create thread: "); 
		_itoa(Status,buffer,16);
		print(buffer);
	}
	for(i = 0; i < 3; i++)
	{
		IntSleep(1000);
		if(abort_flag)
		{
			print("\n\n");
			goto continue_boot;
		}
		//putch('\b');
		putch('0' + 3 - i);
		putch(' ');
	}
	print("\n\n");
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
		print("\nERROR: Can't access ULTRADFG driver! Wait 5 seconds.\n");
		IntSleep(5000);
		goto continue_boot;///abort;
	}
	RtlInitUnicodeString(&strU,L"\\UltraDefragIoEvent");
	InitializeObjectAttributes(&ObjectAttributes,&strU,0,NULL,NULL);
	Status = NtCreateEvent(&hDeviceIoEvent,0x1f01ff,&ObjectAttributes,1,0);
	if(Status)
	{
		print("\nERROR: Can't create \\UltraDefragIoEvent! Wait 5 seconds.\n");
		IntSleep(5000);
		goto continue_boot;///abort;
	}
	/* nt 4.0 specific code */
	Status = NtDeviceIoControlFile(hUltraDefragDevice,hDeviceIoEvent,NULL,NULL,&IoStatusBlock, \
		IOCTL_SET_USER_MODE_BUFFER,user_mode_buffer,0,NULL,0);
	if(Status == STATUS_PENDING)
	{
		Status = NtWaitForSingleObject(hDeviceIoEvent,FALSE,NULL);
		if(!Status)	Status = IoStatusBlock.Status;
	}
	if(Status != STATUS_SUCCESS)
	{
		print("\nERROR: Can't setup user mode buffer! Wait 5 seconds.\n");
		IntSleep(5000);
		goto continue_boot;///abort;
	}

	/* 8a. Batch mode */
	dbg_level = ReadRegDword(L"dbgprint level");
	NtDeviceIoControlFile(hUltraDefragDevice,NULL,NULL,NULL,&IoStatusBlock, \
		IOCTL_SET_DBGPRINT_LEVEL,&dbg_level,sizeof(DWORD),NULL,0);
#if USE_INSTEAD_SMSS
#else
	if(!next_boot && !every_boot)
		goto cmd_loop;
	/* set filters */
	SetFilter(L"boot time include filter",IOCTL_SET_INCLUDE_FILTER);
	SetFilter(L"boot time exclude filter",IOCTL_SET_EXCLUDE_FILTER);
	/* do job */
	letters = ReadLetters();
	if(!letters)
	{
		print("\nNo letters specified!\n");
		goto continue_boot;
	}
	length = wcslen(letters);
	for(i = 0; i < length; i++)
	{
		ProcessVolume((char)(letters[i]),'d');
		if(abort_flag) goto clear_reg;
	}
	/* clear registry */
clear_reg: /* clear reg entries code moved to cleanup proc */
	goto continue_boot;
#endif
	/* 8b. Command Loop */
cmd_loop:
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
continue_boot:
			print("Good bye ...\n");
			Cleanup();
			///IntSleep(6000);
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
			if(a_flag) command = 'a';
			else if(d_flag)	command = 'd';
			else command = 'c';
			ProcessVolume(pos[-1],command);
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
