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
 * @file menu.c
 * @brief Menu.
 * @addtogroup Menu
 * @{
 */

#include "main.h"

HMENU hMainMenu = NULL;

WGX_MENU action_menu[] = {
	{MF_STRING | MF_ENABLED,IDM_ANALYZE,                             NULL, L"&Analyze\tF5"   },
	{MF_STRING | MF_ENABLED,IDM_DEFRAG,                              NULL, L"&Defragment\tF6"},
	{MF_STRING | MF_ENABLED,IDM_OPTIMIZE,                            NULL, L"&Optimize\tF7"  },
	{MF_STRING | MF_ENABLED,IDM_STOP,                                NULL, L"&Stop\tCtrl+C"  },
	{MF_SEPARATOR,0,NULL,NULL},
	{MF_STRING | MF_ENABLED | MF_CHECKED,IDM_IGNORE_REMOVABLE_MEDIA, NULL, L"Skip removable &media\tCtrl+M"},
	{MF_STRING | MF_ENABLED,IDM_RESCAN,                              NULL, L"&Rescan drives\tCtrl+D"},
	{MF_SEPARATOR,0,NULL,NULL},
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_SHUTDOWN,             NULL, L"Shutdown &PC when done\tCtrl+S"},
	{MF_SEPARATOR,0,NULL,NULL},
	{MF_STRING | MF_ENABLED,IDM_EXIT,                                NULL, L"E&xit\tAlt+F4"},
	{0,0,NULL,NULL}
};

WGX_MENU report_menu[] = {
	{MF_STRING | MF_ENABLED,IDM_SHOW_REPORT,                         NULL, L"&Show report\tF8"},
	{0,0,NULL,NULL}
};

WGX_MENU language_menu[] = {
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1,       NULL, L"Belarusian"            },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x2,       NULL, L"Bulgarian"             },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x3,       NULL, L"Burmese (Padauk)"      },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x4,       NULL, L"Catalan"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x5,       NULL, L"Chinese (Simplified)"  },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x6,       NULL, L"Chinese (Traditional)" },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x7,       NULL, L"Croatian"              },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x8,       NULL, L"Czech"                 },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x9,       NULL, L"Danish"                },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0xa,       NULL, L"Dutch"                 },
	{MF_STRING | MF_ENABLED | MF_CHECKED,  IDM_LANGUAGE + 0xb,       NULL, L"English (US)"          },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0xc,       NULL, L"Estonian"              },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0xd,       NULL, L"Farsi"                 },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0xe,       NULL, L"Filipino (Tagalog)"    },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0xf,       NULL, L"Finnish"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x10,      NULL, L"French (FR)"           },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x11,      NULL, L"German"                },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x12,      NULL, L"Greek"                 },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x13,      NULL, L"Hindi"                 },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x14,      NULL, L"Hungarian"             },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x15,      NULL, L"Icelandic"             },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x16,      NULL, L"Italian"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x17,      NULL, L"Japanese"              },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x18,      NULL, L"Korean"                },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x19,      NULL, L"Latvian"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1a,      NULL, L"Macedonian"            },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1b,      NULL, L"Norwegian"             },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1c,      NULL, L"Polish"                },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1d,      NULL, L"Portuguese (BR)"       },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1e,      NULL, L"Portuguese"            },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1f,      NULL, L"Romanian"              },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x20,      NULL, L"Russian"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x21,      NULL, L"Serbian"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x22,      NULL, L"Slovak"                },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x23,      NULL, L"Slovenian"             },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x24,      NULL, L"Spanish (AR)"          },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x25,      NULL, L"Spanish (ES)"          },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x26,      NULL, L"Swedish"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x27,      NULL, L"Thai"                  },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x28,      NULL, L"Turkish"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x29,      NULL, L"Ukrainian"             },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x2a,      NULL, L"Vietnamese (VI)"       },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x2b,      NULL, L"Yiddish"               },
	{0,0,NULL,NULL}
};

WGX_MENU gui_config_menu[] = {
	{MF_STRING | MF_ENABLED,IDM_CFG_GUI_FONT,                        NULL, L"&Font\tF9"    },
	{MF_STRING | MF_ENABLED,IDM_CFG_GUI_SETTINGS,                    NULL, L"&Options\tF10"},
	{0,0,NULL,NULL}
};

