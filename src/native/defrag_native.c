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
#include <stdio.h>

#include "defrag_native.h"
#include "../include/ultradfgver.h"

HANDLE hUltraDefragDevice = NULL;
//HANDLE hStopEvent = NULL;
HANDLE hDeviceIoEvent = NULL;
UNICODE_STRING uStr;
char letter;

ud_settings *settings;

int abort_flag = 0,done_flag = 0,wait_flag = 0;
char last_op = 0;
int i = 0,j = 0,k; /* number of '-' */

extern char kb_buffer[];

char buffer[65536]; /* instead malloc calls */

char msg[2048];

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

int __cdecl putch(int ch);
int __cdecl print(const char *str);
BOOL kb_open();
BOOL kb_scan();
void kb_close();
NTSTATUS WaitKbHit(DWORD msec,KEYBOARD_INPUT_DATA *pkbd);
void IntSleep(DWORD dwMilliseconds);

/*DWORD WINAPI wait_kb_proc(LPVOID unused)
{
	KEYBOARD_INPUT_DATA kbd;

	if(WaitKbHit(3000,&kbd) == STATUS_SUCCESS)
		abort_flag = 1;

	// Exit the thread
	RtlExitUserThread(STATUS_SUCCESS);
	return 0;
}*/

void UpdateProgress()
{
	STATISTIC stat;
	double percentage;

	if(!udefrag_get_progress(&stat,&percentage))
	{
		if(stat.current_operation != last_op)
		{
			if(last_op)
			{
				for(k = i; k < 50; k++)
					putch('-'); /* 100 % of previous operation */
			}
			else
			{
				if(stat.current_operation != 'A')
				{
					print("\nA: ");
					for(k = 0; k < 50; k++)
						putch('-');
				}
			}
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
//	IO_STATUS_BLOCK IoStatusBlock;
	char *err_msg;

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
				//status = NtDeviceIoControlFile(hUltraDefragDevice,hStopEvent,NULL,NULL,&IoStatusBlock, \
				//	IOCTL_ULTRADEFRAG_STOP,NULL,0,NULL,0);
				//if(status == STATUS_PENDING)
				//	status = NtWaitForSingleObject(hStopEvent,FALSE,NULL);
				err_msg = udefrag_stop();
				if(err_msg)
				{
					sprintf(msg,"\nERROR: %s Wait 5 seconds.\n",err_msg);
					print(msg);
				}
				abort_flag = 1;
			}
		}
		UpdateProgress();
	} while(!done_flag);
	if(!abort_flag) /* set progress to 100 % */
	{
		for(k = i; k < 50; k++)
			putch('-');
	}
	///UpdateProgress();

	wait_flag = 0;

	/* Exit the thread */
	/* On NT 4.0 and W2K we should do nothing, on XP SP1 both variants are acceptable,
	   on XP x64 RtlExitUserThread() MUST be called. */
#ifndef NT4_TARGET
	RtlExitUserThread(STATUS_SUCCESS);
#endif
	return 0;
}

void Cleanup()
{
//	IO_STATUS_BLOCK IoStatusBlock;
	char *err_msg;

//print("before stop\n");
	/* stop device */
	err_msg = udefrag_stop();
	if(err_msg)
	{
		sprintf(msg,"\nERROR: %s Wait 5 seconds.\n",err_msg);
		print(msg);
	}
//	NtDeviceIoControlFile(hUltraDefragDevice,NULL,NULL,NULL,&IoStatusBlock, \
//				IOCTL_ULTRADEFRAG_STOP,NULL,0,NULL,0);
//print("after stop\n");
	/* close handles */
//	if(hStopEvent) NtClose(hStopEvent);
	if(hDeviceIoEvent) NtClose(hDeviceIoEvent);
//print("before kb_close\n");
	kb_close();
//print("before unload\n");
	/* unload driver */
	udefrag_unload();
//print("after unload\n");
	/* registry cleanup */
	err_msg = udefrag_save_settings();
	if(err_msg)
	{
		sprintf(msg,"\nERROR: %s Wait 5 seconds.\n",err_msg);
		print(msg);
	}
}

