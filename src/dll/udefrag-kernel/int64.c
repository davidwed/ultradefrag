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

/**
 * @file int64.c
 * @brief 64-bit integer support code.
 * @{
 */

#include "globals.h"

#if defined(__GNUC__)
#ifndef _WIN64
ULONGLONG __udivdi3(ULONGLONG n, ULONGLONG d)
{
	return _aulldiv(n,d);
}
LONGLONG __divdi3(LONGLONG n,LONGLONG d)
{
	return _alldiv(n,d);
}
ULONGLONG __umoddi3(ULONGLONG u, ULONGLONG v)
{
	return _aullrem(u,v);
}
#endif
#endif

#if defined(__POCC__)
ULONGLONG __cdecl __ulldiv(ULONGLONG n, ULONGLONG d)
{
	return _aulldiv(n,d);
}
LONGLONG __cdecl __lldiv(LONGLONG u, LONGLONG v)
{
	return _alldiv(u,v);
}
LONGLONG __cdecl __llmul(LONGLONG n, LONGLONG d)
{
	return _allmul(n,d);
}
ULONGLONG __cdecl __ullmod(ULONGLONG u, ULONGLONG v)
{
	return _allrem(u,v);
}
#endif

/** @} */
