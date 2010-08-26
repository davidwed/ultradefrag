/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
 *  Copyright (c) 2010 by Stefan Pendl (stefanpe@users.sourceforge.net).
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

#include "defrag_native.h"

/*
* Old dash based progress indication has wrong
* algorithm not reliable by definition. Therefore
* it has been replaced by a new single line indicator.
*/

short file_buffer[32768];
short line_buffer[32768];
short name_buffer[128];
short value_buffer[4096];
short *command;
int echo_flag = 1;

/* indicates whether the defragmentation must be aborted or not */
int abort_flag = 0;
/* indicates whether the script execution must be aborted or not */
int escape_flag = 0;

int debug_print = DBG_NORMAL;

UDEFRAG_JOB_TYPE job_type;
BOOLEAN scripting_mode = TRUE;

/* for the progress draw speedup */
int progress_line_length = 0;

#define NAME_BUF_SIZE (sizeof(name_buffer) / sizeof(short))
#define VALUE_BUF_SIZE (sizeof(value_buffer) / sizeof(short))

#define BREAK_MESSAGE "Use Pause/Break key to abort the process.\n\n"

/*
* boot-off command modifies registry, 
* but shutdown\reboot commands releases its effect,
* because all registry modifications becomes lost.
* Therefore we track pending boot-off request through
* a special pending-boot-off file in windows directory.
*/
int pending_boot_off = 0;
void SavePendingBootOffState(void);

void ExtractToken(short *dest, short *src, int max_chars);

void GetDebugLevel()
{
	if(winx_query_env_variable(L"UD_DBGPRINT_LEVEL",value_buffer,sizeof(value_buffer)) >= 0){
		(void)_wcsupr(value_buffer);
		if(!wcscmp(value_buffer,L"DETAILED"))
			debug_print = DBG_DETAILED;
		else if(!wcscmp(value_buffer,L"PARANOID"))
			debug_print = DBG_PARANOID;
		else if(!wcscmp(value_buffer,L"NORMAL"))
			debug_print = DBG_NORMAL;
	}
}

void UpdateProgress(int completed)
{
	STATISTIC stat;
	double percentage;
	int p1, p2, n;
	char op; char *op_name = "";
	char s[64];
	char format[16];

	/* TODO: optimize for speed to make redraw faster */
	if(udefrag_get_progress(&stat,&percentage) >= 0){
		op = stat.current_operation;
		if(op == 'A' || op == 'a')      op_name = "Analyze:  ";
		else if(op == 'D' || op == 'd') op_name = "Defrag:   ";
		else if(op == 'C' || op == 'c') op_name = "Optimize: ";
		else                            op_name = "          ";
		if(!completed || abort_flag){
			p1 = (int)(percentage * 100.00);
			p2 = p1 % 100;
			p1 = p1 / 100;
		} else {
			p1 = 100;
			p2 = 0;
		}
		if(job_type == OPTIMIZE_JOB){
			n = (stat.pass_number == 0xffffffff) ? 0 : stat.pass_number;
			if (abort_flag) _snprintf(s,sizeof(s),"Pass %u:  %s%3u.%02u%% aborted",n,op_name,p1,p2);
			else _snprintf(s,sizeof(s),"Pass %u:  %s%3u.%02u%% completed",n,op_name,p1,p2);
		} else {
			if (abort_flag) _snprintf(s,sizeof(s),"%s%3u.%02u%% aborted",op_name,p1,p2);
			else _snprintf(s,sizeof(s),"%s%3u.%02u%% completed",op_name,p1,p2);
		}
		s[sizeof(s) - 1] = 0;
		_snprintf(format,sizeof(format),"\r%%-%us",progress_line_length);
		format[sizeof(format) - 1] = 0;
		winx_printf(format,s);
		progress_line_length = strlen(s);
	}
}

int __stdcall update_stat(int df)
{
	KBD_RECORD kbd_rec;
	int error_code;
	int escape_detected = 0;
	int break_detected = 0;
	
	/* check for escape and break key hits */
	if(winx_kb_read(&kbd_rec,100) >= 0){
		/* check for escape */
		if(kbd_rec.wVirtualScanCode == 0x1){
			escape_detected = 1;
			escape_flag = 1;
		} else if(kbd_rec.wVirtualScanCode == 0x1d){
			/* distinguish between control keys and break key */
			if(!(kbd_rec.dwControlKeyState & LEFT_CTRL_PRESSED) && \
			  !(kbd_rec.dwControlKeyState & RIGHT_CTRL_PRESSED)){
				break_detected = 1;
			}
		}
		if(escape_detected || break_detected){
			error_code = udefrag_stop();
			if(error_code < 0){
				winx_printf("\nStop request failed!\n");
				winx_printf("%s\n",udefrag_get_error_description(error_code));
			}
			abort_flag = 1;
		}
	}
	UpdateProgress(df);
	return 0;
}

