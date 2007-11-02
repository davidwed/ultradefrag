/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
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
 *  udefrag.dll - middle layer between driver and user interfaces:
 *  functions to work with keyboard and display in native mode.
 */

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <ntddkbd.h>
#include "../../include/ntndk.h"
#include "../../include/udefrag.h"

extern char msg[];
HANDLE hKbDevice = NULL;
HANDLE hKbEvent = NULL;

BOOL kb_read_error = FALSE;

/* internal functions prototypes */
NTSTATUS kb_read(KEYBOARD_INPUT_DATA *pkbd,int msec_timeout);
extern void IntTranslateKey(PKEYBOARD_INPUT_DATA InputData,KBD_RECORD *kbd_rec);

/* inline functions */
#define SafeNtClose(h) if(h) { NtClose(h); h = NULL; }

/*
 * Write Data to Display procedures.
 */

void __cdecl nputch(char ch)
{
	short str[2];
	UNICODE_STRING uStr;

	str[0] = (short)ch; str[1] = 0;
	RtlInitUnicodeString(&uStr,str);
	NtDisplayString(&uStr);
}

void __cdecl nprint(char *str)
{
	int i,len;
	ANSI_STRING aStr;
	UNICODE_STRING uStr;

	RtlInitAnsiString(&aStr,str);
	if(RtlAnsiStringToUnicodeString(&uStr,&aStr,TRUE) == STATUS_SUCCESS)
	{
		NtDisplayString(&uStr);
		RtlFreeUnicodeString(&uStr);
	}
	else
	{
		len = strlen(str);
		for(i = 0; i < len; i ++)
			nputch(str[i]);
	}
}

/*
 * Read Keyboard Input procedures.
 */
char * __cdecl kb_open(void)
{
	short kb_device_name[] = L"\\Device\\KeyboardClass0";
	UNICODE_STRING uStr;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	if(hKbDevice)
	{
		strcpy(msg,"Keyboard %ws is already opened!");
		goto kb_open_fail;
	}
	RtlInitUnicodeString(&uStr,kb_device_name);
	InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
	Status = NtCreateFile(&hKbDevice,
				GENERIC_READ | FILE_RESERVE_OPFILTER | FILE_READ_ATTRIBUTES/*0x80100080*/,
			    &ObjectAttributes,&IoStatusBlock,NULL,FILE_ATTRIBUTE_NORMAL/*0x80*/,
			    0,FILE_OPEN/*1*/,FILE_DIRECTORY_FILE/*1*/,NULL,0);
	if(!NT_SUCCESS(Status))
	{
		sprintf(msg,"Can't open keyboard %ws: %x!",kb_device_name,Status);
		goto kb_open_fail;
	}
	RtlInitUnicodeString(&uStr,L"\\kb_event");
	InitializeObjectAttributes(&ObjectAttributes,&uStr,0,NULL,NULL);
	Status = NtCreateEvent(&hKbEvent,STANDARD_RIGHTS_ALL | 0x1ff/*0x1f01ff*/,
		&ObjectAttributes,SynchronizationEvent,FALSE);
	if(!NT_SUCCESS(Status))
	{
		sprintf(msg,"Can't create kb_event: %x!",Status);
		goto kb_open_fail;
	}
	return NULL;
kb_open_fail:
	kb_close();
	return msg;
}

/* NOTE: if keyboard is inaccessible return value is always FALSE */
int __cdecl kb_hit(KEYBOARD_INPUT_DATA *pkbd,int msec_timeout)
{
	if(kb_read_error) return FALSE;
	return (kb_read(pkbd,msec_timeout) == STATUS_SUCCESS);
}

/* NOTE: if it return error, next kb_read request is very dangerous */
NTSTATUS kb_read(KEYBOARD_INPUT_DATA *pkbd,int msec_timeout)
{
	IO_STATUS_BLOCK Iosb;
	NTSTATUS Status;
	LARGE_INTEGER ByteOffset;
	LARGE_INTEGER interval;

	ByteOffset.QuadPart = 0;
	if(msec_timeout != INFINITE)
		interval.QuadPart = -((signed long)msec_timeout * 10000);
	else
		interval.QuadPart = -0x7FFFFFFFFFFFFFFF;
	Status = NtReadFile(hKbDevice,hKbEvent,NULL,NULL,
			&Iosb,pkbd,sizeof(KEYBOARD_INPUT_DATA),&ByteOffset,0);
	/* wait in case operation is pending */
	if(Status == STATUS_PENDING)
	{
		Status = NtWaitForSingleObject(hKbEvent,FALSE,&interval);
		if(Status == STATUS_TIMEOUT)
		{ /* if we have timeout we should cancel read operation,
		   * because after kb_read return stack data will be invalid */
			Status = NtCancelIoFile(hKbDevice,&Iosb);
			if(!NT_SUCCESS(Status))
			{ /* this is fatal error because the next read request
			   * will corrupt some program data */
				sprintf(msg,"\nCan't cancel keyboard read request: %x!\n",Status);
				nprint(msg);
				kb_read_error = TRUE;
				return Status;
			}
			return STATUS_TIMEOUT;
		}
		if(NT_SUCCESS(Status)) Status = Iosb.Status;
	}
	if(!NT_SUCCESS(Status)) kb_read_error = TRUE;
	return Status;
}

char * __cdecl kb_gets(char *buffer,int max_chars)
{
	KEYBOARD_INPUT_DATA kbd;
	KBD_RECORD kbd_rec;
	NTSTATUS Status;
	int i = 0;
	char ch;

	/* buffer should be correct in all cases */
	buffer[max_chars - 1] = 0;
	max_chars --;
	while(i < max_chars)
	{
		Status = kb_read(&kbd,INFINITE);
		if(!NT_SUCCESS(Status))
		{
			sprintf(msg,"Can't read keyboard input: %x!",Status);
			goto kb_scan_fail;
		}
		IntTranslateKey(&kbd,&kbd_rec);
		if(!kbd_rec.bKeyDown) continue; /* skip bKeyUp codes */
		ch = kbd_rec.AsciiChar;
		if(!ch) continue; /* skip control keys codes */
		if(ch == 13)
		{ /* 'Enter' key was pressed */
			nprint("\r\n");
			goto kb_gets_successful;
		}
		nputch(ch);
		buffer[i] = ch;
		i ++;
	}
	strcpy(msg,"Buffer overflow!");
kb_scan_fail:
	return msg;
kb_gets_successful:
	buffer[i] = 0;
	return NULL;
}

char * __cdecl kb_close(void)
{
	SafeNtClose(hKbDevice);
	SafeNtClose(hKbEvent);
	return NULL;
}
