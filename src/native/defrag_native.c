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
	winx_printf("Interactive mode commands:\n"
				"  udefrag -a X: - volume analysis\n"
				"  udefrag X:    - volume defragmentation\n"
				"  udefrag -o X: - volume optimization\n\n"
				"  Multiple volume letters allowed,\n"
				"  --all, --all-fixed keys are supported here too.\n\n"
				"  udefrag -l    - displays list of volumes except removable\n"
				"  udefrag -la   - displays list of all available volumes\n"
				"  exit          - continue Windows boot\n"
				"  reboot        - reboot the PC\n"
				"  shutdown      - shut down the PC\n"
				"  boot-on       - enable boot time defragger\n"
				"  boot-off      - disable boot time defragger\n"
				"  help          - display this help screen\n"
				);
}

void __stdcall NtProcessStartup(PPEB Peb)
{
	int safe_mode, error_code, result, i;
	/*
	* 60 characters long to ensure that escape and backspace
	* keys will work properly with winx_prompt() function.
	*/
	char buffer[60];

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
		"Copyright (c) Dmitri Arkhangelski, 2007-2010.\n\n"
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
	while(1){
		/* get user input */
		if(winx_prompt("# ",buffer,sizeof(buffer) - 1) < 0) break;
		/* check for help command */
		if(!strcmp(buffer,"help")){
			display_help();
			continue;
		}
		/* execute command */
		result = _snwprintf(line_buffer,sizeof(line_buffer) / sizeof(short),L"%hs",buffer);
		if(result < 0){
			winx_printf("Command line is too long!\n");
			continue;
		}
		line_buffer[sizeof(line_buffer) / sizeof(short) - 1] = 0;
		ParseCommand();
		if(!strcmp(buffer,"exit")) break;
	}

	winx_printf("Good bye ...\n");
	(void)udefrag_unload();
	udefrag_monolithic_native_app_unload();
	winx_exit(0);
	return;
}
