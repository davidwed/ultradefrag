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
* GUI - volume list stuff.
*/

#include "main.h"

extern HINSTANCE hInstance;
extern HWND hWindow;
extern STATISTIC stat[MAX_DOS_DRIVES];
extern BOOL busy_flag;

extern int skip_removable;

HWND hList;
int Index; /* Index of currently selected list item */
char letter_numbers[MAX_DOS_DRIVES];
int work_status[MAX_DOS_DRIVES] = {0};
WNDPROC OldListProc;

HIMAGELIST hImgList;

int user_defined_column_widths[] = {0,0,0,0,0};

DWORD WINAPI RescanDrivesThreadProc(LPVOID);

void InitImageList(void)
{
	hImgList = ImageList_Create(16,16,ILC_COLOR8,2,0);
	if(!hImgList) return;
	
	ImageList_AddIcon(hImgList,LoadIcon(hInstance,MAKEINTRESOURCE(IDI_FIXED)));
	ImageList_AddIcon(hImgList,LoadIcon(hInstance,MAKEINTRESOURCE(IDI_REMOVABLE)));
	SendMessage(hList,LVM_SETIMAGELIST,LVSIL_SMALL,(LRESULT)hImgList);
}

void DestroyImageList(void)
{
	if(hImgList) ImageList_Destroy(hImgList);
}

void InitVolList(void)
{
	LV_COLUMNW lvc;
	RECT rc;
	int dx;

	memset((void *)work_status,0,sizeof(work_status));
	hList = GetDlgItem(hWindow,IDC_VOLUMES);
	GetClientRect(hList,&rc);
	dx = rc.right - rc.left;
	SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,
		(LRESULT)(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT));
	lvc.mask = LVCF_TEXT | LVCF_WIDTH;
	lvc.pszText = GetResourceString(L"VOLUME");
	lvc.cx = 60 * dx / 505;
	SendMessage(hList,LVM_INSERTCOLUMNW,0,(LRESULT)&lvc);
	lvc.pszText = GetResourceString(L"STATUS");
	lvc.cx = 60 * dx / 505;
	SendMessage(hList,LVM_INSERTCOLUMNW,1,(LRESULT)&lvc);

/*	lvc.pszText = GetResourceString(L"FILESYSTEM");
	lvc.cx = 100 * dx / 505;
	SendMessage(hList,LVM_INSERTCOLUMNW,2,(LRESULT)&lvc);
*/
	lvc.pszText = GetResourceString(L"TOTAL");
	lvc.mask |= LVCF_FMT;
	lvc.fmt = LVCFMT_RIGHT;
	lvc.cx = 100 * dx / 505;
	SendMessage(hList,LVM_INSERTCOLUMNW,2/*3*/,(LRESULT)&lvc);
	lvc.pszText = GetResourceString(L"FREE");
	lvc.cx = 100 * dx / 505;
	SendMessage(hList,LVM_INSERTCOLUMNW,3/*4*/,(LRESULT)&lvc);
	lvc.pszText = GetResourceString(L"PERCENT");
	lvc.cx = 85 * dx / 505;
	SendMessage(hList,LVM_INSERTCOLUMNW,4/*5*/,(LRESULT)&lvc);
	/* reduce(?) hight of list view control */
	GetWindowRect(hList,&rc);
	rc.bottom += 5; //--; // changed after icons adding
	SetWindowPos(hList,0,0,0,rc.right - rc.left,
		rc.bottom - rc.top,SWP_NOMOVE);
	OldListProc = (WNDPROC)SetWindowLongPtr(hList,GWLP_WNDPROC,(LONG_PTR)ListWndProc);
	SendMessage(hList,LVM_SETBKCOLOR,0,RGB(255,255,255));
	InitImageList();
}

void UpdateVolList(void)
{
	DWORD thr_id;
	create_thread(RescanDrivesThreadProc,NULL,&thr_id);
}

