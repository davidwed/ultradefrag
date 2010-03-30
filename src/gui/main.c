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
* GUI - main window code.
*/

#include "main.h"

/* Global variables */
HINSTANCE hInstance;
HWND hWindow = NULL;
HWND hMap;
signed int delta_h = 0;
HFONT hFont = NULL;
extern HWND hStatus;
extern HWND hList;
extern WGX_I18N_RESOURCE_ENTRY i18n_table[];
extern RECT win_rc; /* coordinates of main window */
extern int skip_removable;

int shutdown_flag = FALSE;
extern int hibernate_instead_of_shutdown;
extern int show_shutdown_check_confirmation_dialog;
extern int seconds_for_shutdown_rejection;

/* they have the same effect as environment variables for console program */
extern char in_filter[];
extern char ex_filter[];
extern char sizelimit[];
extern int refresh_interval;
extern int disable_reports;
extern char dbgprint_level[];

extern BOOL busy_flag, exit_pressed;
extern NEW_VOLUME_LIST_ENTRY vlist[MAX_DOS_DRIVES + 1];

/* Function prototypes */
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void ShowReports();
void ShowSingleReport(NEW_VOLUME_LIST_ENTRY *v_entry);
void VolListGetColumnWidths(void);
NEW_VOLUME_LIST_ENTRY * vlist_get_first_selected_entry(void);
NEW_VOLUME_LIST_ENTRY * vlist_get_entry(char *name);
void InitVolList(void);
void FreeVolListResources(void);
void UpdateVolList(void);
void VolListNotifyHandler(LPARAM lParam);

void InitFont(void);
void CallGUIConfigurator(void);
void DeleteEnvironmentVariables(void);
BOOL CALLBACK CheckConfirmDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK ShutdownConfirmDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);

void OpenWebPage(char *page)
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

void DisplayLastError(char *caption)
{
	LPVOID lpMsgBuf;
	char buffer[128];
	DWORD error = GetLastError();

	if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,0,NULL)){
				(void)_snprintf(buffer,sizeof(buffer),
						"Error code = 0x%x",(UINT)error);
				buffer[sizeof(buffer) - 1] = 0;
				MessageBoxA(NULL,buffer,caption,MB_OK | MB_ICONHAND);
				return;
	} else {
		MessageBoxA(NULL,(LPCTSTR)lpMsgBuf,caption,MB_OK | MB_ICONHAND);
		LocalFree(lpMsgBuf);
	}
}

void DisplayDefragError(int error_code,char *caption)
{
	char buffer[512];
	
	if(error_code == UDEFRAG_ALREADY_RUNNING){
		MessageBoxA(NULL,udefrag_get_error_description(error_code),
				caption,MB_OK | MB_ICONHAND);
	} else {
		(void)_snprintf(buffer,sizeof(buffer),"%s\n%s",
				udefrag_get_error_description(error_code),
				"Use DbgView program to get more information.");
		buffer[sizeof(buffer) - 1] = 0;
		MessageBoxA(NULL,buffer,caption,MB_OK | MB_ICONHAND);
	}
}

void DisplayStopDefragError(int error_code,char *caption)
{
	char buffer[512];
	
	(void)_snprintf(buffer,sizeof(buffer),"%s\n%s\n%s\n%s",
			udefrag_get_error_description(error_code),
			"Kill ultradefrag.exe program in task manager",
			"[press Ctrl + Alt + Delete to launch it].",
			"Use DbgView program to get more information.");
	buffer[sizeof(buffer) - 1] = 0;
	MessageBoxA(NULL,buffer,caption,MB_OK | MB_ICONHAND);
}

