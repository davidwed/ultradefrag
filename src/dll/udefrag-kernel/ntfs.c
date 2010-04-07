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
 * @file ntfs.c
 * @brief Fast NTFS analysis code.
 * @addtogroup NTFS
 * @{
 */

#include "globals.h"
#include "ntfs.h"

/*
* Add data consistency checks everywhere,
* because NTFS volumes often have invalid
* data in MFT entries.
*/

/*
* FIXME: 
* 1. Volume ditry flag?
*/

/*---------------------------------- NTFS related code -------------------------------------*/

UINT MftScanDirection = MFT_SCAN_RTL;
BOOLEAN MaxMftEntriesNumberUpdated = FALSE;
int number_of_processed_attr_list_entries = 0;

/**
 * @brief Retrieves a type of the file system 
 *        containing on the volume.
 * @details Sets the global partition_type variable.
 */
void CheckForNtfsPartition(void)
{
	PARTITION_INFORMATION *part_info;
	NTFS_DATA *ntfs_data;
	NTSTATUS status;
	IO_STATUS_BLOCK iosb;

	/* allocate memory */
	part_info = winx_heap_alloc(sizeof(PARTITION_INFORMATION));
	if(part_info == NULL){
		DebugPrint("Cannot allocate memory for CheckForNtfsPartition()!\n");
		out_of_memory_condition_counter ++;
		partition_type = FAT32_PARTITION;
		return;
	}
	ntfs_data = winx_heap_alloc(sizeof(NTFS_DATA));
	if(ntfs_data == NULL){
		winx_heap_free(part_info);
		DebugPrint("Cannot allocate memory for CheckForNtfsPartition()!\n");
		out_of_memory_condition_counter ++;
		partition_type = FAT32_PARTITION;
		return;
	}
	
	/*
	* Try to get fs type through IOCTL_DISK_GET_PARTITION_INFO request.
	* Works only on MBR-formatted disks. To retrieve information about 
	* GPT-formatted disks use IOCTL_DISK_GET_PARTITION_INFO_EX.
	*/
	RtlZeroMemory(part_info,sizeof(PARTITION_INFORMATION));
	status = NtDeviceIoControlFile(winx_fileno(fVolume),NULL,NULL,NULL,&iosb, \
				IOCTL_DISK_GET_PARTITION_INFO,NULL,0, \
				part_info, sizeof(PARTITION_INFORMATION));
	if(NT_SUCCESS(status)/* == STATUS_PENDING*/){
		(void)NtWaitForSingleObject(winx_fileno(fVolume),FALSE,NULL);
		status = iosb.Status;
	}
	if(NT_SUCCESS(status)){
		DebugPrint("Partition type: %u\n",part_info->PartitionType);
		if(part_info->PartitionType == 0x7){
			DebugPrint("NTFS found\n");
			partition_type = NTFS_PARTITION;
			winx_heap_free(part_info);
			winx_heap_free(ntfs_data);
			return;
		}
	} else {
		DebugPrint("Can't get fs info: %x!\n",(UINT)status);
	}

	/*
	* To ensure that we have a NTFS formatted partition
	* on GPT disks and when partition type is 0x27
	* FSCTL_GET_NTFS_VOLUME_DATA request can be used.
	*/
	RtlZeroMemory(ntfs_data,sizeof(NTFS_DATA));
	status = NtFsControlFile(winx_fileno(fVolume),NULL,NULL,NULL,&iosb, \
				FSCTL_GET_NTFS_VOLUME_DATA,NULL,0, \
				ntfs_data, sizeof(NTFS_DATA));
	if(NT_SUCCESS(status)/* == STATUS_PENDING*/){
		(void)NtWaitForSingleObject(winx_fileno(fVolume),FALSE,NULL);
		status = iosb.Status;
	}
	if(NT_SUCCESS(status)){
		DebugPrint("NTFS found\n");
		partition_type = NTFS_PARTITION;
		winx_heap_free(part_info);
		winx_heap_free(ntfs_data);
		return;
	} else {
		DebugPrint("Can't get ntfs info: %x!\n",status);
	}

	/*
	* Let's assume that we have a FAT32 formatted partition.
	* Currently we don't need more detailed information.
	*/
	partition_type = FAT32_PARTITION;
	DebugPrint("NTFS not found\n");
	winx_heap_free(part_info);
	winx_heap_free(ntfs_data);
}

/**
 * @brief Retrieves some basic information about MFT.
 * @return An appropriate NTSTATUS code.
 */
NTSTATUS GetMftLayout(void)
{
	IO_STATUS_BLOCK iosb;
	NTFS_DATA *ntfs_data;
	NTSTATUS status;
	ULONGLONG mft_len;

	ntfs_data = winx_heap_alloc(sizeof(NTFS_DATA));
	if(ntfs_data == NULL){
		DebugPrint("Cannot allocate memory for GetMftLayout()!\n");
		out_of_memory_condition_counter ++;
		return STATUS_NO_MEMORY;
	}

	RtlZeroMemory(ntfs_data,sizeof(NTFS_DATA));
	status = NtFsControlFile(winx_fileno(fVolume),NULL,NULL,NULL,&iosb, \
				FSCTL_GET_NTFS_VOLUME_DATA,NULL,0, \
				ntfs_data, sizeof(NTFS_DATA));
	if(NT_SUCCESS(status)/* == STATUS_PENDING*/){
		(void)NtWaitForSingleObject(winx_fileno(fVolume),FALSE,NULL);
		status = iosb.Status;
	}
	if(!NT_SUCCESS(status)){
		DebugPrint("Cannot get ntfs info: %x!\n",status);
		winx_heap_free(ntfs_data);
		return status;
	}

	mft_len = ProcessMftSpace(ntfs_data);

	Stat.mft_size = mft_len * bytes_per_cluster;
	DebugPrint("MFT size = %I64u bytes\n",Stat.mft_size);

	ntfs_record_size = ntfs_data->BytesPerFileRecordSegment;
	DebugPrint("NTFS record size = %u bytes\n",ntfs_record_size);
	if(ntfs_record_size == 0){
		DebugPrint("NTFS record size is equal to zero!\n");
		winx_heap_free(ntfs_data);
		return STATUS_UNSUCCESSFUL;
	}
	
	max_mft_entries = Stat.mft_size / ntfs_record_size;
	DebugPrint("MFT contains no more than %I64u records\n",
		max_mft_entries);
	
	//DbgPrintFreeSpaceList();
	winx_heap_free(ntfs_data);
	return STATUS_SUCCESS;
}

/**
 * @brief Scans the entire MFT retrieving information
 * about each file contained on the volume.
 * @return Boolean value. TRUE indicates success.
 */
BOOLEAN ScanMFT(void)
{
	PMY_FILE_INFORMATION pmfi;
	PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob = NULL;
	ULONG nfrob_size;
	ULONGLONG mft_id, ret_mft_id;
	NTSTATUS status;
	
	ULONGLONG tm, time, tm2, time2;
	
	DebugPrint("MFT scan started!\n");
	number_of_processed_attr_list_entries = 0;

	/* Get information about MFT */
	status = GetMftLayout();
	if(!NT_SUCCESS(status)){
		DebugPrint("ProcessMFT() failed!\n");
		DebugPrint("MFT scan finished!\n");
		return FALSE;
	}

	/* allocate memory for NTFS record */
	nfrob_size = sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + ntfs_record_size - 1;
	tm = _rdtsc();
	pnfrob = (PNTFS_FILE_RECORD_OUTPUT_BUFFER)winx_heap_alloc(nfrob_size);
	if(!pnfrob){
		DebugPrint("No enough memory for NTFS_FILE_RECORD_OUTPUT_BUFFER!\n");
		out_of_memory_condition_counter ++;
		DebugPrint("MFT scan finished!\n");
		return FALSE;
	}
	
	/* allocate memory for MY_FILE_INFORMATION structure */
	pmfi = (PMY_FILE_INFORMATION)winx_heap_alloc(sizeof(MY_FILE_INFORMATION));
	if(!pmfi){
		DebugPrint("No enough memory for MY_FILE_INFORMATION structure!\n");
		out_of_memory_condition_counter ++;
		winx_heap_free(pnfrob);
		DebugPrint("MFT scan finished!\n");
		return FALSE;
	}
	
	/* read all MFT records sequentially */
	//MftBlockmap	= NULL;
	MaxMftEntriesNumberUpdated = FALSE;
	UpdateMaxMftEntriesNumber(pnfrob,nfrob_size);
	mft_id = max_mft_entries - 1;
	DebugPrint("\n");
	
	if(MaxMftEntriesNumberUpdated == FALSE){
		DebugPrint("UpdateMaxMftEntriesNumber() failed!\n");
		DebugPrint("MFT scan finished!\n");
		winx_heap_free(pnfrob);
		winx_heap_free(pmfi);
		//DestroyMftBlockmap();
		return FALSE;
	}

	DebugPrint("+-------------------------------------------------------+\n");
	DebugPrint("|          MFT records scanning loop begins...          |\n");
	DebugPrint("+-------------------------------------------------------+\n");
	tm2 = _rdtsc();
	/* Is MFT size an integral of NTFS record size? */
	/*if(MftClusters == 0 || \
	  (MftClusters * (ULONGLONG)dx->bytes_per_cluster % (ULONGLONG)dx->ntfs_record_size) || \
	  (dx->ntfs_record_size % dx->bytes_per_sector)){*/
	if(TRUE){
		MftScanDirection = MFT_SCAN_RTL;
		while(1){
			if(CheckForStopEvent()) break;
			status = GetMftRecord(pnfrob,nfrob_size,mft_id);
			if(!NT_SUCCESS(status)){
				if(mft_id == 0){
					DebugPrint("FSCTL_GET_NTFS_FILE_RECORD failed: %x!\n",status);
					winx_heap_free(pnfrob);
					winx_heap_free(pmfi);
					DebugPrint("MFT records scanning loop completed!\n");
					DebugPrint("MFT scan finished!\n");
					//DestroyMftBlockmap();
					return FALSE;
				}
				/* it returns 0xc000000d (invalid parameter) for non existing records */
				mft_id --; /* try to retrieve a previous record */
				continue;
			}
	
			/* analyse retrieved mft record */
			ret_mft_id = GetMftIdFromFRN(pnfrob->FileReferenceNumber);
			#ifdef DETAILED_LOGGING
			DebugPrint("NTFS record found, id = %I64u\n",ret_mft_id);
			#endif
			AnalyseMftRecord(pnfrob,nfrob_size,pmfi);
			
			/* go to the next record */
			if(ret_mft_id == 0 || mft_id == 0) break;
			if(ret_mft_id > mft_id){
				/* avoid infinite loops */
				DebugPrint("Returned MFT record ID is above expected!\n");
				mft_id --;
			} else {
				mft_id = ret_mft_id - 1;
			}
		}
	}/* else {
		DebugPrint("-Ultradfg- MFT size is an integral of NTFS record size :-D\n");
		ScanMftDirectly(dx,pnfrob,nfrob_size,pmfi);
	}*/
	
	/* free allocated memory */
	winx_heap_free(pnfrob);
	winx_heap_free(pmfi);
	//DestroyMftBlockmap();
	time2 = _rdtsc() - tm2;
	DebugPrint("MFT records scanning loop completed in %I64u ms.\n",time2);
	
	/* Build paths. */
	BuildPaths();

	DebugPrint("%u attribute lists entries totally processed.\n",
		number_of_processed_attr_list_entries);
	time = _rdtsc() - tm;
	DebugPrint("MFT scan completed in %I64u ms.\n",time);
	return TRUE;
}

/**
 * @brief Defines exactly how many entries has the MFT.
 * @param[in] pnfrob pointer to the buffer receiving a
 * single NTFS record for the internal computations.
 * @param[in] nfrob_size the size of the buffer, in bytes.
 */
void UpdateMaxMftEntriesNumber(PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,ULONG nfrob_size)
{
	NTSTATUS status;
	PFILE_RECORD_HEADER pfrh;

	/* Get record for FILE_MFT using WinAPI, because MftBitmap is not ready yet. */
	status = GetMftRecord(pnfrob,nfrob_size,FILE_MFT);
	if(!NT_SUCCESS(status)){
		DebugPrint("UpdateMaxMftEntriesNumber(): FSCTL_GET_NTFS_FILE_RECORD failed: %x!\n",status);
		return;
	}
	if(GetMftIdFromFRN(pnfrob->FileReferenceNumber) != FILE_MFT){
		DebugPrint("UpdateMaxMftEntriesNumber() failed - unable to get FILE_MFT record.\n");
		return;
	}

	/* Find DATA attribute. */
	pfrh = (PFILE_RECORD_HEADER)pnfrob->FileRecordBuffer;
	if(!IsFileRecord(pfrh)){
		DebugPrint("UpdateMaxMftEntriesNumber() failed - FILE_MFT record has invalid type %u.\n",
			pfrh->Ntfs.Type);
		return;
	}
	if(!(pfrh->Flags & 0x1)){
		DebugPrint("UpdateMaxMftEntriesNumber() failed - FILE_MFT record marked as free.\n");
		return; /* skip free records */
	}
	
	EnumerateAttributes(pfrh,UpdateMaxMftEntriesNumberCallback,NULL);
}

/**
 */
void __stdcall UpdateMaxMftEntriesNumberCallback(PATTRIBUTE pattr,PMY_FILE_INFORMATION pmfi)
{
	PNONRESIDENT_ATTRIBUTE pnr_attr;

	/* here pmfi == NULL always */
	(void)pmfi;
	
	if(pattr->Nonresident && pattr->AttributeType == AttributeData){
		pnr_attr = (PNONRESIDENT_ATTRIBUTE)pattr;
		if(ntfs_record_size){
			max_mft_entries = pnr_attr->DataSize / ntfs_record_size;
			Stat.mft_size = pnr_attr->DataSize;
			//MftClusters = 0;
			//MftBlockmap = NULL;
			//ProcessMFTRunList(dx,pnr_attr);
			/* FIXME: check correctness of MftBlockmap */
			MaxMftEntriesNumberUpdated = TRUE;
		}
		DebugPrint("MFT contains no more than %I64u records (more accurately)\n",
			max_mft_entries);
	}
}

/**
 * @brief Retrieves a single MFT record.
 * @param[out] pnfrob pointer to the buffer
 * receiving the MFT record.
 * @param[in] nfrob_size the size of the buffer, in bytes.
 * @param[in] mft_id the MFT record number.
 * @return An appropriate NTSTATUS code.
 */
NTSTATUS GetMftRecord(PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,
					  ULONG nfrob_size,ULONGLONG mft_id)
{
	NTFS_FILE_RECORD_INPUT_BUFFER nfrib;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;

	nfrib.FileReferenceNumber = mft_id;

	RtlZeroMemory(pnfrob,nfrob_size); /* required by x64 system, otherwise it trashes stack */
	status = NtFsControlFile(winx_fileno(fVolume),NULL,NULL,NULL,&iosb, \
			FSCTL_GET_NTFS_FILE_RECORD, \
			&nfrib,sizeof(nfrib), \
			pnfrob, nfrob_size);
	if(NT_SUCCESS(status)/* == STATUS_PENDING*/){
		(void)NtWaitForSingleObject(winx_fileno(fVolume),FALSE,NULL);
		status = iosb.Status;
	}
	return status;
}

/**
 * @brief Appends the file name to the attribute name.
 * @param[in,out] pfn pointer to structure
 * containing the attribute name.
 * @param[in] pmfi pointer to structure
 * containing the name of the file.
 * @return Zero for success, negative value otherwise.
 */
int UpdateAttributeName(PFILENAME pfn,PMY_FILE_INFORMATION pmfi)
{
	short *buffer;
	UNICODE_STRING us;

	/* if file name is empty, we should do nothing */
	if(pmfi->Name[0] == 0){
		DebugPrint("MftRecord has empty filename, MftId = %I64u, Parent MftId = %I64u\n",
			pmfi->BaseMftId,pmfi->ParentDirectoryMftId);
		return (-1);
	}

	/* if file name is not empty, append them */
	buffer = winx_heap_alloc(MAX_NTFS_PATH * sizeof(short));
	if(buffer == NULL){
		DebugPrint("Cannot allocate memory for buffer in UpdateAttributeName()!\n");
		out_of_memory_condition_counter ++;
		return (-1);
	}
	if(pfn->name.Buffer[0])
		(void)_snwprintf(buffer,MAX_NTFS_PATH,L"%s:%s",pmfi->Name,pfn->name.Buffer);
	else
		(void)wcsncpy(buffer,pmfi->Name,MAX_NTFS_PATH);
	buffer[MAX_NTFS_PATH - 1] = 0;

	if(!RtlCreateUnicodeString(&us,buffer)){
		DebugPrint("UpdateAttributeName: Cannot allocate memory for new name!\n");
		out_of_memory_condition_counter ++;
		winx_heap_free(buffer);
		return (-1);
	}

	RtlFreeUnicodeString(&(pfn->name));
	pfn->name.Buffer = us.Buffer;
	pfn->name.Length = us.Length;
	pfn->name.MaximumLength = us.MaximumLength;

	/*DebugPrint("UpdateAttributeName: %ws, MftId = %I64u, Parent MftId = %I64u\n",
		pfn->name.Buffer,pmfi->BaseMftId,pmfi->ParentDirectoryMftId);*/
	winx_heap_free(buffer);
	return 0;
}

/**
 * @brief Analyzes the MFT record.
 * @param[out] pnfrob pointer to the buffer
 * containing the MFT record.
 * @param[in] nfrob_size the size of the buffer, in bytes.
 * @param[out] pmfi pointer to the structure receiving
 * information about the file containing in MFT record.
 */
void AnalyseMftRecord(PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,
					  ULONG nfrob_size,PMY_FILE_INFORMATION pmfi)
{
	PFILE_RECORD_HEADER pfrh;
	#ifdef DETAILED_LOGGING
	USHORT Flags;
	#endif
	PFILENAME pfn, next_pfn, head;
	BOOLEAN DirectoryAdded;
	
	/* analyse record's header */
	pfrh = (PFILE_RECORD_HEADER)pnfrob->FileRecordBuffer;

	if(!IsFileRecord(pfrh)) return;
	if(!(pfrh->Flags & 0x1)) return; /* skip free records */
	
	/* analyse file record */
	#ifdef DETAILED_LOGGING
	Flags = pfrh->Flags;
	if(Flags & 0x2) DebugPrint("Directory\n"); /* May be wrong? */
	#endif

	if(pfrh->BaseFileRecord){ /* skip child records, we will analyse them later */
		//DebugPrint("-Ultradfg- BaseFileRecord (id) = %I64u\n",
		//	GetMftIdFromFRN(pfrh->BaseFileRecord));
		//DebugPrint("\n");
		return;
	}
	
	/* analyse attributes of MFT entry */
	pmfi->BaseMftId = GetMftIdFromFRN(pnfrob->FileReferenceNumber);
	pmfi->ParentDirectoryMftId = FILE_root;
	pmfi->Flags = 0x0;
	
	/* FIXME: L:\.:$SECURITY_DESCRIPTOR ?! */
	pmfi->IsDirectory = (pfrh->Flags & 0x2) ? TRUE : FALSE;
	
	pmfi->IsReparsePoint = FALSE;
	pmfi->NameType = 0x0; /* Assume FILENAME_POSIX */
	memset(pmfi->Name,0,MAX_NTFS_PATH);

	/* skip AttributeList attributes */
	EnumerateAttributes(pfrh,AnalyseAttributeCallback,pmfi);
	
	/* analyse AttributeList attributes */
	EnumerateAttributes(pfrh,AnalyseAttributeListCallback,pmfi);
	
	/* append the filename to attributes */
	head = filelist;
	for(pfn = filelist; pfn != NULL;){
		if(MftScanDirection == MFT_SCAN_RTL){
			if(pfn->BaseMftId > pmfi->BaseMftId) break; /* we have no chance to find record in list */
		} else {
			if(pfn->BaseMftId < pmfi->BaseMftId) break;
		}
		next_pfn = pfn->next_ptr;
		if(pfn->ParentDirectoryMftId == pmfi->ParentDirectoryMftId && \
		  pfn->BaseMftId == pmfi->BaseMftId){
			if(UpdateAttributeName(pfn,pmfi) < 0){
				winx_list_remove_item((list_entry **)(void *)&filelist,(list_entry *)pfn);
				if(filelist == NULL) break;
				if(filelist != head){
					head = filelist;
					pfn = next_pfn;
					continue;
				}
			}
		}
		pfn = next_pfn;
		if(pfn == head) break;
	}
	
	/* just for debugging */
	/*if(wcsistr(pmfi->Name,L"Scratch"))
		DebugPrint("@@@@@ %ws DIRECTORY FOUND, MFT_ID = %I64u, PARENT ID = %I64u\n",
			pmfi->Name,pmfi->BaseMftId,pmfi->ParentDirectoryMftId);*/

	/* add resident directories to filelist - required by BuildPaths() */
	if(pmfi->IsDirectory){
		/* is directory already added? */
		DirectoryAdded = FALSE;
		for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
			/*
			* we scan mft from the end to the beginning (RTL)
			* and we add new records to the left side of the file list...
			*/
			if(MftScanDirection == MFT_SCAN_RTL){
				if(pfn->BaseMftId > pmfi->BaseMftId) break; /* we have no chance to find record in list */
			} else {
				if(pfn->BaseMftId < pmfi->BaseMftId) break;
			}
			if(!wcscmp(pfn->name.Buffer,pmfi->Name) && \
			  (pfn->ParentDirectoryMftId == pmfi->ParentDirectoryMftId) && \
			  (pfn->BaseMftId == pmfi->BaseMftId))
				DirectoryAdded = TRUE;
			if(pfn->next_ptr == filelist) break;
		}
		if(!DirectoryAdded) AddResidentDirectoryToFileList(pmfi);
	}
	
	/* update cluster map and statistics */
	UpdateClusterMapAndStatistics(pmfi);

	#ifdef DETAILED_LOGGING
	DebugPrint("\n");
	#endif
}

