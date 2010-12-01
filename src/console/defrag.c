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

#include "udefrag.h"

/* global variables */
HANDLE hSynchEvent = NULL;
HANDLE hStdOut; /* handle of the standard output */
WORD default_color = 0x7; /* default text color */

int a_flag = 0,o_flag = 0;
int b_flag = 0,h_flag = 0;
int l_flag = 0,la_flag = 0;
int p_flag = 0,v_flag = 0;
int m_flag = 0;
int obsolete_option = 0;
int screensaver_mode = 0;
int all_flag = 0;
int all_fixed_flag = 0;
object_path *paths = NULL;
char letters[MAX_DOS_DRIVES] = {0};
int wait_flag = 0;
int shellex_flag = 0;

int first_progress_update;
int stop_flag = 0;

#define MAX_ENV_VARIABLE_LENGTH 32766
wchar_t in_filter[MAX_ENV_VARIABLE_LENGTH + 1];
wchar_t new_in_filter[MAX_ENV_VARIABLE_LENGTH + 1];
wchar_t aux_buffer[MAX_ENV_VARIABLE_LENGTH + 1];

/* forward declarations */
static void update_web_statistics(void);
static int show_vollist(void);
static int process_volumes(void);
BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType);

/* -------------------------------------------- */
/*                common routines               */
/* -------------------------------------------- */

/**
 * @brief Fills the current line with spaces.
 */
void clear_line(FILE *f)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD cursor_pos;
	char *line;
	int n;
	
	if(!GetConsoleScreenBufferInfo(hStdOut,&csbi))
		return; /* impossible to determine the screen width */
	n = (int)csbi.dwSize.X;
	line = malloc(n + 1);
	if(line == NULL)
		return;
	
	memset(line,0x20,n);
	line[n] = 0;
	fprintf(f,"\r%s",line);
	free(line);
	
	/* move cursor back to the previous line */
	if(!GetConsoleScreenBufferInfo(hStdOut,&csbi)){
		display_last_error("Cannot retrieve cursor position!");
		return; /* impossible to determine the current cursor position  */
	}
	cursor_pos.X = 0;
	cursor_pos.Y = csbi.dwCursorPosition.Y - 1;
	if(!SetConsoleCursorPosition(hStdOut,cursor_pos))
		display_last_error("Cannot set cursor position!");
}

/* -------------------------------------------- */
/*         UltraDefrag specific routines        */
/* -------------------------------------------- */

/**
 * @brief Performs basic console initialization.
 */
static void init_console(void)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	/* save default color */
	if(GetConsoleScreenBufferInfo(hStdOut,&csbi))
		default_color = csbi.wAttributes;
	/* set green color */
	if(!b_flag)	settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

/**
 * @brief Terminates the program.
 * @details Restores default text color.
 */
static void terminate_console(int code)
{
	/* restore default color */
	if(!b_flag) settextcolor(default_color);
	exit(code);
}

/**
 * @brief Handles Ctrl+C keys.
 */
BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
{
	stop_flag = 1;
	return TRUE;
}

/**
 * @brief Prints the string in red, than restores green color.
 */
void display_error(char *string)
{
	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
	printf("%s",string);
	if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

/**
 * @brief Displays error message
 * specific for failed disk processing tasks.
 */
static void display_defrag_error(int error_code)
{
	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
	printf("\nAnalysis/Defragmentation failed!\n\n");
	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf("%s\n\n",udefrag_get_error_description(error_code));
	if(error_code == UDEFRAG_UNKNOWN_ERROR) printf("Use DbgView program to get more information.\n\n");
	if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

/**
 * @brief Displays error message
 * specific for invalid disk volumes.
 */
static void display_invalid_volume_error(int error_code)
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

/**
 * @brief Displays last error message in red.
 */
void display_last_error(char *caption)
{
	LPVOID lpMsgBuf;
	DWORD error = GetLastError();

	if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,0,NULL)){
				if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
				printf("\n%s\nError code = 0x%x\n\n",caption,(UINT)error);
				if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	} else {
		if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
		printf("\n%s\n%s\n",caption,(char *)lpMsgBuf);
		if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		LocalFree(lpMsgBuf);
	}
}

