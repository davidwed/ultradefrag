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
* GUI - volume list stuff.
*/

#include "main.h"

extern HINSTANCE hInstance;
extern HWND hWindow;

extern BOOL busy_flag;

extern int skip_removable;
extern int iMAP_WIDTH;
extern int iMAP_HEIGHT;

extern WGX_I18N_RESOURCE_ENTRY i18n_table[];

HWND hList;
WNDPROC OldListProc;
BOOL isListUnicode = FALSE;
HIMAGELIST hImgList;
int user_defined_column_widths[] = {0,0,0,0,0};

DWORD WINAPI RescanDrivesThreadProc(LPVOID);

/* no more than 255 volumes :) */
#define MAX_NUMBER_OF_VOLUMES 255
VOLUME_LIST_ENTRY volume_list[MAX_NUMBER_OF_VOLUMES + 1];

BOOL error_flag = FALSE, error_flag2 = FALSE;

void DisplayLastError(char *caption);

void InitImageList(void)
{
	hImgList = ImageList_Create(16,16,ILC_COLOR8,2,0);
	if(!hImgList) return;
	
	(void)ImageList_AddIcon(hImgList,LoadIcon(hInstance,MAKEINTRESOURCE(IDI_FIXED)));
	(void)ImageList_AddIcon(hImgList,LoadIcon(hInstance,MAKEINTRESOURCE(IDI_REMOVABLE)));
	(void)SendMessage(hList,LVM_SETIMAGELIST,LVSIL_SMALL,(LRESULT)hImgList);
}

void DestroyImageList(void)
{
	if(hImgList) (void)ImageList_Destroy(hImgList);
}

void InitVolList(void)
{
	LV_COLUMNW lvc;
	RECT rc;
	int dx;
	
//	int y;

	memset(volume_list,0,sizeof(volume_list));

	hList = GetDlgItem(hWindow,IDC_VOLUMES);
	if(GetClientRect(hList,&rc)){
		dx = rc.right - rc.left;
	} else {
		dx = 586;
	}
	(void)SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,
		(LRESULT)(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT));

	lvc.mask = LVCF_TEXT | LVCF_WIDTH;
	lvc.pszText = WgxGetResourceString(i18n_table,L"VOLUME");
	lvc.cx = 60 * dx / 505;
	(void)SendMessage(hList,LVM_INSERTCOLUMNW,0,(LRESULT)&lvc);
	lvc.pszText = WgxGetResourceString(i18n_table,L"STATUS");
	lvc.cx = 60 * dx / 505;
	(void)SendMessage(hList,LVM_INSERTCOLUMNW,1,(LRESULT)&lvc);
	lvc.pszText = WgxGetResourceString(i18n_table,L"TOTAL");
	lvc.mask |= LVCF_FMT;
	lvc.fmt = LVCFMT_RIGHT;
	lvc.cx = 100 * dx / 505;
	(void)SendMessage(hList,LVM_INSERTCOLUMNW,2,(LRESULT)&lvc);
	lvc.pszText = WgxGetResourceString(i18n_table,L"FREE");
	lvc.cx = 100 * dx / 505;
	(void)SendMessage(hList,LVM_INSERTCOLUMNW,3,(LRESULT)&lvc);
	lvc.pszText = WgxGetResourceString(i18n_table,L"PERCENT");
	lvc.cx = 85 * dx / 505;
	(void)SendMessage(hList,LVM_INSERTCOLUMNW,4,(LRESULT)&lvc);

/*	rc.top = 1;
	rc.left = LVIR_BOUNDS;
	SendMessage(hList,LVM_GETSUBITEMRECT,0,(LRESULT)&rc);
	y = rc.bottom - rc.top;
*/	
	/* adjust hight of list view control */
	if(GetWindowRect(hList,&rc)){
		rc.bottom += 5; //--; // changed after icons adding
		(void)SetWindowPos(hList,0,0,0,rc.right - rc.left,
			rc.bottom - rc.top,SWP_NOMOVE);
	}

	isListUnicode = IsWindowUnicode(hList);
	if(isListUnicode)
		OldListProc = (WNDPROC)SetWindowLongPtrW(hList,GWLP_WNDPROC,
			(LONG_PTR)ListWndProc);
	else
		OldListProc = (WNDPROC)SetWindowLongPtr(hList,GWLP_WNDPROC,
			(LONG_PTR)ListWndProc);
	(void)SendMessage(hList,LVM_SETBKCOLOR,0,RGB(255,255,255));
	InitImageList();
}

