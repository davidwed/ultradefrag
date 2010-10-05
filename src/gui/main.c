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
char class_name[64];
HWND hWindow = NULL;
HWND hList = NULL;
HWND hMap = NULL;
WGX_FONT wgxFont = {{0},0};
RECT win_rc; /* coordinates of main window */
RECT r_rc;   /* coordinates of restored window */
double pix_per_dialog_unit = PIX_PER_DIALOG_UNIT_96DPI;

int shutdown_flag = 0;
int shutdown_requested = 0;
int boot_time_defrag_enabled = 0;
extern int init_maximized_window;
extern int allow_map_redraw;

/* forward declarations */
static void UpdateWebStatistics(void);
LRESULT CALLBACK MainWindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

/**
 * @brief Registers main window class.
 * @return Zero for success, negative value otherwise.
 */
static int RegisterMainWindowClass(void)
{
	WNDCLASSEX wc;
	
	/* make it unique to be able to center configuration dialog over */
	_snprintf(class_name,sizeof(class_name),"udefrag-gui-%u",(int)GetCurrentProcessId());
	class_name[sizeof(class_name) - 1] = 0;
	
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = MainWindowProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = NULL;
	wc.hIconSm       = NULL;
	wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
	/* TODO: do it more accurately */
	wc.hbrBackground = (HBRUSH)(COLOR_MENU + 1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = class_name;
	
	if(!RegisterClassEx(&wc)){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
			"RegisterMainWindowClass failed!");
		return (-1);
	}
	return 0;
}

/**
 * @brief Defines initial
 * coordinates of main window.
 */
static void InitMainWindowCoordinates(void)
{
	int center_on_the_screen = 0;
	int screen_width, screen_height;
	int width, height;
	RECT rc;

    if(scale_by_dpi){
        rc.top = rc.left = 0;
        rc.right = rc.bottom = 100;
        if(MapDialogRect(hWindow,&rc))
            pix_per_dialog_unit = (double)(rc.right - rc.left) / 100;
    }

	if(r_rc.left == UNDEFINED_COORD)
		center_on_the_screen = 1;
	else if(r_rc.top == UNDEFINED_COORD)
		center_on_the_screen = 1;
	else if(restore_default_window_size)
		center_on_the_screen = 1;
	
	if(center_on_the_screen){
		/* center default sized window on the screen */
		width = DPI(DEFAULT_WIDTH);
		height = DPI(DEFAULT_HEIGHT);
		screen_width = GetSystemMetrics(SM_CXSCREEN);
		screen_height = GetSystemMetrics(SM_CYSCREEN);
		if(screen_width < width || screen_height < height){
			r_rc.left = r_rc.top = 0;
		} else {
			r_rc.left = (screen_width - width) / 2;
			r_rc.top = (screen_height - height) / 2;
		}
		r_rc.right = r_rc.left + width;
		r_rc.bottom = r_rc.top + height;
		restore_default_window_size = 0; /* because already done */
	}

	WgxCheckWindowCoordinates(&r_rc,130,50);
	memcpy((void *)&win_rc,(void *)&r_rc,sizeof(RECT));
}

static RECT prev_rc = {0,0,0,0};

/**
 * @brief Adjust positions of controls
 * in accordance with main window dimensions.
 */
void new_ResizeMainWindow(int force)
{
	int vlist_height, sbar_height;
	int spacing;
	RECT rc;
	int w, h;
	
	if(!GetClientRect(hWindow,&rc)){
		WgxDbgPrint("GetClientRect failed in RepositionMainWindowControls()!\n");
		return;
	}
	
	vlist_height = DPI(VLIST_HEIGHT);
	spacing = DPI(SPACING);
	w = rc.right - rc.left;
	h = rc.bottom - rc.top;

	/* prevent resizing if the current size is equal to previous */
	if(!force && (prev_rc.right - prev_rc.left == w) && (prev_rc.bottom - prev_rc.top == h))
		return; /* this usually encounters when user minimizes window and then restores it */
	memcpy((void *)&prev_rc,(void *)&rc,sizeof(RECT));
	
	vlist_height = ResizeVolList(0,0,w,vlist_height);
	sbar_height = ResizeStatusBar(h,w);
	ResizeMap(0,vlist_height,w,h - vlist_height - sbar_height);
}

/**
 * @brief Creates main window.
 * @return Zero for success,
 * negative value otherwise.
 */
