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
#include <winreg.h>
#include <winioctl.h>
#include <commctrl.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <shellapi.h>

#include "../include/misc.h"
#include "../include/ultradfg.h"

#include "resource.h"

typedef LONG NTSTATUS;

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

NTSYSAPI VOID NTAPI RtlInitUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PCWSTR SourceString);

NTSTATUS NTAPI
NtLoadDriver(IN PUNICODE_STRING DriverServiceName);

NTSTATUS NTAPI
NtUnloadDriver(IN PUNICODE_STRING DriverServiceName);

#ifndef USE_WINDDK
#define SetWindowLongPtr SetWindowLong
#define LONG_PTR LONG
#define GWLP_WNDPROC GWL_WNDPROC
#endif

/* Global variables */
HINSTANCE hInstance;
RECT win_rc = {0,0,0x22d,0x1b3}; /* coordinates of main window */
HACCEL hAccel;
DWORD skip_rem = 1;	/* check box state */
HWND hWindow, hList, hStatus;
HWND hBtnAnalyse,hBtnDfrg,hBtnPause,hBtnStop,hBtnRescan;
HWND hBtnCompact,hBtnFragm,hBtnSettings;
HWND hCheckSkipRem;
HWND hProgressMsg,hProgressBar;
HWND hTabCtrl;
HWND hFilterDlg,hGuiDlg,hReportDlg;
HWND hMap;

HANDLE hUltraDefragDevice = INVALID_HANDLE_VALUE;
ULONGLONG sizelimit = 0;
int update_interval = 500;

//#define MAP_WIDTH         0x209
//#define MAP_HEIGHT        0x8c
//#define BLOCK_SIZE        0x9   /* in pixels */

int iMAP_WIDTH = 0x209;
int iMAP_HEIGHT = 0x8c;
int iBLOCK_SIZE = 0x9;   /* in pixels */

#define BLOCKS_PER_HLINE  52
#define BLOCKS_PER_VLINE  14
#define N_BLOCKS          BLOCKS_PER_HLINE * BLOCKS_PER_VLINE

char letter_numbers['Z' - 'A' + 1];
char map['Z' - 'A' + 1][N_BLOCKS];
STATISTIC stat['Z' - 'A' + 1];
int work_status['Z' - 'A' + 1] = {0};

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

HANDLE hEventComplete = 0,hEvt = 0,hEvtStop = 0;

char current_operation;
BOOL stop_pressed;
BOOL show_progress = TRUE;

signed int delta_h = 0;

/* #define GRID_COLOR                  RGB(106,106,106) */
#define GRID_COLOR                  RGB(0,0,0)
#define SYSTEM_COLOR                RGB(0,180,60)
#define SYSTEM_OVERLIMIT_COLOR      RGB(0,90,30)
#define FRAGM_COLOR                 RGB(255,0,0)
#define FRAGM_OVERLIMIT_COLOR       RGB(128,0,0)
#define UNFRAGM_COLOR               RGB(0,0,255)
#define UNFRAGM_OVERLIMIT_COLOR     RGB(0,0,128)
#define MFT_COLOR                   RGB(128,0,128)
#define DIR_COLOR                   RGB(255,255,0)
#define DIR_OVERLIMIT_COLOR         RGB(128,128,0)
#define COMPRESSED_COLOR            RGB(185,185,0)
#define COMPRESSED_OVERLIMIT_COLOR  RGB(93,93,0)
#define NO_CHECKED_COLOR            RGB(0,255,255)

HDC bit_map_dc['Z' - 'A' + 1] = {0};
HDC bit_map_grid_dc = 0;
HBITMAP bit_map['Z' - 'A' + 1] = {0};
HBITMAP bit_map_grid = 0;

#define N_OF_STATUSBAR_PARTS  5
#define IDM_STATUSBAR         500

short *in_filter = NULL, *ex_filter = NULL;
BOOL in_edit_flag,ex_edit_flag;
UCHAR report_format = ASCII_FORMAT;
UCHAR report_type = HTML_REPORT;

short driver_key[] = \
	L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ultradfg";

DWORD dbgprint_level = 0;

/* Function prototypes */
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SettingsDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK FilterDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK EmptyDlgProc(HWND, UINT, WPARAM, LPARAM);
void RescanDrives();
void ClearMap();
void RedrawMap();
void ShowStatus(int,LRESULT);
void ShowFragmented();
void ShowProgress();
void HideProgress();