void FreeVolListResources(void)
{
	int i;

	DestroyImageList();
	
	for(i = 0;; i++){
		if(volume_list[i].VolumeName == NULL) break;
		free(volume_list[i].VolumeName);
		if(volume_list[i].hDC) (void)DeleteDC(volume_list[i].hDC);
		if(volume_list[i].hBitmap) (void)DeleteObject(volume_list[i].hBitmap);
	}
	memset(volume_list,0,sizeof(volume_list));
}

void UpdateVolList(void)
{
	DWORD thr_id;
	HANDLE h = create_thread(RescanDrivesThreadProc,NULL,&thr_id);
	if(h == NULL)
		DisplayLastError("Cannot create thread starting drives rescan!");
	if(h) CloseHandle(h);
}

static PVOLUME_LIST_ENTRY AddVolumeListEntry(char *VolumeName)
{
	PVOLUME_LIST_ENTRY pv = NULL;
	int i;
	HDC hDC, hMainDC;
	HBITMAP hBitmap;
	
	for(i = 0; i <= MAX_NUMBER_OF_VOLUMES; i++){
		if(volume_list[i].VolumeName == NULL) break;
		if(!strcmp(volume_list[i].VolumeName,VolumeName)){
			pv = &(volume_list[i]);
			return pv;
		}
	}
	if(i == MAX_NUMBER_OF_VOLUMES){ /* too many volumes */
		pv = &(volume_list[1]);
		if(pv->VolumeName) free(pv->VolumeName);
		if(pv->hDC) (void)DeleteDC(pv->hDC);
		if(pv->hBitmap) (void)DeleteObject(pv->hBitmap);
		memset(pv,0,sizeof(VOLUME_LIST_ENTRY));
	} else {
		pv = &(volume_list[i]);
	}

	pv->VolumeName = malloc(strlen(VolumeName) + 1);
	if(!pv->VolumeName){
		if(!error_flag){ /* show message once */
			MessageBox(hWindow,"No enough memory for AddVolumeListEntry()!",
				"Error",MB_OK | MB_ICONEXCLAMATION);
			error_flag = TRUE;
		}
		return pv;
	}
	(void)strcpy(pv->VolumeName,VolumeName);

	hMainDC = GetDC(hWindow);
	hDC = CreateCompatibleDC(hMainDC);
	hBitmap = CreateCompatibleBitmap(hMainDC,iMAP_WIDTH,iMAP_HEIGHT);
	(void)ReleaseDC(hWindow,hMainDC);
	if(!hBitmap){
		(void)DeleteDC(hDC);
		if(!error_flag2){ /* show message once */
			MessageBox(hWindow,"Cannot create bitmap in AddVolumeListEntry()!",
				"Error",MB_OK | MB_ICONEXCLAMATION);
			error_flag2 = TRUE;
		}
	} else {
		(void)SelectObject(hDC,hBitmap);
		(void)SetBkMode(hDC,TRANSPARENT);
		pv->hDC = hDC;
		pv->hBitmap = hBitmap;
	}
	return pv;
}

/* DON'T MODIFY RETURNED ENTRY! */
static PVOLUME_LIST_ENTRY GetVolumeListEntry(char *VolumeName)
{
	PVOLUME_LIST_ENTRY pv = NULL;
	int i;
	
	if(VolumeName == NULL){
		pv = &(volume_list[MAX_NUMBER_OF_VOLUMES]);
		return pv;
	}
	
	for(i = 0; i <= MAX_NUMBER_OF_VOLUMES; i++){
		if(volume_list[i].VolumeName == NULL) break;
		if(!strcmp(volume_list[i].VolumeName,VolumeName)) break;
	}
	pv = &(volume_list[i]);
	return pv;
}

