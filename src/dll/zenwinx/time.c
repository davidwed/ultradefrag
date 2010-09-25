/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file time.c
 * @brief Time and performance.
 * @addtogroup Time
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

/**
 * @brief Converts a formatted string to the time value in seconds.
 * @param[in] string the formatted string to be converted.
 * Format example: 3y 12d 4h 8m 37s.
 * @return Time interval in seconds.
 */
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
		c = winx_toupper(c);
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

/**
 * @brief Converts a time value in seconds to the formatted string.
 * @param[in] time the time interval, in seconds.
 * @param[out] buffer the storage for the resulting string.
 * @param[in] size the length of the buffer, in characters.
 * @return The number of characters stored.
 * @note The time interval should not exceed 140 years
 * (0xFFFFFFFF seconds), otherwise it will be truncated.
 */
int __stdcall winx_time2str(ULONGLONG time,char *buffer,int size)
{
	ULONG y,d,h,m,s;
	ULONG t;
	int result;
	
	t = (ULONG)time; /* because w2k has no _aulldvrm() call in ntdll.dll */
	y = t / (3600 * 24 * 356);
	t = t % (3600 * 24 * 356);
	d = t / (3600 * 24);
	t = t % (3600 * 24);
	h = t / 3600;
	t = t % 3600;
	m = t / 60;
	s = t % 60;
	
	result = _snprintf(buffer,size - 1,
		"%uy %ud %uh %um %us",
		y,d,h,m,s);
	buffer[size - 1] = 0;
	return result;
}

/**
 * @brief Returns time interval since 
 * some abstract unique event in the past.
 * @return Time, in milliseconds.
 * Zero indicates failure.
 * @note
 * - Useful for performance measures.
 * - Has no physical meaning.
 */
ULONGLONG __stdcall winx_xtime(void)
{
	NTSTATUS Status;
	LARGE_INTEGER frequency;
	LARGE_INTEGER counter;
	
	Status = NtQueryPerformanceCounter(&counter,&frequency);
	if(!NT_SUCCESS(Status)){
		DebugPrint("NtQueryPerformanceCounter() failed: %x!\n",(UINT)Status);
		return 0;
	}
	if(!frequency.QuadPart){
		DebugPrint("Your hardware has no support for High Resolution timer!\n");
		return 0;
	}
	/*DebugPrint("*** Frequency = %I64u, Counter = %I64u ***\n",frequency.QuadPart,counter.QuadPart);*/
	/* TODO: make calculation more accurately to ensure that overflow is impossible */
	return ((1000 * counter.QuadPart) / frequency.QuadPart);
}

/** @} */
