/*
 *  ZenWINX - WIndows Native eXtended library.
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
* zenwinx.dll time related procedures.
*/

#include "ntndk.h"
#include "zenwinx.h"

/****f* zenwinx.time/winx_str2time
* NAME
*    winx_str2time
* SYNOPSIS
*    time = winx_str2time(string);
* FUNCTION
*    Converts formatted string to time value in seconds.
* INPUTS
*    string - formatted string to be converted
*             Format example: 3y 12d 4h 8m 37s. 
* RESULT
*    Time interval in seconds.
* SEE ALSO
*    winx_time2str
******/
ULONGLONG __stdcall winx_str2time(char *string)
{
	ULONGLONG time = 0;
	char buffer[128] = "";
	int index = 0;
	int i;
	char c;
	ULONGLONG k;
	
	for(i = 0;; i++){ /* loop through all characters of the string */
		c = string[i];
		if(!c) break;
		if(c >= '0' && c <= '9'){
			buffer[index] = c;
			index++;
		}

		k = 0;
		c = (char)toupper((int)c);
		switch(c){
		case 'S':
			k = 1;
			break;
		case 'M':
			k = 60;
			break;
		case 'H':
			k = 3600;
			break;
		case 'D':
			k = 3600 * 24;
			break;
		case 'Y':
			k = 3600 * 24 * 356;
			break;
		}
		if(k){
			buffer[index] = 0;
			index = 0;
			time += k * _atoi64(buffer);
		}
	}
	return time;
}

/****f* zenwinx.time/winx_time2str
* NAME
*    winx_time2str
* SYNOPSIS
*    result = winx_time2str(time,buffer,size);
* FUNCTION
*    Converts time value in seconds to formatted string.
* INPUTS
*    time   - interval in seconds
*    buffer - storage for the resulting string
*    size   - length of the buffer
* RESULT
*    The number of characters stored.
* SEE ALSO
*    winx_str2time
******/
int __stdcall winx_time2str(ULONGLONG time,char *buffer,int size)
{
	ULONGLONG y,d,h,m,s;
	ULONGLONG t;
	int result;
	
	t = time;
	y = t / (3600 * 24 * 356);
	t = t % (3600 * 24 * 356);
	d = t / (3600 * 24);
	t = t % (3600 * 24);
	h = t / 3600;
	t = t % 3600;
	m = t / 60;
	s = t % 60;
	
	result = _snprintf(buffer,size - 1,
		"%I64uy %I64ud %I64uh %I64um %I64us",
		y,d,h,m,s);
	buffer[size - 1] = 0;
	return result;
}
