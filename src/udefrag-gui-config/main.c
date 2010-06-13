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

/* Global variables */
HINSTANCE hInstance;
HWND hWindow;
extern WGX_I18N_RESOURCE_ENTRY i18n_table[];

LOGFONT lf;
HFONT hFont = NULL;

/* Function prototypes */
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void OpenWebPage(char *page);
BOOL GetBootExecuteRegistrationStatus(void);
void InitFont(void);
void SaveFontSettings(void);

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
	hInstance = GetModuleHandle(NULL);
	/*
	* This call needs on dmitriar's pc (on xp) no more than 550 cpu tacts,
	* but InitCommonControlsEx() needs about 90000 tacts.
	* Because the first function is just a stub on xp.
	*/
	InitCommonControls();
	if(DialogBox(hInstance, MAKEINTRESOURCE(IDD_CONFIG),NULL,(DLGPROC)DlgProc) == (-1)){
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
	short path[MAX_PATH];
	LRESULT check_state;
	CHOOSEFONT cf;
	HFONT hNewFont;

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
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_FONT:
			memset(&cf,0,sizeof(cf)); /* FIXME: may fail on x64? */
			cf.lStructSize = sizeof(CHOOSEFONT);
			cf.lpLogFont = &lf;
			cf.Flags = CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;
			cf.hwndOwner = hWindow;
			if(ChooseFont(&cf)){
				hNewFont = WgxSetFont(hWindow,&lf);
				if(hNewFont){
					if(hFont) (void)DeleteObject(hFont);
					hFont = hNewFont;
					SaveFontSettings();
				}
			}
			break;
		case IDC_GUI_SCRIPT:
			#ifndef UDEFRAG_PORTABLE
			(void)WgxShellExecuteW(hWindow,L"open",L".\\options\\guiopts.lua",NULL,NULL,SW_SHOW);
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
				DisplayLastError("Cannot retrieve the Windows directory path!");
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
				DisplayLastError("Cannot retrieve the Windows directory path!");
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
			(void)WgxShellExecuteW(hWindow,L"open",L".\\options\\udreportopts.lua",NULL,NULL,SW_SHOW);
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
		DisplayLastError("Cannot open SMSS key!");
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
		DisplayLastError("Cannot query BootExecute value!");
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

/* returns 0 if variable is not defined */
static int getint(lua_State *L, char *variable)
{
	int ret;
	
	lua_getglobal(L, variable);
	ret = (int)lua_tointeger(L, lua_gettop(L));
	lua_pop(L, 1);
	return ret;
}

void InitFont(void)
{
	lua_State *L;
	int status;
	HFONT hNewFont;
	int x,y,width,height;
	short *cmdline;
	RECT rc;

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

	/* set main window position if requested */
	cmdline = GetCommandLineW();
	if(cmdline == NULL) return;

	if(wcsstr(cmdline,L"CalledByGUI")){
		L = lua_open();  /* create state */
		if(!L) return;
		lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
		luaL_openlibs(L);  /* open libraries */
		lua_gc(L, LUA_GCRESTART, 0);
		status = luaL_dofile(L,".\\options\\guiopts.lua");
		if(!status){ /* successful */
			x = getint(L,"x");
			y = getint(L,"y");
			width = getint(L,"width");
			height = getint(L,"height");
			if(GetWindowRect(hWindow,&rc)){
				if(width < (rc.right - rc.left) || height < (rc.bottom - rc.top))
					(void)SetWindowPos(hWindow,0,x + 50,y + 85,0,0,SWP_NOSIZE);
				else
					(void)SetWindowPos(hWindow,0,
						x + (width - (rc.right - rc.left)) / 2,
						y + (height - (rc.bottom - rc.top)) / 2,
						0,0,SWP_NOSIZE
					);
			}
		}
		lua_close(L);
	}
}

void SaveFontSettings(void)
{
	(void)WgxSaveLogFontStructureToFile(".\\options\\font.lua",&lf);
}
