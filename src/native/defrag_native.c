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
HANDLE hStopEvent = NULL;
HANDLE hDeviceIoEvent;
UNICODE_STRING uStr;
char letter;

DWORD next_boot, every_boot;
short *letters;
ULONGLONG sizelimit = 0;

int abort_flag = 0,done_flag = 0,wait_flag = 0;
char last_op = 0;
int i = 0,j = 0,k; /* number of '-' */

extern char kb_buffer[];

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

void ReadSettings();
void CleanRegistry();
int __cdecl putch(int ch);
int __cdecl print(const char *str);
BOOL kb_open();
BOOL kb_scan();
void kb_close();
NTSTATUS WaitKbHit(DWORD msec,KEYBOARD_INPUT_DATA *pkbd);
BOOL EnablePrivilege(DWORD dwLowPartOfLUID);
void IntSleep(DWORD dwMilliseconds);

DWORD WINAPI wait_kb_proc(LPVOID unused)
{
	KEYBOARD_INPUT_DATA kbd;

	if(WaitKbHit(3000,&kbd) == STATUS_SUCCESS)
		abort_flag = 1;

	/* Exit the thread */
	//RtlExitUserThread(STATUS_SUCCESS);
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

	/* Exit the thread */
	//RtlExitUserThread(STATUS_SUCCESS);
	return 0;
}

void Cleanup()
{
	IO_STATUS_BLOCK IoStatusBlock;

	/* stop device */
	NtDeviceIoControlFile(hUltraDefragDevice,NULL,NULL,NULL,&IoStatusBlock, \
				IOCTL_ULTRADEFRAG_STOP,NULL,0,NULL,0);
	/* close handles */
	if(hUltraDefragDevice) NtClose(hUltraDefragDevice);
	if(hStopEvent) NtClose(hStopEvent);
	if(hDeviceIoEvent) NtClose(hDeviceIoEvent);
	kb_close();
	/* unload driver */
	NtUnloadDriver(&uStr);

	/* registry cleanup */
	CleanRegistry();
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

	Status = NtWriteFile(hUltraDefragDevice,hDeviceIoEvent,NULL,NULL,&IoStatusBlock, \
		&cmd,sizeof(ULTRADFG_COMMAND),&offset,0);
	if(Status == STATUS_PENDING)
	{
		Status = NtWaitForSingleObject(hDeviceIoEvent,FALSE,NULL);
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
	HANDLE hThread = NULL;
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
	if(!kb_open())
		goto continue_boot;
	RtlInitUnicodeString(&strU,L"\\UDefragStopEvent");
	InitializeObjectAttributes(&ObjectAttributes,&strU,0,NULL,NULL);
	Status = NtCreateEvent(&hStopEvent,0x1f01ff,&ObjectAttributes,1,0);
	if(Status)
	{
		print("\nERROR: Can't create \\UDefragStopEvent! Wait 5 seconds.\n");
		IntSleep(5000);
		goto continue_boot;///abort;
	}
	/* 6b. Prompt to exit */
	print("Press any key to exit...  ");
	Status = RtlCreateUserThread(NtCurrentProcess(),NULL,FALSE,
		0,0,0,wait_kb_proc,NULL,&hThread,NULL);
	if(Status)
	{
		print("\nCan't create thread: "); 
		_itoa(Status,buffer,16);
		print(buffer);
		print("\n");
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
	/* read parameters from registry */
	ReadSettings();

	/* 8a. Batch mode */
#if USE_INSTEAD_SMSS
#else
	if(!next_boot && !every_boot)
		goto cmd_loop;
	/* do job */
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
		if(!kb_scan())
		{
			print("\nERROR: Can't read \\Device\\KeyboardClass0! Wait 10 seconds.\n");
			Action = ShutdownReboot;
			goto abort;
		}
		if(strstr(kb_buffer,"shutdown"))
		{
			print("Shutdown ...");
			Action = ShutdownNoReboot;
			goto exit;
		}
		if(strstr(kb_buffer,"reboot"))
		{
			print("Reboot ...");
			goto exit;
		}
		if(strstr(kb_buffer,"batch"))
		{
			print("Prepare to batch processing ...\n");
			continue;
		}
		if(strstr(kb_buffer,"exit"))
		{
continue_boot:
			print("Good bye ...\n");
			Cleanup();
			///IntSleep(6000);
			NtTerminateProcess(NtCurrentProcess(), 0);
			return;
		}

		if(strstr(kb_buffer,"analyse")) a_flag = 1;
		else a_flag = 0;

		if(strstr(kb_buffer,"defrag")) d_flag = 1;
		else d_flag = 0;

		if(strstr(kb_buffer,"compact")) c_flag = 1;
		else c_flag = 0;

		if(a_flag || d_flag || c_flag)
		{
			pos = strchr(kb_buffer,':');
			if(!pos || pos == kb_buffer)
				continue;
			if(a_flag) command = 'a';
			else if(d_flag)	command = 'd';
			else command = 'c';
			ProcessVolume(pos[-1],command);
			continue;
		}
		if(strstr(kb_buffer,"drives"))
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
		if(strstr(kb_buffer,"help"))
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
