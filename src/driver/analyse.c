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
	short path[50] = L"\\??\\A:\\";
	NTSTATUS Status;
	UNICODE_STRING us;

	/* Initialization */
	MarkAllSpaceAsSystem0(dx);
	FreeAllBuffers(dx);
	InitDX(dx);
	KeClearEvent(&dx->stop_event);

	/* Volume space analysis */
	DeleteLogFile(dx);
	Status = OpenVolume(dx);
	if(!NT_SUCCESS(Status)) goto fail;
	Status = GetVolumeInfo(dx);
	if(!NT_SUCCESS(Status)) goto fail;
	__NtFlushBuffersFile(dx->hVol);
	Status = FillFreeSpaceMap(dx);
	if(!NT_SUCCESS(Status)) goto fail;

	/* Find files */
	path[4] = (short)dx->letter;
	if(!RtlCreateUnicodeString(&us,path)){
		DbgPrintNoMem();
		Status = STATUS_NO_MEMORY;
		goto fail;
	}
	if(!FindFiles(dx,&us,TRUE)){
		RtlFreeUnicodeString(&us);
		Status = STATUS_NO_MORE_FILES;
		goto fail;
	}
	RtlFreeUnicodeString(&us);
	if(!dx->filelist){
		Status = STATUS_NO_MORE_FILES;
		goto fail;
	}
	DebugPrint("-Ultradfg- Files found: %u\n",dx->filecounter);
	DebugPrint("-Ultradfg- Fragmented files: %u\n",dx->fragmfilecounter);

	/* Get MFT location */
	if(dx->partition_type == NTFS_PARTITION) ProcessMFT(dx);

	/* Save state */
	SaveFragmFilesListToDisk(dx);
	if(KeReadStateEvent(&dx->stop_event) == 0x0)
		dx->status = STATUS_ANALYSED;
	else
		dx->status = STATUS_BEFORE_PROCESSING;
	dx->clusters_to_move = dx->clusters_to_move_initial = dx->clusters_to_move_tmp;
	dx->clusters_to_compact_initial = dx->clusters_to_compact_tmp;
	return STATUS_SUCCESS;
fail:
	return Status;
}
