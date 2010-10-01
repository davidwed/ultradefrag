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
extern WGX_I18N_RESOURCE_ENTRY i18n_table[];
extern volume_processing_job *current_job;

HWND hList;
WNDPROC OldListWndProc;
HIMAGELIST hImgList;
int user_defined_column_widths[] = {0,0,0,0,0};

DWORD WINAPI RescanDrivesThreadProc(LPVOID);
void DisplayLastError(char *caption);
void InitImageList(void);
void DestroyImageList(void);
static void VolListAddItem(int index, volume_info *v);
static void AddCapacityInformation(int index, volume_info *v);
static void VolListUpdateStatusFieldInternal(int index,volume_processing_job *job);
LRESULT CALLBACK ListWndProc(HWND, UINT, WPARAM, LPARAM);

volume_processing_job * get_first_selected_job(void)
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
			return get_job(buffer[0]);
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
	
	hList = GetDlgItem(hWindow,IDC_VOLUMES);
	(void)SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,
		(LRESULT)(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT));

	/* create header */
	lvc.mask = LVCF_TEXT;
	lvc.pszText = WgxGetResourceString(i18n_table,L"VOLUME");
	(void)SendMessage(hList,LVM_INSERTCOLUMNW,0,(LRESULT)&lvc);

	lvc.pszText = WgxGetResourceString(i18n_table,L"STATUS");
	(void)SendMessage(hList,LVM_INSERTCOLUMNW,1,(LRESULT)&lvc);

	lvc.mask |= LVCF_FMT;
	lvc.fmt = LVCFMT_RIGHT;
	lvc.pszText = WgxGetResourceString(i18n_table,L"TOTAL");
	(void)SendMessage(hList,LVM_INSERTCOLUMNW,2,(LRESULT)&lvc);

	lvc.pszText = WgxGetResourceString(i18n_table,L"FREE");
	(void)SendMessage(hList,LVM_INSERTCOLUMNW,3,(LRESULT)&lvc);

	lvc.pszText = WgxGetResourceString(i18n_table,L"PERCENT");
	(void)SendMessage(hList,LVM_INSERTCOLUMNW,4,(LRESULT)&lvc);

	OldListWndProc = WgxSafeSubclassWindow(hList,ListWndProc);
	(void)SendMessage(hList,LVM_SETBKCOLOR,0,RGB(255,255,255));
	InitImageList();
}

LRESULT CALLBACK ListWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg){
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		if(busy_flag) return 0;
		break;
	case WM_KEYDOWN:
		if(busy_flag && wParam != VK_F1 && wParam != 'S'){
			/* only 'Stop' and 'About' actions are allowed */
			return 0;
		}
		break;
	case WM_VSCROLL:
		/* why? */
		(void)InvalidateRect(hList,NULL,TRUE);
		(void)UpdateWindow(hList);
		break;
	}
	return WgxSafeCallWndProc(OldListWndProc,hWnd,iMsg,wParam,lParam);
}

void AdjustVolListColumns(void)
{
	int cw[] = {90,90,110,125,90};
	int total_width = 0;
	int i, width;
	RECT rc;

	/* adjust columns widths */
	if(GetClientRect(hList,&rc))
		width = rc.right - rc.left;
	else
		width = 586;

	if(user_defined_column_widths[0]){
		for(i = 0; i < 5; i++)
			total_width += user_defined_column_widths[i];
		for(i = 0; i < 5; i++){
			(void)SendMessage(hList,LVM_SETCOLUMNWIDTH,i,
				user_defined_column_widths[i] * width / total_width);
		}
	} else {
		for(i = 0; i < 5; i++){
			(void)SendMessage(hList,LVM_SETCOLUMNWIDTH,i,
				cw[i] * width / 505);
		}
	}
}

int ResizeVolList(int x, int y, int width, int height)
{
	int border_height;
	int border_width;
	int header_height = 0;
	int item_height = 0;
	int n_items;
	HWND hHeader;
	RECT rc;

	/* adjust height of the list */
	border_height = GetSystemMetrics(SM_CYEDGE);
	border_width = GetSystemMetrics(SM_CXEDGE);
	hHeader = (HWND)(LONG_PTR)SendMessage(hList,LVM_GETHEADER,0,0);
	if(hHeader){
		if(SendMessage(hHeader,HDM_GETITEMRECT,0,(LRESULT)&rc))
			header_height = rc.bottom - rc.top;
	}
	rc.top = 1;
	rc.left = LVIR_BOUNDS;
	if(SendMessage(hList,LVM_GETSUBITEMRECT,0,(LRESULT)&rc))
		item_height = rc.bottom - rc.top;
	if(header_height && item_height){
		/* ensure that an integer number of items will be displayed */
		n_items = (height - 2 * border_height - header_height) / item_height;
		height = header_height + n_items * item_height + 2 * border_height;
	}
	(void)SetWindowPos(hList,0,x,y,width,height,0);

	AdjustVolListColumns();
	return height;
}

