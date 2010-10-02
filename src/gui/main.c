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

/**
 * @file main.c
 * @brief Main window.
 * @addtogroup MainWindow
 * @{
 */

#include "main.h"

/* global variables */
HINSTANCE hInstance;
HWND hWindow = NULL;
HWND hMap;
WGX_FONT wgxFont = {{0},0};
RECT win_rc; /* coordinates of main window */
RECT r_rc;   /* coordinates of restored window */
signed int delta_h = 0;
double pix_per_dialog_unit = PIX_PER_DIALOG_UNIT_96DPI;

int shutdown_flag = FALSE;
extern int init_maximized_window;
extern int allow_map_redraw;

/* forward declaration */
static void UpdateWebStatistics(void);

/**
 * @brief Entry point.
 */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	int result;
	
	hInstance = GetModuleHandle(NULL);
    
	GetPrefs();

	if(strstr(lpCmdLine,"--setup")){
		/* create default guiopts.lua file */
		SavePrefs();
		DeleteEnvironmentVariables();
		return 0;
	}
	
	UpdateWebStatistics();
	CheckForTheNewVersion();

	/*
	* This call needs on dmitriar's pc (on xp) no more than 550 cpu tacts,
	* but InitCommonControlsEx() needs about 90000 tacts.
	* Because the first function is just a stub on xp.
	*/
	InitCommonControls();
	
	if(init_jobs() < 0){
		DeleteEnvironmentVariables();
		return 2;
	}
	
	if(DialogBox(hInstance,MAKEINTRESOURCE(IDD_MAIN),NULL,(DLGPROC)DlgProc) == (-1)){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"Cannot create the main window");
		DeleteEnvironmentVariables();
		release_jobs();
		return 3;
	}
	
	/* release all resources */
	release_jobs();
	ReleaseVolList();
	ReleaseMap();
	WgxDestroyFont(&wgxFont);

	/* save settings */
	SavePrefs();
	DeleteEnvironmentVariables();
	
	if(shutdown_flag){
		result = ShutdownOrHibernate();
		WgxDestroyResourceTable(i18n_table);
		return result;
	}
    
	WgxDestroyResourceTable(i18n_table);
	return 0;
}

