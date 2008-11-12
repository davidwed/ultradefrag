/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* GUI - status bar related stuff.
*/

#include "main.h"

#define N_OF_STATUSBAR_PARTS  5

extern HINSTANCE hInstance;
extern HWND hWindow;
HWND hStatus;

void SetIcon(int part,int id)
{
	HANDLE hImg;

	hImg = LoadImage(hInstance,(LPCSTR)(size_t)id,IMAGE_ICON,16,16,LR_VGACOLOR|LR_SHARED);
	SendMessage(hStatus,SB_SETICON,part,(LPARAM)hImg);
	DestroyIcon(hImg);
}

BOOL CreateStatusBar()
{
	RECT rc;
	int width, i;
	int x[N_OF_STATUSBAR_PARTS-1] = {110,210,345,465};
	int a[N_OF_STATUSBAR_PARTS];

	hStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_BORDER, \
									"0 dirs", hWindow, IDM_STATUSBAR);
	if(hStatus){
		GetClientRect(hStatus,&rc);
		width = rc.right - rc.left;
		for(i = 0; i < N_OF_STATUSBAR_PARTS-1; i++)
			a[i] = width * x[i] / 560;
		a[N_OF_STATUSBAR_PARTS-1] = width;
		SendMessage(hStatus,SB_SETPARTS,N_OF_STATUSBAR_PARTS + 1,(LPARAM)a);
		SetIcon(0,IDI_DIR); SetIcon(1,IDI_UNFRAGM);
		SetIcon(2,IDI_FRAGM); SetIcon(3,IDI_CMP);
		SetIcon(4,IDI_MFT);
		return TRUE;
	}
	return FALSE;
}

void UpdateStatusBar(STATISTIC *pst)
{
	char s[32];
	#define BFSIZE 128
	short bf[BFSIZE];

	if(!hStatus) return;

	_snwprintf(bf,BFSIZE - 1,L"%lu %s",pst->dircounter,GetResourceString(L"DIRS"));
	bf[BFSIZE - 1] = 0;
	SendMessage(hStatus,SB_SETTEXTW,0,(LPARAM)bf);

	_snwprintf(bf,BFSIZE - 1,L"%lu %s",pst->filecounter,GetResourceString(L"FILES"));
	bf[BFSIZE - 1] = 0;
	SendMessage(hStatus,SB_SETTEXTW,1,(LPARAM)bf);

	_snwprintf(bf,BFSIZE - 1,L"%lu %s",pst->fragmfilecounter,GetResourceString(L"FRAGMENTED"));
	bf[BFSIZE - 1] = 0;
	SendMessage(hStatus,SB_SETTEXTW,2,(LPARAM)bf);

	_snwprintf(bf,BFSIZE - 1,L"%lu %s",pst->compressedcounter,GetResourceString(L"COMPRESSED"));
	bf[BFSIZE - 1] = 0;
	SendMessage(hStatus,SB_SETTEXTW,3,(LPARAM)bf);

	udefrag_fbsize((ULONGLONG)pst->mft_size,2,s,sizeof(s));
	_snwprintf(bf,BFSIZE - 1,L"%S %s",s,GetResourceString(L"MFT"));
	bf[BFSIZE - 1] = 0;
	SendMessage(hStatus,SB_SETTEXTW,4,(LPARAM)bf);
}
