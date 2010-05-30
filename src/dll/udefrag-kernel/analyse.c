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
	MarkAllSpaceAsFree0();
	
	/* reset file lists */
	DestroyLists();
	
	/* reset statistics */
	pass_number = Stat.pass_number;
	memset(&Stat,0,sizeof(STATISTIC));
	Stat.current_operation = 'A';
	Stat.pass_number = pass_number;

	/* reset coordinates of mft zones */
	mft_start = 0, mft_end = 0, mftzone_start = 0, mftzone_end = 0;
	mftmirr_start = 0, mftmirr_end = 0;
	
	/* open the volume */
	error_code = OpenVolume(volume_name);
	if(error_code < 0) return error_code;
	
	/* flush all file buffers */
	if(initial_analysis){
		DebugPrint("Flush all file buffers!\n");
		FlushAllFileBuffers(volume_name);
	}

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
	
	/* remark space belonging to well known locked files as system */
	RemarkWellKnownLockedFiles();

	GenerateFragmentedFilesList();
	return 0;
}

/**
 * @brief Performs a volume analysis even 
 * when stop signal already received.
 * @param[in] volume_name the name of the volume.
 * @return Zero for success, negative value otherwise.
 */
int AnalyzeForced(char *volume_name)
{
	BOOLEAN stop_event_signaled;
	int result;

	stop_event_signaled = CheckForStopEvent();
	(void)NtClearEvent(hStopEvent);
	result = Analyze(volume_name);
	if(stop_event_signaled) (void)NtSetEvent(hStopEvent,NULL);
	return result;
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
	
	/* locked file found, remark space belonging to it! */
	RemarkFileSpaceAsSystem(pfn);

	/* file is locked by other application, so its state is unknown */
	DeleteBlockmap(pfn);
	return TRUE;
}

/*
* Minimal name lengths!
* Lowercase!
*/
#if 0
short *known_locked_files[] = {
	L"pagefile.sys",
	L"hiberfil.sys",
	L"ntuser.dat",
	/*L"system vol",*/ /* system volume information gives a lot of false detection */
	/*L"em32\\config",*/ /* the same cause */
	NULL
};
#endif

/**
 * @brief Checks whether the file is $MFT or not.
 * @param[in] pfn pointer to structure describing the file.
 * @return Boolean value indicating whether the file is $MFT or not.
 * @note Optimized for speed.
 */
BOOLEAN IsMft(PFILENAME pfn)
{
	short mft_path[] = L"\\??\\C:\\$MFT";
	UNICODE_STRING us;
	BOOLEAN result;
	
	if(pfn->name.Length < 9 * sizeof(short)) /* ensure that we have at least \??\X:\$x */
		return FALSE;
	if(pfn->name.Buffer[7] != '$')
		return FALSE;

	mft_path[4] = (short)volume_letter;
	(void)_wcslwr(mft_path);

	if(!RtlCreateUnicodeString(&us,pfn->name.Buffer)){
		DebugPrint("Cannot allocate memory for IsMft()!\n");
		out_of_memory_condition_counter ++;
		return FALSE;
	}
	(void)_wcslwr(us.Buffer);

	result = (wcscmp(us.Buffer,mft_path) == 0) ? TRUE : FALSE;
	
	RtlFreeUnicodeString(&us);
	return result;
}

/**
 * @brief Checks is file a well known system file or not.
 * @param[in] pfn pointer to structure describing the file.
 * @return Boolean value indicating is file
 * a well known system file or not.
 * @note Optimized for speed.
 */
BOOLEAN IsWellKnownSystemFile(PFILENAME pfn)
{
	UNICODE_STRING us;
	/*int i;*/
	short *pos;
	short config_sign[] = L"\\system32\\config\\";
	
	if(!RtlCreateUnicodeString(&us,pfn->name.Buffer)){
		DebugPrint("Cannot allocate memory for IsWellKnownSystemFile()!\n");
		out_of_memory_condition_counter ++;
		return FALSE;
	}
	(void)_wcslwr(us.Buffer);
	
	/* search for NTFS internal files */
	if(us.Length >= 9 * sizeof(short)){ /* to ensure that we have at least \??\X:\$x */
		/* skip \??\X:\$mft */
		if(us.Buffer[7] == '$' && us.Buffer[8] == 'm'){
			if(IsMft(pfn)) goto not_found;
		}
		if(us.Buffer[7] == '$') goto found;
	}
	
	/* search for big files usually locked by Windows */
	/*for(i = 0;; i++){
		if(known_locked_files[i] == NULL) goto not_found;
		if(wcsstr(us.Buffer,known_locked_files[i])) goto found;
	}*/
	if(wcsstr(us.Buffer,L"pagefile.sys")) goto found;
	if(wcsstr(us.Buffer,L"hiberfil.sys")) goto found;
	if(wcsstr(us.Buffer,L"ntuser.dat")) goto found;
	pos = wcsstr(us.Buffer,config_sign);
	if(pos){
		/* skip sub directories */
		pos += (sizeof(config_sign) / sizeof(short)) - 1;
		if(!wcschr(pos,'\\') && !pfn->is_dir) goto found;
	}
	
	/*
	* Other system files will be detected during the defragmentation,
	* because otherwise this will take a lot of time.
	*/
	goto not_found;

found:
	RtlFreeUnicodeString(&us);
	return TRUE;

not_found:
	RtlFreeUnicodeString(&us);
	return FALSE;
}

/**
 * @brief Remarks space belonging to well known
 * locked files as system.
 * @todo Speedup desired.
 */
void RemarkWellKnownLockedFiles(void)
{
	ULONGLONG tm, time;
	PFILENAME pfn;
	NTSTATUS Status;
	HANDLE hFile;

	DebugPrint("Well known locked files search started...\n");
	tm = _rdtsc();

	for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
		if(pfn->is_reparse_point == FALSE && pfn->blockmap){
			if(IsWellKnownSystemFile(pfn)){
				Status = OpenTheFile(pfn,&hFile);
				if(Status == STATUS_SUCCESS){
					NtCloseSafe(hFile);
					/* possibility of this case must be reduced */
					DebugPrint("False detection %ws!\n",pfn->name.Buffer);
				} else {
					DebugPrintEx(Status,"Cannot open %ws",pfn->name.Buffer);
					
					/* locked file found, remark space belonging to it! */
					RemarkFileSpaceAsSystem(pfn);
					
					/* file is locked by other application, so its state is unknown */
					DeleteBlockmap(pfn);
				}
			}
		}
		if(pfn->next_ptr == filelist) break;
	}

	time = _rdtsc() - tm;
	DebugPrint("Well known locked files search completed in %I64u ms.\n", time);
}

#if 0
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
#endif

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
