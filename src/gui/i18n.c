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
 * @file i18n.c
 * @brief Internationalization.
 * @addtogroup Internationalization
 * @{
 */

#include "main.h"

WGX_I18N_RESOURCE_ENTRY i18n_table[] = {
	/* action menu */
	{0, L"ACTION",                   L"&Action",                  NULL},
	{0, L"ANALYSE",                  L"&Analyze",                 NULL},
	{0, L"DEFRAGMENT",               L"&Defragment",              NULL},
	{0, L"OPTIMIZE",                 L"&Optimize",                NULL},
	{0, L"STOP",                     L"&Stop",                    NULL},
	{0, L"SKIP_REMOVABLE_MEDIA",     L"Skip removable &media",    NULL},
	{0, L"RESCAN_DRIVES",            L"&Rescan drives",           NULL},
	{0, L"SHUTDOWN_PC_AFTER_A_JOB",  L"Shutdown &PC when done",   NULL},
	{0, L"HIBERNATE_PC_AFTER_A_JOB", L"Hibernate &PC when done",  NULL},
	{0, L"EXIT",                     L"E&xit",                    NULL},

	/* report menu */
	{0, L"REPORT",                   L"&Report",                  NULL},
	{0, L"SHOW_REPORT",              L"&Show report",             NULL},

	/* settings menu */
	{0, L"SETTINGS",                 L"&Settings",                NULL},
	{0, L"LANGUAGE",                 L"&Language",                NULL},
	{0, L"TRANSLATIONS_FOLDER",      L"&Translations folder",     NULL},
	{0, L"GRAPHICAL_INTERFACE",      L"&Graphical interface",     NULL},
	{0, L"FONT",                     L"&Font",                    NULL},
	{0, L"OPTIONS",                  L"&Options",                 NULL},
	{0, L"BOOT_TIME_SCAN",           L"&Boot time scan",          NULL},
	{0, L"ENABLE",                   L"&Enable",                  NULL},
	{0, L"SCRIPT",                   L"&Script",                  NULL},
	{0, L"REPORTS",                  L"&Reports",                 NULL},

	/* help menu */
	{0, L"HELP",                     L"&Help",                    NULL},
	{0, L"CONTENTS",                 L"&Contents",                NULL},
	{0, L"BEST_PRACTICE",            L"Best &practice",           NULL},
	{0, L"FAQ",                      L"&FAQ",                     NULL},
	{0, L"ABOUT",                    L"&About",                   NULL},

	/* volume characteristics */
	{0, L"VOLUME",                   L"Volume",                   NULL},
	{0, L"STATUS",                   L"Status",                   NULL},
	{0, L"TOTAL",                    L"Total Space",              NULL},
	{0, L"FREE",                     L"Free Space",               NULL},
	{0, L"PERCENT",                  L"% free",                   NULL},

	/* volume processing status */
	{0, L"STATUS_RUNNING",           L"Executing",                NULL},
	{0, L"STATUS_ANALYSED",          L"Analyzed",                 NULL},
	{0, L"STATUS_DEFRAGMENTED",      L"Defragmented",             NULL},
	{0, L"STATUS_OPTIMIZED",         L"Optimized",                NULL},

	/* status bar */
	{0, L"DIRS",                     L"folders",                  NULL},
	{0, L"FILES",                    L"files",                    NULL},
	{0, L"FRAGMENTED",               L"fragmented",               NULL},
	{0, L"COMPRESSED",               L"compressed",               NULL},
	{0, L"MFT",                      L"MFT",                      NULL},

	/* about box */
	{0, L"ABOUT_WIN_TITLE",          L"About Ultra Defragmenter", NULL},
	{0, L"CREDITS",                  L"&Credits",                 NULL},
	{0, L"LICENSE",                  L"&License",                 NULL},

	/* shutdown confirmation */
	{0,                 L"PLEASE_CONFIRM",             L"Please Confirm",            NULL},
	{0,                 L"REALLY_SHUTDOWN_WHEN_DONE",  L"Do you really want to shutdown when done?",  NULL},
	{0,                 L"REALLY_HIBERNATE_WHEN_DONE", L"Do you really want to hibernate when done?", NULL},
	{0,                 L"SECONDS_TILL_SHUTDOWN",      L"seconds till shutdown",     NULL},
	{0,                 L"SECONDS_TILL_HIBERNATION",   L"seconds till hibernation",  NULL},
	{IDC_YES_BUTTON,    L"YES",                        L"&Yes",                      NULL},
	{IDC_NO_BUTTON,     L"NO",                         L"&No",                       NULL},

	/* end of the table */
	{0,                 NULL,                          NULL,                         NULL}
};

struct menu_item {
	int id;          /* menu item identifier */
	short *key;      /* i18n table entry key */
	char *hotkeys;  /* hotkeys assigned to the item */
};

/**
 * @brief Simplifies build of localized menu.
 */
struct menu_item menu_items[] = {
	{IDM_ANALYZE,                 L"ANALYSE",                  "F5"    },
	{IDM_DEFRAG,                  L"DEFRAGMENT",               "F6"    },
	{IDM_OPTIMIZE,                L"OPTIMIZE",                 "F7"    },
	{IDM_STOP,                    L"STOP",                     "Ctrl+C"},
	{IDM_IGNORE_REMOVABLE_MEDIA,  L"SKIP_REMOVABLE_MEDIA",     "Ctrl+M"},
	{IDM_RESCAN,                  L"RESCAN_DRIVES",            "Ctrl+D"},
	{IDM_EXIT,                    L"EXIT",                     "Alt+F4"},
	{IDM_SHOW_REPORT,             L"SHOW_REPORT",              "F8"    },
	{IDM_TRANSLATIONS_FOLDER,     L"TRANSLATIONS_FOLDER",      NULL    },
	{IDM_CFG_GUI_FONT,            L"FONT",                     "F9"    },
	{IDM_CFG_GUI_SETTINGS,        L"OPTIONS",                  "F10"   },
	{IDM_CFG_BOOT_ENABLE,         L"ENABLE",                   "F11"   },
	{IDM_CFG_BOOT_SCRIPT,         L"SCRIPT",                   "F12"   },
	{IDM_CFG_REPORTS,             L"REPORTS",                  "Ctrl+R"},
	{IDM_CONTENTS,                L"CONTENTS",                 "F1"    },
	{IDM_BEST_PRACTICE,           L"BEST_PRACTICE",            "F2"    },
	{IDM_FAQ,                     L"FAQ",                      "F3"    },
	{IDM_ABOUT,                   L"ABOUT",                    "F4"    },
	/* submenus */
	{IDM_LANGUAGE,                L"LANGUAGE",                 NULL},
	{IDM_CFG_GUI,                 L"GRAPHICAL_INTERFACE",      NULL},
	{IDM_CFG_BOOT,                L"BOOT_TIME_SCAN",           NULL},
	{IDM_ACTION,                  L"ACTION",                   NULL},
	{IDM_REPORT,                  L"REPORT",                   NULL},
	{IDM_SETTINGS,                L"SETTINGS",                 NULL},
	{IDM_HELP,                    L"HELP",                     NULL},
	{0, NULL, NULL}
};

/**
 * @brief Synchronization events.
 */
HANDLE hLangPackEvent = NULL;
HANDLE hLangMenuEvent = NULL;

int lang_ini_tracking_stopped = 0;
int stop_track_lang_ini = 0;
int i18n_folder_tracking_stopped = 0;
int stop_track_i18n_folder = 0;

/**
 * @brief Initializes events needed 
 * for i18n mechanisms synchronization.
 * @return Zero for success, negative value otherwise.
 */
int Init_I18N_Events(void)
{
	hLangPackEvent = CreateEvent(NULL,FALSE,TRUE,NULL);
	if(hLangPackEvent == NULL){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
			"Cannot create language pack synchronization event!");
		return (-1);
	}

	hLangMenuEvent = CreateEvent(NULL,FALSE,TRUE,NULL);
	if(hLangMenuEvent == NULL){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
			"Cannot create language menu synchronization event!");
		CloseHandle(hLangPackEvent);
		return (-1);
	}
	
	return 0;
}