BOOL CreateBitMap(int);
void DrawBlock(HDC,char *,int,COLORREF);
BOOL FillBitMap(int);
BOOL CreateBitMapGrid();

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
	HANDLE hEventIsRunning = 0;
	HKEY hKey, hKey1, hKey2;
	DWORD _len,length;
	int i;
	int err_code = 0;
	DWORD txd;
	OVERLAPPED ovrl;
	UNICODE_STRING uStr;
	NTSTATUS status;
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	REPORT_TYPE rt;

	/* can't run more than 1 ex. of program */
	hEventIsRunning = OpenEvent(EVENT_MODIFY_STATE,FALSE,
				    "UltraDefragIsRunning");
	if(hEventIsRunning)
	{
		CloseHandle(hEventIsRunning);
		MessageBox(0,"Can't run more than 1 exemplar of this program!", \
			"Warning!",MB_OK | MB_ICONHAND);
		ExitProcess(1);
	}
	hEventIsRunning = CreateEvent(NULL,TRUE,TRUE,"UltraDefragIsRunning");
	hEventComplete = CreateEvent(NULL,TRUE,TRUE,"UltraDefragCommandComplete");
	hEvt = CreateEvent(NULL,TRUE,TRUE,"UltraDefragIoComplete");
	hEvtStop = CreateEvent(NULL,TRUE,TRUE,"UltraDefragStop");
	if(!hEventIsRunning || !hEventComplete || !hEvt || !hEvtStop)
	{
		err_code = 2; goto cleanup;
	}
	/* open our device */
	if(OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&hToken))
	{
		tp.PrivilegeCount = 1;
		tp.Privileges->Attributes = SE_PRIVILEGE_ENABLED;
		tp.Privileges->Luid.HighPart = 0;
		tp.Privileges->Luid.LowPart = 0xA; /*SE_LOAD_DRIVER_PRIVILEGE;*/
		AdjustTokenPrivileges(hToken,FALSE,&tp,0,NULL,NULL);
		CloseHandle(hToken);
	}
	RtlInitUnicodeString(&uStr,driver_key);
	status = NtLoadDriver(&uStr);
	if(status != 0x0)
	{
		MessageBox(0,"Can't load ultradfg driver!","Error!",MB_OK | MB_ICONHAND);
		err_code = 3; goto cleanup;
	}
	hUltraDefragDevice = CreateFileW(L"\\\\.\\ultradfg",GENERIC_ALL, \
			FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING, \
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,0);
	if(hUltraDefragDevice == INVALID_HANDLE_VALUE)
	{
		MessageBox(0,"Can't access ultradfg driver!","Error!",MB_OK | MB_ICONHAND);
		err_code = 3; goto cleanup;
	}
	/* get window coordinates and other settings from registry */
	if(RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\DASoft\\NTDefrag",0,KEY_QUERY_VALUE,&hKey) == \
		ERROR_SUCCESS)
	{
		_len = sizeof(RECT);
		RegQueryValueEx(hKey,"position",NULL,NULL,(BYTE*)&win_rc,&_len);
		_len = sizeof(DWORD);
		RegQueryValueEx(hKey,"skip removable",NULL,NULL,(BYTE*)&skip_rem,&_len);
		_len = sizeof(DWORD);
		RegQueryValueEx(hKey,"update interval",NULL,NULL,
			(unsigned char*)&update_interval,&_len);
		_len = sizeof(BOOL);
		RegQueryValueEx(hKey,"show progress",NULL,NULL,(BYTE*)&show_progress,&_len);
		_len = sizeof(ULONGLONG);
		RegQueryValueEx(hKey,"sizelimit",NULL,NULL,(BYTE*)&sizelimit,&_len);
		_len = sizeof(UCHAR);
		RegQueryValueEx(hKey,"report format",NULL,NULL,(BYTE*)&report_format,&_len);
		length = 0;
		if(RegQueryValueExW(hKey,L"include filter",NULL,NULL,NULL,&_len) ==
			ERROR_SUCCESS)
		{
			in_filter = malloc(_len);
			if(in_filter)
			{
				RegQueryValueExW(hKey,L"include filter",NULL,NULL,(BYTE*)in_filter,&_len);
				length = _len;
			}
		}
		ovrl.hEvent = hEvt;
		ovrl.Offset = ovrl.OffsetHigh = 0;
		if(DeviceIoControl(hUltraDefragDevice,IOCTL_SET_INCLUDE_FILTER,
			in_filter,length,NULL,0,&txd,&ovrl))
		{
			WaitForSingleObject(hEvt,INFINITE);
		}
		length = 0;
		if(RegQueryValueExW(hKey,L"exclude filter",NULL,NULL,NULL,&_len) ==
			ERROR_SUCCESS)
		{
			ex_filter = malloc(_len);
			if(ex_filter)
			{
				RegQueryValueExW(hKey,L"exclude filter",NULL,NULL,(BYTE*)ex_filter,&_len);
				length = _len;
			}
		}
		ovrl.Offset = ovrl.OffsetHigh = 0;
		if(DeviceIoControl(hUltraDefragDevice,IOCTL_SET_EXCLUDE_FILTER,
			ex_filter,length,NULL,0,&txd,&ovrl))
		{
			WaitForSingleObject(hEvt,INFINITE);
		}
		_len = sizeof(DWORD);
		RegQueryValueEx(hKey,"dbgprint level",NULL,NULL,
			(unsigned char*)&dbgprint_level,&_len);
		ovrl.Offset = ovrl.OffsetHigh = 0;
		if(DeviceIoControl(hUltraDefragDevice,IOCTL_SET_DBGPRINT_LEVEL,
			&dbgprint_level,sizeof(DWORD),NULL,0,&txd,&ovrl))
		{
			WaitForSingleObject(hEvt,INFINITE);
		}
		_len = sizeof(UCHAR);
		RegQueryValueEx(hKey,"report type",NULL,NULL,
			(unsigned char*)&report_type,&_len);
		rt.format = report_format;
		rt.type = report_type;
		ovrl.Offset = ovrl.OffsetHigh = 0;
		if(DeviceIoControl(hUltraDefragDevice,IOCTL_SET_REPORT_TYPE,
			&rt,sizeof(REPORT_TYPE),NULL,0,&txd,&ovrl))
		{
			WaitForSingleObject(hEvt,INFINITE);
		}
		RegCloseKey(hKey);
	}
	hInstance = GetModuleHandle(NULL);
	memset((void *)work_status,0,sizeof(work_status));
	InitCommonControls();
	delta_h = GetSystemMetrics(SM_CYCAPTION) - 0x13;
	if(delta_h < 0) delta_h = 0;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1),NULL,(DLGPROC)DlgProc);
	/* delete all created gdi objects */
	for(i = 0; i < 'Z' - 'A'; i++)
	{
		if(bit_map[i])
			DeleteObject(bit_map[i]);
		if(bit_map_dc[i])
			DeleteDC(bit_map_dc[i]);
	}
	if(bit_map_grid)
		DeleteObject(bit_map_grid);
	if(bit_map_grid_dc)
		DeleteDC(bit_map_grid_dc);
	/* save window coordinates and other settings to registry */
	if(RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\DASoft\\NTDefrag",0,KEY_SET_VALUE,&hKey) != \
		ERROR_SUCCESS)
	{
		RegCreateKeyEx(HKEY_CURRENT_USER,"Software",0,NULL,0,KEY_CREATE_SUB_KEY,NULL,&hKey1,NULL);
		RegCreateKeyEx(hKey1,"DASoft",0,NULL,0,KEY_CREATE_SUB_KEY,NULL,&hKey2,NULL);
		if(RegCreateKeyEx(hKey2,"NTDefrag",0,NULL,0,KEY_SET_VALUE,NULL,&hKey,NULL) != \
			ERROR_SUCCESS)
		{
			RegCloseKey(hKey1); RegCloseKey(hKey2);
			goto cleanup;
		}
		RegCloseKey(hKey1); RegCloseKey(hKey2);
	}
	RegSetValueEx(hKey,"position",0,REG_BINARY,(BYTE*)&win_rc,sizeof(RECT));
	RegSetValueEx(hKey,"skip removable",0,REG_DWORD,(BYTE*)&skip_rem,sizeof(DWORD));
	RegSetValueEx(hKey,"update interval",0,REG_DWORD,
		(BYTE*)&update_interval,sizeof(DWORD));
	RegSetValueEx(hKey,"show progress",0,REG_BINARY,(BYTE*)&show_progress,sizeof(BOOL));
	RegSetValueEx(hKey,"sizelimit",0,REG_BINARY,(BYTE*)&sizelimit,sizeof(ULONGLONG));
	if(in_filter)
		RegSetValueExW(hKey,L"include filter",0,REG_SZ,
					(BYTE*)in_filter,(wcslen(in_filter) + 1) << 1);
	else
		RegDeleteValue(hKey,"include filter");
	if(ex_filter)
		RegSetValueExW(hKey,L"exclude filter",0,REG_SZ,
					(BYTE*)ex_filter,(wcslen(ex_filter) + 1) << 1);
	else
		RegDeleteValue(hKey,"exclude filter");
	RegSetValueEx(hKey,"report format",0,REG_BINARY,(BYTE*)&report_format,sizeof(UCHAR));
	RegSetValueEx(hKey,"report type",0,REG_BINARY,(BYTE*)&report_type,sizeof(UCHAR));
	RegSetValueEx(hKey,"dbgprint level",0,REG_DWORD,(BYTE*)&dbgprint_level,sizeof(DWORD));
	RegCloseKey(hKey);
cleanup:
	if(in_filter) free((void *)in_filter);
	if(ex_filter) free((void *)ex_filter);
	if(hEventIsRunning) CloseHandle(hEventIsRunning);
	if(hEventComplete) CloseHandle(hEventComplete);
	if(hEvt) CloseHandle(hEvt);
	if(hEvtStop) CloseHandle(hEvtStop);
	if(hUltraDefragDevice != INVALID_HANDLE_VALUE) CloseHandle(hUltraDefragDevice);
	NtUnloadDriver(&uStr);
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
	char s[11];
	char *str;
	int maxcomp, flags;
	static char tmpl[] = "A:\t      \t      \t\t       \t  \t       \t  \t00 %";
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
		if(drive_type == DRIVE_REMOVABLE && skip_rem) continue;
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
				chr = line[0]; /* to flush cpu cache ? */
				memcpy((void *)(line + 18),(void *)str,strlen(str));
				str = FormatBySize(free,s,RIGHT_ALIGNED);
				chr = line[0];
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
	DWORD txd;
	OVERLAPPED ovrl;

	ovrl.hEvent = hEvtStop;
	ovrl.Offset = ovrl.OffsetHigh = 0;
	DeviceIoControl(hUltraDefragDevice,IOCTL_ULTRADEFRAG_STOP,
					NULL,0,NULL,0,&txd,&ovrl);
	WaitForSingleObject(hEvtStop,INFINITE);
	stop_pressed = TRUE;
}

