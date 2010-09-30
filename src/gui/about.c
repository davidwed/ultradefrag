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

/*
* GUI - about box source code.
*/

#include "main.h"

extern HINSTANCE hInstance;
extern HWND hWindow;
extern HFONT hFont;
extern WGX_I18N_RESOURCE_ENTRY i18n_table[];

BOOL CALLBACK AboutDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	HWND hChild;

	switch(msg){
	case WM_INITDIALOG:
		/* Window Initialization */
		WgxCenterWindow(hWnd);
		WgxSetText(hWnd,i18n_table,L"ABOUT_WIN_TITLE");
		WgxSetText(GetDlgItem(hWnd,IDC_CREDITS),i18n_table,L"CREDITS");
		WgxSetText(GetDlgItem(hWnd,IDC_LICENSE),i18n_table,L"LICENSE");
		if(hFont){
			(void)SendMessage(hWnd,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
			hChild = GetWindow(hWnd,GW_CHILD);
			while(hChild){
				(void)SendMessage(hChild,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
				hChild = GetWindow(hChild,GW_HWNDNEXT);
			}
		}
		(void)WgxAddAccelerators(hInstance,hWnd,IDR_ACCELERATOR2);
		return FALSE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_CREDITS:
			(void)WgxShellExecuteW(hWindow,L"open",L".\\CREDITS.TXT",NULL,NULL,SW_SHOW);
			break;
		case IDC_LICENSE:
			(void)WgxShellExecuteW(hWindow,L"open",L".\\LICENSE.TXT",NULL,NULL,SW_SHOW);
			break;
		case IDC_HOMEPAGE:
			(void)SetFocus(GetDlgItem(hWnd,IDC_CREDITS));
			(void)WgxShellExecuteW(hWindow,L"open",L"http://ultradefrag.sourceforge.net",NULL,NULL,SW_SHOW);
		}
		if(LOWORD(wParam) != IDOK)
			return FALSE;
	case WM_CLOSE:
		(void)EndDialog(hWnd,1);
		return TRUE;
	}
	return FALSE;
}
