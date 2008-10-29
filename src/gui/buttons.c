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
* GUI - buttons related stuff.
*/

#include "main.h"

extern HWND hWindow;
extern int skip_removable;

HWND hBtnAnalyse,hBtnDfrg,hBtnPause,hBtnStop,hBtnRescan;
HWND hBtnCompact,hBtnFragm,hBtnSettings;
HWND hCheckSkipRem;

WNDPROC OldBtnWndProc,OldCheckWndProc;

void InitButtons(void)
{
	hBtnAnalyse = GetDlgItem(hWindow,IDC_ANALYSE);
	SetText(hBtnAnalyse,L"ANALYSE");
	hBtnDfrg = GetDlgItem(hWindow,IDC_DEFRAGM);
	SetText(hBtnDfrg,L"DEFRAGMENT");
	hBtnCompact = GetDlgItem(hWindow,IDC_COMPACT);
	SetText(hBtnCompact,L"COMPACT");
	hBtnPause = GetDlgItem(hWindow,IDC_PAUSE);
	SetText(hBtnPause,L"PAUSE");
	hBtnStop = GetDlgItem(hWindow,IDC_STOP);
	SetText(hBtnStop,L"STOP");
	hBtnFragm = GetDlgItem(hWindow,IDC_SHOWFRAGMENTED);
	SetText(hBtnFragm,L"REPORT");
	hBtnRescan = GetDlgItem(hWindow,IDC_RESCAN);
	SetText(hBtnRescan,L"RESCAN_DRIVES");
	hBtnSettings = GetDlgItem(hWindow,IDC_SETTINGS);
	SetText(hBtnSettings,L"SETTINGS");
	hCheckSkipRem = GetDlgItem(hWindow,IDC_SKIPREMOVABLE);
	SetText(hCheckSkipRem,L"SKIP_REMOVABLE_MEDIA");
	if(skip_removable)
		SendMessage(hCheckSkipRem,BM_SETCHECK,BST_CHECKED,0);
	else
		SendMessage(hCheckSkipRem,BM_SETCHECK,BST_UNCHECKED,0);
	SetText(GetDlgItem(hWindow,IDC_ABOUT),L"ABOUT");
	SetText(GetDlgItem(hWindow,IDC_CL_MAP_STATIC),L"CLUSTER_MAP");

	OldBtnWndProc = (WNDPROC)SetWindowLongPtr(hBtnAnalyse,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
	SetWindowLongPtr(hBtnDfrg,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
	SetWindowLongPtr(hBtnCompact,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
	SetWindowLongPtr(hBtnPause,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
	SetWindowLongPtr(hBtnStop,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
	SetWindowLongPtr(hBtnFragm,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
	SetWindowLongPtr(GetDlgItem(hWindow,IDC_SETTINGS),GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
	SetWindowLongPtr(GetDlgItem(hWindow,IDC_ABOUT),GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
	SetWindowLongPtr(hBtnRescan,GWLP_WNDPROC,(LONG_PTR)BtnWndProc);
	OldCheckWndProc = \
		(WNDPROC)SetWindowLongPtr(hCheckSkipRem,GWLP_WNDPROC,(LONG_PTR)CheckWndProc);
}

LRESULT CALLBACK BtnWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	if(iMsg == WM_KEYDOWN)
		HandleShortcuts(hWnd,iMsg,wParam,lParam);
	return CallWindowProc(OldBtnWndProc,hWnd,iMsg,wParam,lParam);
}

LRESULT CALLBACK CheckWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	if(iMsg == WM_KEYDOWN)
		HandleShortcuts(hWnd,iMsg,wParam,lParam);
	return CallWindowProc(OldCheckWndProc,hWnd,iMsg,wParam,lParam);
}

void DisableButtonsBeforeDrivesRescan(void)
{
	EnableWindow(hBtnRescan,FALSE);
	EnableWindow(hBtnAnalyse,FALSE);
	EnableWindow(hBtnDfrg,FALSE);
	EnableWindow(hBtnCompact,FALSE);
	EnableWindow(hBtnFragm,FALSE);
}

void EnableButtonsAfterDrivesRescan(void)
{
	EnableWindow(hBtnRescan,TRUE);
	EnableWindow(hBtnAnalyse,TRUE);
	EnableWindow(hBtnDfrg,TRUE);
	EnableWindow(hBtnCompact,TRUE);
	EnableWindow(hBtnFragm,TRUE);
}

void DisableButtonsBeforeTask(void)
{
	EnableWindow(hBtnAnalyse,FALSE);
	EnableWindow(hBtnDfrg,FALSE);
	EnableWindow(hBtnCompact,FALSE);
	EnableWindow(hBtnFragm,FALSE);
	EnableWindow(hBtnPause,TRUE);
	EnableWindow(hBtnStop,TRUE);
	EnableWindow(hBtnRescan,FALSE);
	//EnableWindow(hBtnSettings,FALSE);
}

void EnableButtonsAfterTask(void)
{
	EnableWindow(hBtnAnalyse,TRUE);
	EnableWindow(hBtnDfrg,TRUE);
	EnableWindow(hBtnCompact,TRUE);
	EnableWindow(hBtnFragm,TRUE);
	EnableWindow(hBtnPause,FALSE);
	EnableWindow(hBtnStop,FALSE);
	EnableWindow(hBtnRescan,TRUE);
	//EnableWindow(hBtnSettings,TRUE);
}
