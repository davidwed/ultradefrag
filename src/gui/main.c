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
 *  GUI - main code.
 */

#include <windows.h>
#include <commctrl.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>
#include <math.h>

#include "main.h"
#include "../include/ultradfg.h"

#include "resource.h"

/* Global variables */
HINSTANCE hInstance;
HACCEL hAccel;
HWND hWindow, hList, hStatus;
HWND hBtnAnalyse,hBtnDfrg,hBtnPause,hBtnStop,hBtnRescan;
HWND hBtnCompact,hBtnFragm,hBtnSettings;
HWND hCheckSkipRem;
HWND hProgressMsg,hProgressBar;
HWND hMap;

BOOL portable_run = FALSE;
BOOL uninstall_run = FALSE;

extern RECT win_rc; /* coordinates of main window */

signed int delta_h = 0;

//#define MAP_WIDTH         0x209
//#define MAP_HEIGHT        0x8c
//#define BLOCK_SIZE        0x9   /* in pixels */

char letter_numbers['Z' - 'A' + 1];
STATISTIC stat['Z' - 'A' + 1];
int work_status['Z' - 'A' + 1] = {0};
extern char map['Z' - 'A' + 1][N_BLOCKS];

#define STAT_CLEAR	0
#define STAT_WORK	1
#define STAT_AN		2
#define STAT_DFRG	3

char *stat_msg[] = {"","Proc","Analys","Defrag"};

int Index;

WNDPROC OldListProc,OldRectangleWndProc,OldBtnWndProc,OldCheckWndProc;

int thr_id;
BOOL busy_flag = 0;
char buffer[64];

HANDLE hEventComplete = NULL, hEvtDone = NULL;

char current_operation;
BOOL stop_pressed;

#define N_OF_STATUSBAR_PARTS  5
#define IDM_STATUSBAR         500

ud_settings *settings;

/* Function prototypes */
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
extern BOOL CALLBACK SettingsDlgProc(HWND, UINT, WPARAM, LPARAM);
void RescanDrives();
void ClearMap();
void RedrawMap();
void ShowStatus(int,LRESULT);
void ShowFragmented();
void ShowProgress();
void HideProgress();

extern void CalculateBlockSize();
extern BOOL FillBitMap(int);
extern BOOL CreateBitMapGrid();
extern void DeleteMaps();

BOOL CreateStatusBar();
void UpdateStatusBar(int index);

LRESULT CALLBACK ListWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RectWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK BtnWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CheckWndProc(HWND, UINT, WPARAM, LPARAM);

DWORD WINAPI ThreadProc(LPVOID);
DWORD WINAPI RescanDrivesThreadProc(LPVOID);

#define create_thread(func,param) \
		CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)func,(void *)param,0,&thr_id)

void HandleError(char *err_msg,int exit_code)
{
	if(err_msg)
	{
		if(err_msg[0]) MessageBox(0,err_msg,"Error!",MB_OK | MB_ICONHAND);
		if(hEvtDone) CloseHandle(hEvtDone);
		if(hEventComplete) CloseHandle(hEventComplete);
		udefrag_unload();
		ExitProcess(exit_code);
	}
}

/*-------------------- Main Function -----------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	settings = udefrag_get_settings();
	/* check command line keys */
	_strupr(lpCmdLine);
	portable_run = (strstr(lpCmdLine,"/P") != NULL) ? TRUE : FALSE;
	uninstall_run = (strstr(lpCmdLine,"/U") != NULL) ? TRUE : FALSE;
	if(uninstall_run)
	{	
		/* clear registry */
		settings->next_boot = settings->every_boot = FALSE;
		udefrag_save_settings();
		ExitProcess(0);
	}
	hEventComplete = CreateEvent(NULL,TRUE,TRUE,"UltraDefragCommandComplete");
	hEvtDone = CreateEvent(NULL,TRUE,TRUE,"UltraDefragDone");
	if(!hEventComplete || !hEvtDone) HandleError("Can't create events!",2);
	HandleError(udefrag_init(FALSE),2);
	/* load settings - always successful */
	udefrag_load_settings(0,NULL);
	HandleError(udefrag_apply_settings(),2);
	win_rc.left = settings->x; win_rc.top = settings->y;
	hInstance = GetModuleHandle(NULL);
	memset((void *)work_status,0,sizeof(work_status));
	InitCommonControls();
	delta_h = GetSystemMetrics(SM_CYCAPTION) - 0x13;
	if(delta_h < 0) delta_h = 0;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN),NULL,(DLGPROC)DlgProc);
	/* delete all created gdi objects */
	DeleteMaps();
	/* save settings */
	settings->x = win_rc.left; settings->y = win_rc.top;
	HandleError(udefrag_save_settings(),4);
	HandleError("",0);
}

