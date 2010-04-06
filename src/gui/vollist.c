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
extern NEW_VOLUME_LIST_ENTRY *processed_entry;

HWND hList;
WNDPROC OldListProc;
BOOL isListUnicode = FALSE;
HIMAGELIST hImgList;
int user_defined_column_widths[] = {0,0,0,0,0};
BOOL error_flag = FALSE, error_flag2 = FALSE;

DWORD WINAPI RescanDrivesThreadProc(LPVOID);
void DisplayLastError(char *caption);
void InitImageList(void);
void DestroyImageList(void);
void MakeVolListNiceLooking(void);
static void VolListAddItem(int index, volume_info *v);
static void AddCapacityInformation(int index, volume_info *v);
static void VolListUpdateStatusFieldInternal(int index,int Status);

/****************************************************************/
/*           internal volume list representation                */
/****************************************************************/

NEW_VOLUME_LIST_ENTRY vlist[MAX_DOS_DRIVES + 1];

static int vlist_init(void)
{
	memset(&vlist,0,sizeof(vlist));
	return 0;
}

static int vlist_destroy(void)
{
	NEW_VOLUME_LIST_ENTRY *entry;
	int i;
	
	for(i = 0; i < (MAX_DOS_DRIVES + 1); i++){
		entry = &vlist[i];
		if(entry->name[0] == 0) break;
		if(entry->hdc) (void)DeleteDC(entry->hdc);
		if(entry->hbitmap) (void)DeleteObject(entry->hbitmap);
	}	
	memset(&vlist,0,sizeof(vlist));
	return 0;
}

static int vlist_add_item(char *name,char *fsname,ULONGLONG total,ULONGLONG free)
{
	NEW_VOLUME_LIST_ENTRY *entry = NULL;
	HDC hDC, hMainDC;
	HBITMAP hBitmap;
	int i;
	
	/* already in list? */
	for(i = 0; i < (MAX_DOS_DRIVES + 1); i++){
		if(strcmp(vlist[i].name,name) == 0){
			vlist[i].stat.total_space = total;
			vlist[i].stat.free_space = free;
			return 0;
		}
	}

	/* search for the first empty entry */
	for(i = 0; i < (MAX_DOS_DRIVES + 1); i++){
		if(vlist[i].name[0] == 0){
			entry = &vlist[i];
			break;
		}
	}
	
	if(entry == NULL)
		return (-1);
	
	(void)strncpy(entry->name,name,MAX_VNAME_LEN);
	(void)strncpy(entry->fsname,fsname,MAX_FSNAME_LEN);
	
	entry->stat.total_space = total;
	entry->stat.free_space = free;
	
	hMainDC = GetDC(hWindow);
	hDC = CreateCompatibleDC(hMainDC);
	hBitmap = CreateCompatibleBitmap(hMainDC,iMAP_WIDTH,iMAP_HEIGHT);
	(void)ReleaseDC(hWindow,hMainDC);
	
	if(hBitmap == NULL){
		(void)DeleteDC(hDC);
		if(!error_flag2){ /* show message once */
			MessageBox(hWindow,"Cannot create bitmap in vlist_add_item()!",
				"Error",MB_OK | MB_ICONEXCLAMATION);
			error_flag2 = TRUE;
		}
		memset(entry,0,sizeof(NEW_VOLUME_LIST_ENTRY));
		return (-1);
	}

	(void)SelectObject(hDC,hBitmap);
	(void)SetBkMode(hDC,TRANSPARENT);
	DrawBitMapGrid(hDC);
	entry->hdc = hDC;
	entry->hbitmap = hBitmap;
	
	/* draw grid */

	return 0;
}

#if 0
static int vlist_remove_item(char *name)
{
	NEW_VOLUME_LIST_ENTRY *entry;
	int i;
	
	for(i = 0; i < (MAX_DOS_DRIVES + 1); i++){
		if(strcmp(vlist[i].name,name) == 0){
			entry = &vlist[i];
			if(entry->hdc) (void)DeleteDC(entry->hdc);
			if(entry->hbitmap) (void)DeleteObject(entry->hbitmap);
			memset(entry,0,sizeof(NEW_VOLUME_LIST_ENTRY));
			break;
		}
	}
	return 0;
}
#endif

