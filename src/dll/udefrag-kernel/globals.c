/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

/**
 * @file globals.c
 * @brief Ancillary routines code.
 * @{
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

UDEFRAG_JOB_TYPE JobType = ANALYSE_JOB;

PFREEBLOCKMAP free_space_map = NULL;
PFILENAME filelist = NULL;
PFRAGMENTED fragmfileslist = NULL;

WINX_FILE *fVolume = NULL;

unsigned char volume_letter = 0;

ULONGLONG bytes_per_cluster = 0;
ULONG bytes_per_sector = 0;
ULONG sectors_per_cluster = 0;
ULONGLONG clusters_total = 0;
ULONGLONG clusters_per_256k = 0;

unsigned char partition_type = UNKNOWN_PARTITION;

ULONG ntfs_record_size = 0;
ULONGLONG max_mft_entries = 0;
ULONGLONG mft_start = 0, mft_end = 0, mftzone_start = 0, mftzone_end = 0;
ULONGLONG mftmirr_start = 0, mftmirr_end = 0;

BOOLEAN optimize_flag = FALSE;
BOOLEAN initial_analysis = FALSE;

ULONGLONG out_of_memory_condition_counter = 0;

void InitSynchObjects(void);
void DestroySynchObjects(void);

/**
 * @brief Initializes the library.
 */
void InitDriverResources(void)
{
	int os_version;
	int mj, mn;
	
	/* print driver version */
	DebugPrint("------------------------------------------------------------\n");
	DebugPrint("%s\n",VERSIONINTITLE);

	/* get windows version */
	os_version = winx_get_os_version();
	mj = os_version / 10;
	mn = os_version % 10;
	DebugPrint("Windows NT %u.%u\n",mj,mn);
	/*dx->xp_compatible = (((mj << 6) + mn) > ((5 << 6) + 0));*/
	nt4_system = (mj == 4) ? 1 : 0;
	w2k_system = (os_version == 50) ? 1 : 0;
	
	InitSynchObjects();
	
	memset(&Stat,0,sizeof(STATISTIC));
	Stat.pass_number = 0xffffffff;
	
	DebugPrint("User mode driver loaded successfully\n");
}

/**
 * @brief Frees resources allocated by the library.
 */
void FreeDriverResources(void)
{
	FreeMap();
	DestroyFilter();
	DestroySynchObjects();
	DestroyLists();
	CloseVolume();
	DebugPrint("User mode driver unloaded successfully\n");
}

/**
 * @brief Initializes synchronization events.
 */
void InitSynchObjects(void)
{
	short event_name[64];
	
	/*
	* Attach process ID to the event names
	* to make the code safe for execution
	* by many parallel processes.
	*/
	if(hSynchEvent == NULL){
		_snwprintf(event_name,64,L"\\udefrag_synch_event_%u",
			(unsigned int)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess));
		event_name[63] = 0;
		(void)winx_create_event(event_name,SynchronizationEvent,&hSynchEvent);
	}
	if(hStopEvent == NULL){
		_snwprintf(event_name,64,L"\\udefrag_stop_event_%u",
			(unsigned int)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess));
		event_name[63] = 0;
		(void)winx_create_event(event_name,NotificationEvent,&hStopEvent);
	}
	if(hMapEvent == NULL){
		_snwprintf(event_name,64,L"\\udefrag_map_event_%u",
			(unsigned int)(DWORD_PTR)(NtCurrentTeb()->ClientId.UniqueProcess));
		event_name[63] = 0;
		(void)winx_create_event(event_name,SynchronizationEvent,&hMapEvent);
	}
	
	if(hSynchEvent) (void)NtSetEvent(hSynchEvent,NULL);
	if(hStopEvent) (void)NtClearEvent(hStopEvent);
	if(hMapEvent) (void)NtSetEvent(hMapEvent,NULL);
}

/**
 * @brief Checks for synchronization events existence.
 * @return Zero for success, negative value otherwise.
 */
int CheckForSynchObjects(void)
{
	if(!hSynchEvent || !hStopEvent || !hMapEvent) return (-1);
	return 0;
}

/**
 * @brief Checks for the signaled stop event.
 * @return Boolean value. TRUE indicates that
 * the stop event is in signaled state, FALSE
 * indicates contrary.
 */
BOOLEAN CheckForStopEvent(void)
{
	LARGE_INTEGER interval;
	NTSTATUS Status;
	
	/*
	* In case when the stop event does not exists,
	* it seems to be safer to indicate that it is
	* signaled.
	*/
	if(!hStopEvent) return TRUE;

	interval.QuadPart = 0; /* check as fast as possible */
	Status = NtWaitForSingleObject(hStopEvent,FALSE,&interval);
	if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status))	return FALSE;
	(void)NtSetEvent(hStopEvent,NULL);
	return TRUE;
}

/**
 * @brief Destroys synchronization events.
 */
void DestroySynchObjects(void)
{
	winx_destroy_event(hSynchEvent);
	winx_destroy_event(hStopEvent);
	winx_destroy_event(hMapEvent);
}

/**
 * @brief Destroys the free space list, the file list
 * and the list of the fragmented files.
 */
void DestroyLists(void)
{
	PFILENAME pfn;
	
	pfn = filelist;
	if(pfn){
		do {
			winx_list_destroy((list_entry **)&pfn->blockmap);
			RtlFreeUnicodeString(&pfn->name);
			pfn = pfn->next_ptr;
		} while(pfn != filelist);
		winx_list_destroy((list_entry **)(void *)&filelist);
	}
	winx_list_destroy((list_entry **)(void *)&free_space_map);
	winx_list_destroy((list_entry **)(void *)&fragmfileslist);
}

/** @} */
