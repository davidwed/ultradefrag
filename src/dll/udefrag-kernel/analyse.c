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
* User mode driver - fragmentation analysis engine.
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

#include "globals.h"

int Analyze(char *volume_name)
{
	short path[] = L"\\??\\A:\\";
	NTSTATUS Status;
	ULONGLONG tm, time;
	PFILENAME pfn;
	HANDLE hFile;
	
	DebugPrint("----- Analyze of %s: -----\n",volume_name);
	
	/* initialize cluster map */
	MarkAllSpaceAsSystem0();
	
	/* reset file lists */
	DestroyLists();
	
	/* reset statistics */
	memset(&Stat,0,sizeof(STATISTIC));
	Stat.current_operation = 'A';
	
	/* open the volume */
	if(OpenVolume(volume_name) < 0) return (-1);

	/* get drive geometry */
	if(GetDriveGeometry(volume_name) < 0) return (-1);
//DebugPrint("fuck!\n");
	/* update map representation */
	MarkAllSpaceAsSystem1();
	
	/* define file system */
	CheckForNtfsPartition();
	
	/* update progress counters */
	tm = _rdtsc();
	Stat.clusters_to_process = clusters_total;
	Stat.processed_clusters = 0;
	
	/* scan volume for free space areas */
	Status = FillFreeSpaceMap();
	if(!NT_SUCCESS(Status)){
		winx_raise_error("E: FillFreeSpaceMap() failed: %x!\n",(UINT)Status);
		return (-1);
	}
	//DebugPrint("FillFreeSpaceMap() time: %I64u mln.\n",(_rdtsc() - tm) / 1000000);

	if(CheckForStopEvent()) return 0;

	switch(partition_type){
	case NTFS_PARTITION:
		/* Experimental support of retrieving information directly from MFT. */
		ScanMFT();
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
		path[4] = (short)volume_letter;
		if(!FindFiles(path)){
			DebugPrint("FindFiles() failed!\n");
			return (-1);//STATUS_NO_MORE_FILES;
		}
		time = _rdtsc() - tm;
		DebugPrint("An universal scan needs %I64u ms\n",time);
	}
	
	///if(dx->partition_type == FAT16_PARTITION) ScanFat16Partition(dx);

	DebugPrint("Files found: %u\n",Stat.filecounter);
	DebugPrint("Fragmented files: %u\n",Stat.fragmfilecounter);

	GenerateFragmentedFilesList();
	
	/* all locked files are in unknown state, right? */
	for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
		if(CheckForStopEvent()) break;
		Status = OpenTheFile(pfn,&hFile);
		if(Status != STATUS_SUCCESS){
			DebugPrint("Can't open %ws file: %x\n",pfn->name.Buffer,(UINT)Status);
			/* we need to destroy the block map to avoid infinite loops */
			DeleteBlockmap(pfn); /* file is locked by other application, so its state is unknown */
		} else {
			NtCloseSafe(hFile);
		}
		if(pfn->next_ptr == filelist) break;
	}

	/* Save state */
	//ApplyFilter(dx);
	return 0;
}

int AnalyzeFreeSpace(char *volume_name)
{
	NTSTATUS Status;

	DebugPrint("----- Analyze free space of %s: -----\n",volume_name);
	
	CloseVolume();
	DestroyList((PLIST *)(void *)&free_space_map);

	/* reopen the volume */
	if(OpenVolume(volume_name) < 0) return (-1);
	/* scan volume for free space areas */
	Status = FillFreeSpaceMap();
	if(!NT_SUCCESS(Status)){
		winx_raise_error("E: FillFreeSpaceMap() failed: %x!\n",(UINT)Status);
		return (-1);
	}
	return 0;
}

void GenerateFragmentedFilesList(void)
{
	PFILENAME pfn;

	for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
		if(pfn->is_fragm && !pfn->is_filtered && !pfn->is_reparse_point)
			InsertFragmentedFile(pfn);
		if(pfn->next_ptr == filelist) break;
	}
}

/* in milliseconds */
ULONGLONG _rdtsc(void)
{
	return _rdtsc_1() / 1000 / 1000;
}

/* in nanoseconds */
ULONGLONG _rdtsc_1(void)
{
	NTSTATUS Status;
	LARGE_INTEGER frequency;
	LARGE_INTEGER counter;
	
	Status = NtQueryPerformanceCounter(&counter,&frequency);
	if(!NT_SUCCESS(Status)){
		DebugPrint("NtQueryPerformanceCounter() failed: %x!\n",(UINT)Status);
		return 0;
	}
	if(!frequency.QuadPart){
		DebugPrint("Your hardware has no support for High Resolution timer!\n");
		return 0;
	}
	return ((1000 * 1000 * 1000) / frequency.QuadPart) * counter.QuadPart;
}
