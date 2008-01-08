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
 *  List processing functions.
 */

#include "driver.h"

LIST* NTAPI InsertFirstItem(PLIST *phead,ULONG size)
{
	PLIST p;

	p = (LIST *)AllocatePool(NonPagedPool,size);
	if(p){
		p->next_ptr = *phead;
		*phead = p;
	}
	#if DBG
	else if(dbg_level > 0)
		DbgPrintNoMem();
	#endif
	return p;
}

LIST* NTAPI InsertMiddleItem(PLIST *pprev,PLIST *pnext,ULONG size)
{
	PLIST p = NULL;

	if(*pprev){
		p = (LIST *)AllocatePool(NonPagedPool,size);
		if(p){
			p->next_ptr = *pnext;
			(*pprev)->next_ptr = p;
		}
		#if DBG
		else if(dbg_level > 0)
			DbgPrintNoMem();
		#endif
	}
	return p;
}

LIST* NTAPI InsertLastItem(PLIST *phead,PLIST *plast,ULONG size)
{
	PLIST p,last;

	last = *plast;
	p = (LIST *)AllocatePool(NonPagedPool,size);
	if(p){
		if(last) last->next_ptr = p;
		else     *phead = p;
		p->next_ptr = NULL;
		*plast = p;
	}
	#if DBG
	else if(dbg_level > 0)
		DbgPrintNoMem();
	#endif
	return p;
}

LIST* NTAPI RemoveItem(PLIST *phead,PLIST *pprev,PLIST *pcurrent)
{
	PLIST next;

	next = (*pcurrent)->next_ptr;
	ExFreePool(*pcurrent);
	if(*pprev)
		(*pprev)->next_ptr = next;
	else
		*phead = next;

	return next;
}

void NTAPI DestroyList(PLIST *phead)
{
	PLIST curr,next;

	curr = *phead;
	while(curr){
		next = curr->next_ptr;
		ExFreePool(curr);
		curr = next;
	}
	*phead = NULL;
}
