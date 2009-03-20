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
* Miscellaneous routines.
*/

#define WIN32_NO_STATUS
#include <windows.h>

#include "wgx.h"

/*
* Enables specified child windows.
* The list of identifiers must be terminated by zero.
*/
void __cdecl WgxEnableWindows(HANDLE hMainWindow, ...)
{
	va_list marker;
	int id;
	
	va_start(marker,hMainWindow);
	do {
		id = va_arg(marker,int);
		if(id) EnableWindow(GetDlgItem(hMainWindow,id),TRUE);
	} while(id);
	va_end(marker);
}

/*
* Disables specified child windows.
* The list of identifiers must be terminated by zero.
*/
void __cdecl WgxDisableWindows(HANDLE hMainWindow, ...)
{
	va_list marker;
	int id;
	
	va_start(marker,hMainWindow);
	do {
		id = va_arg(marker,int);
		if(id) EnableWindow(GetDlgItem(hMainWindow,id),FALSE);
	} while(id);
	va_end(marker);
}

void __stdcall WgxSetIcon(HINSTANCE hInstance,HWND hWindow,UINT IconID)
{
	HICON hIcon;

	hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IconID));
	SendMessage(hWindow,WM_SETICON,1,(LRESULT)hIcon);
	if(hIcon) DeleteObject(hIcon);
}

/* Sets font for specified window and all children. */
HFONT __stdcall WgxSetFont(HWND hWindow,LPLOGFONT lplf)
{
	HFONT hFont;
	HWND hChild;
	
	hFont = CreateFontIndirect(lplf);
	if(!hFont) return NULL;
	
	SendMessage(hWindow,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
	hChild = GetWindow(hWindow,GW_CHILD);
	while(hChild){
		SendMessage(hChild,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
		hChild = GetWindow(hChild,GW_HWNDNEXT);
	}

	/* redraw the main window */
	InvalidateRect(hWindow,NULL,TRUE);
	UpdateWindow(hWindow);
	
	return hFont;
}

/*
* Prevents the specified window to be outside the screen.
* Second and third parameters specifies the minimal size of 
* visible part of the window.
*/
void __stdcall WgxCheckWindowCoordinates(LPRECT lprc,int min_width,int min_height)
{
	int cx,cy;

	cx = GetSystemMetrics(SM_CXSCREEN);
	cy = GetSystemMetrics(SM_CYSCREEN);
	if(lprc->left < 0) lprc->left = 0; if(lprc->top < 0) lprc->top = 0;
	if(lprc->left >= (cx - min_width)) lprc->left = cx - min_width;
	if(lprc->top >= (cy - min_height)) lprc->top = cy - min_height;
}