NEW_VOLUME_LIST_ENTRY * vlist_get_entry(char *name)
{
	int i;
	
	for(i = 0; i < (MAX_DOS_DRIVES + 1); i++){
		if(strcmp(vlist[i].name,name) == 0) return &vlist[i];
	}
	return NULL;
}

NEW_VOLUME_LIST_ENTRY * vlist_get_first_selected_entry(void)
{
	LRESULT SelectedItem;
	LV_ITEM lvi;
	char buffer[128];
	
	SelectedItem = SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
	if(SelectedItem != -1){
		lvi.iItem = (int)SelectedItem;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT;
		lvi.pszText = buffer;
		lvi.cchTextMax = 127;
		if(SendMessage(hList,LVM_GETITEM,0,(LRESULT)&lvi)){
			buffer[2] = 0;
			return vlist_get_entry(buffer);
		}
	}
	return NULL;
}

/****************************************************************/
/*            volume list gui control procedures                */
/****************************************************************/

void InitVolList(void)
{
	LV_COLUMNW lvc;
	RECT rc;
	int dx;
	
	/* initialize the internal representation of the list */
	(void)vlist_init();

	hList = GetDlgItem(hWindow,IDC_VOLUMES);
	if(GetClientRect(hList,&rc)){
		dx = rc.right - rc.left;
	} else {
		dx = 586;
	}
	(void)SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,
		(LRESULT)(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT));

	/* create header */
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

	/* adjust hight of the list view control */
	if(GetWindowRect(hList,&rc)){
		rc.bottom += 5; //--; // changed after icons adding
		(void)SetWindowPos(hList,0,0,0,rc.right - rc.left,
			rc.bottom - rc.top,SWP_NOMOVE);
	}

	/* subclassing */
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

void FreeVolListResources(void)
{
	/* free gui resources */
	DestroyImageList();
	/* free internal list representation resources */
	(void)vlist_destroy();
}

void VolListNotifyHandler(LPARAM lParam)
{
	NEW_VOLUME_LIST_ENTRY *entry;
	LPNMLISTVIEW lpnm = (LPNMLISTVIEW)lParam;

	if(lpnm->hdr.code == LVN_ITEMCHANGED && lpnm->iItem != (-1)){
		entry = vlist_get_first_selected_entry();
		processed_entry = entry;
		HideProgress();
		/* redraw indicator */
		RedrawMap(entry);
		/* update status bar */
		if(entry) UpdateStatusBar(&entry->stat);
	}
}

void UpdateVolList(void)
{
	DWORD thr_id;
	HANDLE h = create_thread(RescanDrivesThreadProc,NULL,&thr_id);
	if(h == NULL)
		DisplayLastError("Cannot create thread starting drives rescan!");
	if(h) CloseHandle(h);
}

DWORD WINAPI RescanDrivesThreadProc(LPVOID lpParameter)
{
	int i;
	volume_info *v;
	LV_ITEM lvi;
	NEW_VOLUME_LIST_ENTRY *entry;
	
	WgxDisableWindows(hWindow,IDC_RESCAN,IDC_ANALYSE,
		IDC_DEFRAGM,IDC_OPTIMIZE,IDC_SHOWFRAGMENTED,0);
	HideProgress();

	/* refill the volume list control */
	(void)SendMessage(hList,LVM_DELETEALLITEMS,0,0);
	if(udefrag_get_avail_volumes(&v,skip_removable) >= 0){
		for(i = 0; v[i].letter != 0; i++)
			VolListAddItem(i,&v[i]);
	}

	MakeVolListNiceLooking();

	/* select the first item */
	lvi.mask = LVIF_STATE;
	lvi.stateMask = LVIS_SELECTED;
	lvi.state = LVIS_SELECTED;
	(void)SendMessage(hList,LVM_SETITEMSTATE,0,(LRESULT)&lvi);

	entry = vlist_get_first_selected_entry();
	RedrawMap(entry);
	if(entry) UpdateStatusBar(&entry->stat);
	
	WgxEnableWindows(hWindow,IDC_RESCAN,IDC_ANALYSE,
		IDC_DEFRAGM,IDC_OPTIMIZE,IDC_SHOWFRAGMENTED,0);
	return 0;
}

