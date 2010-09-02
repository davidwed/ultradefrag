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
* UltraDefrag native interface - native commands implementation.
*/

#include "defrag_native.h"

/**
 * @brief Defines whether @echo is on or not.
 */
int echo_flag = DEFAULT_ECHO_FLAG;

/*
* boot-off command modifies registry, 
* but shutdown\reboot commands releases its effect,
* because all registry modifications becomes lost.
* Therefore we track pending boot-off request through
* a special pending-boot-off file in windows directory.
*/
int pending_boot_off = 0;

/**
 * @brief Defines whether command is executed
 * in scripting mode or in interactive mode.
 */
int scripting_mode = 1;

/* help screen */
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
	"",
	NULL
};

extern winx_history history;
extern int exit_flag;

typedef int (__cdecl *cmd_handler_proc)(int argc,short **argv,short **envp);
typedef struct {
	short *cmd_name;
	cmd_handler_proc cmd_handler;
} cmd_table_entry;
int __cdecl boot_off_handler(int argc,short **argv,short **envp);
int __cdecl udefrag_handler(int argc,short **argv,short **envp);

/**
 * @brief help command handler.
 */
int __cdecl help_handler(int argc,short **argv,short **envp)
{
	(void)argc;
	(void)argv;
	(void)envp;

	return winx_print_array_of_strings(help_message,
		MAX_LINE_WIDTH,MAX_DISPLAY_ROWS,
		DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,
		scripting_mode ? 0 : 1);
}

/**
 * @brief history command handler.
 */
int __cdecl history_handler(int argc,short **argv,short **envp)
{
	winx_history_entry *entry;
	char **strings;
	int i, result;

	(void)argc;
	(void)argv;
	(void)envp;
	
	if(scripting_mode || history.head == NULL)
		return 0;
	
	/* convert list of strings to array */
	strings = winx_heap_alloc((history.n_entries + 1) * sizeof(char *));
	if(strings == NULL){
		winx_printf("\n%ws: Cannot allocate %u bytes of memory!\n\n",
			argv[0],(history.n_entries + 1) * sizeof(char *));
		return (-1);
	}
	i = 0;
	for(entry = history.head; i < history.n_entries; entry = entry->next_ptr){
		if(entry->string){
			strings[i] = entry->string;
			i++;
		}
		if(entry->next_ptr == history.head) break;
	}
	strings[i] = NULL;
	
	winx_printf("\nTyped commands history:\n\n");

	result = winx_print_array_of_strings(strings,
		MAX_LINE_WIDTH,MAX_DISPLAY_ROWS - 2,
		DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,1);

	winx_heap_free(strings);
	return result;
}

/**
 * @brief echo command handler.
 */
int __cdecl echo_handler(int argc,short **argv,short **envp)
{
	int i;
	
	(void)envp;
	
	/* check whether an empty line is requested */
	if(!wcscmp(argv[0],L"echo.")){
		winx_printf("\n");
		return 0;
	}
	
	/* check whether @echo status is requested */
	if(argc < 2){
		if(echo_flag)
			winx_printf("@echo is on\n");
		else
			winx_printf("@echo is off\n");
		return 0;
	}
	
	/* handle @echo command */
	if(!wcscmp(argv[0],L"@echo")){
		if(!wcscmp(argv[1],L"on")){
			echo_flag = 1;
			return 0;
		} else if(!wcscmp(argv[1],L"off")){
			echo_flag = 0;
			return 0;
		}
		/* treat as simple echo */
	}
	
	/* handle echo command */
	for(i = 1; i < argc; i++){
		winx_printf("%ws",argv[i]);
		if(i != argc - 1)
			winx_printf(" ");
	}
	winx_printf("\n");
	return 0;
}

/**
 * @brief type command handler.
 */
