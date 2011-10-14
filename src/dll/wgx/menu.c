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
 * @internal
 * @brief Adds menu bitmaps to menu items.
 * @param[in] hBMSrc handle to the toolbar bitmap.
 * @param[in] nPos position of menu bitmap.
 * @return Handle of the created bitmap,
 * NULL indicates failure.
 * @note Based on an example at
 * http://forum.pellesc.de/index.php?topic=3265.0
 */
HBITMAP WgxGetToolbarBitmapForMenu(HBITMAP hBMSrc, int nPos)
{
	HDC hDCSrc = NULL, hDCDst = NULL;
	BITMAP bm = {0};
	HBITMAP hBMDst = NULL, hOldBmp = NULL;
	int cx, cy;

	cx = GetSystemMetrics(SM_CXMENUCHECK);
	cy = GetSystemMetrics(SM_CYMENUCHECK);
    if(nPos == 0)
        WgxDbgPrint("Menu bitmap size %d x %d",cx,cy);
    
	if ((hDCSrc = CreateCompatibleDC(NULL)) != NULL) {
		if ((hDCDst = CreateCompatibleDC(NULL)) != NULL) {
			SelectObject(hDCSrc, hBMSrc);
			GetObject(hBMSrc, sizeof(bm), &bm);
			hBMDst = CreateBitmap(cx, cy, bm.bmPlanes, bm.bmBitsPixel, NULL);
			if (hBMDst) {
				//GetObject(hBMDst, sizeof(bm), &bm);
				hOldBmp = SelectObject(hDCDst, hBMDst);
				StretchBlt(hDCDst, 0, 0, cx, cy, hDCSrc, nPos*bm.bmHeight, 0, bm.bmHeight, bm.bmHeight, SRCCOPY);
				SelectObject(hDCDst, hOldBmp);
				GetObject(hBMDst, sizeof(bm), &bm);
			}
			DeleteDC(hDCDst);
		}
		DeleteDC(hDCSrc);
	}
	return hBMDst;
}

/**
 * @internal
 * @brief WgxBuildMenu helper.
 */
static HMENU BuildMenu(HMENU hMenu,WGX_MENU *menu_table,HBITMAP toolbar_bmp)
{
	MENUITEMINFO mi;
	HMENU hPopup;
    HBITMAP hBMitem;
	int i;

	for(i = 0; menu_table[i].flags || menu_table[i].id || menu_table[i].text; i++){
		if(menu_table[i].flags & MF_SEPARATOR){
			if(!AppendMenuW(hMenu,MF_SEPARATOR,0,NULL))
				goto append_menu_fail;
			continue;
		}
		if(menu_table[i].flags & MF_POPUP){
			hPopup = WgxBuildPopupMenu(menu_table[i].submenu,toolbar_bmp);
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

        if(toolbar_bmp != NULL){
            hBMitem = NULL;
            
            if(menu_table[i].toolbar_image_id > -1){
                hBMitem = WgxGetToolbarBitmapForMenu(toolbar_bmp,menu_table[i].toolbar_image_id);
                
                if(hBMitem != NULL)
                    SetMenuItemBitmaps(hMenu,menu_table[i].id,MF_BYCOMMAND,hBMitem,hBMitem);
            }
        }
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
 * @param[in] toolbar_bmp handle to the toolbar bitmap.
 * @return Handle of the created menu,
 * NULL indicates failure.
 * @note The following flags are supported:
 * - MF_STRING - data field must point to
 * zero terminated Unicode string.
 * - MF_SEPARATOR - all fields are ignored.
 * - MF_POPUP - id field must point to
 * another menu table describing a submenu.
 */
HMENU WgxBuildMenu(WGX_MENU *menu_table,HBITMAP toolbar_bmp)
{
	HMENU hMenu;
	
	if(menu_table == NULL)
		return NULL;

	hMenu = CreateMenu();
	if(hMenu == NULL){
		WgxDbgPrintLastError("WgxBuildMenu: cannot create menu");
		return NULL;
	}
	
	if(BuildMenu(hMenu,menu_table,toolbar_bmp) == NULL){
		DestroyMenu(hMenu);
		return NULL;
	}
	
	return hMenu;
}

/**
 * @brief WgxBuildMenu analog, 
 * but works for popup menus.
 */
HMENU WgxBuildPopupMenu(WGX_MENU *menu_table,HBITMAP toolbar_bmp)
{
	HMENU hMenu;
	
	if(menu_table == NULL)
		return NULL;

	hMenu = CreatePopupMenu();
	if(hMenu == NULL){
		WgxDbgPrintLastError("WgxBuildPopupMenu: cannot create menu");
		return NULL;
	}
	
	if(BuildMenu(hMenu,menu_table,toolbar_bmp) == NULL){
		DestroyMenu(hMenu);
		return NULL;
	}
	
	return hMenu;
}

/** @} */
