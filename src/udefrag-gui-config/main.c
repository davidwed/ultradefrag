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
* GUI Configurator - main window code.
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

char buffer[MAX_PATH];
char err_msg[1024];

/* Function prototypes */
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void OpenWebPage(char *page);
BOOL GetBootExecuteRegistrationStatus(void);
void InitFont(void);
void SaveFontSettings(void);

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
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_CONFIG),NULL,(DLGPROC)DlgProc);
	if(hFont) DeleteObject(hFont);
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
		GetWindowsDirectoryW(path,MAX_PATH);
		wcscat(path,L"\\UltraDefrag\\ud_config_i18n.lng");
		if(WgxBuildResourceTable(i18n_table,path))
			WgxApplyResourceTable(i18n_table,hWindow);
		WgxSetIcon(hInstance,hWindow,IDI_CONFIG);
		if(GetBootExecuteRegistrationStatus())
			SendMessage(GetDlgItem(hWindow,IDC_ENABLE),BM_SETCHECK,BST_CHECKED,0);
		else
			SendMessage(GetDlgItem(hWindow,IDC_ENABLE),BM_SETCHECK,BST_UNCHECKED,0);
		InitFont();
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_FONT:
			memset(&cf,0,sizeof(cf));
			cf.lStructSize = sizeof(CHOOSEFONT);
			cf.lpLogFont = &lf;
			cf.Flags = CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;
			cf.hwndOwner = hWindow;
			if(ChooseFont(&cf)){
				hNewFont = WgxSetFont(hWindow,&lf);
				if(hNewFont){
					if(hFont) DeleteObject(hFont);
					hFont = hNewFont;
					SaveFontSettings();
				}
			}
			break;
		case IDC_GUI_SCRIPT:
			GetWindowsDirectoryW(path,MAX_PATH);
			wcscat(path,L"\\System32\\udefrag-gui.cmd");
			ShellExecuteW(hWindow,L"edit",path,NULL,NULL,SW_SHOW);
			break;
		case IDC_GUI_HELP:
			OpenWebPage("gui.html");
			break;
		case IDC_ENABLE:
			GetWindowsDirectoryW(path,MAX_PATH);
			check_state = SendMessage(GetDlgItem(hWindow,IDC_ENABLE),BM_GETCHECK,0,0);
			if(check_state == BST_CHECKED)
				wcscat(path,L"\\System32\\boot-on.cmd");
			else
				wcscat(path,L"\\System32\\boot-off.cmd");
			ShellExecuteW(hWindow,L"open",path,NULL,NULL,SW_HIDE);
			break;
		case IDC_BOOT_SCRIPT:
			GetWindowsDirectoryW(path,MAX_PATH);
			wcscat(path,L"\\System32\\ud-boot-time.cmd");
			ShellExecuteW(hWindow,L"edit",path,NULL,NULL,SW_SHOW);
			break;
		case IDC_BOOT_HELP:
			OpenWebPage("boot.html");
			break;
		case IDC_REPORT_OPTIONS:
			GetWindowsDirectoryW(path,MAX_PATH);
			wcscat(path,L"\\UltraDefrag\\options\\udreportopts.lua");
			ShellExecuteW(hWindow,L"edit",path,NULL,NULL,SW_SHOW);
			break;
		case IDC_REPORT_HELP:
			OpenWebPage("reports.html");
			break;
		}
		break;
	case WM_CLOSE:
		EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

