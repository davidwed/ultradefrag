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

/**
 * @file settings.c
 * @brief Preferences.
 * @addtogroup Preferences
 * @{
 */

#include "main.h"

/*
* These options have the same effect as environment 
* variables accepted by the command line interface.
*/
char in_filter[32767];
char ex_filter[32767];
char sizelimit[128];
char timelimit[256];
int fraglimit;
int refresh_interval;
int disable_reports;
char dbgprint_level[32] = {0};

/*
* GUI specific options
*/
int hibernate_instead_of_shutdown = FALSE;
int show_shutdown_check_confirmation_dialog = 1;
int seconds_for_shutdown_rejection = 60;
int scale_by_dpi = 1;
int restore_default_window_size = 0;
int maximized_window = 0;
int init_maximized_window = 0;
int skip_removable = TRUE;
int disable_latest_version_check = 0;
int user_defined_column_widths[] = {0,0,0,0,0};

int rx = UNDEFINED_COORD;
int ry = UNDEFINED_COORD;
int rwidth = 0;
int rheight = 0;

extern RECT r_rc;
extern int map_block_size;
extern int grid_line_width;
extern double pix_per_dialog_unit;
extern int last_block_size;
extern int last_grid_width;
extern int last_x;
extern int last_y;
extern int last_width;
extern int last_height;

