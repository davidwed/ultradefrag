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
char unk_opt[] = "Unknown option: x!";
int unknown_option = 0;

#define BLOCKS_PER_HLINE  68//60//79
#define BLOCKS_PER_VLINE  10//8//16
#define N_BLOCKS          (BLOCKS_PER_HLINE * BLOCKS_PER_VLINE)

/* internal functions prototypes */
void HandleError(char *err_msg,int exit_code);
BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType);
void __stdcall ErrorHandler(short *msg);

void Exit(int exit_code)
{
	if(!b_flag) settextcolor(console_attr);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
	udefrag_unload();
	exit(exit_code);
}

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
		"  -b  use default color scheme\n"
		"  -m  show cluster map\n"
		"  -p  suppress progress indicator\n"
		"  -v  show volume information after a job\n"
		);
	Exit(0);
}

void HandleError(char *err_msg,int exit_code)
{
	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
	printf("%s\n",err_msg);
	Exit(exit_code);
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
	udefrag_set_error_handler(NULL);

	if(udefrag_get_avail_volumes(&v,la_flag ? FALSE : TRUE) < 0){
		if(!b_flag) settextcolor(console_attr);
		udefrag_set_error_handler(ErrorHandler);
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
	udefrag_set_error_handler(ErrorHandler);
	return 0;
}

BOOL stop_flag = FALSE;

BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
{
	stop_flag = TRUE;
	udefrag_stop();
	return TRUE;
}

void __stdcall ErrorHandler(short *msg)
{
	char oem_buffer[1024]; /* see zenwinx.h ERR_MSG_SIZE */
	WORD color = FOREGROUND_RED | FOREGROUND_INTENSITY;

	/* ignore notifications */
	if((short *)wcsstr(msg,L"N: ") == msg) return;
	
	if(!b_flag){
		if((short *)wcsstr(msg,L"W: ") == msg)
			color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		settextcolor(color);
	}
	/* skip W: and E: labels */
	if((short *)wcsstr(msg,L"W: ") == msg || (short *)wcsstr(msg,L"E: ") == msg)
		WideCharToMultiByte(CP_OEMCP,0,msg + 3,-1,oem_buffer,1023,NULL,NULL);
	else
		WideCharToMultiByte(CP_OEMCP,0,msg,-1,oem_buffer,1023,NULL,NULL);
	printf("%s\n",oem_buffer);
	if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

BOOL first_run = TRUE;
BOOL err_flag = FALSE;
BOOL err_flag2 = FALSE;
//char last_op = 0;
char cluster_map[N_BLOCKS] = {};

WORD colors[NUM_OF_SPACE_STATES] = {
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	FOREGROUND_GREEN,
	FOREGROUND_RED | FOREGROUND_INTENSITY,
	FOREGROUND_RED,
	FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_BLUE,
	FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	FOREGROUND_RED | FOREGROUND_GREEN,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	FOREGROUND_RED | FOREGROUND_GREEN,
	FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
};

WORD bcolors[NUM_OF_SPACE_STATES] = {
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	BACKGROUND_GREEN | BACKGROUND_INTENSITY,
	BACKGROUND_GREEN,
	BACKGROUND_RED | BACKGROUND_INTENSITY,
	BACKGROUND_RED,
	BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	BACKGROUND_BLUE,
	BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_INTENSITY,
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY,
	BACKGROUND_RED | BACKGROUND_GREEN,
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY,
	BACKGROUND_RED | BACKGROUND_GREEN,
	BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
};

#define MAP_CHAR '@'
BOOL map_completed = FALSE;

#define BORDER_COLOR (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)

void RedrawMap(void)
{
	int i,j;
	WORD color, prev_color = 0x0;
	char c[2];
	WORD border_color = BORDER_COLOR;

	fprintf(stderr,"\n\n");
	
	settextcolor(border_color);
	prev_color = border_color;
	c[0] = 0xC9; c[1] = 0;
	fprintf(stderr,c);
	for(j = 0; j < BLOCKS_PER_HLINE; j++){
		c[0] = 0xCD; c[1] = 0;
		fprintf(stderr,c);
	}
	c[0] = 0xBB; c[1] = 0;
	fprintf(stderr,c);
	fprintf(stderr,"\n");

	for(i = 0; i < BLOCKS_PER_VLINE; i++){
		if(border_color != prev_color) settextcolor(border_color);
		prev_color = border_color;
		c[0] = 0xBA; c[1] = 0;
		fprintf(stderr,c);
		for(j = 0; j < BLOCKS_PER_HLINE; j++){
			color = colors[(int)cluster_map[i * BLOCKS_PER_HLINE + j]];
			if(color != prev_color) settextcolor(color);
			prev_color = color;
			c[0] = '*'; c[1] = 0;
			fprintf(stderr,c);
		}
		if(border_color != prev_color) settextcolor(border_color);
		prev_color = border_color;
		c[0] = 0xBA; c[1] = 0;
		fprintf(stderr,c);
		fprintf(stderr,"\n");
	}

	if(border_color != prev_color) settextcolor(border_color);
	prev_color = border_color;
	c[0] = 0xC8; c[1] = 0;
	fprintf(stderr,c);
	for(j = 0; j < BLOCKS_PER_HLINE; j++){
		c[0] = 0xCD; c[1] = 0;
		fprintf(stderr,c);
	}
	c[0] = 0xBC; c[1] = 0;
	fprintf(stderr,c);
	fprintf(stderr,"\n");

	fprintf(stderr,"\n");
	if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	else settextcolor(console_attr);
	map_completed = TRUE;
}

int __stdcall ProgressCallback(int done_flag)
{
	STATISTIC stat;
	char op; char *op_name/*, *last_op_name*/;
	double percentage;
	ERRORHANDLERPROC eh = NULL;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD cursor_pos;
	
	if(err_flag || err_flag2) return 0;
	
	if(first_run){
		/*
		* Ultra fast ntfs analysis contains one piece of code 
		* that heavily loads CPU for one-two seconds.
		*/
		SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
		first_run = FALSE;
	}
	
	if(map_completed){
		if(!GetConsoleScreenBufferInfo(hOut,&csbi)){
			if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
			printf("\nCannot retrieve cursor position!\n");
			if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			return 0;
		}
		cursor_pos.X = 0;
		cursor_pos.Y = csbi.dwCursorPosition.Y - BLOCKS_PER_VLINE - 3 - 2;
		if(!SetConsoleCursorPosition(hOut,cursor_pos)){
			if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
			printf("\nCannot set cursor position!\n");
			if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			return 0;
		}
	}
	
	/* show error message no more than once */
	if(err_flag) eh = udefrag_set_error_handler(NULL);
	if(udefrag_get_progress(&stat,&percentage) >= 0){
		op = stat.current_operation;
		if(op == 'A' || op == 'a')      op_name = "analyse:  ";
		else if(op == 'D' || op == 'd') op_name = "defrag:   ";
		else                            op_name = "optimize: ";

		//if(last_op == 'A' || last_op == 'a')      last_op_name = "Analyse:  ";
		//else if(last_op == 'D' || last_op == 'd') last_op_name = "Defrag:   ";
		//else                                      last_op_name = "Optimize: ";

		//if(op != 'A' && !last_op) fprintf(stderr,"\rAnalyse:  100%%\n");
		//if(op != last_op && last_op) fprintf(stderr,"\r%s100%%\n",last_op_name);
		//last_op = op;
		fprintf(stderr,"\r%c: %s%3u%% complete, fragmented/total = %lu/%lu",
			letter,op_name,(int)percentage,stat.fragmfilecounter,stat.filecounter);
	} else {
		err_flag = TRUE;
	}
	if(eh) udefrag_set_error_handler(eh);
	
	if(err_flag) return 0;

	if(done_flag && !stop_flag){ /* set progress indicator to 100% state */
		fprintf(stderr,"\r%c: %s100%% complete, fragmented/total = %lu/%lu",
			letter,op_name,stat.fragmfilecounter,stat.filecounter);
		if(!m_flag) fprintf(stderr,"\n");
	}
	
	if(m_flag){ /* display cluster map */
		/* show error message no more than once */
		if(err_flag2) eh = udefrag_set_error_handler(NULL);
		if(udefrag_get_map(cluster_map,N_BLOCKS) >= 0){
			RedrawMap();
		} else {
			err_flag2 = TRUE;
		}
		if(eh) udefrag_set_error_handler(eh);
	}
	
	return 0;
}

void parse_cmdline(int argc, char **argv)
{
	int i;
	char c1,c2,c3;

	if(argc < 2) h_flag = 1;
	for(i = 1; i < argc; i++){
		c1 = argv[i][0];
		if(!c1) continue;
		c2 = argv[i][1];
		/* next check supports UltraDefrag context menu handler */
		if(c2 == ':') letter = (char)c1;
		if(c1 == '-'){
			c2 = (char)tolower((int)c2);
			if(!strchr("abosideh?lpvm",c2)){
				/* unknown option */
				unk_opt[16] = c2;
				unknown_option = 1;
			}
			if(strchr("iesd",c2))
				obsolete_option = 1;
			if(c2 == 'h' || c2 == '?')
				h_flag = 1;
			else if(c2 == 'a') a_flag = 1;
			else if(c2 == 'o') o_flag = 1;
			else if(c2 == 'b') b_flag = 1;
			else if(c2 == 'l'){
				l_flag = 1;
				c3 = (char)tolower((int)argv[i][2]);
				if(c3 == 'a') la_flag = 1;
			}
			else if(c2 == 'p') p_flag = 1;
			else if(c2 == 'v') v_flag = 1;
			else if(c2 == 'm') m_flag = 1;
		}
	}
	/* if only -b or -p options are specified, show help message */
	//if(argc == 2 && b_flag) h_flag = 1;
	if(!l_flag && !letter) h_flag = 1;
}

int __cdecl main(int argc, char **argv)
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
	printf(VERSIONINTITLE ", " //"console interface\n"
			"Copyright (c) Dmitri Arkhangelski, 2007-2009.\n"
			"UltraDefrag comes with ABSOLUTELY NO WARRANTY. This is free software, \n"
			"and you are welcome to redistribute it under certain conditions.\n\n");
	/* handle unknown option and help request */
	if(unknown_option) HandleError(unk_opt,1);
	if(obsolete_option)
		HandleError("The -i, -e, -s, -d options are oblolete.\n"
					"Use environment variables instead!",1);
	if(h_flag) show_help();

	/* set udefrag.dll ErrorHandler */
	udefrag_set_error_handler(ErrorHandler);
	
	/* show list of volumes if requested */
	if(l_flag){ exit(show_vollist()); }
	/* validate driveletter */
	if(!letter)	HandleError("Drive letter should be specified!",1);
	if(udefrag_validate_volume(letter,FALSE) < 0) Exit(1);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,TRUE);
	/* do our job */
	if(!m_flag){
		if(udefrag_init(0) < 0) Exit(2);
	} else {
		if(udefrag_init(N_BLOCKS) < 0) Exit(2);
	}

	if(m_flag){ /* prepare console buffer for map */
		fprintf(stderr,"\r%c: %s%3u%% complete, fragmented/total = %u/%u",
			letter,"analyse:  ",0,0,0);
		RedrawMap();
	}
	
	if(!p_flag){
		if(a_flag) {
			//printf("Preparing to analyse volume %c: ...\n\n",letter);
			if(udefrag_analyse(letter,ProgressCallback) < 0) Exit(3);
		}
		else if(o_flag) {
			//printf("Preparing to optimize volume %c: ...\n\n",letter);
			if(udefrag_optimize(letter,ProgressCallback) < 0) Exit(3);
		}
		else { 
			//printf("Preparing to defragment volume %c: ...\n\n",letter);
			if(udefrag_defragment(letter,ProgressCallback) < 0) Exit(3);
		}
	} else {
		if(a_flag) { if(udefrag_analyse(letter,NULL) < 0) Exit(3); }
		else if(o_flag) { if(udefrag_optimize(letter,NULL) < 0) Exit(3); }
		else { if(udefrag_defragment(letter,NULL) < 0) Exit(3); }
	}

	/* display results and exit */
	if(v_flag){
		if(udefrag_get_progress(&stat,NULL) >= 0)
			printf("\n%s",udefrag_get_default_formatted_results(&stat));
	}
	Exit(0);
	return 0; /* we will never reach this point */
}