/**
 * @brief Applies selected language pack to all GUI controls.
 */
void ApplyLanguagePack(void)
{
	char lang_name[MAX_PATH];
	wchar_t path[MAX_PATH];
	udefrag_progress_info pi;
	MENUITEMINFOW mi;
	short *s = L"";
	short buffer[256];
	int i;
	LVCOLUMNW lvc;
	
	/* synchronize with other threads */
	if(WaitForSingleObject(hLangPackEvent,INFINITE) != WAIT_OBJECT_0){
		WgxDbgPrintLastError("ApplyLanguagePack: wait on hLangPackEvent failed");
		return;
	}
	
	/* read lang.ini file */
	GetPrivateProfileString("Language","Selected","",lang_name,MAX_PATH,".\\lang.ini");
	if(lang_name[0] == 0){
		WgxDbgPrint("Selected language name not found in lang.ini file\n");
		SetEvent(hLangPackEvent);
		return;
	}
	_snwprintf(path,MAX_PATH,L".\\i18n\\%hs.lng",lang_name);
	path[MAX_PATH - 1] = 0;
	
	/* destroy resource table */
	WgxDestroyResourceTable(i18n_table);
	
	/* build new resource table */
	WgxBuildResourceTable(i18n_table,path);
	
	/* apply new strings to the list of volumes */
	lvc.mask = LVCF_TEXT;
	lvc.pszText = WgxGetResourceString(i18n_table,L"VOLUME");
	SendMessage(hList,LVM_SETCOLUMNW,0,(LPARAM)&lvc);
	lvc.pszText = WgxGetResourceString(i18n_table,L"STATUS");
	SendMessage(hList,LVM_SETCOLUMNW,1,(LPARAM)&lvc);
	lvc.pszText = WgxGetResourceString(i18n_table,L"TOTAL");
	SendMessage(hList,LVM_SETCOLUMNW,2,(LPARAM)&lvc);
	lvc.pszText = WgxGetResourceString(i18n_table,L"FREE");
	SendMessage(hList,LVM_SETCOLUMNW,3,(LPARAM)&lvc);
	lvc.pszText = WgxGetResourceString(i18n_table,L"PERCENT");
	SendMessage(hList,LVM_SETCOLUMNW,4,(LPARAM)&lvc);
	
	/* apply new strings to the main menu */
	for(i = 0; menu_items[i].id; i++){
		s = WgxGetResourceString(i18n_table,menu_items[i].key);
		if(menu_items[i].hotkeys)
			_snwprintf(buffer,256,L"%ws\t%hs",s,menu_items[i].hotkeys);
		else
			_snwprintf(buffer,256,L"%ws",s);
		buffer[255] = 0;
		memset(&mi,0,sizeof(MENUITEMINFOW));
		mi.cbSize = sizeof(MENUITEMINFOW);
		mi.fMask = MIIM_TYPE;
		mi.fType = MFT_STRING;
		mi.dwTypeData = buffer;
		SetMenuItemInfoW(hMainMenu,menu_items[i].id,FALSE,&mi);
	}
	if(hibernate_instead_of_shutdown)
		s = WgxGetResourceString(i18n_table,L"HIBERNATE_PC_AFTER_A_JOB");
	else
		s = WgxGetResourceString(i18n_table,L"SHUTDOWN_PC_AFTER_A_JOB");
	_snwprintf(buffer,256,L"%ws\tCtrl+S",s);
	buffer[255] = 0;
	ModifyMenuW(hMainMenu,IDM_SHUTDOWN,MF_BYCOMMAND | MF_STRING,IDM_SHUTDOWN,buffer);
	
	/* end of synchronization */
	SetEvent(hLangPackEvent);
	
	/* redraw main menu */
	if(!DrawMenuBar(hWindow))
		WgxDbgPrintLastError("Cannot redraw main menu");
	
	/* refresh volume status fields */
	update_status_of_all_jobs();
	
	/* refresh status bar */
	if(current_job){
		UpdateStatusBar(&current_job->pi);
	} else {
		memset(&pi,0,sizeof(udefrag_progress_info));
		UpdateStatusBar(&pi);
	}
}