/*-------------------- Main Function -----------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	int error_code;
//	DWORD error;
//	int requests_counter = 0;
	
	hInstance = GetModuleHandle(NULL);
	//MessageBox(0,"Do you really want to hibernate when done?","Please Confirm",MB_OKCANCEL | MB_ICONEXCLAMATION);
	//DialogBox(hInstance,MAKEINTRESOURCE(IDD_SHUTDOWN),hWindow,(DLGPROC)ShutdownConfirmDlgProc);
	//return 0;

	if(strstr(lpCmdLine,"--setup")){
		GetPrefs();
		if(!ex_filter[0] || !strcmp(ex_filter,"system volume information;temp;recycler"))
			strcpy(ex_filter,"system volume information;temp;recycler;recycled");
		refresh_interval = DEFAULT_REFRESH_INTERVAL; /* nice looking 100 ms */
		SavePrefs();
		DeleteEnvironmentVariables();
		return 0;
	}
	
	GetPrefs();

	//DisplayStopDefragError(-1,"Error!");
	//return 0;

	error_code = udefrag_init();
	if(error_code < 0){
		DisplayDefragError(error_code,"Initialization failed!");
		(void)udefrag_unload();
		DeleteEnvironmentVariables();
		return 2;
	}

	/*
	* This call needs on dmitriar's pc (on xp) no more than 550 cpu tacts,
	* but InitCommonControlsEx() needs about 90000 tacts.
	* Because the first function is just a stub on xp.
	*/
	InitCommonControls();
	
	if(DialogBox(hInstance,MAKEINTRESOURCE(IDD_MAIN),NULL,(DLGPROC)DlgProc) == (-1)){
		DisplayLastError("Cannot create the main window!");
		(void)udefrag_unload();
		DeleteEnvironmentVariables();
		return 3;
	}
	
	/* delete all created gdi objects */
	FreeVolListResources();
	DeleteMaps();
	if(hFont) (void)DeleteObject(hFont);
	/* save settings */
	SavePrefs();
	DeleteEnvironmentVariables();
	
	if(shutdown_flag && seconds_for_shutdown_rejection){
		if(DialogBox(hInstance,MAKEINTRESOURCE(IDD_SHUTDOWN),NULL,(DLGPROC)ShutdownConfirmDlgProc) == 0){
			WgxDestroyResourceTable(i18n_table);
			return 0;
		}
		/* in case of errors we'll shutdown anyway */
		/* to avoid situation when pc works a long time without any control */
	}
	WgxDestroyResourceTable(i18n_table);

	/* check for shutdown request */
	if(shutdown_flag){
		/* SE_SHUTDOWN privilege is set by udefrag_init() called before */
		if(hibernate_instead_of_shutdown){
			/* the second parameter must be FALSE, dmitriar's windows xp hangs otherwise */
			if(!SetSystemPowerState(FALSE,FALSE)){ /* hibernate, request permission from apps and drivers */
				DisplayLastError("Cannot hibernate the computer!");
			}
			return 0;
		}
		
/* the following code works fine but doesn't power off the pc */
#if 0
try_again:
		if(!InitiateSystemShutdown(NULL,
			"System shutdown was scheduled by UltraDefrag application.",
			60,
			FALSE, /* ask user to close apps with unsaved data */
			FALSE  /* shutdown without reboot */
			)){
				error = GetLastError();
				if(error == ERROR_NOT_READY && requests_counter < 3){
					/* wait 1 minute and try again (2 times) */
					requests_counter ++;
					Sleep(60 * 1000);
					goto try_again;
				}
				DisplayLastError("Cannot shut down the computer!");
		}
#else
		if(!ExitWindowsEx(EWX_POWEROFF | EWX_FORCEIFHUNG,
		  SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED)){
			DisplayLastError("Cannot shut down the computer!");
		}
#endif	
	}

	return 0;
}

