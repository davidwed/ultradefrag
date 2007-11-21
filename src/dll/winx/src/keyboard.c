/*
 *  WINX - WIndows Native eXtended library.
 *  Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 *  winx.dll keyboard input procedures.
 */

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>

#include "ntndk.h"
#include "winx.h"

HANDLE hKbDevice = NULL;
HANDLE hKbEvent = NULL;

void __stdcall kb_close(void);

WINX_STATUS __stdcall kb_open(short *kb_device_name)
{
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	if(hKbDevice) goto kb_open_success;
	RtlInitUnicodeString(&uStr,kb_device_name);
	InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
	Status = NtCreateFile(&hKbDevice,
				GENERIC_READ | FILE_RESERVE_OPFILTER | FILE_READ_ATTRIBUTES/*0x80100080*/,
			    &ObjectAttributes,&IoStatusBlock,NULL,FILE_ATTRIBUTE_NORMAL/*0x80*/,
			    0,FILE_OPEN/*1*/,FILE_DIRECTORY_FILE/*1*/,NULL,0);
	if(!NT_SUCCESS(Status))
		return MAKE_WINX_STATUS(WINX_KEYBOARD_INACCESSIBLE,Status);
	RtlInitUnicodeString(&uStr,L"\\kb_event");
	InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
	Status = NtCreateEvent(&hKbEvent,STANDARD_RIGHTS_ALL | 0x1ff/*0x1f01ff*/,
		&ObjectAttributes,SynchronizationEvent,FALSE);
	if(!NT_SUCCESS(Status))
	{
		kb_close();
		return MAKE_WINX_STATUS(WINX_CREATE_KB_EVENT_ERROR,Status);
	}
kb_open_success:
	return MAKE_WINX_STATUS(0,0);
}

void __stdcall kb_close(void)
{
	SafeNtClose(hKbDevice);
	SafeNtClose(hKbEvent);
}
