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

/* Function prototypes */
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

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
	WgxDestroyResourceTable(i18n_table);
	return 0;
}

/*---------------- Main Dialog Callback ---------------------*/
BOOL CALLBACK DlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	short lng_file_path[MAX_PATH];

	switch(msg){
	case WM_INITDIALOG:
		/* Window Initialization */
		hWindow = hWnd;
		GetWindowsDirectoryW(lng_file_path,MAX_PATH);
		wcscat(lng_file_path,L"\\UltraDefrag\\ud_config_i18n.lng");
		if(WgxBuildResourceTable(i18n_table,lng_file_path))
			WgxApplyResourceTable(i18n_table,hWindow);
		WgxSetIcon(hInstance,hWindow,IDI_CONFIG);
		break;
	case WM_COMMAND:
		/*switch(LOWORD(wParam)){
		}*/
		break;
	case WM_CLOSE:
		EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}
