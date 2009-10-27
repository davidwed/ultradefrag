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
* User mode driver - global objects and ancillary routines.
*/

#include "globals.h"

int nt4_system = 0;
int w2k_system = 0;

ULONG disable_reports = FALSE;
ULONG dbgprint_level = DBG_NORMAL;

BOOLEAN context_menu_handler = FALSE;

HANDLE hSynchEvent = NULL;
HANDLE hStopEvent = NULL;
HANDLE hMapEvent = NULL;

STATISTIC Stat;

PFREEBLOCKMAP free_space_map = NULL;
PFILENAME filelist = NULL;
PFRAGMENTED fragmfileslist = NULL;

WINX_FILE *fVolume = NULL;

unsigned char volume_letter = 0;

ULONGLONG bytes_per_cluster = 0;
ULONG bytes_per_sector = 0;
ULONG sectors_per_cluster = 0;
ULONGLONG total_space = 0;
ULONGLONG free_space = 0; /* in bytes */
ULONGLONG clusters_total = 0;
ULONGLONG clusters_per_256k = 0;

unsigned char partition_type = UNKNOWN_PARTITION;

ULONGLONG mft_size = 0;
ULONG ntfs_record_size = 0;
ULONGLONG max_mft_entries = 0;

BOOLEAN optimize_flag = FALSE;

void InitSynchObjects(void);
void DestroySynchObjects(void);

void InitDriverResources(void)
{
	int os_version;
	int mj, mn;
	
	/* print driver version */
	winx_dbg_print("------------------------------------------------------------\n");
	DebugPrint("%s\n",VERSIONINTITLE);

	/* get windows version */
	os_version = winx_get_os_version();
	mj = os_version / 10;
	mn = os_version % 10;
	/* FIXME: windows 9x? */
	DebugPrint("Windows NT %u.%u\n",mj,mn);
	/*dx->xp_compatible = (((mj << 6) + mn) > ((5 << 6) + 0));*/
	nt4_system = (mj == 4) ? 1 : 0;
	w2k_system = (os_version == 50) ? 1 : 0;
	
	InitSynchObjects();
	
	memset(&Stat,0,sizeof(STATISTIC));
	
	DebugPrint("User mode driver loaded successfully\n");
}

void FreeDriverResources(void)
{
	FreeMap();
	DestroyFilter();
	DestroySynchObjects();
	DestroyLists();
	CloseVolume();
}

void __stdcall InitSynchObjectsErrorHandler(short *msg)
{
	winx_dbg_print("------------------------------------------------------------\n");
	winx_dbg_print("%ws\n",msg);
	winx_dbg_print("------------------------------------------------------------\n");
}

void InitSynchObjects(void)
{
	ERRORHANDLERPROC eh;
	eh = winx_set_error_handler(InitSynchObjectsErrorHandler);
	winx_create_event(L"\\udefrag_synch_event",
		SynchronizationEvent,&hSynchEvent);
	winx_create_event(L"\\udefrag_stop_event",
		NotificationEvent,&hStopEvent);
	winx_create_event(L"\\udefrag_map_event",
		SynchronizationEvent,&hMapEvent);
	winx_set_error_handler(eh);
	
	if(hSynchEvent) NtSetEvent(hSynchEvent,NULL);
	if(hStopEvent) NtClearEvent(hStopEvent);
	if(hMapEvent) NtSetEvent(hMapEvent,NULL);
}

int CheckForSynchObjects(void)
{
	if(!hSynchEvent || !hStopEvent || !hMapEvent) return (-1);
	return 0;
}

BOOLEAN CheckForStopEvent(void)
{
	LARGE_INTEGER interval;
	NTSTATUS Status;

	interval.QuadPart = (-1); /* 100 nsec */
	Status = NtWaitForSingleObject(hStopEvent,FALSE,&interval);
	if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status))	return FALSE;
	NtSetEvent(hStopEvent,NULL);
	return TRUE;
}

void DestroySynchObjects(void)
{
	winx_destroy_event(hSynchEvent);
	winx_destroy_event(hStopEvent);
	winx_destroy_event(hMapEvent);
}

void DestroyLists(void)
{
	PFILENAME pfn;
	
	pfn = filelist;
	if(pfn){
		do {
			DestroyList((PLIST *)&pfn->blockmap);
			RtlFreeUnicodeString(&pfn->name);
			pfn = pfn->next_ptr;
		} while(pfn != filelist);
		DestroyList((PLIST *)(void *)&filelist);
	}
	DestroyList((PLIST *)(void *)&free_space_map);
	DestroyList((PLIST *)(void *)&fragmfileslist);
}
