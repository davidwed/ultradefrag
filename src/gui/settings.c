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
 *  GUI - load/save/change program settings.
 */

#include <windows.h>
#include <winreg.h>
#include <winioctl.h>
#include <commctrl.h>
#include <commdlg.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "../include/ultradfg.h"

#include "resource.h"

extern HINSTANCE hInstance;
extern HANDLE hEvt;
extern HANDLE hUltraDefragDevice;
RECT win_rc; /* coordinates of main window */

short *in_filter = NULL, *ex_filter = NULL;
BOOL in_edit_flag,ex_edit_flag;
ULONGLONG sizelimit = 0;

DWORD skip_rem = 1;	/* check box state */
int update_interval = 500;
BOOL show_progress = TRUE;

UCHAR report_type = HTML_REPORT;
UCHAR report_format = ASCII_FORMAT;
DWORD dbgprint_level = 0;

char sched_letters[64] = "";
DWORD every_boot = FALSE;
DWORD next_boot = FALSE;
DWORD only_reg_and_pagefile = FALSE;

short *boot_in_filter = NULL, *boot_ex_filter = NULL;

HWND hTabCtrl;
HWND hFilterDlg,hGuiDlg,hReportDlg,hBootSchedDlg;

BOOL CALLBACK FilterDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK BootSchedDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK EmptyDlgProc(HWND, UINT, WPARAM, LPARAM);

extern void HideProgress();

/* registry paths and value names */
char UDEFRAG_REG_HOME[] = "Software\\DASoft\\NTDefrag";
char POSITION[] = "position";
char SKIP_REM[] = "skip removable";
char MAP_UPDATE_INT[] = "update interval";
char SHOW_PROGRESS[] = "show progress";
char SIZELIMIT[] = "sizelimit";
char REPORT_FMT[] = "report format";
short INCL_FILTER[] = L"include filter";
short EXCL_FILTER[] = L"exclude filter";
short BOOT_INCL_FILTER[] = L"boot time include filter";
short BOOT_EXCL_FILTER[] = L"boot time exclude filter";
char DBGPRINT_LEVEL[] = "dbgprint level";
char REPORTTYPE[] = "report type";
char SCHED_LETTERS[] = "scheduled letters";
char NEXT_BOOT[] = "next boot";
char EVERY_BOOT[] = "every boot";
char ONLY_REG_AND_PAGEFILE[] = "only registry and pagefile";

