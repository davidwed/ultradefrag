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
* GUI - main window code.
*/

#include "main.h"

/* Global variables */
HINSTANCE hInstance;
HACCEL hAccel;
HWND hWindow;
HWND hMap;
HWND hCheckSkipRem;

extern RECT win_rc; /* coordinates of main window */
extern int skip_removable;

extern STATISTIC stat[MAX_DOS_DRIVES];
extern BOOL busy_flag, exit_pressed;

signed int delta_h = 0;

/* Function prototypes */
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void ShowFragmented();
void DestroyImageList(void);

void __stdcall ErrorHandler(short *msg)
{
	/* ignore notifications and warnings */
	if(msg[0] == 'E') MessageBoxW(NULL,msg,L"Error!",MB_OK | MB_ICONHAND);
}

/*-------------------- Main Function -----------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	udefrag_set_error_handler(ErrorHandler);
	if(udefrag_init(N_BLOCKS) < 0){
		udefrag_unload();
		return 2;
	}
	GetPrefs();
	hInstance = GetModuleHandle(NULL);
	BuildResourceTable();

	/*
	* This call needs on dmitriar's pc (on xp) no more than 550 cpu tacts,
	* but InitCommonControlsEx() needs about 90000 tacts.
	* Because the first function is just a stub on xp.
	*/
	InitCommonControls();
	
	delta_h = GetSystemMetrics(SM_CYCAPTION) - 0x13;
	if(delta_h < 0) delta_h = 0;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN),NULL,(DLGPROC)DlgProc);
	/* delete all created gdi objects */
	DeleteMaps();
	DestroyImageList();
	/* save settings */
	SavePrefs();
	DestroyResourceTable();
	udefrag_unload();
	return 0;
}

/*---------------- Main Dialog Callback ---------------------*/

BOOL CALLBACK DlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	HICON hIcon;
	int cx,cy;
	int dx,dy;
	RECT rc;

	switch(msg){
	case WM_INITDIALOG:
		/* Window Initialization */
		hWindow = hWnd;
		hAccel = LoadAccelerators(hInstance,MAKEINTRESOURCE(IDR_ACCELERATOR1));
		InitVolList(); /* before map! */
		InitButtons();
		InitProgress();
		InitMap();
		cx = GetSystemMetrics(SM_CXSCREEN);
		cy = GetSystemMetrics(SM_CYSCREEN);
		if(win_rc.left < 0) win_rc.left = 0; if(win_rc.top < 0) win_rc.top = 0;
		if(win_rc.left >= cx) win_rc.left = cx - 10; if(win_rc.top >= cy) win_rc.top = cy - 10;
		GetWindowRect(hWnd,&rc);
		dx = rc.right - rc.left;
		dy = rc.bottom - rc.top + delta_h;
		SetWindowPos(hWnd,0,win_rc.left,win_rc.top,dx,dy,0);
		hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_APP));
		SendMessage(hWnd,WM_SETICON,1,(LRESULT)hIcon);
		if(hIcon) DeleteObject(hIcon);
		UpdateVolList(skip_removable);
		CreateMaps();
		CreateStatusBar();
		memset((void *)stat,0,sizeof(STATISTIC) * (MAX_DOS_DRIVES));
		UpdateStatusBar(&stat[0]);
		break;
	case WM_NOTIFY:
		VolListNotifyHandler(lParam);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_ANALYSE:
			analyse();
			break;
		case IDC_DEFRAGM:
			defragment();
			break;
		case IDC_COMPACT:
			optimize();
			break;
		case IDC_ABOUT:
			DialogBox(hInstance,MAKEINTRESOURCE(IDD_ABOUT),hWindow,(DLGPROC)AboutDlgProc);
			break;
		case IDC_SETTINGS:
			if(!busy_flag){
				DialogBox(hInstance,MAKEINTRESOURCE(IDD_NEW_SETTINGS),hWindow,(DLGPROC)NewSettingsDlgProc);
				/* reload and apply settings */
				//if(udefrag_reload_settings() < 0) udefrag_pop_error(NULL,0);
			}
			break;
		case IDC_SKIPREMOVABLE:
			if(HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == BN_PUSHED || \
				HIWORD(wParam) == BN_UNPUSHED || HIWORD(wParam) == BN_DBLCLK)
				skip_removable = \
					(SendMessage(hCheckSkipRem,BM_GETCHECK,0,0) == BST_CHECKED);
			break;
		case IDC_SHOWFRAGMENTED:
			if(!busy_flag) ShowFragmented();
			break;
		case IDC_PAUSE:
		case IDC_STOP:
			stop();
			break;
		case IDC_RESCAN:
			if(!busy_flag) UpdateVolList(skip_removable);
		}
		break;
	case WM_MOVE:
		GetWindowRect(hWnd,&rc);
		if((HIWORD(rc.bottom)) != 0xffff){
			rc.bottom -= delta_h;
			memcpy((void *)&win_rc,(void *)&rc,sizeof(RECT));
		}
		break;
	case WM_CLOSE:
		stop();
		GetWindowRect(hWnd,&rc);
		if((HIWORD(rc.bottom)) != 0xffff){
			rc.bottom -= delta_h;
			memcpy((void *)&win_rc,(void *)&rc,sizeof(RECT));
		}
		exit_pressed = TRUE;
		if(!busy_flag) EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

void HandleShortcuts(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	MSG message;

	message.hwnd = hWnd;
	message.message = iMsg;
	message.wParam = wParam;
	message.lParam = lParam;
	message.pt.x = message.pt.y = 0;
	message.time = 0;
	TranslateAccelerator(hWindow,hAccel,&message);
}

void ShowFragmented()
{
	char path[] = "C:\\fraglist.luar";
	LRESULT iItem;

	iItem = VolListGetSelectedItemIndex();
	if(iItem == -1)	return;

	if(VolListGetWorkStatus(iItem) == STAT_CLEAR)
		return;
	path[0] = VolListGetLetter(iItem);
	ShellExecute(hWindow,"view",path,NULL,NULL,SW_SHOW);
}
