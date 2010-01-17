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
 * @file list.c
 * @brief Generic double linked lists code.
 * @addtogroup Lists
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

/**
 * @brief Inserts an item to double linked list.
 * @details Allocates memory for an item to be inserted.
 * @param[in,out] phead pointer to a variable pointing
 *                      to the list head.
 * @param[in]     prev  pointer to an item which must
 *                      preceed to the new item.
 *                      If this parameter is NULL, the new
 *                      head will be inserted.
 * @param[in]     size  the size of an item to be inserted.
 * @return Pointer to the inserted list item. NULL indicates
 *         failure.
 */
list_entry * __stdcall winx_list_insert_item(list_entry **phead,list_entry *prev,long size)
{
	list_entry *new_item = (list_entry *)winx_heap_alloc(size);
	if(new_item == NULL) return NULL;

	/* is list empty? */
	if(*phead == NULL){
		*phead = new_item;
		new_item->p = new_item->n = new_item;
		return new_item;
	}

	/* insert as a new head? */
	if(prev == NULL){
		prev = (*phead)->p;
		*phead = new_item;
	}

	/* insert after an item specified by prev argument */
	new_item->p = prev;
	new_item->n = prev->n;
	new_item->p->n = new_item;
	new_item->n->p = new_item;
	return new_item;
}

/**
 * @brief Removes an item from double linked list.
 * @details Frees memory allocated for an item to be removed.
 * @param[in,out] phead pointer to a variable pointing
 *                      to the list head.
 * @param[in]     prev  pointer to an item which must
 *                      be removed.
 */
void __stdcall winx_list_remove_item(list_entry **phead,list_entry *item)
{
	/* validate an item */
	if(item == NULL) return;
	
	/* is list empty? */
	if(*phead == NULL) return;

	/* remove alone first item? */
	if(item == *phead && item->n == *phead){
		winx_heap_free(item);
		*phead = NULL;
		return;
	}
	
	/* remove first item? */
	if(item == *phead){
		*phead = (*phead)->n;
	}
	item->p->n = item->n;
	item->n->p = item->p;
	winx_heap_free(item);
}

/**
 * @brief Destroys a double linked list.
 * @details Frees memory allocated for all list items.
 * @param[in,out] phead pointer to a variable pointing
 *                      to the list head.
 */
void __stdcall winx_list_destroy(list_entry **phead)
{
	list_entry *item, *next, *head;
	
	/* is list empty? */
	if(*phead == NULL) return;

	head = *phead;
	item = head;

	do {
		next = item->n;
		winx_heap_free(item);
		item = next;
	} while (next != head);

	*phead = NULL;
}

/** @} */