/**
 * @brief Builds Language menu.
 */
void BuildLanguageMenu(void)
{
	MENUITEMINFO mi;
	HMENU hLangMenu;
	intptr_t h;
	struct _wfinddata_t lng_file;
	wchar_t filename[MAX_PATH];
	int i, length;
	wchar_t selected_lang_name[MAX_PATH];
	UINT flags;
	
	/* synchronize with other threads */
	if(WaitForSingleObject(hLangMenuEvent,INFINITE) != WAIT_OBJECT_0){
		WgxDbgPrintLastError("BuildLanguageMenu: wait on hLangMenuEvent failed");
		return;
	}
	
	/* get selected language name */
	GetPrivateProfileStringW(L"Language",L"Selected",L"",selected_lang_name,MAX_PATH,L".\\lang.ini");
	if(selected_lang_name[0] == 0)
		wcscpy(selected_lang_name,L"English (US)");

	/* get submenu handle */
	memset(&mi,0,sizeof(MENUITEMINFO));
	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_SUBMENU;
	if(!GetMenuItemInfo(hMainMenu,IDM_LANGUAGE,FALSE,&mi)){
		WgxDbgPrintLastError("BuildLanguageMenu: cannot get submenu handle");
		SetEvent(hLangMenuEvent);
		return;
	}
	hLangMenu = mi.hSubMenu;
	
	/* detach submenu */
	memset(&mi,0,sizeof(MENUITEMINFO));
	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_SUBMENU;
	mi.hSubMenu = NULL;
	if(!SetMenuItemInfo(hMainMenu,IDM_LANGUAGE,FALSE,&mi)){
		WgxDbgPrintLastError("BuildLanguageMenu: cannot detach submenu");
		SetEvent(hLangMenuEvent);
		return;
	}
	
	/* destroy submenu */
	DestroyMenu(hLangMenu);
	
	/* build new menu from the list of installed files */
	hLangMenu = CreatePopupMenu();
	if(hLangMenu == NULL){
		WgxDbgPrintLastError("BuildLanguageMenu: cannot create submenu");
		SetEvent(hLangMenuEvent);
		return;
	}
	
	/* add translations folder menu item and a separator */
	if(!AppendMenuW(hLangMenu,MF_STRING | MF_ENABLED,IDM_TRANSLATIONS_FOLDER,
	  WgxGetResourceString(i18n_table,L"TRANSLATIONS_FOLDER")))
		WgxDbgPrintLastError("BuildLanguageMenu: cannot append menu item");
	AppendMenu(hLangMenu,MF_SEPARATOR,0,NULL);
	
	h = _wfindfirst(L".\\i18n\\*.lng",&lng_file);
	if(h == -1){
		WgxDbgPrint("BuildLanguageMenu: no language packs found\n");
		/* add default US English */
		if(!AppendMenu(hLangMenu,MF_STRING | MF_ENABLED | MF_CHECKED,IDM_LANGUAGE + 0x1,"English (US)")){
			WgxDbgPrintLastError("BuildLanguageMenu: cannot append menu item");
			DestroyMenu(hLangMenu);
			SetEvent(hLangMenuEvent);
			return;
		}
	} else {
		i = 0x1;
		wcsncpy(filename,lng_file.name,MAX_PATH - 1);
		filename[MAX_PATH - 1] = 0;
		length = wcslen(filename);
		if(length > (int)wcslen(L".lng"))
			filename[length - 4] = 0;
		flags = MF_STRING | MF_ENABLED;
		if(wcscmp(selected_lang_name,filename) == 0)
			flags |= MF_CHECKED;
		if(!AppendMenuW(hLangMenu,flags,IDM_LANGUAGE + i,filename)){
			WgxDbgPrintLastError("BuildLanguageMenu: cannot append menu item");
		} else {
			i++;
		}
		while(_wfindnext(h,&lng_file) == 0){
			wcsncpy(filename,lng_file.name,MAX_PATH - 1);
			filename[MAX_PATH - 1] = 0;
			length = wcslen(filename);
			if(length > (int)wcslen(L".lng"))
				filename[length - 4] = 0;
			flags = MF_STRING | MF_ENABLED;
			if(wcscmp(selected_lang_name,filename) == 0)
				flags |= MF_CHECKED;
			if(!AppendMenuW(hLangMenu,flags,IDM_LANGUAGE + i,filename)){
				WgxDbgPrintLastError("BuildLanguageMenu: cannot append menu item");
			} else {
				i++;
			}
		}
		_findclose(h);
	}
	
	/* attach submenu to the Language menu */
	memset(&mi,0,sizeof(MENUITEMINFO));
	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_SUBMENU;
	mi.hSubMenu = hLangMenu;
	if(!SetMenuItemInfo(hMainMenu,IDM_LANGUAGE,FALSE,&mi)){
		WgxDbgPrintLastError("BuildLanguageMenu: cannot attach submenu");
		DestroyMenu(hLangMenu);
		SetEvent(hLangMenuEvent);
		return;
	}
	
	/* end of synchronization */
	SetEvent(hLangMenuEvent);
}

