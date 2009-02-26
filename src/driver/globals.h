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
* Global variables definitions.
*/

extern ULONG dbg_level;
extern UCHAR BitShift[];
extern short device_name[];
extern short link_name[];

extern char *no_mem;

extern KEVENT sync_event;
extern KEVENT sync_event_2;
extern KEVENT stop_event;
extern KEVENT unload_event;
extern KEVENT dbgprint_event;
extern short *dbg_buffer;
extern unsigned int dbg_offset;
extern short log_path[MAX_PATH];

extern int nt4_system;
extern PVOID kernel_addr;
extern PVOID (NTAPI *ptrMmMapLockedPagesSpecifyCache)(PMDL,KPROCESSOR_MODE,
	MEMORY_CACHING_TYPE,PVOID,ULONG,MM_PAGE_PRIORITY);
extern VOID (NTAPI *ptrExFreePoolWithTag)(PVOID,ULONG);
extern BOOLEAN (NTAPI *ptrKeRegisterBugCheckReasonCallback)(
	PKBUGCHECK_REASON_CALLBACK_RECORD,PKBUGCHECK_REASON_CALLBACK_ROUTINE,
	KBUGCHECK_CALLBACK_REASON,PUCHAR);
extern BOOLEAN (NTAPI *ptrKeDeregisterBugCheckReasonCallback)(
    PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord);

extern KBUGCHECK_CALLBACK_RECORD bug_check_record;
extern KBUGCHECK_REASON_CALLBACK_RECORD bug_check_reason_record;

extern GUID ultradfg_guid;

extern char invalid_request[];

extern ULONGLONG (*new_cluster_map)[NUM_OF_SPACE_STATES];
extern ULONG map_size;

extern BOOLEAN context_menu_handler;
