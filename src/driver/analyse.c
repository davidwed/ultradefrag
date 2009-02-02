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
* Fragmentation analyse engine.
*/

#include "driver.h"
#if 0
ULONGLONG _rdtsc(void)
{
	ULONGLONG retval;

	__asm {
		rdtsc
		lea ebx, retval
		mov [ebx], eax
		mov [ebx+4], edx
	}
	return retval;
}
#endif

/* initialize files list and free space map */
NTSTATUS Analyse(UDEFRAG_DEVICE_EXTENSION *dx)
{
	short path[] = L"\\??\\A:\\";
	NTSTATUS Status;
	UNICODE_STRING us;
	//ULONGLONG tm;

	/* Initialization */
	MarkAllSpaceAsSystem0(dx);
	FreeAllBuffers(dx);
	InitDX(dx);

	/* Volume space analysis */
	DeleteLogFile(dx);
	Status = OpenVolume(dx);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- OpenVolume() failed: %x!\n",NULL,(UINT)Status);
		goto fail;
	}
	Status = GetVolumeInfo(dx);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- GetVolumeInfo() failed: %x!\n",NULL,(UINT)Status);
		goto fail;
	}
	/* update map representation */
	MarkAllSpaceAsSystem1(dx);

	//tm = _rdtsc();
	dx->clusters_to_process = dx->clusters_total;
	dx->processed_clusters = 0;
	Status = FillFreeSpaceMap(dx);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- FillFreeSpaceMap() failed: %x!\n",NULL,(UINT)Status);
		goto fail;
	}
	//DbgPrint("FillFreeSpaceMap() time: %I64u mln.\n",(_rdtsc() - tm) / 1000000);

	/* Find files */
	path[4] = (short)dx->letter;
	if(!RtlCreateUnicodeString(&us,path)){
		DebugPrint("-Ultradfg- no enough memory for path initialization!\n",NULL);
		Status = STATUS_NO_MEMORY;
		goto fail;
	}
	if(!FindFiles(dx,&us)){
		RtlFreeUnicodeString(&us);
		DebugPrint("-Ultradfg- FindFiles() failed!\n",NULL);
		Status = STATUS_NO_MORE_FILES;
		goto fail;
	}
	RtlFreeUnicodeString(&us);
	if(!dx->filelist){
		DebugPrint("-Ultradfg- No files found!\n",NULL);
		Status = STATUS_NO_MORE_FILES;
		goto fail;
	}
	DebugPrint("-Ultradfg- Files found: %u\n",NULL,dx->filecounter);
	DebugPrint("-Ultradfg- Fragmented files: %u\n",NULL,dx->fragmfilecounter);

	/* Get MFT location */
	if(KeReadStateEvent(&stop_event) == 0x0)
		if(dx->partition_type == NTFS_PARTITION) ProcessMFT(dx);

	/* Save state */
	SaveFragmFilesListToDisk(dx);
	if(KeReadStateEvent(&stop_event) == 0x0)
		dx->status = STATUS_ANALYSED;
	else
		dx->status = STATUS_BEFORE_PROCESSING;
	return STATUS_SUCCESS;
fail:
	dx->status = STATUS_BEFORE_PROCESSING;
	return Status;
}
