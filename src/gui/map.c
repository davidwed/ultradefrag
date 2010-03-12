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
extern NEW_VOLUME_LIST_ENTRY *processed_entry;

HDC hGridDC = NULL;
HBITMAP hGridBitmap = NULL;
WNDPROC OldRectangleWndProc;
BOOL isRectangleUnicode = FALSE;

static BOOL CreateBitMapGrid(void);
NEW_VOLUME_LIST_ENTRY * vlist_get_first_selected_entry(void);

void InitMap(void)
{
	RECT rc;
	int i;
	
	hMap = GetDlgItem(hWindow,IDC_MAP);
	/* increase hight of map */
	if(GetWindowRect(hMap,&rc)){
		rc.bottom ++;
		SetWindowPos(hMap,0,0,0,rc.right - rc.left,
			rc.bottom - rc.top,SWP_NOMOVE);
	}

	CalculateBlockSize();

	isRectangleUnicode = IsWindowUnicode(hMap);
	if(isRectangleUnicode)
		OldRectangleWndProc = (WNDPROC)SetWindowLongPtrW(hMap,GWLP_WNDPROC,
			(LONG_PTR)RectWndProc);
	else
		OldRectangleWndProc = (WNDPROC)SetWindowLongPtr(hMap,GWLP_WNDPROC,
			(LONG_PTR)RectWndProc);

	CreateBitMapGrid();
	for(i = 0; i < NUM_OF_SPACE_STATES; i++){
		/* FIXME: check for success */
		hBrushes[i] = CreateSolidBrush(colors[i]);
	}
}

LRESULT CALLBACK RectWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;

	if(iMsg == WM_PAINT){
		(void)BeginPaint(hWnd,&ps); /* (void)? */
		RedrawMap(processed_entry);
		EndPaint(hWnd,&ps);
	}
	if(isRectangleUnicode)
		return CallWindowProcW(OldRectangleWndProc,hWnd,iMsg,wParam,lParam);
	else
		return CallWindowProc(OldRectangleWndProc,hWnd,iMsg,wParam,lParam);
}

void CalculateBlockSize()
{
	RECT rc;
	double n;
	int x_edge,y_edge;
	LONG delta_x, delta_y;

	if(GetClientRect(hMap,&rc)){
		if(MapWindowPoints(hMap,hWindow,(LPPOINT)(PRECT)(&rc),(sizeof(RECT)/sizeof(POINT)))){
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
			(void)SetWindowPos(hMap,NULL,rc.left - y_edge,rc.top - x_edge, \
				iMAP_WIDTH + 2 * y_edge,iMAP_HEIGHT + 2 * x_edge,SWP_NOZORDER);
		}
	}
	(void)InvalidateRect(hMap,NULL,TRUE);
}

/* Since v3.1.0 it supports all screen color depths. */
static BOOL CreateBitMapGrid(void)
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
	(void)SelectObject(hGridDC,hGridBitmap);
	(void)SetBkMode(hGridDC,TRANSPARENT);
	
	/* draw grid */
	rc.top = rc.left = 0;
	rc.bottom = iMAP_HEIGHT;
	rc.right = iMAP_WIDTH;
	hBrush = GetStockObject(WHITE_BRUSH);
	hOldBrush = SelectObject(hGridDC,hBrush);
	(void)FillRect(hGridDC,&rc,hBrush);
	(void)SelectObject(hGridDC,hOldBrush);
	(void)DeleteObject(hBrush);

	hPen = CreatePen(PS_SOLID,1,GRID_COLOR);
	hOldPen = SelectObject(hGridDC,hPen);
	for(i = 0; i < BLOCKS_PER_HLINE + 1; i++){
		(void)MoveToEx(hGridDC,(iBLOCK_SIZE + 1) * i,0,NULL);
		(void)LineTo(hGridDC,(iBLOCK_SIZE + 1) * i,iMAP_HEIGHT);
	}
	for(i = 0; i < BLOCKS_PER_VLINE + 1; i++){
		(void)MoveToEx(hGridDC,0,(iBLOCK_SIZE + 1) * i,NULL);
		(void)LineTo(hGridDC,iMAP_WIDTH,(iBLOCK_SIZE + 1) * i);
	}
	(void)SelectObject(hGridDC,hOldPen);
	(void)DeleteObject(hPen);
	return TRUE;
}

BOOL FillBitMap(char *cluster_map,NEW_VOLUME_LIST_ENTRY *v_entry)
{
	HDC hdc;
	HBRUSH hOldBrush;
	RECT block_rc;
	int i, j;

	if(v_entry == NULL) return FALSE;

	hdc = v_entry->hdc;
	if(!hdc) return FALSE;
	
	hOldBrush = SelectObject(hdc,hBrushes[0]);
	for(i = 0; i < BLOCKS_PER_VLINE; i++){
		for(j = 0; j < BLOCKS_PER_HLINE; j++){
			block_rc.top = (iBLOCK_SIZE + 1) * i + 1;
			block_rc.left = (iBLOCK_SIZE + 1) * j + 1;
			block_rc.right = block_rc.left + iBLOCK_SIZE;
			block_rc.bottom = block_rc.top + iBLOCK_SIZE;
			(void)FillRect(hdc,&block_rc,hBrushes[(int)cluster_map[i * BLOCKS_PER_HLINE + j]]);
		}
	}
	(void)SelectObject(hdc,hOldBrush);
	return TRUE;
}

void ClearMap()
{
	HDC hdc;

	hdc = GetDC(hMap);
	(void)BitBlt(hdc,0,0,iMAP_WIDTH,iMAP_HEIGHT,hGridDC,0,0,SRCCOPY);
	(void)ReleaseDC(hMap,hdc);
}

void RedrawMap(NEW_VOLUME_LIST_ENTRY *v_entry)
{
	HDC hdc;

	if(v_entry != NULL){
		if(v_entry->status > STATUS_UNDEFINED && v_entry->hdc){
			hdc = GetDC(hMap);
			(void)BitBlt(hdc,0,0,iMAP_WIDTH,iMAP_HEIGHT,v_entry->hdc,0,0,SRCCOPY);
			(void)ReleaseDC(hMap,hdc);
			return;
		}
	}
	ClearMap();
}

void DeleteMaps()
{
	int i;

	if(hGridBitmap) (void)DeleteObject(hGridBitmap);
	if(hGridDC) (void)DeleteDC(hGridDC);
	for(i = 0; i < NUM_OF_SPACE_STATES; i++)
		(void)DeleteObject(hBrushes[i]);
}
