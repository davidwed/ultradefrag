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

/**
 * @file statbar.c
 * @brief Status bar.
 * @addtogroup StatusBar
 * @{
 */

#include "main.h"

#define N_OF_STATUSBAR_PARTS  5

extern HINSTANCE hInstance;
extern HWND hWindow;
HWND hStatus;

extern WGX_I18N_RESOURCE_ENTRY i18n_table[];

void SetIcon(int part,int id)
{
	HANDLE hImg;

	hImg = LoadImage(hInstance,(LPCSTR)(size_t)id,IMAGE_ICON,16,16,LR_VGACOLOR|LR_SHARED);
	(void)SendMessage(hStatus,SB_SETICON,part,(LPARAM)hImg);
	(void)DestroyIcon(hImg);
}

void SetStatusBarParts(void)
{
	RECT rc;
	int width, i;
	int x[N_OF_STATUSBAR_PARTS-1] = {110,210,345,465};
	int a[N_OF_STATUSBAR_PARTS];

	if(hStatus){
		if(GetClientRect(hStatus,&rc)){
			width = rc.right - rc.left;
			for(i = 0; i < N_OF_STATUSBAR_PARTS-1; i++)
				a[i] = width * x[i] / 560;
			a[N_OF_STATUSBAR_PARTS-1] = width;
			(void)SendMessage(hStatus,SB_SETPARTS,N_OF_STATUSBAR_PARTS + 1,(LPARAM)a);
		}
		(void)SetIcon(0,IDI_DIR); (void)SetIcon(1,IDI_UNFRAGM);
		(void)SetIcon(2,IDI_FRAGM); (void)SetIcon(3,IDI_CMP);
		(void)SetIcon(4,IDI_MFT);
	}
}

BOOL CreateStatusBar()
{
	hStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_BORDER, \
									"0 dirs", hWindow, IDM_STATUSBAR);
	SetStatusBarParts();
	return (hStatus) ? TRUE : FALSE;
}

void UpdateStatusBar(udefrag_progress_info *pi)
{
	char s[32];
	#define BFSIZE 128
	short bf[BFSIZE];

	if(!hStatus) return;

	(void)_snwprintf(bf,BFSIZE - 1,L"%lu %s",pi->directories,
			WgxGetResourceString(i18n_table,L"DIRS"));
	bf[BFSIZE - 1] = 0;
	(void)SendMessage(hStatus,SB_SETTEXTW,0,(LPARAM)bf);

	(void)_snwprintf(bf,BFSIZE - 1,L"%lu %s",pi->files,
			WgxGetResourceString(i18n_table,L"FILES"));
	bf[BFSIZE - 1] = 0;
	(void)SendMessage(hStatus,SB_SETTEXTW,1,(LPARAM)bf);

	(void)_snwprintf(bf,BFSIZE - 1,L"%lu %s",pi->fragmented,
			WgxGetResourceString(i18n_table,L"FRAGMENTED"));
	bf[BFSIZE - 1] = 0;
	(void)SendMessage(hStatus,SB_SETTEXTW,2,(LPARAM)bf);

	(void)_snwprintf(bf,BFSIZE - 1,L"%lu %s",pi->compressed,
			WgxGetResourceString(i18n_table,L"COMPRESSED"));
	bf[BFSIZE - 1] = 0;
	(void)SendMessage(hStatus,SB_SETTEXTW,3,(LPARAM)bf);

	(void)udefrag_fbsize(pi->mft_size,2,s,sizeof(s));
	(void)_snwprintf(bf,BFSIZE - 1,L"%S %s",s,
			WgxGetResourceString(i18n_table,L"MFT"));
	bf[BFSIZE - 1] = 0;
	(void)SendMessage(hStatus,SB_SETTEXTW,4,(LPARAM)bf);
}

/** @} */
