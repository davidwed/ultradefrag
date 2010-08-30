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
* UltraDefrag native interface.
*/

#include "defrag_native.h"

/*
* We're releasing this module as a monolithic thing to
* prevent BSOD in case of missing UltraDefrag native libraries.
* Otherwise the following modules becomes critical for the Windows
* boot process: udefrag.dll, udefrag-kernel.dll, zenwinx.dll.
* We don't know how to build monolithic app on MinGW,
* but on Windows DDK this works fine.
*/

extern short line_buffer[32768];
extern BOOLEAN scripting_mode;

void ProcessScript(void);
void ParseCommand(void);
void EnableNativeDefragger(void);
void DisableNativeDefragger(void);
void Hibernate(void);
void DisplayAvailableVolumes(int skip_removable);
void ProcessVolume(char letter,char _command);
int ExecPendingBootOff(void);

void display_help(void)
{
    int i, page_limit, row_count;
    char *help_message[] = {
		"Interactive mode commands:",
		"",
		"  help          - display this help screen",
		"  history       - display list of manually entered commands",
		"",
		"    Cycle through history with up/down arrow/cursor keys.",
		"",
		"  @echo on      - enable command line display",
		"  @echo off     - disable command line display",
		"  set           - set environment variable",
		"",
		"  udefrag -l    - displays list of volumes except removable",
		"  udefrag -la   - displays list of all available volumes",
		"  udefrag -a X: - volume analysis",
		"  udefrag X:    - volume defragmentation",
		"  udefrag -o X: - volume optimization",
		"",
		"    Multiple volume letters allowed,",
		"    --all, --all-fixed keys are supported here too.",
		"",
		"  boot-on       - enable boot time defragger",
		"  boot-off      - disable boot time defragger",
		"  reboot        - reboot the PC",
		"  shutdown      - halt the PC",
		"  exit          - continue Windows boot",
		""
	};
    
    row_count = sizeof(help_message)/sizeof(char *);
    
    /* if there are more then HELP_DISPLAY_ROWS to display,
    we must further reduce the page limit */
    page_limit = (row_count > HELP_DISPLAY_ROWS) ? HELP_DISPLAY_ROWS-3 : HELP_DISPLAY_ROWS;
                
	for(i = 0; i < row_count; i++){
        winx_printf("%s\n",help_message[i]);
        
        if((i > 0) && ((i % page_limit) == 0)){
            winx_printf("\n%s\n", DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY);
            while(1){
                if(winx_kbhit(100) >= 0) break;
            }
            winx_printf("\n");
        }
    }
}

void display_history(winx_history *h)
{
	winx_history_entry *entry;
    int i, page_limit, row_count;
	
	if(h == NULL) return;
	if(h->head == NULL) return;

	winx_printf("\n");

    row_count = h->n_entries + 2;
    /* if there are more then HELP_DISPLAY_ROWS to display,
    we must further reduce the page limit */
    page_limit = (row_count > HELP_DISPLAY_ROWS) ? HELP_DISPLAY_ROWS-3 : HELP_DISPLAY_ROWS;
  
	entry = h->head;
	for(i = 0; i < row_count; i++){
		switch(i){
		case 0:
			winx_printf("Typed commands history:\n");
			break;
		case 1:
			winx_printf("\n");
			break;
		default:
			if(entry->string)
				winx_printf("%s\n",entry->string);
			entry = entry->next_ptr;
			if(entry == h->head) return;
			break;
		}
        
        if((i > 0) && ((i % page_limit) == 0)){
            winx_printf("\n%s\n", DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY);
            while(1){
                if(winx_kbhit(100) >= 0) break;
            }
            winx_printf("\n");
        }
    }
}

char *strings[] = {
	"Sherlock Holmes took his bottle from the corner of the mantelpiece, "
	"and his hypodermic syringe from its neat morocco case. With his long, "
	"white, nervous fingers he adjusted the delicate needle and rolled back "
	"his left shirtcuff. For some little time his eyes rested thoughtfully "
	"upon the sinewy forearm and wrist, all dotted and scarred with innumerable "
	"puncture-marks. Finally, he thrust the sharp point home, pressed down the tiny "
	"piston, and sank back into the velvet-lined armchair with a long sigh of satisfaction.",
	"",
	"Three times a day for many months I had witnessed this performance, "
	"but custom had not reconciled my mind to it. On the contrary, from "
	"day to day I had become more irritable at the sight, and my conscience "
	"swelled nightly within me at the thought that I had lacked the courage "
	"to protest. Again and again I had registered a vow that I should deliver "
	"my soul upon the subject; but there was that in the cool, nonchalant air "
	"of my companion which made him the last man with whom one would care to "
	"take anything approaching to a liberty. His great powers, his masterly manner, "
	"and the experience which I had had of his many extraordinary qualities, all "
	"made me diffident and backward in crossing him. ",
	"",
	"Yet upon that afternoon, whether it was the Beaune which I had taken with "
	"my lunch or the additional exasperation produced by the extreme deliberation "
	"of his manner, I suddenly felt that I could hold out no longer. ",
	"",
	"this_is_a_very_very_long_word_qwertyuiop[]asdfghjkl;'zxcvbnm,./1234567890~!@#$#$#%$^%$^%&^&(*&(**)(*",
	"",
	"first line \rsecond\nthird\r\n4th\n\r5th\n",
	"",
	"before_tab\tafter_tab\t\tafter_two_tabs",
	"",
	NULL
};

