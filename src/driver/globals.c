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
* Global variables and misc. functions 
* for data initialization/destroying.
*/

#include "driver.h"

ULONG dbg_level;
/* Bit shifting array for efficient processing of the bitmap */
UCHAR BitShift[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

short device_name[] = L"\\Device\\UltraDefrag";
short link_name[] = L"\\DosDevices\\ultradfg"; 	/* On NT can be L"\\??\\ultradfg" */

char *no_mem = "-Ultradfg- No Enough Memory!\n";

KEVENT sync_event;
KEVENT sync_event_2;
KEVENT stop_event;
KEVENT unload_event;
KEVENT dbgprint_event;
short *dbg_buffer;
unsigned int dbg_offset;
short log_path[MAX_PATH] = L"Empty Path";

char invalid_request[] = "-Ultradfg- 32-bit requests can't be accepted by 64-bit driver!\n";

/*
* Buffer to store the number of clusters of each kind.
* More details at http://www.thescripts.com/forum/thread617704.html
* ('Dynamically-allocated Multi-dimensional Arrays - C').
*/
ULONGLONG (*new_cluster_map)[NUM_OF_SPACE_STATES];
ULONG map_size = 0;

/* Partial Device Extension initialization */
void InitDX(UDEFRAG_DEVICE_EXTENSION *dx)
{
	memset(&dx->z_start,0,(LONG_PTR)&(dx->z_end) - (LONG_PTR)&(dx->z_start));
	//DbgPrint("%u\n",(LONG_PTR)&(dx->z_end) - (LONG_PTR)&(dx->z_start));
	dx->hVol = NULL;
	dx->current_operation = 'A';
}

/* Full Device Extension initialization */
void InitDX_0(UDEFRAG_DEVICE_EXTENSION *dx)
{
	KeInitializeEvent(&sync_event,SynchronizationEvent,TRUE);
	KeInitializeEvent(&sync_event_2,SynchronizationEvent,TRUE);
	KeInitializeEvent(&stop_event,NotificationEvent,FALSE);
	KeInitializeEvent(&unload_event,NotificationEvent,FALSE);
	memset(&dx->z_start,0,(LONG_PTR)&(dx->z0_end) - (LONG_PTR)&(dx->z_start));
	dx->current_operation = 'A';
	dx->disable_reports = FALSE;
	dx->pnextLcn = &dx->nextLcn;
	dx->pmoveFile = &dx->moveFile;
	dx->pstartVcn = &dx->startVcn;
}

void FreeAllBuffers(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFILENAME ptr,next_ptr;

	ptr = dx->filelist;
	while(ptr){
		next_ptr = ptr->next_ptr;
		DestroyList((PLIST *)&ptr->blockmap);
		RtlFreeUnicodeString(&ptr->name);
		ExFreePool((void *)ptr);
		ptr = next_ptr;
	}
	DestroyList((PLIST *)&dx->free_space_map);
	DestroyList((PLIST *)&dx->fragmfileslist);
	dx->filelist = NULL;
	CloseVolume(dx);
}
