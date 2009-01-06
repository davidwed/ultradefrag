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
* zenwinx.dll functions for Events manipulation.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>

#include "ntndk.h"
#include "zenwinx.h"

/****f* zenwinx.event/winx_create_event
* NAME
*    winx_create_event
* SYNOPSIS
*    error = winx_create_event(name, type, phandle);
* FUNCTION
*    Creates the specified event.
* INPUTS
*    name    - event name
*    type    - SynchronizationEvent or NotificationEvent
*    phandle - pointer to event handle
* RESULT
*    If the function succeeds, the return value is zero.
*    Otherwise - negative value and phandle == NULL.
* EXAMPLE
*    winx_create_event(L"\\xyz_event",SynchronizationEvent,&hEvent);
* SEE ALSO
*    winx_destroy_event
******/
int __stdcall winx_create_event(short *name,int type,HANDLE *phandle)
{
	UNICODE_STRING us;
	NTSTATUS Status;
	OBJECT_ATTRIBUTES oa;

	if(!phandle){
		winx_raise_error("E: winx_create_event() invalid phandle!");
		return (-1);
	}
	*phandle = NULL;

	if(!name){
		winx_raise_error("E: winx_create_event() invalid name!");
		return (-1);
	}
	if(type != SynchronizationEvent && type != NotificationEvent){
		winx_raise_error("E: winx_create_event() invalid type!");
		return (-1);
	}

	RtlInitUnicodeString(&us,name);
	InitializeObjectAttributes(&oa,&us,0,NULL,NULL);
	/* (BOOLEAN)type as fourth parameter fails on MSVC!!! */
	if(type == SynchronizationEvent)
		Status = NtCreateEvent(phandle,STANDARD_RIGHTS_ALL | 0x1ff,
			&oa,SynchronizationEvent,TRUE);
	else
		Status = NtCreateEvent(phandle,STANDARD_RIGHTS_ALL | 0x1ff,
			&oa,NotificationEvent,TRUE);
	if(!NT_SUCCESS(Status)){
		winx_raise_error("E: Can't create %ws: %x!",name,(UINT)Status);
		*phandle = NULL;
		return (-1);
	}
	return 0;
}

/****f* zenwinx.event/winx_destroy_event
* NAME
*    winx_destroy_event
* SYNOPSIS
*    winx_destroy_event(handle);
* FUNCTION
*    Destroys the specified event.
* INPUTS
*    handle - event handle
* RESULT
*    Nothing.
* EXAMPLE
*    winx_destroy_event(hEvent); hEvent = NULL;
* SEE ALSO
*    winx_create_event
******/
void __stdcall winx_destroy_event(HANDLE h)
{
	if(h) NtClose(h);
}