void LoadSettings()
{
	DWORD txd;
	OVERLAPPED ovrl;
	REPORT_TYPE rt;
	HKEY hKey;
	DWORD _len,length;

	if(RegOpenKeyEx(HKEY_CURRENT_USER,UDEFRAG_REG_HOME,0,KEY_QUERY_VALUE,&hKey) == \
		ERROR_SUCCESS)
	{
		_len = sizeof(RECT);
		RegQueryValueEx(hKey,POSITION,NULL,NULL,(BYTE*)&win_rc,&_len);
		_len = sizeof(DWORD);
		RegQueryValueEx(hKey,SKIP_REM,NULL,NULL,(BYTE*)&skip_rem,&_len);
		_len = sizeof(DWORD);
		RegQueryValueEx(hKey,MAP_UPDATE_INT,NULL,NULL,
			(unsigned char*)&update_interval,&_len);
		_len = sizeof(BOOL);
		RegQueryValueEx(hKey,SHOW_PROGRESS,NULL,NULL,(BYTE*)&show_progress,&_len);
		_len = sizeof(ULONGLONG);
		RegQueryValueEx(hKey,SIZELIMIT,NULL,NULL,(BYTE*)&sizelimit,&_len);
		_len = sizeof(UCHAR);
		RegQueryValueEx(hKey,REPORT_FMT,NULL,NULL,(BYTE*)&report_format,&_len);
		length = 0;
		if(RegQueryValueExW(hKey,INCL_FILTER,NULL,NULL,NULL,&_len) ==
			ERROR_SUCCESS)
		{
			in_filter = malloc(_len);
			if(in_filter)
			{
				RegQueryValueExW(hKey,INCL_FILTER,NULL,NULL,(BYTE*)in_filter,&_len);
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
		if(RegQueryValueExW(hKey,EXCL_FILTER,NULL,NULL,NULL,&_len) ==
			ERROR_SUCCESS)
		{
			ex_filter = malloc(_len);
			if(ex_filter)
			{
				RegQueryValueExW(hKey,EXCL_FILTER,NULL,NULL,(BYTE*)ex_filter,&_len);
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
		RegQueryValueEx(hKey,DBGPRINT_LEVEL,NULL,NULL,
			(unsigned char*)&dbgprint_level,&_len);
		ovrl.Offset = ovrl.OffsetHigh = 0;
		if(DeviceIoControl(hUltraDefragDevice,IOCTL_SET_DBGPRINT_LEVEL,
			&dbgprint_level,sizeof(DWORD),NULL,0,&txd,&ovrl))
		{
			WaitForSingleObject(hEvt,INFINITE);
		}
		_len = sizeof(UCHAR);
		RegQueryValueEx(hKey,REPORTTYPE,NULL,NULL,
			(unsigned char*)&report_type,&_len);
		rt.format = report_format;
		rt.type = report_type;
		ovrl.Offset = ovrl.OffsetHigh = 0;
		if(DeviceIoControl(hUltraDefragDevice,IOCTL_SET_REPORT_TYPE,
			&rt,sizeof(REPORT_TYPE),NULL,0,&txd,&ovrl))
		{
			WaitForSingleObject(hEvt,INFINITE);
		}
		_len = 40;
		RegQueryValueExA(hKey,SCHED_LETTERS,NULL,NULL,(BYTE*)sched_letters,&_len);
		_len = sizeof(DWORD);
		RegQueryValueEx(hKey,NEXT_BOOT,NULL,NULL,
			(unsigned char*)&next_boot,&_len);
		_len = sizeof(DWORD);
		RegQueryValueEx(hKey,EVERY_BOOT,NULL,NULL,
			(unsigned char*)&every_boot,&_len);
		if(RegQueryValueExW(hKey,BOOT_INCL_FILTER,NULL,NULL,NULL,&_len) ==
			ERROR_SUCCESS)
		{
			boot_in_filter = malloc(_len);
			if(boot_in_filter)
				RegQueryValueExW(hKey,BOOT_INCL_FILTER,NULL,NULL,(BYTE*)boot_in_filter,&_len);
		}
		if(RegQueryValueExW(hKey,BOOT_EXCL_FILTER,NULL,NULL,NULL,&_len) ==
			ERROR_SUCCESS)
		{
			boot_ex_filter = malloc(_len);
			if(boot_ex_filter)
				RegQueryValueExW(hKey,BOOT_EXCL_FILTER,NULL,NULL,(BYTE*)boot_ex_filter,&_len);
		}
		_len = sizeof(DWORD);
		RegQueryValueEx(hKey,ONLY_REG_AND_PAGEFILE,NULL,NULL,
			(unsigned char*)&only_reg_and_pagefile,&_len);
		RegCloseKey(hKey);
	}
}

void SaveSettings(HKEY hRootKey)
{
	HKEY hKey, hKey1, hKey2;

	if(RegOpenKeyEx(hRootKey,UDEFRAG_REG_HOME,0,KEY_SET_VALUE,&hKey) != \
		ERROR_SUCCESS)
	{
		RegCreateKeyEx(hRootKey,"Software",0,NULL,0,KEY_CREATE_SUB_KEY,NULL,&hKey1,NULL);
		RegCreateKeyEx(hKey1,"DASoft",0,NULL,0,KEY_CREATE_SUB_KEY,NULL,&hKey2,NULL);
		if(RegCreateKeyEx(hKey2,"NTDefrag",0,NULL,0,KEY_SET_VALUE,NULL,&hKey,NULL) != \
			ERROR_SUCCESS)
		{
			RegCloseKey(hKey1); RegCloseKey(hKey2);
			return;
		}
		RegCloseKey(hKey1); RegCloseKey(hKey2);
	}
	RegSetValueEx(hKey,POSITION,0,REG_BINARY,(BYTE*)&win_rc,sizeof(RECT));
	RegSetValueEx(hKey,SKIP_REM,0,REG_DWORD,(BYTE*)&skip_rem,sizeof(DWORD));
	RegSetValueEx(hKey,MAP_UPDATE_INT,0,REG_DWORD,
		(BYTE*)&update_interval,sizeof(DWORD));
	RegSetValueEx(hKey,SHOW_PROGRESS,0,REG_BINARY,(BYTE*)&show_progress,sizeof(BOOL));
	RegSetValueEx(hKey,SIZELIMIT,0,REG_BINARY,(BYTE*)&sizelimit,sizeof(ULONGLONG));
	if(in_filter)
		RegSetValueExW(hKey,INCL_FILTER,0,REG_SZ,
					(BYTE*)in_filter,(wcslen(in_filter) + 1) << 1);
	else
		RegDeleteValueW(hKey,INCL_FILTER);
	if(ex_filter)
		RegSetValueExW(hKey,EXCL_FILTER,0,REG_SZ,
					(BYTE*)ex_filter,(wcslen(ex_filter) + 1) << 1);
	else
		RegDeleteValueW(hKey,EXCL_FILTER);
	RegSetValueEx(hKey,REPORT_FMT,0,REG_BINARY,(BYTE*)&report_format,sizeof(UCHAR));
	RegSetValueEx(hKey,REPORTTYPE,0,REG_BINARY,(BYTE*)&report_type,sizeof(UCHAR));
	RegSetValueEx(hKey,DBGPRINT_LEVEL,0,REG_DWORD,(BYTE*)&dbgprint_level,sizeof(DWORD));
	RegSetValueExA(hKey,SCHED_LETTERS,0,REG_SZ,
		(BYTE*)sched_letters,strlen(sched_letters) + 1);
	RegSetValueEx(hKey,NEXT_BOOT,0,REG_DWORD,(BYTE*)&next_boot,sizeof(DWORD));
	RegSetValueEx(hKey,EVERY_BOOT,0,REG_DWORD,(BYTE*)&every_boot,sizeof(DWORD));
	if(boot_in_filter)
		RegSetValueExW(hKey,BOOT_INCL_FILTER,0,REG_SZ,
					(BYTE*)boot_in_filter,(wcslen(boot_in_filter) + 1) << 1);
	else
		RegDeleteValueW(hKey,BOOT_INCL_FILTER);
	if(boot_ex_filter)
		RegSetValueExW(hKey,BOOT_EXCL_FILTER,0,REG_SZ,
					(BYTE*)boot_ex_filter,(wcslen(boot_ex_filter) + 1) << 1);
	else
		RegDeleteValueW(hKey,BOOT_EXCL_FILTER);
	RegSetValueEx(hKey,ONLY_REG_AND_PAGEFILE,0,REG_DWORD,(BYTE*)&only_reg_and_pagefile,sizeof(DWORD));
	RegCloseKey(hKey);
}

void SaveBootTimeSettings()
{
	DWORD _len,len2,length,type;
	HKEY hKey;
	char boot_exec[512] = "";
	char new_boot_exec[512] = "";

	if(next_boot || every_boot)
	{
		/* add native program name to BootExecute registry parameter */
		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\Session Manager",
			0,KEY_QUERY_VALUE | KEY_SET_VALUE,&hKey) == \
			ERROR_SUCCESS)
		{
			_len = 510;
			type = REG_MULTI_SZ;
			RegQueryValueEx(hKey,"BootExecute",NULL,&type,boot_exec,&_len);
			for(length = 0;length < _len - 1;)
			{
				if(!strcmp(boot_exec + length,"defrag_native"))
					goto already_existed;
				length += strlen(boot_exec + length) + 1;
			}
			strcpy(boot_exec + _len - 1, "defrag_native");
			_len += strlen("defrag_native") + 1;
			boot_exec[_len - 1] = 0;
			RegSetValueEx(hKey,"BootExecute",0,REG_MULTI_SZ,boot_exec,_len);
already_existed: {}
		}
	}
	else
	{
		/* remove native program name from BootExecute registry parameter */
		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\Session Manager",
			0,KEY_QUERY_VALUE | KEY_SET_VALUE,&hKey) == \
			ERROR_SUCCESS)
		{
			_len = 510;
			type = REG_MULTI_SZ;
			RegQueryValueEx(hKey,"BootExecute",NULL,&type,boot_exec,&_len);
			len2 = 0;
			for(length = 0;length < _len - 1;)
			{
				if(strcmp(boot_exec + length,"defrag_native"))
				{ /* if strings are not equal */
					strcpy(new_boot_exec + len2,boot_exec + length);
					len2 += strlen(boot_exec + length) + 1;
				}
				length += strlen(boot_exec + length) + 1;
			}
			new_boot_exec[len2] = 0;
			RegSetValueEx(hKey,"BootExecute",0,REG_MULTI_SZ,new_boot_exec,len2 + 1);
		}
	}
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

void UpdateBootTimeFilter(HWND hWnd,short **buffer)
{
	int length;

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
	int check_id;

	switch(msg)
	{
	case WM_INITDIALOG:
		/* Window Initialization */
		GetWindowRect(hWnd,&_rc);
		dx = _rc.right - _rc.left;
		dy = _rc.bottom - _rc.top;
		SetWindowPos(hWnd,0,win_rc.left + 70,win_rc.top + 55,dx,dy,0);
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
		switch(dbgprint_level)
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
		if(show_progress)
			SendMessage(GetDlgItem(hGuiDlg,IDC_SHOWPROGRESS),BM_SETCHECK,BST_CHECKED,0);
		SetWindowText(GetDlgItem(hBootSchedDlg,IDC_LETTERS),sched_letters);
		if(next_boot)
			SendMessage(GetDlgItem(hBootSchedDlg,IDC_NEXTBOOT),BM_SETCHECK,BST_CHECKED,0);
		if(every_boot)
			SendMessage(GetDlgItem(hBootSchedDlg,IDC_EVERYBOOT),BM_SETCHECK,BST_CHECKED,0);
		if(only_reg_and_pagefile)
			SendMessage(GetDlgItem(hBootSchedDlg,IDC_REGANDPAGEFILE),BM_SETCHECK,BST_CHECKED,0);
		if(boot_in_filter)
			SetWindowTextW(GetDlgItem(hBootSchedDlg,IDC_INCLUDE2),boot_in_filter);
		if(boot_ex_filter)
			SetWindowTextW(GetDlgItem(hBootSchedDlg,IDC_EXCLUDE2),boot_ex_filter);
		in_edit_flag = ex_edit_flag = FALSE;
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
			if(SendMessage(GetDlgItem(hReportDlg,IDC_DBG_PARANOID),BM_GETCHECK,0,0) == \
				BST_CHECKED)
				new_dbgprint_level = 2;
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
			GetWindowText(GetDlgItem(hBootSchedDlg,IDC_LETTERS),sched_letters,40);
			next_boot = \
				(SendMessage(GetDlgItem(hBootSchedDlg,IDC_NEXTBOOT),BM_GETCHECK,0,0) == \
				BST_CHECKED) ? TRUE : FALSE;
			every_boot = \
				(SendMessage(GetDlgItem(hBootSchedDlg,IDC_EVERYBOOT),BM_GETCHECK,0,0) == \
				BST_CHECKED) ? TRUE : FALSE;
			only_reg_and_pagefile = \
				(SendMessage(GetDlgItem(hBootSchedDlg,IDC_REGANDPAGEFILE),BM_GETCHECK,0,0) == \
				BST_CHECKED) ? TRUE : FALSE;
			UpdateBootTimeFilter(GetDlgItem(hBootSchedDlg,IDC_INCLUDE2),&boot_in_filter);
			UpdateBootTimeFilter(GetDlgItem(hBootSchedDlg,IDC_EXCLUDE2),&boot_ex_filter);
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

void LoadFilter(HWND hWnd,HWND hIncl,HWND hExcl,BOOL is_boot_time_filter)
{
	OPENFILENAME ofn;
	char szFileTitle[MAX_PATH + 2];
	char szFilter[256] = "All Files\0*.*\0";
	FILE *pf;
	char filename[MAX_PATH + 2] = "";
	short buffer[1024 + 4];
	short CRLF[] = L"\r\n";

	memset(&ofn,0,sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrInitialDir = ".\\presets";
	ofn.Flags = OFN_FILEMUSTEXIST;
	if(GetOpenFileName(&ofn))
	{
		pf = fopen(ofn.lpstrFile,"rt");
		if(!pf) MessageBox(hWnd,"Can't open file!","Error!",MB_OK | MB_ICONHAND);
		else
		{
			fgetws(buffer,1024,pf);
			if(wcslen(buffer) >= wcslen(CRLF))
				buffer[wcslen(buffer) - wcslen(CRLF)] = 0;
			else
				buffer[0] = 0;
			SetWindowTextW(hIncl,buffer);
			fgetws(buffer,1024,pf);
			if(wcslen(buffer) >= wcslen(CRLF))
				buffer[wcslen(buffer) - wcslen(CRLF)] = 0;
			else
				buffer[0] = 0;
			SetWindowTextW(hExcl,buffer);
			if(!is_boot_time_filter)
			{
				in_edit_flag = TRUE;
				ex_edit_flag = TRUE;
			}
			fclose(pf);
		}
	}
}

void SaveFilter(HWND hWnd,HWND hIncl,HWND hExcl)
{
	OPENFILENAME ofn;
	char szFileTitle[MAX_PATH + 2];
	char szFilter[256] = "All Files\0*.*\0";
	FILE *pf;
	char filename[MAX_PATH + 2] = "";
	short buffer[1024 + 4];
	short CRLF[] = L"\r\n";

	memset(&ofn,0,sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrInitialDir = ".\\presets";
	ofn.Flags = OFN_PATHMUSTEXIST;
	if(GetSaveFileName(&ofn))
	{
		pf = fopen(ofn.lpstrFile,"wt");
		if(!pf) MessageBox(hWnd,"Can't open file!","Error!",MB_OK | MB_ICONHAND);
		else
		{
			GetWindowTextW(hIncl,buffer,1024);
			fputws(buffer,pf); fputws(CRLF,pf);
			GetWindowTextW(hExcl,buffer,1024);
			fputws(buffer,pf); fputws(CRLF,pf);
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
		case IDC_INCLUDE:
			if(HIWORD(wParam) == EN_CHANGE)
				in_edit_flag = TRUE;
			break;
		case IDC_EXCLUDE:
			if(HIWORD(wParam) == EN_CHANGE)
				ex_edit_flag = TRUE;
			break;
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

void DestroyFilters()
{
	if(in_filter) free((void *)in_filter);
	if(ex_filter) free((void *)ex_filter);
}