void ShowProgress(void)
{
	if(show_progress)
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
	double n;

	switch(msg)
	{
	case WM_INITDIALOG:
		{
			/* Window Initialization */
			hWindow = hWnd;
			hAccel = LoadAccelerators(hInstance,MAKEINTRESOURCE(IDR_ACCELERATOR1));
			hList = GetDlgItem(hWnd,IDC_LIST1);
			hBtnAnalyse = GetDlgItem(hWnd,IDC_ANALYSE);
			hBtnDfrg = GetDlgItem(hWnd,IDC_DEFRAGM);
			hBtnCompact = GetDlgItem(hWnd,IDC_COMPACT);
			hBtnPause = GetDlgItem(hWnd,IDC_PAUSE);
			hBtnStop = GetDlgItem(hWnd,IDC_STOP);
			hBtnFragm = GetDlgItem(hWnd,IDC_SHOWFRAGMENTED);
			hBtnRescan = GetDlgItem(hWnd,IDC_RESCAN);
			hBtnSettings = GetDlgItem(hWnd,IDC_SETTINGS);
			hCheckSkipRem = GetDlgItem(hWnd,IDC_SKIPREMOVABLE);
			if(skip_rem)
				SendMessage(hCheckSkipRem,BM_SETCHECK,BST_CHECKED,0);
			else
				SendMessage(hCheckSkipRem,BM_SETCHECK,BST_UNCHECKED,0);
			hProgressMsg = GetDlgItem(hWnd,IDC_PROGRESSMSG);
			hProgressBar = GetDlgItem(hWnd,IDC_PROGRESS1);
			HideProgress();
			hMap = GetDlgItem(hWnd,IDC_MAP);
			GetWindowRect(hMap,&_rc);
			iMAP_WIDTH = _rc.right - _rc.left - 2 * 2;
			iMAP_HEIGHT = _rc.bottom - _rc.top - 2 * 2;
			n = (double)(iMAP_WIDTH * iMAP_HEIGHT);
			n /= (double)N_BLOCKS;
			iBLOCK_SIZE = (int)floor(sqrt(n));
			if(iBLOCK_SIZE == 10) iBLOCK_SIZE --;
			cx = GetSystemMetrics(SM_CXSCREEN);
			cy = GetSystemMetrics(SM_CYSCREEN);
			if(win_rc.left < 0) win_rc.left = 0; if(win_rc.top < 0) win_rc.top = 0;
			if(win_rc.left >= cx) win_rc.left = cx - 10; if(win_rc.top >= cy) win_rc.top = cy - 10;
			GetWindowRect(hWnd,&_rc);
			dx = _rc.right - _rc.left;
			dy = _rc.bottom - _rc.top + delta_h;
			SetWindowPos(hWnd,0,win_rc.left,win_rc.top,dx,dy,0);
			hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON1));
			SendMessage(hWnd,WM_SETICON,1,(LRESULT)hIcon);
			if(hIcon) DeleteObject(hIcon);
			/* set window procs */
			OldListProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hWnd,IDC_LIST1),GWLP_WNDPROC,(LONG_PTR)ListWndProc);
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
					skip_rem = (SendMessage(hCheckSkipRem,BM_GETCHECK,0,0) == BST_CHECKED);
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
/*	case WM_LBUTTONDOWN:
		SetWindowPos(hWnd,0,0,0,0x22d,0x1b3,0);
		return FALSE;
*/	case WM_CLOSE:
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

	if((iMsg == WM_KEYDOWN || iMsg == WM_LBUTTONDOWN) && busy_flag)
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
		SetIcon(0,IDI_ICON2); SetIcon(1,IDI_ICON3);
		SetIcon(2,IDI_ICON4); SetIcon(3,IDI_ICON5);
		SetIcon(4,IDI_ICON6);
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
		if(!DeviceIoControl(hUltraDefragDevice,IOCTL_GET_STATISTIC,
			NULL,0,pst,sizeof(STATISTIC),&txd,&ovrl))
				goto wait;
		WaitForSingleObject(hEvt,INFINITE);
		UpdateStatusBar(index);
		if(!show_progress) goto get_map;
		switch(pst->current_operation)
		{
		case 'A':
			percentage = (double)(LONGLONG)pst->processed_clusters *
					(double)(LONGLONG)pst->bytes_per_cluster / 
					(double)(LONGLONG)pst->total_space;
			break;
		case 'D':
			if(pst->clusters_to_move_initial == 0)
				percentage = 1.00;
			else
				percentage = 1.00 - (double)(LONGLONG)pst->clusters_to_move / 
						(double)(LONGLONG)pst->clusters_to_move_initial;
			break;
		case 'C':
			if(pst->clusters_to_compact_initial == 0)
				percentage = 1.00;
			else
				percentage = 1.00 - (double)(LONGLONG)pst->clusters_to_compact / 
						(double)(LONGLONG)pst->clusters_to_compact_initial;
		}
		percentage *= 100.00;
		progress_msg[0] = current_operation = pst->current_operation;
		progress_msg[1] = 0x20;
		_itoa((int)percentage,progress_msg + 2,10);
		strcat(progress_msg," %");
		SetWindowText(hProgressMsg,progress_msg);
		SendMessage(hProgressBar,PBM_SETPOS,(WPARAM)percentage,0);
