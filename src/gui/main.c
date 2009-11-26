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
* GUI - main window code.
*/

#include "main.h"

/* Global variables */
HINSTANCE hInstance;
HWND hWindow;
HWND hMap;
signed int delta_h = 0;
HFONT hFont = NULL;
extern HWND hStatus;
extern WGX_I18N_RESOURCE_ENTRY i18n_table[];
extern VOLUME_LIST_ENTRY volume_list[];
extern RECT win_rc; /* coordinates of main window */
extern int skip_removable;

int shutdown_flag = FALSE;
extern int hibernate_instead_of_shutdown;

/* they have the same effect as environment variables for console program */
extern char in_filter[];
extern char ex_filter[];
extern char sizelimit[];
extern int refresh_interval;
extern int disable_reports;
extern char dbgprint_level[];

extern BOOL busy_flag, exit_pressed;

/* Function prototypes */
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void ShowFragmented();
void VolListGetColumnWidths(void);

void InitFont(void);
void CallGUIConfigurator(void);
void DeleteEnvironmentVariables(void);

void __stdcall ErrorHandler(short *msg)
{
	/* ignore notifications and warnings */
	if((short *)wcsstr(msg,L"N: ") == msg || (short *)wcsstr(msg,L"W: ") == msg) return;
	/* skip E: labels */
	if((short *)wcsstr(msg,L"E: ") == msg)
		MessageBoxW(NULL,msg + 3,L"Error!",MB_OK | MB_ICONHAND);
	else
		MessageBoxW(NULL,msg,L"Error!",MB_OK | MB_ICONHAND);
}

/*-------------------- Main Function -----------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
//	DWORD error;
//	int requests_counter = 0;
	LPVOID error_message;
	
	if(strstr(lpCmdLine,"--setup")){
		GetPrefs();
		if(!ex_filter[0] || !strcmp(ex_filter,"system volume information;temp;recycler"))
			strcpy(ex_filter,"system volume information;temp;recycler;recycled");
		SavePrefs();
		DeleteEnvironmentVariables();
		return 0;
	}
	
	GetPrefs();

	udefrag_set_error_handler(ErrorHandler);
	if(udefrag_init(N_BLOCKS) < 0){
		udefrag_unload();
		DeleteEnvironmentVariables();
		return 2;
	}

	hInstance = GetModuleHandle(NULL);

	/*
	* This call needs on dmitriar's pc (on xp) no more than 550 cpu tacts,
	* but InitCommonControlsEx() needs about 90000 tacts.
	* Because the first function is just a stub on xp.
	*/
	InitCommonControls();
	
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN),NULL,(DLGPROC)DlgProc);
	/* delete all created gdi objects */
	FreeVolListResources();
	DeleteMaps();
	if(hFont) DeleteObject(hFont);
	/* save settings */
	SavePrefs();
	WgxDestroyResourceTable(i18n_table);
	DeleteEnvironmentVariables();
	
	/* check for shutdown request */
	if(shutdown_flag){
		/* SE_SHUTDOWN privilege is set by udefrag_init() called before */
		if(hibernate_instead_of_shutdown){
			/* the second parameter must be FALSE, dmitriar's windows xp hangs otherwise */
			if(!SetSystemPowerState(FALSE,FALSE)){ /* hibernate, request permission from apps and drivers */
				/* format message and display it on the screen */
				if(FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &error_message,
					0,
					NULL )){
						MessageBox(NULL,(LPCTSTR)error_message,"Error",MB_OK | MB_ICONINFORMATION);
						LocalFree(error_message);
				}
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
				/* format message and display it on the screen */
				if(FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					error,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &error_message,
					0,
					NULL )){
						MessageBox(NULL,(LPCTSTR)error_message,"Error",MB_OK | MB_ICONINFORMATION);
						LocalFree(error_message);
				}
		}
#else
		if(!ExitWindowsEx(EWX_POWEROFF | EWX_FORCEIFHUNG,
			SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED)){
				/* format message and display it on the screen */
				if(FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &error_message,
					0,
					NULL )){
						MessageBox(NULL,(LPCTSTR)error_message,"Error",MB_OK | MB_ICONINFORMATION);
						LocalFree(error_message);
				}
		}
#endif	
	}

	return 0;
}

