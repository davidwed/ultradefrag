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
#include <winioctl.h>
#include <commctrl.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>
#include <math.h>

#include "main.h"
#include "../include/misc.h"
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

HANDLE hUltraDefragDevice = INVALID_HANDLE_VALUE;

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

HANDLE hEventComplete = 0,hEvt = 0,/*hEvtStop = 0,*/ hEvtDone = 0;

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
extern BOOL RequestMap(HANDLE hDev,char *map,DWORD *txd,LPOVERLAPPED lpovrl);
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

extern char *FormatBySize(ULONGLONG long_number,char *s,int alignment);
extern void Format2(double number,char *s);

/* small function to exclude letters assigned by SUBST command */
BOOL is_virtual(char vol_letter)
{
	char dev_name[] = "A:";
	char target_path[512];

	dev_name[0] = vol_letter;
	QueryDosDevice(dev_name,target_path,512);
	return (strstr(target_path,"\\??\\") == target_path);
}

/*-------------------- Main Function -----------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	int err_code = 0;
	char *err_msg;
	HKEY hKey,hKey1,hKey2;
	ULONG _len;

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
	hEvt = CreateEvent(NULL,TRUE,TRUE,"UltraDefragIoComplete");
	//hEvtStop = CreateEvent(NULL,TRUE,TRUE,"UltraDefragStop");
	hEvtDone = CreateEvent(NULL,TRUE,TRUE,"UltraDefragDone");
	if(!hEventComplete || !hEvt || /*!hEvtStop ||*/ !hEvtDone)
	{
		err_code = 2; goto cleanup;
	}
	err_msg = udefrag_init(FALSE);
	if(err_msg)
	{
		MessageBox(0,err_msg,"Error!",MB_OK | MB_ICONHAND);
		err_code = 3; goto cleanup;
	}
	hUltraDefragDevice = get_device_handle();
	/* get window coordinates and other settings from registry */
	err_msg = udefrag_load_settings(0,NULL);
	if(err_msg)
	{
		MessageBox(0,err_msg,"Error!",MB_OK | MB_ICONHAND);
		err_code = 3; goto cleanup;
	}
	err_msg = udefrag_apply_settings();
	if(err_msg)
	{
		MessageBox(0,err_msg,"Error!",MB_OK | MB_ICONHAND);
		err_code = 3; goto cleanup;
	}
	/* load window coordinates */
	if(RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\DASoft\\NTDefrag",
		0,KEY_QUERY_VALUE,&hKey) == ERROR_SUCCESS)
	{
		_len = sizeof(RECT);
		RegQueryValueEx(hKey,"position",NULL,NULL,(BYTE*)&win_rc,&_len);
		RegCloseKey(hKey);
	}
	hInstance = GetModuleHandle(NULL);
	memset((void *)work_status,0,sizeof(work_status));
	InitCommonControls();
	delta_h = GetSystemMetrics(SM_CYCAPTION) - 0x13;
	if(delta_h < 0) delta_h = 0;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN),NULL,(DLGPROC)DlgProc);
	/* delete all created gdi objects */
	DeleteMaps();
	/* save window coordinates and other settings to registry */
	if(RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\DASoft\\NTDefrag",
		0,KEY_SET_VALUE,&hKey) != ERROR_SUCCESS)
	{
		RegCreateKeyEx(HKEY_CURRENT_USER,"Software",0,NULL,0,KEY_CREATE_SUB_KEY,NULL,&hKey1,NULL);
		RegCreateKeyEx(hKey1,"DASoft",0,NULL,0,KEY_CREATE_SUB_KEY,NULL,&hKey2,NULL);
		if(RegCreateKeyEx(hKey2,"NTDefrag",0,NULL,0,KEY_SET_VALUE,NULL,&hKey,NULL) != \
			ERROR_SUCCESS)
		{
			RegSetValueEx(hKey,"position",0,REG_BINARY,(BYTE*)&win_rc,sizeof(RECT));
			RegCloseKey(hKey);
		}
		RegCloseKey(hKey1); RegCloseKey(hKey2);
	}
	else
	{
		RegSetValueEx(hKey,"position",0,REG_BINARY,(BYTE*)&win_rc,sizeof(RECT));
		RegCloseKey(hKey);
	}
	err_msg = udefrag_save_settings();
	if(err_msg)
	{
		MessageBox(0,err_msg,"Error!",MB_OK | MB_ICONHAND);
		err_code = 3; goto cleanup;
	}
