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

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> /* for toupper() on mingw */
#include "../../include/ntndk.h"
#include "../../include/udefrag.h"
#include "../../include/ultradfg.h"
#include "../zenwinx/zenwinx.h"
#include "../../include/udefrag-kernel.h"

#define NtCloseSafe(h) if(h) { NtClose(h); h = NULL; }

//eh(L"NTFS volumes with cluster size greater than 4 kb\n"
//   L"cannot be defragmented on Windows 2000.");

BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	return 1;
}

/*
* Start the job.
*
* Return value: -1 indicates error, zero otherwise.
*/
int __stdcall udefrag_kernel_start(char *volume_name, UDEFRAG_JOB_TYPE job_type, int cluster_map_size)
{
	DebugPrint("-Udkernel- Start, volume = %s, job = %u, map size = %u\n",
		volume_name,job_type,cluster_map_size);
	return 0;
}

/*
* Stop the job.
*
* Return value: -1 indicates error, zero otherwise.
*/
int __stdcall udefrag_kernel_stop(void)
{
	DebugPrint("-Udkernel- Stop\n");
	return 0;
}

/*
* Retrieve statistics.
*
* Return value: -1 indicates error, zero otherwise.
*/
int __stdcall udefrag_kernel_get_statistic(STATISTIC *stat, char *map, int map_size)
{
	DebugPrint("-Udkernel- Get Statistic\n");
	return 0;
}
