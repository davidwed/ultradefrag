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
 * @file udefrag-kernel.c
 * @brief Library interface code.
 * @addtogroup Interface
 * @{
 */

#include "globals.h"

/**
 * @brief udefrag-kernel.dll entry point.
 */
BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH)
		InitDriverResources();
	else if(dwReason == DLL_PROCESS_DETACH)
		FreeDriverResources();
	return 1;
}

/**
 * @brief Starts a disk defragmentation/analysis/optimization job.
 * @param[in] volume_name the name of the volume.
 * @param[in] job_type the type of the job.
 * @param[in] cluster_map_size the size of the cluster map, in bytes.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall udefrag_kernel_start(char *volume_name, UDEFRAG_JOB_TYPE job_type, int cluster_map_size)
{
	char *action = "analyzing";
	LARGE_INTEGER interval;
	NTSTATUS Status;
	int error_code = 0;

	/* 0a. check for synchronization objects */
	if(CheckForSynchObjects() < 0){
		DebugPrint("Synchronization objects aren't available!");
		return (-1);
	}
	/* 0b. synchronize with other requests */
	interval.QuadPart = (-1); /* 100 nsec */
	Status = NtWaitForSingleObject(hSynchEvent,FALSE,&interval);
	if(Status == STATUS_TIMEOUT || !NT_SUCCESS(Status)){
		DebugPrint("Driver is busy because the previous request was not completed!\n");
		return (-1);
	}
	
	/* FIXME: stop request will fail when completes before this point :-) */
	(void)NtClearEvent(hStopEvent);
	
	/* 1. print header */
	if(job_type == DEFRAG_JOB) action = "defragmenting";
	if(job_type == OPTIMIZE_JOB) action = "optimizing";
	DebugPrint("Start %s volume %s\n",action,volume_name);
	
	/* 2. allocate cluster map */
	if(AllocateMap(cluster_map_size) < 0){
		DebugPrint("Cannot allocate cluster map!");
		error_code = UDEFRAG_NO_MEM;
		goto failure;
	}
	
	/* 3. read options from environment variables */
	InitializeOptions();
	
	/* 4. prepare for job */
	(void)_strupr(volume_name);
	volume_letter = volume_name[0];
	RemoveReportFromDisk(volume_name);
	if(job_type == OPTIMIZE_JOB) optimize_flag = TRUE;
	else optimize_flag = FALSE;
	JobType = job_type;
	
	Stat.pass_number = 0;
	
	/* 5. analyse volume */
	error_code = Analyze(volume_name);
	if(error_code < 0) goto failure;
	
	/* 6. defragment/optimize volume */
	if(job_type == ANALYSE_JOB) goto success;
	if(CheckForStopEvent()) goto success;
	/*
	* NTFS volumes with cluster size greater than 4 kb
	* cannot be defragmented on Windows 2000.
	* This is a well known limitation of Windows Defrag API.
	*/
	if(partition_type == NTFS_PARTITION && bytes_per_cluster > 4096 && w2k_system){
		DebugPrint("Cannot defragment NTFS volumes with\n"
						 "cluster size greater than 4 kb\n"
						 "on Windows 2000 (read docs for details).");
		error_code = UDEFRAG_W2K_4KB_CLUSTERS;
		goto failure;
	}
	if(job_type == DEFRAG_JOB) (void)Defragment(volume_name);
	else (void)Optimize(volume_name);

success:		
	/* 7. save report */
	(void)SaveReportToDisk(volume_name);
	
	/* FreeMap(); - NEVER CALL IT HERE */
	
	DestroyLists();
	CloseVolume();
	Stat.pass_number = 0xffffffff;
	(void)NtSetEvent(hSynchEvent,NULL);
	(void)NtClearEvent(hStopEvent);
	return 0;
	
failure:
	DestroyLists();
	CloseVolume();
	Stat.pass_number = 0xffffffff;
	(void)NtSetEvent(hSynchEvent,NULL);
	(void)NtClearEvent(hStopEvent);
	return error_code;
}

/**
 * @brief Terminates the running disk defragmentation/analysis/optimization job.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall udefrag_kernel_stop(void)
{
	DebugPrint("Stop\n");
	if(CheckForSynchObjects() < 0){
		DebugPrint("Synchronization objects aren't available!");
		return (-1);
	}
	(void)NtSetEvent(hStopEvent,NULL);
	return 0;
}

/**
 * @brief Retrieves the disk defragmentation statistics.
 * @param[out] stat pointer to a variable receiving
 *                  statistical data.
 * @param[out] map pointer to buffer receiving the
 *                 cluster map.
 * @param[in] map_size the size of the cluster map,
 *                     in bytes.
 * @return Zero for success, negative value otherwise.
 */
int __stdcall udefrag_kernel_get_statistic(STATISTIC *stat, char *map, int map_size)
{
	DebugPrint2("Get Statistic\n");
	if(stat) memcpy(stat,&Stat,sizeof(STATISTIC));
	if(map){
		if(GetMap(map,map_size) < 0) return (-1);
	}
	return 0;
}

/** @} */
