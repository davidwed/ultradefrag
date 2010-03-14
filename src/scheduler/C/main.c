/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007 by Justin Dearing (zippy1981@gmail.com)
 *  Copyright (c) 2009-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* Scheduler - main window code.
*/

/*
* This program is based on the original 
* UltraDefrag Scheduler.NET source code.
*/

#define WIN32_NO_STATUS
#include <windows.h>
/*
* Next definition is very important for mingw:
* _WIN32_IE must be no less than 0x0400
* to include some important constant definitions.
*/
#ifndef _WIN32_IE
#define _WIN32_IE 0x0400
#endif
#include <commctrl.h>

#include <commdlg.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>

#define lua_c
#include "../../lua5.1/lua.h"
#include "../../lua5.1/lauxlib.h"
#include "../../lua5.1/lualib.h"

#include "resource.h"

#include "../../dll/wgx/wgx.h"
#include "../../include/udefrag.h"

#ifdef USE_MSVC
#define DWORD_PTR DWORD*
#endif

#if defined(USE_WINSDK) || defined(USE_WINDDK) || defined(USE_MSVC)
#define NET_API_STATUS DWORD
#define NERR_Success 0 
#define JOB_RUN_PERIODICALLY	1
#define JOB_EXEC_ERROR	2
#define JOB_RUNS_TODAY	4
#define JOB_ADD_CURRENT_DATE	8
#define JOB_NONINTERACTIVE	16
NET_API_STATUS __stdcall NetScheduleJobAdd(
  LPCWSTR Servername,
  LPBYTE Buffer,
  LPDWORD JobId
);
typedef struct _AT_INFO {
	DWORD_PTR JobTime;
	DWORD DaysOfMonth;
	UCHAR DaysOfWeek;
	UCHAR Flags;
	LPWSTR Command;
} AT_INFO, *PAT_INFO, *LPAT_INFO;
#else
#include <lm.h> /* for NetScheduleJobAdd() */
#endif

/* Global variables */
HINSTANCE hInstance;
HWND hWindow, hDrives;
extern WGX_I18N_RESOURCE_ENTRY i18n_table[];

LOGFONT lf;
HFONT hFont = NULL;

//char buffer[MAX_PATH];

/* Function prototypes */
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void InitFont(void);

void InitDrivesList(void);

void SchedulerAddJob(void);

void DisplayLastError(char *caption)
{
	LPVOID lpMsgBuf;
	char buffer[128];
	DWORD error = GetLastError();

	if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,0,NULL)){
				(void)_snprintf(buffer,sizeof(buffer),
						"Error code = 0x%x",(UINT)error);
				buffer[sizeof(buffer) - 1] = 0;
				MessageBoxA(NULL,buffer,caption,MB_OK | MB_ICONHAND);
				return;
	} else {
		MessageBoxA(NULL,(LPCTSTR)lpMsgBuf,caption,MB_OK | MB_ICONHAND);
		LocalFree(lpMsgBuf);
	}
}

/*-------------------- Main Function -----------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	INITCOMMONCONTROLSEX icc;
	
	hInstance = GetModuleHandle(NULL);
	/*
	* A date and time picker control requires
	* comctl32.dll version 4.70 or later.
	*/
	icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icc.dwICC = ICC_DATE_CLASSES;
	if(!InitCommonControlsEx(&icc)){
		DisplayLastError("InitCommonControlsEx() failed!");
		return 2;
	}
	if(DialogBox(hInstance, MAKEINTRESOURCE(IDD_SCHEDULER),NULL,(DLGPROC)DlgProc) == (-1)){
		DisplayLastError("Cannot create the main window!");
		return 1;
	}
	if(hFont) (void)DeleteObject(hFont);
	WgxDestroyResourceTable(i18n_table);
	return 0;
}

/*---------------- Main Dialog Callback ---------------------*/
BOOL CALLBACK DlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg){
	case WM_INITDIALOG:
		/* Window Initialization */
		hWindow = hWnd;
		hDrives = GetDlgItem(hWindow,IDC_DRIVES);
		if(WgxBuildResourceTable(i18n_table,L".\\ud_scheduler_i18n.lng"))
			WgxApplyResourceTable(i18n_table,hWindow);
		WgxSetIcon(hInstance,hWindow,IDI_SCHEDULER);
		InitFont();
		(void)SendMessage(GetDlgItem(hWindow,IDC_WEEKLY),BM_SETCHECK,BST_CHECKED,0);
		InitDrivesList();
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_DAILY:
			if(SendMessage(GetDlgItem(hWindow,IDC_DAILY),BM_GETCHECK,0,0) == BST_CHECKED)
				WgxDisableWindows(hWindow,IDC_MONDAY,IDC_TUESDAY,IDC_WEDNESDAY,
					IDC_THURSDAY,IDC_FRIDAY,IDC_SATURDAY,IDC_SUNDAY,0);
			break;
		case IDC_WEEKLY:
			if(SendMessage(GetDlgItem(hWindow,IDC_WEEKLY),BM_GETCHECK,0,0) == BST_CHECKED)
				WgxEnableWindows(hWindow,IDC_MONDAY,IDC_TUESDAY,IDC_WEDNESDAY,
					IDC_THURSDAY,IDC_FRIDAY,IDC_SATURDAY,IDC_SUNDAY,0);
			break;
		case IDOK:
			SchedulerAddJob();
			break;
		}
		break;
	case WM_CLOSE:
		EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

