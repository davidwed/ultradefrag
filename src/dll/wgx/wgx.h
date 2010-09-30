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

/* definitions missing on several development systems */
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

/* wgx structures */
typedef struct _WGX_I18N_RESOURCE_ENTRY {
	int ControlID;
	short *Key;
	short *DefaultString;
	short *LoadedString;
} WGX_I18N_RESOURCE_ENTRY, *PWGX_I18N_RESOURCE_ENTRY;

/* wgx routines prototypes */
BOOL __stdcall WgxAddAccelerators(HINSTANCE hInstance,HWND hWindow,UINT AccelId);

/* lines in language files are limited by 8191 characters, which is more than enough */
BOOL __stdcall WgxBuildResourceTable(PWGX_I18N_RESOURCE_ENTRY table,short *lng_file_path);
void __stdcall WgxApplyResourceTable(PWGX_I18N_RESOURCE_ENTRY table,HWND hWindow);
void __stdcall WgxSetText(HWND hWnd, PWGX_I18N_RESOURCE_ENTRY table, short *key);
short * __stdcall WgxGetResourceString(PWGX_I18N_RESOURCE_ENTRY table,short *key);
void __stdcall WgxDestroyResourceTable(PWGX_I18N_RESOURCE_ENTRY table);

void __cdecl WgxEnableWindows(HANDLE hMainWindow, ...);
void __cdecl WgxDisableWindows(HANDLE hMainWindow, ...);
void __stdcall WgxSetIcon(HINSTANCE hInstance,HWND hWindow,UINT IconID);
void __stdcall WgxCheckWindowCoordinates(LPRECT lprc,int min_width,int min_height);
void __stdcall WgxCenterWindow(HWND hwnd);
WNDPROC __stdcall WgxSafeSubclassWindow(HWND hwnd,WNDPROC NewProc);
LRESULT __stdcall WgxSafeCallWndProc(WNDPROC OldProc,HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

BOOL __stdcall WgxShellExecuteW(HWND hwnd,LPCWSTR lpOperation,LPCWSTR lpFile,
                               LPCWSTR lpParameters,LPCWSTR lpDirectory,INT nShowCmd);

BOOL __stdcall WgxGetLogFontStructureFromFile(char *path,LOGFONT *lf);
BOOL __stdcall WgxSaveLogFontStructureToFile(char *path,LOGFONT *lf);
HFONT __stdcall WgxSetFont(HWND hWindow,LPLOGFONT lplf);

BOOL __stdcall IncreaseGoogleAnalyticsCounter(char *hostname,char *path,char *account);
void __stdcall IncreaseGoogleAnalyticsCounterAsynch(char *hostname,char *path,char *account);

void __cdecl WgxDbgPrint(char *format, ...);
void __cdecl WgxDbgPrintLastError(char *format, ...);
int  __cdecl WgxDisplayLastError(HWND hParent,UINT msgbox_flags, char *format, ...);

#endif /* _WGX_H_ */
