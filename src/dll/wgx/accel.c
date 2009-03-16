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
* GUI Accelerators related routines.
*/

#define WIN32_NO_STATUS
#include <windows.h>

#include "wgx.h"

#define WIN_ARRAY_SIZE 1024

typedef struct _CHILD_WINDOW {
	HWND hWindow;
	WNDPROC OldWindowProcedure;
	HACCEL hAccelerator;
	HWND hMainWindow;
} CHILD_WINDOW, *PCHILD_WINDOW;

CHILD_WINDOW win[WIN_ARRAY_SIZE];
int idx;
BOOL first_call = TRUE;
BOOL error_flag = FALSE;

LRESULT CALLBACK NewWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	int i, found = 0;
	MSG message;
	
	/* search for our window in win array */
	for(i = 0; i <= idx; i++){
		if(win[i].hWindow == hWnd){
			found = 1; break;
		}
	}
	
	if(found){
		if(iMsg == WM_KEYDOWN){
			message.hwnd = hWnd;
			message.message = iMsg;
			message.wParam = wParam;
			message.lParam = lParam;
			message.pt.x = message.pt.y = 0;
			message.time = 0;
			TranslateAccelerator(win[i].hMainWindow,win[i].hAccelerator,&message);
		}
		return CallWindowProc(win[i].OldWindowProcedure,hWnd,iMsg,wParam,lParam);
	}
	/* very extraordinary situation */
	if(!error_flag){ /* show message box once */
		MessageBox(NULL,"OldWindowProcedure is lost!","Error!",MB_OK | MB_ICONHAND);
		error_flag = TRUE;
	}
	return 0;
}

/*
* Loads the specified accelerator table and adds them to the specified 
* window and all its children.
*/
BOOL __stdcall WgxAddAccelerators(HINSTANCE hInstance,HWND hWindow,UINT AccelId)
{
	HACCEL hAccel;
	HANDLE hChild;
	WNDPROC OldWndProc;
	
	/* Load the accelerator table. */
	hAccel = LoadAccelerators(hInstance,MAKEINTRESOURCE(AccelId));
	if(!hAccel) return FALSE;

	if(first_call){
		memset(win,0,sizeof(win));
		idx = 0;
		first_call = FALSE;
	}
	
	/* Set accelerator for the main window. */
	/* FIXME: decide is it neccessary or not. */
	
	/* Set accelerator for children. */
	hChild = GetWindow(hWindow,GW_CHILD);
	while(hChild){
		if(idx >= (WIN_ARRAY_SIZE - 1))
			return FALSE; /* too many child windows */
		OldWndProc = (WNDPROC)SetWindowLongPtr(hChild,GWLP_WNDPROC,(LONG_PTR)NewWndProc);
		if(OldWndProc){
			win[idx].hWindow = hChild;
			win[idx].OldWindowProcedure = OldWndProc;
			win[idx].hAccelerator = hAccel;
			win[idx].hMainWindow = hWindow;
			idx ++;
		}
		hChild = GetWindow(hChild,GW_HWNDNEXT);
	}
	
	return TRUE;
}
