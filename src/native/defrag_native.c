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
* UltraDefrag native interface.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/ntndk.h"
#include "../include/udefrag.h"
#include "../include/ultradfgver.h"
#include "../dll/zenwinx/zenwinx.h"

#define USE_INSTEAD_SMSS  0

char letter;
int abort_flag = 0;
char last_op = 0;
int i = 0,j = 0,k; /* number of '-' */
char buffer[256];
int udefrag_initialized = FALSE;

short file_buffer[32768];
short line_buffer[32768];

#define USAGE  "Supported commands:\n" \
		"  analyse X:\n" \
		"  defrag X:\n" \
		"  optimize X:\n" \
		"  batch\n" \
		"  drives\n" \
		"  exit\n" \
		"  reboot\n" \
		"  shutdown\n" \
		"  help\n"

void Cleanup();

void HandleError(short *err_msg,int exit_code)
{
	if(err_msg){
		if(err_msg[0]) winx_printf("\nERROR: %ws\n",err_msg);
		Cleanup();
		if(exit_code){
			winx_printf("Wait 10 seconds ...\n");
			winx_sleep(10000); /* show error message at least 5 seconds */
		}
		winx_printf("Good bye ...\n");
		winx_exit(exit_code);
	}
}

void HandleError_(int status,int exit_code)
{
	char err_msg[ERR_MSG_SIZE];

	if(status < 0){
		udefrag_pop_error(err_msg,ERR_MSG_SIZE);
		winx_printf("\nERROR: %s\n",err_msg);
		Cleanup();
		if(exit_code){
			winx_printf("Wait 10 seconds ...\n");
			winx_sleep(10000); /* show error message at least 5 seconds */
		}
		winx_printf("Good bye ...\n");
		winx_exit(exit_code);
	}
}

void UpdateProgress()
{
	STATISTIC stat;
	double percentage;

	if(udefrag_get_progress(&stat,&percentage) == 0){
		if(stat.current_operation != last_op){
			if(last_op){
				for(k = i; k < 50; k++)
					winx_putch('-'); /* 100 % of previous operation */
			} else {
				if(stat.current_operation != 'A'){
					winx_printf("\nA: ");
					for(k = 0; k < 50; k++)
						winx_putch('-');
				}
			}
			i = 0; /* new operation */
			//winx_printf("\n%c: ",stat.current_operation);
			switch(stat.current_operation){
			case 'a':
			case 'A':
				winx_printf("\nAnalyse : ");
				break;
			case 'd':
			case 'D':
				winx_printf("\nDefrag  : ");
				break;
			case 'c':
			case 'C':
				winx_printf("\nOptimize: ");
				break;
			}
			last_op = stat.current_operation;
		}
		j = (int)percentage / 2;
		for(k = i; k < j; k++)
			winx_putch('-');
		i = j;
	} else {
		udefrag_pop_error(NULL,0);
	}
}

int __stdcall update_stat(int df)
{
	char err_msg[ERR_MSG_SIZE];
	char *msg;

	if(winx_breakhit(100) >= 0){
		if(udefrag_stop() < 0){
			udefrag_pop_error(err_msg,ERR_MSG_SIZE);
			winx_printf("\nERROR: %s\n",err_msg);
		}
		abort_flag = 1;
	} else {
		winx_pop_error(NULL,0);
	}
	UpdateProgress();
	if(df == TRUE){
		msg = udefrag_get_command_result();
		if(strlen(msg) > 1)
			winx_printf("\nERROR: %s\n",msg);
		else if(!abort_flag) /* set progress to 100 % */
			for(k = i; k < 50; k++)	winx_putch('-');
	}
	return 0;
}

void Cleanup()
{
	char err_msg[ERR_MSG_SIZE];

	/* unload driver and registry cleanup */
	if(udefrag_unload() < 0){
		udefrag_pop_error(err_msg,ERR_MSG_SIZE);
		if(udefrag_initialized) /* because otherwise the message is trivial */
			winx_printf("\nERROR: %s\n",err_msg);
	}
}

void DisplayAvailableVolumes(void)
{
	volume_info *v;
	int n;
	char err_msg[ERR_MSG_SIZE];

	winx_printf("Available drive letters:   ");
	if(udefrag_get_avail_volumes(&v,FALSE) < 0){
		udefrag_pop_error(err_msg,ERR_MSG_SIZE);
		winx_printf("\nERROR: %s\n",err_msg);
	} else {
		for(n = 0;;n++){
			if(v[n].letter == 0) break;
			winx_printf("%c   ",v[n].letter);
		}
	}
	winx_printf("\r\n");
}

void ProcessVolume(char letter,char command)
{
	STATISTIC stat;
	int status = 0;
	char err_msg[ERR_MSG_SIZE];

	i = j = 0;
	last_op = 0;
	winx_printf("\nPreparing to ");
	switch(command){
	case 'a':
		winx_printf("analyse %c: ...\n",letter);
		status = udefrag_analyse(letter,update_stat);
		break;
	case 'd':
		winx_printf("defragment %c: ...\n",letter);
		status = udefrag_defragment(letter,update_stat);
		break;
	case 'c':
		winx_printf("optimize %c: ...\n",letter);
		status = udefrag_optimize(letter,update_stat);
		break;
	}
	if(status < 0){
		udefrag_pop_error(err_msg,ERR_MSG_SIZE);
		winx_printf("\nERROR: %s\n",err_msg);
		return;
	}

	if(udefrag_get_progress(&stat,NULL) < 0){
		udefrag_pop_error(err_msg,ERR_MSG_SIZE);
		winx_printf("\nERROR: %s\n",err_msg);
	} else {
		winx_printf("\n%s\n\n",udefrag_get_default_formatted_results(&stat));
	}
}