/**
 * @brief Enumerates attributes contained in MFT record.
 * @param[in] pfrh pointer to the file record header.
 * @param[in] ahp pointer to the callback procedure
 * to be called once for each attribute found.
 * @param[out] pmfi pointer to the structure receiving
 * information about the file containing in MFT record.
 * @note The ntfs_record_size global variable must be
 * initialized before this call.
 */
void EnumerateAttributes(PFILE_RECORD_HEADER pfrh,
						 ATTRHANDLER_PROC ahp,PMY_FILE_INFORMATION pmfi)
{
	PATTRIBUTE pattr;
	ULONG attr_length;
	USHORT attr_offset;

	attr_offset = pfrh->AttributeOffset;
	pattr = (PATTRIBUTE)((char *)pfrh + attr_offset);

	while(pattr){
		if(CheckForStopEvent()) break;

		/* is an attribute header inside a record bounds? */
		if(attr_offset + sizeof(ATTRIBUTE) > pfrh->BytesInUse || \
			attr_offset + sizeof(ATTRIBUTE) > ntfs_record_size)	break;
		
		/* is it a valid attribute */
		if(pattr->AttributeType == 0xffffffff) break;
		if(pattr->AttributeType == 0x0) break;
		if(pattr->Length == 0) break;

		/* is an attribute inside a record bounds? */
		if(attr_offset + pattr->Length > pfrh->BytesInUse || \
			attr_offset + pattr->Length > ntfs_record_size) break;
		
		/* is an attribute length valid? */
		if(pattr->Nonresident){
			if(pattr->Length < (sizeof(NONRESIDENT_ATTRIBUTE) - sizeof(ULONGLONG))){
				DebugPrint("Nonresident attribute length is invalid!\n");
				break;
			}
		} else {
			if(pattr->Length < sizeof(RESIDENT_ATTRIBUTE)){
				DebugPrint("Resident attribute length is invalid!\n");
				break;
			}
		}

		/* call specified callback procedure */
		ahp(pattr,pmfi);
		
		/* go to the next attribute */
		attr_length = pattr->Length;
		attr_offset += (USHORT)(attr_length);
		pattr = (PATTRIBUTE)((char *)pattr + attr_length);
	}
}