/**
 * @brief StartLangIniChangesTracking thread routine.
 */
DWORD WINAPI LangIniChangesTrackingProc(LPVOID lpParameter)
{
	HANDLE h;
	DWORD status;
	wchar_t selected_lang_name[MAX_PATH];
	wchar_t text[MAX_PATH];
	MENUITEMINFOW mi;
	int i;
	
	h = FindFirstChangeNotification(".",
			FALSE,FILE_NOTIFY_CHANGE_LAST_WRITE);
	if(h == INVALID_HANDLE_VALUE){
		WgxDbgPrintLastError("LangIniChangesTrackingProc: FindFirstChangeNotification failed");
		lang_ini_tracking_stopped = 1;
		return 0;
	}
	
	while(!stop_track_lang_ini){
		status = WaitForSingleObject(h,500/*INFINITE*/);
		if(status == WAIT_OBJECT_0){
			// TODO: update only if lang.ini changed
			ApplyLanguagePack();
			/* update language menu */
			GetPrivateProfileStringW(L"Language",L"Selected",L"",selected_lang_name,MAX_PATH,L".\\lang.ini");
			if(selected_lang_name[0] == 0)
				wcscpy(selected_lang_name,L"English (US)");
			for(i = IDM_LANGUAGE + 1; i < IDM_CFG_GUI; i++){
				if(CheckMenuItem(hMainMenu,i,MF_BYCOMMAND | MF_UNCHECKED) == -1)
					break;
				/* check the selected language */
				memset(&mi,0,sizeof(MENUITEMINFOW));
				mi.cbSize = sizeof(MENUITEMINFOW);
				mi.fMask = MIIM_TYPE;
				mi.fType = MFT_STRING;
				mi.dwTypeData = text;
				mi.cch = MAX_PATH;
				if(!GetMenuItemInfoW(hMainMenu,i,FALSE,&mi)){
					WgxDbgPrintLastError("LangIniChangesTrackingProc: cannot get menu item info");
				} else {
					if(wcscmp(selected_lang_name,text) == 0)
						CheckMenuItem(hMainMenu,i,MF_BYCOMMAND | MF_CHECKED);
				}
			}
			/* wait for the next notification */
			if(!FindNextChangeNotification(h)){
				WgxDbgPrintLastError("LangIniChangesTrackingProc: FindNextChangeNotification failed");
				break;
			}
		}
	}
	
	/* cleanup */
	FindCloseChangeNotification(h);
	lang_ini_tracking_stopped = 1;
	return 0;
}