void ParseCommand()
{
	winx_printf("%ws\n", line_buffer);
}

void __stdcall NtProcessStartup(PPEB Peb)
{
	DWORD i;
	char cmd[33], arg[5];
	char *commands[] = {"shutdown","reboot","batch","exit",
	                    "analyse","defrag","optimize",
						"drives","help"};
	signed int code;
	char buffer[ERR_MSG_SIZE];
	
	WINX_FILE *f;
	char filename[MAX_PATH];
	int filesize,j,cnt;
	unsigned short ch;

	/* 3. Initialization */
#if USE_INSTEAD_SMSS
	NtInitializeRegistry(FALSE);
#endif
	/* 4. Display Copyright */
	if(winx_get_os_version() < 51) winx_printf("\n\n");
	winx_printf(VERSIONINTITLE " native interface\n"
		"Copyright (c) Dmitri Arkhangelski, 2007,2008.\n\n"
		"UltraDefrag comes with ABSOLUTELY NO WARRANTY.\n\n"
		"If something is wrong, hit F8 on startup\n"
		"and select 'Last Known Good Configuration'.\n"
		"Use Break key to abort defragmentation.\n\n");
	code = winx_init(Peb);
	if(code < 0){
		winx_pop_error(buffer,sizeof(buffer));
		winx_printf("%s\n",buffer);
		winx_printf("Wait 10 seconds ...\n");
		winx_sleep(10000);
		winx_exit(1);
	}
	/* 6b. Prompt to exit */
	winx_printf("Press any key to exit...  ");
	for(i = 0; i < 3; i++){
		if(winx_kbhit(1000) >= 0){ winx_printf("\n"); HandleError(L"",0); }
		else { winx_pop_error(NULL,0); }
		winx_printf("%c ",(char)('0' + 3 - i));
	}
	winx_printf("\n\n");
	/* 7. Initialize the ultradfg device */
	HandleError_(udefrag_init(0),2);
	udefrag_initialized = TRUE;

	/* 8a. Batch mode */
#if USE_INSTEAD_SMSS
#else

#if 0
	if(!settings->sched_letters)
		HandleError(L"No letters specified!",3);
	if(!settings->sched_letters[0])
		HandleError(L"No letters specified!",3);
	length = wcslen(settings->sched_letters);
	for(i = 0; i < length; i++){
		if((char)(settings->sched_letters[i]) == ';')
			continue; /* skip delimiters */
		ProcessVolume((char)(settings->sched_letters[i]),'d');
		if(abort_flag) break;
	}
	HandleError(L"",0);
#endif


	/* open script file */
	if(winx_get_windows_directory(filename,MAX_PATH) < 0){
		udefrag_pop_error(buffer,ERR_MSG_SIZE);
		winx_printf("\nERROR: %s\n",buffer);
		goto cmdloop;
	}
	strncat(filename,"\\system32\\ud-boot-time.sh",
			MAX_PATH - strlen(filename) - 1);
	f = winx_fopen(filename,"r");
	if(!f){
		udefrag_pop_error(buffer,ERR_MSG_SIZE);
		winx_printf("\n%s\n",buffer);
		goto cmdloop;
	}
	/* read contents */
	filesize = winx_fread(file_buffer,sizeof(short),
			(sizeof(file_buffer) / sizeof(short)) - 1,f);
	/* close file */
	winx_fclose(f);
	/* read commands and interpret them */
	if(!filesize){
		udefrag_pop_error(NULL,0);
		goto cmdloop;
	}
	file_buffer[filesize] = 0;
	line_buffer[0] = 0;
	cnt = 0;
	for(j = 0; j < filesize; j++){
		ch = file_buffer[j];
		/* skip first 0xFEFF character added by Notepad */
		if(j == 0 && ch == 0xFEFF) continue;
		if(ch == '\r' || ch == '\n'){
			/* terminate the line buffer */
			line_buffer[cnt] = 0;
			cnt = 0;
			/* parse line buffer contents */
			ParseCommand();
		} else {
			line_buffer[cnt] = ch;
			cnt++;
		}
	}

#endif
	/* 8b. Command Loop */
cmdloop:
	while(1){
		winx_printf("# ");
		if(winx_gets(buffer,sizeof(buffer) - 1) < 0){
			winx_pop_error(buffer,sizeof(buffer));
			if(strcmp(buffer,"Buffer overflow!") == 0){
				winx_printf("%s\n",buffer);
				Cleanup();
				winx_printf("Wait 10 seconds\n");
				winx_sleep(10000);
				winx_exit(4);
				return;
			}
		}
		cmd[0] = arg[0] = 0;
		sscanf(buffer,"%32s %4s",cmd,arg);
		for(i = 0; i < sizeof(commands) / sizeof(char*); i++)
			if(strstr(cmd,commands[i])) break;
		switch(i){
		case 0:
			winx_printf("Shutdown ...");
			Cleanup();
			winx_shutdown();
			continue;
		case 1:
			winx_printf("Reboot ...");
			Cleanup();
			winx_reboot();
			continue;
		case 2:
			winx_printf("Prepare to batch processing ...\n");
			continue;
		case 3:
			HandleError(L"",0);
			return;
		case 4:
			ProcessVolume(arg[0],'a');
			continue;
		case 5:
			ProcessVolume(arg[0],'d');
			continue;
		case 6:
			ProcessVolume(arg[0],'c');
			continue;
		case 7:
			DisplayAvailableVolumes();
			continue;
		case 8:
			winx_printf(USAGE);
			continue;
		default:
			winx_printf("Unknown command!\n");
		}
	}
}
