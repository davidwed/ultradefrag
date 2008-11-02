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
* GUI Launcher.
*/

#include <windows.h>
#include <string.h>
#include <shellapi.h>

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	char buffer[MAX_PATH];
	unsigned long result;

	GetSystemDirectory(buffer,MAX_PATH);
	strcat(buffer,"\\udefrag-gui.cmd");
	result = (unsigned long)(LONG_PTR)ShellExecute(NULL,NULL,buffer,NULL,NULL,SW_HIDE);

	if(result == SE_ERR_FNF || result == SE_ERR_PNF){
		MessageBox(NULL,"udefrag-gui.cmd file not found!","Error!",MB_OK | MB_ICONHAND);
		return 1;
	}
	if(result == SE_ERR_ACCESSDENIED || result == SE_ERR_SHARE){
		MessageBox(NULL,"udefrag-gui.cmd access denied!","Error!",MB_OK | MB_ICONHAND);
		return 1;
	}
	if(result == SE_ERR_OOM){
		MessageBox(NULL,"No enough memory!","Error!",MB_OK | MB_ICONHAND);
		return 1;
	}
	if(result == SE_ERR_ASSOCINCOMPLETE || result == SE_ERR_NOASSOC){
		MessageBox(NULL,"CMD file association not registered!","Error!",MB_OK | MB_ICONHAND);
		return 1;
	}
	return 0;
}