get_map:
		if(!DeviceIoControl(hUltraDefragDevice,IOCTL_GET_CLUSTER_MAP,
			NULL,0,_map,N_BLOCKS,&txd,&ovrl))
				goto wait;
		WaitForSingleObject(hEvt,INFINITE);
		if(!(bit_map_dc[index]))
		{
			if(!CreateBitMap(index))
			{
				MessageBox(hWindow,"Fatal GDI error #1!","Error",MB_OK | MB_ICONEXCLAMATION);
				return 0;
			}
		}
		FillBitMap(index);
		RedrawMap();
wait: 
		Sleep(update_interval);
	} while(wait_result == WAIT_TIMEOUT);
	if(!stop_pressed)
	{
		progress_msg[0] = current_operation;
		progress_msg[1] = 0;
		strcat(progress_msg," 100 %");
		SetWindowText(hProgressMsg,progress_msg);
		SendMessage(hProgressBar,PBM_SETPOS,100,0);
	}
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	LRESULT linenumber;
	int index;
	ULTRADFG_COMMAND cmd;
	DWORD txd;
	OVERLAPPED ovrl;

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
	cmd.command = (char)lpParameter;
	cmd.letter = index + 'A';
	cmd.sizelimit = sizelimit;
	///cmd.report_format = report_format;

	if(cmd.command == 'a')
		ShowStatus(STAT_AN,linenumber);
	else
		ShowStatus(STAT_DFRG,linenumber);

	stop_pressed = FALSE;
	ResetEvent(hEventComplete);
	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)UpdateMapThreadProc,(void *)(size_t)index,0,&thr_id);

	ovrl.hEvent = hEventComplete;
	ovrl.Offset = ovrl.OffsetHigh = 0;
	if(!WriteFile(hUltraDefragDevice,&cmd,sizeof(ULTRADFG_COMMAND),&txd,&ovrl))
	{
		MessageBox(0,"Incorrect drive letter or driver error!","Error!",MB_OK | MB_ICONHAND);
		ShowStatus(STAT_CLEAR,linenumber);
		ClearMap();
		SetEvent(hEventComplete);
	}
	WaitForSingleObject(hEventComplete,INFINITE);
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
	RECT _rc;
	int dx, dy;

	switch(msg)
	{
	case WM_INITDIALOG:
		/* Window Initialization */
		GetWindowRect(hWnd,&_rc);
		dx = _rc.right - _rc.left;
		dy = _rc.bottom - _rc.top;
		SetWindowPos(hWnd,0,win_rc.left + 106,win_rc.top + 84,dx,dy,0);
		return FALSE;
/*	case WM_LBUTTONDOWN:
		SetWindowPos(hWnd,0,0,0,344,286,0);
		return FALSE;
*/	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_CREDITS:
			ShellExecute(hWindow,"open",".\\CREDITS.TXT",NULL,NULL,SW_SHOW);
			break;
		case IDC_LICENSE:
			ShellExecute(hWindow,"open",".\\LICENSE.TXT",NULL,NULL,SW_SHOW);
		}
		if(LOWORD(wParam) != IDOK)
			return FALSE;
	case WM_CLOSE:
		EndDialog(hWnd,1);
		return TRUE;
	}
	return FALSE;
}