cleanup:
	if(hEventComplete) CloseHandle(hEventComplete);
	if(hEvt) CloseHandle(hEvt);
	//if(hEvtStop) CloseHandle(hEvtStop);
	udefrag_unload();
	ExitProcess(err_code);
}

__inline void RescanDrives()
{
	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RescanDrivesThreadProc,NULL,0,&thr_id);
}

DWORD WINAPI RescanDrivesThreadProc(LPVOID lpParameter)
{
	char chr;
	int stat,index;
	static char rootpath[] = "A:\\";
	ULONGLONG total, free, BytesAvailable; /* on NT 4.0 we must specify all parameters 
											  of GetDiskFreeSpaceEx */
	char volname[32];
	char filesys[7];
	char s[16];
	char *str;
	int maxcomp, flags;
	const char tmpl[] = "A:\t      \t      \t\t       \t  \t       \t  \t00 %";
	char line[sizeof(tmpl)+1];
	int percent,digit;
	int drive_type;

	/* Disable 'Rescan' button [and others] */
	EnableWindow(hBtnRescan,FALSE);
	EnableWindow(hBtnAnalyse,FALSE);
	EnableWindow(hBtnDfrg,FALSE);
	EnableWindow(hBtnCompact,FALSE);
	EnableWindow(hBtnFragm,FALSE);
	HideProgress();

	SendMessage(hList,LB_RESETCONTENT,0,0);
	SendMessage(hList,LB_ADDSTRING,0, \
		(LPARAM)"Volume  Status  File system      Total size         Free      Percent");
	SendMessage(hList,LB_ADDSTRING,0, \
		(LPARAM)"------------------------------------------------------------------------"
				"-----------");
	SetErrorMode(SEM_FAILCRITICALERRORS);
	index = 0;
	for(chr = 'A'; chr <= 'Z'; chr++)
	{
		rootpath[0] = chr;
		drive_type = GetDriveType(rootpath);
		if( drive_type != DRIVE_FIXED && drive_type != DRIVE_REMOVABLE && \
			drive_type != DRIVE_RAMDISK ) continue;
		if(is_virtual(chr)) continue;
		if(drive_type == DRIVE_REMOVABLE && settings->skip_removable) continue;
		if(GetDiskFreeSpaceEx(rootpath,(ULARGE_INTEGER *)&BytesAvailable,
			(ULARGE_INTEGER *)&total,(ULARGE_INTEGER *)&free))
		{
			if(GetVolumeInformation(rootpath,volname,sizeof(volname)-1,NULL,&maxcomp,&flags, \
				filesys,sizeof(filesys)-1))
			{
				/*
				 * Add to list following formatted string:
				 * L:    ssssss    ffffff    xxxx.xx AA    yyyy.yy BB    zz %
				 * -------------------------------------------------------------
				 *       status    filesys   total        free         percent
				 */
				memcpy((void *)line,(void *)tmpl,sizeof(tmpl));
				line[0] = chr;
				stat = work_status[chr - 'A'];
				memcpy((void *)(line + 3),(void *)(stat_msg[stat]),strlen(stat_msg[stat]));
				memcpy((void *)(line + 11),(void *)filesys,strlen(filesys));
				str = FormatBySize(total,s,RIGHT_ALIGNED);
				memcpy((void *)(line + 18),(void *)str,strlen(str));
				str = FormatBySize(free,s,RIGHT_ALIGNED);
				memcpy((void *)(line + 29),(void *)str,strlen(str));
				percent = (int)(100 * \
					(double)(signed __int64)free / (double)(signed __int64)total);
				digit = percent / 10;
				if(digit)
					line[40] = (char)digit + '0';
				else
					line[40] = 0x20;
				digit = percent % 10;
				line[41] = (char)digit + '0';
				SendMessage(hList,LB_ADDSTRING,0,(LRESULT)line);
			}
			letter_numbers[index] = chr - 'A';
			index ++;
		}
	}
	SetErrorMode(0);
	SendMessage(hList,LB_SETCURSEL,2,0);
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
/*	DWORD txd;
	OVERLAPPED ovrl;

	ovrl.hEvent = hEvtStop;
	ovrl.Offset = ovrl.OffsetHigh = 0;
	DeviceIoControl(hUltraDefragDevice,IOCTL_ULTRADEFRAG_STOP,
					NULL,0,NULL,0,&txd,&ovrl);
	WaitForSingleObject(hEvtStop,INFINITE);
*/
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
			OldCheckWndProc = (WNDPROC)SetWindowLongPtr(hCheckSkipRem,GWLP_WNDPROC,(LONG_PTR)CheckWndProc);
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
				CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadProc,(void *)(DWORD)'a',0,&thr_id);
				return FALSE;
			case IDC_DEFRAGM:
				CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadProc,(void *)(DWORD)'d',0,&thr_id);
				return FALSE;
			case IDC_COMPACT:
				CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadProc,(void *)(DWORD)'c',0,&thr_id);
				return FALSE;
			case IDC_ABOUT:
				DialogBox(hInstance,MAKEINTRESOURCE(IDD_ABOUT),hWindow,(DLGPROC)AboutDlgProc);
				return FALSE;
			case IDC_SETTINGS:
				DialogBox(hInstance,MAKEINTRESOURCE(IDD_SETTINGS),hWindow,(DLGPROC)SettingsDlgProc);
				return FALSE;
			case IDC_SKIPREMOVABLE:
				if(HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == BN_PUSHED || \
					HIWORD(wParam) == BN_UNPUSHED || HIWORD(wParam) == BN_DBLCLK)
					settings->skip_removable = (SendMessage(hCheckSkipRem,BM_GETCHECK,0,0) == BST_CHECKED);
				return FALSE;
			case IDC_SHOWFRAGMENTED:
				ShowFragmented();
				return FALSE;
			case IDC_PAUSE:
			case IDC_STOP:
				Stop();
				return FALSE;
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
	LRESULT i, retval = 0;

	if((iMsg == WM_KEYDOWN || iMsg == WM_LBUTTONDOWN || iMsg == WM_LBUTTONUP) && busy_flag)
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
		i = SendMessage(hList,LB_GETCURSEL,0,0);
		if(i < 2) /* don't select first two lines! */
		{
			i = 2;
			SendMessage(hList,LB_SETCURSEL,2,0);
		}
		HideProgress();
		/* change Index */
		Index = letter_numbers[i - 2];
		/* redraw indicator */
		RedrawMap();
		/* Update status bar */
		UpdateStatusBar(Index);
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
	char *str;

	if(!hStatus) return;
	sprintf(buffer,"%u dirs",(stat[index]).dircounter);
	SendMessage(hStatus,SB_SETTEXT,0,(LPARAM)buffer);
	sprintf(buffer,"%u files",(stat[index]).filecounter);
	SendMessage(hStatus,SB_SETTEXT,1,(LPARAM)buffer);
	sprintf(buffer,"%u fragmented",(stat[index]).fragmfilecounter);
	SendMessage(hStatus,SB_SETTEXT,2,(LPARAM)buffer);
	sprintf(buffer,"%u compressed",(stat[index]).compressedcounter);
	SendMessage(hStatus,SB_SETTEXT,3,(LPARAM)buffer);
	str = FormatBySize((ULONGLONG)((stat[index]).mft_size),buffer,LEFT_ALIGNED);
	strcat(str," MFT");
	SendMessage(hStatus,SB_SETTEXT,4,(LPARAM)str);
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
	DWORD txd;
	int index = (int)(size_t)lpParameter;
	char *_map;
	DWORD wait_result;
	OVERLAPPED ovrl;
	STATISTIC *pst;
	double percentage;
	char progress_msg[32];

	ovrl.hEvent = hEvt;
	ovrl.Offset = ovrl.OffsetHigh = 0;
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
		if(!RequestMap(hUltraDefragDevice,_map,&txd,&ovrl))
				goto wait;
		WaitForSingleObject(hEvt,INFINITE);
		FillBitMap(index);
		RedrawMap();
