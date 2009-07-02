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
* zenwinx.dll keyboard input procedures.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>

#include "ntndk.h"
#include "zenwinx.h"

HANDLE hKbDevice = NULL;
HANDLE hKbEvent = NULL;

void __stdcall kb_close(void);
int  __stdcall kb_check(void);

int __stdcall kb_open(short *kb_device_name)
{
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	if(hKbDevice) goto kb_open_success;

	/* try to open specified device */
	RtlInitUnicodeString(&uStr,kb_device_name);
	InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
	Status = NtCreateFile(&hKbDevice,
				GENERIC_READ | FILE_RESERVE_OPFILTER | FILE_READ_ATTRIBUTES/*0x80100080*/,
			    &ObjectAttributes,&IoStatusBlock,NULL,FILE_ATTRIBUTE_NORMAL/*0x80*/,
			    0,FILE_OPEN/*1*/,FILE_DIRECTORY_FILE/*1*/,NULL,0);
	if(!NT_SUCCESS(Status)){
		winx_raise_error("W: Can't open the keyboard %ws: %x!",kb_device_name,(UINT)Status);
		hKbDevice = NULL;
		return (-1);
	}
	
	/* ensure that we have opened a really connected keyboard */
	if(kb_check() < 0){
		winx_raise_error("W: Invalid keyboard device %ws: %x!",kb_device_name,(UINT)Status);
		NtCloseSafe(hKbDevice);
		return (-1);
	}
	
	/* create a special event object for internal use */
	RtlInitUnicodeString(&uStr,L"\\kb_event");
	InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
	Status = NtCreateEvent(&hKbEvent,STANDARD_RIGHTS_ALL | 0x1ff/*0x1f01ff*/,
		&ObjectAttributes,SynchronizationEvent,FALSE);
	if(!NT_SUCCESS(Status)){
		hKbEvent = NULL;
		kb_close();
		winx_raise_error("E: Can't create kb_event: %x!",(UINT)Status);
		return (-1);
	}
kb_open_success:
	return 0;
}

#define LIGHTING_REPEAT_COUNT 0x5

/* 0 for success, -1 otherwise */
int __stdcall kb_light_up_indicators(USHORT LedFlags)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK iosb;
	KEYBOARD_INDICATOR_PARAMETERS kip;

	kip.LedFlags = LedFlags;
	kip.UnitId = 0;
	
	Status = NtDeviceIoControlFile(hKbDevice,NULL,NULL,NULL,
			&iosb,IOCTL_KEYBOARD_SET_INDICATORS,
			&kip,sizeof(KEYBOARD_INDICATOR_PARAMETERS),NULL,0);
	if(Status == STATUS_PENDING){
		Status = NtWaitForSingleObject(hKbDevice,FALSE,NULL);
		if(NT_SUCCESS(Status)) Status = iosb.Status;
	}
	if(!NT_SUCCESS(Status) || Status == STATUS_PENDING)	return (-1);
	
	return 0;
}

/* internal function; returns -1 for invalid device, 0 otherwise */
int __stdcall kb_check(void)
{
	USHORT LedFlags;
	NTSTATUS Status;
	IO_STATUS_BLOCK iosb;
	KEYBOARD_INDICATOR_PARAMETERS kip;
	int i;
	
	/* try to get LED flags */
	Status = NtDeviceIoControlFile(hKbDevice,NULL,NULL,NULL,
			&iosb,IOCTL_KEYBOARD_QUERY_INDICATORS,NULL,0,
			&kip,sizeof(KEYBOARD_INDICATOR_PARAMETERS));
	if(Status == STATUS_PENDING){
		Status = NtWaitForSingleObject(hKbDevice,FALSE,NULL);
		if(NT_SUCCESS(Status)) Status = iosb.Status;
	}
	if(!NT_SUCCESS(Status) || Status == STATUS_PENDING)	return (-1);

	LedFlags = kip.LedFlags;
	
	/* light up LED's */
	for(i = 0; i < LIGHTING_REPEAT_COUNT; i++){
		kb_light_up_indicators(KEYBOARD_NUM_LOCK_ON);
		winx_sleep(100);
		kb_light_up_indicators(KEYBOARD_CAPS_LOCK_ON);
		winx_sleep(100);
		kb_light_up_indicators(KEYBOARD_SCROLL_LOCK_ON);
		winx_sleep(100);
	}

	kb_light_up_indicators(LedFlags);
	return 0;
}

void __stdcall kb_close(void)
{
	NtCloseSafe(hKbDevice);
	NtCloseSafe(hKbEvent);
}