__inline void RescanDrives()
{
	create_thread(RescanDrivesThreadProc,NULL);
}

DWORD WINAPI RescanDrivesThreadProc(LPVOID lpParameter)
{
	char chr;
	int stat,index;
	char s[16];
	int p;
	char *err_msg;
	volume_info *v;
	int i;
	LV_ITEM lvi;

	/* Disable 'Rescan' button [and others] */
	EnableWindow(hBtnRescan,FALSE);
	EnableWindow(hBtnAnalyse,FALSE);
	EnableWindow(hBtnDfrg,FALSE);
	EnableWindow(hBtnCompact,FALSE);
	EnableWindow(hBtnFragm,FALSE);
	HideProgress();

	SendMessage(hList,LVM_DELETEALLITEMS,0,0);
	index = 0;
	err_msg = udefrag_get_avail_volumes(&v,settings->skip_removable);
	if(err_msg)
	{
		MessageBox(0,err_msg,"Error!",MB_OK | MB_ICONHAND);
		goto scan_done;
	}
	for(i = 0;;i++)
	{
		chr = v[i].letter;
		if(!chr) break;

		sprintf(s,"%c:",chr);
		lvi.mask = LVIF_TEXT;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.pszText = s;
		SendMessage(hList,LVM_INSERTITEM,0,(LRESULT)&lvi);

		stat = work_status[chr - 'A'];
		lvi.iSubItem = 1;
		lvi.pszText = stat_msg[stat];
		SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);

		lvi.iSubItem = 2;
		lvi.pszText = v[i].fsname;
		SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);

		fbsize(s,(ULONGLONG)(v[i].total_space.QuadPart));
		lvi.iSubItem = 3;
		lvi.pszText = s;
		SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);

		fbsize(s,(ULONGLONG)(v[i].free_space.QuadPart));
		lvi.iSubItem = 4;
		lvi.pszText = s;
		SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);

		p = (int)(100 * \
			((double)(signed __int64)(v[i].free_space.QuadPart) / (double)(signed __int64)(v[i].total_space.QuadPart)));
		sprintf(s,"%u %%",p);
		lvi.iSubItem = 5;
		lvi.pszText = s;
		SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);

		letter_numbers[index] = chr - 'A';
		index ++;
	}
scan_done:
	lvi.mask = LVIF_STATE;
	lvi.stateMask = LVIS_SELECTED;
	lvi.state = LVIS_SELECTED;
	SendMessage(hList,LVM_SETITEMSTATE,0,(LRESULT)&lvi);
	Index = letter_numbers[0];
	RedrawMap();
	UpdateStatusBar(Index);
	/* Enable 'Rescan' button [and others] */
	EnableWindow(hBtnRescan,TRUE);
	EnableWindow(hBtnAnalyse,TRUE);
	EnableWindow(hBtnDfrg,TRUE);
	EnableWindow(hBtnCompact,TRUE);
	EnableWindow(hBtnFragm,TRUE);
	return 0;
}

void Stop()
{
	char *err_msg;

	err_msg = udefrag_stop();
	if(err_msg) MessageBox(0,err_msg,"Error!",MB_OK | MB_ICONHAND);
	stop_pressed = TRUE;
	////WaitForSingleObject(hEvtDone,INFINITE);
}

void ShowProgress(void)
{
	if(settings->show_progress)
	{
		ShowWindow(hProgressMsg,SW_SHOWNORMAL);
		ShowWindow(hProgressBar,SW_SHOWNORMAL);
	}
}

void HideProgress(void)
{
	ShowWindow(hProgressMsg,SW_HIDE);
	ShowWindow(hProgressBar,SW_HIDE);
	SetWindowText(hProgressMsg,"0 %");
	SendMessage(hProgressBar,PBM_SETPOS,0,0);
}

/*---------------- Dialog Box Function ---------------------*/