/**
 */
void __stdcall AnalyseAttributeCallback(PATTRIBUTE pattr,PMY_FILE_INFORMATION pmfi)
{
	if(pattr->AttributeType != AttributeAttributeList)
		AnalyseAttribute(pattr,pmfi);
}

/**
 */
void __stdcall AnalyseAttributeListCallback(PATTRIBUTE pattr,PMY_FILE_INFORMATION pmfi)
{
	if(pattr->AttributeType == AttributeAttributeList)
		AnalyseAttribute(pattr,pmfi);
}

/**
 * @brief Analyzes a file attribute.
 */
void AnalyseAttribute(PATTRIBUTE pattr,PMY_FILE_INFORMATION pmfi)
{
	if(pattr->Nonresident) AnalyseNonResidentAttribute((PNONRESIDENT_ATTRIBUTE)pattr,pmfi);
	else AnalyseResidentAttribute((PRESIDENT_ATTRIBUTE)pattr,pmfi);
}

/**
 * @brief Analyzes a resident attribute of the file.
 * @param[in] pr_attr pointer to the structure
 * describing the resident attribute of the file.
 * @param[out] pmfi pointer to the structure receiving
 * information about the file containing in MFT record.
 * @note Resident attributes never contain fragmented data,
 * because they are placed in MFT record entirely.
 * We analyze resident attributes only if we need some
 * information stored in them.
 */
void AnalyseResidentAttribute(PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi)
{
	if(pr_attr->ValueOffset == 0 || pr_attr->ValueLength == 0){
		/*DebugPrint("AnalyseResidentAttribute: Invalid attribute, "
			"MFT ID = %I64u, Attribute Type = 0x%x, ValueOffset = %u, ValueLength = %u\n",
			pmfi->BaseMftId,(UINT)pr_attr->Attribute.AttributeType,
			(UINT)pr_attr->ValueOffset,(UINT)pr_attr->ValueLength);
		*/
		/*
		* This is an ordinary case when some attribute data was truncated.
		* For example, when some large file becomes empty its $DATA attribute
		* becomes resident and its ValueLength becomes to be equal to zero.
		*/
		return;
	}
	
	switch(pr_attr->Attribute.AttributeType){
	case AttributeStandardInformation: /* always resident */
		GetFileFlags(pr_attr,pmfi); break;

	case AttributeFileName: /* always resident */
		GetFileName(pr_attr,pmfi); break;

	case AttributeVolumeInformation: /* always resident */
		GetVolumeInformationData(pr_attr); break;

	case AttributeAttributeList:
		//DebugPrint("Resident AttributeList found!\n");
		AnalyseResidentAttributeList(pr_attr,pmfi);
		break;

	/*case AttributeIndexRoot:  // always resident */
	/*case AttributeIndexAllocation:*/

	case AttributeReparsePoint:
		CheckReparsePointResident(pr_attr,pmfi);
		break;

	default:
		break;
	}
}

/**
 * @brief Retrieves a flags of the file.
 * @param[in] pr_attr pointer to the structure
 * describing the resident attribute of the file.
 * @param[out] pmfi pointer to the structure receiving
 * information about the file.
 */
void GetFileFlags(PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi)
{
	PSTANDARD_INFORMATION psi;
	ULONG Flags;
	
	psi = (PSTANDARD_INFORMATION)((char *)pr_attr + pr_attr->ValueOffset);
	if(pr_attr->ValueLength < 48){ /* 48 = size of the shortest STANDARD_INFORMATION structure */
		DebugPrint("STANDARD_INFORMATION attribute is too short!\n");
	} else {
		Flags = psi->FileAttributes;
		pmfi->Flags = Flags;
	}
}

/**
 * @brief Retrieves a name of the file.
 * @param[in] pr_attr pointer to the structure
 * describing the resident attribute of the file.
 * @param[out] pmfi pointer to the structure receiving
 * information about the file.
 * @note The pmfi pointed structure MUST be initialized before!
 */
void GetFileName(PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi)
{
	PFILENAME_ATTRIBUTE pfn_attr;
	short *name;
	UCHAR name_type;
	ULONGLONG parent_mft_id;
	
	pfn_attr = (PFILENAME_ATTRIBUTE)((char *)pr_attr + pr_attr->ValueOffset);
	if(pr_attr->ValueLength < sizeof(FILENAME_ATTRIBUTE)){
		DebugPrint("FILENAME_ATTRIBUTE is too short!\n");
		return;
	}
	
	parent_mft_id = GetMftIdFromFRN(pfn_attr->DirectoryFileReferenceNumber);
	
	if(pfn_attr->NameLength){
		name = (short *)winx_heap_alloc((pfn_attr->NameLength + 1) * sizeof(short));
		if(!name){
			DebugPrint("Cannot allocate memory for GetFileName()!\n");
			out_of_memory_condition_counter ++;
			return;
		}
		(void)wcsncpy(name,pfn_attr->Name,pfn_attr->NameLength);
		name[pfn_attr->NameLength] = 0;
		//DbgPrint("-Ultradfg- Filename = %ws, parent id = %I64u\n",name,parent_mft_id);
		if(name[0] == 0) DebugPrint("Empty filename found ;)\n");
		if(parent_mft_id == pmfi->BaseMftId && pmfi->BaseMftId != FILE_root)
			DebugPrint("Recursion found - file identifies themselves as a parent ;)\n");
		if(name[0] && (parent_mft_id != pmfi->BaseMftId || pmfi->BaseMftId == FILE_root)){
			/* update pmfi members */
			pmfi->ParentDirectoryMftId = parent_mft_id;
			/* save filename */
			name_type = pfn_attr->NameType;
			UpdateFileName(pmfi,name,name_type);
		}
		winx_heap_free(name);
	} else DebugPrint("GetFileName: Empty name found, MFT ID = %I64u\n",pmfi->BaseMftId);
}

/**
 * @brief Updates a name of the file.
 * @details Replaces a DOS name by WINDOWS name,
 * WINDOWS name by POSIX when available.
 * @param[in,out] pmfi pointer to the structure
 * containing information about the file.
 * @param[in] name the name of the file.
 * @param[in] name_type the type of the file name.
 */
void UpdateFileName(PMY_FILE_INFORMATION pmfi,WCHAR *name,UCHAR name_type)
{
	/* compare name type with type of saved name */
	if(pmfi->Name[0] == 0 || pmfi->NameType == FILENAME_DOS || \
	  ((pmfi->NameType & FILENAME_WIN32) && (name_type == FILENAME_POSIX))){
		(void)wcsncpy(pmfi->Name,name,MAX_NTFS_PATH);
		pmfi->Name[MAX_NTFS_PATH-1] = 0;
		pmfi->NameType = name_type;
	}
}

