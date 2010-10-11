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
	
	/* load i18n resources */
	ApplyLanguagePack(); // TODO: call it also when user selects another language
	BuildLanguageMenu(); // TODO: call it also when i18n folder contents becomes changed
	
	/* show main window on the screen */
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

	if(Init_I18N_Events() < 0){
		DeleteEnvironmentVariables();
		return 1;
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
		Destroy_I18N_Events();
		DeleteEnvironmentVariables();
		return 2;
	}
	
	/* track changes in guiopts.lua file; synchronized with map redraw */
	StartPrefsChangesTracking();
	StartBootExecChangesTracking();
	StartLangIniChangesTracking();
	StartI18nFolderChangesTracking();

	if(CreateMainWindow(nShowCmd) < 0){
		StopPrefsChangesTracking();
		StopBootExecChangesTracking();
		StopLangIniChangesTracking();
		StopI18nFolderChangesTracking();
		release_jobs();
		Destroy_I18N_Events();
		WgxDestroyResourceTable(i18n_table);
		DeleteEnvironmentVariables();
		return 3;
	}

	/* release all resources */
	StopPrefsChangesTracking();
	StopBootExecChangesTracking();
	StopLangIniChangesTracking();
	StopI18nFolderChangesTracking();
	release_jobs();
	ReleaseVolList();
	ReleaseMap();

	/* save settings */
	SavePrefs();
	DeleteEnvironmentVariables();
	
	if(shutdown_requested){
		result = ShutdownOrHibernate();
		WgxDestroyFont(&wgxFont);
		Destroy_I18N_Events();
		WgxDestroyResourceTable(i18n_table);
		return result;
	}
    
	WgxDestroyFont(&wgxFont);
	Destroy_I18N_Events();
	WgxDestroyResourceTable(i18n_table);
	return 0;
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
