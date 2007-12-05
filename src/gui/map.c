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
#include <commctrl.h>
#include <math.h>

//#include <stdio.h>

#include "main.h"
#include "../include/udefrag.h"
#include "../include/ultradfg.h"

#define GRID_COLOR                  RGB(0,0,0)

COLORREF colors[NUM_OF_SPACE_STATES] = 
{
	RGB(255,255,255),              /* free */
	RGB(0,180,60),RGB(0,90,30),    /* system */
	RGB(255,0,0),RGB(128,0,0),     /* fragmented */
	RGB(0,0,255),RGB(0,0,128),     /* unfragmented */
	RGB(128,0,128),                /* mft */
	RGB(255,255,0),RGB(128,128,0), /* directories */
	RGB(185,185,0),RGB(93,93,0),   /* compressed */
	RGB(0,255,255)                 /* no checked space */
};
HBRUSH hBrushes[NUM_OF_SPACE_STATES];

int iMAP_WIDTH  = 0x209;
int iMAP_HEIGHT = 0x8c;
int iBLOCK_SIZE = 0x9;   /* in pixels */

extern HWND hWindow,hMap,hList;
extern char letter_numbers[];
extern int work_status[];

char map[MAX_DOS_DRIVES][N_BLOCKS];

HDC bit_map_dc[MAX_DOS_DRIVES] = {0};
HDC bit_map_grid_dc = 0;
HBITMAP bit_map[MAX_DOS_DRIVES] = {0};
HBITMAP bit_map_grid = 0;

BOOL CreateBitMap(int);
BOOL FillBitMap(int);
BOOL CreateBitMapGrid();
void ClearMap();

void CalculateBlockSize()
{
	RECT rc;
	double n;
	int x_edge,y_edge;
	LONG delta_x, delta_y;

	GetClientRect(hMap,&rc);
	MapWindowPoints(hMap,hWindow,(LPPOINT)(PRECT)(&rc),(sizeof(RECT)/sizeof(POINT)));
	x_edge = GetSystemMetrics(SM_CXEDGE);
	y_edge = GetSystemMetrics(SM_CYEDGE);
	iMAP_WIDTH = rc.right - rc.left;// - 2 * y_edge;
	iMAP_HEIGHT = rc.bottom - rc.top;// - 2 * x_edge;
///iMAP_WIDTH -= 20;
	n = (double)((iMAP_WIDTH - 1) * (iMAP_HEIGHT - 1));
	n /= (double)N_BLOCKS;
	iBLOCK_SIZE = (int)floor(sqrt(n)) - 1; /* 1 pixel for grid line */
	/* adjust map size */
	/* this is an universal solution for various DPI's */
	iMAP_WIDTH = (iBLOCK_SIZE + 1) * BLOCKS_PER_HLINE + 1;// + 2 * y_edge;
	iMAP_HEIGHT = (iBLOCK_SIZE + 1) * BLOCKS_PER_VLINE + 1;// + 2 * x_edge;
	delta_x = rc.right - rc.left - iMAP_WIDTH;
	delta_y = rc.bottom - rc.top - iMAP_HEIGHT;
	if(delta_x > 0)
	{ /* align="center" */
		rc.left += (delta_x >> 1);
	}
	if(delta_y > 0) rc.top += (delta_y >> 1);
	SetWindowPos(hMap,NULL,rc.left/* - y_edge - 1*/, \
		rc.top/* - GetSystemMetrics(SM_CYCAPTION) - x_edge - 1*/, \
		iMAP_WIDTH + 2 * y_edge,iMAP_HEIGHT + 2 * x_edge,SWP_NOZORDER);
	InvalidateRect(hMap,NULL,TRUE);
//	iMAP_WIDTH -= 2 * y_edge; /* new sizes for map without borders */
//	iMAP_HEIGHT -= 2 * x_edge;
}

void CreateMaps(void)
{
	int i;

	for(i = 0; i < MAX_DOS_DRIVES; i++)
		CreateBitMap(i);
	for(i = 0; i < NUM_OF_SPACE_STATES; i++)
		hBrushes[i] = CreateSolidBrush(colors[i]);
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
	hDC = GetDC(hWindow);
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

BOOL FillBitMap(int index)
{
	HDC hdc;
	char *cl_map;
	int i, j;
	HBRUSH hOldBrush;
	RECT block_rc;

	if(!(bit_map_dc[index])) return FALSE;
	cl_map = map[index];
	hdc = bit_map_dc[index];
	if(!hdc) return FALSE;
	hOldBrush = SelectObject(hdc,hBrushes[0]);
	for(i = 0; i < BLOCKS_PER_VLINE; i++)
	{
		for(j = 0; j < BLOCKS_PER_HLINE; j++)
		{
			block_rc.top = (iBLOCK_SIZE + 1) * i + 1;
			block_rc.left = (iBLOCK_SIZE + 1) * j + 1;
			block_rc.right = block_rc.left + iBLOCK_SIZE;
			block_rc.bottom = block_rc.top + iBLOCK_SIZE;
			FillRect(hdc,&block_rc,hBrushes[cl_map[i * BLOCKS_PER_HLINE + j]]);
		}
	}
	SelectObject(hdc,hOldBrush);
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
	LRESULT iItem, index;

	iItem = SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
	if(iItem != -1)
	{
		index = letter_numbers[iItem];
		if(work_status[index] > 1 && bit_map_dc[index])
		{
			hdc = GetDC(hMap);
			BitBlt(hdc,0,0,iMAP_WIDTH,iMAP_HEIGHT,bit_map_dc[index],0,0,SRCCOPY);
			ReleaseDC(hMap,hdc);
			return;
		}
	}
	ClearMap();
}

void DeleteMaps()
{
	int i;

	for(i = 0; i < 'Z' - 'A'; i++)
	{
		if(bit_map[i]) DeleteObject(bit_map[i]);
		if(bit_map_dc[i]) DeleteDC(bit_map_dc[i]);
	}
	if(bit_map_grid) DeleteObject(bit_map_grid);
	if(bit_map_grid_dc) DeleteDC(bit_map_grid_dc);
	for(i = 0; i < NUM_OF_SPACE_STATES; i++)
		DeleteObject(hBrushes[i]);
}