int CreateMainWindow(int nShowCmd)
{
	char *caption = MAIN_CAPTION;
	HACCEL hAccelTable;
	MSG msg;
	udefrag_progress_info pi;
	
	/* register class */
	if(RegisterMainWindowClass() < 0)
		return (-1);
	
	/* create main window */
	InitMainWindowCoordinates();
	hWindow = CreateWindowEx(
			WS_EX_APPWINDOW,
			class_name,caption,
			WS_OVERLAPPEDWINDOW,
			win_rc.left,win_rc.top,
			win_rc.right - win_rc.left,
			win_rc.bottom - win_rc.top,
			NULL,NULL,hInstance,NULL);
	if(hWindow == NULL){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
			"Cannot create main window!");
		return (-1);
	}
	
	/* create menu */
	if(CreateMainMenu() < 0)
		return (-1);
	
	/* create toolbar */
	if(CreateToolbar() < 0)
		return (-1);
	
	/* create controls */
	hList = CreateWindowEx(WS_EX_CLIENTEDGE,
			"SysListView32","",
			WS_CHILD | WS_VISIBLE \
			| LVS_REPORT | LVS_SHOWSELALWAYS \
			| LVS_NOSORTHEADER,
			0,0,100,100,
			hWindow,NULL,hInstance,NULL);
	if(hList == NULL){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
			"Cannot create volume list control!");
		return (-1);
	}

	hMap = CreateWindowEx(WS_EX_CLIENTEDGE,
			"Static","",
			WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
			0,100,100,100,
			hWindow,NULL,hInstance,NULL);
	if(hMap == NULL){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
			"Cannot create cluster map control!");
		return (-1);
	}
	
	CreateStatusBar();
	if(hStatus == NULL)
		return (-1);
	
	/* set font */
	InitFont();
	WgxSetFont(hWindow,&wgxFont);

	/* initialize controls */
	InitVolList();
	InitMap();

	/* maximize window if required */
	if(init_maximized_window)
		SendMessage(hWindow,(WM_USER + 1),0,0);
	
	/* resize controls */
	new_ResizeMainWindow(1);
	
	/* fill list of volumes */
	UpdateVolList(); /* after a complete map initialization! */

	/* reset status bar */
	memset(&pi,0,sizeof(udefrag_progress_info));
	UpdateStatusBar(&pi);
	
	WgxSetIcon(hInstance,hWindow,IDI_APP);
	SetFocus(hList);
	ShowWindow(hWindow,init_maximized_window ? SW_MAXIMIZE : nShowCmd);
	UpdateWindow(hWindow);

	/* load accelerators */
	hAccelTable = LoadAccelerators(hInstance,MAKEINTRESOURCE(IDR_MAIN_ACCELERATOR));
	if(hAccelTable == NULL){
		WgxDbgPrintLastError("CreateMainWindow: accelerators cannot be loaded");
	}
	
	/* go to the message loop */
	while(GetMessage(&msg,NULL,0,0)){
		if(!TranslateAccelerator(hWindow,hAccelTable,&msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}

/**
 * @brief Opens web page of the handbook,
 * either from local storage or from the network.
 */
static void OpenWebPage(char *page)
{
	short path[MAX_PATH];
	HINSTANCE hApp;
	
	(void)_snwprintf(path,MAX_PATH,L".\\handbook\\%hs",page);
	path[MAX_PATH - 1] = 0;

	hApp = ShellExecuteW(hWindow,L"open",path,NULL,NULL,SW_SHOW);
	if((int)(LONG_PTR)hApp <= 32){
		(void)_snwprintf(path,MAX_PATH,L"http://ultradefrag.sourceforge.net/handbook/%hs",page);
		path[MAX_PATH - 1] = 0;
		(void)WgxShellExecuteW(hWindow,L"open",path,NULL,NULL,SW_SHOW);
	}
}

/**
 * @brief Updates the global win_rc structure.
 * @return Zero for success, negative value otherwise.
 */
int UpdateMainWindowCoordinates(void)
{
	RECT rc;

	if(GetWindowRect(hWindow,&rc)){
		if((HIWORD(rc.bottom)) != 0xffff){
			memcpy((void *)&win_rc,(void *)&rc,sizeof(RECT));
			return 0;
		}
	}
	return (-1);
}

/**
 * @brief Main window procedure.
 */
LRESULT CALLBACK MainWindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	MINMAXINFO *mmi;
	BOOL size_changed;
	RECT rc;
	short path[MAX_PATH];
	CHOOSEFONT cf;

	switch(uMsg){
	case WM_CREATE:
		/* initialize main window */
		return 0;
	case WM_NOTIFY:
		VolListNotifyHandler(lParam);
		return 0;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		/* Action menu handlers */
		case IDM_ANALYZE:
			start_selected_jobs(ANALYSIS_JOB);
			return 0;
		case IDM_DEFRAG:
			start_selected_jobs(DEFRAG_JOB);
			return 0;
		case IDM_OPTIMIZE:
			start_selected_jobs(OPTIMIZER_JOB);
			return 0;
		case IDM_STOP:
			stop_all_jobs();
			return 0;
		case IDM_IGNORE_REMOVABLE_MEDIA:
			if(skip_removable){
				skip_removable = 0;
				CheckMenuItem(hMainMenu,
					IDM_IGNORE_REMOVABLE_MEDIA,
					MF_BYCOMMAND | MF_UNCHECKED);
			} else {
				skip_removable = 1;
				CheckMenuItem(hMainMenu,
					IDM_IGNORE_REMOVABLE_MEDIA,
					MF_BYCOMMAND | MF_CHECKED);
			}
		case IDM_RESCAN:
			if(!busy_flag)
				UpdateVolList();
			return 0;
		case IDM_SHUTDOWN:
			if(shutdown_flag){
				shutdown_flag = 0;
				CheckMenuItem(hMainMenu,
					IDM_SHUTDOWN,MF_BYCOMMAND | MF_UNCHECKED);
			} else {
				if(show_shutdown_check_confirmation_dialog){
					if(DialogBox(hInstance,MAKEINTRESOURCE(IDD_CHECK_CONFIRM),
					  hWindow,(DLGPROC)CheckConfirmDlgProc) != 0)
					{
						shutdown_flag = 1;
						CheckMenuItem(hMainMenu,
							IDM_SHUTDOWN,MF_BYCOMMAND | MF_CHECKED);
					}
				} else {
					shutdown_flag = 1;
					CheckMenuItem(hMainMenu,
						IDM_SHUTDOWN,MF_BYCOMMAND | MF_CHECKED);
				}
			}
			return 0;
		case IDM_EXIT:
			goto done;
		/* Reports menu handler */
		case IDM_SHOW_REPORT:
			if(!busy_flag)
				ShowReports();
			return 0;
		/* Settings menu handlers */
		case IDM_CFG_GUI_FONT:
			memset(&cf,0,sizeof(cf));
			cf.lStructSize = sizeof(CHOOSEFONT);
			cf.lpLogFont = &wgxFont.lf;
			cf.Flags = CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;
			cf.hwndOwner = hWindow;
			if(ChooseFont(&cf)){
				WgxDestroyFont(&wgxFont);
				if(WgxCreateFont("",&wgxFont)){
					WgxSetFont(hWindow,&wgxFont);
					new_ResizeMainWindow(1);
					WgxSaveFont(".\\options\\font.lua",&wgxFont);
				}
			}
			return 0;
		case IDM_CFG_GUI_SETTINGS:
			#ifndef UDEFRAG_PORTABLE
			(void)WgxShellExecuteW(hWindow,L"Edit",L".\\options\\guiopts.lua",NULL,NULL,SW_SHOW);
			#else
			(void)WgxShellExecuteW(hWindow,L"open",L"notepad.exe",L".\\options\\guiopts.lua",NULL,SW_SHOW);
			#endif
			return 0;
		case IDM_CFG_BOOT_ENABLE:
			if(!GetWindowsDirectoryW(path,MAX_PATH)){
				WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot retrieve the Windows directory path!");
			} else {
				if(boot_time_defrag_enabled){
					boot_time_defrag_enabled = 0;
					CheckMenuItem(hMainMenu,
						IDM_CFG_BOOT_ENABLE,
						MF_BYCOMMAND | MF_UNCHECKED);
					(void)wcscat(path,L"\\System32\\boot-off.cmd");
				} else {
					boot_time_defrag_enabled = 1;
					CheckMenuItem(hMainMenu,
						IDM_CFG_BOOT_ENABLE,
						MF_BYCOMMAND | MF_CHECKED);
					(void)wcscat(path,L"\\System32\\boot-on.cmd");
				}
				(void)WgxShellExecuteW(hWindow,L"open",path,NULL,NULL,SW_HIDE);
			}
			return 0;
		case IDM_CFG_BOOT_SCRIPT:
			if(!GetWindowsDirectoryW(path,MAX_PATH)){
				WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot retrieve the Windows directory path");
			} else {
				(void)wcscat(path,L"\\System32\\ud-boot-time.cmd");
				(void)WgxShellExecuteW(hWindow,L"edit",path,NULL,NULL,SW_SHOW);
			}
			return 0;
		case IDM_CFG_REPORTS:
			#ifndef UDEFRAG_PORTABLE
			(void)WgxShellExecuteW(hWindow,L"Edit",L".\\options\\udreportopts.lua",NULL,NULL,SW_SHOW);
			#else
			(void)WgxShellExecuteW(hWindow,L"open",L"notepad.exe",L".\\options\\udreportopts.lua",NULL,SW_SHOW);
			#endif
			return 0;
		/* Help menu handlers */
		case IDM_CONTENTS:
			OpenWebPage("index.html");
			return 0;
		case IDM_BEST_PRACTICE:
			OpenWebPage("Tips.html");
			return 0;
		case IDM_FAQ:
			OpenWebPage("FAQ.html");
			return 0;
		case IDM_ABOUT:
			if(DialogBox(hInstance,MAKEINTRESOURCE(IDD_ABOUT),hWindow,(DLGPROC)AboutDlgProc) == (-1))
				WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot create the About window!");
			return 0;
		}
		break;
	case WM_SIZE:
		/* resize main window and its controls */
		if(wParam == SIZE_MAXIMIZED)
			maximized_window = 1;
		else if(wParam != SIZE_MINIMIZED)
			maximized_window = 0;
		if(UpdateMainWindowCoordinates() >= 0){
			if(!maximized_window)
				memcpy((void *)&r_rc,(void *)&win_rc,sizeof(RECT));
			new_ResizeMainWindow(0);
		} else {
			WgxDbgPrint("Wrong window dimensions on WM_SIZE message!\n");
		}
		return 0;
	case WM_GETMINMAXINFO:
		/* set min size to avoid overlaying controls */
		mmi = (MINMAXINFO *)lParam;
		mmi->ptMinTrackSize.x = DPI(MIN_WIDTH);
		mmi->ptMinTrackSize.y = DPI(MIN_HEIGHT);
		return 0;
	case WM_MOVE:
		/* update coordinates of the main window */
		size_changed = TRUE;
		if(GetWindowRect(hWindow,&rc)){
			if((HIWORD(rc.bottom)) != 0xffff){
				if((rc.right - rc.left == win_rc.right - win_rc.left) &&
					(rc.bottom - rc.top == win_rc.bottom - win_rc.top))
						size_changed = FALSE;
			}
		}
		UpdateMainWindowCoordinates();
		if(!maximized_window && !size_changed)
			memcpy((void *)&r_rc,(void *)&win_rc,sizeof(RECT));
		return 0;
	case (WM_USER + 1):
		/* maximize window */
		ShowWindow(hWnd,SW_MAXIMIZE);
		return 0;
	case WM_DESTROY:
		goto done;
	}
	return DefWindowProc(hWnd,uMsg,wParam,lParam);

done:
	UpdateMainWindowCoordinates();
	if(!maximized_window)
		memcpy((void *)&r_rc,(void *)&win_rc,sizeof(RECT));
	VolListGetColumnWidths();
	exit_pressed = 1;
	stop_all_jobs();
	PostQuitMessage(0);
	return 0;
}

