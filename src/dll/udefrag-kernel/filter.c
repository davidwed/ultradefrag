/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* User mode driver - filter related routines.
*/

#include "globals.h"

FILTER in_filter = {NULL,NULL};
FILTER ex_filter = {NULL,NULL};
ULONGLONG sizelimit = 0;
ULONGLONG fraglimit = 0;

void SetFilter(PFILTER pf,short *buffer);

void InitializeFilter(void)
{
	char buf[64];
	short *env_buffer;
	#define ENV_BUFFER_SIZE 8192
	
	/* allocate memory */
	env_buffer = winx_virtual_alloc(ENV_BUFFER_SIZE * sizeof(short));
	if(!env_buffer){
		winx_dbg_print("Cannot allocate %u bytes for InitializeOptions()\n",
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
		_snprintf(buf,sizeof(buf) - 1,"%ws",env_buffer);
		buf[sizeof(buf) - 1] = 0;
		winx_dfbsize(buf,&sizelimit);
	}

	if(winx_query_env_variable(L"UD_FRAGMENTS_THRESHOLD",env_buffer,ENV_BUFFER_SIZE) >= 0)
		fraglimit = (ULONGLONG)_wtol(env_buffer);

	/* print filtering options */
	DebugPrint("Sizelimit = %I64u\n",sizelimit);
	DebugPrint("Fragments threshold = %I64u\n",fraglimit);
	
	/* free memory */
	winx_virtual_free(env_buffer,ENV_BUFFER_SIZE * sizeof(short));
}

void SetFilter(PFILTER pf,short *buffer)
{
	POFFSET poffset;
	int i;
	char ch;
	int length;
	
	if(pf->buffer){
		winx_heap_free((void *)pf->buffer);
		DestroyList((PLIST *)pf->offsets);
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
	_wcslwr(buffer);

	/* replace double quotes and semicolons with zeros */
	poffset = (POFFSET)InsertItem((PLIST *)&pf->offsets,NULL,sizeof(OFFSET));
	if(!poffset) return;
	if(length > 1 && buffer[0] == 0x0022) poffset->offset = 1; /* skip leading double quote */
	else poffset->offset = 0;

	for(i = 0; i < length - 1; i++){
		if(buffer[i] == 0x0022) { buffer[i] = 0; continue; } /* replace all double quotes with zeros */
		if(buffer[i] == 0x003b){
			buffer[i] = 0;
			poffset = (POFFSET)InsertItem((PLIST *)&pf->offsets,NULL,sizeof(OFFSET));
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
	DestroyList((PLIST *)&in_filter.offsets);
	DestroyList((PLIST *)&ex_filter.offsets);
}