void InitMainWindow(void)
{
	int dx,dy;
	BOOLEAN coord_undefined = FALSE;
	int s_width, s_height;
	RECT rc;
	udefrag_progress_info pi;
	LV_ITEM lvi;
    
	(void)WgxAddAccelerators(hInstance,hWindow,IDR_ACCELERATOR1);
	if(WgxBuildResourceTable(i18n_table,L".\\ud_i18n.lng"))
		WgxApplyResourceTable(i18n_table,hWindow);
	if(hibernate_instead_of_shutdown){
		WgxSetText(GetDlgItem(hWindow,IDC_SHUTDOWN),i18n_table,L"HIBERNATE_PC_AFTER_A_JOB");
	}
	WgxSetIcon(hInstance,hWindow,IDI_APP);

	if(skip_removable)
		(void)SendMessage(GetDlgItem(hWindow,IDC_SKIPREMOVABLE),BM_SETCHECK,BST_CHECKED,0);
	else
		(void)SendMessage(GetDlgItem(hWindow,IDC_SKIPREMOVABLE),BM_SETCHECK,BST_UNCHECKED,0);

	InitVolList();
	InitProgress();
	InitMap();

	if(r_rc.left == UNDEFINED_COORD || r_rc.top == UNDEFINED_COORD)
		coord_undefined = TRUE;
	WgxCheckWindowCoordinates(&r_rc,130,50);

	delta_h = GetSystemMetrics(SM_CYCAPTION) - 0x13;
	if(delta_h < 0) delta_h = 0;

    if (scale_by_dpi) {
        rc.top = rc.left = 0;
        rc.right = rc.bottom = 100;
        if(MapDialogRect(hWindow,&rc))
            pix_per_dialog_unit = (double)(rc.right - rc.left) / 100;
    }
	
	CreateStatusBar();
	InitFont();
	WgxSetFont(hWindow,&wgxFont);

	if(coord_undefined || restore_default_window_size){
		/* center default sized window on the screen */
		dx = DPI(DEFAULT_WIDTH);
		dy = DPI(DEFAULT_HEIGHT) + delta_h;
		s_width = GetSystemMetrics(SM_CXSCREEN);
		s_height = GetSystemMetrics(SM_CYSCREEN);
		if(s_width < dx || s_height < dy){
			r_rc.left = r_rc.top = 0;
		} else {
			r_rc.left = (s_width - dx) / 2;
			r_rc.top = (s_height - dy) / 2;
		}
		r_rc.right = r_rc.left + dx;
		r_rc.bottom = r_rc.top + dy - delta_h;
		SetWindowPos(hWindow,0,r_rc.left,r_rc.top,dx,dy,0);
		restore_default_window_size = 0; /* because already done */
	} else {
		dx = r_rc.right - r_rc.left;
		dy = r_rc.bottom - r_rc.top + delta_h;
		SetWindowPos(hWindow,0,r_rc.left,r_rc.top,dx,dy,0);
	}
	memcpy((void *)&win_rc,(void *)&r_rc,sizeof(RECT));
	
	/* force list of volumes to be resized properly */
	lvi.iItem = 0;
	lvi.iSubItem = 1;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE;
	lvi.pszText = "hi";
	lvi.iImage = 0;
	(void)SendMessage(hList,LVM_INSERTITEM,0,(LRESULT)&lvi);
	
	ResizeMainWindow();

	/* maximize window if required */
	if(init_maximized_window)
		SendMessage(hWindow,(WM_USER + 1),0,0);
	
	UpdateVolList(); /* after a complete map initialization! */

	/* reset status bar */
	memset(&pi,0,sizeof(udefrag_progress_info));
	UpdateStatusBar(&pi);
}

static RECT prev_rc = {0,0,0,0};

/**
 * @brief Adjust positions of controls
 * in accordance with main window dimensions.
 */
