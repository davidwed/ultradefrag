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
* NTFS is analysed directly through reading
* MFT records, because this highly (25 times)
* speed up an analysis. For FAT we have noticed
* no speedup (even slowdown) while trying
* to walk trough FAT entries. This is because
* Windows file cache makes access even faster.
* UDF has been never tested in direct mode
* because of its highly complicated standard.
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
	winx_volume_information v;
	
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

	/* get volume information */
	if(winx_get_volume_information(volume_letter,&v) < 0){
		return (-1);
	} else {
		/* update global variables holding drive geometry */
		sectors_per_cluster = v.sectors_per_cluster;
		bytes_per_cluster = v.bytes_per_cluster;
		bytes_per_sector = v.bytes_per_sector;
		Stat.total_space = v.total_bytes;
		Stat.free_space = v.free_bytes;
		clusters_total = v.total_clusters;
		if(bytes_per_cluster) clusters_per_256k = _256K / bytes_per_cluster;
		else clusters_per_256k = 0;
		DebugPrint("Total clusters: %I64u\n",clusters_total);
		DebugPrint("Cluster size: %I64u\n",bytes_per_cluster);
		if(!clusters_per_256k){
			DebugPrint("Clusters are larger than 256 kbytes!\n");
			clusters_per_256k ++;
		}
		/* validate geometry */
		if(!clusters_total || !bytes_per_cluster){
			DebugPrint("Wrong volume geometry!");
			return (-1);
		}
		/* check partition type */
		DebugPrint("%s partition detected!\n",v.fs_name);
		if(!strcmp(v.fs_name,"NTFS")){
			partition_type = NTFS_PARTITION;
		} else if(!strcmp(v.fs_name,"FAT12")){
			partition_type = FAT12_PARTITION;
		} else if(!strcmp(v.fs_name,"FAT16")){
			partition_type = FAT16_PARTITION;
		} else if(!strcmp(v.fs_name,"FAT32")){
			/* check FAT32 version */
			if(v.fat32_mj_version > 0 || v.fat32_mn_version > 0){
				DebugPrint("Cannot recognize FAT32 version %u.%u!\n",
					v.fat32_mj_version,v.fat32_mn_version);
				/* for safe low level access in future releases */
				partition_type = FAT32_UNRECOGNIZED_PARTITION;
			} else {
				partition_type = FAT32_PARTITION;
			}
		} else {
			DebugPrint("File system type is not recognized.\n");
			DebugPrint("Type independent routines will be used to defragment it.\n");
			partition_type = UNKNOWN_PARTITION;
		}
	}
	
	/* update map representation */
	MarkAllSpaceAsSystem1();
	
	/* define whether some actions are allowed or not */
	switch(partition_type){
        case FAT12_PARTITION:
        case FAT16_PARTITION:
        case FAT32_PARTITION:
        case FAT32_UNRECOGNIZED_PARTITION:
            AllowDirDefrag = FALSE;
            AllowOptimize = FALSE;
            break;
        case NTFS_PARTITION:
            AllowDirDefrag = TRUE;
            AllowOptimize = TRUE;
            break;
        default:
            AllowDirDefrag = FALSE;
            AllowOptimize = FALSE;
            /* AllowDirDefrag = TRUE;
            AllowOptimize = TRUE; */
            break;
	}
	if(AllowDirDefrag) DebugPrint("Directory defragmentation is allowed.\n");
	else DebugPrint("Directory defragmentation is denied (because not possible).\n");
    
	if(AllowOptimize) DebugPrint("Volume optimization is allowed.\n");
	else DebugPrint("Volume optimization is denied (because not possible).\n");
	
	/* update progress counters */
	tm = winx_xtime();
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
			* NTFS formatted volumes cannot be safely accessed
			* in NT4, as mentioned in program's handbook.
			* On our test machines NTFS access caused blue
			* screens regardless of whether we accessed it
			* from kernel mode or user mode.
			* Though, this fact should be tested again,
			* because there was lots of improvements in code
			* since time of the tests.
			*/
            (void)ScanMFT();
            break;
        default:
            /* Find files */
            tm = winx_xtime();
            path[4] = (short)volume_letter;
            error_code = FindFiles(path);
            if(error_code < 0){
                DebugPrint("FindFiles() failed!\n");
                return error_code;
            }
            time = winx_xtime() - tm;
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
	tm = winx_xtime();

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

	time = winx_xtime() - tm;
	DebugPrint("Well known locked files search completed in %I64u ms.\n", time);
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

/** @} */
