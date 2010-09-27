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

#ifndef _UDEFRAG_GUI_MAIN_H_
#define _UDEFRAG_GUI_MAIN_H_

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
#include <shellapi.h>

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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <ctype.h>
#include <wchar.h>
#include <process.h>

#define lua_c
#include "../lua5.1/lua.h"
#include "../lua5.1/lauxlib.h"
#include "../lua5.1/lualib.h"

#include "../dll/wgx/wgx.h"

#include "../include/udefrag.h"
#include "../include/ultradfgver.h"

#include "resource.h"

/*
* An article of Mumtaz Zaheer from Pakistan helped me very much
* to make a valid subclassing:
* http://www.codeproject.com/KB/winsdk/safesubclassing.aspx
*/

typedef struct _udefrag_map {
	HBITMAP hbitmap;      /* bitmap used to draw map on */
	HDC hdc;              /* device context of the bitmap */
	int width;            /* width of the map, in pixels */
	int height;           /* height of the map, in pixels */
	char *buffer;         /* internal map representation */
	int size;             /* size of internal representation, in bytes */
	char *scaled_buffer;  /* scaled map representation */
	int scaled_size;      /* size of scaled representation, in bytes */
} udefrag_map;

typedef struct _volume_processing_job {
	char volume_letter;
	udefrag_job_type job_type;
	int termination_flag;
	udefrag_progress_info pi;
	udefrag_map map;
} volume_processing_job;

/* a type of the job being never executed */
#define NEVER_EXECUTED_JOB 0x100

/* prototypes */
int init_jobs(void);
volume_processing_job *get_job(char volume_letter);
void release_jobs(void);



#define DEFAULT_MAP_BLOCK_SIZE  4 //84
#define DEFAULT_GRID_LINE_WIDTH 1

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

#define create_thread(func,param,ph) \
		CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)func,(void *)param,0,ph)

void DisplayLastError(char *caption);

void DoJob(udefrag_job_type job_type);
void stop(void);

/* settings related functions */
void GetPrefs(void);
void SavePrefs(void);

void CheckForTheNewVersion(void);

/* i18n related functions */
void SetText(HWND hWnd, short *key);

/* map manipulation functions */
void InitMap(void);
void DeleteMaps();

void RedrawMap(volume_processing_job *job);

/* dialog procedures */
BOOL CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ConfirmDlgProc(HWND, UINT, WPARAM, LPARAM);

/* status bar procedures */
BOOL CreateStatusBar();
void UpdateStatusBar(udefrag_progress_info *pi);

/* progress bar stuff */
void InitProgress(void);
void ShowProgress(void);
void HideProgress(void);
void SetProgress(wchar_t *message, int percentage);

/* window procedures */
LRESULT CALLBACK ListWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RectWndProc(HWND, UINT, WPARAM, LPARAM);

#define UNDEFINED_COORD (-10000)

/* This macro converts pixels from 96 DPI to the current one. */
#define PIX_PER_DIALOG_UNIT_96DPI 1.74
#define DPI(x) ((int)((double)x * pix_per_dialog_unit / PIX_PER_DIALOG_UNIT_96DPI))

#endif /* _UDEFRAG_GUI_MAIN_H_ */
