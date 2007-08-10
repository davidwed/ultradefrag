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
 *  Main header.
 */

#ifndef ULTRADFG_NATIVE_H
#define ULTRADFG_NATIVE_H

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>

#include <stdlib.h>

#ifndef USE_WINDDK
#include <ntndk.h>
#else
#include "ntndk.h"
#endif

#include <ntddkbd.h>

#include "../include/misc.h"
#include "../include/ultradfg.h"

#define USE_INSTEAD_SMSS  0

#define SE_MANAGE_VOLUME_PRIVILEGE  0x1c

typedef struct _KBD_RECORD {
	WORD	wVirtualScanCode;
	DWORD	dwControlKeyState;
	UCHAR	AsciiChar;
	BOOL	bKeyDown;
} KBD_RECORD, *PKBD_RECORD;

void IntTranslateKey(PKEYBOARD_INPUT_DATA InputData,KBD_RECORD *kbd_rec);
#endif /* ULTRADFG_NATIVE_H */