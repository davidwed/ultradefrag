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
#include "../include/udefrag.h"
#include "../include/ultradfg.h"
#include "../include/ultradfgver.h"

HANDLE hOut;
WORD console_attr = 0x7;
HANDLE hUltraDefragDevice = INVALID_HANDLE_VALUE;
HANDLE hEvt = NULL;
OVERLAPPED ovrl;

ud_settings *settings;

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
/*	DWORD txd;
	HANDLE hEvtStop = NULL;

	hEvtStop = CreateEvent(NULL,TRUE,TRUE,"UltraDefragStop");
	ovrl.hEvent = hEvtStop;
	ovrl.Offset = ovrl.OffsetHigh = 0;
	if(DeviceIoControl(hUltraDefragDevice,IOCTL_ULTRADEFRAG_STOP,
					NULL,0,NULL,0,&txd,&ovrl))
		WaitForSingleObject(hEvtStop,INFINITE);
	if(hEvtStop) CloseHandle(hEvtStop);
	*/
	char *err_msg;

	err_msg = udefrag_stop();
	if(err_msg)
	{
		display_error(err_msg); putchar('\n');
	}
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
	int a_flag = 0,c_flag = 0;//,f_flag = 0;
//	short *in_filter = NULL, *ex_filter = NULL;
//	int length;
	char letter = 0;
	char rootdir[] = "A:\\";
	DWORD txd;
	char s[32];
	STATISTIC stat;
	int err_code = 0;
	ULONGLONG sizelimit = 0;
	double p;

	ULTRADFG_COMMAND cmd;
	char *err_msg;
	UCHAR command;

	settings = udefrag_get_settings();
	/* display copyright */
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if(GetConsoleScreenBufferInfo(hOut,&csbi))
		console_attr = csbi.wAttributes;
	SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	print(VERSIONINTITLE " console interface\n"
	      "Copyright (c) Dmitri Arkhangelski, 2007.\n\n");
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
/*			case 'f':
			case 'F':
				f_flag = 1;
				break;
*/			case '?':
			case 'h':
			case 'H':
				show_help();
				goto cleanup;
			case 's':
			case 'S':
				//sizelimit = _wtoi64(argv[n] + 2);
				//break;
			case 'i':
			case 'I':
				//in_filter = (argv[n] + 2);
				//break;
			case 'd':
			case 'D':
			case 'e':
			case 'E':
				//ex_filter = (argv[n] + 2);
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
	err_msg = udefrag_init(FALSE);
	if(err_msg)
	{
		display_error(err_msg); putchar('\n');
		err_code = 3; goto cleanup;
	}
	err_msg = udefrag_load_settings(argc,argv);
	if(err_msg)
	{
		display_error(err_msg); putchar('\n');
		err_code = 3; goto cleanup;
	}
	err_msg = udefrag_apply_settings();
	if(err_msg)
	{
		display_error(err_msg); putchar('\n');
		err_code = 3; goto cleanup;
	}
	hUltraDefragDevice = get_device_handle();
/*	length = in_filter ? ((wcslen(in_filter) + 1) << 1) : 0;
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
*/
//	command = a_flag ? 'a' : 'd';
//	if(c_flag) command = 'c';
	//cmd.letter = letter;
	//cmd.sizelimit = settings->sizelimit;
	//cmd.mode = __UserMode;

/*	ovrl.hEvent = hEvt;
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
*/
	if(a_flag)
	{
		err_msg = udefrag_analyse(letter);
		goto done;
	}
	if(!c_flag)
	{
		err_msg = udefrag_defragment(letter);
		goto done;
	}
	else
	{
		err_msg = udefrag_optimize(letter);
		goto done;
	}
done:
	if(err_msg)	{ display_error(err_msg); putchar('\n'); err_code = 3; goto cleanup; }
	/* display results */
	//ovrl.hEvent = hEvt;
	//ovrl.Offset = ovrl.OffsetHigh = 0;
	//if(DeviceIoControl(hUltraDefragDevice,IOCTL_GET_STATISTIC,NULL,0, \
	//	&stat,sizeof(STATISTIC),&txd,&ovrl))
	err_msg = udefrag_get_progress(&stat,NULL);
	if(err_msg)	{ display_error(err_msg); putchar('\n'); err_code = 3; }
	else
	{
	//	WaitForSingleObject(hEvt,INFINITE);
		print("\nVolume information:\n");
		fbsize(s,stat.total_space);
		printf("\n  Volume size                  = %s",s);
		fbsize(s,stat.free_space);
		printf("\n  Free space                   = %s",s);
		printf("\n\n  Total number of files        = %u",stat.filecounter);
		printf("\n  Number of fragmented files   = %u",stat.fragmfilecounter);
		p = (double)(stat.fragmcounter)/(double)(stat.filecounter);
		printf("\n  Fragments per file           = %.2f\n",p);
	}

cleanup:
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
	if(hEvt) CloseHandle(hEvt);
	udefrag_unload();
	SetConsoleTextAttribute(hOut, console_attr);
	ExitProcess(err_code);
}
 