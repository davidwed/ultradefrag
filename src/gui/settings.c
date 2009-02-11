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
* GUI - load/save/change program settings.
*/

#include "main.h"

extern HWND hWindow;
RECT win_rc; /* coordinates of main window */
int skip_removable = TRUE;

char buffer[MAX_PATH];

char err_msg[1024];

extern int user_defined_column_widths[];

BOOL CALLBACK NewSettingsDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	/* When a portable app launches gui the current directory points to a temp dir. */
	char buf[MAX_PATH];
	
	switch(msg){
	case WM_INITDIALOG:
		/* Window Initialization */
		SetWindowPos(hWnd,0,win_rc.left + /*117*/153,win_rc.top + 158/*155*/,0,0,SWP_NOSIZE);
		SetText(hWnd,L"SETTINGS");
		SetText(GetDlgItem(hWnd,IDC_EDITMAINOPTS),L"EDIT_MAIN_OPTS");
		SetText(GetDlgItem(hWnd,IDC_EDITREPORTOPTS),L"EDIT_REPORT_OPTS");
		SetText(GetDlgItem(hWnd,IDC_SETTINGS_HELP),L"READ_USER_MANUAL");
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_EDITMAINOPTS:
			GetSystemDirectory(buf,MAX_PATH);
			strcat(buf,"\\udefrag-gui.cmd");
			//ShellExecute(hWindow,"open",".\\options\\udefrag.cfg",NULL,NULL,SW_SHOW);
			ShellExecute(hWindow,"edit",buf,NULL,NULL,SW_SHOW);
			break;
		case IDC_EDITREPORTOPTS:
			GetWindowsDirectory(buf,MAX_PATH);
			strcat(buf,"\\UltraDefrag\\options\\udreportopts.lua");
			ShellExecute(hWindow,"open",buf,NULL,NULL,SW_SHOW);
			break;
		case IDC_SETTINGS_HELP:
			GetWindowsDirectory(buf,MAX_PATH);
			strcat(buf,"\\UltraDefrag\\handbook\\index.html");
			ShellExecute(hWindow,"open",buf,NULL,NULL,SW_SHOW);
			break;
		}
		return FALSE;
	case WM_CLOSE:
		EndDialog(hWnd,0);
		return TRUE;
	}
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

void GetPrefs(void)
{
	lua_State *L;
	int status;
	
	win_rc.left = win_rc.top = 0;

	L = lua_open();  /* create state */
	if(!L) return;
	lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
	luaL_openlibs(L);  /* open libraries */
	lua_gc(L, LUA_GCRESTART, 0);

	GetWindowsDirectory(buffer,MAX_PATH);
	strcat(buffer,"\\UltraDefrag\\options\\guiopts.lua");
	status = luaL_dofile(L,buffer);
	if(!status){ /* successful */
		win_rc.left = (long)getint(L,"x");
		win_rc.top = (long)getint(L,"y");
		skip_removable = getint(L,"skip_removable");
		user_defined_column_widths[0] = getint(L,"column1_width");
		user_defined_column_widths[1] = getint(L,"column2_width");
		user_defined_column_widths[2] = getint(L,"column3_width");
		user_defined_column_widths[3] = getint(L,"column4_width");
		user_defined_column_widths[4] = getint(L,"column5_width");
	}
	lua_close(L);
}

void SavePrefs(void)
{
	FILE *pf;
	int result;
	
	GetWindowsDirectory(buffer,MAX_PATH);
	strcat(buffer,"\\UltraDefrag\\options\\guiopts.lua");
	pf = fopen(buffer,"wt");
	if(!pf){
		snprintf(err_msg,sizeof(err_msg) - 1,
			"Can't save gui preferences to %s!\n%s",
			buffer,_strerror(NULL));
		err_msg[sizeof(err_msg) - 1] = 0;
		MessageBox(0,err_msg,"Warning!",MB_OK | MB_ICONWARNING);
		return;
	}

	result = fprintf(pf,
		"x = %i\ny = %i\n\nskip_removable = %i\n\n"
		"column1_width = %i\ncolumn2_width = %i\n"
		"column3_width = %i\ncolumn4_width = %i\n"
		"column5_width = %i\n",
		(int)win_rc.left, (int)win_rc.top, skip_removable,
		user_defined_column_widths[0],
		user_defined_column_widths[1],
		user_defined_column_widths[2],
		user_defined_column_widths[3],
		user_defined_column_widths[4]
		);
	fclose(pf);
	if(result < 0){
		snprintf(err_msg,sizeof(err_msg) - 1,
			"Can't write gui preferences to %s!\n%s",
			buffer,_strerror(NULL));
		err_msg[sizeof(err_msg) - 1] = 0;
		MessageBox(0,err_msg,"Warning!",MB_OK | MB_ICONWARNING);
	}
}