/**
 * @brief Retrieves some filesystem information.
 * @param[in] pr_attr pointer to the structure
 * describing the resident attribute of the file.
 */
void GetVolumeInformationData(PRESIDENT_ATTRIBUTE pr_attr)
{
	PVOLUME_INFORMATION pvi;
	ULONG mj_ver, mn_ver;
	BOOLEAN dirty_flag = FALSE;
	
	pvi = (PVOLUME_INFORMATION)((char *)pr_attr + pr_attr->ValueOffset);
	if(pr_attr->ValueLength < sizeof(VOLUME_INFORMATION)){
		DebugPrint("VOLUME_INFORMATION is too short!\n");
		return;
	}
	
	mj_ver = (ULONG)pvi->MajorVersion;
	mn_ver = (ULONG)pvi->MinorVersion;
	if(pvi->Flags & 0x1) dirty_flag = TRUE;
	DebugPrint("NTFS Version %u.%u\n",mj_ver,mn_ver);
	if(dirty_flag) DebugPrint("Volume is dirty!\n");
}

/**
 * @brief Retrieves an information about the reparse point.
 * @param[in] pr_attr pointer to the structure
 * describing the resident attribute of the file.
 * @param[out] pmfi pointer to the structure receiving
 * information about the file.
 */
void CheckReparsePointResident(PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi)
{
	PREPARSE_POINT prp;
	ULONG tag;
	
	prp = (PREPARSE_POINT)((char *)pr_attr + pr_attr->ValueOffset);
	if(pr_attr->ValueLength >= sizeof(ULONG)){
		tag = prp->ReparseTag;
		DebugPrint("Reparse tag = 0x%x\n",tag);
	} else {
		DebugPrint("REPARSE_POINT is too short!\n");
	}
	
	pmfi->IsReparsePoint = TRUE;
}

/**
 * @brief Analyzes a resident attribute list.
 * @details Analyzes attributes of the current file
 * packed in another MFT records.
 * @param[in] pr_attr pointer to the structure
 * describing the resident attribute of the file.
 * @param[out] pmfi pointer to the structure receiving
 * information about the file.
 */
void AnalyseResidentAttributeList(PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi)
{
	PATTRIBUTE_LIST attr_list_entry;
	USHORT length;
	
	attr_list_entry = (PATTRIBUTE_LIST)((char *)pr_attr + pr_attr->ValueOffset);

	while(TRUE){
		if( ((char *)attr_list_entry + sizeof(ATTRIBUTE_LIST) - sizeof(attr_list_entry->AlignmentOrReserved)) > 
			((char *)pr_attr + pr_attr->ValueOffset + pr_attr->ValueLength) ) break;
		if(CheckForStopEvent()) break;
		/* is it a valid attribute */
		if(attr_list_entry->AttributeType == 0xffffffff) break;
		if(attr_list_entry->AttributeType == 0x0) break;
		if(attr_list_entry->Length == 0) break;
		//DebugPrint("@@@@@@@@@ FUCKED Length = %u\n", attr_list_entry->Length);
		AnalyseAttributeFromAttributeList(attr_list_entry,pmfi);
		/* go to the next attribute list entry */
		length = attr_list_entry->Length;
		attr_list_entry = (PATTRIBUTE_LIST)((char *)attr_list_entry + length);
	}
}

/**
 * @brief Analyzes a file attribute from the attribute list.
 * @param[in] attr_list_entry pointer to the attribute list entry.
 * @param[out] pmfi pointer to the structure receiving
 * information about the file.
 */
void AnalyseAttributeFromAttributeList(PATTRIBUTE_LIST attr_list_entry,PMY_FILE_INFORMATION pmfi)
{
	ULONGLONG child_record_mft_id;
	ATTRIBUTE_TYPE attr_type;
	USHORT attr_number;
	UCHAR name_length = 0;
	short *attr_name = NULL;

	/*
	* 21 Feb 2010
	* The right implementation must analyze a single
	* specific attribute from the MFT record pointed
	* by the attribute list entry.
	*/

	/* 1. save the name of the attribute */
	name_length = attr_list_entry->NameLength;
	if(attr_list_entry->NameOffset && name_length){
		attr_name = winx_heap_alloc((name_length + 1) * sizeof(short));
		if(attr_name == NULL){
			DebugPrint("Cannot allocate %u bytes of memory for AnalyseAttributeFromAttributeList()!\n",
				(name_length + 1) * sizeof(short));
			out_of_memory_condition_counter ++;
			return;
		}
		memcpy(attr_name,
			(char *)attr_list_entry + attr_list_entry->NameOffset,
			name_length * sizeof(short));
		attr_name[name_length] = 0;
		if(attr_name[0] == 0){
			winx_heap_free(attr_name);
			attr_name = NULL;
		}
	}
	/* attr_name is NULL here for empty names */
	
	/* 2. save the attribute type */
	attr_type = attr_list_entry->AttributeType;
	
	/* 3. save the identifier of the child record containing the attribute */
	child_record_mft_id = GetMftIdFromFRN(attr_list_entry->FileReferenceNumber);
	
	/* 4. save the AttributeNumber */
	attr_number = attr_list_entry->AttributeNumber;
	
	/* 5. analyze a single attribute */
	AnalyseAttributeFromMftRecord(child_record_mft_id,attr_type,attr_name,attr_number,pmfi);
	
	/* 6. free resources */
	if(attr_name) winx_heap_free(attr_name);
}

/**
 * @brief Analyzes a single attribute from the MFT record.
 * @param[in] mft_id the MFT record identifier.
 * @param[in] attr_type the attribute type.
 * @param[in] attr_name the name of the attribute.
 * @param[in] attr_number the numeric identifier of
 * the instance of the attribute.
 * @param[in] pmfi pointer to the structure containing
 * information about the file.
 * @note The attr_name paramter is equal to NULL for empty names.
 */
void AnalyseAttributeFromMftRecord(ULONGLONG mft_id,ATTRIBUTE_TYPE attr_type,
		short *attr_name,USHORT attr_number,PMY_FILE_INFORMATION pmfi)
{
	PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob = NULL;
	ULONG nfrob_size;
	NTSTATUS status;
	PFILE_RECORD_HEADER pfrh;
	
	/*
	* Skip attributes stored in the base mft record,
	* because they will be scanned anyway.
	*/
	if(mft_id == pmfi->BaseMftId){
		//DebugPrint("Attribute list entry points to 0x%x attribute of the base record.\n",(UINT)attr_type);
		return;
	}
	
	/* allocate memory for a single mft record */
	nfrob_size = sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + ntfs_record_size - 1;
	pnfrob = (PNTFS_FILE_RECORD_OUTPUT_BUFFER)winx_heap_alloc(nfrob_size);
	if(!pnfrob){
		DebugPrint("AnalyseAttributeFromMftRecord():\n");
		DebugPrint("No enough memory for NTFS_FILE_RECORD_OUTPUT_BUFFER!\n");
		out_of_memory_condition_counter ++;
		return;
	}

	/* get specified mft record */
	status = GetMftRecord(pnfrob,nfrob_size,mft_id);
	if(!NT_SUCCESS(status)){
		DebugPrint("AnalyseAttributeFromMftRecord(): FSCTL_GET_NTFS_FILE_RECORD failed: %x!\n",status);
		winx_heap_free(pnfrob);
		return;
	}
	if(GetMftIdFromFRN(pnfrob->FileReferenceNumber) != mft_id){
		DebugPrint("AnalyseAttributeFromAttributeList() failed - unable to get %I64u record.\n",mft_id);
		winx_heap_free(pnfrob);
		return;
	}

	/* validate the record header */
	pfrh = (PFILE_RECORD_HEADER)pnfrob->FileRecordBuffer;
	if(!IsFileRecord(pfrh)){
		DebugPrint("AnalyseAttributeFromMftRecord() failed - %I64u record has invalid type %u.\n",
			mft_id,pfrh->Ntfs.Type);
		winx_heap_free(pnfrob);
		return;
	}
	if(!(pfrh->Flags & 0x1)){
		DebugPrint("AnalyseAttributeFromMftRecord() failed\n");
		DebugPrint("%I64u record marked as free.\n",mft_id);
		winx_heap_free(pnfrob);
		return; /* skip free records */
	}

	if(pfrh->BaseFileRecord == 0){
		DebugPrint("AnalyseAttributeFromMftRecord() failed - %I64u is not a child record.\n",mft_id);
		winx_heap_free(pnfrob);
		return;
	}

	/* search for a specified attribute */
	AnalyseSingleAttribute(mft_id,pfrh,attr_type,attr_name,attr_number,pmfi);

	/* free allocated memory */
	winx_heap_free(pnfrob);
}

/**
 */
