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

/****f* udefrag.misc/fbsize
* NAME
*    fbsize
* SYNOPSIS
*    fbsize(buffer, n);
* FUNCTION
*    Converts the 64-bit number to string with one of
*    the suffixes: Kb, Mb, Gb, Tb, Pb, Eb.
*    With two digits after a dot.
* INPUTS
*    buffer - pointer to the buffer
*    n      - 64-bit integer number
* RESULT
*    The number of characters stored.
* EXAMPLE
*    char buffer[16];
*    fbsize(buffer,n);
* NOTES
*    Original name was StrFormatByteSize.
* SEE ALSO
*    fbsize2, dfbsize2
******/
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

/****f* udefrag.misc/fbsize2
* NAME
*    fbsize2
* SYNOPSIS
*    fbsize2(buffer, n);
* FUNCTION
*    fbsize analog, but returns an integer number.
* INPUTS
*    buffer - pointer to the buffer
*    n      - 64-bit integer number
* RESULT
*    The number of characters stored.
* EXAMPLE
*    char buffer[16];
*    fbsize2(buffer,n);
* SEE ALSO
*    fbsize, dfbsize2
******/
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

/****f* udefrag.misc/dfbsize2
* NAME
*    dfbsize2
* SYNOPSIS
*    dfbsize2(buffer, pn);
* FUNCTION
*    Decode fbsize2 formatted string.
* INPUTS
*    buffer - pointer to the buffer
*    pn     - pointer to 64-bit integer number
* RESULT
*    Zero for success, -1 otherwise.
* EXAMPLE
*    char buffer[16];
*    dfbsize2(buffer,&n);
* SEE ALSO
*    fbsize, fbsize2
******/
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
