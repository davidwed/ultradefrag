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

int map_block_size = DEFAULT_MAP_BLOCK_SIZE;

int map_blocks_per_line = 145; //65
int map_lines = 32; //14
int map_width = 0x209;
int map_height = 0x8c;

char *global_cluster_map = NULL;

COLORREF grid_color = RGB(0,0,0); //RGB(200,200,200)

COLORREF colors[NUM_OF_SPACE_STATES] = 
{
	RGB(255,255,255),              /* free */
	RGB(0,180,60),RGB(0,90,30),    /* system */
	RGB(255,0,0),RGB(128,0,0),     /* fragmented */
	RGB(0,0,255),RGB(0,0,128),     /* unfragmented */
	RGB(128,0,128),                /* mft */
	RGB(255,255,0),RGB(238,221,0), /* directories */
	RGB(185,185,0),RGB(93,93,0),   /* compressed */
	RGB(0,255,255)                 /* not checked (temporary) */
};
HBRUSH hBrushes[NUM_OF_SPACE_STATES];

extern HWND hWindow,hMap,hList;
extern NEW_VOLUME_LIST_ENTRY *processed_entry;

HDC hGridDC = NULL;
HBITMAP hGridBitmap = NULL;
WNDPROC OldRectangleWndProc;
BOOL isRectangleUnicode = FALSE;

static void CalculateBitMapDimensions(void);
static BOOL CreateBitMapGrid(void);
NEW_VOLUME_LIST_ENTRY * vlist_get_first_selected_entry(void);

void InitMap(void)
{
	RECT rc;
	int i;
	
	hMap = GetDlgItem(hWindow,IDC_MAP);
	/* increase hight of the map */
	if(GetWindowRect(hMap,&rc)){
		rc.bottom ++;
		SetWindowPos(hMap,0,0,0,rc.right - rc.left,
			rc.bottom - rc.top,SWP_NOMOVE);
	}

	CalculateBitMapDimensions();
	
	/* reallocate cluster map buffer */
	if(global_cluster_map) free(global_cluster_map);
	global_cluster_map = malloc(map_blocks_per_line * map_lines);
	if(!global_cluster_map){
		MessageBox(hWindow,"Cannot allocate memory for the cluster map!",
				"Error",MB_OK | MB_ICONEXCLAMATION);
	}

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

static void CalculateBitMapDimensions(void)
{
	RECT rc;
	long dx, dy;
	int x_edge,y_edge;

	if(GetClientRect(hMap,&rc)){
		if(MapWindowPoints(hMap,hWindow,(LPPOINT)(PRECT)(&rc),(sizeof(RECT)/sizeof(POINT)))){
			/* calculate number of blocks and a real size of the map control */
			map_blocks_per_line = (rc.right - rc.left - 1) / (map_block_size + 1);
			map_width = (map_block_size + 1) * map_blocks_per_line + 1;
			map_lines = (rc.bottom - rc.top - 1) / (map_block_size + 1);
			map_height = (map_block_size + 1) * map_lines + 1;
			/* center the map control */
			dx = (rc.right - rc.left - map_width) / 2;
			dy = (rc.bottom - rc.top - map_height) / 2;
			if(dx > 0) rc.left += dx;
			if(dy > 0) rc.top += dy;
			/* border width is used because window size = client size + borders */
			x_edge = GetSystemMetrics(SM_CXEDGE);
			y_edge = GetSystemMetrics(SM_CYEDGE);
			(void)SetWindowPos(hMap,NULL,rc.left - y_edge,rc.top - x_edge, \
				map_width + 2 * y_edge,map_height + 2 * x_edge,SWP_NOZORDER);
		}
	}
	(void)InvalidateRect(hMap,NULL,TRUE);
}

/* Since v3.1.0 it supports all screen color depths. */
static BOOL CreateBitMapGrid(void)
{
	HDC hMainDC;
	HBRUSH hBrush, hOldBrush;
	RECT rc;

	hMainDC = GetDC(hWindow);
	hGridDC = CreateCompatibleDC(hMainDC);
	hGridBitmap = CreateCompatibleBitmap(hMainDC,map_width,map_height);
	ReleaseDC(hWindow,hMainDC);
	if(!hGridBitmap) { DeleteDC(hGridDC); hGridDC = NULL; return FALSE; }
	(void)SelectObject(hGridDC,hGridBitmap);
	(void)SetBkMode(hGridDC,TRANSPARENT);
	
	/* draw white field */
	rc.top = rc.left = 0;
	rc.bottom = map_height;
	rc.right = map_width;
	hBrush = GetStockObject(WHITE_BRUSH);
	hOldBrush = SelectObject(hGridDC,hBrush);
	(void)FillRect(hGridDC,&rc,hBrush);
	(void)SelectObject(hGridDC,hOldBrush);
	(void)DeleteObject(hBrush);

	/* draw grid */
	DrawBitMapGrid(hGridDC);
	return TRUE;
}

void DrawBitMapGrid(HDC hdc)
{
	HPEN hPen, hOldPen;
	int i;

	hPen = CreatePen(PS_SOLID,1,grid_color);
	hOldPen = SelectObject(hdc,hPen);
	for(i = 0; i < map_blocks_per_line + 1; i++){
		(void)MoveToEx(hdc,(map_block_size + 1) * i,0,NULL);
		(void)LineTo(hdc,(map_block_size + 1) * i,map_height);
	}
	for(i = 0; i < map_lines + 1; i++){
		(void)MoveToEx(hdc,0,(map_block_size + 1) * i,NULL);
		(void)LineTo(hdc,map_width,(map_block_size + 1) * i);
	}
	(void)SelectObject(hdc,hOldPen);
	(void)DeleteObject(hPen);
}

BOOL FillBitMap(char *cluster_map,NEW_VOLUME_LIST_ENTRY *v_entry)
{
	HDC hdc;
	HBRUSH hOldBrush;
	RECT block_rc;
	int i, j;

	if(cluster_map == NULL) return FALSE;
	if(v_entry == NULL) return FALSE;

	hdc = v_entry->hdc;
	if(!hdc) return FALSE;
	
	/* draw squares */
	hOldBrush = SelectObject(hdc,hBrushes[0]);
	for(i = 0; i < map_lines; i++){
		for(j = 0; j < map_blocks_per_line; j++){
			block_rc.top = (map_block_size + 1) * i + 1;
			block_rc.left = (map_block_size + 1) * j + 1;
			block_rc.right = block_rc.left + map_block_size;
			block_rc.bottom = block_rc.top + map_block_size;
			(void)FillRect(hdc,&block_rc,hBrushes[(int)cluster_map[i * map_blocks_per_line + j]]);
		}
	}
	(void)SelectObject(hdc,hOldBrush);
	return TRUE;
}

void ClearMap()
{
	HDC hdc;

	hdc = GetDC(hMap);
	(void)BitBlt(hdc,0,0,map_width,map_height,hGridDC,0,0,SRCCOPY);
	(void)ReleaseDC(hMap,hdc);
}

void RedrawMap(NEW_VOLUME_LIST_ENTRY *v_entry)
{
	HDC hdc;

	if(v_entry != NULL){
		if(v_entry->status > STATUS_UNDEFINED && v_entry->hdc){
			hdc = GetDC(hMap);
			(void)BitBlt(hdc,0,0,map_width,map_height,v_entry->hdc,0,0,SRCCOPY);
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
