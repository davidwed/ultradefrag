/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* UltraDefrag Configuration dialog - the main window code.
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
#include "../lua5.1/lua.h"
#include "../lua5.1/lauxlib.h"
#include "../lua5.1/lualib.h"

#include "resource.h"

#include "../dll/wgx/wgx.h"

#include "../include/ultradfgver.h"

/* Global variables */
HINSTANCE hInstance;
HWND hWindow;
extern WGX_I18N_RESOURCE_ENTRY i18n_table[];

HWND hParent = NULL;

WGX_FONT wgxFont = {{0},0};

/* Function prototypes */
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void OpenWebPage(char *page);
BOOL GetBootExecuteRegistrationStatus(void);
BOOL InitFont(void);

/*BOOL CALLBACK EnumWindowsProc(HWND hwnd,LPARAM lParam)
{
	char caption[32];
	
	if(GetWindowText(hwnd,caption,sizeof(caption))){
		if(strstr(caption,VERSIONINTITLE)){
			hParent = hwnd;
			return FALSE;
		}
	}
	return TRUE;
}*/

/*-------------------- Main Function -----------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	hInstance = GetModuleHandle(NULL);
    
	/* FIXME: this method is not reliable */
    if(sscanf(lpCmdLine,"%p",&hParent) != 1){
		/* no need to center window over its parent when there is no parent */
        //EnumWindows(EnumWindowsProc,0);
        //WgxDbgPrint("UltraDefrag GUI Config enumerated window handle as %p\n",hParent);
    } else {
		/* works on 32 bit, needs testing on x64, since pointers are 64-bit there */
        WgxDbgPrint("UltraDefrag GUI Config received window handle as %p\n",hParent);
    }
	
	/*
	* This call needs on dmitriar's pc (on xp) no more than 550 cpu tacts,
	* but InitCommonControlsEx() needs about 90000 tacts.
	* Because the first function is just a stub on xp.
	*/
	InitCommonControls();
	if(DialogBox(hInstance, MAKEINTRESOURCE(IDD_CONFIG),hParent,(DLGPROC)DlgProc) == (-1)){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"Cannot create the main window");
		return 1;
	}
	WgxDestroyFont(&wgxFont);
	WgxDestroyResourceTable(i18n_table);
	return 0;
}

/*---------------- Main Dialog Callback ---------------------*/
BOOL CALLBACK DlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	short path[MAX_PATH];
	LRESULT check_state;
	CHOOSEFONT cf;

	switch(msg){
	case WM_INITDIALOG:
		/* Window Initialization */
		hWindow = hWnd;
		if(WgxBuildResourceTable(i18n_table,L".\\ud_config_i18n.lng"/*path*/))
			WgxApplyResourceTable(i18n_table,hWindow);
		WgxSetIcon(hInstance,hWindow,IDI_CONFIG);
		#ifndef UDEFRAG_PORTABLE
		if(GetBootExecuteRegistrationStatus())
			(void)SendMessage(GetDlgItem(hWindow,IDC_ENABLE),BM_SETCHECK,BST_CHECKED,0);
		else
			(void)SendMessage(GetDlgItem(hWindow,IDC_ENABLE),BM_SETCHECK,BST_UNCHECKED,0);
		#else
		(void)SendMessage(GetDlgItem(hWindow,IDC_ENABLE),BM_SETCHECK,BST_UNCHECKED,0);
		WgxDisableWindows(hWindow,IDC_ENABLE,IDC_BOOT_SCRIPT,0);
		#endif
		InitFont();
		WgxCenterWindow(hWindow);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_FONT:
			memset(&cf,0,sizeof(cf)); /* FIXME: may fail on x64? */
			cf.lStructSize = sizeof(CHOOSEFONT);
			cf.lpLogFont = &wgxFont.lf;
			cf.Flags = CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;
			cf.hwndOwner = hWindow;
			if(ChooseFont(&cf)){
				WgxDestroyFont(&wgxFont);
				if(WgxCreateFont("",&wgxFont)){
					WgxSetFont(hWindow,&wgxFont);
					WgxSaveFont(".\\options\\font.lua",&wgxFont);
				}
			}
			break;
		case IDC_GUI_SCRIPT:
			#ifndef UDEFRAG_PORTABLE
			(void)WgxShellExecuteW(hWindow,L"Edit",L".\\options\\guiopts.lua",NULL,NULL,SW_SHOW);
			#else
			(void)WgxShellExecuteW(hWindow,L"open",L"notepad.exe",L".\\options\\guiopts.lua",NULL,SW_SHOW);
			#endif
			break;
		case IDC_GUI_HELP:
			OpenWebPage("GUI.html");
			break;
		case IDC_ENABLE:
			#ifndef UDEFRAG_PORTABLE
			if(!GetWindowsDirectoryW(path,MAX_PATH)){
				WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot retrieve the Windows directory path");
			} else {
				check_state = SendMessage(GetDlgItem(hWindow,IDC_ENABLE),BM_GETCHECK,0,0);
				if(check_state == BST_CHECKED)
					(void)wcscat(path,L"\\System32\\boot-on.cmd");
				else
					(void)wcscat(path,L"\\System32\\boot-off.cmd");
				(void)WgxShellExecuteW(hWindow,L"open",path,NULL,NULL,SW_HIDE);
			}
			#endif
			break;
		case IDC_BOOT_SCRIPT:
			if(!GetWindowsDirectoryW(path,MAX_PATH)){
				WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot retrieve the Windows directory path");
			} else {
				(void)wcscat(path,L"\\System32\\ud-boot-time.cmd");
				(void)WgxShellExecuteW(hWindow,L"edit",path,NULL,NULL,SW_SHOW);
			}
			break;
		case IDC_BOOT_HELP:
			OpenWebPage("Boot.html");
			break;
		case IDC_REPORT_OPTIONS:
			#ifndef UDEFRAG_PORTABLE
			(void)WgxShellExecuteW(hWindow,L"Edit",L".\\options\\udreportopts.lua",NULL,NULL,SW_SHOW);
			#else
			(void)WgxShellExecuteW(hWindow,L"open",L"notepad.exe",L".\\options\\udreportopts.lua",NULL,SW_SHOW);
			#endif
			break;
		case IDC_REPORT_HELP:
			OpenWebPage("Reports.html");
			break;
		}
		break;
	case WM_CLOSE:
		(void)EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