BOOL CALLBACK DlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	HICON hIcon;
	int cx,cy;
	int dx,dy;
	RECT _rc;
	LV_COLUMN lvc;

	switch(msg)
	{
	case WM_INITDIALOG:
		{
			/* Window Initialization */
			hWindow = hWnd;
			hAccel = LoadAccelerators(hInstance,MAKEINTRESOURCE(IDR_ACCELERATOR1));
			hList = GetDlgItem(hWnd,IDC_VOLUMES);
			hBtnAnalyse = GetDlgItem(hWnd,IDC_ANALYSE);
			hBtnDfrg = GetDlgItem(hWnd,IDC_DEFRAGM);
			hBtnCompact = GetDlgItem(hWnd,IDC_COMPACT);
			hBtnPause = GetDlgItem(hWnd,IDC_PAUSE);
			hBtnStop = GetDlgItem(hWnd,IDC_STOP);
			hBtnFragm = GetDlgItem(hWnd,IDC_SHOWFRAGMENTED);
			hBtnRescan = GetDlgItem(hWnd,IDC_RESCAN);
			hBtnSettings = GetDlgItem(hWnd,IDC_SETTINGS);
			hCheckSkipRem = GetDlgItem(hWnd,IDC_SKIPREMOVABLE);
			if(settings->skip_removable)
				SendMessage(hCheckSkipRem,BM_SETCHECK,BST_CHECKED,0);
			else
				SendMessage(hCheckSkipRem,BM_SETCHECK,BST_UNCHECKED,0);
			hProgressMsg = GetDlgItem(hWnd,IDC_PROGRESSMSG);
			hProgressBar = GetDlgItem(hWnd,IDC_PROGRESS1);
			HideProgress();
			hMap = GetDlgItem(hWnd,IDC_MAP);
			GetWindowRect(hMap,&_rc);
			_rc.bottom ++;
			SetWindowPos(hMap,0,0,0,_rc.right - _rc.left,
				_rc.bottom - _rc.top,SWP_NOMOVE);
			CalculateBlockSize();
			cx = GetSystemMetrics(SM_CXSCREEN);
			cy = GetSystemMetrics(SM_CYSCREEN);
			if(win_rc.left < 0) win_rc.left = 0; if(win_rc.top < 0) win_rc.top = 0;
			if(win_rc.left >= cx) win_rc.left = cx - 10; if(win_rc.top >= cy) win_rc.top = cy - 10;
			GetWindowRect(hWnd,&_rc);
			dx = _rc.right - _rc.left;
			dy = _rc.bottom - _rc.top + delta_h;
			SetWindowPos(hWnd,0,win_rc.left,win_rc.top,dx,dy,0);
			hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_APP));
			SendMessage(hWnd,WM_SETICON,1,(LRESULT)hIcon);
			if(hIcon) DeleteObject(hIcon);
			/* initialize listview control */
			SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,
				(LRESULT)(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT));
			lvc.mask = LVCF_TEXT | LVCF_WIDTH;
			lvc.pszText = "Volume";
			lvc.cx = 60;
			SendMessage(hList,LVM_INSERTCOLUMN,0,(LRESULT)&lvc);
			lvc.pszText = "Status";
			lvc.cx = 60;
			SendMessage(hList,LVM_INSERTCOLUMN,1,(LRESULT)&lvc);
			lvc.pszText = "File system";
			lvc.cx = 100;
			SendMessage(hList,LVM_INSERTCOLUMN,2,(LRESULT)&lvc);
			lvc.mask |= LVCF_FMT;
			lvc.fmt = LVCFMT_RIGHT;
			lvc.pszText = "Total space";
			lvc.cx = 100;
			SendMessage(hList,LVM_INSERTCOLUMN,3,(LRESULT)&lvc);
			lvc.pszText = "Free space";
			lvc.cx = 100;
			SendMessage(hList,LVM_INSERTCOLUMN,4,(LRESULT)&lvc);
			lvc.pszText = "Percentage";
			lvc.cx = 85;
			SendMessage(hList,LVM_INSERTCOLUMN,5,(LRESULT)&lvc);
			/* set window procs */
			OldListProc = (WNDPROC)SetWindowLongPtr(hList,GWLP_WNDPROC,(LONG_PTR)ListWndProc);
			OldRectangleWndProc = (WNDPROC)SetWindowLongPtr(hMap,GWLP_WNDPROC,(LONG_PTR)RectWndProc);
			OldBtnWndProc = (WNDPROC)SetWindowLongPtr(hBtnAnalyse,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
			SetWindowLongPtr(hBtnDfrg,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
			SetWindowLongPtr(hBtnCompact,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
			SetWindowLongPtr(hBtnPause,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
			SetWindowLongPtr(hBtnStop,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
			SetWindowLongPtr(hBtnFragm,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
			SetWindowLongPtr(GetDlgItem(hWnd,IDC_SETTINGS),GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
			SetWindowLongPtr(GetDlgItem(hWnd,IDC_ABOUT),GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
			SetWindowLongPtr(hBtnRescan,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
			OldCheckWndProc = \
				(WNDPROC)SetWindowLongPtr(hCheckSkipRem,GWLP_WNDPROC,(LONG_PTR)CheckWndProc);
			RescanDrives();
			CreateBitMapGrid();
			CreateStatusBar();
			memset((void *)stat,0,sizeof(STATISTIC) * ('Z' - 'A' + 1));
			UpdateStatusBar(0);
			return FALSE;
		}
	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case IDC_ANALYSE:
				create_thread(ThreadProc,(DWORD)'a');
				break;
			case IDC_DEFRAGM:
				create_thread(ThreadProc,(DWORD)'d');
				break;
			case IDC_COMPACT:
				create_thread(ThreadProc,(DWORD)'c');
				break;
			case IDC_ABOUT:
				DialogBox(hInstance,MAKEINTRESOURCE(IDD_ABOUT),hWindow,(DLGPROC)AboutDlgProc);
				break;
			case IDC_SETTINGS:
				DialogBox(hInstance,MAKEINTRESOURCE(IDD_SETTINGS),hWindow,(DLGPROC)SettingsDlgProc);
				break;
			case IDC_SKIPREMOVABLE:
				if(HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == BN_PUSHED || \
					HIWORD(wParam) == BN_UNPUSHED || HIWORD(wParam) == BN_DBLCLK)
					settings->skip_removable = \
						(SendMessage(hCheckSkipRem,BM_GETCHECK,0,0) == BST_CHECKED);
				break;
			case IDC_SHOWFRAGMENTED:
				ShowFragmented();
				break;
			case IDC_PAUSE:
			case IDC_STOP:
				Stop();
				break;
			case IDC_RESCAN:
				RescanDrives();
			}
			return FALSE;
		}
	case WM_MOVE:
		{
			GetWindowRect(hWnd,&_rc);
			if((HIWORD(_rc.bottom)) != 0xffff)
			{
				_rc.bottom -= delta_h;
				memcpy((void *)&win_rc,(void *)&_rc,sizeof(RECT));
			}
			return FALSE;
		}
	case WM_CLOSE:
		{
			Stop();
			GetWindowRect(hWnd,&_rc);
			if((HIWORD(_rc.bottom)) != 0xffff)
			{
				_rc.bottom -= delta_h;
				memcpy((void *)&win_rc,(void *)&_rc,sizeof(RECT));
			}
			EndDialog(hWnd,0);
			return TRUE;
		}
	}
	return FALSE;
}

LRESULT CALLBACK ListWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	MSG message;
	LRESULT iItem, retval = 0;

	if((iMsg == WM_KEYDOWN || iMsg == WM_LBUTTONDOWN || iMsg == WM_LBUTTONUP ||
		iMsg == WM_RBUTTONDOWN || iMsg == WM_RBUTTONUP) && busy_flag)
		return 0;
	if(iMsg == WM_KEYDOWN)
	{
		message.hwnd = hWnd;
		message.message = iMsg;
		message.wParam = wParam;
		message.lParam = lParam;
		message.pt.x = message.pt.y = 0;
		message.time = 0;
		TranslateAccelerator(hWindow,hAccel,&message);
	}

	retval = CallWindowProc(OldListProc,hWnd,iMsg,wParam,lParam);

	if(iMsg == WM_KEYDOWN || iMsg == WM_LBUTTONDOWN || iMsg == WM_LBUTTONUP)
	{
		iItem = SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
		if(iItem != -1)
		{
			HideProgress();
			/* change Index */
			Index = letter_numbers[iItem];
			/* redraw indicator */
			RedrawMap();
			/* Update status bar */
			UpdateStatusBar(Index);
		}
	}
	return retval;
}

LRESULT CALLBACK BtnWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	MSG message;

	if(iMsg == WM_KEYDOWN)
	{
		message.hwnd = hWnd;
		message.message = iMsg;
		message.wParam = wParam;
		message.lParam = lParam;
		message.pt.x = message.pt.y = 0;
		message.time = 0;
		TranslateAccelerator(hWindow,hAccel,&message);
	}
	return CallWindowProc(OldBtnWndProc,hWnd,iMsg,wParam,lParam);
}

LRESULT CALLBACK CheckWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	MSG message;

	if(iMsg == WM_KEYDOWN)
	{
		message.hwnd = hWnd;
		message.message = iMsg;
		message.wParam = wParam;
		message.lParam = lParam;
		message.pt.x = message.pt.y = 0;
		message.time = 0;
		TranslateAccelerator(hWindow,hAccel,&message);
	}
	return CallWindowProc(OldCheckWndProc,hWnd,iMsg,wParam,lParam);
}

LRESULT CALLBACK RectWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;

	if(iMsg == WM_PAINT)
	{
		BeginPaint(hWnd,&ps);
		RedrawMap();
		EndPaint(hWnd,&ps);
	}
	return CallWindowProc(OldRectangleWndProc,hWnd,iMsg,wParam,lParam);
}

