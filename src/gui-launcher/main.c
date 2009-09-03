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

/*
* NOTE: Useless since the 3.2.0 version of the program.
*/


#include <windows.h>
#include "resource.h"

int StartGUI(void)
{
	char buffer[MAX_PATH];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	GetSystemDirectory(buffer,MAX_PATH);
	SetCurrentDirectory(buffer);

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	ZeroMemory( &pi, sizeof(pi) );

	if(!CreateProcess("cmd.exe","cmd.exe /C udefrag-gui.cmd",
		NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)){
	    MessageBox(NULL,"Can't execute udefrag-gui.cmd!","Error",MB_OK | MB_ICONHAND);
	    return 1;
	}
	WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}

BOOL CALLBACK EmptyDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg){
	case WM_INITDIALOG:
		/* kill our window before showing them :) */
		EndDialog(hWnd,1);
		return FALSE;
	case WM_CLOSE:
		/* this code - for extraordinary cases */
		EndDialog(hWnd,1);
		return TRUE;
	}
	return FALSE;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	/*
	* To disable the sand glass on the cursor
	* we must show a window on startup.
	*/
	DialogBox(hInst,MAKEINTRESOURCE(IDD_EMPTY),NULL,(DLGPROC)EmptyDlgProc);
	/* start UltraDefrag GUI */
	return StartGUI();
}
