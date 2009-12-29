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
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <commctrl.h>

#include "../include/ultradfgver.h"

int h_flag = 0, r_flag = 0, u_flag = 0;
int silent = 0;
int invalid_opts = 0;
char cmd[MAX_PATH + 1] = "";

void show_help(void)
{
	MessageBox(0,
		"Usage:\n"
		"bootexctrl /r [/s] command - register command\n"
		"bootexctrl /u [/s] command - unregister command\n"
		"Specify /s option to run the program in silent mode."
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
		if(!silent) MessageBox(0,"Cannot open SMSS key!","Error",MB_OK | MB_ICONHAND);
		return 0;
	}
	return 1;
}

int register_cmd(void)
{
	HKEY hKey;
	DWORD type, size;
	char *data, *curr_pos;
	DWORD i, length, curr_len;

	if(!open_smss_key(&hKey)) return 3;
	type = REG_MULTI_SZ;
	RegQueryValueEx(hKey,"BootExecute",NULL,&type,NULL,&size);
	data = malloc(size + strlen(cmd) + 10);
	if(!data){
		RegCloseKey(hKey);
		if(!silent) MessageBox(0,"No enough memory!","Error",MB_OK | MB_ICONHAND);
		return 4;
	}

	type = REG_MULTI_SZ;
	if(RegQueryValueEx(hKey,"BootExecute",NULL,&type,
			data,&size) != ERROR_SUCCESS){
		RegCloseKey(hKey);
		free(data);
		if(!silent) MessageBox(0,"Cannot query BootExecute value!","Error",MB_OK | MB_ICONHAND);
		return 5;
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
		if(!silent) MessageBox(0,"Cannot set BootExecute value!","Error",MB_OK | MB_ICONHAND);
		return 6;
	}
done:
	RegCloseKey(hKey);
	free(data);
	return 0;
}

int unregister_cmd(void)
{
	HKEY hKey;
	DWORD type, size;
	char *data, *new_data, *curr_pos;
	DWORD i, length, new_length, curr_len;

	if(!open_smss_key(&hKey)) return 7;
	type = REG_MULTI_SZ;
	RegQueryValueEx(hKey,"BootExecute",NULL,&type,NULL,&size);
	data = malloc(size + strlen(cmd) + 10);
	if(!data){
		RegCloseKey(hKey);
		if(!silent) MessageBox(0,"No enough memory!","Error",MB_OK | MB_ICONHAND);
		return 8;
	}

	type = REG_MULTI_SZ;
	if(RegQueryValueEx(hKey,"BootExecute",NULL,&type,
			data,&size) != ERROR_SUCCESS){
		RegCloseKey(hKey);
		free(data);
		if(!silent) MessageBox(0,"Cannot query BootExecute value!","Error",MB_OK | MB_ICONHAND);
		return 9;
	}

	new_data = malloc(size);
	if(!new_data){
		RegCloseKey(hKey);
		free(data);
		if(!silent) MessageBox(0,"No enough memory!","Error",MB_OK | MB_ICONHAND);
		return 10;
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
		if(!silent) MessageBox(0,"Cannot set BootExecute value!","Error",MB_OK | MB_ICONHAND);
		return 11;
	}

	RegCloseKey(hKey);
	free(data);
	free(new_data);
	return 0;
}

int parse_cmdline(void)
{
	int argc;
	short **argv;
	int i;
	short *param;

	argv = (short **)CommandLineToArgvW(GetCommandLineW(),&argc);
	if(!argv) return (-1);

	if(argc < 3){
		h_flag = 1;
		GlobalFree(argv);
		return 0;
	}
	for(i = 1; i < argc; i++){
		param = argv[i];
		_wcslwr(param);
		if(!wcscmp(param,L"/r")) r_flag = 1;
		else if(!wcscmp(param,L"/u")) u_flag = 1;
		else if(!wcscmp(param,L"/h")) h_flag = 1;
		else if(!wcscmp(param,L"/?")) h_flag = 1;
		else if(!wcscmp(param,L"/s")) silent = 1;
		else {
			if(wcslen(param) > MAX_PATH){
				invalid_opts = 1;
				if(!silent) MessageBox(0,"Command name is too long!","Error",MB_OK | MB_ICONHAND);
			} else _snprintf(cmd,MAX_PATH,"%ws",param);
		}
	}
	cmd[MAX_PATH] = 0;
	GlobalFree(argv);
	return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	InitCommonControls(); /* strongly required! to be compatible with manifest */

	if(parse_cmdline() < 0) return 2;
	if(invalid_opts) return 1;
	if(h_flag || !cmd[0]) show_help();
	else if(r_flag) return register_cmd();
	else if(u_flag) return unregister_cmd();
	else show_help();
	return 0;
}
