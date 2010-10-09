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
	{MF_STRING | MF_ENABLED,IDM_ANALYZE,                             L"&Analyze\tF5"   },
	{MF_STRING | MF_ENABLED,IDM_DEFRAG,                              L"&Defragment\tF6"},
	{MF_STRING | MF_ENABLED,IDM_OPTIMIZE,                            L"&Optimize\tF7"  },
	{MF_STRING | MF_ENABLED,IDM_STOP,                                L"&Stop\tCtrl+C"  },
	{MF_SEPARATOR,0,NULL},
	{MF_STRING | MF_ENABLED | MF_CHECKED,IDM_IGNORE_REMOVABLE_MEDIA, L"Skip removable &media\tCtrl+M"},
	{MF_STRING | MF_ENABLED,IDM_RESCAN,                              L"&Rescan drives\tCtrl+D"},
	{MF_SEPARATOR,0,NULL},
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_SHUTDOWN,             L"Shutdown &PC when done\tCtrl+S"},
	{MF_SEPARATOR,0,NULL},
	{MF_STRING | MF_ENABLED,IDM_EXIT,                                L"E&xit\tAlt+F4"},
	{0,0,NULL}
};

WGX_MENU report_menu[] = {
	{MF_STRING | MF_ENABLED,IDM_SHOW_REPORT,                         L"&Show report\tF8"},
	{0,0,NULL}
};

WGX_MENU language_menu[] = {
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1,       L"Belarusian"            },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x2,       L"Bulgarian"             },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x3,       L"Burmese (Padauk)"      },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x4,       L"Catalan"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x5,       L"Chinese (Simplified)"  },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x6,       L"Chinese (Traditional)" },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x7,       L"Croatian"              },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x8,       L"Czech"                 },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x9,       L"Danish"                },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0xa,       L"Dutch"                 },
	{MF_STRING | MF_ENABLED | MF_CHECKED,  IDM_LANGUAGE + 0xb,       L"English (US)"          },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0xc,       L"Estonian"              },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0xd,       L"Farsi"                 },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0xe,       L"Filipino (Tagalog)"    },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0xf,       L"Finnish"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x10,      L"French (FR)"           },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x11,      L"German"                },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x12,      L"Greek"                 },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x13,      L"Hindi"                 },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x14,      L"Hungarian"             },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x15,      L"Icelandic"             },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x16,      L"Italian"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x17,      L"Japanese"              },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x18,      L"Korean"                },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x19,      L"Latvian"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1a,      L"Macedonian"            },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1b,      L"Norwegian"             },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1c,      L"Polish"                },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1d,      L"Portuguese (BR)"       },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1e,      L"Portuguese"            },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x1f,      L"Romanian"              },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x20,      L"Russian"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x21,      L"Serbian"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x22,      L"Slovak"                },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x23,      L"Slovenian"             },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x24,      L"Spanish (AR)"          },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x25,      L"Spanish (ES)"          },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x26,      L"Swedish"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x27,      L"Thai"                  },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x28,      L"Turkish"               },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x29,      L"Ukrainian"             },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x2a,      L"Vietnamese (VI)"       },
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_LANGUAGE + 0x2b,      L"Yiddish"               },
	{0,0,NULL}
};

WGX_MENU gui_config_menu[] = {
	{MF_STRING | MF_ENABLED,IDM_CFG_GUI_FONT,                        L"&Font\tF9"},
	{MF_STRING | MF_ENABLED,IDM_CFG_GUI_SETTINGS,                    L"&Options\tF10"},
	{0,0,NULL}
};

WGX_MENU boot_config_menu[] = {
	{MF_STRING | MF_ENABLED | MF_UNCHECKED,IDM_CFG_BOOT_ENABLE,      L"&Enable\tF11"},
	{MF_STRING | MF_ENABLED,IDM_CFG_BOOT_SCRIPT,                     L"&Script\tF12"},
	{0,0,NULL}
};

WGX_MENU settings_menu[] = {
	{MF_STRING | MF_ENABLED | MF_POPUP,(UINT_PTR)language_menu,      L"&Language"},
	{MF_STRING | MF_ENABLED | MF_POPUP,(UINT_PTR)gui_config_menu,    L"&Graphical interface"},
	{MF_STRING | MF_ENABLED | MF_POPUP,(UINT_PTR)boot_config_menu,   L"&Boot time scan"},
	{MF_STRING | MF_ENABLED,IDM_CFG_REPORTS,                         L"&Reports\tCtrl+R"},
	{0,0,NULL}
};

WGX_MENU help_menu[] = {
	{MF_STRING | MF_ENABLED,IDM_CONTENTS,                            L"&Contents\tF1"},
	{MF_SEPARATOR,0,NULL},
	{MF_STRING | MF_ENABLED,IDM_BEST_PRACTICE,                       L"Best &practice\tF2"},
	{MF_STRING | MF_ENABLED,IDM_FAQ,                                 L"&FAQ\tF3"},
	{MF_SEPARATOR,0,NULL},
	{MF_STRING | MF_ENABLED,IDM_ABOUT,                               L"&About\tF4"},
	{0,0,NULL}
};

WGX_MENU main_menu[] = {
	{MF_STRING | MF_ENABLED | MF_POPUP,(UINT_PTR)action_menu,        L"&Action"},
	{MF_STRING | MF_ENABLED | MF_POPUP,(UINT_PTR)report_menu,        L"&Report"},
	{MF_STRING | MF_ENABLED | MF_POPUP,(UINT_PTR)settings_menu,      L"&Settings"},
	{MF_STRING | MF_ENABLED | MF_POPUP,(UINT_PTR)help_menu,          L"&Help"},
	{0,0,NULL}
};

extern int boot_time_defrag_enabled;

/**
 * @brief Creates main menu.
 * @return Zero for success,
 * negative value otherwise.
 */
int CreateMainMenu(void)
{
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

	// TODO: handle hibernate instead of shutdown option

	if(!DrawMenuBar(hWindow))
		WgxDbgPrintLastError("Cannot redraw main menu");

	return 0;
}

/** @} */