wait: 
		Sleep(settings->update_interval);
	} while(wait_result == WAIT_TIMEOUT);
	if(!stop_pressed)
	{
		progress_msg[0] = current_operation;
		progress_msg[1] = 0;
		strcat(progress_msg," 100 %");
		SetWindowText(hProgressMsg,progress_msg);
		SendMessage(hProgressBar,PBM_SETPOS,100,0);
	}
	SetEvent(hEvtDone);
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	LRESULT linenumber;
	int index;
//	ULTRADFG_COMMAND cmd;
//	DWORD txd;
//	OVERLAPPED ovrl;
	UCHAR command;
	char *err_msg;

	busy_flag = 1;
	linenumber = SendMessage(hList,LB_GETCURSEL,0,0);
	if(linenumber == LB_ERR)
	{
		busy_flag = 0;
		return 0;
	}

	ShowStatus(STAT_WORK,linenumber);
	DisableButtons();
	ClearMap();

	index = letter_numbers[linenumber - 2];
	//cmd.command = (char)lpParameter;
	//cmd.letter = index + 'A';
	//cmd.sizelimit = settings->sizelimit;
	//cmd.mode = __UserMode;
	command = (UCHAR)lpParameter;

	if(/*cmd.*/command == 'a')
		ShowStatus(STAT_AN,linenumber);
	else
		ShowStatus(STAT_DFRG,linenumber);

	stop_pressed = FALSE;
	ResetEvent(hEventComplete);
	ResetEvent(hEvtDone);
	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)UpdateMapThreadProc,(void *)(size_t)index,0,&thr_id);

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
		ShowStatus(STAT_CLEAR,linenumber);
		ClearMap();
	}
	SetEvent(hEventComplete);

