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
 * @brief Miscellaneous code.
 * @addtogroup Misc
 * @{
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>

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
 * @brief Sets a font for the window and all its children.
 * @param[in] hWindow handle to the window.
 * @param[in] lplf pointer to the LOGFONT structure
 * describing the font.
 * @return A handle to the font. NULL indicates failure.
 */
HFONT __stdcall WgxSetFont(HWND hWindow,LPLOGFONT lplf)
{
	HFONT hFont;
	HWND hChild;
	
	hFont = CreateFontIndirect(lplf);
	if(!hFont) return NULL;
	
	(void)SendMessage(hWindow,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
	hChild = GetWindow(hWindow,GW_CHILD);
	while(hChild){
		(void)SendMessage(hChild,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
		hChild = GetWindow(hChild,GW_HWNDNEXT);
	}

	/* redraw the main window */
	(void)InvalidateRect(hWindow,NULL,TRUE);
	(void)UpdateWindow(hWindow);
	
	return hFont;
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

/**
 * @brief Calls a Win32 ShellExecute() procedure and shows a message
 * box with detailed information in case of errors.
 */
BOOL __stdcall WgxShellExecuteW(HWND hwnd,LPCWSTR lpOperation,LPCWSTR lpFile,
                               LPCWSTR lpParameters,LPCWSTR lpDirectory,INT nShowCmd)
{
	HINSTANCE hApp;
	int error_code;
	char *error_description = "";
	short *error_msg;
	int buffer_length;
	
	hApp = ShellExecuteW(hwnd,lpOperation,lpFile,lpParameters,lpDirectory,nShowCmd);
	error_code = (int)(LONG_PTR)hApp;
	if(error_code > 32) return TRUE;
	
	/* handle errors */
	switch(error_code){
	case 0:
		error_description = "The operating system is out of memory or resources.";
		break;
	case ERROR_FILE_NOT_FOUND:
	/*case SE_ERR_FNF:*/
		error_description = "The specified file was not found.";
		break;
	case ERROR_PATH_NOT_FOUND:
	/*case SE_ERR_PNF:*/
		error_description = "The specified path was not found.";
		break;
	case ERROR_BAD_FORMAT:
		error_description = "The .exe file is invalid (non-Microsoft Win32 .exe or error in .exe image).";
		break;
	case SE_ERR_ACCESSDENIED:
		error_description = "The operating system denied access to the specified file.";
		break;
	case SE_ERR_ASSOCINCOMPLETE:
		error_description = "The file name association is incomplete or invalid.";
		break;
	case SE_ERR_DDEBUSY:
		error_description = "The Dynamic Data Exchange (DDE) transaction could not be completed\n"
							" because other DDE transactions were being processed.";
		break;
	case SE_ERR_DDEFAIL:
		error_description = "The DDE transaction failed.";
		break;
	case SE_ERR_DDETIMEOUT:
		error_description = "The DDE transaction could not be completed because the request timed out.";
		break;
	case SE_ERR_DLLNOTFOUND:
		error_description = "The specified dynamic-link library (DLL) was not found.";
		break;
	case SE_ERR_NOASSOC:
		error_description = "There is no application associated with the given file name extension.\n"
		                    "Or you attempt to print a file that is not printable.";
		break;
	case SE_ERR_OOM:
		error_description = "There was not enough memory to complete the operation.";
		break;
	case SE_ERR_SHARE:
		error_description = "A sharing violation occurred.";
		break;
	}
	
	if(!lpOperation) lpOperation = L"open";
	if(!lpFile) lpFile = L"";
	if(!lpParameters) lpParameters = L"";

	buffer_length = wcslen(lpOperation) + wcslen(lpFile) + wcslen(lpParameters);
	buffer_length += strlen(error_description);
	buffer_length += 64;

	error_msg = malloc(buffer_length * sizeof(short));
	if(error_msg == NULL){
		MessageBoxW(hwnd,L"No enough memory!",L"Error!",MB_OK | MB_ICONHAND);
		return FALSE;
	}
	
	(void)_snwprintf(error_msg,buffer_length,L"Cannot %ls %ls %ls\n%hs",
			lpOperation,lpFile,lpParameters,error_description);
	error_msg[buffer_length-1] = 0;

	MessageBoxW(hwnd,error_msg,L"Error!",MB_OK | MB_ICONHAND);
	free(error_msg);
	return FALSE;
}

/** @} */
