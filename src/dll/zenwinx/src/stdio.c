/*
 *  ZenWINX - WIndows Native eXtended library.
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
 *  zenwinx.dll console i/o functions.
 */

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
///#include <ntddkbd.h>

#include "ntndk.h"
#include "zenwinx.h"

#define INTERNAL_BUFFER_SIZE 2048

extern HANDLE hKbDevice;
extern HANDLE hKbEvent;

/* global variables for winx_kbhit function */
KEYBOARD_INPUT_DATA kbd;
IO_STATUS_BLOCK iosb;
LARGE_INTEGER ByteOffset;
KBD_RECORD kbd_rec;

void IntTranslateKey(PKEYBOARD_INPUT_DATA InputData, KBD_RECORD *kbd_rec);

/* internal functions */
/****if* zenwinx.stdio/winx_print
 *  NAME
 *     winx_print
 *  SYNOPSIS
 *     winx_print(string);
 *  FUNCTION
 *     Display the ANSI string on the screen.
 ******/
void winx_print(char *string)
{
	ANSI_STRING aStr;
	UNICODE_STRING uStr;
	int i, len;

	if(!string) return;
	RtlInitAnsiString(&aStr,string);
	if(RtlAnsiStringToUnicodeString(&uStr,&aStr,TRUE) == STATUS_SUCCESS)
	{
		NtDisplayString(&uStr);
		RtlFreeUnicodeString(&uStr);
	}
	else
	{
		len = strlen(string);
		for(i = 0; i < len; i ++)
			winx_putch(string[i]);
	}

}

/* ansi functions */
/****f* zenwinx.stdio/winx_putch
 *  NAME
 *     winx_putch
 *  SYNOPSIS
 *     winx_putch(character);
 *  FUNCTION
 *     CRT putch() native equivalent.
 *  BUGS
 *     Does not recognize special characters
 *     such as 'backspace'.
 *  SEE ALSO
 *     winx_puts,winx_printf
 ******/
int __cdecl winx_putch(int ch)
{
	UNICODE_STRING uStr;
	short s[2];

	s[0] = (short)ch; s[1] = 0;
	RtlInitUnicodeString(&uStr,s);
	NtDisplayString(&uStr);
	return ch;
}

/****f* zenwinx.stdio/winx_puts
 *  NAME
 *     winx_puts
 *  SYNOPSIS
 *     winx_puts(string);
 *  FUNCTION
 *     CRT puts() native equivalent.
 *  BUGS
 *     Does not recognize special characters
 *     such as 'backspace'.
 *  SEE ALSO
 *     winx_putch,winx_printf
 ******/
int __cdecl winx_puts(const char *string)
{
	if(!string) return -1;
	return winx_printf("%s\n",string) ? 0 : -1;
}

/****f* zenwinx.stdio/winx_printf
 *  NAME
 *     winx_printf
 *  SYNOPSIS
 *     winx_printf("%s %x!",string,number);
 *  FUNCTION
 *     CRT printf() native equivalent.
 *  BUGS
 *     Does not recognize special characters
 *     such as 'backspace'.
 *  SEE ALSO
 *     winx_putch,winx_puts
 ******/
