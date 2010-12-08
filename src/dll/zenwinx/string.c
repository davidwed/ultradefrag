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
 * @brief Strings.
 * @addtogroup String
 * @{
 */

#include "zenwinx.h"

/**
 * @brief Size of the buffer used by winx_vsprintf
 * initially. Larger sizes tend to reduce time needed
 * to format long strings.
 */
#define WINX_VSPRINTF_BUFFER_SIZE 128

/**
 * @brief Reliable _toupper analog.
 * @details MSDN states: "In order for toupper to give
 * the expected results, __isascii and islower must both
 * return nonzero". winx_toupper has no such limitation.
 * @note Converts ANSI characters, does not accept other
 * encodings.
 */
char __cdecl winx_toupper(char c)
{
	char s[2];
	
	s[0] = c;
	s[1] = 0;
	(void)_strupr(s);

	return s[0];
}

/**
 * @brief Reliable _tolower analog.
 * @details MSDN states: "In order for tolower to give
 * the expected results, __isascii and isupper must both
 * return nonzero". winx_tolower has no such limitation.
 * @note Converts ANSI characters, does not accept other
 * encodings.
 */
char __cdecl winx_tolower(char c)
{
	char s[2];
	
	s[0] = c;
	s[1] = 0;
	(void)_strlwr(s);

	return s[0];
}

/**
 * @brief strdup equivalent.
 */
char * __cdecl winx_strdup(const char *s)
{
	int length;
	char *cp;
	
	if(s == NULL)
		return NULL;
	
	length = strlen(s);
	cp = winx_heap_alloc((length + 1) * sizeof(char));
	if(cp) strcpy(cp,s);
	return cp;
}

/**
 * @brief wcsdup equivalent.
 */
wchar_t * __cdecl winx_wcsdup(const wchar_t *s)
{
	int length;
	wchar_t *cp;
	
	if(s == NULL)
		return NULL;
	
	length = wcslen(s);
	cp = winx_heap_alloc((length + 1) * sizeof(short));
	if(cp) wcscpy(cp,s);
	return cp;
}

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
 * @brief Case insensitive version of wcsstr.
 */
wchar_t * __cdecl winx_wcsistr(const wchar_t * wcs1,const wchar_t * wcs2)
{
	wchar_t *cp = (wchar_t *)wcs1;
	wchar_t *s1, *s2;

	if(wcs1 == NULL || wcs2 == NULL) return NULL;
	
	while(*cp){
		s1 = cp;
		s2 = (wchar_t *)wcs2;
		
		while(*s1 && *s2 && !( towlower((wint_t)(*s1)) - towlower((wint_t)(*s2)) )){ s1++, s2++; }
		if(!*s2) return cp;
		cp++;
	}
	
	return NULL;
}

/**
 * @brief Robust and flexible alternative to _vsnprintf.
 * @param[in] format the format specification.
 * @param[in] arg pointer to list of arguments.
 * @return Pointer to the formatted string, NULL
 * indicates failure. The string must be deallocated
 * by winx_heap_free after its use.
 */
char * __cdecl winx_vsprintf(const char *format,va_list arg)
{
	char *buffer;
	int size;
	int result;
	
	if(format == NULL)
		return NULL;
	
	/* set an initial buffer size */
	size = WINX_VSPRINTF_BUFFER_SIZE;
	do {
		buffer = winx_heap_alloc(size);
		if(buffer == NULL)
			return NULL;
		memset(buffer,0,size); /* needed for _vsnprintf */
		result = _vsnprintf(buffer,size,format,arg);
		if(result != -1 && result != size)
			return buffer;
		/* buffer is too small; try to allocate two times larger */
		winx_heap_free(buffer);
		size <<= 1;
		if(size <= 0)
			return NULL;
	} while(1);
	
	return NULL;
}

/**
 * @brief Robust and flexible alternative to _snprintf.
 * @param[in] format the format specification.
 * @param[in] ... the arguments.
 * @return Pointer to the formatted string, NULL
 * indicates failure. The string must be deallocated
 * by winx_heap_free after its use.
 */
char * __cdecl winx_sprintf(const char *format, ...)
{
	va_list arg;
	
	if(format){
		va_start(arg,format);
		return winx_vsprintf(format,arg);
	}
	
	return NULL;
}

/*
* Lightweight alternative for regular expressions.
*/

/**
 * @brief Compiles string of patterns
 * to internal representation.
 * @param[out] patterns pointer to storage
 * for a single winx_patlist structure.
 * @param[in] string the string of patterns.
 * @param[in] delim the list of delimiters
 * to be used to split string to individual patterns.
 * @param[in] flags combination of WINX_PAT_xxx flags.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall winx_patcomp(winx_patlist *patterns,short *string,short *delim,int flags)
{
	int pattern_detected;
	int i, j, n;
	short *s;
	
	if(patterns == NULL || string == NULL || delim == NULL)
		return (-1);
	
	/* reset patterns structure */
	patterns->flags = flags;
	patterns->count = 0;
	patterns->array = NULL;
	patterns->string = NULL;
	
	if(string[0] == 0)
		return 0; /* empty list of patterns */
	
	/* make a copy of the string */
	s = winx_wcsdup(string);
	if(s == NULL){
		DebugPrint("winx_patcomp: cannot allocate %u bytes of memory",
			(wcslen(string) + 1) * sizeof(short));
		return (-1);
	}
	
	/* TODO: test for speed a case when s is converted to lowercase */
	
	/* replace all delimiters by zeros */
	for(n = 0; s[n]; n++){
		if(wcschr(delim,s[n]))
			s[n] = 0;
	}
	
	/* count all patterns */
	pattern_detected = 0;
	for(i = 0; i < n; i++){
		if(s[i] != 0){
			if(!pattern_detected){
				patterns->count ++;
				pattern_detected = 1;
			}
		} else {
			pattern_detected = 0;
		}
	}
	
	/* build array of patterns */
	patterns->array = winx_heap_alloc(patterns->count * sizeof(short *));
	if(patterns->array == NULL){
		DebugPrint("winx_patcomp: cannot allocate %u bytes of memory",
			patterns->count * sizeof(short *));
		winx_heap_free(s);
		patterns->count = 0;
		return (-1);
	}
	pattern_detected = 0;
	for(i = j = 0; i < n; i++){
		if(s[i] != 0){
			if(!pattern_detected){
				patterns->array[j] = s + i;
				j ++;
				pattern_detected = 1;
			}
		} else {
			pattern_detected = 0;
		}
	}

	patterns->string = s;
	return 0;
}

