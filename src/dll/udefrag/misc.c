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
 *  udefrag.dll - middle layer between driver and user interfaces:
 *  miscellaneous functions.
 */

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../include/ntndk.h"
#include "../../include/udefrag.h"
#include "../zenwinx/zenwinx.h"

/* winx_pop_error() equivalent */
void __stdcall udefrag_pop_error(char *buffer, int size)
{
	winx_pop_error(buffer,size);
}

/* winx_pop_werror() equivalent */
void __stdcall udefrag_pop_werror(short *buffer, int size)
{
	winx_pop_werror(buffer,size);
}

/* winx_fbsize() equivalent */
int __stdcall udefrag_fbsize(ULONGLONG number, int digits, char *buffer, int length)
{
	return winx_fbsize(number,digits,buffer,length);
}

/* winx_dfbsize() equivalent */
int __stdcall udefrag_dfbsize(char *string,ULONGLONG *pnumber)
{
	return winx_dfbsize(string,pnumber);
}