ULONGLONG get_formatted_number(char *buf)
{
	char *ext = buf;
	char c;
	ULONGLONG m = 1;

	while(*ext)
	{
		c = *ext;
		switch(c)
		{
		case 'k':
		case 'K':
			m = 1024;
			*ext = 0;
			goto done;
		case 'm':
		case 'M':
			m = 1024 * 1024;
			*ext = 0;
			goto done;
		case 'g':
		case 'G':
			m = 1024 * 1024 * 1024;
			*ext = 0;
			goto done;
		case 't':
		case 'T':
			m = (ULONGLONG)1024 * 1024 * 1024 * 1024;
			*ext = 0;
			goto done;
		}
		ext++;
	}
done:
	return m * _atoi64(buf);
}

void format_number(ULONGLONG number,char *buf)
{
	char *ext[] = {""," Kb"," Mb"," Gb"," Tb"};
	int i = 0;

	while(number >= 1024)
	{
		i ++;
		number /= 1024;
	}
	_i64toa(number,buf,10);
	strcat(buf,ext[i]);
}

void UpdateFilter(HWND hWnd,short **buffer,DWORD ioctl_code)
{
	int length;
	DWORD txd;
	OVERLAPPED ovrl;

	length = GetWindowTextLength(hWnd);
	if(*buffer)
	{
		free((void *)*buffer);
		*buffer = NULL;
	}
	if(length)
	{
		length ++; /* for term. zero */
		*buffer = malloc(length * sizeof(short));
		if(*buffer) GetWindowTextW(hWnd,*buffer,length);
	}
	ovrl.hEvent = hEvt;
	ovrl.Offset = ovrl.OffsetHigh = 0;
	if(DeviceIoControl(hUltraDefragDevice,ioctl_code,
		*buffer,length * sizeof(short),NULL,0,&txd,&ovrl))
	{
		WaitForSingleObject(hEvt,INFINITE);
	}
}

