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
 *  GUI - load/save/change program settings.
 */

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "../include/ultradfg.h"

#include "resource.h"

extern HINSTANCE hInstance;
extern HWND hWindow;
RECT win_rc; /* coordinates of main window */

HWND hTabCtrl;
HWND hFilterDlg,hGuiDlg,hReportDlg,hBootSchedDlg;

BOOL CALLBACK FilterDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK BootSchedDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK EmptyDlgProc(HWND, UINT, WPARAM, LPARAM);

//extern void HideProgress();

char buffer[32768];
#define MAX_FILTER_SIZE 4096
#define MAX_FILTER_BYTESIZE (MAX_FILTER_SIZE * sizeof(short))
short in_filter[MAX_FILTER_SIZE + 1];
short ex_filter[MAX_FILTER_SIZE + 1];
short boot_in_filter[MAX_FILTER_SIZE + 1];
short boot_ex_filter[MAX_FILTER_SIZE + 1];

extern ud_options *settings;

BOOL CALLBACK SettingsDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	char buf[68];
	TC_ITEM tci;
	LPNMHDR lpn;
	LRESULT i;
	int check_id;

	switch(msg)
	{
	case WM_INITDIALOG:
		/* Window Initialization */
		SetWindowPos(hWnd,0,win_rc.left + 70,win_rc.top + 55,0,0,SWP_NOSIZE);
		hTabCtrl = GetDlgItem(hWnd,IDC_TAB1);
		tci.mask = TCIF_TEXT;
		tci.iImage = -1;
		tci.pszText = "Filter";
		SendMessage(hTabCtrl,TCM_INSERTITEM,0,(LPARAM)&tci);
		tci.pszText = "  GUI";
		SendMessage(hTabCtrl,TCM_INSERTITEM,1,(LPARAM)&tci);
		tci.pszText = "Report";
		SendMessage(hTabCtrl,TCM_INSERTITEM,2,(LPARAM)&tci);
		tci.pszText = "Boot time";
		SendMessage(hTabCtrl,TCM_INSERTITEM,3,(LPARAM)&tci);
		hFilterDlg = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_FILTER),hTabCtrl,
			(DLGPROC)FilterDlgProc,0);
		ShowWindow(hFilterDlg,SW_SHOWNORMAL);
		hGuiDlg = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_GUI),hTabCtrl,
			(DLGPROC)EmptyDlgProc,0);
		ShowWindow(hGuiDlg,SW_HIDE);
		hReportDlg = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_REPORT),hTabCtrl,
			(DLGPROC)EmptyDlgProc,0);
		ShowWindow(hReportDlg,SW_HIDE);
		hBootSchedDlg = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_BOOT_SCHEDULER),hTabCtrl,
			(DLGPROC)BootSchedDlgProc,0);
		ShowWindow(hBootSchedDlg,SW_HIDE);

		fbsize2(buf,settings->sizelimit);
		SetWindowText(GetDlgItem(hFilterDlg,IDC_SIZELIMIT),buf);
		_itoa(settings->update_interval,buf,10);
		SetWindowText(GetDlgItem(hGuiDlg,IDC_UPDATE_INTERVAL),buf);
		SetWindowTextW(GetDlgItem(hFilterDlg,IDC_INCLUDE),settings->in_filter);
		SetWindowTextW(GetDlgItem(hFilterDlg,IDC_EXCLUDE),settings->ex_filter);
		if(settings->report_format == UTF_FORMAT)
			SendMessage(GetDlgItem(hReportDlg,IDC_UTF16),BM_SETCHECK,BST_CHECKED,0);
		else
			SendMessage(GetDlgItem(hReportDlg,IDC_ASCII),BM_SETCHECK,BST_CHECKED,0);
		if(settings->report_type == HTML_REPORT)
			SendMessage(GetDlgItem(hReportDlg,IDC_HTML),BM_SETCHECK,BST_CHECKED,0);
		else
			SendMessage(GetDlgItem(hReportDlg,IDC_NONE),BM_SETCHECK,BST_CHECKED,0);
		switch(settings->dbgprint_level)
		{
		case 0:
			check_id = IDC_DBG_NORMAL;
			break;
		case 1:
			check_id = IDC_DBG_DETAILED;
			break;
		default:
			check_id = IDC_DBG_PARANOID;
		}
		SendMessage(GetDlgItem(hReportDlg,check_id),BM_SETCHECK,BST_CHECKED,0);
		//if(settings->show_progress)
		//	SendMessage(GetDlgItem(hGuiDlg,IDC_SHOWPROGRESS),BM_SETCHECK,BST_CHECKED,0);
		SetWindowTextW(GetDlgItem(hBootSchedDlg,IDC_LETTERS),settings->sched_letters);
		if(settings->next_boot)
			SendMessage(GetDlgItem(hBootSchedDlg,IDC_NEXTBOOT),BM_SETCHECK,BST_CHECKED,0);
		if(settings->every_boot)
			SendMessage(GetDlgItem(hBootSchedDlg,IDC_EVERYBOOT),BM_SETCHECK,BST_CHECKED,0);
		if(settings->only_reg_and_pagefile)
			SendMessage(GetDlgItem(hBootSchedDlg,IDC_REGANDPAGEFILE),BM_SETCHECK,BST_CHECKED,0);
		SetWindowTextW(GetDlgItem(hBootSchedDlg,IDC_INCLUDE2),settings->boot_in_filter);
		SetWindowTextW(GetDlgItem(hBootSchedDlg,IDC_EXCLUDE2),settings->boot_ex_filter);
		return TRUE;
	case WM_NOTIFY:
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
				ShowWindow(hBootSchedDlg,SW_HIDE);
				break;
			case 1:
				ShowWindow(hFilterDlg,SW_HIDE);
				ShowWindow(hGuiDlg,SW_SHOW);
				ShowWindow(hReportDlg,SW_HIDE);
				ShowWindow(hBootSchedDlg,SW_HIDE);
				break;
			case 2:
				ShowWindow(hFilterDlg,SW_HIDE);
				ShowWindow(hGuiDlg,SW_HIDE);
				ShowWindow(hReportDlg,SW_SHOW);
				ShowWindow(hBootSchedDlg,SW_HIDE);
				break;
			case 3:
				ShowWindow(hFilterDlg,SW_HIDE);
				ShowWindow(hGuiDlg,SW_HIDE);
				ShowWindow(hReportDlg,SW_HIDE);
				ShowWindow(hBootSchedDlg,SW_SHOW);
			}
		}
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			GetWindowText(GetDlgItem(hFilterDlg,IDC_SIZELIMIT),buf,22);
			dfbsize2(buf,&settings->sizelimit);
			GetWindowText(GetDlgItem(hGuiDlg,IDC_UPDATE_INTERVAL),buf,22);
			settings->update_interval = atoi(buf);
			settings->report_format = \
				(SendMessage(GetDlgItem(hReportDlg,IDC_UTF16),BM_GETCHECK,0,0) == \
				BST_CHECKED) ? UTF_FORMAT : ASCII_FORMAT;
			settings->report_type = \
				(SendMessage(GetDlgItem(hReportDlg,IDC_HTML),BM_GETCHECK,0,0) == \
				BST_CHECKED) ? HTML_REPORT : NO_REPORT;
			settings->dbgprint_level = \
				(SendMessage(GetDlgItem(hReportDlg,IDC_DBG_NORMAL),BM_GETCHECK,0,0) == \
				BST_CHECKED) ? 0 : 1;
			if(SendMessage(GetDlgItem(hReportDlg,IDC_DBG_PARANOID),BM_GETCHECK,0,0) == \
				BST_CHECKED)
				settings->dbgprint_level = 2;
			//settings->show_progress =
			//	(SendMessage(GetDlgItem(hGuiDlg,IDC_SHOWPROGRESS),BM_GETCHECK,0,0) ==
			//	BST_CHECKED) ? TRUE : FALSE;
			//if(!settings->show_progress)
			//	HideProgress();
			GetWindowTextW(GetDlgItem(hBootSchedDlg,IDC_LETTERS),settings->sched_letters,40);
			settings->next_boot = \
				(SendMessage(GetDlgItem(hBootSchedDlg,IDC_NEXTBOOT),BM_GETCHECK,0,0) == \
				BST_CHECKED) ? TRUE : FALSE;
			settings->every_boot = \
				(SendMessage(GetDlgItem(hBootSchedDlg,IDC_EVERYBOOT),BM_GETCHECK,0,0) == \
				BST_CHECKED) ? TRUE : FALSE;
			settings->only_reg_and_pagefile = \
				(SendMessage(GetDlgItem(hBootSchedDlg,IDC_REGANDPAGEFILE),BM_GETCHECK,0,0) == \
				BST_CHECKED) ? TRUE : FALSE;
			GetWindowTextW(GetDlgItem(hFilterDlg,IDC_INCLUDE),in_filter,MAX_FILTER_SIZE);
			settings->in_filter = in_filter;
			GetWindowTextW(GetDlgItem(hFilterDlg,IDC_EXCLUDE),ex_filter,MAX_FILTER_SIZE);
			settings->ex_filter = ex_filter;
			GetWindowTextW(GetDlgItem(hBootSchedDlg,IDC_INCLUDE2),boot_in_filter,MAX_FILTER_SIZE);
			settings->boot_in_filter = boot_in_filter;
			GetWindowTextW(GetDlgItem(hBootSchedDlg,IDC_EXCLUDE2),boot_ex_filter,MAX_FILTER_SIZE);
			settings->boot_ex_filter = boot_ex_filter;
			udefrag_set_options(settings);
			EndDialog(hWnd,1);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWnd,0);
			return TRUE;
		case IDC_SETTINGS_HELP:
			ShellExecute(hWindow,"open",".\\doc\\manual.html",NULL,NULL,SW_SHOW);
			break;
		}
		return FALSE;
	case WM_CLOSE:
		EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

