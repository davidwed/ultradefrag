/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
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
* UltraDefrag Next Generation code.
*/

#define WIN32_NO_STATUS
#include <windows.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>

#include "../include/ultradfgver.h"
#include "resource.h"

BOOL small_window = FALSE;

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*---------------------------------- Main Function ------------------------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSEX wcx;
	HWND hWindow;
	HBITMAP hBitmap;
	HBRUSH hBrush;
	MSG	msg;
	int screen_width, screen_height;

	screen_width = GetSystemMetrics(SM_CXSCREEN);
	screen_height = GetSystemMetrics(SM_CYSCREEN);
	if(screen_width < 602 || screen_height < 780) small_window = TRUE;
	if(screen_width < 301 || screen_height < 390){
		MessageBox(NULL,"Screen resolution is too low!\nMust be at least 640x480.","Error!",MB_OK | MB_ICONHAND);
		return 2;
	}
	
	if(strstr(lpCmdLine,"--small-window")) small_window = TRUE;

	memset(&wcx,0,sizeof(WNDCLASSEX));
	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.lpfnWndProc = WndProc;
	wcx.hInstance = hInst;
	
	if(small_window == FALSE) hBitmap = LoadBitmap(hInst,MAKEINTRESOURCE(IDB_BACKGROUND));
	else hBitmap = LoadBitmap(hInst,MAKEINTRESOURCE(IDB_BACKGROUND_SMALL));

	if(!hBitmap){
		MessageBox(NULL,"Cannot load background bitmap!","Error!",MB_OK | MB_ICONHAND);
		return 1;
	}
	hBrush = CreatePatternBrush(hBitmap);
	if(!hBrush){
		MessageBox(NULL,"CreatePatternBrush() failed!","Error!",MB_OK | MB_ICONHAND);
		return 1;
	}
	
	wcx.hbrBackground = hBrush;
	wcx.hCursor = LoadCursor(NULL,IDC_ARROW);
	wcx.hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_APP));
	wcx.lpszClassName = "UltraDefragNG";
	if(!RegisterClassEx(&wcx)){
		MessageBox(NULL,"RegisterClassEx() failed!","Error!",MB_OK | MB_ICONHAND);
		return 1;
	}
	
	if(small_window == FALSE){
		hWindow = CreateWindowEx(0,"UltraDefragNG",
			NGVERSIONINTITLE,
			WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX, 
			(screen_width - 602) >> 1,(screen_height - 780) >> 1,602,780,
			NULL,0,hInst,0);
	} else {
		hWindow = CreateWindowEx(0,"UltraDefragNG",
			NGVERSIONINTITLE,
			WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX, 
			(screen_width - 301) >> 1,(screen_height - 390) >> 1,301,390,
			NULL,0,hInst,0);
	}
	if(!hWindow){
		MessageBox(NULL,"CreateWindowEx() failed!","Error!",MB_OK | MB_ICONHAND);
		return 1;
	}

	ShowWindow(hWindow,SW_SHOWNORMAL);
	UpdateWindow(hWindow);
	while(GetMessage(&msg,NULL,0,0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

HFONT __stdcall WgxSetFont(HWND hWindow,LPLOGFONT lplf)
{
	HFONT hFont;
	HWND hChild;
	
	hFont = CreateFontIndirect(lplf);
	if(!hFont) return NULL;
	
	SendMessage(hWindow,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
	hChild = GetWindow(hWindow,GW_CHILD);
	while(hChild){
		SendMessage(hChild,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
		hChild = GetWindow(hChild,GW_HWNDNEXT);
	}

	/* redraw the main window */
	InvalidateRect(hWindow,NULL,TRUE);
	UpdateWindow(hWindow);
	
	return hFont;
}

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HINSTANCE hInst;
	HRSRC hRegion;
	HGLOBAL hRegionData;
	DWORD RegionSize;
	static HWND hBtnScenario;
	static HWND hBtnStart;
	static HWND hBtnExit;
	LOGFONT lf;
	HINSTANCE hApp;
	
	switch(msg){
	case WM_CREATE:
		hInst = GetModuleHandle(NULL);
		
		if(small_window == FALSE) hRegion = FindResource(hInst,"RANGE","RGN");
		else hRegion = FindResource(hInst,"RANGE_SMALL","RGN");
		
		if(!hRegion){
			MessageBox(NULL,"Cannot find region resource!","Error!",MB_OK | MB_ICONHAND);
			return 1;
		}
		
		hRegionData = LoadResource(hInst,hRegion);
		if(!hRegionData){
			MessageBox(NULL,"Cannot load region resource!","Error!",MB_OK | MB_ICONHAND);
			return 1;
		}

		RegionSize = SizeofResource(hInst,hRegion);
		SetWindowRgn(hwnd,ExtCreateRegion(NULL,RegionSize,LockResource(hRegionData)),TRUE);
		
		if(small_window == FALSE){
			hBtnScenario = CreateWindowEx(0,"button","Scenario",
					WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
					29,730,70,25,hwnd,(HMENU)IDC_SCENARIO,hInst,NULL);
			hBtnStart = CreateWindowEx(0,"button","Start",
					WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
					104,730,70,25,hwnd,(HMENU)IDC_START,hInst,NULL);
			hBtnExit = CreateWindowEx(0,"button","Exit",
					WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
					179,730,70,25,hwnd,(HMENU)IDC_EXIT,hInst,NULL);
		} else {
			hBtnScenario = CreateWindowEx(0,"button","Scenario",
					WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
					14,350,70,25,hwnd,(HMENU)IDC_SCENARIO,hInst,NULL);
			hBtnStart = CreateWindowEx(0,"button","Start",
					WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
					89,350,70,25,hwnd,(HMENU)IDC_START,hInst,NULL);
			hBtnExit = CreateWindowEx(0,"button","Exit",
					WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
					164,350,70,25,hwnd,(HMENU)IDC_EXIT,hInst,NULL);
		}

		//SendMessage(hBtnScenario,WM_SETFOCUS,0,0);
		memset(&lf,0,sizeof(LOGFONT));
		/* default font should be Courier New 9pt */
		strcpy(lf.lfFaceName,"Courier New");
		lf.lfHeight = -12;
		WgxSetFont(hwnd,&lf);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_SCENARIO:
			hApp = ShellExecute(hwnd,"edit","UltraDefragNG_Scenario.cmd",NULL,NULL,SW_SHOW);
			if((int)(LONG_PTR)hApp <= 32){
				MessageBox(hwnd,"UltraDefragNG_Scenario.cmd file not found \nin the current directory!",
					"Error!",MB_OK | MB_ICONHAND);
			}
			break;
		case IDC_START:
			hApp = ShellExecute(hwnd,"open","UltraDefragNG_Scenario.cmd",NULL,NULL,SW_SHOW);
			if((int)(LONG_PTR)hApp <= 32){
				MessageBox(hwnd,"UltraDefragNG_Scenario.cmd loading failed \n(not found in the current directory?)!",
					"Error!",MB_OK | MB_ICONHAND);
			}
			break;
		case IDC_EXIT:
			PostQuitMessage(0);
			return 0;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
/*	case WM_KEYDOWN:
		if(wParam == VK_TAB){
			SendMessage(hBtnStart,WM_SETFOCUS,0,0);
		}
		break;
*/	case WM_LBUTTONDOWN:
		/* make our window think that the user moves the caption bar */
		SendMessage(hwnd,WM_NCLBUTTONDOWN,HTCAPTION,lParam);
        break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
