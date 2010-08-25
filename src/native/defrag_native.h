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
* UltraDefrag native interface header.
*/

#ifndef _DEFRAG_NATIVE_H_
#define _DEFRAG_NATIVE_H_

#include "../include/ntndk.h"

#include "../include/udefrag-kernel.h"
#include "../include/udefrag.h"
#include "../include/ultradfgver.h"
#include "../dll/zenwinx/zenwinx.h"

/* uncomment it if you want to replace smss.exe by this program */
//#define USE_INSTEAD_SMSS

/* define how many lines to display for each help page */
#define HELP_DISPLAY_ROWS 14

#define short_dbg_delay() winx_sleep(3000)
#define long_dbg_delay()  winx_sleep(10000)

#endif /* _DEFRAG_NATIVE_H_ */
