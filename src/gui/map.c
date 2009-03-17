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
* GUI - cluster map processing.
*/

#include "main.h"

#define GRID_COLOR                  RGB(0,0,0)

COLORREF colors[NUM_OF_SPACE_STATES] = 
{
	RGB(255,255,255),              /* free */
	RGB(0,180,60),RGB(0,90,30),    /* system */
	RGB(255,0,0),RGB(128,0,0),     /* fragmented */
	RGB(0,0,255),RGB(0,0,128),     /* unfragmented */
	RGB(128,0,128),                /* mft */
	RGB(255,255,0),RGB(238,221,0), /* directories */
	RGB(185,185,0),RGB(93,93,0),   /* compressed */
	RGB(0,255,255)                 /* no checked space */
};
HBRUSH hBrushes[NUM_OF_SPACE_STATES];

int iMAP_WIDTH  = 0x209;
int iMAP_HEIGHT = 0x8c;
int iBLOCK_SIZE = 0x9;   /* in pixels */

extern HWND hWindow,hMap,hList;

HDC hGridDC = NULL;
HBITMAP hGridBitmap = NULL;
WNDPROC OldRectangleWndProc;

void InitMap(void)
{
	RECT rc;
	
	hMap = GetDlgItem(hWindow,IDC_MAP);
	/* increase hight of map */
	GetWindowRect(hMap,&rc);
	rc.bottom ++;
	SetWindowPos(hMap,0,0,0,rc.right - rc.left,
		rc.bottom - rc.top,SWP_NOMOVE);
	CalculateBlockSize();
	OldRectangleWndProc = (WNDPROC)SetWindowLongPtr(hMap,GWLP_WNDPROC,(LONG_PTR)RectWndProc);
}

LRESULT CALLBACK RectWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;

	if(iMsg == WM_PAINT){
		BeginPaint(hWnd,&ps);
		RedrawMap();
		EndPaint(hWnd,&ps);
	}
	return CallWindowProc(OldRectangleWndProc,hWnd,iMsg,wParam,lParam);
}

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
	iMAP_WIDTH = rc.right - rc.left;
	iMAP_HEIGHT = rc.bottom - rc.top;
	n = (double)((iMAP_WIDTH - 1) * (iMAP_HEIGHT - 1));
	n /= (double)N_BLOCKS;
	iBLOCK_SIZE = (int)floor(sqrt(n)) - 1; /* 1 pixel for grid line */
	/* adjust map size */
	/* this is an universal solution for various DPI's */
	iMAP_WIDTH = (iBLOCK_SIZE + 1) * BLOCKS_PER_HLINE + 1;
	iMAP_HEIGHT = (iBLOCK_SIZE + 1) * BLOCKS_PER_VLINE + 1;
	delta_x = rc.right - rc.left - iMAP_WIDTH;
	delta_y = rc.bottom - rc.top - iMAP_HEIGHT;
	/* align="center" */
	if(delta_x > 0)	rc.left += (delta_x >> 1);
	if(delta_y > 0) rc.top += (delta_y >> 1);
	/* border width is used because window size = client size + borders */
	SetWindowPos(hMap,NULL,rc.left - y_edge,rc.top - x_edge, \
		iMAP_WIDTH + 2 * y_edge,iMAP_HEIGHT + 2 * x_edge,SWP_NOZORDER);
	InvalidateRect(hMap,NULL,TRUE);
}

void CreateMaps(void)
{
	int i;

	CreateBitMapGrid();
	for(i = 0; i < NUM_OF_SPACE_STATES; i++)
		hBrushes[i] = CreateSolidBrush(colors[i]);
}