/**
 * @brief Searches for patterns in the string.
 * @param[in] string the string to search in.
 * @param[in] patterns the list of patterns
 * to be searched for.
 * @return Nonzero value indicates
 * that at least one pattern has been found.
 */
int __stdcall winx_patfind(short *string,winx_patlist *patterns)
{
	int i;
	wchar_t *result;
	
	if(patterns == NULL || string == NULL)
		return 0;
	
	for(i = 0; i < patterns->count; i++){
		if(patterns->flags & WINX_PAT_ICASE)
			result = winx_wcsistr(string,patterns->array[i]);
		else
			result = wcsstr(string,patterns->array[i]);
		if(result)
			return 1; /* pattern found */
	}
	/* no patterns found */
	return 0;
}

/**
 * @brief Frees resources allocated by winx_patcomp.
 * @param[in] patterns the list of patterns.
 */
void __stdcall winx_patfree(winx_patlist *patterns)
{
	if(patterns == NULL)
		return;
	
	/* free allocated memory */
	if(patterns->string)
		winx_heap_free(patterns->string);
	if(patterns->array)
		winx_heap_free(patterns->array);
	
	/* reset all fields of the structure */
	patterns->flags = 0;
	patterns->count = 0;
	patterns->array = NULL;
	patterns->string = NULL;
}

/** @} */