WGX_OPTION options[] = {
	/* type, value buffer size, name, value, default value */
	{WGX_CFG_COMMENT, 0, "UltraDefrag GUI options",                                         NULL, ""},
	{WGX_CFG_EMPTY,   0, "",                                                                NULL, ""},
	{WGX_CFG_COMMENT, 0, "Note that you must specify paths in filters",                     NULL, ""},
	{WGX_CFG_COMMENT, 0, "with double back slashes instead of the single ones.",            NULL, ""},
	{WGX_CFG_COMMENT, 0, "For example:",                                                    NULL, ""},
	{WGX_CFG_COMMENT, 0, "ex_filter = \"MyDocs\\\\Music\\\\mp3\\\\Red_Hot_Chili_Peppers\"", NULL, ""},
	{WGX_CFG_EMPTY,   0, "",                                                                NULL, ""},
	{WGX_CFG_STRING,  sizeof(in_filter), "in_filter",           in_filter,  ""},
	/* default value for ex_filter is more advanced to reduce volume processing time */
	{WGX_CFG_STRING,  sizeof(ex_filter), "ex_filter",           ex_filter,  "system volume information;temp;recycle"},
	{WGX_CFG_STRING,  sizeof(sizelimit), "sizelimit",           sizelimit,  ""},
	{WGX_CFG_INT,     0,                 "fragments_threshold", &fraglimit, 0},
	{WGX_CFG_STRING,  sizeof(timelimit), "time_limit",          timelimit,  ""},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},

	{WGX_CFG_INT,     0, "refresh_interval", &refresh_interval, (void *)DEFAULT_REFRESH_INTERVAL},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},
	{WGX_CFG_INT,     0, "disable_reports",  &disable_reports,  0},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},

	{WGX_CFG_COMMENT, 0, "set dbgprint_level to DETAILED for reporting a bug,",      NULL, ""},
	{WGX_CFG_COMMENT, 0, "for normal operation set it to NORMAL or an empty string", NULL, ""},
	{WGX_CFG_STRING,  sizeof(dbgprint_level), "dbgprint_level", dbgprint_level, ""},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},
	
	{WGX_CFG_COMMENT, 0, "set hibernate_instead_of_shutdown to 1, if you prefer to hibernate the system", NULL, ""},
	{WGX_CFG_COMMENT, 0, "after a job is done instead of shutting it down, otherwise set it to 0",        NULL, ""},
	{WGX_CFG_INT,     0, "hibernate_instead_of_shutdown", &hibernate_instead_of_shutdown, 0},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},
	
	{WGX_CFG_COMMENT, 0, "set show_shutdown_check_confirmation_dialog to 1 to display the confirmation dialog", NULL, ""},
	{WGX_CFG_COMMENT, 0, "for shutdown or hibernate, otherwise set it to 0",                                    NULL, ""},
	{WGX_CFG_INT,     0, "show_shutdown_check_confirmation_dialog", &show_shutdown_check_confirmation_dialog, (void *)1},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},
		
	{WGX_CFG_COMMENT, 0, "seconds_for_shutdown_rejection sets the delay for the user to cancel", NULL, ""},
	{WGX_CFG_COMMENT, 0, "the shutdown or hibernate execution, default is 60 seconds", NULL, ""},
	{WGX_CFG_INT,     0, "seconds_for_shutdown_rejection", &seconds_for_shutdown_rejection, (void *)60},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},
	
	{WGX_CFG_COMMENT, 0, "cluster map options (restart required to take effect):", NULL, ""},
	{WGX_CFG_COMMENT, 0, "the size of the block, in pixels; default value is 4", NULL, ""},
	{WGX_CFG_INT,     0, "map_block_size", &map_block_size, (void *)DEFAULT_MAP_BLOCK_SIZE},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},
	
	{WGX_CFG_COMMENT, 0, "the grid line width, in pixels; default value is 1", NULL, ""},
	{WGX_CFG_INT,     0, "grid_line_width", &grid_line_width, (void *)DEFAULT_GRID_LINE_WIDTH},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},

	{WGX_CFG_COMMENT, 0, "set disable_latest_version_check parameter to 1", NULL, ""},
	{WGX_CFG_COMMENT, 0, "to disable the automatic check for the latest available", NULL, ""},
	{WGX_CFG_COMMENT, 0, "version of the program during startup", NULL, ""},
	{WGX_CFG_INT,     0, "disable_latest_version_check", &disable_latest_version_check, 0},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},
	
	{WGX_CFG_COMMENT, 0, "window coordinates etc.", NULL, ""},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},
	
	{WGX_CFG_COMMENT, 0, "set scale_by_dpi parameter to 0", NULL, ""},
	{WGX_CFG_COMMENT, 0, "to not scale the buttons and text according to the", NULL, ""},
	{WGX_CFG_COMMENT, 0, "screens DPI settings", NULL, ""},
	{WGX_CFG_INT,     0, "scale_by_dpi", &scale_by_dpi, (void *)1},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},

	{WGX_CFG_COMMENT, 0, "set restore_default_window_size parameter to 1", NULL, ""},
	{WGX_CFG_COMMENT, 0, "to restore default window size on the next startup", NULL, ""},
	{WGX_CFG_INT,     0, "restore_default_window_size", &restore_default_window_size, 0},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},
	
	{WGX_CFG_COMMENT, 0, "the settings below are not changeable by the user,", NULL, ""},
	{WGX_CFG_COMMENT, 0, "they are always overwritten when the program ends", NULL, ""},
	
	{WGX_CFG_INT,     0, "rx", &rx, (void *)UNDEFINED_COORD},
	{WGX_CFG_INT,     0, "ry", &ry, (void *)UNDEFINED_COORD},
	{WGX_CFG_INT,     0, "rwidth",  &rwidth,  (void *)DEFAULT_WIDTH},
	{WGX_CFG_INT,     0, "rheight", &rheight, (void *)DEFAULT_HEIGHT}, 
	{WGX_CFG_INT,     0, "maximized", &maximized_window, 0},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},

	{WGX_CFG_INT,     0, "skip_removable", &skip_removable, (void *)1},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},

	{WGX_CFG_INT,     0, "column1_width", &user_defined_column_widths[0], 0},
	{WGX_CFG_INT,     0, "column2_width", &user_defined_column_widths[1], 0},
	{WGX_CFG_INT,     0, "column3_width", &user_defined_column_widths[2], 0},
	{WGX_CFG_INT,     0, "column4_width", &user_defined_column_widths[3], 0},
	{WGX_CFG_INT,     0, "column5_width", &user_defined_column_widths[4], 0},
	{WGX_CFG_EMPTY,   0, "", NULL, ""},

	{0,               0, NULL, NULL, NULL}
};

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

