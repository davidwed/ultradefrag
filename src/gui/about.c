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

extern HINSTANCE hInstance;
extern HWND hWindow;
extern RECT win_rc;
extern HFONT hFont;

BOOL CALLBACK AboutDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	/* When a portable app launches gui the current directory points to a temp dir. */
//	char buf[MAX_PATH];
	HWND hChild;
	RECT rc;

	switch(msg){
	case WM_INITDIALOG:
		/* Window Initialization */
		/* WgxCenterWindow(HWND hWindow,HWND hParent); */
		/* WgxCenterWindowRect(LPRECT WinRc,LPRECT ParentRc); */
		GetWindowRect(hWnd,&rc);
		if((win_rc.right - win_rc.left) < (rc.right - rc.left) || 
		  (win_rc.bottom - win_rc.top) < (rc.bottom - rc.top))
			SetWindowPos(hWnd,0,win_rc.left + 98,win_rc.top + 140,0,0,SWP_NOSIZE);
		else
			SetWindowPos(hWnd,0,
				win_rc.left + ((win_rc.right - win_rc.left) - (rc.right - rc.left)) / 2,
				win_rc.top + ((win_rc.bottom - win_rc.top) - (rc.bottom - rc.top)) / 2,
				0,0,SWP_NOSIZE
			);
		SetText(hWnd,L"ABOUT_WIN_TITLE");
		SetText(GetDlgItem(hWnd,IDC_CREDITS),L"CREDITS");
		SetText(GetDlgItem(hWnd,IDC_LICENSE),L"LICENSE");
		if(hFont){
			SendMessage(hWnd,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
			hChild = GetWindow(hWnd,GW_CHILD);
			while(hChild){
				SendMessage(hChild,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
				hChild = GetWindow(hChild,GW_HWNDNEXT);
			}
		}
		WgxAddAccelerators(hInstance,hWnd,IDR_ACCELERATOR2);
		return FALSE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_CREDITS:
			//GetWindowsDirectory(buf,MAX_PATH);
			//strcat(buf,"\\UltraDefrag\\CREDITS.TXT");
			ShellExecute(hWindow,"open",".\\CREDITS.TXT"/*buf*/,NULL,NULL,SW_SHOW);
			break;
		case IDC_LICENSE:
			//GetWindowsDirectory(buf,MAX_PATH);
			//strcat(buf,"\\UltraDefrag\\LICENSE.TXT");
			ShellExecute(hWindow,"open",".\\LICENSE.TXT"/*buf*/,NULL,NULL,SW_SHOW);
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
