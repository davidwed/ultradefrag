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

#include "../include/ntndk.h"

#include "../include/ultradfg.h"
#include "../include/udefrag-kernel.h"
#include "../include/udefrag.h"
#include "../include/ultradfgver.h"
#include "../dll/zenwinx/zenwinx.h"

/* uncomment it if you want to replace smss.exe by this program */
//#define USE_INSTEAD_SMSS

void ProcessScript(void);
void EnableNativeDefragger(void);
void DisableNativeDefragger(void);
void Hibernate(void);
void DisplayAvailableVolumes(int skip_removable);
void ProcessVolume(char letter,char _command);

void display_help(void)
{
	winx_printf("Interactive mode commands:\n"
				"  analyse X:   - volume analysis\n"
				"  defrag X:    - volume defragmentation\n"
				"  optimize X:  - volume optimization\n"
				"  drives       - displays list of available volumes\n"
				"  exit         - continue Windows boot\n"
				"  reboot       - reboot the PC\n"
				"  shutdown     - shut down the PC\n"
				"  boot-on      - enable boot time defragger;\n"
				"                 loses its effect before shutdown/reboot\n"
				"  boot-off     - disable boot time defragger;\n"
				"                 loses its effect before shutdown/reboot\n"
				"  help         - display this help screen\n"
				);
}

void __stdcall NtProcessStartup(PPEB Peb)
{
	int error_code;
	char cmd[33], arg[5];
	char buffer[256];
	int i;

	/* 1. Initialization */
#ifdef USE_INSTEAD_SMSS
	(void)NtInitializeRegistry(0/*FALSE*/); /* saves boot log etc. */
#endif
	/* 2. Display Copyright */
	if(winx_get_os_version() < 51) winx_printf("\n\n");
	winx_printf(VERSIONINTITLE " native interface\n"
		"Copyright (c) Dmitri Arkhangelski, 2007-2010.\n\n"
		"UltraDefrag comes with ABSOLUTELY NO WARRANTY.\n\n"
		"If something is wrong, hit F8 on startup\n"
		"and select 'Last Known Good Configuration'.\n"
		"Use Break key to abort defragmentation.\n\n");
	if(winx_init(Peb) < 0){
		winx_printf("Wait 10 seconds ...\n");
		winx_sleep(10000);
		winx_exit(1);
	}
	/* 3. Prompt to exit */
	winx_printf("\nPress any key to exit ");
	for(i = 0; i < 10; i++){
		if(winx_kbhit(1000) >= 0){
			winx_printf("\nGood bye ...\n");
			winx_exit(0);
		}
		//winx_printf("%c ",(char)('0' + 10 - i));
		winx_printf(".");
	}
	winx_printf("\n\n");

	/* 4. Initialize UltraDefrag driver */
	error_code = udefrag_init(0);
	if(error_code < 0){
		winx_printf("\nInitialization failed!\n");
		winx_printf("%s\n",udefrag_get_error_description(error_code));
		winx_printf("Wait 10 seconds ...\n");
		winx_sleep(10000); /* show error message at least 10 seconds */
		winx_printf("Good bye ...\n");
		(void)udefrag_unload();
		winx_exit(1);
	}

#ifdef KERNEL_MODE_DRIVER_SUPPORT
	if(udefrag_kernel_mode()) winx_printf("(Kernel Mode)\n\n");
	else winx_printf("(User Mode)\n\n");
#endif

	/* 5. Batch script processing */
	ProcessScript();

	/* 6. Command Loop */
	winx_printf("\nInteractive mode:\n"
				"Type 'help' for list of supported commands.\n\n");
	while(1){
		winx_printf("# ");
		if(winx_gets(buffer,sizeof(buffer) - 1) < 0) break;
		cmd[0] = arg[0] = 0;
		(void)sscanf(buffer,"%32s %4s",cmd,arg);

		if(!strcmp(cmd,"shutdown")){
			winx_printf("Shutdown ...");
			(void)udefrag_unload();
			winx_shutdown();
			continue;
		} else if(!strcmp(cmd,"reboot")){
			winx_printf("Reboot ...");
			(void)udefrag_unload();
			winx_reboot();
			continue;
		} else if(!strcmp(cmd,"exit")){
			break;
		} else if(!strcmp(cmd,"analyse")){
			ProcessVolume(arg[0],'a');
			continue;
		} else if(!strcmp(cmd,"defrag")){
			ProcessVolume(arg[0],'d');
			continue;
		} else if(!strcmp(cmd,"optimize")){
			ProcessVolume(arg[0],'c');
			continue;
		} else if(!strcmp(cmd,"drives")){
			DisplayAvailableVolumes(FALSE);
			continue;
		} else if(!strcmp(cmd,"help")){
			display_help();
			continue;
		} else if(!strcmp(cmd,"boot-on")){
			EnableNativeDefragger();
			continue;
		} else if(!strcmp(cmd,"boot-off")){
			DisableNativeDefragger();
			continue;
		} else if(!strcmp(cmd,"hibernate")){
			Hibernate();
			continue;
		} else {
			winx_printf("Unknown command!\n");
		}
	}
	winx_printf("Good bye ...\n");
	(void)udefrag_unload();
	winx_exit(0);
	return;
}
