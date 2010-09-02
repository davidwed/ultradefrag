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
* UltraDefrag native interface - native udefrag command implementation.
*/

#include "defrag_native.h"

/**
 * @brief Current defrag job type.
 */
UDEFRAG_JOB_TYPE current_job;

/**
 * @brief Indicates whether the 
 * defragmentation must be aborted
 * or not.
 */
int abort_flag = 0;

/* for the progress draw speedup */
int progress_line_length = 0;

extern int scripting_mode;
extern int escape_flag;

/**
 * @brief Returns current debug level.
 */
int GetDebugLevel()
{
	int result = DBG_NORMAL;
	short buffer[128];
	
	if(winx_query_env_variable(L"UD_DBGPRINT_LEVEL",
	  buffer,sizeof(buffer) / sizeof(short)) >= 0){
		(void)_wcsupr(buffer);
		if(!wcscmp(buffer,L"DETAILED"))
			result = DBG_DETAILED;
		else if(!wcscmp(buffer,L"PARANOID"))
			result = DBG_PARANOID;
	}
	return result;
}

/**
 * @brief Redraws the progress information.
 * @note Old dash based progress indication has wrong
 * algorithm not reliable by definition. Therefore
 * it has been replaced by a new single line indicator.
 */
void RedrawProgress(int completed)
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
		if(current_job == OPTIMIZE_JOB){
			n = (stat.pass_number == 0xffffffff) ? 0 : stat.pass_number;
			if(abort_flag) _snprintf(s,sizeof(s),"Pass %u:  %s%3u.%02u%% aborted",n,op_name,p1,p2);
			else _snprintf(s,sizeof(s),"Pass %u:  %s%3u.%02u%% completed",n,op_name,p1,p2);
		} else {
			if(abort_flag) _snprintf(s,sizeof(s),"%s%3u.%02u%% aborted",op_name,p1,p2);
			else _snprintf(s,sizeof(s),"%s%3u.%02u%% completed",op_name,p1,p2);
		}
		s[sizeof(s) - 1] = 0;
		_snprintf(format,sizeof(format),"\r%%-%us",progress_line_length);
		format[sizeof(format) - 1] = 0;
		winx_printf(format,s);
		progress_line_length = strlen(s);
	}
}

/**
 * @brief Updates progress information
 * on the screen and raises a job termination
 * when Esc\Break keys are pressed.
 */
int __stdcall update_progress(int df)
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
	RedrawProgress(df);
	return 0;
}

/**
 * @brief Processes single volume.
 */
void ProcessVolume(char letter)
{
	STATISTIC stat;
	int status;
	char volume_name[2];

	/* validate the volume before any processing */
	status = udefrag_validate_volume(letter,FALSE);
	if(status < 0){
		winx_printf("\nThe %c: volume cannot be processed!\n",letter);
		if(status == UDEFRAG_UNKNOWN_ERROR)
			winx_printf("Volume is missing or some error has been encountered.\n");
		else
			winx_printf("%s\n",udefrag_get_error_description(status));
		return;
	}
	
	volume_name[0] = letter; volume_name[1] = 0;
	progress_line_length = 0;
	winx_printf("\nPreparing to ");
	switch(current_job){
	case ANALYSE_JOB:
		winx_printf("analyse %c: ...\n",letter);
		break;
	case DEFRAG_JOB:
		winx_printf("defragment %c: ...\n",letter);
		break;
	case OPTIMIZE_JOB:
		winx_printf("optimize %c: ...\n",letter);
		break;
	}
	winx_printf(BREAK_MESSAGE);
	status = udefrag_start(volume_name,current_job,0,update_progress);
	if(status < 0){
		winx_printf("\nAnalysis/Defragmentation failed!\n");
		winx_printf("%s\n",udefrag_get_error_description(status));
		return;
	}

	if(udefrag_get_progress(&stat,NULL) >= 0)
		winx_printf("\n\n%s\n",udefrag_get_default_formatted_results(&stat));
}

/**
 * @brief Displays list of volumes
 * available for defragmentation.
 * @param[in] skip_removable defines
 * whether to skip removable media or not.
 * @return Zero for success, negative
 * value otherwise.
 */
