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
int grid_line_width = DEFAULT_GRID_LINE_WIDTH; //10; //50;

int map_blocks_per_line = 145; //65
int map_lines = 32; //14
int map_width = 0x209;
int map_height = 0x8c;

COLORREF grid_color = RGB(0,0,0); //RGB(200,200,200)

COLORREF colors[NUM_OF_SPACE_STATES] = 
{
	RGB(255,255,255),               /* free */
	/*RGB(0,180,60),RGB(0,90,30),*/ /* dark green is too dark here... */
	RGB(0,215,32),RGB(4,164,0),     /* system */
	RGB(255,0,0),RGB(128,0,0),      /* fragmented */
	RGB(0,0,255),RGB(0,0,128),      /* unfragmented */
	RGB(211,0,255),RGB(128,0,128),  /* mft zone; mft itself */
	RGB(255,255,0),RGB(238,221,0),  /* directories */
	RGB(185,185,0),RGB(93,93,0),    /* compressed */
	RGB(0,255,255)                  /* not checked (temporary) */
};
HBRUSH hBrushes[NUM_OF_SPACE_STATES];

extern HWND hWindow,hMap,hList;
extern volume_processing_job *current_job;

WNDPROC OldRectangleWndProc;
BOOL isRectangleUnicode = FALSE;

void CalculateBitMapDimensions(void);

extern HANDLE hMapEvent;

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
	
	isRectangleUnicode = IsWindowUnicode(hMap);
	if(isRectangleUnicode)
		OldRectangleWndProc = (WNDPROC)SetWindowLongPtrW(hMap,GWLP_WNDPROC,
			(LONG_PTR)RectWndProc);
	else
		OldRectangleWndProc = (WNDPROC)SetWindowLongPtr(hMap,GWLP_WNDPROC,
			(LONG_PTR)RectWndProc);

	for(i = 0; i < NUM_OF_SPACE_STATES; i++){
		/* FIXME: check for success */
		hBrushes[i] = CreateSolidBrush(colors[i]);
	}
}

LRESULT CALLBACK RectWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;

	//if(iMsg == WM_ERASEBKGND)
		//return 1;
	
	if(iMsg == WM_PAINT){
		(void)BeginPaint(hWnd,&ps); /* (void)? */
		RedrawMap(current_job);
		EndPaint(hWnd,&ps);
	}
	if(isRectangleUnicode)
		return CallWindowProcW(OldRectangleWndProc,hWnd,iMsg,wParam,lParam);
	else
		return CallWindowProc(OldRectangleWndProc,hWnd,iMsg,wParam,lParam);
}