WGX_MENU boot_config_menu[] = {
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_CFG_BOOT_ENABLE,      NULL, L"&Enable\tF11"},
	{MF_STRING | MF_ENABLED,IDM_CFG_BOOT_SCRIPT,                     NULL, L"&Script\tF12"},
	{0,0,NULL,NULL}
};

WGX_MENU settings_menu[] = {
	{MF_STRING | MF_ENABLED | MF_POPUP, IDM_LANGUAGE,    language_menu,     L"&Language"            },
	{MF_STRING | MF_ENABLED | MF_POPUP, IDM_CFG_GUI,     gui_config_menu,   L"&Graphical interface" },
	{MF_STRING | MF_ENABLED | MF_POPUP, IDM_CFG_BOOT,    boot_config_menu,  L"&Boot time scan"      },
	{MF_STRING | MF_ENABLED,            IDM_CFG_REPORTS, NULL,              L"&Reports\tCtrl+R"     },
	{0,0,NULL,NULL}
};

WGX_MENU help_menu[] = {
	{MF_STRING | MF_ENABLED,IDM_CONTENTS,                            NULL, L"&Contents\tF1"},
	{MF_SEPARATOR,0,NULL,NULL},
	{MF_STRING | MF_ENABLED,IDM_BEST_PRACTICE,                       NULL, L"Best &practice\tF2"},
	{MF_STRING | MF_ENABLED,IDM_FAQ,                                 NULL, L"&FAQ\tF3"},
	{MF_SEPARATOR,0,NULL,NULL},
	{MF_STRING | MF_ENABLED,IDM_ABOUT,                               NULL, L"&About\tF4"},
	{0,0,NULL,NULL}
};

WGX_MENU main_menu[] = {
	{MF_STRING | MF_ENABLED | MF_POPUP, IDM_ACTION,   action_menu,        L"&Action"},
	{MF_STRING | MF_ENABLED | MF_POPUP, IDM_REPORT,   report_menu,        L"&Report"},
	{MF_STRING | MF_ENABLED | MF_POPUP, IDM_SETTINGS, settings_menu,      L"&Settings"},
	{MF_STRING | MF_ENABLED | MF_POPUP, IDM_HELP,     help_menu,          L"&Help"},
	{0,0,NULL,NULL}
};

extern int boot_time_defrag_enabled;

/**
 * @brief Creates main menu.
 * @return Zero for success,
 * negative value otherwise.
 */
int CreateMainMenu(void)
{
	short *s = L"";
	short buffer[256];
	
	/* create menu */
	hMainMenu = WgxBuildMenu(main_menu);
	
	/* attach menu to the window */
	if(!SetMenu(hWindow,hMainMenu)){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
			"Cannot set main menu!");
		DestroyMenu(hMainMenu);
		return (-1);
	}
	
	if(skip_removable == 0){
		CheckMenuItem(hMainMenu,
			IDM_IGNORE_REMOVABLE_MEDIA,
			MF_BYCOMMAND | MF_UNCHECKED);
	}
	
#ifndef UDEFRAG_PORTABLE
	if(IsBootTimeDefragEnabled()){
		boot_time_defrag_enabled = 1;
		CheckMenuItem(hMainMenu,
			IDM_CFG_BOOT_ENABLE,
			MF_BYCOMMAND | MF_CHECKED);
	}
#else
	EnableMenuItem(hMainMenu,IDM_CFG_BOOT_ENABLE,MF_BYCOMMAND | MF_GRAYED);
#endif

	if(hibernate_instead_of_shutdown)
		s = WgxGetResourceString(i18n_table,L"HIBERNATE_PC_AFTER_A_JOB");
	else
		s = WgxGetResourceString(i18n_table,L"SHUTDOWN_PC_AFTER_A_JOB");
	_snwprintf(buffer,256,L"%ws\tCtrl+S",s);
	buffer[255] = 0;
	ModifyMenuW(hMainMenu,IDM_SHUTDOWN,MF_BYCOMMAND | MF_STRING,IDM_SHUTDOWN,buffer);

	if(!DrawMenuBar(hWindow))
		WgxDbgPrintLastError("Cannot redraw main menu");

	return 0;
}

/** @} */
