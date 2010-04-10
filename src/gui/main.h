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
 *  GUI - common definitions.
 */

#ifndef _DFRG_MAIN_H_
#define _DFRG_MAIN_H_

#define DEFAULT_MAP_BLOCK_SIZE  6 //84
#define DEFAULT_GRID_LINE_WIDTH 1

/*
* An article of Mumtaz Zaheer from Pakistan helped me very much
* to make a valid subclassing:
* http://www.codeproject.com/KB/winsdk/safesubclassing.aspx
*/

/*
* VERY IMPORTANT NOTE: (bug #1839755 cause)
* 
* + You should not wait for one SendMessage handler completion
*   from another SendMessage handler. Because it will be a deadlock cause.
*
* + Example:
*
* int done;
*
* window_proc()
* {
*   if(stop button was pressed)
*   {
*     stop_the_driver();
*     wait for done flag;
*   }
* }
*
* analyse_thread_proc()
* {
*   done = 0;
*   analyse();
*   RedrawMap();
*   done = 1;
* }
*
* + How it works:
* 
* RedrawMap() will send message to the map control 
* that can not be handled by system before window_proc() returns.
* But window_proc() is waiting for done flag, that can be set
* after(!) RedrawMap() call. Therefore we have a deadlock.
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

#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>
#include <math.h>
#include <process.h>

#define lua_c
#include "../lua5.1/lua.h"
#include "../lua5.1/lauxlib.h"
#include "../lua5.1/lualib.h"

#include "../include/udefrag.h"
#include "resource.h"

#include "../dll/wgx/wgx.h"

enum {
	STATUS_UNDEFINED,
	STATUS_RUNNING,
	STATUS_ANALYSED,
	STATUS_DEFRAGMENTED,
	STATUS_OPTIMIZED
};

#define create_thread(func,param,ph) \
		CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)func,(void *)param,0,ph)

#define MAX_VNAME_LEN  31
#define MAX_FSNAME_LEN 31

typedef struct {
	char name[MAX_VNAME_LEN + 1];
	char fsname[MAX_FSNAME_LEN + 1];
	int status;
	STATISTIC stat;
	HDC hdc;
	HBITMAP hbitmap;
} NEW_VOLUME_LIST_ENTRY, *PNEW_VOLUME_LIST_ENTRY;
		
void DoJob(char job_type);
void stop(void);

/* settings related functions */
void GetPrefs(void);
void SavePrefs(void);

short *GetNewVersionAnnouncement(void);

/* i18n related functions */
void SetText(HWND hWnd, short *key);

/* map manipulation functions */
void InitMap(void);
void DrawBitMapGrid(HDC hdc);
void DeleteMaps();

void RedrawMap(NEW_VOLUME_LIST_ENTRY *v_entry);
void ClearMap();

/* dialog procedures */
BOOL CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ConfirmDlgProc(HWND, UINT, WPARAM, LPARAM);

/* status bar procedures */
BOOL CreateStatusBar();
void UpdateStatusBar(STATISTIC *pst);

/* progress bar stuff */
void InitProgress(void);
void ShowProgress(void);
void HideProgress(void);
void SetProgress(char *message, int percentage);

/* window procedures */
LRESULT CALLBACK ListWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RectWndProc(HWND, UINT, WPARAM, LPARAM);

#ifndef SHTDN_REASON_MAJOR_OTHER
#define SHTDN_REASON_MAJOR_OTHER   0x00000000
#endif
#ifndef SHTDN_REASON_MINOR_OTHER
#define SHTDN_REASON_MINOR_OTHER   0x00000000
#endif
#ifndef SHTDN_REASON_FLAG_PLANNED
#define SHTDN_REASON_FLAG_PLANNED  0x80000000
#endif
#ifndef EWX_FORCEIFHUNG
#define EWX_FORCEIFHUNG     0x00000010
#endif

#define UNDEFINED_COORD (-10000)

#endif /* _DFRG_MAIN_H_ */
