/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 *  GUI - cluster map processing.
 */

#include <windows.h>
#include <winioctl.h>
#include <math.h>

#include "main.h"
#include "../include/ultradfg.h"

/* #define GRID_COLOR                  RGB(106,106,106) */
#define GRID_COLOR                  RGB(0,0,0)
#define SYSTEM_COLOR                RGB(0,180,60)
#define SYSTEM_OVERLIMIT_COLOR      RGB(0,90,30)
#define FRAGM_COLOR                 RGB(255,0,0)
#define FRAGM_OVERLIMIT_COLOR       RGB(128,0,0)
#define UNFRAGM_COLOR               RGB(0,0,255)
#define UNFRAGM_OVERLIMIT_COLOR     RGB(0,0,128)
#define MFT_COLOR                   RGB(128,0,128)
#define DIR_COLOR                   RGB(255,255,0)
#define DIR_OVERLIMIT_COLOR         RGB(128,128,0)
#define COMPRESSED_COLOR            RGB(185,185,0)
#define COMPRESSED_OVERLIMIT_COLOR  RGB(93,93,0)
#define NO_CHECKED_COLOR            RGB(0,255,255)

int iMAP_WIDTH = 0x209;
int iMAP_HEIGHT = 0x8c;
int iBLOCK_SIZE = 0x9;   /* in pixels */

extern HWND hWindow,hMap,hList;
extern char letter_numbers['Z' - 'A' + 1];
extern int work_status['Z' - 'A' + 1];

char map['Z' - 'A' + 1][N_BLOCKS];

HDC bit_map_dc['Z' - 'A' + 1] = {0};
HDC bit_map_grid_dc = 0;
HBITMAP bit_map['Z' - 'A' + 1] = {0};
HBITMAP bit_map_grid = 0;

BOOL CreateBitMap(int);
void DrawBlock(HDC,char *,int,COLORREF);
BOOL FillBitMap(int);
BOOL CreateBitMapGrid();

BOOL RequestMap(HANDLE hDev,char *map,DWORD *txd,LPOVERLAPPED lpovrl)
{
	return DeviceIoControl(hDev,IOCTL_GET_CLUSTER_MAP,
			NULL,0,map,N_BLOCKS,txd,lpovrl);
}

void CalculateBlockSize()
{
	RECT _rc;
	double n;
	int x_edge,y_edge;
	LONG delta_x;

	GetWindowRect(hMap,&_rc);
	x_edge = GetSystemMetrics(SM_CXEDGE);
	y_edge = GetSystemMetrics(SM_CYEDGE);
	iMAP_WIDTH = _rc.right - _rc.left - 2 * y_edge;
	iMAP_HEIGHT = _rc.bottom - _rc.top - 2 * x_edge;
	n = (double)((iMAP_WIDTH - 1) * (iMAP_HEIGHT - 1));
	n /= (double)N_BLOCKS;
	iBLOCK_SIZE = (int)floor(sqrt(n)) - 1; /* 1 pixel for grid line */
	/* adjust map size */
	/* this is an universal solution for various DPI's */
	iMAP_WIDTH = (iBLOCK_SIZE + 1) * BLOCKS_PER_HLINE + 1 + 2 * y_edge;
	iMAP_HEIGHT = (iBLOCK_SIZE + 1) * BLOCKS_PER_VLINE + 1 + 2 * x_edge;
	delta_x = _rc.right - _rc.left - iMAP_WIDTH;
	if(delta_x > 0)
	{ /* align="center" */
		_rc.left += (delta_x >> 1);
	}
	SetWindowPos(hMap,0,_rc.left - y_edge - 1, \
		_rc.top - GetSystemMetrics(SM_CYCAPTION) - x_edge - 1, \
		iMAP_WIDTH,iMAP_HEIGHT,0);
	iMAP_WIDTH -= 2 * y_edge; /* new sizes for map without borders */
	iMAP_HEIGHT -= 2 * x_edge;
}