int __cdecl winx_printf(const char *format, ...)
{
	va_list arg;
	int done;
	char small_buffer[INTERNAL_BUFFER_SIZE];
	char *big_buffer = NULL;
	int size = INTERNAL_BUFFER_SIZE * 2;

	if(!format) return 0;
	/* if we have just one argument then call winx_print directly */
	va_start(arg,format);
	if(!va_arg(arg,int))
	{
		winx_print((char *)format);
		return strlen(format);
	}
	va_end(arg);
	/* prepare for _vsnprintf call */
	va_start(arg,format);
	/****if* zenwinx.internals/_vsnprintf
	 *  NAME
	 *     _vsnprintf
	 *  NOTES
	 *     Undocumented requirement: buffer must be filled
	 *     with zero characters before _vsnprintf() call!
	 ******/
	memset(small_buffer,0,INTERNAL_BUFFER_SIZE);
	done = _vsnprintf(small_buffer,sizeof(INTERNAL_BUFFER_SIZE),format,arg);
	if(done == -1 || done == INTERNAL_BUFFER_SIZE)
	{   /* buffer is too small; try to allocate two times larger */
		do
		{
			big_buffer = winx_virtual_alloc((unsigned long)size);
			if(!big_buffer) { va_end(arg); return 0; }
			memset(big_buffer,0,size);
			done = _vsnprintf(big_buffer,size,format,arg);
			if(done != -1 && done != size) break;
			winx_virtual_free(big_buffer,(unsigned long)size);
			size = size << 1;
			if(!size) { va_end(arg); return 0; }
		} while(1);
	}
	if(big_buffer)
	{
		winx_print(big_buffer);
		winx_virtual_free(big_buffer,(unsigned long)size);
	}
	else
	{
		winx_print(small_buffer);
	}
	va_end(arg);
	return done;
}

/****f* zenwinx.stdio/winx_kbhit
 *  NAME
 *     winx_kbhit
 *  SYNOPSIS
 *     result = winx_kbhit(msec);
 *  FUNCTION
 *     Read a character from the keyboard
 *     waiting a specified time interval.
 *  INPUTS
 *     msec - the time interval, in milliseconds.
 *  RESULT
 *     If any key was pressed, the return value is
 *     the ascii character or zero for the control keys.
 *     Otherwise, the return value is -1.
 *  EXAMPLE
 *     // Prompt to exit.
 *     winx_printf("Press any key to exit...  ");
 *     for(i = 0; i < 3; i++)
 *     {
 *         if(winx_kbhit(1000) != -1)
 *         {
 *             winx_printf("\nGood bye ...\n");
 *             winx_exit(0);
 *         }
 *         winx_printf("%c ",(char)('0' + 3 - i));
 *     }
 *  NOTES
 *     If msec is INFINITE, the function's
 *     time-out interval never elapses. 
 *  BUGS
 *     The erroneous situation, when we have
 *     timeout for read request but
 *     the next NtCancelIoFile() call is
 *     unsuccessful, is not handled.
 *  SEE ALSO
 *     winx_breakhit
 ******/
int __cdecl winx_kbhit(int msec)
{
	NTSTATUS Status;
	LARGE_INTEGER interval;

	if(!hKbDevice) return -1;
	if(msec != INFINITE)
		interval.QuadPart = -((signed long)msec * 10000);
	else
		interval.QuadPart = MAX_WAIT_INTERVAL;
	ByteOffset.QuadPart = 0;
	Status = NtReadFile(hKbDevice,hKbEvent,NULL,NULL,
		&iosb,&kbd,sizeof(KEYBOARD_INPUT_DATA),&ByteOffset,0);
	/* wait in case operation is pending */
	if(Status == STATUS_PENDING)
	{
		Status = NtWaitForSingleObject(hKbEvent,FALSE,&interval);
		if(Status == STATUS_TIMEOUT)
		{ 
		   /* 
		    * If we have timeout we should cancel read operation
		    * to empty keyboard pending operations queue.
			*/
			Status = NtCancelIoFile(hKbDevice,&iosb);
			if(!NT_SUCCESS(Status))
			{
				/* this is a hard error because the next read request
			     * fill global kbd record instead of the special one */
				/*
				 * FIXME: handle this error.
				 */
			}
			return -1;
		}
		if(NT_SUCCESS(Status)) Status = iosb.Status;
	}
	if(!NT_SUCCESS(Status)) return -1;
	IntTranslateKey(&kbd,&kbd_rec);
	if(!kbd_rec.bKeyDown) return -1;
	return (int)kbd_rec.AsciiChar;
}

