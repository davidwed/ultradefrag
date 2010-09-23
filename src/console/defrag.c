/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <process.h>

#include "../include/udefrag.h"
#include "../include/ultradfgver.h"

void __stdcall IncreaseGoogleAnalyticsCounterAsynch(char *hostname,char *path,char *account);

#define settextcolor(c) (void)SetConsoleTextAttribute(hOut,c)

/* global variables */
HANDLE hOut;
WORD console_attr = 0x7;
int a_flag = 0,o_flag = 0;
int b_flag = 0,h_flag = 0;
int l_flag = 0,la_flag = 0;
int p_flag = 0,v_flag = 0;
int m_flag = 0;
int obsolete_option = 0;
char letter = 0;
int screensaver_mode = 0;
int all_flag = 0;
int all_fixed_flag = 0;
char letters[MAX_DOS_DRIVES] = {0};
int wait_flag = 0;

int stop_flag = 0;

extern int map_rows;
extern int map_symbols_per_line;

void parse_cmdline(int argc, char **argv);
void show_help(void);
void AllocateClusterMap(void);
void FreeClusterMap(void);
void RedrawMap(void);
void InitializeMapDisplay(void);
void __stdcall update_progress(udefrag_progress_info *pi, void *p);

BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType);
void display_error(char *string);
void DisplayLastError(char *caption);

/* fills the current line with spaces */
void clear_line(FILE *f)
{
	char *line;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD cursor_pos;
	int n;
	
	if(!GetConsoleScreenBufferInfo(hOut,&csbi))
		return; /* impossible to determine the screen width */
	n = (int)csbi.dwSize.X;
	line = malloc(n + 1);
	if(!line) return;
	
	memset(line,0x20,n);
	line[n] = 0;
	fprintf(f,"\r%s",line);
	free(line);
	
	/* move cursor back to the previous line */
	if(!GetConsoleScreenBufferInfo(hOut,&csbi)){
		DisplayLastError("Cannot retrieve cursor position!");
		return; /* impossible to determine the current cursor position  */
	}
	cursor_pos.X = 0;
	cursor_pos.Y = csbi.dwCursorPosition.Y - 1;
	if(!SetConsoleCursorPosition(hOut,cursor_pos))
		DisplayLastError("Cannot set cursor position!");
}

/* prints specified string in red, than restores green/default color */
void display_error(char *string)
{
	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
	printf("%s",string);
	if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

void DisplayLastError(char *caption)
{
	LPVOID lpMsgBuf;
	DWORD error = GetLastError();

	if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,0,NULL)){
				printf("\n%s\nError code = 0x%x\n\n",caption,(UINT)error);
				return;
	} else {
		printf("\n%s\n%s\n",caption,(char *)lpMsgBuf);
		LocalFree(lpMsgBuf);
	}
}

