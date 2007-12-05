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

/*
 * VERY IMPORTANT NOTE: (bug #1839755 cause)
 * 
 * + You should not wait for one SendMessage handler completion
 *   from another SendMessage handler. Because it will be a deadlock cause.
 *
 * + Example:
 *
 * int done;
 *
 * window_proc()
 * {
 *   if(stop button was pressed)
 *   {
 *     stop_the_driver();
 *     wait for done flag;
 *   }
 * }
 *
 * analyse_thread_proc()
 * {
 *   done = 0;
 *   analyse();
 *   RedrawMap();
 *   done = 1;
 * }
 *
 * + How it works:
 * 
 * RedrawMap() will send message to the map control 
 * that can not be handled by system before window_proc() returns.
 * But window_proc() is waiting for done flag, that can be set
 * after(!) RedrawMap() call. Therefore we have a deadlock.
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <commctrl.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>
#include <math.h>

#include "main.h"
#include "../include/ntndk.h"
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

char letter_numbers[MAX_DOS_DRIVES];
STATISTIC stat[MAX_DOS_DRIVES];
int work_status[MAX_DOS_DRIVES] = {0};
extern char map[MAX_DOS_DRIVES][N_BLOCKS];

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

char current_operation;
BOOL stop_pressed, exit_pressed = FALSE;
int My_Index;

#define N_OF_STATUSBAR_PARTS  5
#define IDM_STATUSBAR         500

ud_options *settings;

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
extern void CreateMaps();
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

void HandleError(short *err_msg,int exit_code)
{
	if(err_msg)
	{
		if(err_msg[0]) MessageBoxW(0,err_msg,L"Error!",MB_OK | MB_ICONHAND);
		udefrag_unload(TRUE);
		ExitProcess(exit_code);
	}
}

/*-------------------- Main Function -----------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	settings = udefrag_get_options();
	/* check command line keys */
	_strupr(lpCmdLine);
	portable_run = (strstr(lpCmdLine,"/P") != NULL) ? TRUE : FALSE;
	uninstall_run = (strstr(lpCmdLine,"/U") != NULL) ? TRUE : FALSE;
	if(uninstall_run)
	{	
		udefrag_clean_registry();
		ExitProcess(0);
	}
	HandleError(udefrag_init(0,NULL,FALSE),2);
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
	HandleError(L"",0);
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
	short *err_msg;
	volume_info *v;
	int i;
	LV_ITEM lvi;
	RECT rc;
	int dx;
	int cw[] = {60,60,100,100,100,85};

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
		MessageBoxW(0,err_msg,L"Error!",MB_OK | MB_ICONHAND);
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
	/* adjust columns widths */
	GetClientRect(hList,&rc);
	dx = rc.right - rc.left;
	//if(SendMessage(hList,LVM_GETITEMCOUNT,0,0) > SendMessage(hList,LVM_GETCOUNTPERPAGE,0,0))
	//	dx -= GetSystemMetrics(SM_CXVSCROLL);
	for(i = 0; i < 6; i++)
		SendMessage(hList,LVM_SETCOLUMNWIDTH,i,cw[i] * dx / 505);
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
	short *err_msg;

	stop_pressed = TRUE;
	err_msg = udefrag_stop();
	if(err_msg) MessageBoxW(0,err_msg,L"Error!",MB_OK | MB_ICONHAND);
}

void ShowProgress(void)
{
//	if(settings->show_progress)
//	{
		ShowWindow(hProgressMsg,SW_SHOWNORMAL);
		ShowWindow(hProgressBar,SW_SHOWNORMAL);
//	}
}

void HideProgress(void)
{
	ShowWindow(hProgressMsg,SW_HIDE);
	ShowWindow(hProgressBar,SW_HIDE);
	SetWindowText(hProgressMsg,"0 %");
	SendMessage(hProgressBar,PBM_SETPOS,0,0);
}

/*---------------- Main Dialog Callback ---------------------*/

