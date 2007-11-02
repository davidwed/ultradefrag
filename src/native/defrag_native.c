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
#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <ntddkbd.h>

#include "../include/ntndk.h"
#include "../include/udefrag.h"
#include "../include/ultradfgver.h"

#define USE_INSTEAD_SMSS  0

ud_settings *settings;

char letter;

int abort_flag = 0,done_flag = 0,wait_flag = 0;
char last_op = 0;
int i = 0,j = 0,k; /* number of '-' */

char msg[2048];
char buffer[256];

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
void Cleanup();
void HandleError(char *err_msg,int exit_code)
{
	if(err_msg)
	{
		if(err_msg[0])
		{
			nprint(err_msg);
			nprint("\n");
		}
		nprint("Good bye ...\n");
		Cleanup();
		NtTerminateProcess(NtCurrentProcess(),4);
	}
}

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
					nputch('-'); /* 100 % of previous operation */
			}
			else
			{
				if(stat.current_operation != 'A')
				{
					nprint("\nA: ");
					for(k = 0; k < 50; k++)
						nputch('-');
				}
			}
			i = 0; /* new operation */
			sprintf(msg,"\n%c: ",stat.current_operation);
			nprint(msg);
			last_op = stat.current_operation;
		}
		j = (int)percentage / 2;
		for(k = i; k < j; k++)
			nputch('-');
		i = j;
	}
}

DWORD WINAPI wait_break_proc(LPVOID unused)
{
	KEYBOARD_INPUT_DATA kbd;
	char *err_msg;

	i = j = 0;
	last_op = 0;
	do
	{
		if(kb_hit(&kbd,100))
		{
			if((kbd.Flags & KEY_E1) && (kbd.MakeCode == 0x1d))
			{
				err_msg = udefrag_stop();
				if(err_msg)
				{
					sprintf(msg,"\nERROR: %s Wait 5 seconds.\n",err_msg);
					nprint(msg);
				}
				abort_flag = 1;
			}
		}
		UpdateProgress();
	} while(!done_flag);
	if(!abort_flag) /* set progress to 100 % */
	{
		for(k = i; k < 50; k++)
			nputch('-');
	}

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
	char *err_msg;

//nprint("before stop\n");
	/* stop device */
	err_msg = udefrag_stop();
	if(err_msg)
	{
		sprintf(msg,"\nERROR: %s Wait 5 seconds.\n",err_msg);
		nprint(msg);
	}
nprint("before kb_close\n");
	kb_close();
nprint("before unload\n");
	/* unload driver */
	udefrag_unload();
nprint("after unload\n");
	/* registry cleanup */
//	err_msg = udefrag_save_settings();
//	if(err_msg)
//	{
//		sprintf(msg,"\nERROR: %s Wait 5 seconds.\n",err_msg);
//		nprint(msg);
//	}
}

void ProcessVolume(char letter,char command)
{
	STATISTIC stat;
	NTSTATUS Status;
	HANDLE hThread;
	char *err_msg;

	done_flag = 0;
	Status = RtlCreateUserThread(NtCurrentProcess(),NULL,FALSE,
		0,0,0,wait_break_proc,NULL,&hThread,NULL);
	if(!NT_SUCCESS(Status))
	{
		sprintf(msg,"\nCan't create thread: %x!\n",Status); 
		nprint(msg);
	}

	wait_flag = 1;
	nprint("\nPreparing to ");
	switch(command)
	{
	case 'a':
		sprintf(msg,"analyse %c: ...\n",letter);
		nprint(msg);
		err_msg = udefrag_analyse(letter);
		break;
	case 'd':
		sprintf(msg,"defragment %c: ...\n",letter);
		nprint(msg);
		err_msg = udefrag_defragment(letter);
		break;
	case 'c':
		sprintf(msg,"optimize %c: ...\n",letter);
		nprint(msg);
		err_msg = udefrag_optimize(letter);
		break;
	}
	done_flag = 1;
	while(wait_flag) nsleep(10);
	if(err_msg)
	{
		sprintf(msg,"\nERROR: %s Wait 5 seconds.\n",err_msg);
		nprint(msg);
		return;
	}

	err_msg = udefrag_get_progress(&stat,NULL);
	if(err_msg)
	{
		sprintf(msg,"\nERROR: %s Wait 5 seconds.\n",err_msg);
		nprint(msg);
	}
	else
	{
		nprint("\n");
		nprint(get_default_formatted_results(&stat));
		nprint("\n\n");
	}
}