void InitFont(void)
{
	HFONT hNewFont;

	/* initialize LOGFONT structure */
	memset(&lf,0,sizeof(LOGFONT)); /* FIXME: may fail on x64? */
	/* default font should be Courier New 9pt */
	(void)strcpy(lf.lfFaceName,"Courier New");
	lf.lfHeight = -12;
	
	/* load saved font settings */
	if(!WgxGetLogFontStructureFromFile(".\\options\\font.lua",&lf))
		return;
	
	/* apply font to application's window */
	hNewFont = WgxSetFont(hWindow,&lf);
	if(hNewFont){
		if(hFont) (void)DeleteObject(hFont);
		hFont = hNewFont;
	}
}

void SchedulerAddJob(void)
{
	SYSTEMTIME st;
	AT_INFO ati;
	WCHAR drive[32];
	WCHAR cmd[256];
	DWORD JobId;
	int SelectedItems[MAX_DOS_DRIVES];
	int n, i, index;
	NET_API_STATUS status;
	
	if(SendMessage(GetDlgItem(hWindow,IDC_DATETIMEPICKER1),DTM_GETSYSTEMTIME,
	  0,(LPARAM)(LPSYSTEMTIME)&st) != GDT_VALID){
		MessageBox(hWindow,"Cannot retrieve time specified!","Error",MB_OK | MB_ICONHAND);
		return;
	}
	
	n = (int)(DWORD_PTR)SendMessage(hDrives,LB_GETSELITEMS,MAX_DOS_DRIVES,(LPARAM)SelectedItems);
	if(n != LB_ERR && n <= MAX_DOS_DRIVES){
		(void)wcscpy(cmd,L"udefrag ");
		for(i = 0; i < n; i++){
			index = SelectedItems[i];
			if(SendMessageW(hDrives,LB_GETTEXT,(WPARAM)index,(LPARAM)drive) != LB_ERR){
				(void)wcscat(cmd,drive);
				(void)wcscat(cmd,L" ");
			}
		}
			  
		ati.JobTime = (st.wHour * 60 * 60 * 1000) + (st.wMinute * 60 * 1000) + (st.wSecond * 1000);
		ati.DaysOfMonth = 0;
		ati.DaysOfWeek = 0;
		ati.Flags = JOB_RUN_PERIODICALLY | JOB_NONINTERACTIVE;
		ati.Command = cmd;
		
		if(SendMessage(GetDlgItem(hWindow,IDC_DAILY),BM_GETCHECK,0,0) == BST_CHECKED)
			ati.DaysOfWeek = 0x7F;
		else {
			if(SendMessage(GetDlgItem(hWindow,IDC_MONDAY),BM_GETCHECK,0,0) == BST_CHECKED) ati.DaysOfWeek |= 0x1;
			if(SendMessage(GetDlgItem(hWindow,IDC_TUESDAY),BM_GETCHECK,0,0) == BST_CHECKED) ati.DaysOfWeek |= 0x2;
			if(SendMessage(GetDlgItem(hWindow,IDC_WEDNESDAY),BM_GETCHECK,0,0) == BST_CHECKED) ati.DaysOfWeek |= 0x4;
			if(SendMessage(GetDlgItem(hWindow,IDC_THURSDAY),BM_GETCHECK,0,0) == BST_CHECKED) ati.DaysOfWeek |= 0x8;
			if(SendMessage(GetDlgItem(hWindow,IDC_FRIDAY),BM_GETCHECK,0,0) == BST_CHECKED) ati.DaysOfWeek |= 0x10;
			if(SendMessage(GetDlgItem(hWindow,IDC_SATURDAY),BM_GETCHECK,0,0) == BST_CHECKED) ati.DaysOfWeek |= 0x20;
			if(SendMessage(GetDlgItem(hWindow,IDC_SUNDAY),BM_GETCHECK,0,0) == BST_CHECKED) ati.DaysOfWeek |= 0x40;
		}
		
		status = NetScheduleJobAdd(NULL,(LPBYTE)&ati,&JobId);
		if(status != NERR_Success){
			SetLastError((DWORD)status);
			DisplayLastError("Cannot add a specified job!");
		}
	}
}

void InitDrivesList(void)
{
	volume_info *v;
	int i;
	char buffer[64];

	if(udefrag_get_avail_volumes(&v,TRUE) >= 0){ /* skip removable media */
		for(i = 0; v[i].letter != 0; i++){
			(void)sprintf(buffer,"%c:\\",v[i].letter);
			(void)SendMessage(hDrives,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)buffer);
		}
	}
	(void)SendMessage(hDrives,LB_SETCURSEL,0,0);
}