int __cdecl type_handler(int argc,short **argv,short **envp)
{
	char path[MAX_PATH];
	short *filename;
	int i, length;
	size_t filesize;
	unsigned char *buffer, *second_buffer;
	int unicode_detected;
	char *strings[] = { NULL, NULL };
	
	(void)envp;
	
	/* display boot time script if filename is missing */
	if(argc < 2){
		if(winx_get_windows_directory(path,MAX_PATH) < 0){
			winx_printf("\n%ws: Cannot retrieve the Windows directory path!\n\n",argv[0]);
			return (-1);
		}
		(void)strncat(path,"\\system32\\ud-boot-time.cmd",
				MAX_PATH - strlen(path) - 1);
	} else {
		length = 0;
		for(i = 1; i < argc; i++)
			length += wcslen(argv[i]) + 1;
		filename = winx_heap_alloc(length * sizeof(short));
		if(filename == NULL){
			winx_printf("\n%ws: Cannot allocate %u bytes of memory!\n\n",
				argv[0],length * sizeof(short));
			return (-1);
		}
		filename[0] = 0;
		for(i = 1; i < argc; i++){
			wcscat(filename,argv[i]);
			if(i != argc - 1)
				wcscat(filename,L" ");
		}
		(void)_snprintf(path,MAX_PATH - 1,"\\??\\%ws",filename);
		path[MAX_PATH - 1] = 0;
		winx_heap_free(filename);
	}
	(void)filename;

	/* read file contents entirely */
	buffer = winx_get_file_contents(path,&filesize);
	if(buffer == NULL)
		return 0; /* file is empty or some error */
	
	/* terminate buffer by two zeros */
	buffer[filesize] = buffer[filesize + 1] = 0;

	/* check for UTF-16 signature which exists in files edited in Notepad */
	unicode_detected = 0;
	if(filesize >= sizeof(short)){
		if(buffer[0] == 0xFF && buffer[1] == 0xFE)
			unicode_detected = 1;
	}

	/* print file contents */
	if(unicode_detected){
		second_buffer = winx_heap_alloc(filesize + 1);
		if(second_buffer == NULL){
			winx_printf("\n%ws: Cannot allocate %u bytes of memory!\n\n",
				argv[0],filesize + 1);
			winx_release_file_contents(buffer);
			return (-1);
		}
		(void)_snprintf(second_buffer,filesize + 1,"%ws",(short *)(buffer + 2));
		second_buffer[filesize] = 0;
		strings[0] = second_buffer;
		winx_print_array_of_strings(strings,MAX_LINE_WIDTH,
			MAX_DISPLAY_ROWS,DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,
			scripting_mode ? 0 : 1);
		winx_heap_free(second_buffer);
	} else {
		strings[0] = buffer;
		winx_print_array_of_strings(strings,MAX_LINE_WIDTH,
			MAX_DISPLAY_ROWS,DEFAULT_PAGING_PROMPT_TO_HIT_ANY_KEY,
			scripting_mode ? 0 : 1);
	}
	
	/* cleanup */
	winx_release_file_contents(buffer);
	return 0;
}

/**
 * @brief set command handler.
 */
int __cdecl set_handler(int argc,short **argv,short **envp)
{
	int name_length = 0, value_length = 0;
	short *name = NULL, *value = NULL;
	int i, j, n, result;
	
	/*
	* Check whether environment variables
	* listing is requested or not.
	*/
	if(argc < 2){
		/* list all environment variables */
	} else {
		/* the first parameter must contain '=' character */
		if(!wcschr(argv[1],'=')){
			winx_printf("\n%ws: invalid syntax!\n\n",argv[0]);
			return (-1);
		}
		/* calculate name and value lengths */
		n = wcslen(argv[1]);
		for(i = 0; i < n; i++){
			if(argv[1][i] == '='){
				name_length = i;
				value_length = n - i - 1;
				break;
			}
		}
		/* validate '=' character position */
		if(name_length == 0 || (value_length == 0 && argc >= 3)){
			winx_printf("\n%ws: invalid syntax!\n\n",argv[0]);
			return (-1);
		}
		/* append all remaining parts of the value string */
		for(i = 2; i < argc; i++)
			value_length += 1 + wcslen(argv[i]);
		/* allocate memory */
		name = winx_heap_alloc((name_length + 1) * sizeof(short));
		if(name == NULL){
			winx_printf("\n%ws: Cannot allocate %u bytes of memory!\n",
				argv[0],(name_length + 1) * sizeof(short));
			return (-1);
		}
		if(value_length){
			value = winx_heap_alloc((value_length + 1) * sizeof(short));
			if(value == NULL){
				winx_printf("\n%ws: Cannot allocate %u bytes of memory!\n",
					argv[0],(value_length + 1) * sizeof(short));
				winx_heap_free(name);
				return (-1);
			}
		}
		/* extract name and value */
		n = wcslen(argv[1]);
		for(i = 0; i < n; i++){
			if(argv[1][i] == '=') break;
			name[i] = argv[1][i];
		}
		name[i] = 0;
		if(value_length){
			for(i++, j = 0; i < n; i++){
				value[j] = argv[1][i];
				j++;
			}
			value[j] = 0;
			for(i = 2; i < argc; i++){
				wcscat(value,L" ");
				wcscat(value,argv[i]);
			}
		}
		if(value_length){
			/* set environment variable */
			result = winx_set_env_variable(name,value);
			winx_heap_free(value);
		} else {
			/* clear environment variable */
			result = winx_set_env_variable(name,NULL);
		}
		winx_heap_free(name);
		return result;
	}
	
	return 0;
}