BOOL CreateBitMap(signed int index)
{
	BYTE *ppvBits;
	BYTE *data;
	BITMAPINFOHEADER *bh;
	HDC hDC;
	unsigned short res;
	HBITMAP hBmp;

	data = (BYTE *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, \
		iMAP_WIDTH * iMAP_HEIGHT * sizeof(RGBQUAD) + sizeof(BITMAPINFOHEADER));
	if(!data) return FALSE;
	bh = (BITMAPINFOHEADER*)data;
	hDC    = GetDC(hWindow);
	res = (unsigned short)GetDeviceCaps(hDC, BITSPIXEL);
	ReleaseDC(hWindow, hDC);

	bh->biWidth       = iMAP_WIDTH;
	bh->biHeight      = iMAP_HEIGHT;
	bh->biPlanes      = 1;
	bh->biBitCount    = res;
	bh->biClrUsed     = bh->biClrImportant = 32;
	bh->biSizeImage   = 0;
	bh->biCompression = BI_RGB;
	bh->biSize        = sizeof(BITMAPINFOHEADER);

	/* create the bitmap */
	hBmp = CreateDIBSection(0, (BITMAPINFO*)bh, \
		DIB_RGB_COLORS, &ppvBits, NULL, 0);
	if(!hBmp)
	{
		HeapFree(GetProcessHeap(),0,data);
		return FALSE;
	}

	hDC = CreateCompatibleDC(0);
	SelectObject(hDC,hBmp);
	SetBkMode(hDC,TRANSPARENT);

	if(index >= 0)
	{
		bit_map[index] = hBmp;
		bit_map_dc[index] = hDC;
	}
	else
	{
		bit_map_grid = hBmp;
		bit_map_grid_dc = hDC;
	}
	return TRUE;
}

BOOL CreateBitMapGrid()
{
	HBRUSH hBrush, hOldBrush;
	HPEN hPen, hOldPen;
	int i;
	RECT rc;

	if(!CreateBitMap(-1))
		return FALSE;
	/* draw grid */
	rc.top = rc.left = 0;
	rc.bottom = iMAP_HEIGHT;
	rc.right = iMAP_WIDTH;
	hBrush = GetStockObject(WHITE_BRUSH);
	hOldBrush = SelectObject(bit_map_grid_dc,hBrush);
	FillRect(bit_map_grid_dc,&rc,hBrush);
	SelectObject(bit_map_grid_dc,hOldBrush);
	DeleteObject(hBrush);

	hPen = CreatePen(PS_SOLID,1,GRID_COLOR);
	hOldPen = SelectObject(bit_map_grid_dc,hPen);
	for(i = 0; i < BLOCKS_PER_HLINE + 1; i++)
	{
		MoveToEx(bit_map_grid_dc,(iBLOCK_SIZE + 1) * i,0,NULL);
		LineTo(bit_map_grid_dc,(iBLOCK_SIZE + 1) * i,iMAP_HEIGHT);
	}
	for(i = 0; i < BLOCKS_PER_VLINE + 1; i++)
	{
		MoveToEx(bit_map_grid_dc,0,(iBLOCK_SIZE + 1) * i,NULL);
		LineTo(bit_map_grid_dc,iMAP_WIDTH,(iBLOCK_SIZE + 1) * i);
	}
	SelectObject(bit_map_grid_dc,hOldPen);
	DeleteObject(hPen);
	return TRUE;
}

void DrawBlock(HDC hdc,char *__map,int space_state,COLORREF color)
{
	HBRUSH hBrush, hOldBrush;
	int k,col,row;
	RECT block_rc;

	hBrush = CreateSolidBrush(color);
	hOldBrush = SelectObject(hdc,hBrush);

	for(k = 0; k < N_BLOCKS; k++)
	{
		if(__map[k] == space_state)
		{
			col = k % BLOCKS_PER_HLINE;
			row = k / BLOCKS_PER_HLINE;
			block_rc.top = (iBLOCK_SIZE + 1) * row + 1;
			block_rc.left = (iBLOCK_SIZE + 1) * col + 1;
			block_rc.right = block_rc.left + iBLOCK_SIZE;
			block_rc.bottom = block_rc.top + iBLOCK_SIZE;
			FillRect(hdc,&block_rc,hBrush);
		}
	}
	SelectObject(hdc,hOldBrush);
	DeleteObject(hBrush);
}