void CalculateBitMapDimensions(void)
{
	RECT rc;
	long dx, dy;
	int x_edge,y_edge;

	if(GetClientRect(hMap,&rc)){
		if(MapWindowPoints(hMap,hWindow,(LPPOINT)(PRECT)(&rc),(sizeof(RECT)/sizeof(POINT)))){
			/* calculate number of blocks and a real size of the map control */
			map_blocks_per_line = (rc.right - rc.left - grid_line_width) / (map_block_size + grid_line_width);
			map_width = (map_block_size + grid_line_width) * map_blocks_per_line + grid_line_width;
			map_lines = (rc.bottom - rc.top - grid_line_width) / (map_block_size + grid_line_width);
			map_height = (map_block_size + grid_line_width) * map_lines + grid_line_width;
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

/* Since v3.1.0 all screen color depths are supported */

void DrawBitMapGrid(HDC hdc)
{
	HPEN hPen, hOldPen;
	HBRUSH hBrush, hOldBrush;
	RECT rc;
	int i, j;

	/* draw white field */
	rc.top = rc.left = 0;
	rc.bottom = map_height;
	rc.right = map_width;
	hBrush = GetStockObject(WHITE_BRUSH);
	hOldBrush = SelectObject(hdc,hBrush);
	(void)FillRect(hdc,&rc,hBrush);
	(void)SelectObject(hdc,hOldBrush);
	(void)DeleteObject(hBrush);

	if(grid_line_width == 0) return;
	hPen = CreatePen(PS_SOLID,1,grid_color);
	hOldPen = SelectObject(hdc,hPen);
	/* draw vertical lines */
	for(i = 0; i < map_blocks_per_line + 1; i++){
		for(j = 0; j < grid_line_width; j++){
			(void)MoveToEx(hdc,(map_block_size + grid_line_width) * i + j,0,NULL);
			(void)LineTo(hdc,(map_block_size + grid_line_width) * i + j,map_height);
		}
	}
	/* draw horizontal lines */
	for(i = 0; i < map_lines + 1; i++){
		for(j = 0; j < grid_line_width; j++){
			(void)MoveToEx(hdc,0,(map_block_size + grid_line_width) * i + j,NULL);
			(void)LineTo(hdc,map_width,(map_block_size + grid_line_width) * i + j);
		}
	}
	(void)SelectObject(hdc,hOldPen);
	(void)DeleteObject(hPen);
}

BOOL _FillBitMap(volume_processing_job *job)
{
	HDC hdc = job->map.hdc;
	HBRUSH hOldBrush;
	RECT block_rc;
	int i, j;

	/* draw squares */
	hOldBrush = SelectObject(hdc,hBrushes[0]);
	for(i = 0; i < map_lines; i++){
		for(j = 0; j < map_blocks_per_line; j++){
			block_rc.top = (map_block_size + grid_line_width) * i + grid_line_width;
			block_rc.left = (map_block_size + grid_line_width) * j + grid_line_width;
			block_rc.right = block_rc.left + map_block_size;
			block_rc.bottom = block_rc.top + map_block_size;
			(void)FillRect(hdc,&block_rc,hBrushes[(int)job->map.scaled_buffer[i * map_blocks_per_line + j]]);
		}
	}
	(void)SelectObject(hdc,hOldBrush);
	return TRUE;
}

void RedrawMap(volume_processing_job *job)
{
	HDC hdc;
	HDC hDC, hMainDC;
	HBITMAP hBitmap;
	int i, j, k, x, ratio;
	int array[NUM_OF_SPACE_STATES];
	int maximum;
	
	if(job == NULL)
		return;
	
	if(WaitForSingleObject(hMapEvent,INFINITE) != WAIT_OBJECT_0){
		// TODO
		return;
	}
	
	/* if volume bitmap is empty or have improper size, recreate it */
	if(job->map.hdc == NULL || job->map.hbitmap == NULL ||
		job->map.width != map_width || job->map.height != map_height){
		if(job->map.hdc) (void)DeleteDC(job->map.hdc);
		if(job->map.hbitmap) (void)DeleteObject(job->map.hbitmap);
		hMainDC = GetDC(hWindow);
		hDC = CreateCompatibleDC(hMainDC);
		hBitmap = CreateCompatibleBitmap(hMainDC,map_width,map_height);
		(void)ReleaseDC(hWindow,hMainDC);
		
		if(hBitmap == NULL){
			// TODO: debug print here
			(void)DeleteDC(hDC);
			SetEvent(hMapEvent);
			return;
		}
	
		(void)SelectObject(hDC,hBitmap);
		(void)SetBkMode(hDC,TRANSPARENT);
		DrawBitMapGrid(hDC);
		job->map.hdc = hDC;
		job->map.hbitmap = hBitmap;
		job->map.width = map_width;
		job->map.height = map_height;
	}
		
	/* if cluster map does not exist, draw empty bitmap */
	if(job->map.buffer == NULL)
		goto redraw;
	
	/* scale map */
	if(job->map.scaled_buffer == NULL \
		|| job->map.scaled_size != map_blocks_per_line * map_lines){
		if(job->map.scaled_buffer) free(job->map.scaled_buffer);
		job->map.scaled_buffer = malloc(map_blocks_per_line * map_lines);
		if(job->map.scaled_buffer == NULL){
			// TODO: debug print here
			job->map.scaled_size = 0;
			SetEvent(hMapEvent);
			return;
		} else {
			job->map.scaled_size = map_blocks_per_line * map_lines;
		}
	}
	ratio = job->map.scaled_size / job->map.size;
	if(ratio != 0){
		/* scale up */
		for(i = 0, k = 0; i < job->map.size; i++){
			for(j = 0; j < ratio; j++){
				job->map.scaled_buffer[k] = job->map.buffer[i];
				k++;
			}
		}
		for(; k < job->map.scaled_size; k++)
			job->map.scaled_buffer[k] = job->map.buffer[i - 1];
	} else {
		/* scale down */
		ratio = job->map.size / job->map.scaled_size;
		for(i = 0, k = 0; i < job->map.scaled_size - 1; i++){
			memset(array,0,sizeof(array));
			for(j = 0; j < ratio; j++){
				// check bounds
				array[(int)job->map.buffer[k]] ++;
				k++;
			}
			maximum = array[0], x = 0;
			for(j = 1; j < NUM_OF_SPACE_STATES; j++){
				if(array[j] >= maximum){
					maximum = array[j], x = j;
				}
			}
			job->map.scaled_buffer[i] = x;
		}
		memset(array,0,sizeof(array));
		for(; k < job->map.size; k++)
			array[(int)job->map.buffer[k]] ++;
		maximum = array[0], x = 0;
		for(j = 1; j < NUM_OF_SPACE_STATES; j++){
			if(array[j] >= maximum){
				maximum = array[j], x = j;
			}
		}
		job->map.scaled_buffer[i] = x;
	}
	
	/* fill bitmap */
	_FillBitMap(job);
	
redraw:
	hdc = GetDC(hMap);
	(void)BitBlt(hdc,0,0,map_width,map_height,job->map.hdc,0,0,SRCCOPY);
	(void)ReleaseDC(hMap,hdc);
	SetEvent(hMapEvent);
}

void DeleteMaps()
{
	int i;

	for(i = 0; i < NUM_OF_SPACE_STATES; i++)
		(void)DeleteObject(hBrushes[i]);
}
