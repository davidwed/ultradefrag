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
* User mode driver.
*/

#include "globals.h"

#define NtCloseSafe(h) if(h) { NtClose(h); h = NULL; }

BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH)
		InitDriverResources();
	else if(dwReason == DLL_PROCESS_DETACH)
		FreeDriverResources();
	return 1;
}

/*
* Start the job.
*
* Return value: -1 indicates error, zero otherwise.
*/
int __stdcall udefrag_kernel_start(char *volume_name, UDEFRAG_JOB_TYPE job_type, int cluster_map_size)
{
	char *action = "analyzing";
	LARGE_INTEGER interval;
	NTSTATUS Status;
	
	/* 0a. check for synchronization objects */
	if(CheckForSynchObjects() < 0){
		winx_raise_error("E: Synchronization objects aren't available!");
		return (-1);
	}
	/* 0b. synchronize with other requests */
	interval.QuadPart = (-1); /* 100 nsec */
	Status = NtWaitForSingleObject(hSynchEvent,FALSE,&interval);
	if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status)){
		winx_raise_error("E: Driver is busy because the previous request was not completed!\n");
		return (-1);
	}
	
	/* 1. print header */
	if(job_type == DEFRAG_JOB) action = "defragmenting";
	if(job_type == OPTIMIZE_JOB) action = "optimizing";
	DebugPrint("Start %s volume %s\n",action,volume_name);
	
	/* 2. allocate cluster map */
	if(AllocateMap(cluster_map_size) < 0){
		winx_raise_error("E: Cannot allocate cluster map!");
		goto failure;
	}
	
	/* 3. read options from environment variables */
	InitializeOptions();
	
	/* FreeMap(); - NEVER CALL IT HERE */
	
	NtSetEvent(hSynchEvent,NULL);
	NtClearEvent(hStopEvent);
	return 0;
	
failure:
	NtSetEvent(hSynchEvent,NULL);
	NtClearEvent(hStopEvent);
	return (-1);
}

/*
* Stop the job.
*
* Return value: -1 indicates error, zero otherwise.
*/
int __stdcall udefrag_kernel_stop(void)
{
	DebugPrint("Stop\n");
	if(CheckForSynchObjects() < 0){
		winx_raise_error("E: Synchronization objects aren't available!");
		return (-1);
	}
	NtSetEvent(hStopEvent,NULL);
	return 0;
}

/*
* Retrieve statistics.
*
* Return value: -1 indicates error, zero otherwise.
*/
int __stdcall udefrag_kernel_get_statistic(STATISTIC *stat, char *map, int map_size)
{
	DebugPrint2("Get Statistic\n");
	if(stat) memcpy(stat,&Stat,sizeof(STATISTIC));
	if(map) GetMap(map,map_size);
	return 0;
}