static void AddCapacityInformation(int index, volume_info *v)
{
	LV_ITEM lvi;
	char s[32];
	double d;
	int p;

	lvi.mask = LVIF_TEXT/* | LVIF_IMAGE*/;
	lvi.iItem = index;
	//lvi.iImage = v->is_removable ? 1 : 0;

	(void)udefrag_fbsize((ULONGLONG)(v->total_space.QuadPart),2,s,sizeof(s));
	lvi.iSubItem = 2;
	lvi.pszText = s;
 	(void)SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);

	(void)udefrag_fbsize((ULONGLONG)(v->free_space.QuadPart),2,s,sizeof(s));
	lvi.iSubItem = 3;
	lvi.pszText = s;
	(void)SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);

	d = (double)(signed __int64)(v->free_space.QuadPart);
	/* 0.1 constant is used to exclude divide by zero error */
	d /= ((double)(signed __int64)(v->total_space.QuadPart) + 0.1);
	p = (int)(100 * d);
	(void)sprintf(s,"%u %%",p);
	lvi.iSubItem = 4;
	lvi.pszText = s;
	(void)SendMessage(hList,LVM_SETITEM,0,(LRESULT)&lvi);
}

static void VolListUpdateStatusField(int index,int Status)
{
	LV_ITEMW lviw;

	lviw.mask = LVIF_TEXT;
	lviw.iItem = index;
	lviw.iSubItem = 1;
	
	switch(Status){
	case STATUS_RUNNING:
		lviw.pszText = WgxGetResourceString(i18n_table,L"STATUS_RUNNING");
		break;
	case STATUS_ANALYSED:
		lviw.pszText = WgxGetResourceString(i18n_table,L"STATUS_ANALYSED");
		break;
	case STATUS_DEFRAGMENTED:
		lviw.pszText = WgxGetResourceString(i18n_table,L"STATUS_DEFRAGMENTED");
		break;
	case STATUS_OPTIMIZED:
		lviw.pszText = WgxGetResourceString(i18n_table,L"STATUS_OPTIMIZED");
		break;
	default:
		lviw.pszText = L"";
	}

	(void)SendMessage(hList,LVM_SETITEMW,0,(LRESULT)&lviw);
}

static void VolListAddItem(int index, volume_info *v)
{
	PVOLUME_LIST_ENTRY pv;
	LV_ITEM lvi;
	char s[32];

	(void)sprintf(s,"%c: [%s]",v->letter,v->fsname);
	lvi.mask = LVIF_TEXT | LVIF_IMAGE;
	lvi.iItem = index;
	lvi.iSubItem = 0;
	lvi.pszText = s;
	lvi.iImage = v->is_removable ? 1 : 0;
	(void)SendMessage(hList,LVM_INSERTITEM,0,(LRESULT)&lvi);

	pv = AddVolumeListEntry(s);
	VolListUpdateStatusField(index,pv->Status);
	AddCapacityInformation(index,v);
}

DWORD WINAPI RescanDrivesThreadProc(LPVOID lpParameter)
{
	volume_info *v;
	int i;
	RECT rc;
	int dx;
	int cw[] = {90,90,110,125,90};
	int user_defined_widths = 0;
	int total_width = 0;
	LV_ITEM lvi;

	HWND hHeader;
	int h,y,height,delta;
	
	WgxDisableWindows(hWindow,IDC_RESCAN,IDC_ANALYSE,
		IDC_DEFRAGM,IDC_OPTIMIZE,IDC_SHOWFRAGMENTED,0);
	HideProgress();

	(void)SendMessage(hList,LVM_DELETEALLITEMS,0,0);
	if(udefrag_get_avail_volumes(&v,skip_removable) >= 0){
		for(i = 0; v[i].letter != 0; i++)
			VolListAddItem(i,&v[i]);
	}
	/* adjust columns widths */
	if(GetClientRect(hList,&rc))
		dx = rc.right - rc.left;
	else
		dx = 586;

	if(user_defined_column_widths[0])
		user_defined_widths = 1;
	for(i = 0; i < 5; i++)
		total_width += user_defined_column_widths[i];
	
	for(i = 0; i < 5; i++){
		if(user_defined_widths){
			(void)SendMessage(hList,LVM_SETCOLUMNWIDTH,i,
				user_defined_column_widths[i] * dx / total_width);
		}else{
			(void)SendMessage(hList,LVM_SETCOLUMNWIDTH,i,
				cw[i] * dx / 505);
		}
	}
	lvi.mask = LVIF_STATE;
	lvi.stateMask = LVIS_SELECTED;
	lvi.state = LVIS_SELECTED;
	(void)SendMessage(hList,LVM_SETITEMSTATE,0,(LRESULT)&lvi);

	rc.top = 1;
	rc.left = LVIR_BOUNDS;
	if(SendMessage(hList,LVM_GETSUBITEMRECT,0,(LRESULT)&rc)){
		y = rc.bottom - rc.top;
		
		hHeader = (HWND)(LONG_PTR)SendMessage(hList,LVM_GETHEADER,0,0);
		if(hHeader){
			if(SendMessage(hHeader,HDM_GETITEMRECT,0,(LRESULT)&rc)){
				h = rc.bottom - rc.top;
				
				if(GetWindowRect(hList,&rc)){
					height = rc.bottom - rc.top;
					delta = (height - h) % y;
					rc.bottom -= (delta - 4);
				
					(void)SetWindowPos(hList,0,0,0,rc.right - rc.left,
						rc.bottom - rc.top,SWP_NOMOVE);
				}
			}
		}
	}
	
	RedrawMap();
	UpdateStatusBar(&(volume_list[0].Statistics));
	
	WgxEnableWindows(hWindow,IDC_RESCAN,IDC_ANALYSE,
		IDC_DEFRAGM,IDC_OPTIMIZE,IDC_SHOWFRAGMENTED,0);
	return 0;
}

