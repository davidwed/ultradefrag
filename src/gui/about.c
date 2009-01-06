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
* GUI - about box source code.
*/

#include "main.h"

extern HWND hWindow;
extern RECT win_rc;

BOOL CALLBACK AboutDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	/* When a portable app launches gui the current directory points to a temp dir. */
	char buf[MAX_PATH];

	switch(msg){
	case WM_INITDIALOG:
		/* Window Initialization */
		SetWindowPos(hWnd,0,win_rc.left + /*65*/98,win_rc.top + 140/*137*/,0,0,SWP_NOSIZE);
		SetText(hWnd,L"ABOUT_WIN_TITLE");
		SetText(GetDlgItem(hWnd,IDC_CREDITS),L"CREDITS");
		SetText(GetDlgItem(hWnd,IDC_LICENSE),L"LICENSE");
		return FALSE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_CREDITS:
			GetWindowsDirectory(buf,MAX_PATH);
			strcat(buf,"\\UltraDefrag\\CREDITS.TXT");
			ShellExecute(hWindow,"open",buf,NULL,NULL,SW_SHOW);
			break;
		case IDC_LICENSE:
			GetWindowsDirectory(buf,MAX_PATH);
			strcat(buf,"\\UltraDefrag\\LICENSE.TXT");
			ShellExecute(hWindow,"open",buf,NULL,NULL,SW_SHOW);
			break;
		case IDC_HOMEPAGE:
			SetFocus(GetDlgItem(hWnd,IDC_CREDITS));
			ShellExecute(hWindow,"open","http://ultradefrag.sourceforge.net",NULL,NULL,SW_SHOW);
		}
		if(LOWORD(wParam) != IDOK)
			return FALSE;
	case WM_CLOSE:
		EndDialog(hWnd,1);
		return TRUE;
	}
	return FALSE;
}