void FreeVolListResources(void)
{
	/* free gui resources */
	DestroyImageList();
}

void VolListNotifyHandler(LPARAM lParam)
{
	volume_processing_job *job;
	LPNMLISTVIEW lpnm = (LPNMLISTVIEW)lParam;

	if(lpnm->hdr.code == LVN_ITEMCHANGED && lpnm->iItem != (-1)){
		job = get_first_selected_job();
		current_job = job;
		HideProgress();
		/* redraw indicator */
		RedrawMap(job);
		/* update status bar */
		if(job) UpdateStatusBar(&job->pi);
	}
}

void UpdateVolList(void)
{
	DWORD thr_id;
	HANDLE h = create_thread(RescanDrivesThreadProc,NULL,&thr_id);
	if(h == NULL){
		WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,
			"UltraDefrag: cannot create thread starting drives rescan!");
	} else {
		CloseHandle(h);
	}
}

DWORD WINAPI RescanDrivesThreadProc(LPVOID lpParameter)
{
	int i;
	volume_info *v;
	LV_ITEM lvi;
	volume_processing_job *job;
	
	WgxDisableWindows(hWindow,IDC_RESCAN,IDC_ANALYSE,
		IDC_DEFRAGM,IDC_OPTIMIZE,IDC_SHOWFRAGMENTED,0);
	HideProgress();

	/* refill the volume list control */
	(void)SendMessage(hList,LVM_DELETEALLITEMS,0,0);
	v = udefrag_get_vollist(skip_removable);
	if(v){
		for(i = 0; v[i].letter != 0; i++)
			VolListAddItem(i,&v[i]);
		udefrag_release_vollist(v);
	}

	/* select the first item */
	lvi.mask = LVIF_STATE;
	lvi.stateMask = LVIS_SELECTED;
	lvi.state = LVIS_SELECTED;
	(void)SendMessage(hList,LVM_SETITEMSTATE,0,(LRESULT)&lvi);
	
	/* scrollbar may appear/disappear after a scan */
	AdjustVolListColumns();
	
	job = get_first_selected_job();
	RedrawMap(job);
	if(job) UpdateStatusBar(&job->pi);
	
	WgxEnableWindows(hWindow,IDC_RESCAN,IDC_ANALYSE,
		IDC_DEFRAGM,IDC_OPTIMIZE,IDC_SHOWFRAGMENTED,0);
	return 0;
}

static void VolListAddItem(int index, volume_info *v)
{
	volume_processing_job *job;
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

	job = get_job(v->letter);
	VolListUpdateStatusFieldInternal(index,job);
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

static void VolListUpdateStatusFieldInternal(int index,volume_processing_job *job)
{
	LV_ITEMW lviw;

	lviw.mask = LVIF_TEXT;
	lviw.iItem = index;
	lviw.iSubItem = 1;
	
	if(job->pi.completion_status < 0){
		lviw.pszText = L"";
	} else if(job->pi.completion_status == 0){
		lviw.pszText = WgxGetResourceString(i18n_table,L"STATUS_RUNNING");
	} else {
		switch(job->job_type){
		case ANALYSIS_JOB:
			lviw.pszText = WgxGetResourceString(i18n_table,L"STATUS_ANALYSED");
			break;
		case DEFRAG_JOB:
			lviw.pszText = WgxGetResourceString(i18n_table,L"STATUS_DEFRAGMENTED");
			break;
		case OPTIMIZER_JOB:
			lviw.pszText = WgxGetResourceString(i18n_table,L"STATUS_OPTIMIZED");
			break;
		default:
			lviw.pszText = L"";
		}
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

	for(i = 0; i < 5; i++){
		user_defined_column_widths[i] = \
			(int)(LONG_PTR)SendMessage(hList,LVM_GETCOLUMNWIDTH,i,0);
	}
}

void VolListUpdateStatusField(volume_processing_job *job)
{
	LV_ITEM lvi;
	char buffer[128];
	int index = -1;
	int item;

	if(job == NULL) return;

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
			if(udefrag_tolower(buffer[0]) == udefrag_tolower(job->volume_letter)){
				VolListUpdateStatusFieldInternal(index,job);
				return;
			}
		}
	}
}

void VolListRefreshItem(volume_processing_job *job)
{
	LV_ITEM lvi;
	char buffer[128];
	int index = -1;
	int item;
	volume_info v;

	if(job == NULL)
		return;

	if(udefrag_get_volume_information(job->volume_letter,&v) < 0)
		return;
	
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
			if(udefrag_tolower(buffer[0]) == udefrag_tolower(job->volume_letter)){
				AddCapacityInformation(index,&v);
				return;
			}
		}
	}
}