static int DisplayAvailableVolumes(int skip_removable)
{
	volume_info *v;
	int n;

	if(udefrag_get_avail_volumes(&v,skip_removable) >= 0){
		winx_printf("\nAvailable drive letters:   ");
		for(n = 0;;n++){
			if(v[n].letter == 0) break;
			winx_printf("%c   ",v[n].letter);
		}
		winx_printf("\n\n");
		return 0;
	}
	winx_printf("\n\n");
	return (-1);
}

/**
 * @brief udefrag command handler.
 */
int __cdecl udefrag_handler(int argc,short **argv,short **envp)
{
	int a_flag = 0, o_flag = 0;
	int all_flag = 0, all_fixed_flag = 0;
	char letters[MAX_DOS_DRIVES];
	int i, n_letters = 0;
	char letter;
	volume_info *v;
	int debug_level;
	
	(void)envp;
	
	if(argc < 2){
		winx_printf("\nNo volume letter specified!\n\n");
		return (-1);
	}
	
	/* handle volumes listing request */
	if(!wcscmp(argv[1],L"-l"))
		return DisplayAvailableVolumes(TRUE);
	if(!wcscmp(argv[1],L"-la"))
		return DisplayAvailableVolumes(FALSE);
	
	/* parse command line */
	for(i = 1; i < argc; i++){
		/* handle flags */
		if(!wcscmp(argv[i],L"-a")){
			a_flag = 1;
			continue;
		} else if(!wcscmp(argv[i],L"-o")){
			o_flag = 1;
			continue;
		} else if(!wcscmp(argv[i],L"--all")){
			all_flag = 1;
			continue;
		} else if(!wcscmp(argv[i],L"--all-fixed")){
			all_fixed_flag = 1;
			continue;
		}
		/* handle drive letters */
		if(argv[i][0] != 0){
			if(argv[i][1] == ':'){
				if(n_letters > (MAX_DOS_DRIVES - 1)){
					winx_printf("\n%ws: Too many letters specified on the command line.\n\n",
						argv[0]);
				} else {
					letters[n_letters] = (char)argv[i][0];
					n_letters ++;
				}
				continue;
			}
		}
		/* handle unknown options */
		winx_printf("\n%ws: unknown option \'%ws\' found!\n\n",
			argv[0],argv[i]);
		return (-1);
	}
	
	/* check whether volume letters are specified or not */
	if(!n_letters && !all_flag && !all_fixed_flag){
		winx_printf("\nNo volume letter specified!\n\n");
		return (-1);
	}
	
	/* set current_job global variable */
	if(a_flag) current_job = ANALYSE_JOB;
	else if(o_flag) current_job = OPTIMIZE_JOB;
	else current_job = DEFRAG_JOB;
	
	/*
	* In scripting mode the abort_flag has initial value 0.
	* The Break key sets them to 1 => disables any further
	* defragmentation jobs.
	* On the other hand, in interactive mode we are setting
	* this flag to 0 before any defragmentation job. This
	* technique breaks only the current job.
	*/
	if(!scripting_mode) abort_flag = 0;

	debug_level = GetDebugLevel();
	
	/* process volumes specified on the command line */
	for(i = 0; i < n_letters; i++){
		if(abort_flag) break;
		letter = letters[i];
		ProcessVolume(letter);
		if(debug_level > DBG_NORMAL) winx_sleep(5000);
	}

	if(abort_flag)
		return 0;
	
	/* process all volumes if requested */
	if(all_flag || all_fixed_flag){
		if(udefrag_get_avail_volumes(&v,all_fixed_flag ? TRUE : FALSE) < 0){
			winx_printf("\n%ws: udefrag_get_avail_volumes() failed!\n\n",argv[0]);
			return (-1);
		}
		for(i = 0;;i++){
			if(v[i].letter == 0) break;
			if(abort_flag) break;
			letter = v[i].letter;
			ProcessVolume(letter);
			if(debug_level > DBG_NORMAL) winx_sleep(5000);
		}
	}
	return 0;
}
