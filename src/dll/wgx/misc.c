/*
 *  WGX - Windows GUI Extended Library.
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
 * @file misc.c
 * @brief Miscellaneous.
 * @addtogroup Misc
 * @{
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "wgx.h"

/**
 * @brief Enables a child windows.
 * @param[in] hMainWindow handle to the main window.
 * @param[in] ... the list of resource identifiers
 *                of child windows.
 * @note The list of identifiers must be terminated by zero.
 */
void __cdecl WgxEnableWindows(HANDLE hMainWindow, ...)
{
	va_list marker;
	int id;
	
	va_start(marker,hMainWindow);
	do {
		id = va_arg(marker,int);
		if(id) (void)EnableWindow(GetDlgItem(hMainWindow,id),TRUE);
	} while(id);
	va_end(marker);
}

/**
 * @brief Disables a child windows.
 * @param[in] hMainWindow handle to the main window.
 * @param[in] ... the list of resource identifiers
 *                of child windows.
 * @note The list of identifiers must be terminated by zero.
 */
void __cdecl WgxDisableWindows(HANDLE hMainWindow, ...)
{
	va_list marker;
	int id;
	
	va_start(marker,hMainWindow);
	do {
		id = va_arg(marker,int);
		if(id) (void)EnableWindow(GetDlgItem(hMainWindow,id),FALSE);
	} while(id);
	va_end(marker);
}

/**
 * @brief Sets a window icon.
 * @param[in] hInstance handle to an instance
 * of the module whose executable file contains
 * the icon to load.
 * @param[in] hWindow handle to the window.
 * @param[in] IconID the resource identifier of the icon.
 */
void __stdcall WgxSetIcon(HINSTANCE hInstance,HWND hWindow,UINT IconID)
{
	HICON hIcon;

	hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IconID));
	(void)SendMessage(hWindow,WM_SETICON,1,(LRESULT)hIcon);
	if(hIcon) (void)DeleteObject(hIcon);
}

/**
 * @brief Prevents a window to be outside the screen.
 * @param[in,out] lprc pointer to the structure 
 * containing windows coordinates.
 * @param[in] min_width width of the minimal
 * visible part of the window.
 * @param[in] min_height height of the minimal
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

/** @} */