void InitMainWindow(void)
{
	int dx,dy;
	RECT rc;
	
#ifdef KERNEL_MODE_DRIVER_SUPPORT
	char title[128];

	/* update window title */
	if(GetWindowText(hWindow,title,64)){
		if(udefrag_kernel_mode()) strcat(title," (Kernel Mode)");
		else strcat(title," (User Mode)");
		(void)SetWindowText(hWindow,title);
	}
#endif

	(void)WgxAddAccelerators(hInstance,hWindow,IDR_ACCELERATOR1);
	if(WgxBuildResourceTable(i18n_table,L".\\ud_i18n.lng"/*lng_file_path*/))
		WgxApplyResourceTable(i18n_table,hWindow);
	if(hibernate_instead_of_shutdown){
		(void)SetText(GetDlgItem(hWindow,IDC_SHUTDOWN),L"HIBERNATE_PC_AFTER_A_JOB");
	}
	WgxSetIcon(hInstance,hWindow,IDI_APP);

	if(skip_removable)
		(void)SendMessage(GetDlgItem(hWindow,IDC_SKIPREMOVABLE),BM_SETCHECK,BST_CHECKED,0);
	else
		(void)SendMessage(GetDlgItem(hWindow,IDC_SKIPREMOVABLE),BM_SETCHECK,BST_UNCHECKED,0);

	InitVolList(); /* before map! */
	InitProgress();
	InitMap();

	WgxCheckWindowCoordinates(&win_rc,130,50);

	delta_h = GetSystemMetrics(SM_CYCAPTION) - 0x13;
	if(delta_h < 0) delta_h = 0;

	if(GetWindowRect(hWindow,&rc)){
		dx = rc.right - rc.left;
		dy = rc.bottom - rc.top + delta_h;
		SetWindowPos(hWindow,0,win_rc.left,win_rc.top,dx,dy,0);
	}

	UpdateVolList();
	InitFont();
	
	/* status bar will always have default font */
	CreateStatusBar();
	UpdateStatusBar(&(vlist[0].stat)); /* can be initialized here by any entry */
}

/*---------------- Main Dialog Callback ---------------------*/