/**
 * @brief Starts tracking of lang.ini changes.
 */
void StartLangIniChangesTracking()
{
	HANDLE h;
	DWORD id;
	
	h = create_thread(LangIniChangesTrackingProc,NULL,&id);
	if(h == NULL){
		WgxDbgPrintLastError("Cannot create thread for lang.ini changes tracking");
	} else {
		CloseHandle(h);
	}
}

/**
 * @brief Stops tracking of lang.ini changes.
 */
void StopLangIniChangesTracking()
{
	stop_track_lang_ini = 1;
	while(!lang_ini_tracking_stopped)
		Sleep(100);
}

/**
 * @brief StartI18nFolderChangesTracking thread routine.
 */
DWORD WINAPI I18nFolderChangesTrackingProc(LPVOID lpParameter)
{
	HANDLE h;
	DWORD status;
	
	h = FindFirstChangeNotification(".\\i18n",
			FALSE,FILE_NOTIFY_CHANGE_LAST_WRITE \
			| FILE_NOTIFY_CHANGE_FILE_NAME \
			| FILE_NOTIFY_CHANGE_DIR_NAME \
			| FILE_NOTIFY_CHANGE_SIZE);
	if(h == INVALID_HANDLE_VALUE){
		WgxDbgPrintLastError("I18nFolderChangesTrackingProc: FindFirstChangeNotification failed");
		i18n_folder_tracking_stopped = 1;
		return 0;
	}
	
	while(!stop_track_i18n_folder){
		status = WaitForSingleObject(h,500/*INFINITE*/);
		if(status == WAIT_OBJECT_0){
			// TODO: update only if current translation updated
			ApplyLanguagePack();
			/* update language menu anyway */
			BuildLanguageMenu();
			/* wait for the next notification */
			if(!FindNextChangeNotification(h)){
				WgxDbgPrintLastError("I18nFolderChangesTrackingProc: FindNextChangeNotification failed");
				break;
			}
		}
	}
	
	/* cleanup */
	FindCloseChangeNotification(h);
	i18n_folder_tracking_stopped = 1;
	return 0;
}

/**
 * @brief Starts tracking of i18n folder changes.
 */
void StartI18nFolderChangesTracking()
{
	HANDLE h;
	DWORD id;
	
	h = create_thread(I18nFolderChangesTrackingProc,NULL,&id);
	if(h == NULL){
		WgxDbgPrintLastError("Cannot create thread for i18n folder changes tracking");
	} else {
		CloseHandle(h);
	}
}

/**
 * @brief Stops tracking of i18n folder changes.
 */
void StopI18nFolderChangesTracking()
{
	stop_track_i18n_folder = 1;
	while(!i18n_folder_tracking_stopped)
		Sleep(100);
}

/**
 * @brief Destroys events needed 
 * for i18n mechanisms synchronization.
 */
void Destroy_I18N_Events(void)
{
	if(hLangPackEvent)
		CloseHandle(hLangPackEvent);
	if(hLangMenuEvent)
		CloseHandle(hLangMenuEvent);
}

/** @} */