void AnalyseSingleAttribute(ULONGLONG mft_id,PFILE_RECORD_HEADER pfrh,
		ATTRIBUTE_TYPE attr_type,short *attr_name,USHORT attr_number,PMY_FILE_INFORMATION pmfi)
{
	int name_length;
	PATTRIBUTE pattr;
	ULONG attr_length;
	USHORT attr_offset;
	short *name = NULL;
	BOOLEAN attribute_found = FALSE;
	char *resident_status = "";

	attr_offset = pfrh->AttributeOffset;
	pattr = (PATTRIBUTE)((char *)pfrh + attr_offset);

	while(pattr){
		if(CheckForStopEvent()) break;

		/* is an attribute header inside a record bounds? */
		if(attr_offset + sizeof(ATTRIBUTE) > pfrh->BytesInUse || \
			attr_offset + sizeof(ATTRIBUTE) > ntfs_record_size)	break;
		
		/* is it a valid attribute */
		if(pattr->AttributeType == 0xffffffff) break;
		if(pattr->AttributeType == 0x0) break;
		if(pattr->Length == 0) break;

		/* is an attribute inside a record bounds? */
		if(attr_offset + pattr->Length > pfrh->BytesInUse || \
			attr_offset + pattr->Length > ntfs_record_size) break;
		
		/* is an attribute length valid? */
		if(pattr->Nonresident){
			if(pattr->Length < (sizeof(NONRESIDENT_ATTRIBUTE) - sizeof(ULONGLONG))){
				DebugPrint("Nonresident attribute length is invalid!\n");
				break;
			}
		} else {
			if(pattr->Length < sizeof(RESIDENT_ATTRIBUTE)){
				DebugPrint("Resident attribute length is invalid!\n");
				break;
			}
		}

		/* do we have found the specified attribute? */
		if(pattr->AttributeType == attr_type){
			if(pattr->NameOffset && pattr->NameLength){
				name = (short *)((char *)pattr + pattr->NameOffset);
				if(name[0] == 0) name = NULL;
			}
			if(attr_name == NULL){
				if(name == NULL && pattr->AttributeNumber == attr_number)
					attribute_found = TRUE;
			} else {
				if(name != NULL){
					name_length = wcslen(attr_name);
					if(name_length == pattr->NameLength){
						if(memcmp((void *)attr_name,(void *)name,name_length * sizeof(short)) == 0){
							if(pattr->AttributeNumber == attr_number)
								attribute_found = TRUE;
						}
					}
				}
			}
		}
		
		if(attribute_found){
			if(pattr->Nonresident) resident_status = "Nonresident";
			else resident_status = "Resident";
			//DebugPrint("An attribute pointed by the attribute list entry found...\n");
			DebugPrint("AttrListEntry: Base MftId = %I64u, MftId = %I64u, Attribute Type = 0x%x, Attribute Number = %u, %s\n",
				pmfi->BaseMftId,mft_id,(UINT)attr_type,(UINT)attr_number,resident_status);
			if(pattr->Nonresident) AnalyseNonResidentAttribute((PNONRESIDENT_ATTRIBUTE)pattr,pmfi);
			else AnalyseResidentAttribute((PRESIDENT_ATTRIBUTE)pattr,pmfi);
			number_of_processed_attr_list_entries ++;
			return;
		}
		
		/* go to the next attribute */
		attr_length = pattr->Length;
		attr_offset += (USHORT)(attr_length);
		pattr = (PATTRIBUTE)((char *)pattr + attr_length);
	}
}

/**
 * @brief Array containing default names of the attributes.
 */
ATTRIBUTE_NAME default_attribute_names[] = {
	{AttributeAttributeList,       L"$ATTRIBUTE_LIST"       },
	{AttributeEA,                  L"$EA"                   },
	{AttributeEAInformation,       L"$EA_INFORMATION"       },
	{AttributeSecurityDescriptor,  L"$SECURITY_DESCRIPTOR"  },
	{AttributeData,                L"$DATA"                 },
	{AttributeIndexRoot,           L"$INDEX_ROOT"           },
	{AttributeIndexAllocation,     L"$INDEX_ALLOCATION"     },
	{AttributeBitmap,              L"$BITMAP"               },
	{AttributeReparsePoint,        L"$REPARSE_POINT"        },
	{AttributeLoggedUtulityStream, L"$LOGGED_UTILITY_STREAM"}, /* used by EFS */
	{0,                            NULL                     }
};

/**
 * @brief Retrieves the default name of the attribute.
 * @param[in] attr_type the type of the attribute.
 * @return The default name of the attribute.
 * NULL indicates that the attribute has unknown type.
 */
short *GetDefaultAttributeName(ATTRIBUTE_TYPE attr_type)
{
	int i;
	
	for(i = 0;; i++){
		if(default_attribute_names[i].AttributeName == NULL) break;
		if(default_attribute_names[i].AttributeType == attr_type) break;
	}
	return default_attribute_names[i].AttributeName;
}

/**
 * @brief Analyzes a nonresident attribute of the file.
 * @param[in] pnr_attr pointer to the structure
 * describing the nonresident attribute of the file.
 * @param[out] pmfi pointer to the structure receiving
 * information about the file.
 * @note
 * - Nonresident attributes may contain fragmented data.
 * - Never use a name of the file here, because
 *   it may be not available yet.
 */
void AnalyseNonResidentAttribute(PNONRESIDENT_ATTRIBUTE pnr_attr,PMY_FILE_INFORMATION pmfi)
{
	ATTRIBUTE_TYPE attr_type;
	WCHAR *default_attr_name = NULL;
	short *attr_name;
	BOOLEAN NonResidentAttrListFound = FALSE;
	
	/* get default name of the attribute */
	attr_type = pnr_attr->Attribute.AttributeType;
	default_attr_name = GetDefaultAttributeName(attr_type);
	
	/* skip attributes of unknown type */
	if(default_attr_name == NULL){
		DebugPrint("Nonresident attribute of unknown type 0x%x found!\n",(UINT)attr_type);
		return;
	}

	/* allocate memory */
	attr_name = (short *)winx_heap_alloc((MAX_NTFS_PATH + 1) * sizeof(short));
	if(!attr_name){
		DebugPrint("Cannot allocate memory for attr_name in AnalyseNonResidentAttribute()!\n");
		out_of_memory_condition_counter ++;
		return;
	}
	
	/* additional checks */
	if(attr_type == AttributeAttributeList){ 
		DebugPrint("Nonresident AttributeList found!\n");
		NonResidentAttrListFound = TRUE;
	}

	if(attr_type == AttributeReparsePoint)
		pmfi->IsReparsePoint = TRUE;
	
	/* ------------------------------------------------------------------------- */
	/*                          get the attribute name                           */
 	/* ------------------------------------------------------------------------- */
	
	attr_name[0] = 0;
	if(pnr_attr->Attribute.NameLength){
		/* NameLength is always less than MAX_PATH! */
		(void)wcsncpy(attr_name,(short *)((char *)pnr_attr + pnr_attr->Attribute.NameOffset),
			pnr_attr->Attribute.NameLength);
		attr_name[pnr_attr->Attribute.NameLength] = 0;
	}
	
	if(attr_name[0] == 0){
		(void)wcsncpy(attr_name,default_attr_name,MAX_NTFS_PATH);
		attr_name[MAX_NTFS_PATH - 1] = 0;
	}

	/* never append $DATA attribute name */
	if(wcscmp(attr_name,L"$DATA") == 0) attr_name[0] = 0;
	
	/* do not append $I30 attribute name - required by GetFileNameAndParentMftId() */
	if(wcscmp(attr_name,L"$I30") == 0) attr_name[0] = 0;
	if(wcscmp(attr_name,L"$INDEX_ALLOCATION") == 0) attr_name[0] = 0;
	
	/* ------------------------------------------------------------------------- */
	/*                   now we have the name of the attribute                   */
	/* ------------------------------------------------------------------------- */

	/* just for debugging */
	/*if(wcsistr(pmfi->Name,L"Scratch")){
		DebugPrint("@@@@ %ws DIRECTORY FOUND, MFT_ID = %I64u, PARENT ID = %I64u\n",
			pmfi->Name,pmfi->BaseMftId,pmfi->ParentDirectoryMftId);
		DebugPrint("@@@@ FULL ATTRIBUTE NAME = %ws:%ws, DEFAULT ATTR NAME = %ws\n",
			pmfi->Name,attr_name,default_attr_name);
	}*/

	if(NonResidentAttrListFound) DebugPrint("%ws:%ws\n",pmfi->Name,attr_name);
	
	/* skip $BadClus file which may have wrong number of clusters */
	if(/*wcsistr(attr_name,L"$BadClus") || */wcsistr(pmfi->Name,L"$BadClus")){
		/* on my system this file always exists, even after chkdsk execution */
		/*DebugPrint("WARNING: %ws:%ws file found! Run CheckDisk program!\n",pmfi->Name,attr_name);*/
	} else {
		ProcessRunList(attr_name,pnr_attr,pmfi,NonResidentAttrListFound);
	}

	/* free allocated memory */
	winx_heap_free(attr_name);
}

/**
 */
static ULONG RunLength(PUCHAR run)
{
	return (*run & 0xf) + ((*run >> 4) & 0xf) + 1;
}

/**
 */
static LONGLONG RunLCN(PUCHAR run)
{
	LONG i;
	UCHAR n1 = *run & 0xf;
	UCHAR n2 = (*run >> 4) & 0xf;
	LONGLONG lcn = (n2 == 0) ? 0 : (LONGLONG)(((signed char *)run)[n1 + n2]);
	
	for(i = n1 + n2 - 1; i > n1; i--)
		lcn = (lcn << 8) + run[i];
	return lcn;
}

/**
 */
static ULONGLONG RunCount(PUCHAR run)
{
	ULONG i;
	UCHAR n = *run & 0xf;
	ULONGLONG count = 0;
	
	for(i = n; i > 0; i--)
		count = (count << 8) + run[i];
	return count;
}

/**
 * @brief Retrieves VCN/LCN pairs of the attribute.
 * @param[in] full_path the full path of the attribute.
 * @param[in] pnr_attr pointer to the structure
 * describing the nonresident attribute of the file.
 * @param[out] pmfi pointer to the structure receiving
 * information about the file.
 * @param[in] is_attr_list boolean value indicating, is
 * attribute list to be analysed or not.
 */
void ProcessRunList(WCHAR *full_path,
					PNONRESIDENT_ATTRIBUTE pnr_attr,PMY_FILE_INFORMATION pmfi,
					BOOLEAN is_attr_list)
{
	PUCHAR run;
	ULONGLONG lcn, vcn, length;
	PFILENAME pfn;
	BOOLEAN is_compressed = (pnr_attr->Attribute.Flags & 0x1) ? TRUE : FALSE;
	
/*	if(pnr_attr->Attribute.Flags & 0x1)
		DbgPrint("[CMP] %ws VCN %I64u - %I64u\n",full_path,pnr_attr->LowVcn,pnr_attr->HighVcn);
	else
		DbgPrint("[ORD] %ws VCN %I64u - %I64u\n",full_path,pnr_attr->LowVcn,pnr_attr->HighVcn);
*/	
	/* find corresponding FILENAME structure in dx->filelist or add a new one */
	pfn = FindFileListEntryForTheAttribute(full_path,pmfi);
	if(pfn == NULL) return;
	
	if(is_compressed) pfn->is_compressed = TRUE;
	
	/* loop through runs */
	lcn = 0; vcn = pnr_attr->LowVcn;
	run = (PUCHAR)((char *)pnr_attr + pnr_attr->RunArrayOffset);
	while(*run){
		lcn += RunLCN(run);
		length = RunCount(run);
		
		/* skip virtual runs */
		if(RunLCN(run)){
			/* check for data consistency */
			if(!CheckBlock(lcn,length)){
				DebugPrint("Error in MFT found, run Check Disk program!\n");
				break;
			}
			ProcessRun(full_path,pmfi,pfn,vcn,length,lcn);
		}
		
		/* go to the next run */
		run += RunLength(run);
		vcn += length;
	}
	
	/* analyze nonresident attribute lists */
	if(is_attr_list) AnalyseNonResidentAttributeList(pfn,pmfi,pnr_attr->InitializedSize);
}

