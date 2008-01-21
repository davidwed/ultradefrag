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
#include "../dll/zenwinx/src/zenwinx.h"

#define USE_INSTEAD_SMSS  0

ud_options *settings;
char letter;
int abort_flag = 0;
char last_op = 0;
int i = 0,j = 0,k; /* number of '-' */
char buffer[256];

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

void DisplayMessage(short *err_msg)
{
	if(err_msg)
		if(err_msg[0]) winx_printf("\nERROR: %ws\n",err_msg);
}

void HandleError(short *err_msg,int exit_code)
{
	if(err_msg){
		DisplayMessage(err_msg);
		Cleanup();
		if(exit_code){
			winx_printf("Wait 10 seconds ...\n");
			nsleep(10000); /* show error message at least 5 seconds */
		}
		winx_printf("Good bye ...\n");
		winx_exit(exit_code);
	}
}

void UpdateProgress()
{
	STATISTIC stat;
	double percentage;

	if(!udefrag_get_progress(&stat,&percentage)){
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
			winx_printf("\n%c: ",stat.current_operation);
			last_op = stat.current_operation;
		}
		j = (int)percentage / 2;
		for(k = i; k < j; k++)
			winx_putch('-');
		i = j;
	}
}

int __stdcall update_stat(int df)
{
	short *err_msg;

	if(winx_breakhit(100) >= 0){
		DisplayMessage(udefrag_stop());
		abort_flag = 1;
	}
	UpdateProgress();
	if(df == TRUE){
		err_msg = udefrag_get_ex_command_result();
		if(wcslen(err_msg) > 0)
			DisplayMessage(err_msg);
		else if(!abort_flag) /* set progress to 100 % */
			for(k = i; k < 50; k++)	winx_putch('-');
	}
	return 0;
}

void Cleanup()
{
	/* unload driver and registry cleanup */
	udefrag_unload(FALSE);
	DisplayMessage(udefrag_native_clean_registry());
}

void DisplayAvailableVolumes(void)
{
	short *err_msg;
	volume_info *v;
	int n;

	winx_printf("Available drive letters:   ");
	err_msg = udefrag_get_avail_volumes(&v,FALSE);
	if(err_msg) DisplayMessage(err_msg);
	else {
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
	short *err_msg = NULL;

	i = j = 0;
	last_op = 0;
	winx_printf("\nPreparing to ");
	switch(command){
	case 'a':
		winx_printf("analyse %c: ...\n",letter);
		err_msg = udefrag_analyse_ex(letter,update_stat);
		break;
	case 'd':
		winx_printf("defragment %c: ...\n",letter);
		err_msg = udefrag_defragment_ex(letter,update_stat);
		break;
	case 'c':
		winx_printf("optimize %c: ...\n",letter);
		err_msg = udefrag_optimize_ex(letter,update_stat);
		break;
	}
	if(err_msg){
		DisplayMessage(err_msg);
		return;
	}

	err_msg = udefrag_get_progress(&stat,NULL);
	if(err_msg)
		DisplayMessage(err_msg);
	else
		winx_printf("\n%s\n\n",udefrag_get_default_formatted_results(&stat));
}

void __stdcall NtProcessStartup(PPEB Peb)
{
	DWORD i,length;
	char cmd[33], arg[5];
	char *commands[] = {"shutdown","reboot","batch","exit",
	                    "analyse","defrag","optimize",
						"drives","help"};
	signed int code;
	unsigned long excode;
	char buffer[256];

	/* 3. Initialization */
	settings = udefrag_get_options();
#if USE_INSTEAD_SMSS
	NtInitializeRegistry(FALSE);
#endif
	/* 4. Display Copyright */
	if(get_os_version() < 51) winx_printf("\n\n");
	winx_printf(VERSIONINTITLE " native interface\n"
		"Copyright (c) Dmitri Arkhangelski, 2007,2008.\n\n"
		"UltraDefrag comes with ABSOLUTELY NO WARRANTY.\n\n"
		"If something is wrong, hit F8 on startup\n"
		"and select 'Last Known Good Configuration'.\n"
		"Use Break key to abort defragmentation.\n\n");
	code = winx_init(Peb);
	if(code < 0){
		if(winx_get_error_message_ex(buffer,sizeof(buffer),code) < 0)
			winx_printf("Initialization failed and buffer is too small!\n");
		else
			winx_printf("%s\n",buffer);
		winx_printf("Wait 10 seconds ...\n");
		nsleep(10000);
		winx_exit(1);
	}
	/* 6b. Prompt to exit */
	winx_printf("Press any key to exit...  ");
	for(i = 0; i < 3; i++){
		if(winx_kbhit(1000) >= 0){ winx_printf("\n"); HandleError(L"",0); }
		winx_printf("%c ",(char)('0' + 3 - i));
	}
	winx_printf("\n\n");
	/* 7. Initialize the ultradfg device */
	HandleError(udefrag_init(0,NULL,TRUE,0),2);

	/* 8a. Batch mode */
#if USE_INSTEAD_SMSS
#else
	if(settings->next_boot || settings->every_boot){
		/* do scheduled job and exit */
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
	}
#endif
	/* 8b. Command Loop */
	while(1){
		winx_printf("# ");
		if(winx_gets(buffer,sizeof(buffer) - 1) < 0){
			excode = winx_get_last_error();
			if(excode){
				winx_printf("Keyboard read error %x!\n%s\n",(UINT)excode,
				            winx_get_error_description(excode));
				Cleanup();
				winx_printf("Wait 10 seconds\n");
				nsleep(10000);
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