void ResizeMainWindow(void)
{
	int vlist_height, cmap_height, button_height;
	int progress_height, sbar_height;
	
	int vlist_width, cmap_width;
	int cmap_label_width, skip_media_width, rescan_btn_width;
	int button_width, progress_label_width, progress_width;
	
	int spacing;
	int padding_x;
	int padding_y;
	
	RECT rc;
	int w, h;
	int offset_y, offset_x;
	int cmap_offset;
	int lines, lw, i;
	int buttons_offset;
	
	int cw;
	HWND hChild;
	
	/*
	* Assign layout variables by layout constants
	* with the amendment to the current DPI.
	*/
	spacing = DPI(SPACING);
	padding_x = DPI(PADDING_X);
	padding_y = DPI(PADDING_Y);
	button_width = DPI(BUTTON_WIDTH);
	button_height = DPI(BUTTON_HEIGHT);
	vlist_height = DPI(VLIST_HEIGHT);
	cmap_label_width = DPI(CMAP_LABEL_WIDTH);
	skip_media_width = DPI(SKIP_MEDIA_WIDTH);
	rescan_btn_width = DPI(RESCAN_BTN_WIDTH);
	progress_label_width = DPI(PROGRESS_LABEL_WIDTH);
	progress_height = DPI(PROGRESS_HEIGHT);
	
	/* get dimensions of the client area of the main window */
	if(!GetClientRect(hWindow,&rc)){
		WgxDbgPrint("GetClientRect failed in RepositionMainWindowControls()!\n");
		return;
	}
	
	w = rc.right - rc.left;
	h = rc.bottom - rc.top;
	cw = w - padding_x * 2;
	offset_y = padding_y;

	/* prevent resizing if the current size is equal to previous */
	if((prev_rc.right - prev_rc.left == w) && (prev_rc.bottom - prev_rc.top == h))
		return; /* this usually encounters when user minimizes window and then restores it */
	memcpy((void *)&prev_rc,(void *)&rc,sizeof(RECT));
	
	//SendMessage(hWindow,WM_SETREDRAW,FALSE,0);
	allow_map_redraw = 0;

	/* reposition the volume list */
    if(cmap_label_width + skip_media_width + rescan_btn_width > cw) vlist_height = vlist_height / 2;
	vlist_width = cw;
	vlist_height = ResizeVolList(padding_x,offset_y,vlist_width,vlist_height);
	
	offset_y += vlist_height + spacing;
	
	/* redraw controls below the volume list */
	if(cmap_label_width + skip_media_width + rescan_btn_width > cw){
		/* put volume list related controls firstly, then the cluster map label */
		offset_x = padding_x;
		(void)SetWindowPos(GetDlgItem(hWindow,IDC_SKIPREMOVABLE),0,offset_x,offset_y,skip_media_width,button_height,0);
		offset_x += skip_media_width;
		if(offset_x + rescan_btn_width > padding_x + cw){
			offset_x = padding_x;
			offset_y += button_height + spacing;
		} else {
			offset_x = padding_x + cw - rescan_btn_width;
		}
		(void)SetWindowPos(GetDlgItem(hWindow,IDC_RESCAN),0,offset_x,offset_y,rescan_btn_width,button_height,0);
		offset_x = padding_x;
		offset_y += button_height + spacing;
		(void)SetWindowPos(GetDlgItem(hWindow,IDC_CL_MAP_STATIC),0,offset_x,offset_y,cmap_label_width,button_height,0);
		offset_y += button_height + spacing;
	} else {
		/* put the cluster map label, then volume list related controls */
		offset_x = padding_x;
		(void)SetWindowPos(GetDlgItem(hWindow,IDC_CL_MAP_STATIC),0,offset_x,offset_y,cmap_label_width,button_height,0);
		offset_x = padding_x + cw - rescan_btn_width - skip_media_width;
		(void)SetWindowPos(GetDlgItem(hWindow,IDC_SKIPREMOVABLE),0,offset_x,offset_y,skip_media_width,button_height,0);
		offset_x = padding_x + cw - rescan_btn_width;
		(void)SetWindowPos(GetDlgItem(hWindow,IDC_RESCAN),0,offset_x,offset_y,rescan_btn_width,button_height,0);
		offset_y += button_height + spacing;
	}

	/* save cluster map offset */
	cmap_offset = offset_y;
	
	/* reposition the status bar */
	offset_y = h;
	sbar_height = ResizeStatusBar(h,w);
	offset_y -= sbar_height;
	offset_y -= spacing;
	
	/* set progress indicator coordinates */
	offset_y -= button_height;
	offset_x = padding_x;
	(void)SetWindowPos(GetDlgItem(hWindow,IDC_PROGRESSMSG),0,offset_x,offset_y,progress_label_width,button_height,0);
	offset_x += progress_label_width + spacing;
	progress_width = cw - progress_label_width - spacing;
	(void)SetWindowPos(GetDlgItem(hWindow,IDC_PROGRESS1),0,offset_x,
		offset_y + (button_height - progress_height) / 2,progress_width,
		progress_height,0);
	offset_y -= spacing;
	
	/* set buttons above the progress indicator */
	lines = 1;
	lw = cw;
	/* for the first 7 buttons */
	for(i = 0; i < 6; i++){
		lw -= button_width + spacing;
		if(button_width > lw) lines ++, lw = cw;
	}
	/* for the shutdown checkbox */
	lw -= button_width + spacing;
	if(button_width * 3 + spacing * 2 > lw) lines ++;
	
	/* draw lines of controls */
	offset_y -= lines * button_height + (lines - 1) * spacing;
	buttons_offset = offset_y;
	offset_x = padding_x;
	(void)SetWindowPos(GetDlgItem(hWindow,IDC_ANALYSE),0,offset_x,offset_y,button_width,button_height,0);
	offset_x += button_width + spacing;
	if(offset_x + button_width > padding_x + cw){
		offset_x = padding_x;
		offset_y += button_height + spacing;
	}
	(void)SetWindowPos(GetDlgItem(hWindow,IDC_DEFRAGM),0,offset_x,offset_y,button_width,button_height,0);
	offset_x += button_width + spacing;
	if(offset_x + button_width > padding_x + cw){
		offset_x = padding_x;
		offset_y += button_height + spacing;
	}
	(void)SetWindowPos(GetDlgItem(hWindow,IDC_OPTIMIZE),0,offset_x,offset_y,button_width,button_height,0);
	offset_x += button_width + spacing;
	if(offset_x + button_width > padding_x + cw){
		offset_x = padding_x;
		offset_y += button_height + spacing;
	}
	(void)SetWindowPos(GetDlgItem(hWindow,IDC_STOP),0,offset_x,offset_y,button_width,button_height,0);
	offset_x += button_width + spacing;
	if(offset_x + button_width > padding_x + cw){
		offset_x = padding_x;
		offset_y += button_height + spacing;
	}
	(void)SetWindowPos(GetDlgItem(hWindow,IDC_SHOWFRAGMENTED),0,offset_x,offset_y,button_width,button_height,0);
	offset_x += button_width + spacing;
	if(offset_x + button_width > padding_x + cw){
		offset_x = padding_x;
		offset_y += button_height + spacing;
	}
	(void)SetWindowPos(GetDlgItem(hWindow,IDC_SETTINGS),0,offset_x,offset_y,button_width,button_height,0);
	offset_x += button_width + spacing;
	if(offset_x + button_width > padding_x + cw){
		offset_x = padding_x;
		offset_y += button_height + spacing;
	}
	(void)SetWindowPos(GetDlgItem(hWindow,IDC_ABOUT),0,offset_x,offset_y,button_width,button_height,0);
	offset_x += button_width + spacing;
	if(offset_x + button_width * 3 + spacing * 2 > padding_x + cw){
		offset_x = padding_x;
		offset_y += button_height + spacing;
	}
	(void)SetWindowPos(GetDlgItem(hWindow,IDC_SHUTDOWN),0,offset_x,offset_y,
		button_width * 3 + spacing * 2,button_height,0);

	/* adjust cluster map */
	cmap_width = cw;
	cmap_height = buttons_offset - cmap_offset - spacing;
	if(cmap_height < 0) cmap_height = 0;
	ResizeMap(padding_x,cmap_offset,cmap_width,cmap_height);
	
	//SendMessage(hWindow,WM_SETREDRAW,TRUE,0);
	hChild = GetWindow(hWindow,GW_CHILD);
	while(hChild){
		if(hChild != hList && hChild != hMap && hChild != hStatus)
			(void)InvalidateRect(hChild,NULL,TRUE);
		hChild = GetWindow(hChild,GW_HWNDNEXT);
	}
	
	allow_map_redraw = 1;
}