/*	ovrl.hEvent = hEventComplete;
	ovrl.Offset = ovrl.OffsetHigh = 0;
	if(!WriteFile(hUltraDefragDevice,&cmd,sizeof(ULTRADFG_COMMAND),&txd,&ovrl))
	{
		MessageBox(0,"Incorrect drive letter or driver error!","Error!",MB_OK | MB_ICONHAND);
		ShowStatus(STAT_CLEAR,linenumber);
		ClearMap();
		SetEvent(hEventComplete);
	}
	WaitForSingleObject(hEventComplete,INFINITE);
	*/
	EnableButtons();
	busy_flag = 0;
	return 0;
}

void ShowStatus(int stat,LRESULT linenumber)
{
	char buffer[256];

	work_status[letter_numbers[linenumber-2]] = stat;
	if(stat == STAT_CLEAR)
		return;
	SendMessage(hList,LB_GETTEXT,linenumber,(LPARAM)buffer);
	memset((void *)(buffer + 3),0x20,6);
	memcpy((void *)(buffer + 3),(void *)(stat_msg[stat]),strlen(stat_msg[stat]));
	SendMessage(hList,LB_DELETESTRING,linenumber,0);
	SendMessage(hList,LB_INSERTSTRING,linenumber,(LPARAM)buffer);
	SendMessage(hList,LB_SETCURSEL,linenumber,0);
}

void ShowFragmented()
{
	char path[] = "C:\\FRAGLIST.HTM";
	LRESULT linenumber;
	int index;

	linenumber = SendMessage(hList,LB_GETCURSEL,0,0);
	if(linenumber == LB_ERR)
		return;

	index = letter_numbers[linenumber - 2];
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
