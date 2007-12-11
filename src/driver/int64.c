/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

/* int64.c - memory functions for long arrays */

/*#include "driver.h"*/

/* This functions aren't used in UD v1.2.4+ */

/*  NOTE! If size > MAXULONG then return ERROR.
	Because more than 4 bln. CLUSTERS on disk
	is very strange and illogical!
*/
/*
void *_int64_malloc(ULONGLONG size)
{
	if(size <= (ULONGLONG)MAXULONG)
		return AllocatePool(NonPagedPool,(ULONG)size);
	return NULL;
}
void *_int64_memset(void *buf,int ch,ULONGLONG size)
{
	if(size <= (ULONGLONG)MAXULONG)
		return memset(buf,ch,(ULONG)size);
	return NULL;
}
*/