/**
* @brief Synchronizes with other scheduled 
* jobs when running with --wait option.
*/
void begin_synchronization(void)
{
	/* create event */
	do {
		hSynchEvent = CreateEvent(NULL,FALSE,TRUE,"udefrag-exe-synch-event");
		if(hSynchEvent == NULL){
			display_last_error("Cannot create udefrag-exe-synch-event event!");
			return;
		}
		if(GetLastError() == ERROR_ALREADY_EXISTS && wait_flag){
			CloseHandle(hSynchEvent);
			Sleep(1000);
			continue;
		}
		return;
	} while (1);
}

void end_synchronization(void)
{
	/* release event */
	if(hSynchEvent)
		CloseHandle(hSynchEvent);
}

/*DWORD WINAPI test_thread(LPVOID p)
{
	udefrag_start_job('c',ANALYSIS_JOB,0,NULL,NULL,NULL);
	return 0;
}

void test(void)
{
	int i;
	DWORD id;
	
	for(i = 0; i < 10; i++)
		CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)test_thread,NULL,0,&id);
}
*/

/**
 * @brief Command line tool entry point.
 */
int __cdecl main(int argc, char **argv)
{
	int result, pause_result;
	
	/*test();
	getch();
	return 0;
	*/

    (void)SetEnvironmentVariableW(L"UD_LOG_SUFFIX",L"_console");
	
	/* initialize the program */
	parse_cmdline(argc,argv);
	init_console();
	update_web_statistics();
	
	/* handle help request */
	if(h_flag){
		show_help();
		terminate_console(0);
	}

	printf(VERSIONINTITLE ", Copyright (c) Dmitri Arkhangelski, 2007-2010.\n"
		"UltraDefrag comes with ABSOLUTELY NO WARRANTY. This is free software, \n"
		"and you are welcome to redistribute it under certain conditions.\n\n"
		);

	/* handle obsolete options */
	if(obsolete_option){
		display_error("The -i, -e, -s, -d options are oblolete.\n"
			"Use environment variables instead!\n");
		terminate_console(1);
	}

	/* show list of volumes if requested */
	if(l_flag) terminate_console(show_vollist());
	
	/* run disk defragmentation job */
	if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,TRUE))
		display_last_error("Cannot set Ctrl + C handler!");
	
	begin_synchronization();
	
	/* uncomment for the --wait option testing */
	//printf("the job gets running\n");
	//_getch();
	result = process_volumes();
	
	end_synchronization();
	
	(void)SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
	
	/* display prompt to hit any key in case of context menu handler */
	if(shellex_flag){
		if(!b_flag) settextcolor(default_color);
		printf("\n");
		pause_result = system("pause");
		if(pause_result > 0){
			/* command not found */
			printf("\n");
		}
		if(pause_result != 0){
			/* command or a command interpreter itself not found */
			printf("Hit any key to continue...");
			_getch();
		}
	}

	destroy_paths();
	terminate_console(result);
	return 0;
}

/**
 * @brief Updates progress information on the screen.
 */
void __stdcall update_progress(udefrag_progress_info *pi, void *p)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD cursor_pos;
	char volume_letter;
	char op;
	char *op_name = "";
	char *results;
	
	volume_letter = (char)(DWORD_PTR)p;
    
	if(!p_flag){
		if(first_progress_update){
			/*
			* Ultra fast ntfs analysis contains one piece of code 
			* that heavily loads CPU for one-two seconds.
			*/
			(void)SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
			first_progress_update = 0;
		}
		
		if(map_completed){ /* we could use simple check for m_flag here */
			if(!GetConsoleScreenBufferInfo(hStdOut,&csbi)){
				display_last_error("Cannot retrieve cursor position!");
				return;
			}
			cursor_pos.X = 0;
			cursor_pos.Y = csbi.dwCursorPosition.Y - map_rows - 3 - 2;
			if(!SetConsoleCursorPosition(hStdOut,cursor_pos)){
				display_last_error("Cannot set cursor position!");
				return;
			}
		}
		
		op = udefrag_tolower(pi->current_operation);
		if(op == 'a')
			op_name = "analyze:  ";
		else if(op == 'd')
			op_name = "defrag:   ";
		else
			op_name = "optimize: ";
		clear_line(stderr);
		fprintf(stderr,"\r%c: %s%6.2lf%% complete, fragmented/total = %lu/%lu",
			volume_letter,op_name,pi->percentage,pi->fragmented,pi->files);
		
		if(pi->completion_status != 0 && !stop_flag){
			/* set progress indicator to 100% state */
			clear_line(stderr);
			fprintf(stderr,"\r%c: %s100.00%% complete, fragmented/total = %lu/%lu",
				volume_letter,op_name,pi->fragmented,pi->files);
			if(!m_flag) fprintf(stderr,"\n");
		}
		
		if(m_flag)
			RedrawMap(pi);
	}
	
	if(pi->completion_status != 0 && v_flag){
		/* print results of the completed job */
		results = udefrag_get_default_formatted_results(pi);
		if(results){
			printf("\n%s",results);
			udefrag_release_default_formatted_results(results);
		}
	}
}

