/*
 *  WGX - Windows GUI Extended Library.
 *  Copyright (c) 2007-2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* WGX - Windows GUI Extended Library.
* Contains functions that extend Windows Graphical API 
* to simplify GUI applications development.
*/

#ifndef _WGX_H_
#define _WGX_H_

/* -------- Definitions missing on several development systems. -------- */
#ifndef USE_WINDDK
#ifndef SetWindowLongPtr
#define SetWindowLongPtr SetWindowLong
#endif
#ifndef SetWindowLongPtrW
#define SetWindowLongPtrW SetWindowLongW
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

/* -------- WGX structures. -------- */

typedef struct _WGX_I18N_RESOURCE_ENTRY {
	int ControlID;
	short *Key;
	short *DefaultString;
	short *LoadedString;
} WGX_I18N_RESOURCE_ENTRY, *PWGX_I18N_RESOURCE_ENTRY;

/* -------- Exported functions prototypes. -------- */
BOOL __stdcall WgxAddAccelerators(HINSTANCE hInstance,HWND hWindow,UINT AccelId);

BOOL __stdcall WgxBuildResourceTable(PWGX_I18N_RESOURCE_ENTRY table,short *lng_file_path);
void __stdcall WgxApplyResourceTable(PWGX_I18N_RESOURCE_ENTRY table,HWND hWindow);
short * __stdcall WgxGetResourceString(PWGX_I18N_RESOURCE_ENTRY table,short *key);
void __stdcall WgxDestroyResourceTable(PWGX_I18N_RESOURCE_ENTRY table);

void __cdecl WgxEnableWindows(HANDLE hMainWindow, ...);
void __cdecl WgxDisableWindows(HANDLE hMainWindow, ...);
HFONT __stdcall WgxSetFont(HWND hWindow,LPLOGFONT lplf);
void __stdcall WgxSetIcon(HINSTANCE hInstance,HWND hWindow,UINT IconID);
void __stdcall WgxCheckWindowCoordinates(LPRECT lprc,int min_width,int min_height);
BOOL __stdcall WgxShellExecuteW(HWND hwnd,LPCWSTR lpOperation,LPCWSTR lpFile,
                               LPCWSTR lpParameters,LPCWSTR lpDirectory,INT nShowCmd);

BOOL __stdcall WgxGetLogFontStructureFromFile(char *path,LOGFONT *lf);
BOOL __stdcall WgxSaveLogFontStructureToFile(char *path,LOGFONT *lf);

BOOL __stdcall IncreaseWebAnalyticsCounter(char *url);
void __stdcall IncreaseWebAnalyticsCounterAsynch(char *url);
#endif /* _WGX_H_ */
