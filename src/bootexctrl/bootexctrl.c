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
* BootExecute Control program.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "../include/ultradfgver.h"

int h_flag = 0, r_flag = 0, u_flag = 0;
int invalid_opts = 0;
char cmd[MAX_PATH + 1] = "";

void show_help(void)
{
	MessageBox(0,
		"Usage:\n"
		"bootexctrl /r command - register command\n"
		"bootexctrl /u command - unregister command"
		,
		"BootExecute Control",
		MB_OK
		);
}

int open_smss_key(HKEY *phkey)
{
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\Session Manager",
			0,
			KEY_QUERY_VALUE | KEY_SET_VALUE,
			phkey) != ERROR_SUCCESS){
		MessageBox(0,"Cannot open SMSS key!","Error",MB_OK | MB_ICONHAND);
		return 0;
	}
	return 1;
}

void register_cmd(void)
{
	HKEY hKey;
	DWORD type, size;
	char *data, *curr_pos;
	DWORD i, length, curr_len;

	if(!open_smss_key(&hKey)) return;
	type = REG_MULTI_SZ;
	RegQueryValueEx(hKey,"BootExecute",NULL,&type,NULL,&size);
	data = malloc(size + strlen(cmd) + 10);
	if(!data){
		RegCloseKey(hKey);
		MessageBox(0,"No enough memory!","Error",MB_OK | MB_ICONHAND);
		return;
	}

	type = REG_MULTI_SZ;
	if(RegQueryValueEx(hKey,"BootExecute",NULL,&type,
			data,&size) != ERROR_SUCCESS){
		RegCloseKey(hKey);
		free(data);
		MessageBox(0,"Cannot query BootExecute value!","Error",MB_OK | MB_ICONHAND);
		return;
	}

	length = size - 1;
	for(i = 0; i < length;){
		curr_pos = data + i;
		curr_len = strlen(curr_pos) + 1;
		/* if the command is yet registered then exit */
		if(!strcmp(curr_pos,cmd)) goto done;
		i += curr_len;
	}
	strcpy(data + i,cmd);
	data[i + strlen(cmd) + 1] = 0;
	length += strlen(cmd) + 1 + 1;

	if(RegSetValueEx(hKey,"BootExecute",0,REG_MULTI_SZ,
			data,length) != ERROR_SUCCESS){
		RegCloseKey(hKey);
		free(data);
		MessageBox(0,"Cannot set BootExecute value!","Error",MB_OK | MB_ICONHAND);
		return;
	}
done:
	RegCloseKey(hKey);
	free(data);
}

void unregister_cmd(void)
{
	HKEY hKey;
	DWORD type, size;
	char *data, *new_data, *curr_pos;
	DWORD i, length, new_length, curr_len;

	if(!open_smss_key(&hKey)) return;
	type = REG_MULTI_SZ;
	RegQueryValueEx(hKey,"BootExecute",NULL,&type,NULL,&size);
	data = malloc(size + strlen(cmd) + 10);
	if(!data){
		RegCloseKey(hKey);
		MessageBox(0,"No enough memory!","Error",MB_OK | MB_ICONHAND);
		return;
	}

	type = REG_MULTI_SZ;
	if(RegQueryValueEx(hKey,"BootExecute",NULL,&type,
			data,&size) != ERROR_SUCCESS){
		RegCloseKey(hKey);
		free(data);
		MessageBox(0,"Cannot query BootExecute value!","Error",MB_OK | MB_ICONHAND);
		return;
	}

	new_data = malloc(size);
	if(!new_data){
		RegCloseKey(hKey);
		free(data);
		MessageBox(0,"No enough memory!","Error",MB_OK | MB_ICONHAND);
		return;
	}

	/*
	* Now we should copy all strings except specified command
	* to the destination buffer.
	*/
	memset((void *)new_data,0,size);
	length = size - 1;
	new_length = 0;
	for(i = 0; i < length;){
		curr_pos = data + i;
		curr_len = strlen(curr_pos) + 1;
		if(strcmp(curr_pos,cmd)){
			strcpy(new_data + new_length,curr_pos);
			new_length += curr_len;
		}
		i += curr_len;
	}
	new_data[new_length] = 0;

	if(RegSetValueEx(hKey,"BootExecute",0,REG_MULTI_SZ,
			new_data,new_length + 1) != ERROR_SUCCESS){
		RegCloseKey(hKey);
		free(data);
		free(new_data);
		MessageBox(0,"Cannot set BootExecute value!","Error",MB_OK | MB_ICONHAND);
		return;
	}

	RegCloseKey(hKey);
	free(data);
	free(new_data);
}