/* Since v3.1.0 it supports all screen color depths. */
BOOL CreateBitMapGrid()
{
	HDC hMainDC;
	HBRUSH hBrush, hOldBrush;
	HPEN hPen, hOldPen;
	RECT rc;
	int i;

	hMainDC = GetDC(hWindow);
	hGridDC = CreateCompatibleDC(hMainDC);
	hGridBitmap = CreateCompatibleBitmap(hMainDC,iMAP_WIDTH,iMAP_HEIGHT);
	ReleaseDC(hWindow,hMainDC);
	if(!hGridBitmap) { DeleteDC(hGridDC); hGridDC = NULL; return FALSE; }
	SelectObject(hGridDC,hGridBitmap);
	SetBkMode(hGridDC,TRANSPARENT);
	
	/* draw grid */
	rc.top = rc.left = 0;
	rc.bottom = iMAP_HEIGHT;
	rc.right = iMAP_WIDTH;
	hBrush = GetStockObject(WHITE_BRUSH);
	hOldBrush = SelectObject(hGridDC,hBrush);
	FillRect(hGridDC,&rc,hBrush);
	SelectObject(hGridDC,hOldBrush);
	DeleteObject(hBrush);

	hPen = CreatePen(PS_SOLID,1,GRID_COLOR);
	hOldPen = SelectObject(hGridDC,hPen);
	for(i = 0; i < BLOCKS_PER_HLINE + 1; i++){
		MoveToEx(hGridDC,(iBLOCK_SIZE + 1) * i,0,NULL);
		LineTo(hGridDC,(iBLOCK_SIZE + 1) * i,iMAP_HEIGHT);
	}
	for(i = 0; i < BLOCKS_PER_VLINE + 1; i++){
		MoveToEx(hGridDC,0,(iBLOCK_SIZE + 1) * i,NULL);
		LineTo(hGridDC,iMAP_WIDTH,(iBLOCK_SIZE + 1) * i);
	}
	SelectObject(hGridDC,hOldPen);
	DeleteObject(hPen);
	return TRUE;
}

BOOL FillBitMap(char *cluster_map)
{
	PVOLUME_LIST_ENTRY vl;
	HDC hdc;
	HBRUSH hOldBrush;
	RECT block_rc;
	int i, j;

	vl = VolListGetSelectedEntry();
	if(vl->VolumeName == NULL) return FALSE;

	hdc = vl->hDC;
	if(!hdc) return FALSE;
	
	hOldBrush = SelectObject(hdc,hBrushes[0]);
	for(i = 0; i < BLOCKS_PER_VLINE; i++){
		for(j = 0; j < BLOCKS_PER_HLINE; j++){
			block_rc.top = (iBLOCK_SIZE + 1) * i + 1;
			block_rc.left = (iBLOCK_SIZE + 1) * j + 1;
			block_rc.right = block_rc.left + iBLOCK_SIZE;
			block_rc.bottom = block_rc.top + iBLOCK_SIZE;
			FillRect(hdc,&block_rc,hBrushes[(int)cluster_map[i * BLOCKS_PER_HLINE + j]]);
		}
	}
	SelectObject(hdc,hOldBrush);
	return TRUE;
}

void ClearMap()
{
	HDC hdc;

	hdc = GetDC(hMap);
	BitBlt(hdc,0,0,iMAP_WIDTH,iMAP_HEIGHT,hGridDC,0,0,SRCCOPY);
	ReleaseDC(hMap,hdc);
}

void RedrawMap()
{
	PVOLUME_LIST_ENTRY vl;
	HDC hdc;

	vl = VolListGetSelectedEntry();
	if(vl->VolumeName != NULL){
		if(vl->Status > 1 && vl->hDC){
			hdc = GetDC(hMap);
			BitBlt(hdc,0,0,iMAP_WIDTH,iMAP_HEIGHT,vl->hDC,0,0,SRCCOPY);
			ReleaseDC(hMap,hdc);
			return;
		}
	}
	ClearMap();
}

void DeleteMaps()
{
	int i;

	if(hGridBitmap) DeleteObject(hGridBitmap);
	if(hGridDC) DeleteDC(hGridDC);
	for(i = 0; i < NUM_OF_SPACE_STATES; i++)
		DeleteObject(hBrushes[i]);
}