void SetIcon(int part,int id)
{
	HANDLE hImg;

	hImg = LoadImage(hInstance,(LPCSTR)(size_t)id,IMAGE_ICON,16,16,LR_VGACOLOR|LR_SHARED);
	SendMessage(hStatus,SB_SETICON,part,(LPARAM)hImg);
	DestroyIcon(hImg);
}

BOOL CreateStatusBar()
{
	RECT rc;
	double total = 560.00;
	double x;

	int a[N_OF_STATUSBAR_PARTS];

	hStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_BORDER, \
									"0 dirs", hWindow, IDM_STATUSBAR);
	if(hStatus)
	{
		GetClientRect(hStatus,&rc);
		x = (double)(rc.right - rc.left) / total;
		a[0] = (int)floor(100 * x); a[1] = (int)floor(200 * x);
		a[2] = (int)floor(320 * x); a[3] = (int)floor(440 * x);
		a[4] = rc.right - rc.left;
		SendMessage(hStatus,SB_SETPARTS,N_OF_STATUSBAR_PARTS + 1,(LPARAM)a);
		SetIcon(0,IDI_DIR); SetIcon(1,IDI_UNFRAGM);
		SetIcon(2,IDI_FRAGM); SetIcon(3,IDI_CMP);
		SetIcon(4,IDI_MFT);
		return TRUE;
	}
	return FALSE;
}

