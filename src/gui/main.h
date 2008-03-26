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
 *  GUI - common definitions.
 */

#ifndef _DFRG_MAIN_H_
#define _DFRG_MAIN_H_

#define WIN32_NO_STATUS
#include <windows.h>
/*
* Next definition is very important for mingw:
* _WIN32_IE must be no less than 0x0400
* to include some important constant definitions.
*/
#ifndef _WIN32_IE
#define _WIN32_IE 0x0400
#endif
#include <commctrl.h>

#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>
#include <math.h>

#define lua_c
#include "../lua5.1/lua.h"
#include "../lua5.1/lauxlib.h"
#include "../lua5.1/lualib.h"

#include "../include/udefrag.h"
#include "../include/ultradfg.h"
#include "resource.h"

#ifndef USE_WINDDK
#ifndef SetWindowLongPtr
#define SetWindowLongPtr SetWindowLong
#endif
#define LONG_PTR LONG
#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC GWL_WNDPROC
#endif
#endif

#ifndef LR_VGACOLOR
/* this constant is not defined in winuser.h on mingw */
#define LR_VGACOLOR         0x0080
#endif

#define BLOCKS_PER_HLINE  52
#define BLOCKS_PER_VLINE  14
#define N_BLOCKS          (BLOCKS_PER_HLINE * BLOCKS_PER_VLINE)

void GetPrefs(void);
void SavePrefs(void);

void BuildResourceTable(void);
void DestroyResourceTable(void);
int  GetResourceString(short *id,short *buf,int maxchars);
void SetText(HWND hWnd, short *id);

#endif /* _DFRG_MAIN_H_ */
