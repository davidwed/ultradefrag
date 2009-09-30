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
 *  GUI - common definitions.
 */

#ifndef _DFRG_MAIN_H_
#define _DFRG_MAIN_H_

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
#include "../include/ultradfg.h"
#include "resource.h"

#include "../dll/wgx/wgx.h"

/* application defined constants */
#define BLOCKS_PER_HLINE  65 /*52*/ /*60*/
#define BLOCKS_PER_VLINE  14 /*16*/
#define N_BLOCKS          (BLOCKS_PER_HLINE * BLOCKS_PER_VLINE)

#define STAT_CLEAR	0
//#define STAT_WORK	1
#define STAT_AN		2
#define STAT_DFRG	3

#define create_thread(func,param,ph) \
		CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)func,(void *)param,0,ph)

typedef struct _VOLUME_LIST_ENTRY {
	char *VolumeName;
	int Status;
	STATISTIC Statistics;
	HDC hDC;
	HBITMAP hBitmap;
} VOLUME_LIST_ENTRY, *PVOLUME_LIST_ENTRY;

		
void analyse(void);
void defragment(void);
void optimize(void);
void stop(void);

/* settings related functions */
void GetPrefs(void);
void SavePrefs(void);

/* i18n related functions */
void SetText(HWND hWnd, short *key);

/* map manipulation functions */
void InitMap(void);
void CalculateBlockSize();
void DeleteMaps();

BOOL FillBitMap(char *);

void RedrawMap();
void ClearMap();

/* dialog procedures */
BOOL CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);

/* status bar procedures */
BOOL CreateStatusBar();
void UpdateStatusBar(STATISTIC *pst);

/* progress bar stuff */
void InitProgress(void);
void ShowProgress(void);
void HideProgress(void);
void SetProgress(char *message, int percentage);

/* volume list manipulations */
void InitVolList(void);
void FreeVolListResources(void);
void UpdateVolList(void);
PVOLUME_LIST_ENTRY VolListGetSelectedEntry(void);
void VolListUpdateSelectedStatusField(int Status);
void VolListRefreshSelectedItem(void);
void VolListNotifyHandler(LPARAM lParam);

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

#endif /* _DFRG_MAIN_H_ */
