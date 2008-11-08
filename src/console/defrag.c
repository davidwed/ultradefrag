/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* UltraDefrag console interface.
*/

#include <windows.h>
#include <shellapi.h>
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
int l_flag = 0,la_flag = 0;
int obsolete_option = 0;
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
		"  -l  list all available volumes except removable\n"
		"  -la list all available volumes\n"
		"  -?  show this help\n"
		"  If command is not specified it will defragment volume.\n"
		"Options:\n"
		"  -b       use default color scheme"
		);
	HandleError("",0);
}

void HandleError(char *err_msg,int exit_code)
{
	if(err_msg){ /* we should display error and terminate process */
		if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
		printf("%s\n",err_msg);
		if(!b_flag) settextcolor(console_attr);
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
		if(udefrag_unload() < 0) udefrag_pop_error(NULL,0);
		exit(exit_code);
	}
}

void HandleErrorW(int status,int exit_code)
{
	short buffer[ERR_MSG_SIZE];
	
	if(status < 0){ /* we should display error and terminate process */
		udefrag_pop_werror(buffer,ERR_MSG_SIZE);
		if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
		WideCharToMultiByte(CP_OEMCP,0,buffer,-1,oem_buffer,ERR_MSG_SIZE - 1,NULL,NULL);
		printf("%s\n",oem_buffer);
		if(!b_flag) settextcolor(console_attr);
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
		if(udefrag_unload() < 0) udefrag_pop_werror(NULL,0);
		exit(exit_code);
	}
}

/* returns an exit code for console program terminating */
int show_vollist(void)
{
	volume_info *v;
	int n;
	char s[32];
	double d;
	int p;
	short buffer[ERR_MSG_SIZE];

	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf("Volumes available for defragmentation:\n\n");

	if(udefrag_get_avail_volumes(&v,la_flag ? FALSE : TRUE) < 0){
		udefrag_pop_werror(buffer,ERR_MSG_SIZE);
		if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
		WideCharToMultiByte(CP_OEMCP,0,buffer,-1,oem_buffer,ERR_MSG_SIZE - 1,NULL,NULL);
		printf("%s\n",oem_buffer);
		if(!b_flag) settextcolor(console_attr);
		return 1;
	} else {
		for(n = 0;;n++){
			if(v[n].letter == 0) break;
			udefrag_fbsize((ULONGLONG)(v[n].total_space.QuadPart),2,s,sizeof(s));
			d = (double)(signed __int64)(v[n].free_space.QuadPart);
			/* 0.1 constant is used to exclude divide by zero error */
			d /= ((double)(signed __int64)(v[n].total_space.QuadPart) + 0.1);
			p = (int)(100 * d);
			printf("%c:  %8s %12s %8u %%\n",
				v[n].letter,v[n].fsname,s,p);
		}
	}

	if(!b_flag) settextcolor(console_attr);
	return 0;
}

BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
{
	short buffer[ERR_MSG_SIZE];
	
	if(udefrag_stop() < 0){
		udefrag_pop_werror(buffer,ERR_MSG_SIZE);
		if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
		WideCharToMultiByte(CP_OEMCP,0,buffer,-1,oem_stop_buffer,ERR_MSG_SIZE - 1,NULL,NULL);
		printf("\n%s\n",oem_stop_buffer);
		if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	}
	return TRUE;
}

void parse_cmdline(void)
{
	int argc;
	short **argv;
	int i;
	short c1,c2,c3;

	argv = (short **)CommandLineToArgvW(GetCommandLineW(),&argc);
	if(!argv) return;

	if(argc < 2) h_flag = 1;
	for(i = 1; i < argc; i++){
		/* FIXME: if argv[i] has zero length */
		c1 = argv[i][0];
		if(!c1) continue;
		c2 = argv[i][1];
		if(c2 == ':') letter = (char)c1;
		if(c1 == '-'){
			c2 = (short)towlower((wint_t)c2);
			if(!wcschr(L"abosideh?l",c2)){
				/* unknown option */
				unk_opt[16] = (char)c2;
				unknown_option = 1;
			}
			if(wcschr(L"iesd",c2))
				obsolete_option = 1;
			if(c2 == 'h' || c2 == '?')
				h_flag = 1;
			else if(c2 == 'a') a_flag = 1;
			else if(c2 == 'o') o_flag = 1;
			else if(c2 == 'b') b_flag = 1;
			else if(c2 == 'l'){
				l_flag = 1;
				c3 = (short)towlower((wint_t)argv[i][2]);
				if(c3 == 'a') la_flag = 1;
			}
		}
	}
	/* if only -b option is specified, show help message */
	if(argc == 2 && b_flag) h_flag = 1;
	GlobalFree(argv);
}

int __cdecl main(int argc, char **argv)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	STATISTIC stat;

	/* analyse command line */
	parse_cmdline();
	/* display copyright */
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if(GetConsoleScreenBufferInfo(hOut,&csbi))
		console_attr = csbi.wAttributes;
	if(!b_flag)
		settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf(VERSIONINTITLE " console interface\n"
	      "Copyright (c) Dmitri Arkhangelski, 2007,2008.\n\n");
	/* handle unknown option and help request */
	if(unknown_option) HandleError(unk_opt,1);
	if(obsolete_option)
		HandleError("The -i, -e, -s, -d options are oblolete.\n"
					"Use environment variables instead!",1);
	if(h_flag) show_help();
	/* show list of volumes if requested */
	if(l_flag){ exit(show_vollist()); }
	/* validate driveletter */
	if(!letter)	HandleError("Drive letter should be specified!",1);
	HandleErrorW(udefrag_validate_volume(letter,FALSE),1);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,TRUE);
	/* do our job */
	HandleErrorW(udefrag_init(0),2);

	if(a_flag) HandleErrorW(udefrag_analyse(letter,NULL),3);
	else if(o_flag) HandleErrorW(udefrag_optimize(letter,NULL),3);
	else HandleErrorW(udefrag_defragment(letter,NULL),3);

	/* display results and exit */
	HandleErrorW(udefrag_get_progress(&stat,NULL),0);
	printf("\n%s",udefrag_get_default_formatted_results(&stat));
	HandleError("",0);
	return 0; /* we will never reach this point */
}
