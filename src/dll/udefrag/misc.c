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
#include "../zenwinx/src/zenwinx.h"

/* original name was StrFormatByteSize */
int __stdcall fbsize(char *s,ULONGLONG n)
{
	char symbols[] = {'K','M','G','T','P','E'};
	double fn;
	unsigned int i;
	unsigned int in;

	/* convert n to Kb - enough for ~8 mln. Tb volumes */
	fn = (double)(signed __int64)n / 1024.00;
	for(i = 0; fn >= 1024.00; i++) fn /= 1024.00;
	if(i > sizeof(symbols) - 1) i = sizeof(symbols) - 1;
	/* %.2f don't work in native mode */
	in = (unsigned int)(fn * 100.00);
	return sprintf(s,"%u.%02u %cb",in / 100,in % 100,symbols[i]);
}

/* fbsize variant for other purposes */
int __stdcall fbsize2(char *s,ULONGLONG n)
{
	char symbols[] = {'K','M','G','T','P','E'};
	unsigned int i;

	if(n < 1024) return sprintf(s,"%u",(int)n);
	n /= 1024;
	for(i = 0; n >= 1024; i++) n /= 1024;
	if(i > sizeof(symbols) - 1) i = sizeof(symbols) - 1;
	return sprintf(s,"%u %cb",(int)n,symbols[i]);
}

/* decode fbsize2 formatted string */
int __stdcall dfbsize2(char *s,ULONGLONG *pn)
{
	char symbols[] = {'K','M','G','T','P','E'};
	char t[64];
	signed int i;
	ULONGLONG m = 1;

	if(strlen(s) > 63) return (-1);
	strcpy(t,s);
	_strupr(t);
	for(i = 0; i < sizeof(symbols); i++)
		if(strchr(t,symbols[i])) break;
	if(i < sizeof(symbols)) /* suffix found */
		for(; i >= 0; i--) m *= 1024;
	*pn = m * _atoi64(s);
	return 0;
}