BOOL CALLBACK SettingsDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	char buf[24];
	DWORD new_dbgprint_level;
	OVERLAPPED ovrl;
	DWORD txd;
	TC_ITEM tci;
	LPNMHDR lpn;
	LRESULT i;
	REPORT_TYPE rt;
	RECT _rc;
	int dx, dy;

	switch(msg)
	{
	case WM_INITDIALOG:
		/* Window Initialization */
		GetWindowRect(hWnd,&_rc);
		dx = _rc.right - _rc.left;
		dy = _rc.bottom - _rc.top;
		SetWindowPos(hWnd,0,win_rc.left + 138,win_rc.top + 81,dx,dy,0);
		hTabCtrl = GetDlgItem(hWnd,IDC_TAB1);
		tci.mask = TCIF_TEXT;
		tci.iImage = -1;
		tci.pszText = "Filter";
		SendMessage(hTabCtrl,TCM_INSERTITEM,0,(LPARAM)&tci);
		tci.pszText = "  GUI";
		SendMessage(hTabCtrl,TCM_INSERTITEM,1,(LPARAM)&tci);
		tci.pszText = "Report";
		SendMessage(hTabCtrl,TCM_INSERTITEM,2,(LPARAM)&tci);
		hFilterDlg = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_FILTER),hTabCtrl,
			(DLGPROC)FilterDlgProc,0);
		ShowWindow(hFilterDlg,SW_SHOWNORMAL);
		hGuiDlg = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_GUI),hTabCtrl,
			(DLGPROC)EmptyDlgProc,0);
		ShowWindow(hGuiDlg,SW_HIDE);
		hReportDlg = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_REPORT),hTabCtrl,
			(DLGPROC)EmptyDlgProc,0);
		ShowWindow(hReportDlg,SW_HIDE);

		format_number(sizelimit,buf);
		SetWindowText(GetDlgItem(hFilterDlg,IDC_SIZELIMIT),buf);
		_itoa(update_interval,buf,10);
		SetWindowText(GetDlgItem(hGuiDlg,IDC_UPDATE_INTERVAL),buf);
		if(in_filter)
			SetWindowTextW(GetDlgItem(hFilterDlg,IDC_INCLUDE),in_filter);
		if(ex_filter)
			SetWindowTextW(GetDlgItem(hFilterDlg,IDC_EXCLUDE),ex_filter);
		if(report_format == UTF_FORMAT)
			SendMessage(GetDlgItem(hReportDlg,IDC_UTF16),BM_SETCHECK,BST_CHECKED,0);
		else
			SendMessage(GetDlgItem(hReportDlg,IDC_ASCII),BM_SETCHECK,BST_CHECKED,0);
		if(report_type == HTML_REPORT)
			SendMessage(GetDlgItem(hReportDlg,IDC_HTML),BM_SETCHECK,BST_CHECKED,0);
		else
			SendMessage(GetDlgItem(hReportDlg,IDC_NONE),BM_SETCHECK,BST_CHECKED,0);
		if(dbgprint_level == 0)
			SendMessage(GetDlgItem(hReportDlg,IDC_DBG_NORMAL),BM_SETCHECK,BST_CHECKED,0);
		else
			SendMessage(GetDlgItem(hReportDlg,IDC_DBG_DETAILED),BM_SETCHECK,BST_CHECKED,0);
		if(show_progress)
			SendMessage(GetDlgItem(hGuiDlg,IDC_SHOWPROGRESS),BM_SETCHECK,BST_CHECKED,0);
		in_edit_flag = ex_edit_flag = FALSE;
		return TRUE;
