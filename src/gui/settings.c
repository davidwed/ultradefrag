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
* GUI - load/save/change program settings.
*/

#include "main.h"

RECT win_rc; /* coordinates of main window */
int restore_default_window_size = 0;
int skip_removable = TRUE;
int hibernate_instead_of_shutdown = FALSE;
char buffer[MAX_PATH];
char err_msg[1024];
extern int user_defined_column_widths[];
extern int map_block_size;
BOOLEAN map_block_size_loaded = FALSE;
int reloaded_map_block_size = 0;
extern int grid_line_width;
BOOLEAN grid_line_width_loaded = FALSE;
int reloaded_grid_line_width = 0;
extern int disable_latest_version_check;

/* they have the same effect as environment variables for console program */
char in_filter[4096] = {0};
char ex_filter[4096] = {0};
char sizelimit[64] = {0};
char timelimit[256] = {0};
int fraglimit;
int refresh_interval;
int disable_reports;
char dbgprint_level[32] = {0};

int show_shutdown_check_confirmation_dialog = 1;
int seconds_for_shutdown_rejection = 60;

extern HWND hWindow;
extern HFONT hFont;

void DisplayLastError(char *caption);

void DeleteEnvironmentVariables(void)
{
	(void)SetEnvironmentVariable("UD_IN_FILTER",NULL);
	(void)SetEnvironmentVariable("UD_EX_FILTER",NULL);
	(void)SetEnvironmentVariable("UD_SIZELIMIT",NULL);
	(void)SetEnvironmentVariable("UD_FRAGMENTS_THRESHOLD",NULL);
	(void)SetEnvironmentVariable("UD_REFRESH_INTERVAL",NULL);
	(void)SetEnvironmentVariable("UD_DISABLE_REPORTS",NULL);
	(void)SetEnvironmentVariable("UD_DBGPRINT_LEVEL",NULL);
	(void)SetEnvironmentVariable("UD_TIME_LIMIT",NULL);
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
	if(timelimit[0])
		(void)SetEnvironmentVariable("UD_TIME_LIMIT",timelimit);
	if(fraglimit){
		(void)sprintf(buffer,"%i",fraglimit);
		(void)SetEnvironmentVariable("UD_FRAGMENTS_THRESHOLD",buffer);
	}
	if(refresh_interval){
		(void)sprintf(buffer,"%i",refresh_interval);
		(void)SetEnvironmentVariable("UD_REFRESH_INTERVAL",buffer);
	}
	(void)sprintf(buffer,"%i",disable_reports);
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
	BOOLEAN coord_undefined = TRUE;
	
	win_rc.left = win_rc.top = 0;
	in_filter[0] = ex_filter[0] = sizelimit[0] = dbgprint_level[0] = 0;
	fraglimit = 0;
	refresh_interval = DEFAULT_REFRESH_INTERVAL;
	disable_reports = 0;

	DeleteEnvironmentVariables();
	
	L = lua_open();  /* create state */
	if(!L) return;
	lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
	luaL_openlibs(L);  /* open libraries */
	lua_gc(L, LUA_GCRESTART, 0);

	status = luaL_dofile(L,".\\options\\guiopts.lua");
	if(!status){ /* successful */
		/* get main window coordinates */
		coord_undefined = FALSE;
		lua_getglobal(L, "x");
		if(!lua_isnil(L, lua_gettop(L))){
			win_rc.left = (long)lua_tointeger(L, lua_gettop(L));
			lua_pop(L, 1);
		} else {
			coord_undefined = TRUE;
		}
		lua_getglobal(L, "y");
		if(!lua_isnil(L, lua_gettop(L))){
			win_rc.top = (long)lua_tointeger(L, lua_gettop(L));
			lua_pop(L, 1);
		} else {
			coord_undefined = TRUE;
		}

		win_rc.right = win_rc.left + (long)getint(L,"width");
		win_rc.bottom = win_rc.top + (long)getint(L,"height");
		restore_default_window_size = getint(L,"restore_default_window_size");
		skip_removable = getint(L,"skip_removable");
		user_defined_column_widths[0] = getint(L,"column1_width");
		user_defined_column_widths[1] = getint(L,"column2_width");
		user_defined_column_widths[2] = getint(L,"column3_width");
		user_defined_column_widths[3] = getint(L,"column4_width");
		user_defined_column_widths[4] = getint(L,"column5_width");

		lua_getglobal(L, "in_filter");
		string = (char *)lua_tostring(L, lua_gettop(L));
		if(string){
			(void)strncpy(in_filter,string,sizeof(in_filter));
			in_filter[sizeof(in_filter) - 1] = 0;
		}
		lua_pop(L, 1);

		lua_getglobal(L, "ex_filter");
		string = (char *)lua_tostring(L, lua_gettop(L));
		if(string){
			(void)strncpy(ex_filter,string,sizeof(ex_filter));
			ex_filter[sizeof(ex_filter) - 1] = 0;
		}
		lua_pop(L, 1);

		lua_getglobal(L, "sizelimit");
		string = (char *)lua_tostring(L, lua_gettop(L));
		if(string){
			(void)strncpy(sizelimit,string,sizeof(sizelimit));
			sizelimit[sizeof(sizelimit) - 1] = 0;
		}
		lua_pop(L, 1);

		lua_getglobal(L, "time_limit");
		string = (char *)lua_tostring(L, lua_gettop(L));
		if(string){
			(void)strncpy(timelimit,string,sizeof(timelimit));
			timelimit[sizeof(timelimit) - 1] = 0;
		}
		lua_pop(L, 1);

		fraglimit = getint(L,"fragments_threshold");
		refresh_interval = getint(L,"refresh_interval");
		if(!refresh_interval) refresh_interval = DEFAULT_REFRESH_INTERVAL;
		disable_reports = getint(L,"disable_reports");

		lua_getglobal(L, "dbgprint_level");
		string = (char *)lua_tostring(L, lua_gettop(L));
		if(string){
			(void)strncpy(dbgprint_level,string,sizeof(dbgprint_level));
			dbgprint_level[sizeof(dbgprint_level) - 1] = 0;
		}
		lua_pop(L, 1);
		
		hibernate_instead_of_shutdown = getint(L,
				"hibernate_instead_of_shutdown");

		lua_getglobal(L, "show_shutdown_check_confirmation_dialog");
		if(!lua_isnil(L, lua_gettop(L))){
			show_shutdown_check_confirmation_dialog = (int)lua_tointeger(L, lua_gettop(L));
			lua_pop(L, 1);
		}

		lua_getglobal(L, "seconds_for_shutdown_rejection");
		if(!lua_isnil(L, lua_gettop(L))){
			seconds_for_shutdown_rejection = (int)lua_tointeger(L, lua_gettop(L));
			lua_pop(L, 1);
		}
		
		/* load the size of the cluster map block only if it is not already loaded */
		reloaded_map_block_size = getint(L,"map_block_size");
		if(map_block_size_loaded == FALSE){
			map_block_size = reloaded_map_block_size;
			if(map_block_size == 0) map_block_size = DEFAULT_MAP_BLOCK_SIZE;
			map_block_size_loaded = TRUE;
		}

		/* load the grid line width only if it is not already loaded */
		lua_getglobal(L, "grid_line_width");
		if(!lua_isnil(L, lua_gettop(L))){
			reloaded_grid_line_width = (int)lua_tointeger(L, lua_gettop(L));
			lua_pop(L, 1);
		} else {
			reloaded_grid_line_width = DEFAULT_GRID_LINE_WIDTH;
		}
		if(grid_line_width_loaded == FALSE){
			grid_line_width = reloaded_grid_line_width;
			grid_line_width_loaded = TRUE;
		}

		disable_latest_version_check = getint(L,
				"disable_latest_version_check");
	}
	lua_close(L);

	/* center main window on the screen if coordinates aren't defined */
	if(coord_undefined){
		win_rc.left = win_rc.top = UNDEFINED_COORD;
	}

	SetEnvironmentVariables();
}

