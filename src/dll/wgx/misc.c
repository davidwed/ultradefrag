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
