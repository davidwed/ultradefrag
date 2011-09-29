/*
 *  WGX - Windows GUI Extended Library.
 *  Copyright (c) 2007-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

#define WIN32_NO_STATUS
#include <windows.h>

#include "wgx.h"

/**
 * @brief WgxBuildMenu helper.
 */
static HMENU BuildMenu(HMENU hMenu,WGX_MENU *menu_table)
{
	MENUITEMINFO mi;
	HMENU hPopup;
	int i;

	for(i = 0; menu_table[i].flags || menu_table[i].id || menu_table[i].text; i++){
		if(menu_table[i].flags & MF_SEPARATOR){
			if(!AppendMenuW(hMenu,MF_SEPARATOR,0,NULL))
				goto append_menu_fail;
			continue;
		}
		if(menu_table[i].flags & MF_POPUP){
			hPopup = WgxBuildPopupMenu(menu_table[i].submenu);
			if(hPopup == NULL){
				WgxDbgPrintLastError("WgxBuildMenu: cannot build popup menu");
				return NULL;
			}
			if(!AppendMenuW(hMenu,menu_table[i].flags,(UINT_PTR)hPopup,menu_table[i].text))
				goto append_menu_fail;
			/* set id anyway */
			memset(&mi,0,sizeof(MENUITEMINFO));
			mi.cbSize = sizeof(MENUITEMINFO);
			mi.fMask = MIIM_ID;
			mi.fType = MFT_STRING;
			mi.wID = menu_table[i].id;
			if(!SetMenuItemInfo(hMenu,i,TRUE,&mi))
				goto set_menu_info_fail;
			continue;
		}
		if(!AppendMenuW(hMenu,menu_table[i].flags,menu_table[i].id,menu_table[i].text))
			goto append_menu_fail;
	}
	
	/* success */
	return hMenu;

append_menu_fail:
	WgxDbgPrintLastError("WgxBuildMenu: cannot append menu");
	return NULL;

set_menu_info_fail:
	WgxDbgPrintLastError("WgxBuildMenu: cannot set menu item id");
	return NULL;
}

/**
 * @brief Builds menu from user defined tables.
 * @param[in] menu_table pointer to array of
 * WGX_MENU structures. All fields of the structure
 * equal to zero indicate the end of the table.
 * @return Handle of the created menu,
 * NULL indicates failure.
 * @note The following flags are supported:
 * - MF_STRING - data field must point to
 * zero terminated Unicode string.
 * - MF_SEPARATOR - all fields are ignored.
 * - MF_POPUP - id field must point to
 * another menu table describing a submenu.
 */
HMENU WgxBuildMenu(WGX_MENU *menu_table)
{
	HMENU hMenu;
	
	if(menu_table == NULL)
		return NULL;

	hMenu = CreateMenu();
	if(hMenu == NULL){
		WgxDbgPrintLastError("WgxBuildMenu: cannot create menu");
		return NULL;
	}
	
	if(BuildMenu(hMenu,menu_table) == NULL){
		DestroyMenu(hMenu);
		return NULL;
	}
	
	return hMenu;
}

/**
 * @brief WgxBuildMenu analog, 
 * but works for popup menus.
 */
HMENU WgxBuildPopupMenu(WGX_MENU *menu_table)
{
	HMENU hMenu;
	
	if(menu_table == NULL)
		return NULL;

	hMenu = CreatePopupMenu();
	if(hMenu == NULL){
		WgxDbgPrintLastError("WgxBuildPopupMenu: cannot create menu");
		return NULL;
	}
	
	if(BuildMenu(hMenu,menu_table) == NULL){
		DestroyMenu(hMenu);
		return NULL;
	}
	
	return hMenu;
}

/** @} */
