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
 *  Native functions to read keyboard input and display strings.
 */

#include "defrag_native.h"
#include "../include/ultradfgver.h"

HANDLE hStdInput = NULL;
HANDLE hKbEvent = NULL;
char kb_buffer[256];

void IntSleep(DWORD dwMilliseconds);

NTSTATUS kb_read(PKEYBOARD_INPUT_DATA InputData);

/* Display characters and strings */
int __cdecl putch(int ch)
{
	short str[2];
	UNICODE_STRING strU;

	str[0] = (short)ch; str[1] = 0;
	RtlInitUnicodeString(&strU,str);
	NtDisplayString(&strU);
	return 0;
}

int __cdecl print(const char *str)
{
	int i,len;
	ANSI_STRING as;
	UNICODE_STRING us;

	RtlInitAnsiString(&as,str);
	if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) == STATUS_SUCCESS)
	{
		NtDisplayString(&us);
		RtlFreeUnicodeString(&us);
	}
	else
	{
		len = strlen(str);
		for(i = 0; i < len; i ++)
			putch(str[i]);
	}
	return 0;
}

/* Read keyboard input */
BOOL kb_open()
{
	UNICODE_STRING us;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	RtlInitUnicodeString(&us,L"\\Device\\KeyboardClass0");
	InitializeObjectAttributes(&ObjectAttributes,&us,0,NULL,NULL);
	Status = NtCreateFile(&hStdInput,
				GENERIC_READ | FILE_RESERVE_OPFILTER | FILE_READ_ATTRIBUTES/*0x80100080*/,
			    &ObjectAttributes,&IoStatusBlock,NULL,FILE_ATTRIBUTE_NORMAL/*0x80*/,
			    0,FILE_OPEN/*1*/,FILE_DIRECTORY_FILE/*1*/,NULL,0);
	if(Status)
	{
		print("\nERROR: No keyboard available!");
		goto fail;
	}
	RtlInitUnicodeString(&us,L"\\KbEvent");
	InitializeObjectAttributes(&ObjectAttributes,&us,0,NULL,NULL);
	Status = NtCreateEvent(&hKbEvent,STANDARD_RIGHTS_ALL | 0x1ff/*0x1f01ff*/,
		&ObjectAttributes,SynchronizationEvent,FALSE);
	if(Status == STATUS_SUCCESS)
		return TRUE;
	print("\nERROR: Can't create \\KbEvent!");
fail:
	print(" Wait 5 seconds.\n");
	IntSleep(5000);
	return FALSE;
}

BOOL kb_scan()
{
	KEYBOARD_INPUT_DATA kbd;
	NTSTATUS Status;
	KBD_RECORD kbd_rec;
	int index = 0;
	char ch;

	while(index < sizeof(kb_buffer) - 1)
	{
		Status = kb_read(&kbd);
		if(Status == STATUS_SUCCESS)
		{
			IntTranslateKey(&kbd,&kbd_rec);
			if(!kbd_rec.bKeyDown) continue;
			ch = kbd_rec.AsciiChar;
			if(!ch) continue; /* for control keys */
			if(ch == 13) /* 'enter' key */
			{
				print("\r\n");
				break;
			}
			putch(ch);
			kb_buffer[index] = ch;
			index ++;
		}
		else return FALSE;
	}
	kb_buffer[index] = 0;
	return TRUE;
}

NTSTATUS kb_read(PKEYBOARD_INPUT_DATA InputData)
{
	IO_STATUS_BLOCK Iosb;
	NTSTATUS Status;
	LARGE_INTEGER ByteOffset;

	ByteOffset.QuadPart = 0;
	Status = NtReadFile(hStdInput,hKbEvent,NULL,NULL,
			&Iosb,InputData,sizeof(KEYBOARD_INPUT_DATA),&ByteOffset,0);
	/* wait in case operation is pending */
	if(Status == STATUS_PENDING)
	{
		Status = NtWaitForSingleObject(hKbEvent,FALSE,NULL);
		if(!Status) Status = Iosb.Status;
	}
	return Status;
}

NTSTATUS WaitKbHit(DWORD msec,KEYBOARD_INPUT_DATA *pkbd)
{
	IO_STATUS_BLOCK Iosb;
	NTSTATUS Status;
	LARGE_INTEGER ByteOffset;
	LARGE_INTEGER interval;

	ByteOffset.QuadPart = 0;
	if (msec != INFINITE)
		interval.QuadPart = -((signed long)msec * 10000);
	else
		interval.QuadPart = -0x7FFFFFFFFFFFFFFF;
	Status = NtReadFile(hStdInput,hKbEvent,NULL,NULL,
			&Iosb,pkbd,sizeof(KEYBOARD_INPUT_DATA),&ByteOffset,0);
	/* wait in case operation is pending */
	if(Status == STATUS_PENDING)
	{
		Status = NtWaitForSingleObject(hKbEvent,FALSE,&interval);
		if(!Status) Status = Iosb.Status;
	}
	return Status;
}

void kb_close()
{
	if(hStdInput) NtClose(hStdInput);
	if(hKbEvent) NtClose(hKbEvent);
}
