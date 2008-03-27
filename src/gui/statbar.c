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
	int x[N_OF_STATUSBAR_PARTS-1] = {100,200,320,440};
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
	short bf[128];
	short tmp_bf[64];

	if(!hStatus) return;

	if(!GetResourceString(L"DIRS",tmp_bf,sizeof(tmp_bf) / sizeof(short)))
		_snwprintf(bf,sizeof(bf) / sizeof(short) - 1,L"%lu %s",pst->dircounter,tmp_bf);
	else
		_snwprintf(bf,sizeof(bf) / sizeof(short) - 1,L"%lu dirs",pst->dircounter);
	bf[sizeof(bf) / sizeof(short) - 1] = 0;
	SendMessage(hStatus,SB_SETTEXTW,0,(LPARAM)bf);

	if(!GetResourceString(L"FILES",tmp_bf,sizeof(tmp_bf)))
		_snwprintf(bf,sizeof(bf) / sizeof(short) - 1,L"%lu %s",pst->filecounter,tmp_bf);
	else
		_snwprintf(bf,sizeof(bf) / sizeof(short) - 1,L"%lu files",pst->filecounter);
	bf[sizeof(bf) / sizeof(short) - 1] = 0;
	SendMessage(hStatus,SB_SETTEXTW,1,(LPARAM)bf);

	if(!GetResourceString(L"FRAGMENTED",tmp_bf,sizeof(tmp_bf) / sizeof(short)))
		_snwprintf(bf,sizeof(bf) / sizeof(short) - 1,L"%lu %s",pst->fragmfilecounter,tmp_bf);
	else
		_snwprintf(bf,sizeof(bf) / sizeof(short) - 1,L"%lu fragmented",pst->fragmfilecounter);
	bf[sizeof(bf) / sizeof(short) - 1] = 0;
	SendMessage(hStatus,SB_SETTEXTW,2,(LPARAM)bf);

	if(!GetResourceString(L"COMPRESSED",tmp_bf,sizeof(tmp_bf) / sizeof(short)))
		_snwprintf(bf,sizeof(bf) / sizeof(short) - 1,L"%lu %s",pst->compressedcounter,tmp_bf);
	else
		_snwprintf(bf,sizeof(bf) / sizeof(short) - 1,L"%lu compressed",pst->compressedcounter);
	bf[sizeof(bf) / sizeof(short) - 1] = 0;
	SendMessage(hStatus,SB_SETTEXTW,3,(LPARAM)bf);

	fbsize(s,(ULONGLONG)pst->mft_size);
	if(!GetResourceString(L"MFT",tmp_bf,sizeof(tmp_bf) / sizeof(short)))
		_snwprintf(bf,sizeof(bf) / sizeof(short) - 1,L"%S %s",s,tmp_bf);
	else
		_snwprintf(bf,sizeof(bf) / sizeof(short) - 1,L"%S MFT",s);
	bf[sizeof(bf) / sizeof(short) - 1] = 0;
	SendMessage(hStatus,SB_SETTEXTW,4,(LPARAM)bf);
}