PVOLUME_LIST_ENTRY VolListGetSelectedEntry(void)
{
	LRESULT SelectedItem;
	LV_ITEM lvi;
	char *VolumeName = NULL;
	char buffer[128];
	
	SelectedItem = SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
	if(SelectedItem != -1){
		lvi.iItem = (int)SelectedItem;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT;
		lvi.pszText = buffer;
		lvi.cchTextMax = 127;
		if(SendMessage(hList,LVM_GETITEM,0,(LRESULT)&lvi))
			VolumeName = lvi.pszText;
	}
	return GetVolumeListEntry(VolumeName);
}

void VolListUpdateSelectedStatusField(int Status)
{
	PVOLUME_LIST_ENTRY vl;
	LRESULT SelectedItem;

	vl = VolListGetSelectedEntry();
	if(vl->VolumeName) vl->Status = Status;

	SelectedItem = SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
	if(SelectedItem != -1)
		VolListUpdateStatusField((int)SelectedItem,Status);
}

void VolListNotifyHandler(LPARAM lParam)
{
	PVOLUME_LIST_ENTRY vl;
	LPNMLISTVIEW lpnm = (LPNMLISTVIEW)lParam;
	if(lpnm->hdr.code == LVN_ITEMCHANGED && (lpnm->uNewState & LVIS_SELECTED)){
		HideProgress();
		/* redraw indicator */
		RedrawMap();
		/* Update status bar */
		vl = VolListGetSelectedEntry();
		UpdateStatusBar(&(vl->Statistics));
	}
}

LRESULT CALLBACK ListWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	if((iMsg == WM_LBUTTONDOWN || iMsg == WM_LBUTTONUP ||
		iMsg == WM_RBUTTONDOWN || iMsg == WM_RBUTTONUP) && busy_flag)
		return 0;
	if(iMsg == WM_KEYDOWN){
		if(busy_flag){ /* only 'Stop' and 'About' actions are allowed */
			if(wParam != VK_F1 && wParam != 'S') return 0;
		}
	}

	/* why? */
	if(iMsg == WM_VSCROLL){
		(void)InvalidateRect(hList,NULL,TRUE);
		(void)UpdateWindow(hList);
	}

	if(isListUnicode)
		return CallWindowProcW(OldListProc,hWnd,iMsg,wParam,lParam);
	else
		return CallWindowProc(OldListProc,hWnd,iMsg,wParam,lParam);
}

void VolListGetColumnWidths(void)
{
	int i;
	RECT rc;
	int dx;

	if(GetClientRect(hList,&rc))
		dx = rc.right - rc.left;
	else
		dx = 586;
	if(!dx) return;
	for(i = 0; i < 5; i++){
		user_defined_column_widths[i] = \
			(int)(LONG_PTR)SendMessage(hList,LVM_GETCOLUMNWIDTH,i,0);
	}
}

/* TODO: optimize */
void VolListRefreshSelectedItem(void)
{
	LRESULT SelectedItem;
	PVOLUME_LIST_ENTRY vl;
	volume_info *v;
	int i;

	vl = VolListGetSelectedEntry();
	if(vl->VolumeName == NULL) return;
	if(udefrag_get_avail_volumes(&v,skip_removable) >= 0){
		for(i = 0; v[i].letter != 0; i++){
			if(v[i].letter == vl->VolumeName[0]){
				SelectedItem = SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
				if(SelectedItem != -1)
					AddCapacityInformation((int)SelectedItem,&v[i]);
				break;
			}
		}
	}
}
