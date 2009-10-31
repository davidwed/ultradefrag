/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* User mode driver - volume information related code.
*/

#include "globals.h"

int OpenVolume(char *volume_name)
{
	char path[64];
	char flags[2];
	#define FLAG 'r'
	
	CloseVolume();
	_snprintf(path,64,"\\??\\%s:",volume_name);
	path[63] = 0;
#if FLAG != 'r'
#error Volume must be opened for read access!
#endif
	flags[0] = FLAG; flags[1] = 0;
	fVolume = winx_fopen(path,flags);
	if(!fVolume){
		winx_raise_error("E: Cannot open volume %s!",path);
		return (-1);
	}
	return 0;
}

int GetDriveGeometry(char *volume_name)
{
	char path[64];
	char flags[2];
	WINX_FILE *fRoot;
	#define FLAG 'r'

	FILE_FS_SIZE_INFORMATION FileFsSize;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;
	ULONGLONG bpc; /* bytes per cluster */
	
	/* reset geometry related variables */
	bytes_per_cluster = 0;
	bytes_per_sector = 0;
	sectors_per_cluster = 0;
	total_space = 0;
	free_space = 0;
	clusters_total = 0;
	clusters_per_256k = 0;
	
	/* get drive geometry */
	_snprintf(path,64,"\\??\\%s:\\",volume_name);
	path[63] = 0;
#if FLAG != 'r'
#error Root directory must be opened for read access!
#endif
	flags[0] = FLAG; flags[1] = 0;
	fRoot = winx_fopen(path,flags);
	if(!fRoot){
		winx_raise_error("E: Cannot open the root directory %s!",path);
		return (-1);
	}
	/* get logical geometry */
	status = NtQueryVolumeInformationFile(winx_fileno(fRoot),&iosb,&FileFsSize,
			  sizeof(FILE_FS_SIZE_INFORMATION),FileFsSizeInformation);
	winx_fclose(fRoot);
	if(status != STATUS_SUCCESS){
		winx_raise_error("E: FileFsSizeInformation() request failed for %s: %x!",
			path,(UINT)status);
		return (-1);
	}

	bpc = FileFsSize.SectorsPerAllocationUnit * FileFsSize.BytesPerSector;
	sectors_per_cluster = FileFsSize.SectorsPerAllocationUnit;
	bytes_per_cluster = bpc;
	bytes_per_sector = FileFsSize.BytesPerSector;
	total_space = FileFsSize.TotalAllocationUnits.QuadPart * bpc;
	free_space = FileFsSize.AvailableAllocationUnits.QuadPart * bpc;
	clusters_total = (ULONGLONG)(FileFsSize.TotalAllocationUnits.QuadPart);
	if(bytes_per_cluster) clusters_per_256k = _256K / bytes_per_cluster;
	DebugPrint("Total clusters: %I64u\n",clusters_total);
	DebugPrint("Cluster size: %I64u\n",bytes_per_cluster);
	if(!clusters_per_256k){
		DebugPrint("Clusters are larger than 256 kbytes!\n");
		clusters_per_256k ++;
	}
	
	/* validate geometry */
	if(!clusters_total || !bytes_per_cluster){
		winx_raise_error("E: Wrong volume geometry!");
		return (-1);
	}
	return 0;
}

void CloseVolume(void)
{
	winx_fclose(fVolume);
	fVolume = NULL;
}