static void VolListAddItem(int index, volume_info *v)
{
	LV_ITEM lvi;
	LV_ITEMW lviw;
	char s[32];
	int st;
	double d;
	int p;

	sprintf(s,"%c: [%s]",v->letter,v->fsname);
	lvi.mask = LVIF_TEXT | LVIF_IMAGE;
	lvi.iItem = index;
	lvi.iSubItem = 0;
	lvi.pszText = s;
	lvi.iImage = v->is_removable ? 1 : 0;
	SendMessage(hList,LVM_INSERTITEM,0,(LRESULT)&lvi);

	st = work_status[v->letter - 'A'];
	lviw.mask = LVIF_TEXT | LVIF_IMAGE;
	lviw.iItem = index;
	lviw.iSubItem = 1;

	if(st == STAT_AN){
		lviw.pszText = GetResourceString(L"ANALYSE_STATUS");
	} else if(st == STAT_DFRG){
		lviw.pszText = GetResourceString(L"DEFRAG_STATUS");
	} else {
		lviw.pszText = L"";
	}

	SendMessage(hList,LVM_SETITEMW,0,(LRESULT)&lviw);
/*
	lvi.iSubItem = 2;
	lvi.pszText = v->fsname;
	SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);
*/
	udefrag_fbsize((ULONGLONG)(v->total_space.QuadPart),2,s,sizeof(s));
	lvi.iSubItem = 2;//3;
	lvi.pszText = s;
 	SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);

	udefrag_fbsize((ULONGLONG)(v->free_space.QuadPart),2,s,sizeof(s));
	lvi.iSubItem = 3;//4;
	lvi.pszText = s;
	SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);

	d = (double)(signed __int64)(v->free_space.QuadPart);
	/* 0.1 constant is used to exclude divide by zero error */
	d /= ((double)(signed __int64)(v->total_space.QuadPart) + 0.1);
	p = (int)(100 * d);
	sprintf(s,"%u %%",p);
	lvi.iSubItem = 4;//5;
	lvi.pszText = s;
	SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);
}

/* TODO: cleanup */
DWORD WINAPI RescanDrivesThreadProc(LPVOID lpParameter)
{
	char chr;
	int index;
	volume_info *v;
	int i;
	RECT rc;
	int dx;
	int cw[] = {90,90,110,125,90};
	int user_defined_widths = 0;
	int total_width = 0;
	LV_ITEM lvi;
	ERRORHANDLERPROC eh;
	
	DisableButtonsBeforeDrivesRescan();
	HideProgress();

	SendMessage(hList,LVM_DELETEALLITEMS,0,0);
	index = 0;
	eh = udefrag_set_error_handler(NULL);
	if(udefrag_get_avail_volumes(&v,skip_removable) >= 0){
		for(i = 0;;i++){
			chr = v[i].letter;
			if(!chr) break;
			VolListAddItem(i,&v[i]);
			letter_numbers[index] = chr - 'A';
			CreateBitMap(chr - 'A');
			index ++;
		}
	}
	udefrag_set_error_handler(eh);
	/* adjust columns widths */
	GetClientRect(hList,&rc);
	dx = rc.right - rc.left;

	if(user_defined_column_widths[0])
		user_defined_widths = 1;
	for(i = 0; i < 5; i++)
		total_width += user_defined_column_widths[i];
	
	for(i = 0; i < 5; i++){
		if(user_defined_widths){
			SendMessage(hList,LVM_SETCOLUMNWIDTH,i,
				user_defined_column_widths[i] * dx / total_width);
		}else{
			SendMessage(hList,LVM_SETCOLUMNWIDTH,i,
				cw[i] * dx / 505);
		}
	}
	lvi.mask = LVIF_STATE;
	lvi.stateMask = LVIS_SELECTED;
	lvi.state = LVIS_SELECTED;
	SendMessage(hList,LVM_SETITEMSTATE,0,(LRESULT)&lvi);
	Index = letter_numbers[0];
	RedrawMap();
	UpdateStatusBar(&stat[Index]);
	EnableButtonsAfterDrivesRescan();
	return 0;
}

LRESULT VolListGetSelectedItemIndex(void)
{
	return SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
}

int VolListGetWorkStatus(LRESULT iItem)
{
	int index = letter_numbers[iItem];
	return work_status[index];
}

