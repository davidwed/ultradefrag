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
#else
/* in milliseconds */
ULONGLONG _rdtsc(void)
{
	LARGE_INTEGER i;
	
	/* ugly implementation, but... */
	KeQueryTickCount(&i);
	return (ULONGLONG)i.QuadPart * KeQueryTimeIncrement() * 100 / 1000000;
}

/* in nanoseconds */
ULONGLONG _rdtsc_1(void)
{
	return KeQueryInterruptTime() * 100;
}
#endif

/* initialize files list and free space map */
NTSTATUS Analyse(UDEFRAG_DEVICE_EXTENSION *dx)
{
	short path[] = L"\\??\\A:\\";
	NTSTATUS Status;
	UNICODE_STRING us;
	ULONGLONG tm, time;

	DebugPrint("-Ultradfg- ----- Analyse of %c: -----\n",NULL,dx->letter);
	
	/* Initialization */
	MarkAllSpaceAsSystem0(dx);
	FreeAllBuffers(dx);
	InitDX(dx);

	/* Volume space analysis */
	Status = OpenVolume(dx);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- OpenVolume() failed: %x!\n",NULL,(UINT)Status);
		return Status;
	}

	/* update map representation */
	MarkAllSpaceAsSystem1(dx);

	tm = _rdtsc();
	dx->clusters_to_process = dx->clusters_total;
	dx->processed_clusters = 0;
	Status = FillFreeSpaceMap(dx);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- FillFreeSpaceMap() failed: %x!\n",NULL,(UINT)Status);
		return Status;
	}
	//DbgPrint("FillFreeSpaceMap() time: %I64u mln.\n",(_rdtsc() - tm) / 1000000);

	if(dx->partition_type != NTFS_PARTITION){
		/* Find files */
		path[4] = (short)dx->letter;
		if(!RtlCreateUnicodeString(&us,path)){
			DebugPrint("-Ultradfg- no enough memory for path initialization!\n",NULL);
			return STATUS_NO_MEMORY;
		}
		if(!FindFiles(dx,&us)){
			RtlFreeUnicodeString(&us);
			DebugPrint("-Ultradfg- FindFiles() failed!\n",NULL);
			return STATUS_NO_MORE_FILES;
		}
		RtlFreeUnicodeString(&us);
		time = _rdtsc() - tm;
		DbgPrint("An universal scan needs %I64u ms\n",time);
	} else {
		if(KeReadStateEvent(&stop_event) == 0x0){
			/* Get MFT location */
			Status = ProcessMFT(dx);
			if(!NT_SUCCESS(Status)){
				DebugPrint("-Ultradfg- ProcessMFT() failed!\n",NULL);
				return Status;
			}
			/* Experimental support of retrieving information directly from MFT. */
			/* Don't forget ProcessMFT() before! */
			ScanMFT(dx);
		}
	}

	DebugPrint("-Ultradfg- Files found: %u\n",NULL,dx->filecounter);
	DebugPrint("-Ultradfg- Fragmented files: %u\n",NULL,dx->fragmfilecounter);

	GenerateFragmentedFilesList(dx);

	/* Save state */
	//ApplyFilter(dx);
	return STATUS_SUCCESS;
}

void GenerateFragmentedFilesList(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFILENAME pfn;

	for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
		if(pfn->is_fragm && !pfn->is_filtered && !pfn->is_reparse_point)
			InsertFragmentedFile(dx,pfn);
		if(pfn->next_ptr == dx->filelist) break;
	}
}
