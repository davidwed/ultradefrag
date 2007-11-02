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

#include "../include/udefrag.h"
#include "../include/ultradfgver.h"

#define settextcolor(c) SetConsoleTextAttribute(hOut,c)

/* global variables */
HANDLE hOut;
WORD console_attr = 0x7;
int a_flag = 0,c_flag = 0;
char letter = 0;

/* internal functions prototypes */
void HandleError(char *err_msg,int exit_code);
BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType);

void show_help(void)
{
	printf("Usage: defrag [-a,-c,-?] driveletter:\n"
		 " -a\tanalyse only\n"
		 " -c\tcompact space\n"
		 " -?\tshow this help");
	HandleError("",0);
}

void HandleError(char *err_msg,int exit_code)
{
	if(err_msg)
	{ /* we should display error and terminate process */
		settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
		printf("%s\n",err_msg);
		settextcolor(console_attr);
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
		udefrag_unload();
		exit(exit_code);
	}
}

BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
{
	char *err_msg;

	err_msg = udefrag_stop();
	if(err_msg)
	{
		settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
		printf("\n%s\n",err_msg);
		settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	}
	return TRUE;
}

void parse_cmdline(int argc, short **argv)
{
	int i;
	short c1,c2;
	char unk_opt[] = "Unknown option: x!";

	if(argc < 2) show_help();
	for(i = 1; i < argc; i++)
	{
		c1 = argv[i][0]; c2 = argv[i][1];
		if(c1 && c2 == ':')	letter = (char)c1;
		if(c1 == '-')
		{
			c2 = (short)towlower((wint_t)c2);
			if(!wcschr(L"acsideh?",c2))
			{ /* unknown option */
				unk_opt[16] = (char)c2;
				HandleError(unk_opt,1);
			}
			if(c2 == 'h' || c2 == '?')
				show_help();
			else if(c2 == 'a') a_flag = 1;
			else if(c2 == 'c') c_flag = 1;
		}
	}
}

int __cdecl wmain(int argc, short **argv)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	STATISTIC stat;

	/* display copyright */
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if(GetConsoleScreenBufferInfo(hOut,&csbi))
		console_attr = csbi.wAttributes;
	settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf(VERSIONINTITLE " console interface\n"
	      "Copyright (c) Dmitri Arkhangelski, 2007.\n\n");
	/* analyse command line */
	parse_cmdline(argc,argv);
	/* validate driveletter */
	if(!letter)	HandleError("Drive letter should be specified!",1);
	HandleError(udefrag_validate_volume(letter,FALSE),1);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,TRUE);
	/* do our job */
	HandleError(udefrag_init(FALSE),2);
	udefrag_load_settings(argc,argv);
	HandleError(udefrag_apply_settings(),2);

	if(a_flag) HandleError(udefrag_analyse(letter),3);
	else if(c_flag) HandleError(udefrag_optimize(letter),3);
	else HandleError(udefrag_defragment(letter),3);

	/* display results and exit */
	HandleError(udefrag_get_progress(&stat,NULL),0);
	printf("\n%s",get_default_formatted_results(&stat));
	HandleError("",0);
}