char VolListGetLetter(LRESULT iItem)
{
	int index = letter_numbers[iItem];
	return (index + 'A');
}

/* 0 for A, 1 for B etc. */
int VolListGetLetterNumber(LRESULT iItem)
{
	return letter_numbers[iItem];
}

void VolListUpdateStatusField(int stat,LRESULT iItem)
{
	LV_ITEMW lviw;

	work_status[(int)letter_numbers[(int)iItem]] = stat;
	if(stat == STAT_CLEAR)
		return;
	lviw.mask = LVIF_TEXT;
	lviw.iItem = (int)iItem;
	lviw.iSubItem = 1;

	if(stat == STAT_AN){
		lviw.pszText = GetResourceString(L"ANALYSE_STATUS");
	} else if(stat == STAT_DFRG){
		lviw.pszText = GetResourceString(L"DEFRAG_STATUS");
	} else {
		lviw.pszText = L"";
	}

	SendMessage(hList,LVM_SETITEMW,0,(LRESULT)&lviw);
}

void VolListNotifyHandler(LPARAM lParam)
{
	LPNMLISTVIEW lpnm = (LPNMLISTVIEW)lParam;
	if(lpnm->hdr.code == LVN_ITEMCHANGED && (lpnm->uNewState & LVIS_SELECTED)){
		HideProgress();
		/* change Index */
		Index = letter_numbers[lpnm->iItem];
		/* redraw indicator */
		RedrawMap();
		/* Update status bar */
		UpdateStatusBar(&stat[Index]);
	}
}

LRESULT CALLBACK ListWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	if((iMsg == WM_LBUTTONDOWN || iMsg == WM_LBUTTONUP ||
		iMsg == WM_RBUTTONDOWN || iMsg == WM_RBUTTONUP) && busy_flag)
		return 0;
	if(iMsg == WM_KEYDOWN){
		HandleShortcuts(hWnd,iMsg,wParam,lParam);
		if(busy_flag) return 0;
	}
	return CallWindowProc(OldListProc,hWnd,iMsg,wParam,lParam);
}

void VolListGetColumnWidths(void)
{
	int i;
	RECT rc;
	int dx;

	GetClientRect(hList,&rc);
	dx = rc.right - rc.left;
	if(!dx) return;
	for(i = 0; i < 5; i++){
		user_defined_column_widths[i] = \
			(int)(LONG_PTR)SendMessage(hList,LVM_GETCOLUMNWIDTH,i,0);
	}
}

/* TODO: optimize */
void VolListRefreshItem(LRESULT iItem)
{
	ERRORHANDLERPROC eh;
	volume_info *v;
	int i;
	char chr, letter;
	
	LV_ITEM lvi;
	char s[32];
	double d;
	int p;

	letter = VolListGetLetter(iItem);
	chr = 0;
	eh = udefrag_set_error_handler(NULL);
	if(udefrag_get_avail_volumes(&v,skip_removable) >= 0){
		for(i = 0;;i++){
			chr = v[i].letter;
			if(!chr) break;
			if(chr == letter) break;
		}
	}
	if(chr){
		/* update the Total space, Free space and Percentage fields */
		lvi.mask = LVIF_TEXT | LVIF_IMAGE;
		lvi.iItem = (int)(LONG_PTR)iItem;
		lvi.iImage = v[i].is_removable ? 1 : 0;

		udefrag_fbsize((ULONGLONG)(v[i].total_space.QuadPart),2,s,sizeof(s));
		lvi.iSubItem = 2;
		lvi.pszText = s;
		SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);
	
		udefrag_fbsize((ULONGLONG)(v[i].free_space.QuadPart),2,s,sizeof(s));
		lvi.iSubItem = 3;
		lvi.pszText = s;
		SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);
	
		d = (double)(signed __int64)(v[i].free_space.QuadPart);
		/* 0.1 constant is used to exclude divide by zero error */
		d /= ((double)(signed __int64)(v[i].total_space.QuadPart) + 0.1);
		p = (int)(100 * d);
		sprintf(s,"%u %%",p);
		lvi.iSubItem = 4;
		lvi.pszText = s;
		SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);
	}
	udefrag_set_error_handler(eh);
}
