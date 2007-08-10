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

/*
 * Miscellaneous functions.
 */

#include <windows.h>

#include "../include/misc.h"

#define SPACE_CHAR	0x20

char *FormatBySize(ULONGLONG long_number,char *s,int alignment)
{
	char symbols[3] = {'M','G','T'};
	char c;
	double short_number;
	unsigned int number,dvs = 100000,digit;
	unsigned int i,index = 3,n = 0;
	int flag = 0;

	/* enough for ~8 mln. Tb volumes */
	short_number = (double)(signed __int64)long_number / (1024.00 * 1024.00);
	for(i = 0; short_number >= 1024.00; i++)
		short_number /= 1024.00;
	if(i > 2) i = 2;
	c = symbols[i];

	s[0] = s[1] = s[2] = SPACE_CHAR;
	number = (int)(short_number * 100);
	for(i = 0; i < 3; i++)
	{
		digit = number / dvs;
		number %= dvs;
		dvs /= 10;
		if(digit || flag)
		{
			flag = 1;
			s[index] = digit + '0';
			index++; n++;
		}
	}

	s[index] = number / 100 + '0';
	s[index+1] = '.';
	number %= 100;
	s[index+2] = number / 10 + '0';
	s[index+3] = number % 10 + '0';
	s[index+4] = SPACE_CHAR;
	s[index+5] = c;
	s[index+6] = 'b';
	s[index+7] = 0;
	if(alignment == LEFT_ALIGNED)
		return (s + 3);
	else
		return (s + n);
}

void Format2(double number,char *s)
{
	register unsigned int digit;
	register unsigned int index = 0;
	register unsigned int flag = 0;

	digit = (int)number / 100;
	if(digit)
	{
		flag = 1;
		s[0] = digit + '0';
		index ++;
	}
	digit = (int)number % 100;
	digit /= 10;
	if(digit || flag)
	{
		s[index] = digit + '0';
		index ++;
	}
	digit = (int)number % 10;
	s[index] = digit + '0';
	s[index+1] = '.';

	number *= 100;
	digit = (int)number % 100;
	s[index+2] = digit / 10 + '0';
	s[index+3] = digit % 10 + '0';
	s[index+4] = 0;
}
