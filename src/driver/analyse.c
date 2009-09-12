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

/*
* NOTE: the following file systems will be analysed 
* through reading their structures directly from disk
* and interpreting them: NTFS, FAT12, FAT16, FAT32.
* This cool idea was suggested by Parvez Reza (parvez_ku_00@yahoo.com).
*
* UDF filesystem is missing in this list, because its
* standard is too complicated (http://www.osta.org/specs/).
* Therefore it will be analysed through standard Windows API.
*
* Why a special code?
* It works faster:
* NTFS ultra fast scan may be 25 times faster than universal scan,
* FAT16 scan may be 40% faster.
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
	return Nt_KeQueryInterruptTime() * 100;
}
#endif

/* initialize files list and free space map */
NTSTATUS Analyse(UDEFRAG_DEVICE_EXTENSION *dx)
{
	short path[] = L"\\??\\A:\\";
	NTSTATUS Status;
	ULONGLONG tm, time;

	DebugPrint("-Ultradfg- ----- Analyse of %c: -----\n",dx->letter);
	
	/* Initialization */
	MarkAllSpaceAsSystem0(dx);
	FreeAllBuffers(dx);
	InitDX(dx);

	/* Volume space analysis */
	Status = OpenVolume(dx);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- OpenVolume() failed: %x!\n",(UINT)Status);
		return Status;
	}

	/* update map representation */
	MarkAllSpaceAsSystem1(dx);

	tm = _rdtsc();
	dx->clusters_to_process = dx->clusters_total;
	dx->processed_clusters = 0;
	Status = FillFreeSpaceMap(dx);
	if(!NT_SUCCESS(Status)){
		DebugPrint("-Ultradfg- FillFreeSpaceMap() failed: %x!\n",(UINT)Status);
		return Status;
	}
	//DbgPrint("FillFreeSpaceMap() time: %I64u mln.\n",(_rdtsc() - tm) / 1000000);

	if(KeReadStateEvent(&stop_event) == 0x1) return STATUS_SUCCESS;

	switch(dx->partition_type){
	case NTFS_PARTITION:
		/* Experimental support of retrieving information directly from MFT. */
		ScanMFT(dx);
		break;
	/*case FAT12_PARTITION:
		ScanFat12Partition(dx);
		break;
	case FAT16_PARTITION:
		ScanFat16Partition(dx);
		break;
	case FAT32_PARTITION:
		ScanFat32Partition(dx);
		break;*/
	default: /* UDF, Ext2 and so on... */
		/* Find files */
		tm = _rdtsc();
		path[4] = (short)dx->letter;
		if(!FindFiles(dx,path)){
			DebugPrint("-Ultradfg- FindFiles() failed!\n");
			return STATUS_NO_MORE_FILES;
		}
		time = _rdtsc() - tm;
		DbgPrint("An universal scan needs %I64u ms\n",time);
	}
	
	///if(dx->partition_type == FAT16_PARTITION) ScanFat16Partition(dx);

	DebugPrint("-Ultradfg- Files found: %u\n",dx->filecounter);
	DebugPrint("-Ultradfg- Fragmented files: %u\n",dx->fragmfilecounter);

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
