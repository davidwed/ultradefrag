/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 *  x64 installation helper - it move driver from WOW64 to native system.
 */

#include <windows.h>

char path[MAX_PATH+20];

char command[] = "cmd.exe /C move /Y %WINDIR%\\SysWOW64\\Drivers\\ultradfg.sys "
		 "%WINDIR%\\system32\\Drivers\\";
char command2[] = "cmd.exe /C move /Y %WINDIR%\\SysWOW64\\defrag_native.exe "
		 "%WINDIR%\\system32\\";
char command3[] = "cmd.exe /C del /F /Q %WINDIR%\\System32\\defrag_native.exe";
char command4[] = "cmd.exe /C del /F /Q %WINDIR%\\System32\\Drivers\\ultradfg.sys";
char command5[] = "cmd.exe /C move /Y %WINDIR%\\SysWOW64\\udefrag.exe "
		 "%WINDIR%\\system32\\";
char command6[] = "cmd.exe /C del /F /Q %WINDIR%\\System32\\udefrag.exe";

void Exec(char *shell,char *command)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pinfo;

	memset(&si,0,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	if(CreateProcess(shell,command,NULL,NULL,1,0,NULL,NULL,&si,&pinfo))
	{
		CloseHandle(pinfo.hProcess);
		CloseHandle(pinfo.hThread);
	}
}

void EntryPoint(void)
{
	GetSystemDirectory(path,MAX_PATH);
	strcat(path,"\\cmd.exe");
	if(!strstr(GetCommandLine(),"-u"))
	{
		Exec(path,command); Exec(path,command2);
		Exec(path,command5);
	}
	else
	{
		Exec(path,command3); Exec(path,command4);
		Exec(path,command6);
	}
	ExitProcess(0);
}