void DisplayDefragError(int error_code,char *caption)
{
	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
	printf("\n%s\n\n",caption);
	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf("%s\n\n",udefrag_get_error_description(error_code));
	if(error_code == UDEFRAG_UNKNOWN_ERROR) printf("Use DbgView program to get more information.\n\n");
	if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

void DisplayInvalidVolumeError(int error_code)
{
	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
	printf("The volume cannot be processed.\n\n");
	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);

	if(error_code == UDEFRAG_UNKNOWN_ERROR){
		printf("Volume is missing or some error has been encountered.\n");
		printf("Use DbgView program to get more information.\n\n");
	} else {
		printf("%s\n\n",udefrag_get_error_description(error_code));
	}

	if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

/* returns an exit code for console program terminating */
int show_vollist(void)
{
	volume_info *v;
	int i;
	char s[32];
	double d;
	int p;

	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf("Volumes available for defragmentation:\n\n");

	v = udefrag_get_vollist(la_flag ? FALSE : TRUE);
	if(v == NULL)
		return 1;

	for(i = 0; v[i].letter != 0; i++){
		udefrag_fbsize((ULONGLONG)(v[i].total_space.QuadPart),2,s,sizeof(s));
		d = (double)(signed __int64)(v[i].free_space.QuadPart);
		/* 0.1 constant is used to exclude divide by zero error */
		d /= ((double)(signed __int64)(v[i].total_space.QuadPart) + 0.1);
		p = (int)(100 * d);
		if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		printf("%c:  %8s %12s %8u %%\n",
			v[i].letter,v[i].fsname,s,p);
	}
	udefrag_release_vollist(v);
	return 0;
}

BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
{
	stop_flag = 1;
	return TRUE;
}

int __stdcall terminator(void *p)
{
	/* do it as quickly as possible :-) */
	return stop_flag;
}

void RunScreenSaver(void)
{
	printf("Hello!\n");
}

int process_single_volume(void)
{
	long map_size = 0;
	int error_code = 0;

	/* validate driveletter */
	if(!letter){
		display_error("Drive letter should be specified!\n");
		return 1;
	}
	error_code = udefrag_validate_volume(letter,FALSE);
	if(error_code < 0){
		DisplayInvalidVolumeError(error_code);
		return 1;
	}

	/* allocate memory for cluster map */
	if(m_flag) AllocateClusterMap();
	
	/* check if we need to run in screensaver mode */
	if(screensaver_mode){
		RunScreenSaver();
		FreeClusterMap();
		return 0;
	}

	/* do our job */
	if(m_flag) map_size = map_rows * map_symbols_per_line;
	
	/* TODO: wait until already running task completes */
//	while(1){
//		error_code = udefrag_init();
//		if(error_code >= 0) break; /* initialization succeded */
//		if(error_code == UDEFRAG_ALREADY_RUNNING && wait_flag){
//			/* wait one second and try again */
//			Sleep(1000);
//			continue;
//		} else {
//			DisplayDefragError(error_code,"Initialization failed!");
//			FreeClusterMap();
//			return 2;
//		}
//	}

	if(m_flag) /* prepare console buffer for map */
		InitializeMapDisplay();
	
	stop_flag = 0;
	if(a_flag){
		error_code = udefrag_start_job(letter,ANALYSIS_JOB,map_size,update_progress,terminator,NULL);
	} else if(o_flag){
		error_code = udefrag_start_job(letter,OPTIMIZER_JOB,map_size,update_progress,terminator,NULL);
	} else {
		error_code = udefrag_start_job(letter,DEFRAG_JOB,map_size,update_progress,terminator,NULL);
	}
	if(error_code < 0){
		DisplayDefragError(error_code,"Analysis/Defragmentation failed!");
		FreeClusterMap();
		return 3;
	}

	FreeClusterMap();
	return 0;
}

int process_multiple_volumes(void)
{
	volume_info *v;
	int i;
	
	/* process volumes specified on the command line */
	if(letters[1]){
		for(i = 0; i < MAX_DOS_DRIVES; i++){
			if(!letters[i]) break;
			if(stop_flag) break;
			/**/
			//printf("%c:\n",letters[i]);
			letter = letters[i];
			(void)process_single_volume();
		}
	}
	
	if(stop_flag) return 0;
	
	/* process all volumes if requested */
	if(all_flag || all_fixed_flag){
		v = udefrag_get_vollist(all_fixed_flag ? TRUE : FALSE);
		if(v == NULL)
			return 1;
		for(i = 0; v[i].letter != 0; i++){
			if(stop_flag) break;
			//printf("%c:\n",v[i].letter);
			letter = v[i].letter;
			(void)process_single_volume();
		}
		udefrag_release_vollist(v);
	}
	
	return 0;
}

int check_for_obsolete_options(void)
{
	if(obsolete_option){
		display_error("The -i, -e, -s, -d options are oblolete.\n"
					  "Use environment variables instead!\n"
					  );
		return 1;
	} else return 0;
}

void display_copyright(void)
{
	printf(VERSIONINTITLE ", "
		   "Copyright (c) Dmitri Arkhangelski, 2007-2010.\n"
		   "UltraDefrag comes with ABSOLUTELY NO WARRANTY. This is free software, \n"
		   "and you are welcome to redistribute it under certain conditions.\n\n");
}

int __cdecl main(int argc, char **argv)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	int error_code;

	/* analyse command line */
	parse_cmdline(argc,argv);

	/* prepare console */
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if(GetConsoleScreenBufferInfo(hOut,&csbi))
		console_attr = csbi.wAttributes;
	if(!b_flag)	settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	
	/* collect statistics about the UltraDefrag command line client use */
#ifndef _WIN64
	IncreaseGoogleAnalyticsCounterAsynch("ultradefrag.sourceforge.net","/appstat/console-x86.html","UA-15890458-1");
#else
	#if defined(_IA64_)
		IncreaseGoogleAnalyticsCounterAsynch("ultradefrag.sourceforge.net","/appstat/console-ia64.html","UA-15890458-1");
	#else
		IncreaseGoogleAnalyticsCounterAsynch("ultradefrag.sourceforge.net","/appstat/console-x64.html","UA-15890458-1");
	#endif
#endif

	/* handle help request */
	if(h_flag){
		show_help();
		error_code = 0; goto done;
	}

	/* display copyright */
	display_copyright();

	/* handle obsolete options */
	if(check_for_obsolete_options()){
		error_code = 1; goto done;
	}

	/* show list of volumes if requested */
	if(l_flag){
		error_code = show_vollist(); goto done;
	}
	
	if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,TRUE))
		DisplayLastError("Cannot set Ctrl + C handler!");

	/* process multiple volumes if requested */
	if(letters[1] || all_flag || all_fixed_flag)
		error_code = process_multiple_volumes();
	else
		error_code = process_single_volume();

	(void)SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);

done:
	if(!b_flag) settextcolor(console_attr);
	return error_code;
}