void SavePrefs(void)
{
	FILE *pf;
	int result;
	
	if(!GetCurrentDirectory(MAX_PATH,buffer)){
		DisplayLastError("Cannot retrieve the current directory path!");
		return;
	}
	(void)strcat(buffer,"\\options\\guiopts.lua");
	pf = fopen(buffer,"wt");
	if(!pf){
		(void)_snprintf(err_msg,sizeof(err_msg) - 1,
			"Can't save gui preferences to %s!\n%s",
			buffer,_strerror(NULL));
		err_msg[sizeof(err_msg) - 1] = 0;
		MessageBox(0,err_msg,"Warning!",MB_OK | MB_ICONWARNING);
		return;
	}

	/* save main window size for configuraton initialization */
	result = fprintf(pf,
		"-- UltraDefrag GUI options\n\n"
		"-- Note that you must specify paths in filters\n"
		"-- with double back slashes instead of the single ones.\n"
		"-- For example:\n"
		"-- ex_filter = \"MyDocs\\\\Music\\\\mp3\\\\Red_Hot_Chili_Peppers\"\n\n"
		"in_filter = \"%s\"\n"
		"ex_filter = \"%s\"\n"
		"sizelimit = \"%s\"\n"
		"fragments_threshold = %i\n"
		"refresh_interval = %i\n"
		"time_limit = \"%s\"\n\n"
		"disable_reports = %i\n\n"
		"-- set dbgprint_level to DETAILED for reporting a bug,\n"
		"-- for normal operation set it to NORMAL or an empty string\n"
		"dbgprint_level = \"%s\"\n\n"
		"-- set hibernate_instead_of_shutdown to 1, if you prefer to hibernate the system\n"
		"-- after a job is done instead of shutting it down, otherwise set it to 0\n"
		"hibernate_instead_of_shutdown = %u\n\n"
		"-- set show_shutdown_check_confirmation_dialog to 1 to display the confirmation dialog\n"
		"-- for shutdown or hibernate, otherwise set it to 0\n"
		"show_shutdown_check_confirmation_dialog = %u\n\n"
		"-- seconds_for_shutdown_rejection sets the delay for the user to cancel\n"
		"-- the shutdown or hibernate execution, default is 60 seconds\n"
		"seconds_for_shutdown_rejection = %u\n\n"
		"-- cluster map options (restart required to take effect):\n"
		"-- the size of the block, in pixels; default value is %i\n"
		"map_block_size = %i\n\n"
		"-- the grid line width, in pixels; default value is %i\n"
		"grid_line_width = %i\n\n"
		"-- set disable_latest_version_check parameter to 1\n"
		"-- to disable the automatic check for the latest available\n"
		"-- version of the program during startup\n"
		"disable_latest_version_check = %i\n\n"
		"-- window coordinates etc.\n\n"
		"-- set restore_default_window_size parameter to 1\n"
		"-- to restore default window size on the next startup\n"
		"restore_default_window_size = %i\n\n"
		"-- the settings below are not changable by the user,\n"
		"-- they are always overwritten when the program ends\n"
		"x = %i\ny = %i\n"
		"width = %i\nheight = %i\n\n"
		"skip_removable = %i\n\n"
		"column1_width = %i\ncolumn2_width = %i\n"
		"column3_width = %i\ncolumn4_width = %i\n"
		"column5_width = %i\n",
		in_filter,
		ex_filter,
		sizelimit,
		fraglimit,
		refresh_interval,
		timelimit,
		disable_reports,
		dbgprint_level,
		hibernate_instead_of_shutdown,
		show_shutdown_check_confirmation_dialog,
		seconds_for_shutdown_rejection,
		DEFAULT_MAP_BLOCK_SIZE,
		reloaded_map_block_size ? reloaded_map_block_size : map_block_size,
		DEFAULT_GRID_LINE_WIDTH,
		reloaded_grid_line_width ? reloaded_grid_line_width : grid_line_width,
		disable_latest_version_check,
		restore_default_window_size,
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
		(void)_snprintf(err_msg,sizeof(err_msg) - 1,
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
}