void ProcessVolume(char letter,char defrag_command)
{
	STATISTIC stat;
	int status = 0;
	char volume_name[2];

	/* validate the volume before any processing */
	status = udefrag_validate_volume(letter,FALSE);
	if(status < 0){
		winx_printf("\nThe volume cannot be processed!\n");
		if(status == UDEFRAG_UNKNOWN_ERROR)
			winx_printf("Volume is missing or some error has been encountered.\n");
		else
			winx_printf("%s\n",udefrag_get_error_description(status));
		return;
	}
	
	volume_name[0] = letter; volume_name[1] = 0;
	progress_line_length = 0;
	winx_printf("\nPreparing to ");
	switch(defrag_command){
	case 'a':
		job_type = ANALYSE_JOB;
		winx_printf("analyse %c: ...\n",letter);
		winx_printf(BREAK_MESSAGE);
		status = udefrag_start(volume_name,ANALYSE_JOB,0,update_stat);
		break;
	case 'd':
		job_type = DEFRAG_JOB;
		winx_printf("defragment %c: ...\n",letter);
		winx_printf(BREAK_MESSAGE);
		status = udefrag_start(volume_name,DEFRAG_JOB,0,update_stat);
		break;
	case 'c':
		job_type = OPTIMIZE_JOB;
		winx_printf("optimize %c: ...\n",letter);
		winx_printf(BREAK_MESSAGE);
		status = udefrag_start(volume_name,OPTIMIZE_JOB,0,update_stat);
		break;
	}
	if(status < 0){
		winx_printf("\nAnalysis/Defragmentation failed!\n");
		winx_printf("%s\n",udefrag_get_error_description(status));
		return;
	}

	if(udefrag_get_progress(&stat,NULL) >= 0)
		winx_printf("\n\n%s\n",udefrag_get_default_formatted_results(&stat));
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
	(void)_wcsupr(name_buffer);
	if(!name_buffer[0]){
		winx_printf("\nVariable name is not specified!\n\n");
		return;
	}
	if(!value_buffer[0]) val = NULL;
	if(winx_set_env_variable(name_buffer,val) < 0){
		winx_printf("\nCannot set environment variable.\n\n");
	}
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
int EnableNativeDefragger(void)
{
	if(winx_register_boot_exec_command(L"defrag_native") < 0){
		winx_printf("\nCannot enable the boot time defragmenter.\n\n");
		return (-1);
	}
	return 0;
}

/* disables boot time defragmentation */
int DisableNativeDefragger(void)
{
	if(winx_unregister_boot_exec_command(L"defrag_native") < 0){
		winx_printf("\nCannot disable the boot time defragmenter.\n\n");
		return (-1);
	}
	return 0;
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

/**
 * @brief Executes pending boot-off command.
 * @return Boolean value indicating whether
 * pending boot-off command was detected or not.
 */
int ExecPendingBootOff(void)
{
	char path[MAX_PATH];
	WINX_FILE *f;

	if(winx_get_windows_directory(path,MAX_PATH) < 0){
		DebugPrint("ExecPendingBootOff(): Cannot retrieve the Windows directory path!");
		winx_printf("\nExecPendingBootOff(): Cannot retrieve the Windows directory path!\n\n");
		short_dbg_delay();
		return 0;
	}
	(void)strncat(path,"\\pending-boot-off",
			MAX_PATH - strlen(path) - 1);

	f = winx_fopen(path,"r");
	if(f == NULL) return 0;

	winx_fclose(f);
	if(DisableNativeDefragger() < 0){
		short_dbg_delay();
	}
	if(winx_delete_file(path) < 0){
		DebugPrint("ExecPendingBootOff(): Cannot delete %%windir%%\\pending-boot-off file!");
		winx_printf("\nExecPendingBootOff(): Cannot delete %%windir%%\\pending-boot-off file!\n\n");
		short_dbg_delay();
	}
	winx_printf("\nPending boot-off command execution completed.\n");
	return 1;
}

void SavePendingBootOffState(void)
{
	char path[MAX_PATH];
	WINX_FILE *f;
	char *comment = "UltraDefrag boot-off command is pending.";
	
	if(!pending_boot_off) return;

	if(winx_get_windows_directory(path,MAX_PATH) < 0){
		DebugPrint("SavePendingBootOffState(): Cannot retrieve the Windows directory path!");
		winx_printf("\nSavePendingBootOffState(): Cannot retrieve the Windows directory path!\n\n");
		short_dbg_delay();
		return;
	}
	(void)strncat(path,"\\pending-boot-off",
			MAX_PATH - strlen(path) - 1);
	f = winx_fopen(path,"w");
	if(f == NULL){
		DebugPrint("%%windir%%\\pending-boot-off file creation failed!");
		winx_printf("\n%%windir%%\\pending-boot-off file creation failed!\n\n");
		short_dbg_delay();
		return;
	}
	(void)winx_fwrite(comment,sizeof(char),sizeof(comment)/sizeof(char) - 1,f);
	winx_fclose(f);
}

void ParseCommand(void)
{
	int echo_cmd = 0;
	int comment_flag = 0;
	int a_flag = 0, o_flag = 0;
	int all_flag = 0, all_fixed_flag = 0;
	char letter = 0, cmd = 'd';
	char letters[MAX_DOS_DRIVES];
	int n_letters;
	int i, length;
	volume_info *v;
	
	/* supported commands: @echo, set, udefrag, pause, boot-on, boot-off, reboot, shutdown, exit */
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
		/* handle drives listing request */
		if(wcsstr(command,L" -la")){
			DisplayAvailableVolumes(FALSE);
			return;
		}
		if(wcsstr(command,L" -l")){
			DisplayAvailableVolumes(TRUE);
			return;
		}
		
		/* get flags */
		if(wcsstr(command,L" -a")) a_flag = 1;
		if(wcsstr(command,L" -o")) o_flag = 1;
		
		if(wcsstr(command,L" --all-fixed")) all_fixed_flag = 1;
		else if(wcsstr(command,L" --all")) all_flag = 1;
		
		if(a_flag) cmd = 'a';
		else if(o_flag) cmd = 'c';

		/* find volume letters */
		length = wcslen(command);
		n_letters = 0;
		for(i = 0; i < length; i++){
			if(command[i] == ':' && i > 0){
				if(n_letters > (MAX_DOS_DRIVES - 1)){
					winx_printf("Too many letters specified on the command line.\n");
				} else {
					letters[n_letters] = (char)command[i - 1];
					n_letters ++;
				}
			}
		}

		if(!n_letters && !all_flag && !all_fixed_flag){
			winx_printf("\nNo volume letter specified!\n\n");
			return;
		}
		
		/*
		* In scripting mode the abort_flag has initial value 0.
		* The Break key sets them to 1 => disables any further
		* defragmentation jobs.
		* On the other hand, in interactive mode we are setting
		* this flag to 0 before any defragmentation job. This
		* technique breaks only the current job.
		*/
		if(!scripting_mode) abort_flag = 0;
		
		GetDebugLevel();
		
		/* process volumes specified on the command line */
		for(i = 0; i < n_letters; i++){
			if(abort_flag) break;
			letter = letters[i];
			ProcessVolume(letter,cmd);
			if (debug_print > DBG_NORMAL) winx_sleep(5000);
		}
		
		if(abort_flag) return;
		
		/* process all volumes if requested */
		if(all_flag || all_fixed_flag){
			if(udefrag_get_avail_volumes(&v,all_fixed_flag ? TRUE : FALSE) < 0) return;
			for(i = 0;;i++){
				if(v[i].letter == 0) break;
				if(abort_flag) break;
				letter = v[i].letter;
				ProcessVolume(letter,cmd);
				if (debug_print > DBG_NORMAL) winx_sleep(5000);
			}
		}
		
		return;
	}
	/* handle exit command */
	if((short *)wcsstr(command,L"exit") == command){
		winx_printf("Good bye ...\n");
		(void)udefrag_unload();
		udefrag_monolithic_native_app_unload();
		winx_exit(0);
		return;
	}
	/* handle shutdown command */
	if((short *)wcsstr(command,L"shutdown") == command){
		winx_printf("Shutdown ...");
		(void)udefrag_unload();
		SavePendingBootOffState();
		winx_shutdown();
		return;
	}
	/* handle reboot command */
	if((short *)wcsstr(command,L"reboot") == command){
		winx_printf("Reboot ...");
		(void)udefrag_unload();
		SavePendingBootOffState();
		winx_reboot();
		return;
	}
	if((short *)wcsstr(command,L"pause") == command){
		PauseExecution();
		return;
	}
	if((short *)wcsstr(command,L"boot-on") == command){
		(void)EnableNativeDefragger();
		pending_boot_off = 0;
		return;
	}
	if((short *)wcsstr(command,L"boot-off") == command){
		(void)DisableNativeDefragger();
		pending_boot_off = 1;
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
	KBD_RECORD kbd_rec;
	
	scripting_mode = TRUE;

	/* open script file */
	if(winx_get_windows_directory(filename,MAX_PATH) < 0){
		winx_printf("\nCannot retrieve the Windows directory path!\n\n");
		return;
	}
	(void)strncat(filename,"\\system32\\ud-boot-time.cmd",
			MAX_PATH - strlen(filename) - 1);
	f = winx_fopen(filename,"r");
	if(!f){
		winx_printf("\nCannot open the boot time script!\n\n");
		return;
	}
	/* read contents */
	filesize = winx_fread(file_buffer,sizeof(short),
			(sizeof(file_buffer) / sizeof(short)) - 1,f);
	/* close file */
	winx_fclose(f);
	/* read commands and interpret them */
	if(!filesize){
		winx_printf("\nCannot read the boot time script!\n\n");
		return;
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
			/* check for escape key hits */
			if(escape_flag) return;
			if(winx_kb_read(&kbd_rec,0) == 0){
				if(kbd_rec.wVirtualScanCode == 0x1)
					return;
			}
		} else {
			line_buffer[cnt] = ch;
			cnt++;
		}
	}
}