/**
 * @brief Analyzes a nonresident attribute list.
 * @param[out] pfn pointer to the structure containing
 * information about the attribute list file.
 * @param[out] pmfi pointer to the structure receiving
 * information about the file.
 * @param[in] size the size of the attribute list file, in bytes.
 */
void AnalyseNonResidentAttributeList(PFILENAME pfn,PMY_FILE_INFORMATION pmfi,ULONGLONG size)
{
	ULONGLONG clusters_to_read;
	char *cluster;
	char *current_cluster;
	PBLOCKMAP block;
	ULONGLONG lsn;
	NTSTATUS status;
	PATTRIBUTE_LIST attr_list_entry;
	int i;
	USHORT length;
	
	DebugPrint("Allocated size = %I64u bytes.\n",size);
	if(size == 0){
		DebugPrint("Empty nonresident attribute list found.\n");
		return;
	}
	
	/* allocate memory for an integral number of cluster to hold a whole AttributeList */
	clusters_to_read = size / bytes_per_cluster;
	/* the following check is a little bit complicated, because _aulldvrm() call is missing on w2k */
	if(size - clusters_to_read * bytes_per_cluster/*size % bytes_per_cluster*/) clusters_to_read ++;
	cluster = (char *)winx_heap_alloc((SIZE_T)(bytes_per_cluster * clusters_to_read));
	if(!cluster){
		DebugPrint("Cannot allocate %I64u bytes of memory for AnalyseNonResidentAttributeList()!\n",
			bytes_per_cluster * clusters_to_read);
		out_of_memory_condition_counter ++;
		return;
	}
	
	/* loop through all blocks of file */
	current_cluster = cluster;
	for(block = pfn->blockmap; block != NULL; block = block->next_ptr){
		/* loop through clusters of the current block */
		for(i = 0; i < block->length; i++){
			/* read current cluster */
			lsn = (block->lcn + i) * sectors_per_cluster;
			status = ReadSectors(lsn,current_cluster,(ULONG)bytes_per_cluster);
			if(!NT_SUCCESS(status)){
				DebugPrint("Cannot read the %I64u sector: %x!\n",
					lsn,(UINT)status);
				goto scan_done;
			}
			clusters_to_read --;
			if(clusters_to_read == 0){
				/* is it the last cluster of the file? */
				if(i < (block->length - 1) || block->next_ptr != pfn->blockmap)
					DebugPrint("The attribute list has more clusters than expected.\n");
				goto analyze_list;
			}
			current_cluster += bytes_per_cluster;
		}
		if(block->next_ptr == pfn->blockmap) break;
	}

analyze_list:
	if(clusters_to_read){
		DebugPrint("The attribute list has less number of clusters than expected.\n");
		DebugPrint("Therefore it will be skipped, because anyway we don\'t know its exact size.\n");
		goto scan_done;
	}

	DebugPrint("Attribute list analysis started...\n");
	attr_list_entry = (PATTRIBUTE_LIST)cluster;

	while(TRUE){
		if( ((char *)attr_list_entry + sizeof(ATTRIBUTE_LIST) - sizeof(attr_list_entry->AlignmentOrReserved)) > 
			((char *)cluster + size) ) break;
		if(CheckForStopEvent()) break;
		/* is it a valid attribute */
		if(attr_list_entry->AttributeType == 0xffffffff) break;
		if(attr_list_entry->AttributeType == 0x0) break;
		if(attr_list_entry->Length == 0) break;
		//DebugPrint("@@@@@@@@@ FUCKED Length = %u\n", attr_list_entry->Length);
		AnalyseAttributeFromAttributeList(attr_list_entry,pmfi);
		/* go to the next attribute list entry */
		length = attr_list_entry->Length;
		attr_list_entry = (PATTRIBUTE_LIST)((char *)attr_list_entry + length);
	}
	DebugPrint("Attribute list analysis completed.\n");

scan_done:	
	/* free allocated resources */
	winx_heap_free(cluster);
}

/*------------------------ Defragmentation related code ------------------------------*/

/**
 * @brief Retrieves MFT position and size
 * and applies them to the cluster map.
 * @param[in] nd pointer to the structure
 * containing all information needed for
 * computations.
 * @return The complete MFT length, in clusters.
 */
ULONGLONG ProcessMftSpace(PNTFS_DATA nd)
{
	ULONGLONG start,len,mft_len = 0,mirror_size;

	/*
	* MFT space must be excluded from the free space list.
	* Because Windows 2000 disallows to move files there.
	* And because on other systems this dirty technique 
	* causes MFT fragmentation.
	*/
	
	/* 
	* Don't increment dx->processed_clusters here, 
	* because some parts of MFT are really free.
	*/
	DebugPrint("MFT part   : start : length\n");

	/* $MFT */
	start = nd->MftStartLcn.QuadPart;
	if(nd->BytesPerCluster)
		len = nd->MftValidDataLength.QuadPart / nd->BytesPerCluster;
	else
		len = 0;
	DebugPrint("$MFT       :%I64u :%I64u\n",start,len);
	if(CheckBlock(start,len)){
		/* remark space as reserved */
		RemarkBlock(start,len,MFT_ZONE_SPACE,SYSTEM_SPACE);
		RemoveFreeSpaceBlock(start,len);
		mft_len += len;
	}

	/* MFT Zone */
	start = nd->MftZoneStart.QuadPart;
	len = nd->MftZoneEnd.QuadPart - nd->MftZoneStart.QuadPart;
	DebugPrint("MFT Zone   :%I64u :%I64u\n",start,len);
	if(CheckBlock(start,len)){
		/* remark space as reserved */
		RemarkBlock(start,len,MFT_ZONE_SPACE,SYSTEM_SPACE);
		RemoveFreeSpaceBlock(start,len);
	}

	/* $MFTMirror */
	start = nd->Mft2StartLcn.QuadPart;
	len = 1;
	mirror_size = nd->BytesPerFileRecordSegment * 4;
	if(nd->BytesPerCluster && mirror_size > nd->BytesPerCluster){
		len = mirror_size / nd->BytesPerCluster;
		if(mirror_size - len * nd->BytesPerCluster)
			len ++;
	}
	DebugPrint("$MFTMirror :%I64u :%I64u\n",start,len);
	if(CheckBlock(start,len)){
		/* remark space as reserved */
		RemarkBlock(start,len,MFT_ZONE_SPACE,SYSTEM_SPACE);
		RemoveFreeSpaceBlock(start,len);
	}
	
	return mft_len;
}

/*
* This code may slow down the program: 
* for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr) for each file
*/

/**
 * @brief Adds information about a single VCN/LCN pair
 * to the structure describing the file.
 * @param[in] full_path the full path of the file.
 * @param[out] pmfi pointer to the structure receiving
 * information about the file. 
 * @param[in,out] pfn pointer to the structure receiving
 * information about the file.
 * @param[in] vcn the virtual cluster number.
 * @param[in] length the length of the block.
 * @param[in] lcn the logical cluster number.
 */
void ProcessRun(WCHAR *full_path,PMY_FILE_INFORMATION pmfi,
				PFILENAME pfn,ULONGLONG vcn,ULONGLONG length,ULONGLONG lcn)
{
	PBLOCKMAP block, prev_block = NULL;
	
/*	if(!wcscmp(full_path,L"\\??\\L:\\go.zip")){
		DbgPrint("VCN %I64u : LEN %I64u : LCN %I64u\n",vcn,length,lcn);
	}
*/
	(void)pmfi;
	
	/* add information to blockmap member of specified pfn structure */
	if(pfn->blockmap) prev_block = pfn->blockmap->prev_ptr;
	block = (PBLOCKMAP)winx_list_insert_item((list_entry **)&pfn->blockmap,(list_entry *)prev_block,sizeof(BLOCKMAP));
	if(!block){
		DebugPrint("Cannot allocate %u bytes of memory for ProcessRun()!\n",sizeof(BLOCKMAP));
		out_of_memory_condition_counter ++;
		return;
	}
	
	block->vcn = vcn;
	block->length = length;
	block->lcn = lcn;

	pfn->n_fragments ++;
	pfn->clusters_total += block->length;
	/*
	* Sometimes normal file has more than one fragment, 
	* but is not fragmented yet! 8-) 
	*/
	if(block != pfn->blockmap && \
	  block->lcn != (block->prev_ptr->lcn + block->prev_ptr->length))
		pfn->is_fragm = TRUE;
}

/**
 * @brief Searches for the file list
 * entry describing the file.
 * @param[in] full_path the full path of the file.
 * @param[in] pmfi pointer to the structure containing
 * information about the file. 
 * @return A pointer to the structure describing the file.
 * NULL indicates failure.
 */
PFILENAME FindFileListEntryForTheAttribute(WCHAR *full_path,PMY_FILE_INFORMATION pmfi)
{
	PFILENAME pfn;
	
	/* few entries (attributes) may have the same mft id */
	for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
		/*
		* we scan mft from the end to the beginning (RTL)
		* and we add new records to the left side of the file list...
		*/
		if(MftScanDirection == MFT_SCAN_RTL){
			if(pfn->BaseMftId > pmfi->BaseMftId) break; /* we have no chance to find record in list */
		} else {
			if(pfn->BaseMftId < pmfi->BaseMftId) break;
		}
		if(!wcscmp(pfn->name.Buffer,full_path) && \
		  (pfn->ParentDirectoryMftId == pmfi->ParentDirectoryMftId) && \
		  (pfn->BaseMftId == pmfi->BaseMftId))
			return pfn; /* very slow? */
		//if(pfn->BaseMftId == pmfi->BaseMftId) return pfn; /* safe? */
		if(pfn->next_ptr == filelist) break;
	}
	
	pfn = (PFILENAME)winx_list_insert_item((list_entry **)(void *)&filelist,NULL,sizeof(FILENAME));
	if(pfn == NULL){
		DebugPrint("Cannot allocate %u bytes of memory for FindFileListEntryForTheAttribute()!\n",sizeof(FILENAME));
		out_of_memory_condition_counter ++;
		return NULL;
	}
	
	/* fill a name member of the created structure */
	if(!RtlCreateUnicodeString(&pfn->name,full_path)){
		DebugPrint("No enough memory for pfn->name initialization!\n");
		out_of_memory_condition_counter ++;
		winx_list_remove_item((list_entry **)(void *)&filelist,(list_entry *)pfn);
		return NULL;
	}
	pfn->blockmap = NULL; /* !!! */
	pfn->BaseMftId = pmfi->BaseMftId;
	pfn->ParentDirectoryMftId = pmfi->ParentDirectoryMftId;
	pfn->PathBuilt = FALSE;
	pfn->n_fragments = 0;
	pfn->clusters_total = 0;
	pfn->is_fragm = FALSE;
	pfn->is_compressed = FALSE;
	pfn->is_dirty = TRUE;
	pfn->is_filtered = FALSE; /* initial state */
	return pfn;
}