void UpdateStatusBar(int index)
{
	char s[32];

	if(!hStatus) return;
	sprintf(buffer,"%u dirs",(stat[index]).dircounter);
	SendMessage(hStatus,SB_SETTEXT,0,(LPARAM)buffer);
	sprintf(buffer,"%u files",(stat[index]).filecounter);
	SendMessage(hStatus,SB_SETTEXT,1,(LPARAM)buffer);
	sprintf(buffer,"%u fragmented",(stat[index]).fragmfilecounter);
	SendMessage(hStatus,SB_SETTEXT,2,(LPARAM)buffer);
	sprintf(buffer,"%u compressed",(stat[index]).compressedcounter);
	SendMessage(hStatus,SB_SETTEXT,3,(LPARAM)buffer);
	fbsize(s,(ULONGLONG)((stat[index]).mft_size));
	strcat(s," MFT");
	SendMessage(hStatus,SB_SETTEXT,4,(LPARAM)s);
}

__inline void EnableButtons(void)
{
	EnableWindow(hBtnAnalyse,TRUE);
	EnableWindow(hBtnDfrg,TRUE);
	EnableWindow(hBtnCompact,TRUE);
	EnableWindow(hBtnFragm,TRUE);
	EnableWindow(hBtnPause,FALSE);
	EnableWindow(hBtnStop,FALSE);
	EnableWindow(hBtnRescan,TRUE);
	EnableWindow(hBtnSettings,TRUE);
}

__inline void DisableButtons(void)
{
	EnableWindow(hBtnAnalyse,FALSE);
	EnableWindow(hBtnDfrg,FALSE);
	EnableWindow(hBtnCompact,FALSE);
	EnableWindow(hBtnFragm,FALSE);
	EnableWindow(hBtnPause,TRUE);
	EnableWindow(hBtnStop,TRUE);
	EnableWindow(hBtnRescan,FALSE);
	EnableWindow(hBtnSettings,FALSE);
}