void __stdcall NtProcessStartup(PPEB Peb)
{
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	DWORD Action = ShutdownReboot;
	char *pos;
	int a_flag, d_flag, c_flag;
	char command;
	HANDLE hThread = NULL;
	DWORD i,length;
	KEYBOARD_INPUT_DATA kbd;
	volume_info *v;
	int n;
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
	/* 4. Display Copyright */
#ifdef NT4_TARGET
	nprint("\n\n");
#endif
	nprint(VERSIONINTITLE " native interface\n"
		"Copyright (c) Dmitri Arkhangelski, 2007.\n\n"
		"UltraDefrag comes with ABSOLUTELY NO WARRANTY.\n\n"
		"If something is wrong, hit F8 on startup\n"
		"and select 'Last Known Good Configuration'.\n"
		"Use Break key to abort defragmentation.\n\n");
	/* 6. Open the keyboard */
	HandleError(kb_open(),2);
	/* 6b. Prompt to exit */
	nprint("Press any key to exit...  ");
	for(i = 0; i < 3; i++)
	{
		if(kb_hit(&kbd,1000)) HandleError("\n",0);
		nputch((char)('0' + 3 - i));
		nputch(' ');
	}
	nprint("\n\n");
	/* 7. Initialize the ultradfg device */
	HandleError(udefrag_init(TRUE),2);
	/* read parameters from registry */
	udefrag_load_settings(0,NULL);
	HandleError(udefrag_apply_settings(),2);

	/* 8a. Batch mode */
#if USE_INSTEAD_SMSS
#else
	if(settings->next_boot || settings->every_boot)
	{
		/* do scheduled job and exit */
		if(!settings->sched_letters)
			HandleError("\nNo letters specified!",1);
		if(!settings->sched_letters[0])
			HandleError("\nNo letters specified!",1);
		length = wcslen(settings->sched_letters);
		for(i = 0; i < length; i++)
		{
			ProcessVolume((char)(settings->sched_letters[i]),'d');
			if(abort_flag) HandleError("",0);
		}
		HandleError("",0);
	}
#endif
	/* 8b. Command Loop */
	while(1)
	{
		nprint("# ");
		HandleError(kb_gets(buffer,sizeof(buffer)),5);
		if(strstr(buffer,"shutdown"))
		{
			nprint("Shutdown ...");
			Action = ShutdownNoReboot;
			goto exit;
		}
		if(strstr(buffer,"reboot"))
		{
			nprint("Reboot ...");
			goto exit;
		}
		if(strstr(buffer,"batch"))
		{
			nprint("Prepare to batch processing ...\n");
			continue;
		}
		if(strstr(buffer,"exit"))
		{
			HandleError("",0);
			return;
		}

		if(strstr(buffer,"analyse")) a_flag = 1;
		else a_flag = 0;

		if(strstr(buffer,"defrag")) d_flag = 1;
		else d_flag = 0;

		if(strstr(buffer,"compact")) c_flag = 1;
		else c_flag = 0;

		if(a_flag || d_flag || c_flag)
		{
			pos = strchr(buffer,':');
			if(!pos || pos == buffer)
				continue;
			if(a_flag) command = 'a';
			else if(d_flag)	command = 'd';
			else command = 'c';
			ProcessVolume(pos[-1],command);
			continue;
		}
		if(strstr(buffer,"drives"))
		{
			nprint("Available drive letters:   ");
			err_msg = udefrag_get_avail_volumes(&v,FALSE);
			if(err_msg) nprint(err_msg);
			else
			{
				for(n = 0;;n++)
				{
					if(v[n].letter == 0) break;
					sprintf(msg,"%c   ",v[n].letter);
					nprint(msg);
				}
			}
			nprint("\r\n");
			continue;
		}
		if(strstr(buffer,"help"))
		{
			nprint(USAGE);
			continue;
		}
		nprint("Unknown command!\n");
	}
exit:
	Cleanup();
	NtShutdownSystem(Action);
	/* 8. We're done here */
	NtTerminateProcess(NtCurrentProcess(), 0);
}
