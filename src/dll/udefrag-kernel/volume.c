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
 * @file volume.c
 * @brief Volume parameters retrieving code.
 * @addtogroup Volume
 * @{
 */

#include "globals.h"

/**
 * @brief Opens the volume handle.
 * @param[in] volume_name the name of the volume.
 * @return Zero for success, negative value otherwise.
 */
int OpenVolume(char *volume_name)
{
	char path[64];
	char flags[2];
	#define FLAG 'r'
	
	CloseVolume();
	(void)_snprintf(path,64,"\\??\\%s:",volume_name);
	path[63] = 0;
#if FLAG != 'r'
#error Volume must be opened for read access!
#endif
	flags[0] = FLAG; flags[1] = 0;
	fVolume = winx_fopen(path,flags);
	if(!fVolume){
		DebugPrint("Cannot open volume %s!\n",path);
		return (-1);
	}
	return 0;
}

/**
 * @brief Retrieves the volume geometry.
 * @param[in] volume_name the name of the volume.
 * @return Zero for success, negative value otherwise.
 */
int GetDriveGeometry(char *volume_name)
{
	char path[64];
	char flags[2];
	WINX_FILE *fRoot;
	#define FLAG 'r'

	FILE_FS_SIZE_INFORMATION *pFileFsSize;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;
	ULONGLONG bpc; /* bytes per cluster */
	
	/* reset geometry related variables */
	bytes_per_cluster = 0;
	bytes_per_sector = 0;
	sectors_per_cluster = 0;
	Stat.total_space = 0;
	Stat.free_space = 0;
	clusters_total = 0;
	clusters_per_256k = 0;
	
	/* get drive geometry */
	(void)_snprintf(path,64,"\\??\\%s:\\",volume_name);
	path[63] = 0;
#if FLAG != 'r'
#error Root directory must be opened for read access!
#endif
	flags[0] = FLAG; flags[1] = 0;
	fRoot = winx_fopen(path,flags);
	if(!fRoot){
		DebugPrint("Cannot open the root directory %s!\n",path);
		return (-1);
	}

	pFileFsSize = winx_heap_alloc(sizeof(FILE_FS_SIZE_INFORMATION));
	if(!pFileFsSize){
		winx_fclose(fRoot);
		DebugPrint("udefrag-kernel.dll GetDriveGeometry(): no enough memory!");
		return UDEFRAG_NO_MEM;
	}

	/* get logical geometry */
	RtlZeroMemory(pFileFsSize,sizeof(FILE_FS_SIZE_INFORMATION));
	status = NtQueryVolumeInformationFile(winx_fileno(fRoot),&iosb,pFileFsSize,
			  sizeof(FILE_FS_SIZE_INFORMATION),FileFsSizeInformation);
	winx_fclose(fRoot);
	if(status != STATUS_SUCCESS){
		winx_heap_free(pFileFsSize);
		DebugPrintEx(status,"FileFsSizeInformation() request failed for %s",path);
		return (-1);
	}

	bpc = pFileFsSize->SectorsPerAllocationUnit * pFileFsSize->BytesPerSector;
	sectors_per_cluster = pFileFsSize->SectorsPerAllocationUnit;
	bytes_per_cluster = bpc;
	bytes_per_sector = pFileFsSize->BytesPerSector;
	Stat.total_space = pFileFsSize->TotalAllocationUnits.QuadPart * bpc;
	Stat.free_space = pFileFsSize->AvailableAllocationUnits.QuadPart * bpc;
	clusters_total = (ULONGLONG)(pFileFsSize->TotalAllocationUnits.QuadPart);
	if(bytes_per_cluster) clusters_per_256k = _256K / bytes_per_cluster;
	DebugPrint("Total clusters: %I64u\n",clusters_total);
	DebugPrint("Cluster size: %I64u\n",bytes_per_cluster);
	if(!clusters_per_256k){
		DebugPrint("Clusters are larger than 256 kbytes!\n");
		clusters_per_256k ++;
	}
	
	/* validate geometry */
	if(!clusters_total || !bytes_per_cluster){
		winx_heap_free(pFileFsSize);
		DebugPrint("Wrong volume geometry!");
		return (-1);
	}
	winx_heap_free(pFileFsSize);
	return 0;
}

/**
 * @brief Closes the volume handle.
 */
void CloseVolume(void)
{
	winx_fclose(fVolume);
	fVolume = NULL;
}

/** @} */
