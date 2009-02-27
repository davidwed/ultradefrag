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
* Simple filter implementation.
*/

#include "driver.h"

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

BOOLEAN CheckForContextMenuHandler(UDEFRAG_DEVICE_EXTENSION *dx)
{
	POFFSET po;
	short *p;
	
	if(!dx->in_filter.buffer) return FALSE;
	po = dx->in_filter.offsets;
	if(po->next_ptr != po) return FALSE;
	p = dx->in_filter.buffer + po->offset;
	if(wcslen(p) < 3) return FALSE;
	if(p[1] != ':' || p[2] != '\\') return FALSE;
	return TRUE;
}

void ApplyFilter(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFRAGMENTED pf;
	UNICODE_STRING us;

	for(pf = dx->fragmfileslist; pf != NULL; pf = pf->next_ptr){
		pf->pfn->is_filtered = TRUE;

		if(!RtlCreateUnicodeString(&us,pf->pfn->name.Buffer)){
			DebugPrint2("-Ultradfg- cannot allocate memory for ApplyFilter()!\n",NULL);
			goto L0;
		}
		_wcslwr(us.Buffer);

		if(dx->in_filter.buffer){
			if(!IsStringInFilter(us.Buffer,&dx->in_filter)){
				RtlFreeUnicodeString(&us); goto L0;
			}
		}

		if(dx->ex_filter.buffer){
			if(IsStringInFilter(us.Buffer,&dx->ex_filter)){
				RtlFreeUnicodeString(&us); goto L0;
			}
		}
		pf->pfn->is_filtered = FALSE;
		RtlFreeUnicodeString(&us);
	L0:
		if(pf->next_ptr == dx->fragmfileslist) break;
	}
}

void UpdateFilter(UDEFRAG_DEVICE_EXTENSION *dx,PFILTER pf,
				  short *buffer,int length)
{
	POFFSET poffset;
	int i;
	char ch;

	if(pf->buffer){
		Nt_ExFreePool((void *)pf->buffer);
		pf->buffer = NULL;
		DestroyList((PLIST *)pf->offsets);
	}

	if(length <= sizeof(short)) return;

	pf->offsets = NULL;
	pf->buffer = AllocatePool(NonPagedPool,length);
	if(!pf->buffer){
		DebugPrint("-Ultradfg- cannot allocate memory for pf->buffer in UpdateFilter()!\n",NULL);
		return;
	}

	buffer[(length >> 1) - 1] = 0;
	_wcslwr(buffer);

	/* replace double quotes and semicolons with zeros */
	poffset = (POFFSET)InsertItem((PLIST *)&pf->offsets,NULL,sizeof(OFFSET));
	if(!poffset) return;
	if(length > sizeof(short) && buffer[0] == 0x0022) poffset->offset = 1; /* skip leading double quote */
	else poffset->offset = 0;

	for(i = 0; i < (length >> 1) - 1; i++){
		if(buffer[i] == 0x0022) { buffer[i] = 0; continue; } /* replace all double quotes with zeros */
		if(buffer[i] == 0x003b){
			buffer[i] = 0;
			poffset = (POFFSET)InsertItem((PLIST *)&pf->offsets,NULL,sizeof(OFFSET));
			if(!poffset) break;
			if(buffer[i + 1] == 0x0022) poffset->offset = i + 2; /* safe, because we always have null terminated buffer */
			else poffset->offset = i + 1;
		}
	}

	RtlCopyMemory(pf->buffer,buffer,length);

	DebugPrint("-Ultradfg- Filter strings:\n",NULL);
	ch = (pf == &dx->in_filter) ? '+' : '-';
	for(poffset = pf->offsets; poffset != NULL; poffset = poffset->next_ptr){
		DebugPrint("-Ultradfg-  %c\n",pf->buffer + poffset->offset,ch);
		if(poffset->next_ptr == pf->offsets) break;
	}
}

void DestroyFilter(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ExFreePoolSafe(dx->in_filter.buffer);
	ExFreePoolSafe(dx->ex_filter.buffer);
	DestroyList((PLIST *)&dx->in_filter.offsets);
	DestroyList((PLIST *)&dx->ex_filter.offsets);
}