/*	case WM_LBUTTONDOWN:
		SetWindowPos(hWnd,0,0,0,279,266,0);
		return FALSE;
*/	case WM_NOTIFY:
		lpn = (LPNMHDR)lParam;
		if(lpn->code == TCN_SELCHANGE)
		{
			i = SendMessage(hTabCtrl,TCM_GETCURSEL,(WPARAM)lpn->hwndFrom,0);
			switch(i)
			{
			case 0:
				ShowWindow(hFilterDlg,SW_SHOW);
				ShowWindow(hGuiDlg,SW_HIDE);
				ShowWindow(hReportDlg,SW_HIDE);
				break;
			case 1:
				ShowWindow(hFilterDlg,SW_HIDE);
				ShowWindow(hGuiDlg,SW_SHOW);
				ShowWindow(hReportDlg,SW_HIDE);
				break;
			case 2:
				ShowWindow(hFilterDlg,SW_HIDE);
				ShowWindow(hGuiDlg,SW_HIDE);
				ShowWindow(hReportDlg,SW_SHOW);
			}
		}
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			GetWindowText(GetDlgItem(hFilterDlg,IDC_SIZELIMIT),buf,22);
			sizelimit = get_formatted_number(buf);
			GetWindowText(GetDlgItem(hGuiDlg,IDC_UPDATE_INTERVAL),buf,22);
			update_interval = atoi(buf);
			if(in_edit_flag)
				UpdateFilter(GetDlgItem(hFilterDlg,IDC_INCLUDE),&in_filter,
												IOCTL_SET_INCLUDE_FILTER);
			if(ex_edit_flag)
				UpdateFilter(GetDlgItem(hFilterDlg,IDC_EXCLUDE),&ex_filter,
												IOCTL_SET_EXCLUDE_FILTER);
			report_format = \
				(SendMessage(GetDlgItem(hReportDlg,IDC_UTF16),BM_GETCHECK,0,0) == \
				BST_CHECKED) ? UTF_FORMAT : ASCII_FORMAT;
			report_type = \
				(SendMessage(GetDlgItem(hReportDlg,IDC_HTML),BM_GETCHECK,0,0) == \
				BST_CHECKED) ? HTML_REPORT : NO_REPORT;
			rt.format = report_format;
			rt.type = report_type;
			ovrl.hEvent = hEvt;
			ovrl.Offset = ovrl.OffsetHigh = 0;
			if(DeviceIoControl(hUltraDefragDevice,IOCTL_SET_REPORT_TYPE,
				&rt,sizeof(REPORT_TYPE),NULL,0,&txd,&ovrl))
			{
				WaitForSingleObject(hEvt,INFINITE);
			}
			new_dbgprint_level = \
				(SendMessage(GetDlgItem(hReportDlg,IDC_DBG_NORMAL),BM_GETCHECK,0,0) == \
				BST_CHECKED) ? 0 : 1;
			if(new_dbgprint_level != dbgprint_level)
			{
				dbgprint_level = new_dbgprint_level;
				ovrl.hEvent = hEvt;
				ovrl.Offset = ovrl.OffsetHigh = 0;
				if(DeviceIoControl(hUltraDefragDevice,IOCTL_SET_DBGPRINT_LEVEL,
					&dbgprint_level,sizeof(DWORD),NULL,0,&txd,&ovrl))
				{
					WaitForSingleObject(hEvt,INFINITE);
				}
			}
			show_progress = \
				(SendMessage(GetDlgItem(hGuiDlg,IDC_SHOWPROGRESS),BM_GETCHECK,0,0) == \
				BST_CHECKED) ? TRUE : FALSE;
			if(!show_progress)
				HideProgress();
			EndDialog(hWnd,1);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWnd,0);
			return TRUE;
		}
		return FALSE;
	case WM_CLOSE:
		EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

BOOL CALLBACK FilterDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_INCLUDE:
			if(HIWORD(wParam) == EN_CHANGE)
				in_edit_flag = TRUE;
			break;
		case IDC_EXCLUDE:
			if(HIWORD(wParam) == EN_CHANGE)
				ex_edit_flag = TRUE;
			break;
		}
		return FALSE;
	case WM_CLOSE:
		EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

BOOL CALLBACK EmptyDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_CLOSE:
		EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

BOOL CreateBitMap(signed int index)
{
	BYTE *ppvBits;
	BYTE *data;
	BITMAPINFOHEADER *bh;
	HDC hDC;
	unsigned short res;
	HBITMAP hBmp;

	data = (BYTE *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, \
		iMAP_WIDTH * iMAP_HEIGHT * sizeof(RGBQUAD) + sizeof(BITMAPINFOHEADER));
	if(!data) return FALSE;
	bh = (BITMAPINFOHEADER*)data;
	hDC    = GetDC(hWindow);
	res = (unsigned short)GetDeviceCaps(hDC, BITSPIXEL);
	ReleaseDC(hWindow, hDC);

	bh->biWidth       = iMAP_WIDTH;
	bh->biHeight      = iMAP_HEIGHT;
	bh->biPlanes      = 1;
	bh->biBitCount    = res;
	bh->biClrUsed     = bh->biClrImportant = 32;
	bh->biSizeImage   = 0;
	bh->biCompression = BI_RGB;
	bh->biSize        = sizeof(BITMAPINFOHEADER);

	/* create the bitmap */
	hBmp = CreateDIBSection(0, (BITMAPINFO*)bh, \
		DIB_RGB_COLORS, &ppvBits, NULL, 0);
	if(!hBmp)
	{
		HeapFree(GetProcessHeap(),0,data);
		return FALSE;
	}

	hDC = CreateCompatibleDC(0);
	SelectObject(hDC,hBmp);
	SetBkMode(hDC,TRANSPARENT);

	if(index >= 0)
	{
		bit_map[index] = hBmp;
		bit_map_dc[index] = hDC;
	}
	else
	{
		bit_map_grid = hBmp;
		bit_map_grid_dc = hDC;
	}
	return TRUE;
}

BOOL CreateBitMapGrid()
{
	HBRUSH hBrush, hOldBrush;
	HPEN hPen, hOldPen;
	int i;
	RECT rc;

	if(!CreateBitMap(-1))
		return FALSE;
	/* draw grid */
	rc.top = rc.left = 0;
	rc.bottom = iMAP_HEIGHT;
	rc.right = iMAP_WIDTH;
	hBrush = GetStockObject(WHITE_BRUSH);
	hOldBrush = SelectObject(bit_map_grid_dc,hBrush);
	FillRect(bit_map_grid_dc,&rc,hBrush);
	SelectObject(bit_map_grid_dc,hOldBrush);
	DeleteObject(hBrush);

	hPen = CreatePen(PS_SOLID,1,GRID_COLOR);
	hOldPen = SelectObject(bit_map_grid_dc,hPen);
	for(i = 0; i < BLOCKS_PER_HLINE + 1; i++)
	{
		MoveToEx(bit_map_grid_dc,(iBLOCK_SIZE + 1) * i,0,NULL);
		LineTo(bit_map_grid_dc,(iBLOCK_SIZE + 1) * i,iMAP_HEIGHT);
	}
	for(i = 0; i < BLOCKS_PER_VLINE + 1; i++)
	{
		MoveToEx(bit_map_grid_dc,0,(iBLOCK_SIZE + 1) * i,NULL);
		LineTo(bit_map_grid_dc,iMAP_WIDTH,(iBLOCK_SIZE + 1) * i);
	}
	SelectObject(bit_map_grid_dc,hOldPen);
	DeleteObject(hPen);
	return TRUE;
}