/**
 * @brief Applies information about the file
 * to the cluster map and statistics.
 * @param[in] pmfi pointer to the structure containing
 * information about the file.
 */
void UpdateClusterMapAndStatistics(PMY_FILE_INFORMATION pmfi)
{
	PFILENAME pfn;
	ULONGLONG filesize;

	/* All stuff commented with C++ style comments was moved to BuildPaths() function. */
	for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
		/* only the first few entries may have dirty flag set */
		if(pfn->is_dirty == FALSE) break;

		pfn->is_dirty = FALSE;
		/* 1. fill all members of pfn */
		/* 1.1 set flags in pfn ??? */
		/* Note, FILE_ATTR_DIRECTORY is not considered valid in NT.  It is
		   reserved for the DOS SUBDIRECTORY flag. */
		/* always sets is_dir flag to FALSE */
		/*if(pmfi->Flags & FILE_ATTRIBUTE_DIRECTORY) pfn->is_dir = TRUE;
		else pfn->is_dir = FALSE;
		*/
		pfn->is_dir = pmfi->IsDirectory;
		/* ordinary attributes in compressed file? */
		/*if(pmfi->Flags & FILE_ATTRIBUTE_COMPRESSED) pfn->is_compressed = TRUE;
		else pfn->is_compressed = FALSE;
		*/
		if((pmfi->Flags & FILE_ATTRIBUTE_REPARSE_POINT) || pmfi->IsReparsePoint){
			DebugPrint("Reparse point found %ws\n",pfn->name.Buffer);
			pfn->is_reparse_point = TRUE;
		} else pfn->is_reparse_point = FALSE;
		/* 1.2 calculate size of attribute data */
		filesize = pfn->clusters_total * bytes_per_cluster;
		if(sizelimit && filesize > sizelimit) pfn->is_overlimit = TRUE;
		else pfn->is_overlimit = FALSE;
		/* mark some files as filtered out */
		CHECK_FOR_FRAGLIMIT(pfn);
		/* 1.3 detect temporary files and other unwanted stuff */
		if(TemporaryStuffDetected(pmfi)) pfn->is_filtered = TRUE;
		/* 1.4 detect sparse files */
		if(pmfi->Flags & FILE_ATTRIBUTE_SPARSE_FILE)
			DebugPrint("Sparse file found %ws\n",pfn->name.Buffer);
		/* 1.5 detect encrypted files */
		if(pmfi->Flags & FILE_ATTRIBUTE_ENCRYPTED){
			DebugPrint2("Encrypted file found %ws\n",pfn->name.Buffer);
		}
		/* 2. redraw cluster map */
//		MarkSpace(dx,pfn,SYSTEM_SPACE);
		/* 3. update statistics */
		Stat.filecounter ++;
		if(pfn->is_dir) Stat.dircounter ++;
		if(pfn->is_compressed) Stat.compressedcounter ++;
		/* skip here filtered out and big files and reparse points */
//		if(pfn->is_fragm && !pfn->is_filtered && !pfn->is_overlimit && !pfn->is_reparse_point){
//			dx->fragmfilecounter ++;
//			dx->fragmcounter += pfn->n_fragments;
//		} else {
//			dx->fragmcounter ++;
//		}
		/* MFT? */
		//if(pfn->clusters_total > 10000) DbgPrint("%I64u %ws\n",pfn->clusters_total,pfn->name.Buffer);
		//DbgPrint("%ws %I64u\n",pfn->name.Buffer,pfn->clusters_total);
		Stat.processed_clusters += pfn->clusters_total;

		if(pfn->next_ptr == filelist) break;
	}
}

/**
 * @brief Defines, is a file temporary or not.
 * @param[in] pmfi pointer to the structure containing
 * information about the file.
 * @return Boolean value. TRUE indicates that the file
 * is temporary.
 */
BOOLEAN TemporaryStuffDetected(PMY_FILE_INFORMATION pmfi)
{
	/* skip temporary files ;-) */
	if(pmfi->Flags & FILE_ATTRIBUTE_TEMPORARY){
		DebugPrint2("-Ultradfg- Temporary file found %ws\n",pmfi->Name);
		return TRUE;
	}
	return FALSE;
}

/* that's unbelievable, but this function runs fast */
/**
 * @brief Checks, must file be skipped or not.
 * @param[in] pfn pointer to the variable
 * containing information about the file.
 * @return Boolean value. TRUE indicates that
 * the file must be skipped, FALSE indicates contrary.
 */
BOOLEAN UnwantedStuffDetected(PFILENAME pfn)
{
	UNICODE_STRING us;

	/* skip all unwanted files by user defined patterns */
	if(!RtlCreateUnicodeString(&us,pfn->name.Buffer)){
		DebugPrint("Cannot allocate memory for UnwantedStuffDetected()!\n");
		out_of_memory_condition_counter ++;
		return FALSE;
	}
	(void)_wcslwr(us.Buffer);

	if(in_filter.buffer){
		if(!IsStringInFilter(us.Buffer,&in_filter)){
			RtlFreeUnicodeString(&us); return TRUE; /* not included */
		}
	}

	if(ex_filter.buffer){
		if(IsStringInFilter(us.Buffer,&ex_filter)){
			RtlFreeUnicodeString(&us); return TRUE; /* excluded */
		}
	}
	RtlFreeUnicodeString(&us);

	return FALSE;
}

PMY_FILE_ENTRY mf = NULL; /* pointer to array of MY_FILE_ENTRY structures */
ULONG n_entries;
BOOLEAN mf_allocated = FALSE;

/**
 * @brief Builds a full paths of all files
 * contained on the volume.
 */
void BuildPaths(void)
{
	ULONGLONG tm, time;
	PFILENAME pfn;
	ULONG i;
	
	DebugPrint("BuildPaths() started...\n");
	tm = _rdtsc();

	/* prepare data for fast binary search */
	mf_allocated = FALSE;
	/*n_entries = Stat.filecounter;*/
	/* more accurately */
	n_entries = 0;
	for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
		n_entries++;
		if(pfn->next_ptr == filelist) break;
	}
	
	if(n_entries){
		mf = (PMY_FILE_ENTRY)winx_heap_alloc(n_entries * sizeof(MY_FILE_ENTRY));
		if(mf != NULL) mf_allocated = TRUE;
		else DebugPrint("Cannot allocate %u bytes of memory for mf array!\n",
				n_entries * sizeof(MY_FILE_ENTRY));
	}
	if(mf_allocated){
		/* fill array */
		i = 0;
		for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
			mf[i].mft_id = pfn->BaseMftId;
			mf[i].pfn = pfn;
			if(i == (n_entries - 1)){ 
				if(pfn->next_ptr != filelist)
					DebugPrint("BuildPaths(): ???\n");
				break;
			}
			i++;
			if(pfn->next_ptr == filelist) break;
		}
		DebugPrint("Fast binary search will be used.\n");
	} else {
		DebugPrint("Slow linear search will be used.\n");
	}
	
	for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
		BuildPath2(pfn);
		if(UnwantedStuffDetected(pfn)) pfn->is_filtered = TRUE;
		MarkFileSpace(pfn,SYSTEM_SPACE);
		/* skip here filtered out and big files and reparse points */
		if(pfn->is_fragm && !pfn->is_reparse_point &&
			((!pfn->is_filtered && !pfn->is_overlimit) || optimize_flag)
			){
			Stat.fragmfilecounter ++;
			Stat.fragmcounter += pfn->n_fragments;
		} else {
			Stat.fragmcounter ++;
		}
		if(pfn->next_ptr == filelist) break;
	}
	
	/* free allocated resources */
	if(mf_allocated) winx_heap_free(mf);
	time = _rdtsc() - tm;
	DebugPrint("BuildPaths() completed in %I64u ms.\n",time);
}

/**
 * @brief Builds a full path of the file.
 * @param[in,out] pfn pointer to the structure
 * containing information about the file.
 */