void GetPrefs(void)
{
	WgxGetOptions(".\\options\\guiopts.lua",options);
	
	/* get restored main window coordinates */
	r_rc.left = rx;
	r_rc.top = ry;
	r_rc.right = rx + rwidth;
	r_rc.bottom = ry + rheight;
	
	init_maximized_window = maximized_window;

	DeleteEnvironmentVariables();
	SetEnvironmentVariables();
}

void __stdcall SavePrefsCallback(char *error)
{
	MessageBox(NULL,error,"Warning!",MB_OK | MB_ICONWARNING);
}

void SavePrefs(void)
{
	rx = (int)r_rc.left;
	ry = (int)r_rc.top;
	rwidth = (int)(r_rc.right - r_rc.left);
	rheight = (int)(r_rc.bottom - r_rc.top);
	
	WgxSaveOptions(".\\options\\guiopts.lua",options,SavePrefsCallback);
}

/**
 * @brief Initializes default font
 * and replaces it by a custom one
 * if an appropriate configuration
 * file exists.
 */
void InitFont(void)
{
	memset(&wgxFont.lf,0,sizeof(LOGFONT));
	/* default font should be Courier New 9pt */
	(void)strcpy(wgxFont.lf.lfFaceName,"Courier New");
	wgxFont.lf.lfHeight = DPI(-12);
	WgxCreateFont(".\\options\\font.lua",&wgxFont);
}

/**
 * OpenConfigurationDialog thread procedure.
 */
DWORD WINAPI ConfigThreadProc(LPVOID lpParameter)
{
	char path[MAX_PATH];
	char buffer[MAX_PATH + 64];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	/* update configuration file by actual preferences */
	SavePrefs();

	/* pass window handle to the configuration dialog to center it */
	(void)strcpy(path,".\\udefrag-gui-config.exe");
	sprintf(buffer,"%s %p",path,hWindow);
    WgxDbgPrint("UltraDefrag GUI passed window handle as %p\n",hWindow);

	ZeroMemory(&si,sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
	ZeroMemory(&pi,sizeof(pi));

	if(!CreateProcess(path,buffer,
	  NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)){
	    WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,
			"Cannot execute udefrag-gui-config.exe program!");
	    return 0;
	}
	(void)WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	/* stop running jobs */
	stop_all_jobs();

	/* reinitialize GUI */
	GetPrefs();
	WgxDestroyFont(&wgxFont);
	InitFont();
	WgxSetFont(hWindow,&wgxFont);
		
	if(hibernate_instead_of_shutdown)
		WgxSetText(GetDlgItem(hWindow,IDC_SHUTDOWN),i18n_table,L"HIBERNATE_PC_AFTER_A_JOB");
	else
		WgxSetText(GetDlgItem(hWindow,IDC_SHUTDOWN),i18n_table,L"SHUTDOWN_PC_AFTER_A_JOB");

	/* if block size or grid line width changed since last redraw, resize map */
	if(map_block_size != last_block_size  || grid_line_width != last_grid_width){
		ResizeMap(last_x,last_y,last_width,last_height);
		InvalidateRect(hMap,NULL,TRUE);
		UpdateWindow(hMap);
	}
	return 0;
}

/**
 * @brief Opens configuration dialog.
 */
void OpenConfigurationDialog(void)
{
	HANDLE h;
	DWORD id;
	
	h = create_thread(ConfigThreadProc,NULL,&id);
	if(h == NULL){
		WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,
			"Cannot create thread opening the Configuration dialog!");
	} else {
		CloseHandle(h);
	}
}

/** @} */