BOOL FillBitMap(int index)
{
	HDC hdc;
	HBRUSH hBrush, hOldBrush;
	char *_map;
	RECT rc;

	if(!(bit_map_dc[index]))
	{
		if(!CreateBitMap(index))
		{
			MessageBox(hWindow,"Fatal GDI error #1!","Error",MB_OK | MB_ICONEXCLAMATION);
			return FALSE;
		}
	}
	rc.top = rc.left = 0;
	rc.bottom = iMAP_HEIGHT;
	rc.right = iMAP_WIDTH;
	_map = map[index];
	hdc = bit_map_dc[index];
	if(!hdc)
		return FALSE;
	hBrush = CreateSolidBrush(GRID_COLOR);
	hOldBrush = SelectObject(hdc,hBrush);
	FillRect(hdc,&rc,hBrush);
	SelectObject(hdc,hOldBrush);
	DeleteObject(hBrush);
	/* Show free space */
	DrawBlock(hdc,_map,FREE_SPACE,RGB(255,255,255));
	/* Show unfragmented space */
	DrawBlock(hdc,_map,UNFRAGM_SPACE,UNFRAGM_COLOR);
	/* Show fragmented space */
	DrawBlock(hdc,_map,FRAGM_SPACE,FRAGM_COLOR);
	/* Show system space */
	DrawBlock(hdc,_map,SYSTEM_SPACE,SYSTEM_COLOR);
	/* Show MFT space */
	DrawBlock(hdc,_map,MFT_SPACE,MFT_COLOR);
	/* Show directory space */
	DrawBlock(hdc,_map,DIR_SPACE,DIR_COLOR);
	/* Show compressed space */
	DrawBlock(hdc,_map,COMPRESSED_SPACE,COMPRESSED_COLOR);
	/* Show no checked space */
	DrawBlock(hdc,_map,NO_CHECKED_SPACE,NO_CHECKED_COLOR);
	/* Show space of big files */
	DrawBlock(hdc,_map,SYSTEM_OVERLIMIT_SPACE,SYSTEM_OVERLIMIT_COLOR);
	DrawBlock(hdc,_map,FRAGM_OVERLIMIT_SPACE,FRAGM_OVERLIMIT_COLOR);
	DrawBlock(hdc,_map,UNFRAGM_OVERLIMIT_SPACE,UNFRAGM_OVERLIMIT_COLOR);
	DrawBlock(hdc,_map,DIR_OVERLIMIT_SPACE,DIR_OVERLIMIT_COLOR);
	DrawBlock(hdc,_map,COMPRESSED_OVERLIMIT_SPACE,COMPRESSED_OVERLIMIT_COLOR);
	return TRUE;
}

void ClearMap()
{
	HDC hdc;

	hdc = GetDC(hMap);
	BitBlt(hdc,0,0,iMAP_WIDTH,iMAP_HEIGHT,bit_map_grid_dc,0,0,SRCCOPY);
	ReleaseDC(hMap,hdc);
}

void RedrawMap()
{
	HDC hdc;
	LRESULT index;

	hdc = GetDC(hMap);
	index = SendMessage(hList,LB_GETCURSEL,0,0);
	if(index != LB_ERR)
	{
		index = letter_numbers[index - 2];
	}
	else
	{
		BitBlt(hdc,0,0,iMAP_WIDTH,iMAP_HEIGHT,bit_map_grid_dc,0,0,SRCCOPY);
		goto exit_redraw;
	}
	if(work_status[index] <= 1 || !bit_map_dc[index])
	{
		BitBlt(hdc,0,0,iMAP_WIDTH,iMAP_HEIGHT,bit_map_grid_dc,0,0,SRCCOPY);
		goto exit_redraw;
	}
	BitBlt(hdc,0,0,iMAP_WIDTH,iMAP_HEIGHT,bit_map_dc[index],0,0,MERGECOPY);
exit_redraw:
	ReleaseDC(hMap,hdc);
}

void DeleteMaps()
{
	int i;

	for(i = 0; i < 'Z' - 'A'; i++)
	{
		if(bit_map[i])
			DeleteObject(bit_map[i]);
		if(bit_map_dc[i])
			DeleteDC(bit_map_dc[i]);
	}
	if(bit_map_grid)
		DeleteObject(bit_map_grid);
	if(bit_map_grid_dc)
		DeleteDC(bit_map_grid_dc);
}