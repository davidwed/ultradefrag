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
#include <ctype.h>

#include "../include/udefrag.h"
#include "../include/ultradfgver.h"

#define settextcolor(c) SetConsoleTextAttribute(hOut,c)

/* global variables */
HANDLE hOut;
WORD console_attr = 0x7;
int a_flag = 0,o_flag = 0;
int b_flag = 0,h_flag = 0;
char letter = 0;
char unk_opt[] = "Unknown option: x!";
int unknown_option = 0;
char oem_buffer[4096];
char oem_stop_buffer[4096];

/* internal functions prototypes */
void HandleError(char *err_msg,int exit_code);
BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType);

void show_help(void)
{
	printf(
		"Usage: udefrag [command] [options] [volumeletter:]\n"
		"  The default action is to display this help message.\n"
		"Commands:\n"
		"  -a  analyse only\n"
		"  -o  optimize volume space\n"
		"  -?  show this help\n"
		"  If command is not specified it will defragment volume.\n"
		"Options:\n"
		"  -sN      ignore files larger than N bytes\n"
		"  -iA;B;C  set include filter with items A, B and C\n"
		"  -eX;Y;Z  set exclude filter with items X, Y and Z\n"
		"  -dN      set debug print level:\n"
		"           N=0 Normal, N=1 Detailed, N=2 Paranoid\n"
		"  -b       use default color scheme"
		);
	HandleError("",0);
}

void HandleError(char *err_msg,int exit_code)
{
	if(err_msg)
	{ /* we should display error and terminate process */
		if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
		printf("%s\n",err_msg);
		if(!b_flag) settextcolor(console_attr);
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
		udefrag_unload(FALSE);
		exit(exit_code);
	}
}

void HandleErrorW(short *err_msg,int exit_code)
{
	if(err_msg)
	{ /* we should display error and terminate process */
		if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
		WideCharToMultiByte(CP_OEMCP,0,err_msg,-1,oem_buffer,4095,NULL,NULL);
		printf("%s\n",oem_buffer);
		if(!b_flag) settextcolor(console_attr);
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
		udefrag_unload(FALSE);
		exit(exit_code);
	}
}

BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
{
	short *err_msg;

	err_msg = udefrag_stop();
	if(err_msg)
	{
		if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
		WideCharToMultiByte(CP_OEMCP,0,err_msg,-1,oem_stop_buffer,4095,NULL,NULL);
		printf("\n%s\n",oem_stop_buffer);
		if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	}
	return TRUE;
}

void parse_cmdline(int argc, short **argv)
{
	int i;
	short c1,c2;

	if(argc < 2) h_flag = 1;
	for(i = 1; i < argc; i++)
	{
		c1 = argv[i][0]; c2 = argv[i][1];
		if(c1 && c2 == ':')	letter = (char)c1;
		if(c1 == '-')
		{
			c2 = (short)towlower((wint_t)c2);
			if(!wcschr(L"abosideh?",c2))
			{ /* unknown option */
				unk_opt[16] = (char)c2;
				unknown_option = 1;
			}
			if(c2 == 'h' || c2 == '?')
				h_flag = 1;
			else if(c2 == 'a') a_flag = 1;
			else if(c2 == 'o') o_flag = 1;
			else if(c2 == 'b') b_flag = 1;
		}
	}
	/* if only -b option is specified, show help message */
	if(argc == 2 && b_flag) h_flag = 1;
}

int __cdecl wmain(int argc, short **argv)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	STATISTIC stat;

	/* analyse command line */
	parse_cmdline(argc,argv);
	/* display copyright */
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if(GetConsoleScreenBufferInfo(hOut,&csbi))
		console_attr = csbi.wAttributes;
	if(!b_flag)
		settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf(VERSIONINTITLE " console interface\n"
	      "Copyright (c) Dmitri Arkhangelski, 2007.\n\n");
	/* handle unknown option and help request */
	if(unknown_option) HandleError(unk_opt,1);
	if(h_flag) show_help();
	/* validate driveletter */
	if(!letter)	HandleError("Drive letter should be specified!",1);
	HandleErrorW(udefrag_validate_volume(letter,FALSE),1);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,TRUE);
	/* do our job */
	HandleErrorW(udefrag_init(argc,argv,FALSE),2);

	if(a_flag) HandleErrorW(udefrag_analyse(letter),3);
	else if(o_flag) HandleErrorW(udefrag_optimize(letter),3);
	else HandleErrorW(udefrag_defragment(letter),3);

	/* display results and exit */
	HandleErrorW(udefrag_get_progress(&stat,NULL),0);
	printf("\n%s",udefrag_get_default_formatted_results(&stat));
	HandleError("",0);
	return 0; /* we will never reach this point */
}

#if defined(__GNUC__)
/* HACK - MINGW HAS NO OFFICIAL SUPPORT FOR wmain()!!! */
/* Copyright (c) ReactOS Development Team */
int main( int argc, char **argv )
{
    WCHAR **argvW;
    int i, j, Ret = 1;

    if ((argvW = malloc(argc * sizeof(WCHAR*))))
    {
        /* convert the arguments */
        for (i = 0, j = 0; i < argc; i++)
        {
            if (!(argvW[i] = malloc((strlen(argv[i]) + 1) * sizeof(WCHAR))))
            {
                j++;
            }
            swprintf(argvW[i], L"%hs", argv[i]);
        }
        
        if (j == 0)
        {
            /* no error converting the parameters, call wmain() */
            Ret = wmain(argc, (short **)argvW);
        }
		else
		{
			printf("No enough memory for the program execution!\n");
		}
        
        /* free the arguments */
        for (i = 0; i < argc; i++)
        {
            if (argvW[i])
                free(argvW[i]);
        }
        free(argvW);
    }
    
    return Ret;
}
#endif