BOOL CALLBACK DlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	HICON hIcon;
	int cx,cy;
	int dx,dy;
	RECT rc;
	LV_COLUMN lvc;
	LPNMLISTVIEW lpnm;

	switch(msg)
	{
	case WM_INITDIALOG:
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
		/* increase hight of map */
		GetWindowRect(hMap,&rc);
		rc.bottom ++;
		SetWindowPos(hMap,0,0,0,rc.right - rc.left,
			rc.bottom - rc.top,SWP_NOMOVE);
		CalculateBlockSize();
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
		/* initialize listview control */
		GetClientRect(hList,&rc);
		dx = rc.right - rc.left;
		SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,
			(LRESULT)(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT));
		lvc.mask = LVCF_TEXT | LVCF_WIDTH;
		lvc.pszText = "Volume";
		lvc.cx = 60 * dx / 505;
		SendMessage(hList,LVM_INSERTCOLUMN,0,(LRESULT)&lvc);
		lvc.pszText = "Status";
		lvc.cx = 60 * dx / 505;
		SendMessage(hList,LVM_INSERTCOLUMN,1,(LRESULT)&lvc);
		lvc.pszText = "File system";
		lvc.cx = 100 * dx / 505;
		SendMessage(hList,LVM_INSERTCOLUMN,2,(LRESULT)&lvc);
		lvc.mask |= LVCF_FMT;
		lvc.fmt = LVCFMT_RIGHT;
		lvc.pszText = "Total space";
		lvc.cx = 100 * dx / 505;
		SendMessage(hList,LVM_INSERTCOLUMN,3,(LRESULT)&lvc);
		lvc.pszText = "Free space";
		lvc.cx = 100 * dx / 505;
		SendMessage(hList,LVM_INSERTCOLUMN,4,(LRESULT)&lvc);
		lvc.pszText = "Percentage";
		lvc.cx = 85 * dx / 505;
		SendMessage(hList,LVM_INSERTCOLUMN,5,(LRESULT)&lvc);
		/* reduce hight of list view control */
		GetWindowRect(hList,&rc);
		rc.bottom --;
		SetWindowPos(hList,0,0,0,rc.right - rc.left,
			rc.bottom - rc.top,SWP_NOMOVE);
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
		CreateMaps();
		CreateBitMapGrid();
		CreateStatusBar();
		memset((void *)stat,0,sizeof(STATISTIC) * (MAX_DOS_DRIVES));
		UpdateStatusBar(0);
		break;
	case WM_NOTIFY:
		lpnm = (LPNMLISTVIEW)lParam;
		if(lpnm->hdr.code == LVN_ITEMCHANGED && (lpnm->uNewState & LVIS_SELECTED))
		{
			HideProgress();
			/* change Index */
			Index = letter_numbers[lpnm->iItem];
			/* redraw indicator */
			RedrawMap();
			/* Update status bar */
			UpdateStatusBar(Index);
		}
		break;
	case WM_COMMAND:
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
			if(!busy_flag)
				DialogBox(hInstance,MAKEINTRESOURCE(IDD_SETTINGS),hWindow,(DLGPROC)SettingsDlgProc);
			break;
		case IDC_SKIPREMOVABLE:
			if(HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == BN_PUSHED || \
				HIWORD(wParam) == BN_UNPUSHED || HIWORD(wParam) == BN_DBLCLK)
				settings->skip_removable = \
					(SendMessage(hCheckSkipRem,BM_GETCHECK,0,0) == BST_CHECKED);
			break;
		case IDC_SHOWFRAGMENTED:
			if(!busy_flag) ShowFragmented();
			break;
		case IDC_PAUSE:
		case IDC_STOP:
			Stop();
			break;
		case IDC_RESCAN:
			if(!busy_flag) RescanDrives();
		}
		break;
	case WM_MOVE:
		GetWindowRect(hWnd,&rc);
		if((HIWORD(rc.bottom)) != 0xffff)
		{
			rc.bottom -= delta_h;
			memcpy((void *)&win_rc,(void *)&rc,sizeof(RECT));
		}
		break;
	case WM_CLOSE:
		Stop();
		GetWindowRect(hWnd,&rc);
		if((HIWORD(rc.bottom)) != 0xffff)
		{
			rc.bottom -= delta_h;
			memcpy((void *)&win_rc,(void *)&rc,sizeof(RECT));
		}
		exit_pressed = TRUE;
		if(!busy_flag) EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

LRESULT CALLBACK ListWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	MSG message;

	if((iMsg == WM_LBUTTONDOWN || iMsg == WM_LBUTTONUP ||
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
		if(busy_flag) return 0;
	}
	return CallWindowProc(OldListProc,hWnd,iMsg,wParam,lParam);
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

int __stdcall update_stat(int df)
{
	int index = My_Index;
	char *cl_map;
	STATISTIC *pst;
	double percentage;
	char progress_msg[32];
	short *err_msg;

	if(stop_pressed) return 0; /* it's neccessary: see above one comment in Stop() */
	cl_map = map[index];
	pst = &stat[index];
	if(udefrag_get_progress(pst,&percentage)) goto done;
	UpdateStatusBar(index);
//	if(settings->show_progress)
//	{
		sprintf(progress_msg,"%c %u %%",
			current_operation = pst->current_operation,(int)percentage);
		SetWindowText(hProgressMsg,progress_msg);
		SendMessage(hProgressBar,PBM_SETPOS,(WPARAM)percentage,0);
//	}
	if(udefrag_get_map(cl_map,N_BLOCKS)) goto done;
	FillBitMap(index);
	RedrawMap();
done:
	if(df == FALSE) return 0;
	err_msg = udefrag_get_ex_command_result();
	if(wcslen(err_msg) > 0)
	{
		MessageBoxW(0,err_msg,L"Error!",MB_OK | MB_ICONHAND);
		//DisplayError(err_msg);
		return 0;
	}
	if(!stop_pressed)
	{
		sprintf(progress_msg,"%c 100 %%",current_operation);
		SetWindowText(hProgressMsg,progress_msg);
		SendMessage(hProgressBar,PBM_SETPOS,100,0);
	}
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	LRESULT iItem;
	int index;
	UCHAR command;
	short *err_msg;

	/* return immediately if we are busy */
	if(busy_flag) return 0;
	busy_flag = 1;
	stop_pressed = FALSE;

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

	My_Index = index;

	ShowProgress();
	SetWindowText(hProgressMsg,"0 %");
	SendMessage(hProgressBar,PBM_SETPOS,0,0);
	switch(command)
	{
	case 'a':
		err_msg = udefrag_analyse_ex((UCHAR)(index + 'A'),update_stat);
		break;
	case 'd':
		err_msg = udefrag_defragment_ex((UCHAR)(index + 'A'),update_stat);
		break;
	default:
		err_msg = udefrag_optimize_ex((UCHAR)(index + 'A'),update_stat);
	}
	if(err_msg)
	{
		MessageBoxW(0,err_msg,L"Error!",MB_OK | MB_ICONHAND);
		//DisplayError(err_msg);
		ShowStatus(STAT_CLEAR,iItem);
		ClearMap();
	}
	EnableButtons();
	busy_flag = 0;
	if(exit_pressed) EndDialog(hWindow,0);
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