/**
 * @brief Updates the global win_rc structure.
 */
BOOL UpdateMainWindowCoordinates(void)
{
	RECT rc;

	if(GetWindowRect(hWindow,&rc)){
		if((HIWORD(rc.bottom)) != 0xffff){
			rc.bottom -= delta_h;
			memcpy((void *)&win_rc,(void *)&rc,sizeof(RECT));
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * @brief Main window procedure.
 */
BOOL CALLBACK DlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	MINMAXINFO *mmi;
	BOOL size_changed;
	RECT rc;
	
	switch(msg){
	case WM_INITDIALOG:
		hWindow = hWnd;
		InitMainWindow();
		break;
	case WM_NOTIFY:
		VolListNotifyHandler(lParam);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_ANALYSE:
			start_selected_jobs(ANALYSIS_JOB);
			break;
		case IDC_DEFRAGM:
			start_selected_jobs(DEFRAG_JOB);
			break;
		case IDC_OPTIMIZE:
			start_selected_jobs(OPTIMIZER_JOB);
			break;
		case IDC_ABOUT:
			if(DialogBox(hInstance,MAKEINTRESOURCE(IDD_ABOUT),hWindow,(DLGPROC)AboutDlgProc) == (-1))
				WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"UltraDefrag: cannot create the About window!");
			break;
		case IDC_SETTINGS:
			if(!busy_flag)
				OpenConfigurationDialog();
			break;
		case IDC_SKIPREMOVABLE:
			skip_removable = (
				SendMessage(GetDlgItem(hWindow,IDC_SKIPREMOVABLE),
					BM_GETCHECK,0,0) == BST_CHECKED
				);
			break;
		case IDC_SHOWFRAGMENTED:
			if(!busy_flag)
				ShowReports();
			break;
		case IDC_PAUSE:
		case IDC_STOP:
			stop_all_jobs();
			break;
		case IDC_RESCAN:
			if(!busy_flag)
				UpdateVolList();
			break;
		case IDC_SHUTDOWN:
			if(show_shutdown_check_confirmation_dialog){
				if(SendMessage(GetDlgItem(hWindow,IDC_SHUTDOWN),BM_GETCHECK,0,0) == BST_CHECKED){
					if(DialogBox(hInstance,MAKEINTRESOURCE(IDD_CHECK_CONFIRM),hWindow,(DLGPROC)CheckConfirmDlgProc) == 0)
						SendMessage(GetDlgItem(hWindow,IDC_SHUTDOWN),BM_SETCHECK,(WPARAM)BST_UNCHECKED,0);
				}
			}
		}
		break;
	case WM_SIZE:
		if(wParam == SIZE_MAXIMIZED)
			maximized_window = 1;
		else if(wParam != SIZE_MINIMIZED)
			maximized_window = 0;
		if(UpdateMainWindowCoordinates()){
			if(!maximized_window)
				memcpy((void *)&r_rc,(void *)&win_rc,sizeof(RECT));
			ResizeMainWindow();
		} else {
			WgxDbgPrint("Wrong window dimensions on WM_SIZE message!\n");
		}
		break;
	case (WM_USER + 1):
		ShowWindow(hWnd,SW_MAXIMIZE);
		break;
	case WM_GETMINMAXINFO:
		mmi = (MINMAXINFO *)lParam;
		/* set min size to avoid overlaying controls */
		mmi->ptMinTrackSize.x = DPI(MIN_WIDTH);
		mmi->ptMinTrackSize.y = DPI(MIN_HEIGHT);
		break;
	case WM_MOVE:
		size_changed = TRUE;
		if(GetWindowRect(hWindow,&rc)){
			if((HIWORD(rc.bottom)) != 0xffff){
				rc.bottom -= delta_h;
				if((rc.right - rc.left == win_rc.right - win_rc.left) &&
					(rc.bottom - rc.top == win_rc.bottom - win_rc.top))
						size_changed = FALSE;
			}
		}
		(void)UpdateMainWindowCoordinates();
		if(!maximized_window && !size_changed)
			memcpy((void *)&r_rc,(void *)&win_rc,sizeof(RECT));
		break;
	case WM_CLOSE:
		(void)UpdateMainWindowCoordinates();
		if(!maximized_window)
			memcpy((void *)&r_rc,(void *)&win_rc,sizeof(RECT));
		VolListGetColumnWidths();
		exit_pressed = 1;
		stop_all_jobs();
		(void)EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

/**
 * @brief Updates web statistics of the program use.
 */
static void UpdateWebStatistics(void)
{
#ifndef _WIN64
	IncreaseGoogleAnalyticsCounterAsynch("ultradefrag.sourceforge.net","/appstat/gui-x86.html","UA-15890458-1");
#else
	#if defined(_IA64_)
		IncreaseGoogleAnalyticsCounterAsynch("ultradefrag.sourceforge.net","/appstat/gui-ia64.html","UA-15890458-1");
	#else
		IncreaseGoogleAnalyticsCounterAsynch("ultradefrag.sourceforge.net","/appstat/gui-x64.html","UA-15890458-1");
	#endif
#endif
}

/** @} */
