/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
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
 * @file filter.c
 * @brief File names filtering code.
 * @addtogroup Filter
 * @{
 */

#include "globals.h"

FILTER in_filter = {NULL,NULL};
FILTER ex_filter = {NULL,NULL};
ULONGLONG sizelimit = 0;
ULONGLONG fraglimit = 0;

void SetFilter(PFILTER pf,short *buffer);

/**
 * @brief Initializes all the filters.
 */
void InitializeFilter(void)
{
	char buf[64];
	short *env_buffer;
	#define ENV_BUFFER_SIZE 8192
	
	/* allocate memory */
	env_buffer = winx_virtual_alloc(ENV_BUFFER_SIZE * sizeof(short));
	if(!env_buffer){
		DebugPrint("Cannot allocate %u bytes for InitializeOptions()\n",
			ENV_BUFFER_SIZE * sizeof(short));
		return;
	}

	/* reset filter */
	sizelimit = 0; fraglimit = 0;
	DestroyFilter();
	
	/* read program's filter options from environment variables */
	if(winx_query_env_variable(L"UD_IN_FILTER",env_buffer,ENV_BUFFER_SIZE) >= 0){
		DebugPrint("Include: %ws\n",env_buffer);
		SetFilter(&in_filter,env_buffer);
	} else {
		DebugPrint("Include: %ws\n",L"");
	}
	context_menu_handler = CheckForContextMenuHandler();
	if(context_menu_handler) DebugPrint("Context menu handler?\n");
	
	if(winx_query_env_variable(L"UD_EX_FILTER",env_buffer,ENV_BUFFER_SIZE) >= 0){
		DebugPrint("Exclude: %ws\n",env_buffer);
		SetFilter(&ex_filter,env_buffer);
	} else {
		DebugPrint("Exclude: %ws\n",L"");
	}

	if(winx_query_env_variable(L"UD_SIZELIMIT",env_buffer,ENV_BUFFER_SIZE) >= 0){
		(void)_snprintf(buf,sizeof(buf) - 1,"%ws",env_buffer);
		buf[sizeof(buf) - 1] = 0;
		(void)winx_dfbsize(buf,&sizelimit);
	}

	if(winx_query_env_variable(L"UD_FRAGMENTS_THRESHOLD",env_buffer,ENV_BUFFER_SIZE) >= 0)
		fraglimit = (ULONGLONG)_wtol(env_buffer);

	/* print filtering options */
	DebugPrint("Sizelimit = %I64u\n",sizelimit);
	DebugPrint("Fragments threshold = %I64u\n",fraglimit);
	
	/* free memory */
	winx_virtual_free(env_buffer,ENV_BUFFER_SIZE * sizeof(short));
}

/**
 * @brief Initializes the including/excluding filter.
 * @param[in] pf pointer to the head of the list 
 * representing the appropriate filter.
 * @param[in] buffer the null-terminated filter string.
 */
void SetFilter(PFILTER pf,short *buffer)
{
	POFFSET poffset;
	int i;
	char ch;
	int length;
	
	if(pf->buffer){
		winx_heap_free((void *)pf->buffer);
		winx_list_destroy((list_entry **)pf->offsets);
	}
	pf->buffer = NULL;
	pf->offsets = NULL;
	
	length = wcslen(buffer) + 1;
	if(length <= 1) return;

	pf->buffer = winx_heap_alloc(length * sizeof(short));
	if(!pf->buffer){
		DebugPrint("Cannot allocate memory for pf->buffer in SetFilter()!\n");
		return;
	}

	buffer[length - 1] = 0;
	(void)_wcslwr(buffer);

	/* replace double quotes and semicolons with zeros */
	poffset = (POFFSET)winx_list_insert_item((list_entry **)&pf->offsets,NULL,sizeof(OFFSET));
	if(!poffset) return;
	if(length > 1 && buffer[0] == 0x0022) poffset->offset = 1; /* skip leading double quote */
	else poffset->offset = 0;

	for(i = 0; i < length - 1; i++){
		if(buffer[i] == 0x0022) { buffer[i] = 0; continue; } /* replace all double quotes with zeros */
		if(buffer[i] == 0x003b){
			buffer[i] = 0;
			poffset = (POFFSET)winx_list_insert_item((list_entry **)&pf->offsets,NULL,sizeof(OFFSET));
			if(!poffset) break;
			if(buffer[i + 1] == 0x0022) poffset->offset = i + 2; /* safe, because we always have null terminated buffer */
			else poffset->offset = i + 1;
		}
	}

	memcpy(pf->buffer,buffer,length * sizeof(short));

	DebugPrint("Filter strings:\n");
	ch = (pf == &in_filter) ? '+' : '-';
	for(poffset = pf->offsets; poffset != NULL; poffset = poffset->next_ptr){
		DebugPrint("  %c %ws\n",ch,pf->buffer + poffset->offset);
		if(poffset->next_ptr == pf->offsets) break;
	}
}

/**
 * @brief Checks the string for an occurrence in the filter.
 * @param[in] str the null-terminated string to search for.
 * @param[in] pf pointer to the head of the list 
 * representing the filter to search in.
 * @return Boolean value. TRUE indicates that the string
 * has been found in the filter, FALSE indicates contrary.
 */
BOOLEAN IsStringInFilter(short *str,PFILTER pf)
{
	POFFSET po;
	short *p;

	for(po = pf->offsets; po != NULL; po = po->next_ptr){
		p = pf->buffer + po->offset;
		if(p[0]){ if(wcsstr(str,p)) return TRUE; }
		if(po->next_ptr == pf->offsets) break;
	}
	return FALSE;
}

/**
 * @brief Checks for the Explorer's context menu handler.
 * @return Boolean value. TRUE indicates that the Explorer's
 * context menu handler sent the disk defragmentation request,
 * FALSE indicates contrary.
 */
BOOLEAN CheckForContextMenuHandler(void)
{
	POFFSET po;
	short *p;
	
	if(!in_filter.buffer) return FALSE;
	po = in_filter.offsets;
	if(!po) return FALSE;
	if(po->next_ptr != po) return FALSE;
	p = in_filter.buffer + po->offset;
	if(wcslen(p) < 3) return FALSE;
	if(p[1] != ':' || p[2] != '\\') return FALSE;
	return TRUE;
}

/**
 * @brief Destroys all the lists representing filters.
 */
void DestroyFilter(void)
{
	if(in_filter.buffer){
		winx_heap_free(in_filter.buffer);
		in_filter.buffer = NULL;
	}
	if(ex_filter.buffer){
		winx_heap_free(ex_filter.buffer);
		ex_filter.buffer = NULL;
	}
	winx_list_destroy((list_entry **)&in_filter.offsets);
	winx_list_destroy((list_entry **)&ex_filter.offsets);
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
 * @brief Case insensitive version of wcsstr().
 */
wchar_t * __cdecl wcsistr(const wchar_t * wcs1,const wchar_t * wcs2)
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