void OpenWebPage(char *page)
{
	char path[MAX_PATH];
	HINSTANCE hApp;
	
	GetWindowsDirectory(path,MAX_PATH);
	strcat(path,"\\UltraDefrag\\handbook\\");
	strcat(path,page);
	hApp = ShellExecute(hWindow,"open",path,NULL,NULL,SW_SHOW);
	if((int)(LONG_PTR)hApp <= 32){
		strcpy(path,"http:\\ultradefrag.sourceforge.net\\handbook\\");
		strcat(path,page);
		ShellExecute(hWindow,"open",path,NULL,NULL,SW_SHOW);
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
		MessageBox(0,"Cannot open SMSS key!","Error",MB_OK | MB_ICONHAND);
		return FALSE;
	}

	type = REG_MULTI_SZ;
	RegQueryValueEx(hKey,"BootExecute",NULL,&type,NULL,&size);
	data = malloc(size + 10);
	if(!data){
		RegCloseKey(hKey);
		MessageBox(0,"No enough memory for GetBootExecuteRegistrationStatus()!",
			"Error",MB_OK | MB_ICONHAND);
		return FALSE;
	}

	type = REG_MULTI_SZ;
	if(RegQueryValueEx(hKey,"BootExecute",NULL,&type,
			data,&size) != ERROR_SUCCESS){
		RegCloseKey(hKey);
		free(data);
		MessageBox(0,"Cannot query BootExecute value!","Error",MB_OK | MB_ICONHAND);
		return FALSE;
	}

	length = size - 1;
	for(i = 0; i < length;){
		curr_pos = data + i;
		curr_len = strlen(curr_pos) + 1;
		/* if the command is yet registered then exit */
		if(!strcmp(curr_pos,"defrag_native")){
			RegCloseKey(hKey);
			free(data);
			return TRUE;
		}
		i += curr_len;
	}

	RegCloseKey(hKey);
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
	char *string;
	HFONT hNewFont;
	int x,y,width,height;
	RECT rc;
	short *cmdline;

	/* initialize LOGFONT structure */
	memset(&lf,0,sizeof(LOGFONT));
	/* default font should be Courier New 9pt */
	strcpy(lf.lfFaceName,"Courier New");
	lf.lfHeight = -12;
	
	/* load saved font settings */
	L = lua_open();  /* create state */
	if(!L) return;
	lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
	luaL_openlibs(L);  /* open libraries */
	lua_gc(L, LUA_GCRESTART, 0);

	GetWindowsDirectory(buffer,MAX_PATH);
	strcat(buffer,"\\UltraDefrag\\options\\font.lua");
	status = luaL_dofile(L,buffer);
	if(!status){ /* successful */
		lf.lfHeight = getint(L,"height");
		lf.lfWidth = getint(L,"width");
		lf.lfEscapement = getint(L,"escapement");
		lf.lfOrientation = getint(L,"orientation");
		lf.lfWeight = getint(L,"weight");
		lf.lfItalic = (BYTE)getint(L,"italic");
		lf.lfUnderline = (BYTE)getint(L,"underline");
		lf.lfStrikeOut = (BYTE)getint(L,"strikeout");
		lf.lfCharSet = (BYTE)getint(L,"charset");
		lf.lfOutPrecision = (BYTE)getint(L,"outprecision");
		lf.lfClipPrecision = (BYTE)getint(L,"clipprecision");
		lf.lfQuality = (BYTE)getint(L,"quality");
		lf.lfPitchAndFamily = (BYTE)getint(L,"pitchandfamily");
		lua_getglobal(L, "facename");
		string = (char *)lua_tostring(L, lua_gettop(L));
		if(string){
			strncpy(lf.lfFaceName,string,LF_FACESIZE);
			lf.lfFaceName[LF_FACESIZE - 1] = 0;
		}
		lua_pop(L, 1);
	}

	/* apply font to application's window */
	hNewFont = WgxSetFont(hWindow,&lf);
	if(hNewFont){
		if(hFont) DeleteObject(hFont);
		hFont = hNewFont;
	}

	/* set main window position if requested */
	cmdline = GetCommandLineW();
	if(cmdline){
		if(wcsstr(cmdline,L"CalledByGUI")){
			GetWindowsDirectory(buffer,MAX_PATH);
			strcat(buffer,"\\UltraDefrag\\options\\guiopts.lua");
			status = luaL_dofile(L,buffer);
			if(!status){ /* successful */
				x = getint(L,"x");
				y = getint(L,"y");
				width = getint(L,"width");
				height = getint(L,"height");
				GetWindowRect(hWindow,&rc);
				if(width < (rc.right - rc.left) || height < (rc.bottom - rc.top))
					SetWindowPos(hWindow,0,x + 50,y + 85,0,0,SWP_NOSIZE);
				else
					SetWindowPos(hWindow,0,
						x + (width - (rc.right - rc.left)) / 2,
						y + (height - (rc.bottom - rc.top)) / 2,
						0,0,SWP_NOSIZE
					);
			}
		}
	}

	lua_close(L);
}

void SaveFontSettings(void)
{
	FILE *pf;
	int result;
	
	GetWindowsDirectory(buffer,MAX_PATH);
	strcat(buffer,"\\UltraDefrag\\options\\font.lua");
	pf = fopen(buffer,"wt");
	if(!pf){
		_snprintf(err_msg,sizeof(err_msg) - 1,
			"Can't save font preferences to %s!\n%s",
			buffer,_strerror(NULL));
		err_msg[sizeof(err_msg) - 1] = 0;
		MessageBox(0,err_msg,"Warning!",MB_OK | MB_ICONWARNING);
		return;
	}

	result = fprintf(pf,
		"height = %li\n"
		"width = %li\n"
		"escapement = %li\n"
		"orientation = %li\n"
		"weight = %li\n"
		"italic = %i\n"
		"underline = %i\n"
		"strikeout = %i\n"
		"charset = %i\n"
		"outprecision = %i\n"
		"clipprecision = %i\n"
		"quality = %i\n"
		"pitchandfamily = %i\n"
		"facename = \"%s\"\n",
		lf.lfHeight,
		lf.lfWidth,
		lf.lfEscapement,
		lf.lfOrientation,
		lf.lfWeight,
		lf.lfItalic,
		lf.lfUnderline,
		lf.lfStrikeOut,
		lf.lfCharSet,
		lf.lfOutPrecision,
		lf.lfClipPrecision,
		lf.lfQuality,
		lf.lfPitchAndFamily,
		lf.lfFaceName
		);
	fclose(pf);
	if(result < 0){
		_snprintf(err_msg,sizeof(err_msg) - 1,
			"Can't write gui preferences to %s!\n%s",
			buffer,_strerror(NULL));
		err_msg[sizeof(err_msg) - 1] = 0;
		MessageBox(0,err_msg,"Warning!",MB_OK | MB_ICONWARNING);
	}
}