DWORD WINAPI UpdateMapThreadProc(LPVOID lpParameter)
{
	int index = (int)(size_t)lpParameter;
	char *_map;
	DWORD wait_result;
	STATISTIC *pst;
	double percentage;
	char progress_msg[32];

	_map = map[index];
	pst = &stat[index];
	ShowProgress();
	SetWindowText(hProgressMsg,"0 %");
	SendMessage(hProgressBar,PBM_SETPOS,0,0);
	do
	{
		wait_result = WaitForSingleObject(hEventComplete,1);
		if(udefrag_get_progress(pst,&percentage)) goto wait;
		UpdateStatusBar(index);
		if(settings->show_progress)
		{
			sprintf(progress_msg,"%c %u %%",
				current_operation = pst->current_operation,(int)percentage);
			SetWindowText(hProgressMsg,progress_msg);
			SendMessage(hProgressBar,PBM_SETPOS,(WPARAM)percentage,0);
		}
		if(udefrag_get_map(_map,N_BLOCKS)) goto wait;
		FillBitMap(index);
		RedrawMap();
wait: 
		Sleep(settings->update_interval);
	} while(wait_result == WAIT_TIMEOUT);
	if(!stop_pressed)
	{
		sprintf(progress_msg,"%c 100 %%",current_operation);
		SetWindowText(hProgressMsg,progress_msg);
		SendMessage(hProgressBar,PBM_SETPOS,100,0);
	}
	SetEvent(hEvtDone);
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	LRESULT iItem;
	int index;
	UCHAR command;
	char *err_msg;

	busy_flag = 1;
	iItem = SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
	if(iItem == -1)
	{
		busy_flag = 0;
		return 0;
	}

	ShowStatus(STAT_WORK,iItem);
	DisableButtons();
	ClearMap();

	index = letter_numbers[iItem];
	command = (UCHAR)lpParameter;

	if(command == 'a')
		ShowStatus(STAT_AN,iItem);
	else
		ShowStatus(STAT_DFRG,iItem);

	stop_pressed = FALSE;
	ResetEvent(hEventComplete);
	ResetEvent(hEvtDone);
	create_thread(UpdateMapThreadProc,(size_t)index);

	switch(command)
	{
	case 'a':
		err_msg = udefrag_analyse((UCHAR)(index + 'A'));
		break;
	case 'd':
		err_msg = udefrag_defragment((UCHAR)(index + 'A'));
		break;
	default:
		err_msg = udefrag_optimize((UCHAR)(index + 'A'));
	}
	if(err_msg)
	{
		MessageBox(0,err_msg,"Error!",MB_OK | MB_ICONHAND);
		ShowStatus(STAT_CLEAR,iItem/*linenumber*/);
		ClearMap();
	}
	SetEvent(hEventComplete);
	EnableButtons();
	busy_flag = 0;
	return 0;
}

void ShowStatus(int stat,LRESULT iItem)
{
	LV_ITEM lvi;

	work_status[letter_numbers[(int)iItem]] = stat;
	if(stat == STAT_CLEAR)
		return;
	lvi.mask = LVIF_TEXT;
	lvi.iItem = (int)iItem;
	lvi.iSubItem = 1;
	lvi.pszText = stat_msg[stat];
	SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);
}

void ShowFragmented()
{
	char path[] = "C:\\FRAGLIST.HTM";
	LRESULT iItem;
	int index;

	iItem = SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
	if(iItem == -1)
		return;

	index = letter_numbers[iItem];
	if(work_status[index] == STAT_CLEAR)
		return;
	path[0] = index + 'A';
	ShellExecute(hWindow,NULL,path,NULL,NULL,SW_SHOW);
}

BOOL CALLBACK AboutDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		/* Window Initialization */
		SetWindowPos(hWnd,0,win_rc.left + 106,win_rc.top + 59,0,0,SWP_NOSIZE);
		return FALSE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_CREDITS:
			ShellExecute(hWindow,"open",".\\CREDITS.TXT",NULL,NULL,SW_SHOW);
			break;
		case IDC_LICENSE:
			ShellExecute(hWindow,"open",".\\LICENSE.TXT",NULL,NULL,SW_SHOW);
			break;
		case IDC_HOMEPAGE:
			SetFocus(GetDlgItem(hWnd,IDOK));
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