void parse_cmdline(int argc, char **argv)
{
	int i;
	char *param;

	if(argc < 3){
		h_flag = 1;
		return;
	}
	for(i = 1; i < argc; i++){
		param = argv[i];
		_strlwr(param);
		if(!strcmp(param,"/r")) r_flag = 1;
		else if(!strcmp(param,"/u")) u_flag = 1;
		else if(!strcmp(param,"/h")) h_flag = 1;
		else if(!strcmp(param,"/?")) h_flag = 1;
		else {
			if(strlen(argv[i]) > MAX_PATH){
				invalid_opts = 1;
				MessageBox(0,"Command name is too long!","Error",MB_OK | MB_ICONHAND);
			} else strncpy(cmd,argv[i],MAX_PATH);
		}
	}
	cmd[MAX_PATH] = 0;
}

int __cdecl main(int argc, char **argv)
{
	parse_cmdline(argc,argv);
	if(invalid_opts) return 1;
	if(h_flag || !cmd[0]) show_help();
	else if(r_flag) register_cmd();
	else if(u_flag) unregister_cmd();
	else show_help();
	return 0;
}

/* Copyright (C) Alexander A. Telyatnikov (http://alter.org.ua/) */
PCHAR*
CommandLineToArgvA(
	PCHAR CmdLine,
	int* _argc
	)
{
	PCHAR* argv;
	PCHAR  _argv;
	ULONG   len;
	ULONG   argc;
	CHAR   a;
	ULONG   i, j;

	BOOLEAN  in_QM;
	BOOLEAN  in_TEXT;
	BOOLEAN  in_SPACE;

	len = strlen(CmdLine);
	i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

	argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
		i + (len+2)*sizeof(CHAR));

	/* added by dmitriar */
	if(!argv){
		*_argc = 0;
		return NULL;
	}

	_argv = (PCHAR)(((PUCHAR)argv)+i);

	argc = 0;
	argv[argc] = _argv;
	in_QM = FALSE;
	in_TEXT = FALSE;
	in_SPACE = TRUE;
	i = 0;
	j = 0;

	while( (a = CmdLine[i]) ) {
		if(in_QM) {
			if(a == '\"') {
				in_QM = FALSE;
			} else {
				_argv[j] = a;
				j++;
			}
		} else {
			switch(a) {
			case '\"':
				in_QM = TRUE;
				in_TEXT = TRUE;
				if(in_SPACE) {
					argv[argc] = _argv+j;
					argc++;
				}
				in_SPACE = FALSE;
				break;
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				if(in_TEXT) {
					_argv[j] = '\0';
					j++;
				}
				in_TEXT = FALSE;
				in_SPACE = TRUE;
				break;
			default:
				in_TEXT = TRUE;
				if(in_SPACE) {
					argv[argc] = _argv+j;
					argc++;
				}
				_argv[j] = a;
				j++;
				in_SPACE = FALSE;
				break;
			}
		}
		i++;
	}
	_argv[j] = '\0';
	argv[argc] = NULL;

	(*_argc) = argc;
	return argv;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	int argc;
	char **argv;
	int ret;
	
	argv = CommandLineToArgvA(GetCommandLineA(),&argc);
	ret = main(argc,argv);
	if(argv) GlobalFree(argv);
	return ret;
}