void BuildPath2(PFILENAME pfn)
{
	WCHAR *buffer1;
	WCHAR *buffer2;
	ULONG offset;
	ULONGLONG mft_id,parent_mft_id;
	ULONG name_length;
	WCHAR header[] = L"\\??\\A:";
	BOOLEAN FullPathRetrieved = FALSE;
	UNICODE_STRING us;
	
	/* skip invalid files which have no name */
	if(pfn->name.Buffer[0] == 0){
		DebugPrint("BuildPath2: Invalid entry found: file has no name!\n");
		DebugPrint("BuildPath2: MFT ID = %I64u\n",pfn->BaseMftId);
		return;
	}

	/* allocate memory */
	buffer1 = (WCHAR *)winx_heap_alloc((MAX_NTFS_PATH) * sizeof(short));
	if(!buffer1){
		DebugPrint("BuildPath2(): cannot allocate memory for buffer1\n");
		out_of_memory_condition_counter ++;
		return;
	}
	buffer2 = (WCHAR *)winx_heap_alloc((MAX_NTFS_PATH) * sizeof(short));
	if(!buffer2){
		DebugPrint("BuildPath2(): cannot allocate memory for buffer2\n");
		out_of_memory_condition_counter ++;
		winx_heap_free(buffer1);
		return;
	}

	/* terminate buffer1 with zero */
	offset = MAX_NTFS_PATH - 1;
	buffer1[offset] = 0; /* terminating zero */
	offset --;
	
	/* copy filename to the right side of the buffer1 */
	name_length = wcslen(pfn->name.Buffer);
	if(offset < (name_length - 1)){
		DebugPrint("BuildPath2(): %ws filename is too long (%u characters)\n",
			pfn->name.Buffer,name_length);
		winx_heap_free(buffer1);
		winx_heap_free(buffer2);
		return;
	}

	offset -= (name_length - 1);
	(void)wcsncpy(buffer1 + offset,pfn->name.Buffer,name_length);

	if(offset == 0) goto path_is_too_long;
	offset --;
	
	/* add backslash */
	buffer1[offset] = '\\';

	if(offset == 0) goto path_is_too_long;
	offset --;
	
	if(offset == 0) goto path_is_too_long;

	parent_mft_id = pfn->ParentDirectoryMftId;
	while(parent_mft_id != FILE_root){
		if(CheckForStopEvent()) goto build_path_done;
		mft_id = parent_mft_id;
		FullPathRetrieved = GetFileNameAndParentMftId(mft_id,&parent_mft_id,buffer2,MAX_NTFS_PATH);
		if(buffer2[0] == 0){
			DebugPrint("BuildPath2(): cannot retrieve parent directory name!\n");
			goto build_path_done;
		}
		//DbgPrint("%ws\n",buffer2);
		/* append buffer2 contents to the right side of buffer1 */
		name_length = wcslen(buffer2);
		if(offset < (name_length - 1)) goto path_is_too_long;
		offset -= (name_length - 1);
		(void)wcsncpy(buffer1 + offset,buffer2,name_length);
		
		if(FullPathRetrieved) goto update_filename;
		
		if(offset == 0) goto path_is_too_long;
		offset --;

		/* add backslash */
		buffer1[offset] = '\\';

		if(offset == 0) goto path_is_too_long;
		offset --;

		if(offset == 0) goto path_is_too_long;
	}
	
	/* append volume letter */
	header[4] = volume_letter;
	name_length = wcslen(header);
	if(offset < (name_length - 1)) goto path_is_too_long;
	offset -= (name_length - 1);
	(void)wcsncpy(buffer1 + offset,header,name_length);

update_filename:	
	/* replace pfn->name contents with full path */
	//wcsncpy(pmfi->Name,buffer1 + offset,MAX_NTFS_PATH);
	//pmfi->Name[MAX_NTFS_PATH - 1] = 0;
	if(!RtlCreateUnicodeString(&us,buffer1 + offset)){
		DebugPrint("Cannot allocate memory for BuildPath2()!\n");
		out_of_memory_condition_counter ++;
	} else {
		RtlFreeUnicodeString(&(pfn->name));
		pfn->name.Buffer = us.Buffer;
		pfn->name.Length = us.Length;
		pfn->name.MaximumLength = us.MaximumLength;
	}
	pfn->PathBuilt = TRUE;
	
	#ifdef DETAILED_LOGGING
	DebugPrint("FULL PATH = %ws\n",pfn->name.Buffer);
	#endif
	
build_path_done:
	winx_heap_free(buffer1);
	winx_heap_free(buffer2);
	return;
	
path_is_too_long:
	DebugPrint("BuildPath2(): path is too long: %ws\n",buffer1);
	winx_heap_free(buffer1);
	winx_heap_free(buffer2);
}

/**
 * @brief Retrieves a file name and a parent MFT
 * identifier for the MFT record.
 * @param[in] mft_id the MFT record number.
 * @param[out] parent_mft_id pointer to the
 * variable receiving the parent MFT identifier.
 * @param[out] buffer pointer to buffer receiving
 * a file name.
 * @param[in] length the length of the buffer,
 * in characters.
 * @return Boolean value. TRUE indicates that
 * the full path has been retrieved, FALSE
 * indicates that it was retrieved just partially.
 */
BOOLEAN GetFileNameAndParentMftId(ULONGLONG mft_id,ULONGLONG *parent_mft_id,WCHAR *buffer,ULONG length)
{
	PFILENAME pfn;
	BOOLEAN FullPathRetrieved = FALSE;
	
	/* initialize data */
	buffer[0] = 0;
	*parent_mft_id = FILE_root;

	/* find an appropriate pfn structure */
	pfn = FindDirectoryByMftId(mft_id);
	
	if(pfn == NULL){
		DebugPrint("%I64u directory not found!\n",mft_id);
		return FullPathRetrieved;
	}
	
	/* update data */
	*parent_mft_id = pfn->ParentDirectoryMftId;
	(void)wcsncpy(buffer,pfn->name.Buffer,length);
	buffer[length-1] = 0;
	FullPathRetrieved = pfn->PathBuilt;

	if(buffer[0] == 0){
		DebugPrint("GetFileNameAndParentMftId: Invalid entry found: file has no name!\n");
		DebugPrint("GetFileNameAndParentMftId: MFT ID = %I64u\n",mft_id);
	}
	
	return FullPathRetrieved;
}

/**
 * @brief Adds a resident directory to the file list.
 * @param[in] pmfi pointer to the structure describing
 * the directory.
 */
void AddResidentDirectoryToFileList(PMY_FILE_INFORMATION pmfi)
{
	PFILENAME pfn;
	
	if(pmfi->Name[0] == 0){
		DebugPrint("AddResidentDirectoryToFileList: Invalid entry found: file has no name!\n");
		DebugPrint("AddResidentDirectoryToFileList: MFT ID = %I64u\n",pmfi->BaseMftId);
		return;
	}

	pfn = (PFILENAME)winx_list_insert_item((list_entry **)(void *)&filelist,NULL,sizeof(FILENAME));
	if(pfn == NULL){
		DebugPrint("Cannot allocate %u bytes of memory for AddResidentDirectoryToFileList()!\n",sizeof(FILENAME));
		out_of_memory_condition_counter ++;
		return;
	}
	
	/* fill a name member of the created structure */
	if(!RtlCreateUnicodeString(&pfn->name,pmfi->Name)){
		DebugPrint("AddResidentDirectoryToFileList():\n");
		DebugPrint("no enough memory for pfn->name initialization!\n");
		out_of_memory_condition_counter ++;
		winx_list_remove_item((list_entry **)(void *)&filelist,(list_entry *)pfn);
		return;
	}
	pfn->blockmap = NULL; /* !!! */
	pfn->BaseMftId = pmfi->BaseMftId;
	pfn->ParentDirectoryMftId = pmfi->ParentDirectoryMftId;
	pfn->PathBuilt = FALSE; // what's fucked mistake - it was TRUE here;
	pfn->n_fragments = 0;
	pfn->clusters_total = 0;
	pfn->is_fragm = FALSE;
	pfn->is_compressed = FALSE;
	pfn->is_dir = TRUE;
	pfn->is_reparse_point = pmfi->IsReparsePoint;
	pfn->is_overlimit = FALSE;
	pfn->is_filtered = TRUE;
	pfn->is_dirty = TRUE;
}

/**
 * @brief Searches for a directory by its MFT identifier.
 * @param[in] mft_id the MFT identifier of the directory.
 * @return A pointer to the structure describing the
 * directory. NULL indicates failure.
 */
PFILENAME FindDirectoryByMftId(ULONGLONG mft_id)
{
	PFILENAME pfn;
	ULONG lim, i, k;
	signed long m;
	BOOLEAN ascending_order;

	if(mf_allocated == FALSE){ /* use slow search */
		for(pfn = filelist; pfn != NULL; pfn = pfn->next_ptr){
			if(pfn->BaseMftId == mft_id){
				if(wcsstr(pfn->name.Buffer,L":$") == NULL)
					return pfn;
			}
			if(pfn->next_ptr == filelist) break;
		}
		return NULL;
	} else { /* use fast binary search */
		/* Based on bsearch() algorithm copyrighted by DJ Delorie (1994). */
		ascending_order = (MftScanDirection == MFT_SCAN_RTL) ? TRUE : FALSE;
		i = 0;
		for(lim = n_entries; lim != 0; lim >>= 1){
			k = i + (lim >> 1);
			if(mf[k].mft_id == mft_id){
				/* search for proper entry in neighbourhood of found entry */
				for(m = k; m >= 0; m --){
					if(mf[m].mft_id != mft_id) break;
				}
				for(m = m + 1; (unsigned long)(m) < n_entries; m ++){
					if(mf[m].mft_id != mft_id) break;
					if(wcsstr(mf[m].pfn->name.Buffer,L":$") == NULL)
						return mf[m].pfn;
				}
				DebugPrint("FUCK 1\n");
				return NULL;
			}
			if(ascending_order){
				if(mft_id > mf[k].mft_id){
					i = k + 1; lim --; /* move right */
				} /* else move left */
			} else {
				if(mft_id < mf[k].mft_id){
					i = k + 1; lim --; /* move right */
				} /* else move left */
			}
		}
		DebugPrint("FUCK 2\n");
		return NULL;
	}
}

/**
 * @brief Reads sectors from disk.
 * @param[in] lsn the logical sector number.
 * @param[out] buffer pointer to the buffer
 * receiving sector data.
 * @param[in] length the length of the buffer,
 * in bytes.
 * @return An appropriate NTSTATUS code.
 * @note
 * - LSN, buffer, length must be valid before this call!
 * - Length MUST BE an integral of the sector size.
 */
NTSTATUS ReadSectors(ULONGLONG lsn,PVOID buffer,ULONG length)
{
	IO_STATUS_BLOCK ioStatus;
	LARGE_INTEGER offset;
	NTSTATUS Status;
	ULONGLONG tm, time;

	offset.QuadPart = lsn * bytes_per_sector;
	Status = NtReadFile(winx_fileno(fVolume),NULL,NULL,NULL,&ioStatus,buffer,length,&offset,NULL);
	if(NT_SUCCESS(Status)/* == STATUS_PENDING*/){
		DebugPrint("ReadSectors waiting started...\n");
		tm = _rdtsc();
		Status = NtWaitForSingleObject(winx_fileno(fVolume),FALSE,NULL);
		time = _rdtsc() - tm;
		DebugPrint("ReadSectors waiting completed in %I64u ms.\n", time);
		if(NT_SUCCESS(Status)) Status = ioStatus.Status;
	}
	/* FIXME: number of bytes actually read check? */
	return Status;
}

/** @} */
