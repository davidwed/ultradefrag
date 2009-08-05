/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007 by Justin Dearing (zippy1981@gmail.com)
 *  Copyright (c) 2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

#include <lm.h> /* for NetScheduleJobAdd() */

/* Global variables */
HINSTANCE hInstance;
HWND hWindow, hDrives;
extern WGX_I18N_RESOURCE_ENTRY i18n_table[];

LOGFONT lf;
HFONT hFont = NULL;

char buffer[MAX_PATH];

/* Function prototypes */
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void InitFont(void);

void InitDrivesList(void);

void SchedulerAddJob(void);

/*-------------------- Main Function -----------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	hInstance = GetModuleHandle(NULL);
	/*
	* This call needs on dmitriar's pc (on xp) no more than 550 cpu tacts,
	* but InitCommonControlsEx() needs about 90000 tacts.
	* Because the first function is just a stub on xp.
	*/
	InitCommonControls();
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_SCHEDULER),NULL,(DLGPROC)DlgProc);
	if(hFont) DeleteObject(hFont);
	WgxDestroyResourceTable(i18n_table);
	return 0;
}

/*---------------- Main Dialog Callback ---------------------*/
BOOL CALLBACK DlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	short path[MAX_PATH];
//	LRESULT check_state;

	switch(msg){
	case WM_INITDIALOG:
		/* Window Initialization */
		hWindow = hWnd;
		hDrives = GetDlgItem(hWindow,IDC_DRIVES);
		GetWindowsDirectoryW(path,MAX_PATH);
		wcscat(path,L"\\UltraDefrag\\ud_scheduler_i18n.lng");
		if(WgxBuildResourceTable(i18n_table,path))
			WgxApplyResourceTable(i18n_table,hWindow);
		WgxSetIcon(hInstance,hWindow,IDI_SCHEDULER);
		InitFont();
		SendMessage(GetDlgItem(hWindow,IDC_WEEKLY),BM_SETCHECK,BST_CHECKED,0);
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
	memset(&lf,0,sizeof(LOGFONT));
	/* default font should be Courier New 9pt */
	strcpy(lf.lfFaceName,"Courier New");
	lf.lfHeight = -12;
	
	/* load saved font settings */
	GetWindowsDirectory(buffer,MAX_PATH);
	strcat(buffer,"\\UltraDefrag\\options\\font.lua");
	if(!WgxGetLogFontStructureFromFile(buffer,&lf)) return;
	
	/* apply font to application's window */
	hNewFont = WgxSetFont(hWindow,&lf);
	if(hNewFont){
		if(hFont) DeleteObject(hFont);
		hFont = hNewFont;
	}
}

void SchedulerAddJob(void)
{
	SYSTEMTIME st;
	AT_INFO ati;
	WCHAR drive[32];
	WCHAR cmd[64];
	DWORD JobId;
	int index;
	
	if(SendMessage(GetDlgItem(hWindow,IDC_DATETIMEPICKER1),DTM_GETSYSTEMTIME,
	  0,(LPARAM)(LPSYSTEMTIME)&st) != GDT_VALID){
		MessageBox(hWindow,"Cannot retrieve specified time!","Error",MB_OK | MB_ICONHAND);
		return;
	}
	
	index = (int)(DWORD_PTR)SendMessage(hDrives,CB_GETCURSEL,0,0);
	SendMessageW(hDrives,CB_GETLBTEXT,(WPARAM)index,(LPARAM)drive);
	swprintf(cmd,L"udefrag %s",drive);
	  
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
	
	if(NetScheduleJobAdd(NULL,(LPBYTE)&ati,&JobId) != NERR_Success)
		MessageBox(hWindow,"Cannot add specified job!","Error",MB_OK | MB_ICONHAND);
}

void InitDrivesList(void)
{
	ERRORHANDLERPROC eh;
	volume_info *v;
	int i;
	char buffer[64];

	eh = udefrag_set_error_handler(NULL);
	if(udefrag_get_avail_volumes(&v,TRUE) >= 0){ /* skip removable media */
		for(i = 0; v[i].letter != 0; i++){
			sprintf(buffer,"%c:\\",v[i].letter);
			SendMessage(hDrives,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)buffer);
		}
	}
	udefrag_set_error_handler(eh);
	SendMessage(hDrives,CB_SETCURSEL,0,0);
}