/**
 * @brief Entry point.
 */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	int result;
	
	hInstance = GetModuleHandle(NULL);
    
	if(strstr(lpCmdLine,"--setup")){
		/* create default guiopts.lua file */
		GetPrefs();
		SavePrefs();
		DeleteEnvironmentVariables();
		return 0;
	}

	GetPrefs();
	WgxBuildResourceTable(i18n_table,L".\\ud_i18n.lng");
	UpdateWebStatistics();
	CheckForTheNewVersion();

	/*
	* This call needs on dmitriar's pc (on xp) no more than 550 cpu tacts,
	* but InitCommonControlsEx() needs about 90000 tacts.
	* Because the first function is just a stub on xp.
	*/
	InitCommonControls();
	
	if(init_jobs() < 0){
		WgxDestroyResourceTable(i18n_table);
		DeleteEnvironmentVariables();
		return 2;
	}
	
	/* track changes in guiopts.lua file; synchronized with map redraw */
	StartPrefsChangesTracking();
	StartBootExecChangesTracking();

#ifdef NEW_DESIGN
	if(CreateMainWindow(nShowCmd) < 0){
		WgxDestroyResourceTable(i18n_table);
		DeleteEnvironmentVariables();
		StopPrefsChangesTracking();
		StopBootExecChangesTracking();
		release_jobs();
		return 3;
	}