/**
 * @brief Callback routine
 * calling each time when
 * the volume processing routines
 * would like to know whether they
 * should be terminated or not.
 */
int __stdcall terminator(void *p)
{
	/* do it as quickly as possible :-) */
	return stop_flag;
}

/**
 * @brief A stub.
 */
static void RunScreenSaver(void)
{
	printf("Hello!\n");
}

/**
 * @brief Processes a single volume.
 * @return Zero for success, nonzero value indicates failure.
 */
static int process_single_volume(char volume_letter)
{
	long map_size = 0;
	udefrag_job_type job_type = DEFRAG_JOB;
	int result;

	/* validate volume letter */
	if(volume_letter == 0){
		display_error("Drive letter should be specified!\n");
		return 3;
	}
	result = udefrag_validate_volume(volume_letter,FALSE);
	if(result < 0){
		display_invalid_volume_error(result);
		return 3;
	}

	/* initialize cluster map */
	if(m_flag){
		if(AllocateClusterMap() < 0){
			/* terminate process */
			(void)SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
			terminate_console(4);
		}
		InitializeMapDisplay(volume_letter);
		map_size = map_rows * map_symbols_per_line;
	}
	
	/* check if we need to run in screensaver mode */
	if(screensaver_mode){
		RunScreenSaver();
		FreeClusterMap();
		return 0;
	}

	/* run the job */
	stop_flag = 0;
	if(a_flag) job_type = ANALYSIS_JOB;
	else if(o_flag) job_type = OPTIMIZER_JOB;
	first_progress_update = 1;
	result = udefrag_start_job(volume_letter,job_type,map_size,
		update_progress,terminator,(void *)(DWORD_PTR)volume_letter);
	if(result < 0){
		display_defrag_error(result);
		FreeClusterMap();
		return 5;
	}

	FreeClusterMap();
	return 0;
}

/**
 * @brief Processes all volumes specified on the command line.
 * @return Zero for success, nonzero value indicates failure.
 */