BOOL CALLBACK DlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	RECT rc;

	switch(msg){
	case WM_INITDIALOG:
		/* Window Initialization */
		hWindow = hWnd;
		InitMainWindow();
		break;
	case WM_NOTIFY:
		VolListNotifyHandler(lParam);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDC_ANALYSE:
			DoJob('a');
			break;
		case IDC_DEFRAGM:
			DoJob('d');
			break;
		case IDC_OPTIMIZE:
			DoJob('c');
			break;
		case IDC_ABOUT:
			if(DialogBox(hInstance,MAKEINTRESOURCE(IDD_ABOUT),hWindow,(DLGPROC)AboutDlgProc) == (-1)){
				DisplayLastError("Cannot create the About window!");
			}
			break;
		case IDC_SETTINGS:
			if(!busy_flag) CallGUIConfigurator();
			break;
		case IDC_SKIPREMOVABLE:
			skip_removable = (
				SendMessage(GetDlgItem(hWindow,IDC_SKIPREMOVABLE),
					BM_GETCHECK,0,0) == BST_CHECKED
				);
			break;
		case IDC_SHOWFRAGMENTED:
			if(!busy_flag) ShowReports();
			break;
		case IDC_PAUSE:
		case IDC_STOP:
			stop();
			break;
		case IDC_RESCAN:
			if(!busy_flag) UpdateVolList();
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
	case WM_MOVE:
		if(GetWindowRect(hWnd,&rc)){
			if((HIWORD(rc.bottom)) != 0xffff){
				rc.bottom -= delta_h;
				memcpy((void *)&win_rc,(void *)&rc,sizeof(RECT));
			}
		}
		break;
	case WM_CLOSE:
		if(GetWindowRect(hWnd,&rc)){
			if((HIWORD(rc.bottom)) != 0xffff){
				rc.bottom -= delta_h;
				memcpy((void *)&win_rc,(void *)&rc,sizeof(RECT));
			}
		}
		
		VolListGetColumnWidths();

		exit_pressed = TRUE;
		stop();
		(void)udefrag_unload();
		(void)EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

void ShowReports()
{
	NEW_VOLUME_LIST_ENTRY *v_entry;
	LRESULT SelectedItem;
	LV_ITEM lvi;
	char buffer[128];
	int index;
	
	index = -1;
	while(1){
		SelectedItem = SendMessage(hList,LVM_GETNEXTITEM,(WPARAM)index,LVNI_SELECTED);
		if(SelectedItem == -1 || SelectedItem == index) break;
		lvi.iItem = (int)SelectedItem;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT;
		lvi.pszText = buffer;
		lvi.cchTextMax = 127;
		if(SendMessage(hList,LVM_GETITEM,0,(LRESULT)&lvi)){
			buffer[2] = 0;
			v_entry = vlist_get_entry(buffer);
			ShowSingleReport(v_entry);
		}
		index = (int)SelectedItem;
	}
}

void ShowSingleReport(NEW_VOLUME_LIST_ENTRY *v_entry)
{
#ifndef UDEFRAG_PORTABLE
	short l_path[] = L"C:\\fraglist.luar";
#else
	char path[] = "C:\\fraglist.luar";
	char cmd[MAX_PATH];
	char buffer[MAX_PATH + 64];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
#endif

	if(v_entry == NULL) return;
	if(!v_entry->name[0] || v_entry->status == STATUS_UNDEFINED) return;

#ifndef UDEFRAG_PORTABLE
	l_path[0] = (short)(v_entry->name[0]);
	(void)WgxShellExecuteW(hWindow,L"view",l_path,NULL,NULL,SW_SHOW);
#else
	path[0] = v_entry->name[0];
	(void)strcpy(cmd,".\\lua5.1a_gui.exe");
	(void)strcpy(buffer,cmd);
	(void)strcat(buffer," .\\scripts\\udreportcnv.lua ");
	(void)strcat(buffer,path);
	(void)strcat(buffer," null -v");

	ZeroMemory(&si,sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
	ZeroMemory(&pi,sizeof(pi));

	if(!CreateProcess(cmd,buffer,
	  NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)){
	    DisplayLastError("Cannot execute lua5.1a_gui.exe program!");
	    return;
	}
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
#endif
}

DWORD WINAPI ConfigThreadProc(LPVOID lpParameter)
{
	char path[MAX_PATH];
	char buffer[MAX_PATH + 64];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	int error_code;

	/* window coordinates must be accessible by configurator */
	SavePrefs();
	
	(void)strcpy(path,".\\udefrag-gui-config.exe");
	/* the configurator must know when it should reposition its window */
	(void)strcpy(buffer,path);
	(void)strcat(buffer," CalledByGUI");

	ZeroMemory(&si,sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
	ZeroMemory(&pi,sizeof(pi));

	if(!CreateProcess(path,buffer,
	  NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)){
	    DisplayLastError("Cannot execute udefrag-gui-config.exe program!");
	    return 0;
	}
	(void)WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	
	/* reinitialize GUI */
	GetPrefs();
	stop();
	(void)udefrag_unload();
	error_code = udefrag_init();
	if(error_code < 0){
		DisplayDefragError(error_code,"Initialization failed!");
		(void)udefrag_unload();
	}
	InitFont();
	(void)SendMessage(hStatus,WM_SETFONT,(WPARAM)0,MAKELPARAM(TRUE,0));
	if(hibernate_instead_of_shutdown)
		SetText(GetDlgItem(hWindow,IDC_SHUTDOWN),L"HIBERNATE_PC_AFTER_A_JOB");
	else
		SetText(GetDlgItem(hWindow,IDC_SHUTDOWN),L"SHUTDOWN_PC_AFTER_A_JOB");

	return 0;
}

void CallGUIConfigurator(void)
{
	HANDLE h;
	DWORD id;
	
	h = create_thread(ConfigThreadProc,NULL,&id);
	if(h == NULL)
		DisplayLastError("Cannot create thread starting the Configurator program!");
	if(h) CloseHandle(h);
}