/****f* zenwinx.stdio/winx_breakhit
 *  NAME
 *     winx_breakhit
 *  SYNOPSIS
 *     result = winx_breakhit(msec);
 *  FUNCTION
 *     A special variant of winx_kbhit() function.
 *  INPUTS
 *     msec - the time interval, in milliseconds.
 *  RESULT
 *     If 'Break' key was pressed, the return value is zero.
 *     Otherwise, the return value is -1.
 *  EXAMPLE
 *     // Prompt to exit.
 *     winx_printf("Press 'Break' key to exit...  ");
 *     if(winx_breakhit(1000) != -1)
 *     {
 *         winx_printf("\nGood bye ...\n");
 *         winx_exit(0);
 *     }
 *  NOTES
 *     If msec is INFINITE, the function's
 *     time-out interval never elapses.
 *  BUGS
 *     This function is based on winx_kbhit(),
 *     so look at bugs of them.
 *  SEE ALSO
 *     winx_kbhit
 ******/
int __cdecl winx_breakhit(int msec)
{
	if(winx_kbhit(msec) == -1) return -1;
	return ((kbd.Flags & KEY_E1) && (kbd.MakeCode == 0x1d)) ? 0 : -1;
}

/****f* zenwinx.stdio/winx_getch
 *  NAME
 *     winx_getch
 *  SYNOPSIS
 *     character = winx_getch();
 *  FUNCTION
 *     CRT getch() native equivalent.
 *  SEE ALSO
 *     winx_getche,winx_gets
 ******/
int __cdecl winx_getch(void)
{
	KEYBOARD_INPUT_DATA kbd;
	IO_STATUS_BLOCK iosb;
	LARGE_INTEGER ByteOffset;
	KBD_RECORD kbd_rec;
	NTSTATUS Status;

	if(!hKbDevice) return -1;
repeate_attempt:
	ByteOffset.QuadPart = 0;
	Status = NtReadFile(hKbDevice,hKbEvent,NULL,NULL,
		&iosb,&kbd,sizeof(KEYBOARD_INPUT_DATA),&ByteOffset,0);
	/* wait in case operation is pending */
	if(Status == STATUS_PENDING)
	{
		Status = NtWaitForSingleObject(hKbEvent,FALSE,NULL);
		if(NT_SUCCESS(Status)) Status = iosb.Status;
	}
	if(!NT_SUCCESS(Status)) return -1;
	IntTranslateKey(&kbd,&kbd_rec);
	if(!kbd_rec.bKeyDown) goto repeate_attempt;
	return (int)kbd_rec.AsciiChar;
}

/****f* zenwinx.stdio/winx_getche
 *  NAME
 *     winx_getche
 *  SYNOPSIS
 *     character = winx_getche();
 *  FUNCTION
 *     CRT getche() native equivalent.
 *  BUGS
 *     Does not recognize special characters
 *     such as 'backspace'.
 *  SEE ALSO
 *     winx_getch,winx_gets
 ******/
int __cdecl winx_getche(void)
{
	int ch;

	ch = winx_getch();
	if(ch != -1 && ch != 0) winx_putch(ch);
	return ch;
}

/****f* zenwinx.stdio/winx_gets
 *  NAME
 *     winx_gets
 *  SYNOPSIS
 *     result = winx_gets(buffer, n);
 *  FUNCTION
 *     CRT gets() native equivalent, but with
 *     limited number of characters to read.
 *  INPUTS
 *     buffer - storage location for input string.
 *     n      - the maximum number of characters to read.
 *  RESULT
 *     nonnegative value for number of characters
 *     -1 for error.
 *  BUGS
 *     Does not recognize special characters
 *     such as 'backspace'.
 *  SEE ALSO
 *     winx_getch,winx_getche
 ******/
int __cdecl winx_gets(char *string,int n)
{
	int i;
	int ch;

	if(!string) return -1;
	for(i = 0; i < n; i ++)
	{
repeate_attempt:
		ch = winx_getche();
		if(ch == -1) return -1;
		if(ch == 0) goto repeate_attempt;
		if(ch == 13) { winx_putch('\n'); string[i] = 0; break; }
		string[i] = (char)ch;
	}
	if(i == n) return -1;
	return i;
}
