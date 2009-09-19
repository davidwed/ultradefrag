/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
*  Procedures for debug logging.
*/

#include <stdarg.h>
#include <stdio.h>

#include "driver.h"

#if 0
/*
* It must be always greater than PAGE_SIZE (8192 ?), because the 
* dbg_buffer needs to be aligned on a page boundary. This is a 
* special requirement of BugCheckSecondaryDumpDataCallback() 
* function described in DDK documentation.
*/
#define DBG_BUFFER_SIZE (16 * 1024) /* 16 kb - more than enough */

VOID __stdcall BugCheckCallback(PVOID Buffer,ULONG Length);
VOID __stdcall BugCheckSecondaryDumpDataCallback(
    KBUGCHECK_CALLBACK_REASON Reason,
    PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    PVOID ReasonSpecificData,
    ULONG ReasonSpecificDataLength);

KBUGCHECK_CALLBACK_RECORD bug_check_record;
KBUGCHECK_REASON_CALLBACK_RECORD bug_check_reason_record;

// {B7B5FCD6-FB0A-48f9-AEE5-02AF097C9504}
GUID ultradfg_guid = \
{ 0xb7b5fcd6, 0xfb0a, 0x48f9, { 0xae, 0xe5, 0x2, 0xaf, 0x9, 0x7c, 0x95, 0x4 } };

void __stdcall RegisterBugCheckCallbacks(void)
{
	KeInitializeCallbackRecord((&bug_check_record)); /* double brackets are required */
	KeInitializeCallbackRecord((&bug_check_reason_record));
	/*
	* Register callback on systems older than XP SP1 / Server 2003.
	* There is one significant restriction: bug data will be saved only 
	* in full kernel memory dump.
	*/
	if(!KeRegisterBugCheckCallback(&bug_check_record,(VOID *)BugCheckCallback,
		NULL/*dbg_buffer*/,0/*DBG_BUFFER_SIZE*/,"UltraDefrag"))
			DebugPrint("=Ultradfg= Cannot register bug check routine!\n");
	/*
	* Register another callback procedure on modern systems: XP SP1, Server 2003 etc.
	* Bug data will be available even in small memory dump, but just in theory :(
	* Though it seems that Vista saves it.
	*/
	if(ptrKeRegisterBugCheckReasonCallback){
		if(!ptrKeRegisterBugCheckReasonCallback(&bug_check_reason_record,
			(VOID *)BugCheckSecondaryDumpDataCallback,
			KbCallbackSecondaryDumpData,
			"UltraDefrag"))
				DebugPrint("=Ultradfg= Cannot register bug check dump data routine!\n");
	}
}

VOID __stdcall BugCheckCallback(PVOID Buffer,ULONG Length)
{
}

VOID __stdcall BugCheckSecondaryDumpDataCallback(
    KBUGCHECK_CALLBACK_REASON Reason,
    PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    PVOID ReasonSpecificData,
    ULONG ReasonSpecificDataLength)
{
	PKBUGCHECK_SECONDARY_DUMP_DATA p;
	
	p = (PKBUGCHECK_SECONDARY_DUMP_DATA)ReasonSpecificData;
	p->OutBuffer = NULL;//dbg_buffer;
	p->OutBufferLength = 0;//DBG_BUFFER_SIZE;
	p->Guid = ultradfg_guid;
}

void __stdcall DeregisterBugCheckCallbacks(void)
{
	KeDeregisterBugCheckCallback(&bug_check_record);
	if(ptrKeDeregisterBugCheckReasonCallback)
		ptrKeDeregisterBugCheckReasonCallback(&bug_check_reason_record);
}
#else
void __stdcall RegisterBugCheckCallbacks(void)
{
}
void __stdcall DeregisterBugCheckCallbacks(void)
{
}
#endif
