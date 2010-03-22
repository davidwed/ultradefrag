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
* GUI - confirm box source code.
*/

#include "main.h"

extern HINSTANCE hInstance;
extern HWND hWindow;
extern RECT win_rc;
extern HFONT hFont;
extern int hibernate_instead_of_shutdown;
extern WGX_I18N_RESOURCE_ENTRY i18n_table[];
extern int seconds_for_shutdown_rejection;

BOOL CALLBACK CheckConfirmDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
//	HWND hChild;
	RECT rc;

	switch(msg){
	case WM_INITDIALOG:
		/* Window Initialization */
		/* WgxCenterWindow(HWND hWindow,HWND hParent); */
		/* WgxCenterWindowRect(LPRECT WinRc,LPRECT ParentRc); */
		if(GetWindowRect(hWnd,&rc)){
			if((win_rc.right - win_rc.left) < (rc.right - rc.left) || 
			  (win_rc.bottom - win_rc.top) < (rc.bottom - rc.top))
				(void)SetWindowPos(hWnd,0,win_rc.left + 158,win_rc.top + 160,0,0,SWP_NOSIZE);
			else
				(void)SetWindowPos(hWnd,0,
					win_rc.left + ((win_rc.right - win_rc.left) - (rc.right - rc.left)) / 2,
					win_rc.top + ((win_rc.bottom - win_rc.top) - (rc.bottom - rc.top)) / 2 + 5,
					0,0,SWP_NOSIZE
				);
		}
		SetText(hWnd,L"PLEASE_CONFIRM");
		if(hibernate_instead_of_shutdown)
			SetText(GetDlgItem(hWnd,IDC_MESSAGE),L"REALLY_HIBERNATE_WHEN_DONE");
		else
			SetText(GetDlgItem(hWnd,IDC_MESSAGE),L"REALLY_SHUTDOWN_WHEN_DONE");
		SetText(GetDlgItem(hWnd,IDC_YES_BUTTON),L"YES");
		SetText(GetDlgItem(hWnd,IDC_NO_BUTTON), L"NO");

		/*if(hFont){
			(void)SendMessage(hWnd,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
			hChild = GetWindow(hWnd,GW_CHILD);
			while(hChild){
				(void)SendMessage(hChild,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
				hChild = GetWindow(hChild,GW_HWNDNEXT);
			}
		}*/
		return FALSE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_YES_BUTTON:
			(void)EndDialog(hWnd,1);
			break;
		case IDC_NO_BUTTON:
			(void)EndDialog(hWnd,0);
			break;
		}
		return TRUE;
	case WM_CLOSE:
		(void)EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

BOOL CALLBACK ShutdownConfirmDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
//	HWND hChild;
	RECT rc;
	static UINT timer;
	static UINT counter;
	#define TIMER_ID 0x16748382
	static short buffer[128];
	static short *message;

	switch(msg){
	case WM_INITDIALOG:
		/* Window Initialization */
		/* WgxCenterWindow(HWND hWindow,HWND hParent); */
		/* WgxCenterWindowRect(LPRECT WinRc,LPRECT ParentRc); */
		if(GetWindowRect(hWnd,&rc)){
			if((win_rc.right - win_rc.left) < (rc.right - rc.left) || 
			  (win_rc.bottom - win_rc.top) < (rc.bottom - rc.top))
				(void)SetWindowPos(hWnd,0,win_rc.left + 158,win_rc.top + 160,0,0,SWP_NOSIZE);
			else
				(void)SetWindowPos(hWnd,0,
					win_rc.left + ((win_rc.right - win_rc.left) - (rc.right - rc.left)) / 2,
					win_rc.top + ((win_rc.bottom - win_rc.top) - (rc.bottom - rc.top)) / 2 + 5,
					0,0,SWP_NOSIZE
				);
		}
		SetText(hWnd,L"PLEASE_CONFIRM");
		if(hibernate_instead_of_shutdown){
			message = WgxGetResourceString(i18n_table,L"SECONDS_TILL_HIBERNATION");
			SetText(GetDlgItem(hWnd,IDC_MESSAGE),L"REALLY_HIBERNATE_WHEN_DONE");
		} else {
			message = WgxGetResourceString(i18n_table,L"SECONDS_TILL_SHUTDOWN");
			SetText(GetDlgItem(hWnd,IDC_MESSAGE),L"REALLY_SHUTDOWN_WHEN_DONE");
		}
		_snwprintf(buffer,128,L"%u %ls",seconds_for_shutdown_rejection,message);
		buffer[127] = 0;
		SetWindowTextW(GetDlgItem(hWnd,IDC_DELAY_MSG),buffer);
		SetText(GetDlgItem(hWnd,IDC_YES_BUTTON),L"YES");
		SetText(GetDlgItem(hWnd,IDC_NO_BUTTON), L"NO");

		/*if(hFont){
			(void)SendMessage(hWnd,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
			hChild = GetWindow(hWnd,GW_CHILD);
			while(hChild){
				(void)SendMessage(hChild,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
				hChild = GetWindow(hChild,GW_HWNDNEXT);
			}
		}*/
		
		/* set timer */
		counter = seconds_for_shutdown_rejection;
		if(counter == 0)
			(void)EndDialog(hWnd,1);
		timer = SetTimer(hWnd,TIMER_ID,1000,NULL);
		if(timer == 0){
			//MessageBox(hWindow,"SetTimer failed!","Error!",MB_OK | MB_ICONHAND);
			// the code above will prevent shutdown which is dangerous
			/* force shutdown to avoid situation when pc works a long time without any control */
			(void)EndDialog(hWnd,1);
		}
		return FALSE;
	case WM_TIMER:
		counter --;
		_snwprintf(buffer,128,L"%u %ls",counter,message);
		buffer[127] = 0;
		SetWindowTextW(GetDlgItem(hWnd,IDC_DELAY_MSG),buffer);
		if(counter == 0){
			(void)KillTimer(hWnd,TIMER_ID);
			(void)EndDialog(hWnd,1);
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_YES_BUTTON:
			(void)KillTimer(hWnd,TIMER_ID);
			(void)EndDialog(hWnd,1);
			break;
		case IDC_NO_BUTTON:
			(void)KillTimer(hWnd,TIMER_ID);
			(void)EndDialog(hWnd,0);
			break;
		}
		return TRUE;
	case WM_CLOSE:
		(void)KillTimer(hWnd,TIMER_ID);
		(void)EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}
