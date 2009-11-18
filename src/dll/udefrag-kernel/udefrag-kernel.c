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

/* FIXME: Better error handling! */

#include "globals.h"

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
	
	/* FIXME: stop request will fail when completes before this point */
	NtClearEvent(hStopEvent);
	
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
	
	/* 4. prepare for job */
	_strupr(volume_name);
	volume_letter = volume_name[0];
	RemoveReportFromDisk(volume_name);
	if(job_type == OPTIMIZE_JOB) optimize_flag = TRUE;
	else optimize_flag = FALSE;
	JobType = job_type;
	
	/* 5. analyse volume */
	if(Analyze(volume_name) < 0) goto failure;
	
	/* 6. defragment/optimize volume */
	if(job_type == ANALYSE_JOB) goto success;
	if(CheckForStopEvent()) goto success;
	/*
	* NTFS volumes with cluster size greater than 4 kb
	* cannot be defragmented on Windows 2000.
	* This is a well known limitation of Windows Defrag API.
	*/
	if(partition_type == NTFS_PARTITION && bytes_per_cluster > 4096 && w2k_system){
		winx_raise_error("E: Cannot defragment NTFS volumes with\n"
						 "cluster size greater than 4 kb\n"
						 "on Windows 2000 (read docs for details).");
		goto failure;
	}
	if(job_type == DEFRAG_JOB) Defragment(volume_name);
	else Optimize(volume_name);

success:		
	/* 7. save report */
	SaveReportToDisk(volume_name);
	
	/* FreeMap(); - NEVER CALL IT HERE */
	
	DestroyLists();
	CloseVolume();
	NtSetEvent(hSynchEvent,NULL);
	NtClearEvent(hStopEvent);
	return 0;
	
failure:
	DestroyLists();
	CloseVolume();
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
