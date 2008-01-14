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
 *  Simple filter implementation.
 */

#include "driver.h"

/* FIXME: clusters_to_move(!) and filters(!!) changing after analysis */

BOOLEAN IsStringInFilter(short *str,PFILTER pf)
{
	POFFSET po;
	short *p;

	for(po = pf->offsets; po != NULL; po = po->next_ptr){
		p = pf->buffer + po->offset;
		if(p[0]){
			if(wcsstr(str,p)) return TRUE;
		}
	}
	return FALSE;
}

void ApplyFilter(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	UNICODE_STRING str;

	if(!RtlCreateUnicodeString(&str,pfn->name.Buffer)){
		DbgPrintNoMem();
		pfn->is_filtered = FALSE;
		return;
	}
	_wcslwr(str.Buffer);
	if(dx->in_filter.buffer)
		if(!IsStringInFilter(str.Buffer,&dx->in_filter))
			goto excl;
	if(dx->ex_filter.buffer)
		if(IsStringInFilter(str.Buffer,&dx->ex_filter))
			goto excl;
	RtlFreeUnicodeString(&str);
	pfn->is_filtered = FALSE;
	return;
excl:
	RtlFreeUnicodeString(&str);
	pfn->is_filtered = TRUE;
	return;
}

void UpdateFilter(PFILTER pf,short *buffer,int length)
{
	POFFSET poffset;
	int i;

	if(pf->buffer){
		ExFreePool((void *)pf->buffer);
		pf->buffer = NULL;
		DestroyList((PLIST *)pf->offsets);
	}

	if(length <= sizeof(short)) return;

	pf->offsets = NULL;
	pf->buffer = AllocatePool(NonPagedPool,length);
	if(pf->buffer){
		buffer[(length >> 1) - 1] = 0;
		_wcslwr(buffer);

		poffset = (POFFSET)InsertFirstItem((PLIST *)&pf->offsets,sizeof(OFFSET));
		if(!poffset) return;
		poffset->offset = 0;
		for(i = 0; i < (length >> 1) - 1; i++){
			if(buffer[i] == 0x003b){
				buffer[i] = 0;
				poffset = (POFFSET)InsertFirstItem((PLIST *)&pf->offsets,sizeof(OFFSET));
				if(!poffset) break;
				poffset->offset = i + 1;
			}
		}
		RtlCopyMemory(pf->buffer,buffer,length);
	}
}

void DestroyFilter(UDEFRAG_DEVICE_EXTENSION *dx)
{
	ExFreePoolSafe(dx->in_filter.buffer);
	ExFreePoolSafe(dx->ex_filter.buffer);
	DestroyList((PLIST *)&dx->in_filter.offsets);
	DestroyList((PLIST *)&dx->ex_filter.offsets);
}
