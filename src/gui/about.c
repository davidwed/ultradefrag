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

BOOL about_win_closed;

DWORD WINAPI PlayMainTheme(LPVOID lpParameter)
{
	int i,j;
	HWND hAboutWin = (HWND)lpParameter;
	HWND hPic = GetDlgItem(hAboutWin,IDC_PIC1);
	HWND hRTL = GetDlgItem(hAboutWin,IDC_RTL);
	HWND hLTR = GetDlgItem(hAboutWin,IDC_LTR);
	SetWindowPos(hPic,0,300,8,0,0,SWP_NOSIZE);
	ShowWindow(hRTL,SW_HIDE);
	ShowWindow(hLTR,SW_HIDE);

	ShellExecute(hAboutWin,"open",".\\music\\main_theme.mid",NULL,NULL,SW_SHOW);

	for(i = 0; i < 293; i++){
		SetWindowPos(hPic,0,300 - i,32,0,0,SWP_NOSIZE);
		Sleep(15);
		if(about_win_closed) return 0;
	}

	while(!about_win_closed){
		ShowWindow(hRTL,SW_HIDE);
		ShowWindow(hLTR,SW_SHOWNORMAL);
		for(j = 380; j > -120; j--){
			SetWindowPos(hLTR,0,300 - j,182,0,0,SWP_NOSIZE);
			Sleep(30);
			if(about_win_closed) return 0;
		}
		ShowWindow(hLTR,SW_HIDE);
		ShowWindow(hRTL,SW_SHOWNORMAL);
		for(j = -120; j < 380; j++){
			SetWindowPos(hRTL,0,300 - j,0,0,0,SWP_NOSIZE);
			Sleep(30);
			if(about_win_closed) return 0;
		}
	}
	return 0;
}

BOOL CALLBACK AboutDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	DWORD th_id;
	HDC hdc;
	
	switch(msg){
	case WM_INITDIALOG:
		/* Window Initialization */
		SetWindowPos(hWnd,0,win_rc.left + 65,win_rc.top + /*137*/97,0,0,SWP_NOSIZE);
		SetText(hWnd,L"ABOUT_WIN_TITLE");
		SetText(GetDlgItem(hWnd,IDC_CREDITS),L"CREDITS");
		SetText(GetDlgItem(hWnd,IDC_LICENSE),L"LICENSE");
		hdc = GetDC(hWnd);
		SetBkMode(hdc,TRANSPARENT);
		ReleaseDC(hWnd,hdc);
		about_win_closed = FALSE;
		create_thread(PlayMainTheme,(void *)(LONG_PTR)hWnd,&th_id);
		return FALSE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_CREDITS:
			ShellExecute(hWindow,"open",".\\CREDITS.TXT",NULL,NULL,SW_SHOW);
			break;
		case IDC_LICENSE:
			ShellExecute(hWindow,"open",".\\LICENSE.TXT",NULL,NULL,SW_SHOW);
			break;
		case IDC_HOMEPAGE:
			SetFocus(GetDlgItem(hWnd,IDC_CREDITS));
			ShellExecute(hWindow,"open","http://ultradefrag.sourceforge.net",NULL,NULL,SW_SHOW);
		}
		if(LOWORD(wParam) != IDOK)
			return FALSE;
	case WM_CLOSE:
		about_win_closed = TRUE;
		EndDialog(hWnd,1);
		return TRUE;
	}
	return FALSE;
}