static void VolListAddItem(int index, volume_info *v)
{
	NEW_VOLUME_LIST_ENTRY *v_entry;
	LV_ITEM lvi;
	char s[128];
	char name[32];

	(void)sprintf(s,"%c: [%s]",v->letter,v->fsname);
	(void)sprintf(name,"%c:",v->letter);
	lvi.mask = LVIF_TEXT | LVIF_IMAGE;
	lvi.iItem = index;
	lvi.iSubItem = 0;
	lvi.pszText = s;
	lvi.iImage = v->is_removable ? 1 : 0;
	(void)SendMessage(hList,LVM_INSERTITEM,0,(LRESULT)&lvi);

	vlist_add_item(name,v->fsname,v->total_space.QuadPart,v->free_space.QuadPart);
	v_entry = vlist_get_entry(name);
	VolListUpdateStatusFieldInternal(index,v_entry->status);
	AddCapacityInformation(index,v);
}

static void AddCapacityInformation(int index, volume_info *v)
{
	LV_ITEM lvi;
	char s[32];
	double d;
	int p;

	lvi.mask = LVIF_TEXT;
	lvi.iItem = index;

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

static void VolListUpdateStatusFieldInternal(int index,int Status)
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

/****************************************************************/
/*                   ancillary procedures                       */
/****************************************************************/

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

void MakeVolListNiceLooking(void)
{
	RECT rc;
	int dx;
	int cw[] = {90,90,110,125,90};
	int user_defined_widths = 0;
	int total_width = 0;
	HWND hHeader;
	int i,h,y,height,delta;

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
}


/****************************************************************/
/*                         trash                                */
/****************************************************************/
void VolListUpdateStatusField(NEW_VOLUME_LIST_ENTRY *v_entry,int status)
{
	LV_ITEM lvi;
	char buffer[128];
	int index = -1;
	int item;

	if(v_entry == NULL) return;
	v_entry->status = status;

	while(1){
		item = (int)SendMessage(hList,LVM_GETNEXTITEM,(WPARAM)index,LVNI_ALL);
		if(item == -1 || item == index) break;
		index = item;
		lvi.iItem = index;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT;
		lvi.pszText = buffer;
		lvi.cchTextMax = 127;
		if(SendMessage(hList,LVM_GETITEM,0,(LRESULT)&lvi)){
			buffer[2] = 0;
			if(strcmp(buffer,v_entry->name) == 0){
				VolListUpdateStatusFieldInternal(index,status);
				return;
			}
		}
	}
}

/* TODO: optimize */
void VolListRefreshItem(NEW_VOLUME_LIST_ENTRY *v_entry)
{
	LV_ITEM lvi;
	char buffer[128];
	int index = -1;
	int item;
	volume_info *v;
	int i;

	if(v_entry == NULL) return;
	if(udefrag_get_avail_volumes(&v,skip_removable) >= 0){
		for(i = 0; v[i].letter != 0; i++){
			if(v[i].letter == v_entry->name[0]){
				v_entry->stat.total_space = v[i].total_space.QuadPart;
				v_entry->stat.free_space = v[i].free_space.QuadPart;

				while(1){
					item = (int)SendMessage(hList,LVM_GETNEXTITEM,(WPARAM)index,LVNI_ALL);
					if(item == -1 || item == index) break;
					index = item;
					lvi.iItem = index;
					lvi.iSubItem = 0;
					lvi.mask = LVIF_TEXT;
					lvi.pszText = buffer;
					lvi.cchTextMax = 127;
					if(SendMessage(hList,LVM_GETITEM,0,(LRESULT)&lvi)){
						buffer[2] = 0;
						if(strcmp(buffer,v_entry->name) == 0){
							AddCapacityInformation(index,&v[i]);
							return;
						}
					}
				}
				break;
			}
		}
	}
}