void __stdcall NtProcessStartup(PPEB Peb)
{
	int safe_mode, error_code, result, i;
	/*
	* 60 characters long to ensure that escape and backspace
	* keys will work properly with winx_prompt() function.
	*/
	char buffer[MAX_LINE_WIDTH + 1];
	winx_history h;

	/* 1. Initialization */
#ifdef USE_INSTEAD_SMSS
	(void)NtInitializeRegistry(0/*FALSE*/); /* saves boot log etc. */
#endif

	/*
	* Since v4.3.0 this app is monolithic
	* to reach the highest level of reliability.
	* It was not reliable before because crashed
	* the system in case of missing DLL's.
	*/
	udefrag_monolithic_native_app_init();

	/* 2. Display Copyright */
	/* if(winx_get_os_version() < 51) */
	winx_printf("\n\n");
	winx_printf(VERSIONINTITLE " native interface\n"
		"Copyright (c) Dmitri Arkhangelski, 2007-2010.\n"
		"Copyright (c) Stefan Pendl, 2010.\n\n"
		"UltraDefrag comes with ABSOLUTELY NO WARRANTY.\n\n"
		"If something is wrong, hit F8 on startup\n"
		"and select 'Last Known Good Configuration'.\n\n");
		
	/*
	* In Windows Safe Mode the boot time defragmenter
	* is absolutely useless, because it cannot display
	* text on the screen in this mode.
	*/
	safe_mode = winx_windows_in_safe_mode();
	/*
	* In case of errors we'll assume that we're running in Safe Mode.
	* To avoid a very unpleasant situation when ultradefrag works
	* in background and nothing happens on the black screen.
	*/
	if(safe_mode > 0 || safe_mode < 0){
		/* display yet a message, for debugging purposes */
		winx_printf("In Windows Safe Mode this program is useless!\n");
		/* if someone will see the message, a little delay will help him to read it */
		short_dbg_delay();
		udefrag_monolithic_native_app_unload();
		winx_exit(0);
	}
	
	/*
	* Check for the pending boot-off command.
	*/
	if(ExecPendingBootOff()){
#ifndef USE_INSTEAD_SMSS
		winx_printf("Good bye ...\n");
		short_dbg_delay();
		udefrag_monolithic_native_app_unload();
		winx_exit(0);
#endif
	}
		
	if(winx_init(Peb) < 0){
		winx_printf("Wait 10 seconds ...\n");
		long_dbg_delay();
		udefrag_monolithic_native_app_unload();
		winx_exit(1);
	}
	/* 3. Prompt to exit */
	winx_printf("\nPress any key to exit ");
	for(i = 0; i < 5; i++){
		if(winx_kbhit(1000) >= 0){
			winx_printf("\nGood bye ...\n");
			udefrag_monolithic_native_app_unload();
			winx_exit(0);
		}
		//winx_printf("%c ",(char)('0' + 10 - i));
		winx_printf(".");
	}
	winx_printf("\n\n");

	/* 4. Initialize UltraDefrag driver */
	error_code = udefrag_init();
	if(error_code < 0){
		winx_printf("\nInitialization failed!\n");
		winx_printf("%s\n",udefrag_get_error_description(error_code));
		winx_printf("Wait 10 seconds ...\n");
		long_dbg_delay(); /* show error message at least 10 seconds */
		winx_printf("Good bye ...\n");
		(void)udefrag_unload();
		udefrag_monolithic_native_app_unload();
		winx_exit(1);
	}

	/* 5. Batch script processing */
	ProcessScript();

	/* 6. Command Loop */
	winx_printf("\nInteractive mode:\nType 'help' for a list of supported commands.\n");
	winx_printf("\nOnly the English keyboard layout is available.\n\n");
	scripting_mode = FALSE;
	winx_init_history(&h);
	while(1){
		/* get user input */
		if(winx_prompt_ex("# ",buffer,sizeof(buffer) - 1,&h) < 0) break;
		/* handle help command */
		if(!strcmp(buffer,"help")){
			display_help();
			continue;
		}
		/* handle history command */
		if(!strcmp(buffer,"history")){
			display_history(&h);
			continue;
		}
		/* handle test command */
		if(!strcmp(buffer,"test")){
			winx_print_array_of_strings(strings,MAX_LINE_WIDTH,
				HELP_DISPLAY_ROWS,DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,1);
			continue;
		}
		/* handle exit command */
		if(!strcmp(buffer,"exit")) break;
		/* execute command */
		result = _snwprintf(line_buffer,sizeof(line_buffer) / sizeof(short),L"%hs",buffer);
		if(result < 0){
			winx_printf("Command line is too long!\n");
			continue;
		}
		line_buffer[sizeof(line_buffer) / sizeof(short) - 1] = 0;
		ParseCommand();
	}

	winx_printf("Good bye ...\n");
	winx_destroy_history(&h);
	(void)udefrag_unload();
	udefrag_monolithic_native_app_unload();
	winx_exit(0);
	return;
}
