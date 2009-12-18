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

#include "ntndk.h"
#include "zenwinx.h"

#define MAX_NUM_OF_KEYBOARDS 100

typedef struct _KEYBOARD {
	int device_number; /* for debugging purposes */
	HANDLE hKbDevice;
	HANDLE hKbEvent;
} KEYBOARD, *PKEYBOARD;

KEYBOARD kb[MAX_NUM_OF_KEYBOARDS];
int number_of_keyboards;

/* prototypes */
void __stdcall kb_close(void);
int  __stdcall kb_check(HANDLE hKbDevice);
int __stdcall kb_open_internal(int device_number);
int __stdcall kb_read_internal(int kb_index,PKEYBOARD_INPUT_DATA pKID,PLARGE_INTEGER pInterval);

/* opens all existing keyboards */
int __stdcall kb_open(short *kb_device_name)
{
	int i;

	(void)kb_device_name;
	
	/* initialize kb array */
	memset((void *)kb,0,sizeof(kb));
	number_of_keyboards = 0;
	
	/* required for USB devices? */
	winx_printf("Wait 10 seconds for keyboard initialization ");
	for(i = 0; i < 10; i++){
		winx_sleep(1000);
		winx_printf(".");
	}
	winx_printf("\n\n");
	
	for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++)
		(void)kb_open_internal(i);
	
	if(kb[0].hKbDevice) return 0; /* success, at least one keyboard found */
	else return (-1);
}

/* closes all opened keyboards */
void __stdcall kb_close(void)
{
	int i;
	
	for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++){
		if(kb[i].hKbDevice == NULL) break;
		NtCloseSafe(kb[i].hKbDevice);
		NtCloseSafe(kb[i].hKbEvent);
		/* don't reset device_number member here */
		number_of_keyboards --;
	}
}

#define MAX_LATENCY 100 /* msec */

/*
* Tries to read keyboards until msec_timeout expires.
* Returns zero when some key was pressed, -1 otherwise.
*/
int __stdcall kb_read(PKEYBOARD_INPUT_DATA pKID,int msec_timeout)
{
	int delay;
	int attempts = 0;
	LARGE_INTEGER interval;
	int i,j;
	
	if(number_of_keyboards == 0) return (-1);
	
	delay = (MAX_LATENCY / number_of_keyboards) + 1;
	if(msec_timeout != INFINITE)
		attempts = (msec_timeout / MAX_LATENCY) + 1;
	interval.QuadPart = -((signed long)delay * 10000);
	
	if(msec_timeout != INFINITE){
		for(i = 0; i < attempts; i++){
			/* loop through all keyboards */
			for(j = 0; j < number_of_keyboards; j++){
				if(kb_read_internal(j,pKID,&interval) >= 0)
					return 0;
			}
			if(number_of_keyboards == 0) return (-1);
		}
	} else {
		while(1){
			/* loop through all keyboards */
			for(j = 0; j < number_of_keyboards; j++){
				if(kb_read_internal(j,pKID,&interval) >= 0)
					return 0;
			}
			if(number_of_keyboards == 0) return (-1);
		}
	}
	return (-1);
}

/*
**************************************************************
*                   internal functions                       *
**************************************************************
*/