void LoadFilter(HWND hWnd,HWND hIncl,HWND hExcl,BOOL is_boot_time_filter)
{
	OPENFILENAMEW ofn;
	short szFileTitle[MAX_PATH + 2];
	short szFilter[256] = L"All Files\0*.*\0";
	FILE *pf;
	short filename[MAX_PATH + 2] = L"";
	short buffer[2048 + 8];
	int len;

	memset(&ofn,0,sizeof(OPENFILENAMEW));
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrInitialDir = L".\\presets";
	ofn.Flags = OFN_FILEMUSTEXIST;
	if(GetOpenFileNameW(&ofn))
	{
		pf = _wfopen(ofn.lpstrFile,L"rb");
		if(!pf) MessageBox(hWnd,"Can't open file!","Error!",MB_OK | MB_ICONHAND);
		else
		{
			fread(buffer,sizeof(short),2048,pf);
			buffer[2048] = buffer[2049] = 0;
			SetWindowTextW(hIncl,buffer);
			len = wcslen(buffer) + 1;
			SetWindowTextW(hExcl,buffer + len);
			fclose(pf);
		}
	}
}

void SaveFilter(HWND hWnd,HWND hIncl,HWND hExcl)
{
	OPENFILENAMEW ofn;
	short szFileTitle[MAX_PATH + 2];
	short szFilter[256] = L"All Files\0*.*\0";
	FILE *pf;
	short filename[MAX_PATH + 2] = L"";
	short buffer[1024 + 4];

	memset(&ofn,0,sizeof(OPENFILENAMEW));
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrInitialDir = L".\\presets";
	ofn.Flags = OFN_PATHMUSTEXIST;
	if(GetSaveFileNameW(&ofn))
	{
		pf = _wfopen(ofn.lpstrFile,L"wb");
		if(!pf) MessageBox(hWnd,"Can't open file!","Error!",MB_OK | MB_ICONHAND);
		else
		{
			GetWindowTextW(hIncl,buffer,1024);
			fwrite(buffer,sizeof(short),wcslen(buffer) + 1,pf);
			GetWindowTextW(hExcl,buffer,1024);
			fwrite(buffer,sizeof(short),wcslen(buffer) + 1,pf);
			fclose(pf);
		}
	}
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
		case IDC_LOAD1:
			LoadFilter(hWnd,GetDlgItem(hFilterDlg,IDC_INCLUDE),
				GetDlgItem(hFilterDlg,IDC_EXCLUDE),FALSE);
			break;
		case IDC_SAVE1:
			SaveFilter(hWnd,GetDlgItem(hFilterDlg,IDC_INCLUDE),
				GetDlgItem(hFilterDlg,IDC_EXCLUDE));
			break;
		}
		return FALSE;
	case WM_CLOSE:
		EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

BOOL CALLBACK BootSchedDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_LOAD2:
			LoadFilter(hWnd,GetDlgItem(hBootSchedDlg,IDC_INCLUDE2),
				GetDlgItem(hBootSchedDlg,IDC_EXCLUDE2),FALSE);
			break;
		case IDC_SAVE2:
			SaveFilter(hWnd,GetDlgItem(hBootSchedDlg,IDC_INCLUDE2),
				GetDlgItem(hBootSchedDlg,IDC_EXCLUDE2));
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
