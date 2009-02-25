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
* List processing functions.
*/

#include "driver.h"

/* OLD ALGORITHM */
#if 0
LIST* NTAPI InsertFirstItem(PLIST *phead,ULONG size)
{
	PLIST p;

	p = (LIST *)AllocatePool(NonPagedPool,size);
	if(p){ p->next_ptr = *phead; *phead = p; }
	else { DebugPrint2("-Ultradfg- InsertFirstItem() no enough memory!\n",NULL); }
	return p;
}

LIST* NTAPI InsertMiddleItem(PLIST *pprev,PLIST *pnext,ULONG size)
{
	PLIST p = NULL;

	if(*pprev){
		p = (LIST *)AllocatePool(NonPagedPool,size);
		if(p){ p->next_ptr = *pnext; (*pprev)->next_ptr = p; }
		else { DebugPrint2("-Ultradfg- InsertMiddleItem() no enough memory!\n",NULL); }
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
	} else {
		DebugPrint2("-Ultradfg- InsertLastItem() no enough memory!\n",NULL);
	}
	return p;
}

LIST* NTAPI RemoveItem(PLIST *phead,PLIST *pprev,PLIST *pcurrent)
{
	PLIST next;

	next = (*pcurrent)->next_ptr;
	Nt_ExFreePool(*pcurrent);
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
		Nt_ExFreePool(curr);
		curr = next;
	}
	*phead = NULL;
}
#endif

/* NEW ALGORITHM */
/*
* Returns NULL or pointer to successfully inserted structure.
* To insert item as a new head of list use InsertItem(phead,NULL,size).
*/
LIST * NTAPI InsertItem(PLIST *phead,PLIST prev,ULONG size)
{
	PLIST new_item;

	new_item = (LIST *)AllocatePool(NonPagedPool,size);
	if(!new_item){
		DebugPrint2("-Ultradfg- InsertItem() no enough memory!\n",NULL);
		return NULL;
	}

	/* is list empty? */
	if(*phead == NULL){
		*phead = new_item;
		new_item->prev_ptr = new_item->next_ptr = new_item;
		return new_item;
	}

	/* insert as a new head? */
	if(prev == NULL){
		prev = (*phead)->prev_ptr;
		*phead = new_item;
	}

	/* insert after an item specified by prev argument */
	new_item->prev_ptr = prev;
	new_item->next_ptr = prev->next_ptr;
	new_item->prev_ptr->next_ptr = new_item;
	new_item->next_ptr->prev_ptr = new_item;
	return new_item;
}

void NTAPI RemoveItem(PLIST *phead,PLIST item)
{
	/* is list empty? */
	if(*phead == NULL) return;

	/* remove alone first item? */
	if(item == *phead && item->next_ptr == *phead){
		Nt_ExFreePool(item);
		*phead = NULL;
		return;
	}
	
	/* remove first item? */
	if(item == *phead){
		*phead = (*phead)->next_ptr;
	}
	item->prev_ptr->next_ptr = item->next_ptr;
	item->next_ptr->prev_ptr = item->prev_ptr;
	Nt_ExFreePool(item);
}

void NTAPI DestroyList(PLIST *phead)
{
	PLIST item, next, head;
	
	if(*phead == NULL) return;

	head = *phead;
	item = head;

	do {
		next = item->next_ptr;
		Nt_ExFreePool(item);
		item = next;
	} while (next != head);

	*phead = NULL;
}