/* tries to open specified keyboard */
int __stdcall kb_open_internal(int device_number)
{
	short device_name[32];
	short event_name[32];
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	HANDLE hKbDevice = NULL;
	HANDLE hKbEvent = NULL;
	int i;

	_snwprintf(device_name,32,L"\\Device\\KeyboardClass%u",device_number);
	RtlInitUnicodeString(&uStr,device_name);
	InitializeObjectAttributes(&ObjectAttributes,&uStr,OBJ_CASE_INSENSITIVE,NULL,NULL);
	Status = NtCreateFile(&hKbDevice,
				GENERIC_READ | FILE_RESERVE_OPFILTER | FILE_READ_ATTRIBUTES/*0x80100080*/,
			    &ObjectAttributes,&IoStatusBlock,NULL,FILE_ATTRIBUTE_NORMAL/*0x80*/,
			    0,FILE_OPEN/*1*/,FILE_DIRECTORY_FILE/*1*/,NULL,0);
	if(!NT_SUCCESS(Status)){
		if(device_number < 2)
			winx_raise_error("W: Can't open the keyboard %ws: %x!",
				device_name,(UINT)Status);
		return (-1);
	}
	
	/* ensure that we have opened a really connected keyboard */
	if(kb_check(hKbDevice) < 0){
		winx_raise_error("W: Invalid keyboard device %ws: %x!",device_name,(UINT)Status);
		NtCloseSafe(hKbDevice);
		return (-1);
	}
	
	/* create a special event object for internal use */
	_snwprintf(event_name,32,L"\\kb_event%u",device_number);
	RtlInitUnicodeString(&uStr,event_name);
	InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
	Status = NtCreateEvent(&hKbEvent,STANDARD_RIGHTS_ALL | 0x1ff/*0x1f01ff*/,
		&ObjectAttributes,SynchronizationEvent,FALSE);
	if(!NT_SUCCESS(Status)){
		NtCloseSafe(hKbDevice);
		winx_raise_error("E: Can't create kb_event%u: %x!",device_number,(UINT)Status);
		return (-1);
	}
	
	/* add information to kb array */
	for(i = 0; i < MAX_NUM_OF_KEYBOARDS; i++){
		if(kb[i].hKbDevice == NULL){
			kb[i].hKbDevice = hKbDevice;
			kb[i].hKbEvent = hKbEvent;
			kb[i].device_number = device_number;
			number_of_keyboards ++;
			winx_printf("Keyboard device found: %ws.\n",device_name);
			return 0;
		}
	}

	winx_raise_error("E: kb array is full!");
	return (-1);
}

#define LIGHTING_REPEAT_COUNT 0x5

/* 0 for success, -1 otherwise */
int __stdcall kb_light_up_indicators(HANDLE hKbDevice,USHORT LedFlags)
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
int __stdcall kb_check(HANDLE hKbDevice)
{
	USHORT LedFlags;
	NTSTATUS Status;
	IO_STATUS_BLOCK iosb;
	KEYBOARD_INDICATOR_PARAMETERS kip;
	int i;
	
	/* try to get LED flags */
	RtlZeroMemory(&kip,sizeof(KEYBOARD_INDICATOR_PARAMETERS));
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
		kb_light_up_indicators(hKbDevice,KEYBOARD_NUM_LOCK_ON);
		winx_sleep(100);
		kb_light_up_indicators(hKbDevice,KEYBOARD_CAPS_LOCK_ON);
		winx_sleep(100);
		kb_light_up_indicators(hKbDevice,KEYBOARD_SCROLL_LOCK_ON);
		winx_sleep(100);
	}

	kb_light_up_indicators(hKbDevice,LedFlags);
	return 0;
}

int __stdcall kb_read_internal(int kb_index,PKEYBOARD_INPUT_DATA pKID,PLARGE_INTEGER pInterval)
{
	LARGE_INTEGER ByteOffset;
	IO_STATUS_BLOCK iosb;
	NTSTATUS Status;

	if(kb[kb_index].hKbDevice == NULL) return (-1);
	if(kb[kb_index].hKbEvent == NULL) return (-1);

	ByteOffset.QuadPart = 0;
	Status = NtReadFile(kb[kb_index].hKbDevice,kb[kb_index].hKbEvent,NULL,NULL,
		&iosb,pKID,sizeof(KEYBOARD_INPUT_DATA),&ByteOffset,0);
	/* wait in case operation is pending */
	if(Status == STATUS_PENDING){
		Status = NtWaitForSingleObject(kb[kb_index].hKbEvent,FALSE,pInterval);
		if(Status == STATUS_TIMEOUT){ 
			/* 
			* If we have timeout we should cancel read operation
			* to empty keyboard pending operations queue.
			*/
			Status = NtCancelIoFile(kb[kb_index].hKbDevice,&iosb);
			if(!NT_SUCCESS(Status)){
				/*
				* This is a hard error because the next read request
				* may destroy stack where pKID pointed data may be allocated.
				*/
				winx_raise_error("E: NtCancelIoFile for KeyboadClass%u failed: %x!",
					kb[kb_index].device_number,(UINT)Status);
				/* Terminate the program! */
				winx_exit(1000);
				return (-1);
			}
			/* Timeout - this is a normal situation. */
			return (-1);
		}
		if(NT_SUCCESS(Status)) Status = iosb.Status;
	}
	if(!NT_SUCCESS(Status)){
		winx_raise_error("E: Cannot read the KeyboadClass%u device: %x!",
			kb[kb_index].device_number,(UINT)Status);
		return (-1);
	}
	return 0;
}