#else
	if(DialogBox(hInstance,MAKEINTRESOURCE(IDD_MAIN),NULL,(DLGPROC)DlgProc) == (-1)){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"Cannot create the main window!");
		DeleteEnvironmentVariables();
		StopPrefsChangesTracking();
		StopBootExecChangesTracking();
		release_jobs();
		return 3;
	}
#endif

	/* release all resources */
	StopPrefsChangesTracking();
	StopBootExecChangesTracking();
	release_jobs();
	ReleaseVolList();
	ReleaseMap();
	WgxDestroyFont(&wgxFont);

	/* save settings */
	SavePrefs();
	DeleteEnvironmentVariables();
	
	if(shutdown_requested){
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
    
	(void)WgxAddAccelerators(hInstance,hWindow,IDR_ACCELERATOR1);
	WgxApplyResourceTable(i18n_table,hWindow);
	if(hibernate_instead_of_shutdown){
		WgxSetText(GetDlgItem(hWindow,IDC_SHUTDOWN),i18n_table,L"HIBERNATE_PC_AFTER_A_JOB");
	}
	WgxSetIcon(hInstance,hWindow,IDI_APP);
	InitFont();
	WgxSetFont(hWindow,&wgxFont);

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

    if (scale_by_dpi) {
        rc.top = rc.left = 0;
        rc.right = rc.bottom = 100;
        if(MapDialogRect(hWindow,&rc))
            pix_per_dialog_unit = (double)(rc.right - rc.left) / 100;
    }
	
	CreateStatusBar();
	WgxSetFont(hStatus,&wgxFont);

	if(coord_undefined || restore_default_window_size){
		/* center default sized window on the screen */
		dx = DPI(DEFAULT_WIDTH);
		dy = DPI(DEFAULT_HEIGHT);
		s_width = GetSystemMetrics(SM_CXSCREEN);
		s_height = GetSystemMetrics(SM_CYSCREEN);
		if(s_width < dx || s_height < dy){
			r_rc.left = r_rc.top = 0;
		} else {
			r_rc.left = (s_width - dx) / 2;
			r_rc.top = (s_height - dy) / 2;
		}
		r_rc.right = r_rc.left + dx;
		r_rc.bottom = r_rc.top + dy;
		SetWindowPos(hWindow,0,r_rc.left,r_rc.top,dx,dy,0);
		restore_default_window_size = 0; /* because already done */
	} else {
		dx = r_rc.right - r_rc.left;
		dy = r_rc.bottom - r_rc.top;
		SetWindowPos(hWindow,0,r_rc.left,r_rc.top,dx,dy,0);
	}
	memcpy((void *)&win_rc,(void *)&r_rc,sizeof(RECT));
	
	ResizeMainWindow();

	/* maximize window if required */
	if(init_maximized_window)
		SendMessage(hWindow,(WM_USER + 1),0,0);
	
	UpdateVolList(); /* after a complete map initialization! */

	/* reset status bar */
	memset(&pi,0,sizeof(udefrag_progress_info));
	UpdateStatusBar(&pi);
}

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
				WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,"Cannot create the About window!");
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
		if(UpdateMainWindowCoordinates() >= 0){
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
				if((rc.right - rc.left == win_rc.right - win_rc.left) &&
					(rc.bottom - rc.top == win_rc.bottom - win_rc.top))
						size_changed = FALSE;
			}
		}
		UpdateMainWindowCoordinates();
		if(!maximized_window && !size_changed)
			memcpy((void *)&r_rc,(void *)&win_rc,sizeof(RECT));
		break;
	case WM_CLOSE:
		UpdateMainWindowCoordinates();
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