void InitMainWindow(void)
{
//	short lng_file_path[MAX_PATH];
	char title[128];
	int dx,dy;
	RECT rc;
	
	/* update window title */
	GetWindowText(hWindow,title,64);
	if(udefrag_kernel_mode()) strcat(title," (Kernel Mode)");
	else strcat(title," (User Mode)");
	SetWindowText(hWindow,title);

	WgxAddAccelerators(hInstance,hWindow,IDR_ACCELERATOR1);
//	GetWindowsDirectoryW(lng_file_path,MAX_PATH);
//	wcscat(lng_file_path,L"\\UltraDefrag\\ud_i18n.lng");
	if(WgxBuildResourceTable(i18n_table,L".\\ud_i18n.lng"/*lng_file_path*/))
		WgxApplyResourceTable(i18n_table,hWindow);
	if(hibernate_instead_of_shutdown){
		SetText(GetDlgItem(hWindow,IDC_SHUTDOWN),L"HIBERNATE_PC_AFTER_A_JOB");
	}
	WgxSetIcon(hInstance,hWindow,IDI_APP);

	if(skip_removable)
		SendMessage(GetDlgItem(hWindow,IDC_SKIPREMOVABLE),BM_SETCHECK,BST_CHECKED,0);
	else
		SendMessage(GetDlgItem(hWindow,IDC_SKIPREMOVABLE),BM_SETCHECK,BST_UNCHECKED,0);

	InitVolList(); /* before map! */
	InitProgress();
	InitMap();

	WgxCheckWindowCoordinates(&win_rc,130,50);

	delta_h = GetSystemMetrics(SM_CYCAPTION) - 0x13;
	if(delta_h < 0) delta_h = 0;

	GetWindowRect(hWindow,&rc);
	dx = rc.right - rc.left;
	dy = rc.bottom - rc.top + delta_h;
	SetWindowPos(hWindow,0,win_rc.left,win_rc.top,dx,dy,0);

	UpdateVolList();
	InitFont();
	
	/* status bar will always have default font */
	CreateStatusBar();
	UpdateStatusBar(&(volume_list[0].Statistics));
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
			analyse();
			break;
		case IDC_DEFRAGM:
			defragment();
			break;
		case IDC_COMPACT:
			optimize();
			break;
		case IDC_ABOUT:
			DialogBox(hInstance,MAKEINTRESOURCE(IDD_ABOUT),hWindow,(DLGPROC)AboutDlgProc);
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
			if(!busy_flag) ShowFragmented();
			break;
		case IDC_PAUSE:
		case IDC_STOP:
			stop();
			break;
		case IDC_RESCAN:
			if(!busy_flag) UpdateVolList();
		}
		break;
	case WM_MOVE:
		GetWindowRect(hWnd,&rc);
		if((HIWORD(rc.bottom)) != 0xffff){
			rc.bottom -= delta_h;
			memcpy((void *)&win_rc,(void *)&rc,sizeof(RECT));
		}
		break;
	case WM_CLOSE:
		GetWindowRect(hWnd,&rc);
		if((HIWORD(rc.bottom)) != 0xffff){
			rc.bottom -= delta_h;
			memcpy((void *)&win_rc,(void *)&rc,sizeof(RECT));
		}
		VolListGetColumnWidths();

		exit_pressed = TRUE;
		stop();
		udefrag_unload();
		EndDialog(hWnd,0);
		return TRUE;
	}
	return FALSE;
}

void ShowFragmented()
{
	char path[] = "C:\\fraglist.luar";
	PVOLUME_LIST_ENTRY vl;
#ifdef UDEFRAG_PORTABLE
	char cmd[MAX_PATH];
	char buffer[MAX_PATH + 64];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
#endif

	vl = VolListGetSelectedEntry();
	if(vl->VolumeName == NULL || vl->Status == STAT_CLEAR) return;

	path[0] = vl->VolumeName[0];
#ifndef UDEFRAG_PORTABLE
	ShellExecute(hWindow,"view",path,NULL,NULL,SW_SHOW);
#else
	strcpy(cmd,".\\lua5.1a_gui.exe");
	strcpy(buffer,cmd);
	strcat(buffer," .\\scripts\\udreportcnv.lua ");
	strcat(buffer,path);
	strcat(buffer," null -v");

	ZeroMemory(&si,sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
	ZeroMemory(&pi,sizeof(pi));

	if(!CreateProcess(cmd,buffer,
		NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)){
	    MessageBox(NULL,"Can't execute lua5.1a_gui.exe program!",
			"Error",MB_OK | MB_ICONHAND);
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

	/* window coordinates must be accessible by configurator */
	SavePrefs();
	
	//GetWindowsDirectory(path,MAX_PATH);
	//strcat(path,"\\System32\\udefrag-gui-config.exe");
	strcpy(path,".\\udefrag-gui-config.exe");
	
	/* the configurator must know when it should reposition its window */
	strcpy(buffer,path);
	strcat(buffer," CalledByGUI");

	ZeroMemory(&si,sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
	ZeroMemory(&pi,sizeof(pi));

	if(!CreateProcess(path,buffer,
		NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)){
	    MessageBox(NULL,"Can't execute udefrag-gui-config.exe program!",
			"Error",MB_OK | MB_ICONHAND);
	    return 0;
	}
	WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	
	/* reinitialize GUI */
	GetPrefs();
	stop();
	udefrag_unload();
	if(udefrag_init(N_BLOCKS) < 0) udefrag_unload();
	InitFont();
	SendMessage(hStatus,WM_SETFONT,(WPARAM)0,MAKELPARAM(TRUE,0));
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
	if(h) CloseHandle(h);
}
