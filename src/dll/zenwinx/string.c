/*
 *  ZenWINX - WIndows Native eXtended library.
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
 * @file string.c
 * @brief String processing routines.
 * @addtogroup String
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

/*	1. wcsstr(L"AGTENTLEPSE.SDFGSDFSDFSRG",L"AGTENTLEPSE"); x 1 mln. times = 63 ms
	2. empty cycle = 0 ms
	3. wcsistr(L"AGTENTLEPSE.SDFGSDFSDFSRG",L"AGTENTLEPSE"); x 1 mln. times = 340 ms
	4. wcsistr 2 kanji strings x 1 mln. times = 310 ms
	WCHAR s1[] = {0x30ea,0x30e0,0x30fc,0x30d0,0x30d6,0x30eb,0x30e1,0x30c7,0x30a3,0x30a2,0x306f,0x9664,0x5916,0};
	WCHAR s2[] = {0x30ea,0x30e0,0x30fc,0x30d0,0x30d6,0x30eb,0x30e1,0x30c7,0x30a3,0x30a2,0x306f,0};
	ULONGLONG t = _rdtsc();
	for(i = 0; i < 1000000; i++) wcsistr(s1,s2);//wcsistr(L"AGTENTLEPSE.SDFGSDFSDFSRG",L"AGTENTLEPSE");
	DbgPrint("AAAA %I64u ms\n",_rdtsc() - t);
*/

/**
 * @brief Case insensitive version of wcsstr().
 */
wchar_t * __cdecl winx_wcsistr(const wchar_t * wcs1,const wchar_t * wcs2)
{
	wchar_t *cp = (wchar_t *)wcs1;
	wchar_t *s1, *s2;

	if(wcs1 == NULL || wcs2 == NULL) return NULL;
	
	while(*cp){
		s1 = cp;
		s2 = (wchar_t *)wcs2;
		
		while(*s1 && *s2 && !( towlower((int)(*s1)) - towlower((int)(*s2)) )){ s1++, s2++; }
		if(!*s2) return cp;
		cp++;
	}
	
	return NULL;
}

/** @} */