void DrawBlock(HDC hdc,char *__map,int space_state,COLORREF color)
{
	HBRUSH hBrush, hOldBrush;
	int k,col,row;
	RECT block_rc;

	hBrush = CreateSolidBrush(color);
	hOldBrush = SelectObject(hdc,hBrush);

	for(k = 0; k < N_BLOCKS; k++)
	{
		if(__map[k] == space_state)
		{
			col = k % BLOCKS_PER_HLINE;
			row = k / BLOCKS_PER_HLINE;
			block_rc.top = (iBLOCK_SIZE + 1) * row + 1;
			block_rc.left = (iBLOCK_SIZE + 1) * col + 1;
			block_rc.right = block_rc.left + iBLOCK_SIZE;
			block_rc.bottom = block_rc.top + iBLOCK_SIZE;
			FillRect(hdc,&block_rc,hBrush);
		}
	}
	SelectObject(hdc,hOldBrush);
	DeleteObject(hBrush);
}

BOOL FillBitMap(int index)
{
	HDC hdc;
	HBRUSH hBrush, hOldBrush;
	char *_map;
	RECT rc;

	rc.top = rc.left = 0;
	rc.bottom = iMAP_HEIGHT;
	rc.right = iMAP_WIDTH;
	_map = map[index];
	hdc = bit_map_dc[index];
	if(!hdc)
		return FALSE;
	hBrush = CreateSolidBrush(GRID_COLOR);
	hOldBrush = SelectObject(hdc,hBrush);
	FillRect(hdc,&rc,hBrush);
	SelectObject(hdc,hOldBrush);
	DeleteObject(hBrush);
	/* Show free space */
	DrawBlock(hdc,_map,FREE_SPACE,RGB(255,255,255));
	/* Show unfragmented space */
	DrawBlock(hdc,_map,UNFRAGM_SPACE,UNFRAGM_COLOR);
	/* Show fragmented space */
	DrawBlock(hdc,_map,FRAGM_SPACE,FRAGM_COLOR);
	/* Show system space */
	DrawBlock(hdc,_map,SYSTEM_SPACE,SYSTEM_COLOR);
	/* Show MFT space */
	DrawBlock(hdc,_map,MFT_SPACE,MFT_COLOR);
	/* Show directory space */
	DrawBlock(hdc,_map,DIR_SPACE,DIR_COLOR);
	/* Show compressed space */
	DrawBlock(hdc,_map,COMPRESSED_SPACE,COMPRESSED_COLOR);
	/* Show no checked space */
	DrawBlock(hdc,_map,NO_CHECKED_SPACE,NO_CHECKED_COLOR);
	/* Show space of big files */
	DrawBlock(hdc,_map,SYSTEM_OVERLIMIT_SPACE,SYSTEM_OVERLIMIT_COLOR);
	DrawBlock(hdc,_map,FRAGM_OVERLIMIT_SPACE,FRAGM_OVERLIMIT_COLOR);
	DrawBlock(hdc,_map,UNFRAGM_OVERLIMIT_SPACE,UNFRAGM_OVERLIMIT_COLOR);
	DrawBlock(hdc,_map,DIR_OVERLIMIT_SPACE,DIR_OVERLIMIT_COLOR);
	DrawBlock(hdc,_map,COMPRESSED_OVERLIMIT_SPACE,COMPRESSED_OVERLIMIT_COLOR);
	return TRUE;
}

void ClearMap()
{
	HDC hdc;

	hdc = GetDC(hMap);
	BitBlt(hdc,0,0,iMAP_WIDTH,iMAP_HEIGHT,bit_map_grid_dc,0,0,SRCCOPY);
	ReleaseDC(hMap,hdc);
}

void RedrawMap()
{
	HDC hdc;
	LRESULT index;

	hdc = GetDC(hMap);
	index = SendMessage(hList,LB_GETCURSEL,0,0);
	if(index != LB_ERR)
	{
		index = letter_numbers[index - 2];
	}
	else
	{
		BitBlt(hdc,0,0,iMAP_WIDTH,iMAP_HEIGHT,bit_map_grid_dc,0,0,SRCCOPY);
		goto exit_redraw;
	}
	if(work_status[index] <= 1 || !bit_map_dc[index])
	{
		BitBlt(hdc,0,0,iMAP_WIDTH,iMAP_HEIGHT,bit_map_grid_dc,0,0,SRCCOPY);
		goto exit_redraw;
	}
	BitBlt(hdc,0,0,iMAP_WIDTH,iMAP_HEIGHT,bit_map_dc[index],0,0,MERGECOPY);
exit_redraw:
	ReleaseDC(hMap,hdc);
}
