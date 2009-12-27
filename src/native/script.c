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
* UltraDefrag native interface - scripting support code.
*/

#include "../include/ntndk.h"

#include "../include/ultradfg.h"
#include "../include/udefrag-kernel.h"
#include "../include/udefrag.h"
#include "../include/ultradfgver.h"
#include "../dll/zenwinx/zenwinx.h"

short file_buffer[32768];
short line_buffer[32768];
short name_buffer[128];
short value_buffer[4096];
short *command;
int echo_flag = 1;
int abort_flag = 0;
char last_op = 0;
int i = 0,j = 0,k; /* number of '-' */
UDEFRAG_JOB_TYPE job_type;
ULONG pass_number;

#define NAME_BUF_SIZE (sizeof(name_buffer) / sizeof(short))
#define VALUE_BUF_SIZE (sizeof(value_buffer) / sizeof(short))

void ExtractToken(short *dest, short *src, int max_chars);

/* Volume defragmentation code */
void UpdateProgress()
{
	STATISTIC stat;
	double percentage;
	char s[64];

	if(udefrag_get_progress(&stat,&percentage) < 0) return;

	if(stat.current_operation != last_op){
		if(last_op){
			for(k = i; k < 50; k++)
				winx_putch('-'); /* 100 % of previous operation */
			if(stat.pass_number != 0xffffffff && stat.pass_number > pass_number){
				pass_number = stat.pass_number;
				_itoa(pass_number,s,10);
				winx_printf("\n\nPass %s ...\n",s);
			}
		} else {
			if(stat.current_operation != 'A'){
				winx_printf("\nAnalyse : ");
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
	return;
}

int __stdcall update_stat(int df)
{
	if(winx_breakhit(100) >= 0){
		if(udefrag_stop() < 0){
			winx_printf("\nStop request failed!\n");
		}
		abort_flag = 1;
	}
	UpdateProgress();
	if(df == TRUE){
		if(!abort_flag) /* set progress to 100 % */
			for(k = i; k < 50; k++)	winx_putch('-');
	}
	return 0;
}

void ProcessVolume(char letter,char defrag_command)
{
	STATISTIC stat;
	int status = 0;

	udefrag_reload_settings();
	
	i = j = 0;
	last_op = 0;
	winx_printf("\nPreparing to ");
	switch(defrag_command){
	case 'a':
		job_type = ANALYSE_JOB;
		winx_printf("analyse %c: ...\n",letter);
		status = udefrag_analyse(letter,update_stat);
		break;
	case 'd':
		job_type = DEFRAG_JOB;
		winx_printf("defragment %c: ...\n",letter);
		status = udefrag_defragment(letter,update_stat);
		break;
	case 'c':
		job_type = OPTIMIZE_JOB;
		pass_number = 0;
		winx_printf("optimize %c: ...\n",letter);
		winx_printf("\nPass 0 ...\n");
		status = udefrag_optimize(letter,update_stat);
		break;
	}
	if(status < 0){
		winx_printf("\nYou are trying to defragment cdrom/network/missing volume\n");
		winx_printf("or system disallows this or unknown internal bug encountered.\n");
		return;
	}

	if(udefrag_get_progress(&stat,NULL) >= 0)
		winx_printf("\n%s\n\n",udefrag_get_default_formatted_results(&stat));
}

/* various commands */

void DisplayAvailableVolumes(int skip_removable)
{
	volume_info *v;
	int n;

	if(udefrag_get_avail_volumes(&v,skip_removable) >= 0){
		winx_printf("\nAvailable drive letters:   ");
		for(n = 0;;n++){
			if(v[n].letter == 0) break;
			winx_printf("%c   ",v[n].letter);
		}
	}
	winx_printf("\n\n");
}

void SetEnvVariable()
{
	short *eq_pos;
	int name_len, value_len;
	short *val = value_buffer;
	
	/* find the equality sign */
	eq_pos = wcschr(command,'=');
	if(!eq_pos){
		winx_printf("\nThere is no name-value pair specified!\n\n");
		return;
	}
	/* extract a name-value pair */
	name_buffer[0] = value_buffer[0] = 0;
	name_len = (int)(LONG_PTR)(eq_pos - command - wcslen(L"set"));
	value_len = (int)(LONG_PTR)(command + wcslen(command) - eq_pos - 1);
	ExtractToken(name_buffer,command + wcslen(L"set"),min(name_len,NAME_BUF_SIZE - 1));
	ExtractToken(value_buffer,eq_pos + 1,min(value_len,VALUE_BUF_SIZE - 1));
	_wcsupr(name_buffer);
	if(!name_buffer[0]){
		winx_printf("\nVariable name is not specified!\n\n");
		return;
	}
	if(!value_buffer[0]) val = NULL;
	winx_set_env_variable(name_buffer,val);
}

void PauseExecution()
{
	int msec;
	
	ExtractToken(value_buffer,command + wcslen(L"pause"),
		min(wcslen(command) - wcslen(L"pause"),VALUE_BUF_SIZE - 1));
	msec = (int)_wtol(value_buffer);
	winx_sleep(msec);
}

/* enables boot time defragmentation */
void EnableNativeDefragger(void)
{
	(void)winx_register_boot_exec_command(L"defrag_native");
}

/* disables boot time defragmentation */
void DisableNativeDefragger(void)
{
	(void)winx_unregister_boot_exec_command(L"defrag_native");
}

void Hibernate(void)
{
	winx_printf("Hibernation ...");
	NtSetSystemPowerState(
		PowerActionHibernate, /* constants defined in winnt.h */
		PowerSystemHibernate,
		0
		);
	winx_printf("\nHibernation?\n");
	/*
	* It seems that hibernation is impossible at 
	* windows boot time. At least on XP and earlier systems.
	*/
}

void ParseCommand()
{
	int echo_cmd = 0;
	int comment_flag = 0;
	int a_flag = 0, o_flag = 0;
	char letter = 0, cmd = 'd';
	short *pos;
	
	/* supported commands: @echo, set, udefrag, exit, shutdown, reboot, pause */
	/* skip leading spaces and tabs */
	command = line_buffer;
	while(*command == 0x20 || *command == '\t'){
		command ++;
	}
	/* check for comment */
	if(command[0] == ';' || command[0] == '#') comment_flag = 1;
	/* check for an empty string */
	if(!command[0]) comment_flag = 1;
	/* check for @echo command */
	if((short *)wcsstr(command,L"@echo on") == command){
		echo_flag = 1; echo_cmd = 1;
	}
	if((short *)wcsstr(command,L"@echo off") == command){
		echo_flag = 0; echo_cmd = 1;
	}
	if(echo_flag && !echo_cmd) winx_printf("%ws\n", command);
	if(echo_cmd || comment_flag) return;
	/* check for set command */
	if((short *)wcsstr(command,L"set") == command){
		SetEnvVariable();
		return;
	}
	/* handle udefrag command */
	if((short *)wcsstr(command,L"udefrag") == command){
		if(wcsstr(command,L"-la")){
			DisplayAvailableVolumes(FALSE);
			return;
		}
		if(wcsstr(command,L"-l")){
			DisplayAvailableVolumes(TRUE);
			return;
		}
		if(wcsstr(command,L"-a")) a_flag = 1;
		if(wcsstr(command,L"-o")) o_flag = 1;
		/* find volume letter */
		pos = (short *)wcschr(command,':');
		if(pos > command) letter = (char)pos[-1];
		if(!letter){
			winx_printf("\nNo volume letter specified!\n\n");
			return;
		}
		if(a_flag) cmd = 'a';
		else if(o_flag) cmd = 'c';
		ProcessVolume(letter,cmd);
		/* if an operation was aborted then exit */
		if(abort_flag) goto break_execution;
		return;
	}
	/* handle exit command */
	if((short *)wcsstr(command,L"exit") == command){
break_execution:
		winx_printf("Good bye ...\n");
		udefrag_unload();
		winx_exit(0);
		return;
	}
	/* handle shutdown command */
	if((short *)wcsstr(command,L"shutdown") == command){
		winx_printf("Shutdown ...");
		udefrag_unload();
		winx_shutdown();
		return;
	}
	/* handle reboot command */
	if((short *)wcsstr(command,L"reboot") == command){
		winx_printf("Reboot ...");
		udefrag_unload();
		winx_reboot();
		return;
	}
	if((short *)wcsstr(command,L"pause") == command){
		PauseExecution();
		return;
	}
	if((short *)wcsstr(command,L"boot-on") == command){
		EnableNativeDefragger();
		return;
	}
	if((short *)wcsstr(command,L"boot-off") == command){
		DisableNativeDefragger();
		return;
	}
	if((short *)wcsstr(command,L"hibernate") == command){
		Hibernate();
		return;
	}
	winx_printf("\nUnknown command %ws!\n\n",command);
}

void ExtractToken(short *dest, short *src, int max_chars)
{
	signed int i,cnt;
	short ch;
	
	cnt = 0;
	for(i = 0; i < max_chars; i++){
		ch = src[i];
		/* skip spaces and tabs in the beginning */
		if((ch != 0x20 && ch != '\t') || cnt){
			dest[cnt] = ch;
			cnt++;
		}
	}
	dest[cnt] = 0;
	/* remove spaces and tabs from the end */
	if(cnt == 0) return;
	for(i = (cnt - 1); i >= 0; i--){
		ch = dest[i];
		if(ch != 0x20 && ch != '\t') break;
		dest[i] = 0;
	}
}

void ProcessScript(void)
{
	WINX_FILE *f;
	char filename[MAX_PATH];
	int filesize,j,cnt;
	unsigned short ch;

	/* open script file */
	if(winx_get_windows_directory(filename,MAX_PATH) < 0) return;
	strncat(filename,"\\system32\\ud-boot-time.cmd",
			MAX_PATH - strlen(filename) - 1);
	f = winx_fopen(filename,"r");
	if(!f) return;
	/* read contents */
	filesize = winx_fread(file_buffer,sizeof(short),
			(sizeof(file_buffer) / sizeof(short)) - 1,f);
	/* close file */
	winx_fclose(f);
	/* read commands and interpret them */
	if(!filesize) return;
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
}