void ProcessVolume(char letter,char command)
{
	ULTRADFG_COMMAND cmd;
	STATISTIC stat;
	char s[32];
	LARGE_INTEGER offset;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	HANDLE hThread;
	double p;
	unsigned int ip;

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
	cmd.sizelimit = settings->sizelimit;
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
		fbsize(s,stat.total_space);
		print("\n  Volume size                  = ");
		print(s);
		fbsize(s,stat.free_space);
		print("\n  Free space                   = ");
		print(s);
		sprintf(msg,"\n\n  Total number of files        = %u",stat.filecounter);
		print(msg);
		sprintf(msg,"\n  Number of fragmented files   = %u",stat.fragmfilecounter);
		print(msg);
		p = (double)(stat.fragmcounter)/(double)(stat.filecounter);
		ip = (unsigned int)(p * 100.00);
		sprintf(msg,"\n  Fragments per file           = %u.%02u\r\n\r\n",ip / 100,ip % 100);
		print(msg);
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
	KEYBOARD_INPUT_DATA kbd;
	char *err_msg;

	/* 1. Normalize and get the Process Parameters */
	ProcessParameters = RtlNormalizeProcessParams(Peb->ProcessParameters);

	/* 2. Breakpoint if we were requested to do so */
	if (ProcessParameters->DebugFlags) DbgBreakPoint();

	/* 3. Initialization */
	settings = udefrag_get_settings();
#if USE_INSTEAD_SMSS
	NtInitializeRegistry(FALSE);
#endif
	offset.QuadPart = 0;
	/* 4. Display Copyright */
#ifdef NT4_TARGET
	print("\n\n");
#endif
	RtlInitUnicodeString(&strU,
		VERSIONINTITLE_U L" native interface\n"
		L"Copyright (c) Dmitri Arkhangelski, 2007.\n\n"
		L"UltraDefrag comes with ABSOLUTELY NO WARRANTY.\n\n"
		L"If something is wrong, hit F8 on startup\n"
		L"and select 'Last Known Good Configuration'.\n"
		L"Use Break key to abort defragmentation.\n\n");
	NtDisplayString(&strU);
	/* 6. Open the keyboard */
	if(!kb_open())
		goto continue_boot;
/*	RtlInitUnicodeString(&strU,L"\\UDefragStopEvent");
	InitializeObjectAttributes(&ObjectAttributes,&strU,0,NULL,NULL);
	Status = NtCreateEvent(&hStopEvent,0x1f01ff,&ObjectAttributes,1,0);
	if(Status)
	{
		print("\nERROR: Can't create \\UDefragStopEvent! Wait 5 seconds.\n");
		IntSleep(5000);
		goto continue_boot;///abort;
	}
*/	/* 6b. Prompt to exit */
	print("Press any key to exit...  ");
	/*Status = RtlCreateUserThread(NtCurrentProcess(),NULL,FALSE,
		0,0,0,wait_kb_proc,NULL,&hThread,NULL);
	if(Status)
	{
		print("\nCan't create thread: "); 
		_itoa(Status,buffer,16);
		print(buffer);
		print("\n");
	}
	*/
	for(i = 0; i < 3; i++)
	{
		///IntSleep(1000);
		if(WaitKbHit(1000,&kbd) == STATUS_SUCCESS)
			//abort_flag = 1;

		//if(abort_flag)
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
	err_msg = udefrag_init(TRUE);
	if(err_msg)
	{
		sprintf(msg,"\nERROR: %s Wait 5 seconds.\n",err_msg);
		print(msg); IntSleep(5000);
		goto continue_boot;
	}
	hUltraDefragDevice = get_device_handle();
	RtlInitUnicodeString(&strU,L"\\UltraDefragIoEvent");
	InitializeObjectAttributes(&ObjectAttributes,&strU,0,NULL,NULL);
	Status = NtCreateEvent(&hDeviceIoEvent,0x1f01ff,&ObjectAttributes,1,0);
	if(Status)
	{
		print("\nERROR: Can't create \\UltraDefragIoEvent! Wait 5 seconds.\n");
		IntSleep(5000);
		goto continue_boot;///abort;
	}
	/* read parameters from registry */
	err_msg = udefrag_load_settings(0,NULL);
	if(err_msg)
	{
		sprintf(msg,"\nERROR: %s Wait 5 seconds.\n",err_msg);
		print(msg); IntSleep(5000);
		goto continue_boot;
	}
	err_msg = udefrag_apply_settings();
	if(err_msg)
	{
		sprintf(msg,"\nERROR: %s Wait 5 seconds.\n",err_msg);
		print(msg); IntSleep(5000);
		goto continue_boot;
	}

	/* 8a. Batch mode */
#if USE_INSTEAD_SMSS
#else
	if(!settings->next_boot && !settings->every_boot)
		goto cmd_loop;
	/* do job */
	if(!settings->sched_letters) // and if empty string
	{
		print("\nNo letters specified!\n");
		goto continue_boot;
	}
	length = wcslen(settings->sched_letters);
	for(i = 0; i < length; i++)
	{
		ProcessVolume((char)(settings->sched_letters[i]),'d');
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
