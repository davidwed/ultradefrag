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

#ifndef USE_WINDDK
#ifndef SetWindowLongPtr
#define SetWindowLongPtr SetWindowLong
#endif
#define LONG_PTR LONG
#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC GWL_WNDPROC
#endif
#endif

#ifndef LR_VGACOLOR
/* this constant is not defined in winuser.h on mingw */
#define LR_VGACOLOR         0x0080
#endif

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

void analyse(void);
void defragment(void);
void optimize(void);
void stop(void);

/* settings related functions */
void GetPrefs(void);
void SavePrefs(void);

/* i18n related functions */
void BuildResourceTable(void);
void DestroyResourceTable(void);
short * GetResourceString(short *id);
void SetText(HWND hWnd, short *id);

/* map manipulation functions */
void InitMap(void);
void CalculateBlockSize();
void CreateMaps();
void DeleteMaps();

BOOL CreateBitMap(int);
BOOL CreateBitMapGrid();
BOOL FillBitMap(int);

void RedrawMap();
void ClearMap();

/* dialog procedures */
BOOL CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK NewSettingsDlgProc(HWND, UINT, WPARAM, LPARAM);

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
void UpdateVolList(void);
LRESULT VolListGetSelectedItemIndex(void);
char VolListGetLetter(LRESULT iItem);
int VolListGetLetterNumber(LRESULT iItem);
int  VolListGetWorkStatus(LRESULT iItem);
void VolListUpdateStatusField(int stat,LRESULT iItem);
void VolListNotifyHandler(LPARAM lParam);
void VolListRefreshItem(LRESULT iItem);

/* window procedures */
void HandleShortcuts(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RectWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK BtnWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CheckWndProc(HWND, UINT, WPARAM, LPARAM);

/* buttons manipulation */
void InitButtons(void);
void DisableButtonsBeforeDrivesRescan(void);
void EnableButtonsAfterDrivesRescan(void);
void DisableButtonsBeforeTask(void);
void EnableButtonsAfterTask(void);

#endif /* _DFRG_MAIN_H_ */
