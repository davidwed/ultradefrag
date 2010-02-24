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
 * @file analyse.c
 * @brief Volume analysis code.
 * @addtogroup Analysis
 * @{
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
* - Why a special code?
* - It works faster:
*   * NTFS ultra fast scan may be 25 times faster than universal scan
*   * FAT16 scan may be 40% faster.
*
* P.S.: We decided to avoid direct FAT scan in this driver,
* because it works slower than the universal scan. Due to the
* file system caching Windows feature.
*/

#include "globals.h"

/**
 * @brief Performs a volume analysis.
 * @param[in] volume_name the name of the volume.
 * @return Zero for success, negative value otherwise.
 * @bug Crashes the NT 4.0 system during the NTFS v1.2 
 * volumes analysis.
 * @todo Remove a system files checking which takes
 * a lot of time.
 */
int Analyze(char *volume_name)
{
	unsigned short path[] = L"\\??\\A:\\";
	NTSTATUS Status;
	ULONGLONG tm, time;
	ULONG pass_number;
	int error_code;
	
	DebugPrint("----- Analyze of %s: -----\n",volume_name);
	
	/* initialize cluster map */
	MarkAllSpaceAsSystem0();
	
	/* reset file lists */
	DestroyLists();
	
	/* reset statistics */
	pass_number = Stat.pass_number;
	memset(&Stat,0,sizeof(STATISTIC));
	Stat.current_operation = 'A';
	Stat.pass_number = pass_number;
	
	/* open the volume */
	error_code = OpenVolume(volume_name);
	if(error_code < 0) return error_code;

	/* get drive geometry */
	error_code = GetDriveGeometry(volume_name);
	if(error_code < 0) return error_code;

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
		DebugPrintEx(Status,"FillFreeSpaceMap() failed");
		if(Status == STATUS_NO_MEMORY) return UDEFRAG_NO_MEM;
		return (-1);
	}
	//DebugPrint("FillFreeSpaceMap() time: %I64u mln.\n",(_rdtsc() - tm) / 1000000);

	if(CheckForStopEvent()) return 0;

	switch(partition_type){
	case NTFS_PARTITION:
		/*
		* ScanMFT() causes BSOD on NT 4.0, even in user mode!
		* It seems that NTFS driver is imperfect in this system
		* and ScanMFT() breaks something inside it.
		* Sometimes BSOD appears during an analysis after volume optimization,
		* sometimes it appears immediately during the first analysis.
		* Examples:
		* 1. kmd compact ntfs nt4 -> BSOD 0xa (0x48, 0xFF, 0x0, 0x80101D5D)
		* immediately after the last analysis
		* 2. user mode - optimize - BSOD during the second analysis
		* 0x50 (0xB51CA820,0,0,2) or 0x1E (0xC...5,0x8013A7B6,0,0x20)
		* 3. kmd - nt4 - gui - optimize - chkdsk /F for ntfs - bsod
		*/
		/*if(nt4_system){
			DebugPrint("Ultrafast NTFS scan is not avilable on NT 4.0\n");
			DebugPrint("due to ntfs.sys driver imperfectness.\n");
			goto universal_scan;
		}*/
		(void)ScanMFT();
		break;
	/*case FAT12_PARTITION:
		(void)ScanFat12Partition();
		break;
	case FAT16_PARTITION:
		(void)ScanFat16Partition();
		break;
	case FAT32_PARTITION:
		(void)ScanFat32Partition();
		break;*/
	default: /* UDF, Ext2 and so on... */
/*universal_scan:*/
		/* Find files */
		tm = _rdtsc();
		path[4] = (short)volume_letter;
		error_code = FindFiles(path);
		if(error_code < 0){
			DebugPrint("FindFiles() failed!\n");
			return error_code;
		}
		time = _rdtsc() - tm;
		DebugPrint("An universal scan needs %I64u ms\n",time);
	}
	
	DebugPrint("Files found: %u\n",Stat.filecounter);
	DebugPrint("Fragmented files: %u\n",Stat.fragmfilecounter);

	GenerateFragmentedFilesList();
	return 0;
}

