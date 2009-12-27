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
int p_flag = 0,v_flag = 0;
int m_flag = 0;
int obsolete_option = 0;
char letter = 0;
int screensaver_mode = 0;

BOOL stop_flag = FALSE;

extern int map_rows;
extern int map_symbols_per_line;

void parse_cmdline(int argc, char **argv);
void show_help(void);
void AllocateClusterMap(void);
void FreeClusterMap(void);
void RedrawMap(void);
int __stdcall ProgressCallback(int done_flag);

BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType);
void display_error(char *string);

/* prints specified string in red, than restores green/default color */
void display_error(char *string)
{
	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
	printf("%s",string);
	if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

/* returns an exit code for console program terminating */
int show_vollist(void)
{
	volume_info *v;
	int n;
	char s[32];
	double d;
	int p;

	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf("Volumes available for defragmentation:\n\n");

	if(udefrag_get_avail_volumes(&v,la_flag ? FALSE : TRUE) < 0){
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
			if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			printf("%c:  %8s %12s %8u %%\n",
				v[n].letter,v[n].fsname,s,p);
		}
	}

	if(!b_flag) settextcolor(console_attr);
	return 0;
}

BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
{
	stop_flag = TRUE;
	if(udefrag_stop() < 0){
		display_error("\nDefragmentation cannot be stopped!\n\n"
					  "Some unknown internal bug has been encountered.\n"
					  "You can kill ultradefrag.exe process in task manager\n"
					  "(press Ctrl + Alt + Delete to launch it).\n"
					  "Run DbgView program for more information.\n\n"
					  );
	}
	return TRUE;
}

void cleanup(void)
{
	udefrag_unload();
	FreeClusterMap();
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
	if(!b_flag) settextcolor(console_attr);
}

void RunScreenSaver(void)
{
	printf("Hello!\n");
}

int __cdecl main(int argc, char **argv)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	STATISTIC stat;
	STATUPDATEPROC stat_callback = NULL;
	long map_size = 0;
	int job_error = 0;

	/* analyse command line */
	parse_cmdline(argc,argv);

	/* prepare console */
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if(GetConsoleScreenBufferInfo(hOut,&csbi))
		console_attr = csbi.wAttributes;
	if(!b_flag)	settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	
	/* handle help request */
	if(h_flag){
		show_help();
		if(!b_flag) settextcolor(console_attr);
		return 0;
	}

	/* display copyright */
	printf(VERSIONINTITLE ", "
		   "Copyright (c) Dmitri Arkhangelski, 2007-2009.\n"
		   "UltraDefrag comes with ABSOLUTELY NO WARRANTY. This is free software, \n"
		   "and you are welcome to redistribute it under certain conditions.\n\n");

	/* handle obsolete options */
	if(obsolete_option){
		display_error("The -i, -e, -s, -d options are oblolete.\n"
					  "Use environment variables instead!\n"
					  );
		if(!b_flag) settextcolor(console_attr);
		return 1;
	}

	/* show list of volumes if requested */
	if(l_flag)
		return show_vollist();

	/* validate driveletter */
	if(!letter){
		display_error("Drive letter should be specified!\n");
		if(!b_flag) settextcolor(console_attr);
		return 1;
	}
	if(udefrag_validate_volume(letter,FALSE) < 0){
		display_error("The specified volume cannot be processed.\n\n"
					  "Possible reasons are:\n"
					  " - remote/cdrom/missing volume specified\n"
					  " - volume letter is assigned by \'subst\' command\n\n"
					  "Run DbgView program for more information.\n"
					  );
		if(!b_flag) settextcolor(console_attr);
		return 1;
	}

	/* allocate memory for cluster map */
	if(m_flag) AllocateClusterMap();
	
	/* check if we need to run in screensaver mode */
	if(screensaver_mode){
		RunScreenSaver();
		FreeClusterMap();
		if(!b_flag) settextcolor(console_attr);
		return 0;
	}

	/* do our job */
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,TRUE);
	if(m_flag) map_size = map_rows * map_symbols_per_line;
	if(udefrag_init(map_size) < 0){
		display_error("\nInitialization failed!\n\n"
					  "Another instance of the program is running\n"
					  "or unknown internal bug encountered.\n"
					  "DbgView program could help you to recognize.\n\n"
					  );
		cleanup();
		return 2;
	}

	if(udefrag_kernel_mode()) printf("(Kernel Mode)\n\n");
	else printf("(User Mode)\n\n");

	if(m_flag){ /* prepare console buffer for map */
		fprintf(stderr,"\r%c: %s%3u%% complete, fragmented/total = %u/%u",
			letter,"analyse:  ",0,0,0);
		RedrawMap();
	}
	
	if(!p_flag) stat_callback = ProgressCallback;
	if(a_flag) { if(udefrag_analyse(letter,stat_callback) < 0) job_error = 1; }
	else if(o_flag) { if(udefrag_optimize(letter,stat_callback) < 0) job_error = 1; }
	else { if(udefrag_defragment(letter,stat_callback) < 0) job_error = 1; }
	if(job_error){
		display_error("\nAnalysis/Defragmentation failure!\n\n"
					  "System disallows it or unknown internal bug encountered.\n"
					  "DbgView and ChkDsk programs could help you to recognize.\n\n"
					  );
		cleanup();
		return 3;
	}

	/* display results and exit */
	if(v_flag){
		if(udefrag_get_progress(&stat,NULL) >= 0)
			printf("\n%s",udefrag_get_default_formatted_results(&stat));
	}

	cleanup();
	return 0;
}