static int process_volumes(void)
{
	object_path *path, *another_path;
	int single_path = 0;
	volume_info *v;
	int i, result = 0;
	char letter;
	int n, path_found;
	int first_path = 1;
	
	/* 1. process paths specified on the command line */
	/* skip invalid paths */
	for(path = paths; path; path = path->next){
		if(wcslen(path->path) < 2){
			printf("incomplete path detected: %ls\n",path->path);
			path->processed = 1;
		}
		if(path->path[1] != ':'){
			printf("incomplete path detected: %ls\n",path->path);
			path->processed = 1;
		}
		if(path->next == paths) break;
	}
	/* process valid paths */
	for(path = paths; path; path = path->next){
		if(path->processed == 0){
			if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			if(first_path)
				printf("%ls\n",path->path);
			else
				printf("\n%ls\n",path->path);
			if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			first_path = 0;
			path->processed = 1;
			path_found = 1;
			
			/* extract drive letter */
			letter = (char)path->path[0];
			
			/* save %UD_IN_FILTER% */
			in_filter[0] = 0;
			if(!GetEnvironmentVariableW(L"UD_IN_FILTER",in_filter,MAX_ENV_VARIABLE_LENGTH + 1)){
				if(GetLastError() != ERROR_ENVVAR_NOT_FOUND)
					display_last_error("process_volumes: cannot get %%UD_IN_FILTER%%!");
			}
			
			/* save the current path to %UD_IN_FILTER% */
			n = _snwprintf(new_in_filter,MAX_ENV_VARIABLE_LENGTH + 1,L"%ls",path->path);
			if(n < 0){
				display_error("process_volumes: cannot set %%UD_IN_FILTER%% - path is too long!");
				wcscpy(new_in_filter,in_filter);
				path_found = 0;
			} else {
				new_in_filter[MAX_ENV_VARIABLE_LENGTH] = 0;
			}
			
			/* search for another paths with the same drive letter */
			for(another_path = path->next; another_path; another_path = another_path->next){
				if(another_path == paths) break;
				if(udefrag_toupper(letter) == udefrag_toupper((char)another_path->path[0])){
					/* try to append it to %UD_IN_FILTER% */
					n = _snwprintf(aux_buffer,MAX_ENV_VARIABLE_LENGTH + 1,L"%ls;%ls",new_in_filter,another_path->path);
					if(n >= 0){
						aux_buffer[MAX_ENV_VARIABLE_LENGTH] = 0;
						wcscpy(new_in_filter,aux_buffer);
						path_found = 1;
						if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
						printf("%ls\n",another_path->path);
						if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
						another_path->processed = 1;
					}
				}
			}
			
			/* set %UD_IN_FILTER% */
			if(stop_flag) return 0;
			if(!SetEnvironmentVariableW(L"UD_IN_FILTER",new_in_filter)){
				display_last_error("process_volumes: cannot set %%UD_IN_FILTER%%!");
			}
			
			/* run the job */
			if(path_found)
				result = process_single_volume(letter);
			
			/* restore %UD_IN_FILTER% */
			if(!SetEnvironmentVariableW(L"UD_IN_FILTER",in_filter)){
				display_last_error("process_volumes: cannot restore %%UD_IN_FILTER%%!");
			}
		}
		if(path->next == paths) break;
	}
	
	/* if the job was stopped return success */
	if(stop_flag)
		return 0;
	
	/* in case of a single path processing return result */
	if(paths){
		if(paths->next == paths)
			single_path = 1;
	}
	if(single_path && letters[0] == 0 && !all_flag && !all_fixed_flag)
		return result;
	
	/* 2. process individual volumes specified on the command line */
	for(i = 0; i < MAX_DOS_DRIVES; i++){
		if(letters[i] == 0) break;
		if(stop_flag) return 0;
		//printf("%c:\n",letters[i]);
		result = process_single_volume(letters[i]);
	}
	
	/* if the job was stopped return success */
	if(stop_flag)
		return 0;
	
	/* in case of a single volume processing return result */
	if(letters[1] == 0 && !all_flag && !all_fixed_flag)
		return result;
	
	/* 3. handle --all and --all-fixed options */
	if(all_flag || all_fixed_flag){
		v = udefrag_get_vollist(all_fixed_flag ? TRUE : FALSE);
		if(v == NULL)
			return 1;
		for(i = 0; v[i].letter != 0; i++){
			if(stop_flag) break;
			//printf("%c:\n",v[i].letter);
			(void)process_single_volume(v[i].letter);
		}
		udefrag_release_vollist(v);
	}
	return 0;
}

/**
 * @brief Displays list of disk volumes
 * available for defragmentation.
 * @return Zero for success, nonzero
 * value indicates failure.
 */
static int show_vollist(void)
{
	volume_info *v;
	int i, percent;
	char s[32];
	double d;

	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf("Volumes available for defragmentation:\n\n");

	v = udefrag_get_vollist(la_flag ? FALSE : TRUE);
	if(v == NULL){
		if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		return 2;
	}

	for(i = 0; v[i].letter != 0; i++){
		udefrag_fbsize((ULONGLONG)(v[i].total_space.QuadPart),2,s,sizeof(s));
		d = (double)(signed __int64)(v[i].free_space.QuadPart);
		/* 0.1 constant is used to exclude divide by zero error */
		d /= ((double)(signed __int64)(v[i].total_space.QuadPart) + 0.1);
		percent = (int)(100 * d);
		if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		printf("%c:  %8s %12s %8u %%\n",
			v[i].letter,v[i].fsname,s,percent);
	}
	udefrag_release_vollist(v);
	if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	return 0;
}

/**
 * @brief Updates web statistics of the program use.
 */
static void update_web_statistics(void)
{
#ifndef _WIN64
	IncreaseGoogleAnalyticsCounterAsynch("ultradefrag.sourceforge.net","/appstat/console-x86.html","UA-15890458-1");
#else
	#if defined(_IA64_)
		IncreaseGoogleAnalyticsCounterAsynch("ultradefrag.sourceforge.net","/appstat/console-ia64.html","UA-15890458-1");
	#else
		IncreaseGoogleAnalyticsCounterAsynch("ultradefrag.sourceforge.net","/appstat/console-x64.html","UA-15890458-1");
	#endif
#endif
}