void OpenWebPage(char *page)
{
	short path[MAX_PATH];
	HINSTANCE hApp;
	
	(void)_snwprintf(path,MAX_PATH,L".\\handbook\\%hs",page);
	path[MAX_PATH - 1] = 0;

	hApp = ShellExecuteW(hWindow,L"open",path,NULL,NULL,SW_SHOW);
	if((int)(LONG_PTR)hApp <= 32){
		(void)_snwprintf(path,MAX_PATH,L"http://ultradefrag.sourceforge.net/handbook/%hs",page);
		path[MAX_PATH - 1] = 0;
		(void)WgxShellExecuteW(hWindow,L"open",path,NULL,NULL,SW_SHOW);
	}
}

BOOL GetBootExecuteRegistrationStatus(void)
{
	HKEY hKey;
	DWORD type, size;
	char *data, *curr_pos;
	DWORD i, length, curr_len;

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\Session Manager",
			0,
			KEY_QUERY_VALUE,
			&hKey) != ERROR_SUCCESS){
		WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot open SMSS key");
		return FALSE;
	}

	type = REG_MULTI_SZ;
	(void)RegQueryValueEx(hKey,"BootExecute",NULL,&type,NULL,&size);
	data = malloc(size + 10);
	if(!data){
		(void)RegCloseKey(hKey);
		MessageBox(0,"Not enough memory for GetBootExecuteRegistrationStatus()!",
			"Error",MB_OK | MB_ICONHAND);
		return FALSE;
	}

	type = REG_MULTI_SZ;
	if(RegQueryValueEx(hKey,"BootExecute",NULL,&type,
			data,&size) != ERROR_SUCCESS){
		WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot query BootExecute value");
		(void)RegCloseKey(hKey);
		free(data);
		return FALSE;
	}

	length = size - 1;
	for(i = 0; i < length;){
		curr_pos = data + i;
		curr_len = strlen(curr_pos) + 1;
		/* if the command is yet registered then exit */
		if(!strcmp(curr_pos,"defrag_native")){
			(void)RegCloseKey(hKey);
			free(data);
			return TRUE;
		}
		i += curr_len;
	}

	(void)RegCloseKey(hKey);
	free(data);
	return FALSE;
}

BOOL InitFont(void)
{
	BOOL result;
	
	memset(&wgxFont.lf,0,sizeof(LOGFONT));
	/* default font should be Courier New 9pt */
	(void)strcpy(wgxFont.lf.lfFaceName,"Courier New");
	wgxFont.lf.lfHeight = -12;
	
	result = WgxCreateFont(".\\options\\font.lua",&wgxFont);
	WgxSetFont(hWindow,&wgxFont);
	return result;
}
