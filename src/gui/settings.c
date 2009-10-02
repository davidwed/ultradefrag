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
int hibernate_instead_of_shutdown = FALSE;
char buffer[MAX_PATH];
char err_msg[1024];
extern int user_defined_column_widths[];

/* they have the same effect as environment variables for console program */
char in_filter[4096] = {0};
char ex_filter[4096] = {0};
char sizelimit[64] = {0};
int refresh_interval;
int disable_reports;
char dbgprint_level[32] = {0};

extern HWND hWindow;
extern HFONT hFont;

void DeleteEnvironmentVariables(void)
{
	(void)SetEnvironmentVariable("UD_IN_FILTER",NULL);
	(void)SetEnvironmentVariable("UD_EX_FILTER",NULL);
	(void)SetEnvironmentVariable("UD_SIZELIMIT",NULL);
	(void)SetEnvironmentVariable("UD_REFRESH_INTERVAL",NULL);
	(void)SetEnvironmentVariable("UD_DISABLE_REPORTS",NULL);
	(void)SetEnvironmentVariable("UD_DBGPRINT_LEVEL",NULL);
}

void SetEnvironmentVariables(void)
{
	char buffer[128];
	
	if(in_filter[0])
		(void)SetEnvironmentVariable("UD_IN_FILTER",in_filter);
	if(ex_filter[0])
		(void)SetEnvironmentVariable("UD_EX_FILTER",ex_filter);
	if(sizelimit[0])
		(void)SetEnvironmentVariable("UD_SIZELIMIT",sizelimit);
	if(refresh_interval){
		sprintf(buffer,"%i",refresh_interval);
		(void)SetEnvironmentVariable("UD_REFRESH_INTERVAL",buffer);
	}
	sprintf(buffer,"%i",disable_reports);
	(void)SetEnvironmentVariable("UD_DISABLE_REPORTS",buffer);
	if(dbgprint_level[0])
		(void)SetEnvironmentVariable("UD_DBGPRINT_LEVEL",dbgprint_level);
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
	char *string;
	
	win_rc.left = win_rc.top = 0;
	in_filter[0] = ex_filter[0] = sizelimit[0] = dbgprint_level[0] = 0;
	refresh_interval = 0;
	disable_reports = 0;

	DeleteEnvironmentVariables();
	
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
		win_rc.right = win_rc.left + (long)getint(L,"width");
		win_rc.bottom = win_rc.top + (long)getint(L,"height");
		skip_removable = getint(L,"skip_removable");
		user_defined_column_widths[0] = getint(L,"column1_width");
		user_defined_column_widths[1] = getint(L,"column2_width");
		user_defined_column_widths[2] = getint(L,"column3_width");
		user_defined_column_widths[3] = getint(L,"column4_width");
		user_defined_column_widths[4] = getint(L,"column5_width");

		lua_getglobal(L, "in_filter");
		string = (char *)lua_tostring(L, lua_gettop(L));
		if(string){
			strncpy(in_filter,string,sizeof(in_filter));
			in_filter[sizeof(in_filter) - 1] = 0;
		}
		lua_pop(L, 1);

		lua_getglobal(L, "ex_filter");
		string = (char *)lua_tostring(L, lua_gettop(L));
		if(string){
			strncpy(ex_filter,string,sizeof(ex_filter));
			ex_filter[sizeof(ex_filter) - 1] = 0;
		}
		lua_pop(L, 1);

		lua_getglobal(L, "sizelimit");
		string = (char *)lua_tostring(L, lua_gettop(L));
		if(string){
			strncpy(sizelimit,string,sizeof(sizelimit));
			sizelimit[sizeof(sizelimit) - 1] = 0;
		}
		lua_pop(L, 1);

		refresh_interval = getint(L,"refresh_interval");
		disable_reports = getint(L,"disable_reports");

		lua_getglobal(L, "dbgprint_level");
		string = (char *)lua_tostring(L, lua_gettop(L));
		if(string){
			strncpy(dbgprint_level,string,sizeof(dbgprint_level));
			dbgprint_level[sizeof(dbgprint_level) - 1] = 0;
		}
		lua_pop(L, 1);
		
		hibernate_instead_of_shutdown = getint(L,
				"hibernate_instead_of_shutdown");
	}
	lua_close(L);

	SetEnvironmentVariables();
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
		"-- UltraDefrag GUI options\n\n"
		"-- Note that you must specify paths in filters\n"
		"-- with double back slashes instead of the single.\n"
		"-- Par example:\n"
		"-- ex_filter = \"MyDocs\\\\Music\\\\mp3\\\\Red_Hot_Chili_Peppers\"\n\n"
		"in_filter = \"%s\"\n"
		"ex_filter = \"%s\"\n"
		"sizelimit = \"%s\"\n"
		"refresh_interval = %i\n"
		"disable_reports = %i\n"
		"dbgprint_level = \"%s\"\n\n"
		"-- set this parameter to 1 if you prefer to hibernate system\n"
		"-- after a job instead of shutting them down, otherwise set it to 0\n\n"
		"hibernate_instead_of_shutdown = %u\n\n"
		"-- window coordinates etc.\n"
		"x = %i\ny = %i\n"
		"width = %i\nheight = %i\n\n"
		"skip_removable = %i\n\n"
		"column1_width = %i\ncolumn2_width = %i\n"
		"column3_width = %i\ncolumn4_width = %i\n"
		"column5_width = %i\n",
		in_filter,
		ex_filter,
		sizelimit,
		refresh_interval,
		disable_reports,
		dbgprint_level,
		hibernate_instead_of_shutdown,
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
	HFONT hNewFont;

	/* initialize LOGFONT structure */
	memset(&lf,0,sizeof(LOGFONT));
	/* default font should be Courier New 9pt */
	strcpy(lf.lfFaceName,"Courier New");
	lf.lfHeight = -12;
	
	/* load saved font settings */
	GetWindowsDirectory(buffer,MAX_PATH);
	strcat(buffer,"\\UltraDefrag\\options\\font.lua");
	if(!WgxGetLogFontStructureFromFile(buffer,&lf)) return;

	/* apply font to application's window */
	hNewFont = WgxSetFont(hWindow,&lf);
	if(hNewFont){
		if(hFont) DeleteObject(hFont);
		hFont = hNewFont;
	}
}