/**
 * @brief pause command handler.
 */
int __cdecl pause_handler(int argc,short **argv,short **envp)
{
	int msec;

	(void)envp;
	
	/* check whether "Hit any key to continue..." prompt is requested */
	if(argc < 2){
		winx_printf("%s",PAUSE_MESSAGE);
		(void)winx_kbhit(INFINITE);
		winx_printf("\n\n");
		return 0;
	}
	
	/* pause execution for specified time interval */
	msec = (int)_wtol(argv[1]);
	winx_sleep(msec);
	return 0;
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
	if(boot_off_handler(0,NULL,NULL) < 0){
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

/**
 * @brief boot-on command handler.
 */
int __cdecl boot_on_handler(int argc,short **argv,short **envp)
{
	(void)argc;
	(void)argv;
	(void)envp;

	pending_boot_off = 0;
	
	if(winx_register_boot_exec_command(L"defrag_native") < 0){
		winx_printf("\nCannot enable the boot time defragmenter.\n\n");
		return (-1);
	}
	return 0;
}

/**
 * @brief boot-off command handler.
 * @note May be used without parameters.
 */
int __cdecl boot_off_handler(int argc,short **argv,short **envp)
{
	(void)argc;
	(void)argv;
	(void)envp;

	pending_boot_off = 1;
	
	if(winx_unregister_boot_exec_command(L"defrag_native") < 0){
		winx_printf("\nCannot disable the boot time defragmenter.\n\n");
		return (-1);
	}
	return 0;
}

/**
 * @brief shutdown command handler.
 */
int __cdecl shutdown_handler(int argc,short **argv,short **envp)
{
	(void)argc;
	(void)argv;
	(void)envp;
	
	winx_printf("Shutdown ...");
	SavePendingBootOffState();
	winx_shutdown();
	winx_printf("\nShutdown your computer manually.\n");
	return 0;
}

/**
 * @brief reboot command handler.
 */
int __cdecl reboot_handler(int argc,short **argv,short **envp)
{
	(void)argc;
	(void)argv;
	(void)envp;
	
	winx_printf("Reboot ...");
	SavePendingBootOffState();
	winx_reboot();
	winx_printf("\nReboot your computer manually.\n");
	return 0;
}

/**
 * @brief hibernate command handler.
 */
int __cdecl hibernate_handler(int argc,short **argv,short **envp)
{
	(void)argc;
	(void)argv;
	(void)envp;
	
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
	return 0;
}

/**
 * @brief exit command handler.
 * @note May be used without parameters.
 */
int __cdecl exit_handler(int argc,short **argv,short **envp)
{
	int exit_code = 0;
	
	(void)argc;
	(void)argv;
	(void)envp;
	
	if(!scripting_mode)
		exit_flag = 1;
	
	if(argc > 1)
		exit_code = _wtoi(argv[1]);
	
	winx_printf("Good bye ...\n");
	winx_destroy_history(&history);
	udefrag_monolithic_native_app_unload();
	winx_exit(exit_code);
	return 0;
}

/**
 * @brief test command handler.
 * @details Paste here any code
 * which needs to be tested.
 */
int __cdecl test_handler(int argc,short **argv,short **envp)
{
	(void)argc;
	(void)argv;
	(void)envp;
	
	winx_printf("Hi, I'm here ;-)\n");
	return 0;
}

/**
 * @brief List of supported commands.
 */
cmd_table_entry cmd_table[] = {
	{ L"@echo", echo_handler },
	{ L"boot-off", boot_off_handler },
	{ L"boot-on", boot_on_handler },
	{ L"echo", echo_handler },
	{ L"echo.", echo_handler },
	{ L"exit", exit_handler },
	{ L"help", help_handler },
	{ L"hibernate", hibernate_handler },
	{ L"history", history_handler },
	{ L"pause", pause_handler },
	{ L"reboot", reboot_handler },
	{ L"set", set_handler },
	{ L"shutdown", shutdown_handler },
	{ L"test", test_handler },
	{ L"type", type_handler },
	{ L"udefrag", udefrag_handler },
	{ L"", NULL }
};

/**
 * @brief Executes the command.
 * @param[in] cmdline the command line.
 * @return Zero for success, negative 
 * value otherwise.
 */
int parse_command(short *cmdline)
{
	int i, j, n, argc;
	int arg_detected;
	short *cmdline_copy;
	short **argv;
	short **envp;
	int result;
	
	/*
	* Cleanup the command line by removing
	* spaces and newlines from the beginning
	* and the end of the string.
	*/
	while(*cmdline == 0x20 || *cmdline == '\t')
		cmdline ++; /* skip leading spaces */
	n = wcslen(cmdline);
	for(i = n - 1; i >= 0; i--){
		if(cmdline[i] != 0x20 && cmdline[i] != '\t' && \
			cmdline[i] != '\n' && cmdline[i] != '\r') break;
		cmdline[i] = 0; /* remove trailing spaces and newlines */
	}
	
	/*
	* Handle empty lines and comments.
	*/
	if(cmdline[0] == 0 || cmdline[0] == ';' || cmdline[0] == '#'){
		if(echo_flag)
			winx_printf("%ws\n",cmdline);
		return 0;
	}
	
	/*
	* Prepare argc, argv, envp variables.
	* Return immediately if argc == 0.
	*/
	/* a. make a copy of command line */
	n = wcslen(cmdline);
	cmdline_copy = winx_heap_alloc((n + 1) * sizeof(short));
	if(cmdline_copy == NULL){
		winx_printf("\nCannot allocate %u bytes of memory for %ws command!\n\n",
			(n + 1) * sizeof(short),cmdline);
		return (-1);
	}
	wcscpy(cmdline_copy,cmdline);
	/* b. replace all spaces by zeros */
	argc = 1;
	for(i = 0; i < n; i++){
		if(cmdline_copy[i] == 0x20 || cmdline_copy[i] == '\t'){
			cmdline_copy[i] = 0;
			if(cmdline_copy[i+1] != 0x20 && cmdline_copy[i+1] != '\t')
				argc ++;
		}
	}
	/* c. allocate memory for argv array */
	argv = winx_heap_alloc(sizeof(short *) * argc);
	if(argv == NULL){
		winx_printf("\nCannot allocate %u bytes of memory for %ws command!\n\n",
			sizeof(short *) * argc,cmdline);
		winx_heap_free(cmdline_copy);
		return (-1);
	}
	/* d. fill argv array */
	j = 0; arg_detected = 0;
	for(i = 0; i < n; i++){
		if(cmdline_copy[i]){
			if(!arg_detected){
				argv[j] = cmdline_copy + i;
				j ++;
				arg_detected = 1;
			}
		} else {
			arg_detected = 0;
		}
	}
	/* e. build environment */
	envp = NULL;
	
	/*
	* Print command line if @echo is on.
	* Skip this if either @echo or echo
	* commands are executed.
	*/
	if(echo_flag && wcscmp(argv[0],L"@echo") && \
	  wcscmp(argv[0],L"echo") && wcscmp(argv[0],L"echo."))
		winx_printf("%ws\n",cmdline);
	
	/*
	* Check whether the command 
	* is supported or not.
	*/
	for(i = 0; cmd_table[i].cmd_handler != NULL; i++)
		if(!wcscmp(argv[0],cmd_table[i].cmd_name)) break;
	
	/*
	* Handle unknown commands.
	*/
	if(cmd_table[i].cmd_handler == NULL){
		winx_printf("\nUnknown command %ws!\n\n",argv[0]);
		winx_heap_free(argv);
		winx_heap_free(cmdline_copy);
		return 0;
	}
	
	/*
	* Handle the command.
	*/
	result = cmd_table[i].cmd_handler(argc,argv,envp);
	winx_heap_free(argv);
	winx_heap_free(cmdline_copy);
	return result;
}