/**
 * @brief Checks is file locked or not.
 * @param[in,out] pfn pointer to structure
 * describing the file.
 * @return Boolean value indicating is file
 * locked or not.
 * @note 
 * - Side effect: pfn->blockmap becomes
 * to be equal to NULL if the file is locked.
 * - Kalle Koseck (http://dosbatchsubs.sourceforge.net/)
 * suggested me to move all file checking to defragmenter/optimizer
 * to dramatically speed up an analysis. Thus this function born.
 */
BOOLEAN IsFileLocked(PFILENAME pfn)
{
	NTSTATUS Status;
	HANDLE hFile;

	/* is the file already checked? */
	if(pfn->blockmap == NULL) return TRUE;

	/* check the file */
	Status = OpenTheFile(pfn,&hFile);
	if(Status == STATUS_SUCCESS){
		NtCloseSafe(hFile);
		return FALSE;
	}

	DebugPrintEx(Status,"Cannot open %ws",pfn->name.Buffer);
	
	/* file is locked by other application, so its state is unknown */
	DeleteBlockmap(pfn);
	return TRUE;
}

/**
 * @brief Checks all fragmented files,
 * are they locked or not.
 * @note Works slow.
 */
void CheckAllFragmentedFiles(void)
{
	ULONGLONG tm, time;
	PFRAGMENTED pf;

	DebugPrint("+-------------------------------------------------------+\n");
	DebugPrint("|          Fragmented files checking started...         |\n");
	DebugPrint("+-------------------------------------------------------+\n");
	DebugPrint("UltraDefrag will try to open them to ensure that they are not locked.\n");
	DebugPrint("This may take few minutes if there are many fragmented files on the disk.\n");
	DebugPrint("\n");
	tm = _rdtsc();

	for(pf = fragmfileslist; pf != NULL; pf = pf->next_ptr){
		(void)IsFileLocked(pf->pfn);
		if(pf->next_ptr == fragmfileslist) break;
	}

	time = _rdtsc() - tm;
	DebugPrint("---------------------------------------------------------\n");
	DebugPrint("Fragmented files checking completed in %I64u ms.\n",  time);
	DebugPrint("---------------------------------------------------------\n");
}

/**
 * @brief Checks all files,
 * are they locked or not.
 * @note Works slow.
 */
void CheckAllFiles(void)
{
	ULONGLONG tm, time;
	PFILENAME pfn;

	DebugPrint("+-------------------------------------------------------+\n");
	DebugPrint("|               Files checking started...               |\n");
	DebugPrint("+-------------------------------------------------------+\n");
	DebugPrint("UltraDefrag will try to open them to ensure that they are not locked.\n");
	DebugPrint("This may take few minutes if there are many files on the disk.\n");
	DebugPrint("\n");
	tm = _rdtsc();

	for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
		(void)IsFileLocked(pfn);
		if(pfn->next_ptr == filelist) break;
	}

	time = _rdtsc() - tm;
	DebugPrint("---------------------------------------------------------\n");
	DebugPrint("Files checking completed in %I64u ms.\n",  time);
	DebugPrint("---------------------------------------------------------\n");
}

/**
 * @brief Produces a list of fragmented files.
 */
void GenerateFragmentedFilesList(void)
{
	PFILENAME pfn;

	for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
		if(pfn->is_fragm && /* !pfn->is_filtered && */!pfn->is_reparse_point)
			AddFileToFragmented(pfn);
		if(pfn->next_ptr == filelist) break;
	}
}

/**
 * @brief Returns the current time in milliseconds since...
 * @return Time, in milliseconds.
 * @note
 * - Useful for the performance measures.
 * - Has no physical meaning.
 */
ULONGLONG _rdtsc(void)
{
	return _rdtsc_1() / 1000 / 1000;
}

/**
 * @brief Returns the current time in nanoseconds since...
 * @return Time, in nanoseconds. Zero indicates failure.
 * @note
 * - Useful for the performance measures.
 * - Has no physical meaning.
 */
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

/** @} */
