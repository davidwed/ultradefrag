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

RECT win_rc; /* coordinates of main window */
int skip_removable = TRUE;
char buffer[MAX_PATH];
char err_msg[1024];
extern int user_defined_column_widths[];

extern HWND hWindow;
extern HFONT hFont;

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
		_snprintf(err_msg,sizeof(err_msg) - 1,
			"Can't save gui preferences to %s!\n%s",
			buffer,_strerror(NULL));
		err_msg[sizeof(err_msg) - 1] = 0;
		MessageBox(0,err_msg,"Warning!",MB_OK | MB_ICONWARNING);
		return;
	}

	/* save main window size for configurator's initialization */
	result = fprintf(pf,
		"x = %i\ny = %i\n"
		"width = %i\nheight = %i\n\n"
		"skip_removable = %i\n\n"
		"column1_width = %i\ncolumn2_width = %i\n"
		"column3_width = %i\ncolumn4_width = %i\n"
		"column5_width = %i\n",
		(int)win_rc.left, (int)win_rc.top,
		(int)(win_rc.right - win_rc.left),
		(int)(win_rc.bottom - win_rc.top),
		skip_removable,
		user_defined_column_widths[0],
		user_defined_column_widths[1],
		user_defined_column_widths[2],
		user_defined_column_widths[3],
		user_defined_column_widths[4]
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

void InitFont(void)
{
	LOGFONT lf;
	lua_State *L;
	int status;
	char *string;
	HFONT hNewFont;

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
	lua_close(L);
	
	/* apply font to application's window */
	hNewFont = WgxSetFont(hWindow,&lf);
	if(hNewFont){
		if(hFont) DeleteObject(hFont);
		hFont = hNewFont;
	}
}
