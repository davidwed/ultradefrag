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
* NTFS related code.
*/

#include "driver.h"
#include "ntfs.h"

/*
* We should exclude here as many files as possible.
* All temporary files (in large meaning) 
* may be left fragmented ;-)
*
* Better is to include all files in full list.
* Because this method has simplest implementation ;-)
*/

/*
* FIXME: add data consistency checks everywhere,
* because NTFS volumes often have invalid data in MFT entries.
*/

/*
* FIXME: 
* 1. Volume ditry flag?
* 2. Reading non resident AttributeLists from disk.
*/

/*---------------------------------- NTFS related code -------------------------------------*/

UINT MftScanDirection = MFT_SCAN_RTL;
BOOLEAN MaxMftEntriesNumberUpdated = FALSE;
BOOLEAN ResidentDirectory;

/* sets dx->partition_type member */
void CheckForNtfsPartition(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PARTITION_INFORMATION part_info;
	NTFS_DATA ntfs_data;
	NTSTATUS status;
	IO_STATUS_BLOCK iosb;

	/*
	* Try to get fs type through IOCTL_DISK_GET_PARTITION_INFO request.
	* Works only on MBR-formatted disks. To retrieve information about 
	* GPT-formatted disks use IOCTL_DISK_GET_PARTITION_INFO_EX.
	*/
	status = ZwDeviceIoControlFile(dx->hVol,NULL,NULL,NULL,&iosb, \
				IOCTL_DISK_GET_PARTITION_INFO,NULL,0, \
				&part_info, sizeof(PARTITION_INFORMATION));
	if(status == STATUS_PENDING){
		if(nt4_system)
			NtWaitForSingleObject(dx->hVol,FALSE,NULL);
		else
			ZwWaitForSingleObject(dx->hVol,FALSE,NULL);
		status = iosb.Status;
	}
	if(NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- partition type: %u\n",NULL,part_info.PartitionType);
		if(part_info.PartitionType == 0x7){
			DebugPrint("-Ultradfg- NTFS found\n",NULL);
			dx->partition_type = NTFS_PARTITION;
			return;
		}
	} else {
		DebugPrint("-Ultradfg- Can't get fs info: %x!\n",NULL,(UINT)status);
	}

	/*
	* To ensure that we have a NTFS formatted partition
	* on GPT disks and when partition type is 0x27
	* FSCTL_GET_NTFS_VOLUME_DATA request can be used.
	*/
	status = ZwFsControlFile(dx->hVol,NULL,NULL,NULL,&iosb, \
				FSCTL_GET_NTFS_VOLUME_DATA,NULL,0, \
				&ntfs_data, sizeof(NTFS_DATA));
	if(status == STATUS_PENDING){
		if(nt4_system)
			NtWaitForSingleObject(dx->hVol,FALSE,NULL);
		else
			ZwWaitForSingleObject(dx->hVol,FALSE,NULL);
		status = iosb.Status;
	}
	if(NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- NTFS found\n",NULL);
		dx->partition_type = NTFS_PARTITION;
		return;
	} else {
		DebugPrint("-Ultradfg- Can't get ntfs info: %x!\n",NULL,status);
	}

	/*
	* Let's assume that we have a FAT32 formatted partition.
	* Currently we don't need more detailed information.
	*/
	dx->partition_type = FAT32_PARTITION;
	DebugPrint("-Ultradfg- NTFS not found\n",NULL);
}

NTSTATUS GetMftLayout(UDEFRAG_DEVICE_EXTENSION *dx)
{
	IO_STATUS_BLOCK iosb;
	NTFS_DATA ntfs_data;
	NTSTATUS status;
	ULONGLONG mft_len;

	status = ZwFsControlFile(dx->hVol,NULL,NULL,NULL,&iosb, \
				FSCTL_GET_NTFS_VOLUME_DATA,NULL,0, \
				&ntfs_data, sizeof(NTFS_DATA));
	if(status == STATUS_PENDING){
		if(nt4_system)
			NtWaitForSingleObject(dx->hVol,FALSE,NULL);
		else
			ZwWaitForSingleObject(dx->hVol,FALSE,NULL);
		status = iosb.Status;
	}
	if(!NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- Can't get ntfs info: %x!\n",NULL,status);
		return status;
	}

	mft_len = ProcessMftSpace(dx,&ntfs_data);

	dx->mft_size = mft_len * dx->bytes_per_cluster;
	DebugPrint("-Ultradfg- MFT size = %I64u bytes\n",NULL,dx->mft_size);

	dx->ntfs_record_size = ntfs_data.BytesPerFileRecordSegment;
	DebugPrint("-Ultradfg- NTFS record size = %u bytes\n",NULL,dx->ntfs_record_size);

	dx->max_mft_entries = dx->mft_size / dx->ntfs_record_size;
	DebugPrint("-Ultradfg- MFT contains no more than %I64u records\n",NULL,
		dx->max_mft_entries);
	
	//DbgPrintFreeSpaceList(dx);
	return STATUS_SUCCESS;
}

BOOLEAN ScanMFT(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PMY_FILE_INFORMATION pmfi;
	PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob = NULL;
	ULONG nfrob_size;
	ULONGLONG mft_id, ret_mft_id;
	NTSTATUS status;
	
	ULONGLONG tm, time;
	
	DebugPrint("-Ultradfg- MFT scan started!\n",NULL);

	/* Get information about MFT */
	status = GetMftLayout(dx);
	if(!NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- ProcessMFT() failed!\n",NULL);
		DebugPrint("-Ultradfg- MFT scan finished!\n",NULL);
		return FALSE; /* FIXME: better error handling */
	}

	/* allocate memory for NTFS record */
	nfrob_size = sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + dx->ntfs_record_size - 1;
	tm = _rdtsc();
	pnfrob = (PNTFS_FILE_RECORD_OUTPUT_BUFFER)AllocatePool(NonPagedPool,nfrob_size);
	if(!pnfrob){
		DebugPrint("-Ultradfg- no enough memory for NTFS_FILE_RECORD_OUTPUT_BUFFER!\n",NULL);
		DebugPrint("-Ultradfg- MFT scan finished!\n",NULL);
		return FALSE;
	}
	
	/* allocate memory for MY_FILE_INFORMATION structure */
	pmfi = (PMY_FILE_INFORMATION)AllocatePool(NonPagedPool,sizeof(MY_FILE_INFORMATION));
	if(!pmfi){
		DebugPrint("-Ultradfg- no enough memory for MY_FILE_INFORMATION structure!\n",NULL);
		Nt_ExFreePool(pnfrob);
		DebugPrint("-Ultradfg- MFT scan finished!\n",NULL);
		return FALSE;
	}
	
	/* read all MFT records sequentially */
	//MftBlockmap	= NULL;
	MaxMftEntriesNumberUpdated = FALSE;
	UpdateMaxMftEntriesNumber(dx,pnfrob,nfrob_size);
	mft_id = dx->max_mft_entries - 1;
	DebugPrint("\n",NULL);
	
	if(MaxMftEntriesNumberUpdated == FALSE){
		DebugPrint("-Ultradfg- UpdateMaxMftEntriesNumber() failed!\n",NULL);
		DebugPrint("-Ultradfg- MFT scan finished!\n",NULL);
		Nt_ExFreePool(pnfrob);
		Nt_ExFreePool(pmfi);
		//DestroyMftBlockmap();
		return FALSE; /* FIXME: better error handling */
	}

	/* Is MFT size an integral of NTFS record size? */
	/*if(MftClusters == 0 || \
	  (MftClusters * (ULONGLONG)dx->bytes_per_cluster % (ULONGLONG)dx->ntfs_record_size) || \
	  (dx->ntfs_record_size % dx->bytes_per_sector)){*/
	if(TRUE){
		MftScanDirection = MFT_SCAN_RTL;
		while(1){
			if(KeReadStateEvent(&stop_event) == 0x1) break;
			status = GetMftRecord(dx,pnfrob,nfrob_size,mft_id);
			if(!NT_SUCCESS(status)){
				if(mft_id == 0){
					DebugPrint("-Ultradfg- FSCTL_GET_NTFS_FILE_RECORD failed: %x!\n",NULL,status);
					Nt_ExFreePool(pnfrob);
					Nt_ExFreePool(pmfi);
					DebugPrint("-Ultradfg- MFT scan finished!\n",NULL);
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
			DebugPrint("-Ultradfg- NTFS record found, id = %I64u\n",NULL,ret_mft_id);
			#endif
			AnalyseMftRecord(dx,pnfrob,nfrob_size,pmfi);
			
			/* go to the next record */
			if(ret_mft_id == 0) break;
			mft_id = ret_mft_id - 1;
		}
	}/* else {
		DebugPrint("-Ultradfg- MFT size is an integral of NTFS record size :-D\n",NULL);
		ScanMftDirectly(dx,pnfrob,nfrob_size,pmfi);
	}*/
	
	/* free allocated memory */
	Nt_ExFreePool(pnfrob);
	Nt_ExFreePool(pmfi);
	//DestroyMftBlockmap();
	
	/* Build paths. */
	BuildPaths(dx);

	DebugPrint("-Ultradfg- MFT scan finished!\n",NULL);
	time = _rdtsc() - tm;
	DbgPrint("MFT scan needs %I64u ms\n",time);
	return TRUE;
}

void UpdateMaxMftEntriesNumber(UDEFRAG_DEVICE_EXTENSION *dx,
		PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,ULONG nfrob_size)
{
	NTSTATUS status;
	PFILE_RECORD_HEADER pfrh;

	/* Get record for FILE_MFT using WinAPI, because MftBitmap is not ready yet. */
	status = GetMftRecord(dx,pnfrob,nfrob_size,FILE_MFT);
	if(!NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- UpdateMaxMftEntriesNumber(): FSCTL_GET_NTFS_FILE_RECORD failed: %x!\n",NULL,status);
		return;
	}
	if(GetMftIdFromFRN(pnfrob->FileReferenceNumber) != FILE_MFT){
		DebugPrint("-Ultradfg- UpdateMaxMftEntriesNumber() failed - unable to get FILE_MFT record.\n",NULL);
		return;
	}

	/* Find DATA attribute. */
	pfrh = (PFILE_RECORD_HEADER)pnfrob->FileRecordBuffer;
	if(pfrh->Ntfs.Type != TAG('F','I','L','E')){
		DebugPrint("-Ultradfg- UpdateMaxMftEntriesNumber() failed - FILE_MFT record has invalid type %u.\n",
			NULL,pfrh->Ntfs.Type);
		return;
	}
	if(!(pfrh->Flags & 0x1)){
		DebugPrint("-Ultradfg- UpdateMaxMftEntriesNumber() failed - FILE_MFT record marked as free.\n",NULL);
		return; /* skip free records */
	}
	
	EnumerateAttributes(dx,pfrh,UpdateMaxMftEntriesNumberCallback,NULL);
}

/* pmfi == NULL */
void __stdcall UpdateMaxMftEntriesNumberCallback(UDEFRAG_DEVICE_EXTENSION *dx,
												 PATTRIBUTE pattr,PMY_FILE_INFORMATION pmfi)
{
	PNONRESIDENT_ATTRIBUTE pnr_attr;

	if(pattr->Nonresident && pattr->AttributeType == AttributeData){
		pnr_attr = (PNONRESIDENT_ATTRIBUTE)pattr;
		if(dx->ntfs_record_size){
			dx->max_mft_entries = pnr_attr->DataSize / dx->ntfs_record_size;
			//MftClusters = 0;
			//MftBlockmap = NULL;
			//ProcessMFTRunList(dx,pnr_attr);
			/* FIXME: check correctness of MftBlockmap */
			MaxMftEntriesNumberUpdated = TRUE;
		}
		DebugPrint("-Ultradfg- MFT contains no more than %I64u records (more accurately)\n",NULL,
			dx->max_mft_entries);
	}
}

/* it seems that Win API works very slow here */
NTSTATUS GetMftRecord(UDEFRAG_DEVICE_EXTENSION *dx,PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,
					  ULONG nfrob_size,ULONGLONG mft_id)
{
	NTFS_FILE_RECORD_INPUT_BUFFER nfrib;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;

	nfrib.FileReferenceNumber = mft_id;

	status = ZwFsControlFile(dx->hVol,NULL,NULL,NULL,&iosb, \
			FSCTL_GET_NTFS_FILE_RECORD, \
			&nfrib,sizeof(nfrib), \
			pnfrob, nfrob_size);
	if(status == STATUS_PENDING){
		if(nt4_system)
			NtWaitForSingleObject(dx->hVol,FALSE,NULL);
		else
			ZwWaitForSingleObject(dx->hVol,FALSE,NULL);
		status = iosb.Status;
	}
	return status;
}

void AnalyseMftRecord(UDEFRAG_DEVICE_EXTENSION *dx,PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,
					  ULONG nfrob_size,PMY_FILE_INFORMATION pmfi)
{
	PFILE_RECORD_HEADER pfrh;
	#ifdef DETAILED_LOGGING
	USHORT Flags;
	#endif
	
	/* analyse record's header */
	pfrh = (PFILE_RECORD_HEADER)pnfrob->FileRecordBuffer;

	if(pfrh->Ntfs.Type != TAG('F','I','L','E')) return;
	if(!(pfrh->Flags & 0x1)) return; /* skip free records */
	
	/* analyse file record */
	#ifdef DETAILED_LOGGING
	Flags = pfrh->Flags;
	if(Flags & 0x2) DebugPrint("-Ultradfg- Directory\n",NULL); /* May be wrong? */
	#endif

	if(pfrh->BaseFileRecord){ /* skip child records, we will analyse them later */
		//DebugPrint("-Ultradfg- BaseFileRecord (id) = %I64u\n",
		//	NULL,GetMftIdFromFRN(pfrh->BaseFileRecord));
		//DebugPrint("\n",NULL);
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

	ResidentDirectory = TRUE; /* let's assume that we have a resident directory */
	
	/* skip AttributeList attributes */
	EnumerateAttributes(dx,pfrh,AnalyseAttributeCallback,pmfi);
	
	/* analyse AttributeList attributes */
	EnumerateAttributes(dx,pfrh,AnalyseAttributeListCallback,pmfi);
	
	/* add resident directories to filelist - required by BuildPaths() */
	if(pmfi->IsDirectory && ResidentDirectory)
		AddResidentDirectoryToFileList(dx,pmfi);
	
	/* update cluster map and statistics */
	UpdateClusterMapAndStatistics(dx,pmfi);

	#ifdef DETAILED_LOGGING
	DebugPrint("\n",NULL);
	#endif
}

/* dx->ntfs_record_size must be initialized before */
void EnumerateAttributes(UDEFRAG_DEVICE_EXTENSION *dx,PFILE_RECORD_HEADER pfrh,
						 ATTRHANDLER_PROC ahp,PMY_FILE_INFORMATION pmfi)
{
	PATTRIBUTE pattr;
	ULONG attr_length;
	USHORT attr_offset;

	attr_offset = pfrh->AttributeOffset;
	pattr = (PATTRIBUTE)((char *)pfrh + attr_offset);

	while(pattr){
		if(KeReadStateEvent(&stop_event) == 0x1) break;

		/* is an attribute header inside a record bounds? */
		if(attr_offset + sizeof(ATTRIBUTE) > pfrh->BytesInUse || \
			attr_offset + sizeof(ATTRIBUTE) > dx->ntfs_record_size)	break;
		
		/* is it a valid attribute */
		if(pattr->AttributeType == 0xffffffff) break;
		if(pattr->AttributeType == 0x0) break;
		if(pattr->Length == 0) break;

		/* is an attribute inside a record bounds? */
		if(attr_offset + pattr->Length > pfrh->BytesInUse || \
			attr_offset + pattr->Length > dx->ntfs_record_size) break;

		/* call specified callback procedure */
		ahp(dx,pattr,pmfi);
		
		/* go to the next attribute */
		attr_length = pattr->Length;
		attr_offset += (USHORT)(attr_length);
		pattr = (PATTRIBUTE)((char *)pattr + attr_length);
	}
}

void __stdcall AnalyseAttributeCallback(UDEFRAG_DEVICE_EXTENSION *dx,
										PATTRIBUTE pattr,PMY_FILE_INFORMATION pmfi)
{
	if(pattr->AttributeType != AttributeAttributeList)
		AnalyseAttribute(dx,pattr,pmfi);
}

void __stdcall AnalyseAttributeListCallback(UDEFRAG_DEVICE_EXTENSION *dx,
										    PATTRIBUTE pattr,PMY_FILE_INFORMATION pmfi)
{
	if(pattr->AttributeType == AttributeAttributeList)
		AnalyseAttribute(dx,pattr,pmfi);
}

void AnalyseAttribute(UDEFRAG_DEVICE_EXTENSION *dx,PATTRIBUTE pattr,PMY_FILE_INFORMATION pmfi)
{
	if(pattr->Nonresident) AnalyseNonResidentAttribute(dx,(PNONRESIDENT_ATTRIBUTE)pattr,pmfi);
	else AnalyseResidentAttribute(dx,(PRESIDENT_ATTRIBUTE)pattr,pmfi);
}

/*
* Resident attributes never contain fragmented data,
* because they are placed in MFT record entirely.
* We analyse resident attributes only if we need some
* information stored in them.
*/
void AnalyseResidentAttribute(UDEFRAG_DEVICE_EXTENSION *dx,PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi)
{
	if(pr_attr->ValueOffset == 0 || pr_attr->ValueLength == 0) return;
	
	switch(pr_attr->Attribute.AttributeType){
	case AttributeStandardInformation: /* always resident */
		GetFileFlags(dx,pr_attr,pmfi); break;

	case AttributeFileName: /* always resident */
		GetFileName(dx,pr_attr,pmfi); break;

	case AttributeVolumeInformation: /* always resident */
		GetVolumeInformation(dx,pr_attr); break;

	case AttributeAttributeList:
		//DebugPrint("Resident AttributeList found!\n",NULL);
		AnalyseResidentAttributeList(dx,pr_attr,pmfi);
		break;

	/*case AttributeIndexRoot:  // always resident */
	/*case AttributeIndexAllocation:*/

	case AttributeReparsePoint:
		CheckReparsePointResident(dx,pr_attr,pmfi);
		break;

	default:
		break;
	}
}

void GetFileFlags(UDEFRAG_DEVICE_EXTENSION *dx,PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi)
{
	PSTANDARD_INFORMATION psi;
	ULONG Flags;
	
	psi = (PSTANDARD_INFORMATION)((char *)pr_attr + pr_attr->ValueOffset);
	Flags = psi->FileAttributes;
	pmfi->Flags = Flags;
}

/* MY_FILE_INFORMATION structure MUST be initialized before! */
void GetFileName(UDEFRAG_DEVICE_EXTENSION *dx,PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi)
{
	PFILENAME_ATTRIBUTE pfn_attr;
	short *name;
	UCHAR name_type;
	ULONGLONG parent_mft_id;
	
	pfn_attr = (PFILENAME_ATTRIBUTE)((char *)pr_attr + pr_attr->ValueOffset);
	parent_mft_id = GetMftIdFromFRN(pfn_attr->DirectoryFileReferenceNumber);
	
	/* allocate memory */
	name = (short *)AllocatePool(NonPagedPool,(MAX_PATH + 1) * sizeof(short));
	if(!name){
		DebugPrint("-Ultradfg- Cannot allocate memory for GetFileName()!\n",NULL);
		return;
	}

	if(pfn_attr->NameLength){
		wcsncpy(name,pfn_attr->Name,pfn_attr->NameLength);
		name[pfn_attr->NameLength] = 0;
		//DbgPrint("-Ultradfg- Filename = %ws, parent id = %I64u\n",name,parent_mft_id);
		/* update pmfi members */
		pmfi->ParentDirectoryMftId = parent_mft_id;
		/* save filename */
		name_type = pfn_attr->NameType;
		UpdateFileName(dx,pmfi,name,name_type);
	}// else DebugPrint("-Ultradfg- File has no name\n",NULL);

	/* free allocated memory */
	Nt_ExFreePool(name);
}

void UpdateFileName(UDEFRAG_DEVICE_EXTENSION *dx,PMY_FILE_INFORMATION pmfi,WCHAR *name,UCHAR name_type)
{
	/* compare name type with type of saved name */
	if(pmfi->Name[0] == 0 || pmfi->NameType == FILENAME_DOS || \
	  ((pmfi->NameType & FILENAME_WIN32) && (name_type == FILENAME_POSIX))){
		wcsncpy(pmfi->Name,name,MAX_NTFS_PATH);
		pmfi->Name[MAX_NTFS_PATH-1] = 0;
		pmfi->NameType = name_type;
	}
}

void GetVolumeInformation(UDEFRAG_DEVICE_EXTENSION *dx,PRESIDENT_ATTRIBUTE pr_attr)
{
	PVOLUME_INFORMATION pvi;
	ULONG mj_ver, mn_ver;
	BOOLEAN dirty_flag = FALSE;
	
	pvi = (PVOLUME_INFORMATION)((char *)pr_attr + pr_attr->ValueOffset);
	mj_ver = (ULONG)pvi->MajorVersion;
	mn_ver = (ULONG)pvi->MinorVersion;
	if(pvi->Flags & 0x1) dirty_flag = TRUE;
	DebugPrint("-Ultradfg- NTFS Version %u.%u\n",NULL,mj_ver,mn_ver);
	if(dirty_flag) DebugPrint("-Ultradfg- Volume is dirty!\n",NULL);
}

void CheckReparsePointResident(UDEFRAG_DEVICE_EXTENSION *dx,PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi)
{
	PREPARSE_POINT prp;
	ULONG tag;
	
	prp = (PREPARSE_POINT)((char *)pr_attr + pr_attr->ValueOffset);
	tag = prp->ReparseTag;
	DebugPrint("-Ultradfg- Reparse tag = 0x%x\n",NULL,tag);
	
	pmfi->IsReparsePoint = TRUE;
}

/* Analyse attributes of the current file packed in another mft records. */
void AnalyseResidentAttributeList(UDEFRAG_DEVICE_EXTENSION *dx,PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi)
{
	ULONG n_entries;
	PATTRIBUTE_LIST attr_list_entry;
	
	attr_list_entry = (PATTRIBUTE_LIST)((char *)pr_attr + pr_attr->ValueOffset);
	n_entries = pr_attr->ValueLength / sizeof(ATTRIBUTE_LIST);
	while(n_entries){
		if(KeReadStateEvent(&stop_event) == 0x1) break;
		n_entries --;
		/* is it a valid attribute */
		if(attr_list_entry->AttributeType == 0xffffffff) break;
		if(attr_list_entry->AttributeType == 0x0) break;
		if(attr_list_entry->Length == 0) break;
		AnalyseAttributeFromAttributeList(dx,attr_list_entry,pmfi);
		/* go to the next attribute list entry */
		attr_list_entry = (PATTRIBUTE_LIST)((char *)attr_list_entry + sizeof(ATTRIBUTE_LIST));
	}
}

void AnalyseAttributeFromAttributeList(UDEFRAG_DEVICE_EXTENSION *dx,PATTRIBUTE_LIST attr_list_entry,PMY_FILE_INFORMATION pmfi)
{
	PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob = NULL;
	ULONG nfrob_size;
	ULONGLONG child_record_mft_id;
	NTSTATUS status;
	PFILE_RECORD_HEADER pfrh;
	
	/* skip entries describing attributes stored in base mft record */
	if(GetMftIdFromFRN(attr_list_entry->FileReferenceNumber) == pmfi->BaseMftId){
		//DebugPrint("Resident attribute found, type = 0x%x\n",NULL,attr_list_entry->AttributeType);
		return;
	}
	
	/* allocate memory for another mft record */
	nfrob_size = sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + dx->ntfs_record_size - 1;
	pnfrob = (PNTFS_FILE_RECORD_OUTPUT_BUFFER)AllocatePool(NonPagedPool,nfrob_size);
	if(!pnfrob){
		DebugPrint("-Ultradfg- AnalyseAttributeFromAttributeList():\n",NULL);
		DebugPrint("-Ultradfg- no enough memory for NTFS_FILE_RECORD_OUTPUT_BUFFER!\n",NULL);
		return;
	}

	/* get another mft record */
	child_record_mft_id = GetMftIdFromFRN(attr_list_entry->FileReferenceNumber);
	status = GetMftRecord(dx,pnfrob,nfrob_size,child_record_mft_id);
	if(!NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- AnalyseAttributeFromAttributeList(): FSCTL_GET_NTFS_FILE_RECORD failed: %x!\n",NULL,status);
		Nt_ExFreePool(pnfrob);
		return;
	}
	if(GetMftIdFromFRN(pnfrob->FileReferenceNumber) != child_record_mft_id){
		DebugPrint("-Ultradfg- AnalyseAttributeFromAttributeList() failed - unable to get %I64u record.\n",
			NULL,child_record_mft_id);
		Nt_ExFreePool(pnfrob);
		return;
	}

	/* Analyse all nonresident attributes. */
	pfrh = (PFILE_RECORD_HEADER)pnfrob->FileRecordBuffer;
	if(pfrh->Ntfs.Type != TAG('F','I','L','E')){
		DebugPrint("-Ultradfg- AnalyseAttributeFromAttributeList() failed - %I64u record has invalid type %u.\n",
			NULL,child_record_mft_id,pfrh->Ntfs.Type);
		Nt_ExFreePool(pnfrob);
		return;
	}
	if(!(pfrh->Flags & 0x1)){
		DebugPrint("-Ultradfg- AnalyseAttributeFromAttributeList() failed\n",NULL);
		DebugPrint("-Ultradfg- %I64u record marked as free.\n",NULL,child_record_mft_id);
		Nt_ExFreePool(pnfrob);
		return; /* skip free records */
	}

	if(pfrh->BaseFileRecord == 0){
		DebugPrint("-Ultradfg- AnalyseAttributeFromAttributeList() failed - %I64u is not a child record.\n",
			NULL,child_record_mft_id);
		Nt_ExFreePool(pnfrob);
		return;
	}
	
	EnumerateAttributes(dx,pfrh,AnalyseAttributeFromAttributeListCallback,pmfi);

	/* free allocated memory */
	Nt_ExFreePool(pnfrob);
}

void __stdcall AnalyseAttributeFromAttributeListCallback(UDEFRAG_DEVICE_EXTENSION *dx,
														 PATTRIBUTE pattr,PMY_FILE_INFORMATION pmfi)
{
	if(pattr->Nonresident) AnalyseNonResidentAttribute(dx,(PNONRESIDENT_ATTRIBUTE)pattr,pmfi);
}

/*
* These attributes may contain fragmented data.
* 
* Standard Information and File Name are resident and 
* always before nonresident attributes. So we will always have 
* file flags and name before this call.
*/
void AnalyseNonResidentAttribute(UDEFRAG_DEVICE_EXTENSION *dx,PNONRESIDENT_ATTRIBUTE pnr_attr,PMY_FILE_INFORMATION pmfi)
{
	WCHAR *default_attr_name = NULL;
	short *attr_name;
	short *full_path;

	/* allocate memory */
	attr_name = (short *)AllocatePool(NonPagedPool,(MAX_NTFS_PATH + 1) * sizeof(short));
	if(!attr_name){
		DebugPrint("-Ultradfg- Cannot allocate memory for attr_name in AnalyseNonResidentAttribute()!\n",NULL);
		return;
	}
	full_path = (short *)AllocatePool(NonPagedPool,(MAX_NTFS_PATH + 1) * sizeof(short));
	if(!full_path){
		DebugPrint("-Ultradfg- Cannot allocate memory for full_path in AnalyseNonResidentAttribute()!\n",NULL);
		Nt_ExFreePool(attr_name);
		return;
	}
	
	/* print characteristics of the attribute */
//	DebugPrint("-Ultradfg- type = 0x%x NonResident\n",NULL,pattr->AttributeType);
//	if(pnr_attr->Attribute.Flags & 0x1) DebugPrint("-Ultradfg- Compressed\n",NULL);
	
	switch(pnr_attr->Attribute.AttributeType){
	case AttributeAttributeList: /* always nonresident? */
		//DebugPrint("Nonresident AttributeList found!\n",NULL);
		default_attr_name = L"$ATTRIBUTE_LIST";
		break;
    case AttributeEA:
		default_attr_name = L"$EA";
		break;
    case AttributeEAInformation:
		default_attr_name = L"$EA_INFORMATION";
		break;
    case AttributeSecurityDescriptor:
		default_attr_name = L"$SECURITY_DESCRIPTOR";
		break;
	case AttributeData:
		default_attr_name = L"$DATA";
		break;
	case AttributeIndexRoot:
		default_attr_name = L"$INDEX_ROOT";
		break;
	case AttributeIndexAllocation:
		default_attr_name = L"$INDEX_ALLOCATION";
		break;
	case AttributeBitmap:
		default_attr_name = L"$BITMAP";
		break;
	case AttributeReparsePoint:
		pmfi->IsReparsePoint = TRUE;
		default_attr_name = L"$REPARSE_POINT";
		break;
	case AttributeLoggedUtulityStream:
		/* used by EFS */
		default_attr_name = L"$LOGGED_UTILITY_STREAM";
		break;
	default:
		break;
	}
	
	if(default_attr_name == NULL){
		Nt_ExFreePool(attr_name);
		Nt_ExFreePool(full_path);
		return;
	}

	/* do not append $I30 attributes - required by GetFileNameAndParentMftId() */
	
	if(pnr_attr->Attribute.NameLength){
		/* append a name of attribute to filename */
		/* NameLength is always less than MAX_PATH :) */
		wcsncpy(attr_name,(short *)((char *)pnr_attr + pnr_attr->Attribute.NameOffset),
			pnr_attr->Attribute.NameLength);
		attr_name[pnr_attr->Attribute.NameLength] = 0;
		if(wcscmp(attr_name,L"$I30") != 0){
			_snwprintf(full_path,MAX_NTFS_PATH,L"%s:%s",pmfi->Name,attr_name);
		} else {
			wcsncpy(full_path,pmfi->Name,MAX_NTFS_PATH);
			ResidentDirectory = FALSE;
		}
	} else {
		/* append default name of attribute to filename */
		if(wcscmp(default_attr_name,L"$DATA")){
			_snwprintf(full_path,MAX_NTFS_PATH,L"%s:%s",pmfi->Name,default_attr_name);
		} else {
			wcsncpy(full_path,pmfi->Name,MAX_NTFS_PATH);
			ResidentDirectory = FALSE; /* let's assume that */
		}
	}
	full_path[MAX_NTFS_PATH - 1] = 0;

	/* skip all filtered out attributes */
	//if(AttributeNeedsToBeDefragmented(dx,full_path,pnr_attr->DataSize,pmfi))
		ProcessRunList(dx,full_path,pnr_attr,pmfi);

	/* free allocated memory */
	Nt_ExFreePool(attr_name);
	Nt_ExFreePool(full_path);
}

ULONG RunLength(PUCHAR run)
{
	return (*run & 0xf) + ((*run >> 4) & 0xf) + 1;
}

LONGLONG RunLCN(PUCHAR run)
{
	LONG i;
	UCHAR n1 = *run & 0xf;
	UCHAR n2 = (*run >> 4) & 0xf;
	LONGLONG lcn = (n2 == 0) ? 0 : (LONGLONG)(((signed char *)run)[n1 + n2]);
	
	for(i = n1 + n2 - 1; i > n1; i--)
		lcn = (lcn << 8) + run[i];
	return lcn;
}

ULONGLONG RunCount(PUCHAR run)
{
	ULONG i;
	UCHAR n = *run & 0xf;
	ULONGLONG count = 0;
	
	for(i = n; i > 0; i--)
		count = (count << 8) + run[i];
	return count;
}

/* Saves information about VCN/LCN pairs of the attribute specified by full_path parameter. */
void ProcessRunList(UDEFRAG_DEVICE_EXTENSION *dx,WCHAR *full_path,PNONRESIDENT_ATTRIBUTE pnr_attr,PMY_FILE_INFORMATION pmfi)
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
	pfn = FindFileListEntryForTheAttribute(dx,full_path,pmfi);
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
			if((lcn + length) > dx->clusters_total){
				DebugPrint("-Ultradfg- Error in MFT found, run Check Disk program!\n",NULL);
				break;
			}
			ProcessRun(dx,full_path,pmfi,pfn,vcn,length,lcn);
		}
		
		/* go to the next run */
		run += RunLength(run);
		vcn += length;
	}
}

/*------------------------ Defragmentation related code ------------------------------*/

/* returns complete MFT length, in clusters */
ULONGLONG ProcessMftSpace(UDEFRAG_DEVICE_EXTENSION *dx,PNTFS_DATA nd)
{
	ULONGLONG start,len,mft_len = 0;

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
	DebugPrint("-Ultradfg- MFT_file   : start : length\n",NULL);

	/* $MFT */
	start = nd->MftStartLcn.QuadPart;
	if(nd->BytesPerCluster)
		len = nd->MftValidDataLength.QuadPart / nd->BytesPerCluster;
	else
		len = 0;
	DebugPrint("-Ultradfg- $MFT       :%I64u :%I64u\n",NULL,start,len);
	ProcessBlock(dx,start,len,MFT_SPACE,SYSTEM_SPACE);
	CleanupFreeSpaceList(dx,start,len);
	mft_len += len;

	/* $MFT2 */
	start = nd->MftZoneStart.QuadPart;
	len = nd->MftZoneEnd.QuadPart - nd->MftZoneStart.QuadPart;
	DebugPrint("-Ultradfg- $MFT2      :%I64u :%I64u\n",NULL,start,len);
	ProcessBlock(dx,start,len,MFT_SPACE,SYSTEM_SPACE);
	CleanupFreeSpaceList(dx,start,len);
	mft_len += len;

	/* $MFTMirror */
	start = nd->Mft2StartLcn.QuadPart;
	DebugPrint("-Ultradfg- $MFTMirror :%I64u :1\n",NULL,start);
	ProcessBlock(dx,start,1,MFT_SPACE,SYSTEM_SPACE);
	CleanupFreeSpaceList(dx,start,1);
	mft_len ++;
	
	return mft_len;
}

/*
* This code may slow down the program: 
* for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr) for each file
*/

void ProcessRun(UDEFRAG_DEVICE_EXTENSION *dx,WCHAR *full_path,PMY_FILE_INFORMATION pmfi,
				PFILENAME pfn,ULONGLONG vcn,ULONGLONG length,ULONGLONG lcn)
{
	PBLOCKMAP block, prev_block = NULL;
	
/*	if(!wcscmp(full_path,L"\\??\\L:\\go.zip")){
		DbgPrint("VCN %I64u : LEN %I64u : LCN %I64u\n",vcn,length,lcn);
	}
*/
	/* add information to blockmap member of specified pfn structure */
	if(pfn->blockmap) prev_block = pfn->blockmap->prev_ptr;
	block = (PBLOCKMAP)InsertItem((PLIST *)&pfn->blockmap,(PLIST)prev_block,sizeof(BLOCKMAP),PagedPool);
	if(!block) return;
	
	block->vcn = vcn;
	block->length = length;
	block->lcn = lcn;

	pfn->n_fragments ++;
	pfn->clusters_total += block->length;
	/*
	* Sometimes normal file has more than one fragment, 
	* but is not fragmented yet! *CRAZY* 
	*/
	if(block != pfn->blockmap && \
	  block->lcn != (block->prev_ptr->lcn + block->prev_ptr->length))
		pfn->is_fragm = TRUE;
}

PFILENAME FindFileListEntryForTheAttribute(UDEFRAG_DEVICE_EXTENSION *dx,WCHAR *full_path,PMY_FILE_INFORMATION pmfi)
{
	PFILENAME pfn;
	
	/* few entries (attributes) may have the same mft id */
	for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
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
		if(pfn->next_ptr == dx->filelist) break;
	}
	
	pfn = (PFILENAME)InsertItem((PLIST *)&dx->filelist,NULL,sizeof(FILENAME),PagedPool);
	if(pfn == NULL) return NULL;
	
	/* fill a name member of the created structure */
	if(!RtlCreateUnicodeString(&pfn->name,full_path)){
		DebugPrint2("-Ultradfg- no enough memory for pfn->name initialization!\n",NULL);
		RemoveItem((PLIST *)&dx->filelist,(LIST *)pfn);
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
	return pfn;
}

void UpdateClusterMapAndStatistics(UDEFRAG_DEVICE_EXTENSION *dx,PMY_FILE_INFORMATION pmfi)
{
	PFILENAME pfn;
	ULONGLONG filesize;

	/* All stuff commented with C++ style comments was moved to BuildPaths() function. */
	for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
		/* only the first few entries may have dirty flag set */
		if(pfn->is_dirty == FALSE) break;

		pfn->is_dirty = FALSE;
		/* 1. fill all members of pfn */
		/* 1.1 set flags in pfn ??? */
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
			DebugPrint("-Ultradfg- Reparse point found\n",pfn->name.Buffer);
			pfn->is_reparse_point = TRUE;
		} else pfn->is_reparse_point = FALSE;
		/* 1.2 calculate size of attribute data */
		filesize = pfn->clusters_total * dx->bytes_per_cluster;
		if(dx->sizelimit && filesize > dx->sizelimit) pfn->is_overlimit = TRUE;
		else pfn->is_overlimit = FALSE;
		/* 1.3 detect temporary files and other unwanted stuff */
		if(TemporaryStuffDetected(dx,pmfi)) pfn->is_filtered = TRUE;
		else pfn->is_filtered = FALSE;
		/* 1.4 detect sparse files */
		if(pmfi->Flags & FILE_ATTRIBUTE_SPARSE_FILE)
			DebugPrint("-Ultradfg- Sparse file found\n",pfn->name.Buffer);
		/* 1.5 detect encrypted files */
		if(pmfi->Flags & FILE_ATTRIBUTE_ENCRYPTED)
			DbgPrint("-Ultradfg- Encrypted file found %ws\n",pfn->name.Buffer);
		/* 2. redraw cluster map */
//		MarkSpace(dx,pfn,SYSTEM_SPACE);
		/* 3. update statistics */
		dx->filecounter ++;
		if(pfn->is_dir) dx->dircounter ++;
		if(pfn->is_compressed) dx->compressedcounter ++;
		/* skip here filtered out and big files and reparse points */
//		if(pfn->is_fragm && !pfn->is_filtered && !pfn->is_overlimit && !pfn->is_reparse_point){
//			dx->fragmfilecounter ++;
//			dx->fragmcounter += pfn->n_fragments;
//		} else {
//			dx->fragmcounter ++;
//		}
		dx->processed_clusters += pfn->clusters_total;

		if(pfn->next_ptr == dx->filelist) break;
	}
}

BOOLEAN TemporaryStuffDetected(UDEFRAG_DEVICE_EXTENSION *dx,PMY_FILE_INFORMATION pmfi)
{
	/* skip temporary files ;-) */
	if(pmfi->Flags & FILE_ATTRIBUTE_TEMPORARY){
		DebugPrint2("-Ultradfg- Temporary file found\n",pmfi->Name);
		return TRUE;
	}
	return FALSE;
}

/* that's unbelievable, but this function runs fast */
BOOLEAN UnwantedStuffDetected(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	UNICODE_STRING us;

	/* skip all unwanted files by user defined patterns */
	if(!RtlCreateUnicodeString(&us,pfn->name.Buffer)){
		DebugPrint2("-Ultradfg- cannot allocate memory for UnwantedStuffDetected()!\n",NULL);
		return FALSE;
	}
	_wcslwr(us.Buffer);

	if(dx->in_filter.buffer){
		if(!IsStringInFilter(us.Buffer,&dx->in_filter)){
			RtlFreeUnicodeString(&us); return TRUE; /* not included */
		}
	}

	if(dx->ex_filter.buffer){
		if(IsStringInFilter(us.Buffer,&dx->ex_filter)){
			RtlFreeUnicodeString(&us); return TRUE; /* excluded */
		}
	}
	RtlFreeUnicodeString(&us);

	return FALSE;
}

PMY_FILE_ENTRY mf = NULL; /* pointer to array of MY_FILE_ENTRY structures */
ULONG n_entries;
BOOLEAN mf_allocated = FALSE;

void BuildPaths(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PFILENAME pfn;
	ULONG i;

	/* prepare data for fast binary search */
	mf_allocated = FALSE;
	n_entries = dx->filecounter;
	if(n_entries){
		mf = (PMY_FILE_ENTRY)AllocatePool(PagedPool,n_entries * sizeof(MY_FILE_ENTRY));
		if(mf != NULL) mf_allocated = TRUE;
		else DebugPrint("-Ultradfg- cannot allocate %u bytes of memory for mf array!\n",
				NULL,n_entries * sizeof(MY_FILE_ENTRY));
	}
	if(mf_allocated){
		/* fill array */
		i = 0;
		for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
			mf[i].mft_id = pfn->BaseMftId;
			mf[i].pfn = pfn;
			if(i == (n_entries - 1)) break;
			i++;
			if(pfn->next_ptr == dx->filelist) break;
		}
	}
	
	for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
		BuildPath2(dx,pfn);
		if(UnwantedStuffDetected(dx,pfn)) pfn->is_filtered = TRUE;
		else pfn->is_filtered = FALSE;
		MarkSpace(dx,pfn,SYSTEM_SPACE);
		/* skip here filtered out and big files and reparse points */
		if(pfn->is_fragm && !pfn->is_filtered && !pfn->is_overlimit && !pfn->is_reparse_point){
			dx->fragmfilecounter ++;
			dx->fragmcounter += pfn->n_fragments;
		} else {
			dx->fragmcounter ++;
		}
		if(pfn->next_ptr == dx->filelist) break;
	}
	
	/* free allocated resources */
	if(mf_allocated) ExFreePoolSafe(mf);
}

void BuildPath2(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	WCHAR *buffer1;
	WCHAR *buffer2;
	ULONG offset;
	ULONGLONG mft_id,parent_mft_id;
	ULONG name_length;
	WCHAR header[] = L"\\??\\A:";
	BOOLEAN FullPathRetrieved = FALSE;
	UNICODE_STRING us;
	
	/* allocate memory */
	buffer1 = (WCHAR *)AllocatePool(NonPagedPool,(MAX_NTFS_PATH) * sizeof(short));
	if(!buffer1){
		DebugPrint("-Ultradfg- BuildPath2(): cannot allocate memory for buffer1\n",NULL);
		return;
	}
	buffer2 = (WCHAR *)AllocatePool(NonPagedPool,(MAX_NTFS_PATH) * sizeof(short));
	if(!buffer2){
		DebugPrint("-Ultradfg- BuildPath2(): cannot allocate memory for buffer2\n",NULL);
		Nt_ExFreePool(buffer1);
		return;
	}

	/* terminate buffer1 with zero */
	offset = MAX_NTFS_PATH - 1;
	buffer1[offset] = 0; /* terminating zero */
	offset --;
	
	/* copy filename to the right side of buffer1 */
	name_length = wcslen(pfn->name.Buffer);
	if(offset < (name_length - 1)){
		DebugPrint("-Ultradfg- BuildPath2(): filename is too long (%u characters)\n",
			pfn->name.Buffer,name_length);
		Nt_ExFreePool(buffer1);
		Nt_ExFreePool(buffer2);
		return;
	}

	offset -= (name_length - 1);
	wcsncpy(buffer1 + offset,pfn->name.Buffer,name_length);

	if(offset == 0) goto path_is_too_long;
	offset --;
	
	/* add backslash */
	buffer1[offset] = '\\';
	offset --;
	
	if(offset == 0) goto path_is_too_long;

	parent_mft_id = pfn->ParentDirectoryMftId;
	while(parent_mft_id != FILE_root){
		if(KeReadStateEvent(&stop_event) == 0x1) break;
		mft_id = parent_mft_id;
		FullPathRetrieved = GetFileNameAndParentMftId(dx,mft_id,&parent_mft_id,buffer2,MAX_NTFS_PATH);
		if(buffer2[0] == 0){
			DebugPrint("-Ultradfg- BuildPath2(): cannot retrieve parent directory name!\n",NULL);
			goto build_path_done;
		}
		//DbgPrint("%ws\n",buffer2);
		/* append buffer2 contents to the right side of buffer1 */
		name_length = wcslen(buffer2);
		if(offset < (name_length - 1)) goto path_is_too_long;
		offset -= (name_length - 1);
		wcsncpy(buffer1 + offset,buffer2,name_length);
		
		if(FullPathRetrieved) goto update_filename;
		
		if(offset == 0) goto path_is_too_long;
		offset --;
		/* add backslash */
		buffer1[offset] = '\\';
		offset --;
		if(offset == 0) goto path_is_too_long;
	}
	
	/* append volume letter */
	header[4] = dx->letter;
	name_length = wcslen(header);
	if(offset < (name_length - 1)) goto path_is_too_long;
	offset -= (name_length - 1);
	wcsncpy(buffer1 + offset,header,name_length);

update_filename:	
	/* replace pfn->name contents with full path */
	//wcsncpy(pmfi->Name,buffer1 + offset,MAX_NTFS_PATH);
	//pmfi->Name[MAX_NTFS_PATH - 1] = 0;
	if(!RtlCreateUnicodeString(&us,buffer1 + offset)){
		DebugPrint2("-Ultradfg- cannot allocate memory for BuildPath2()!\n",NULL);
	} else {
		RtlFreeUnicodeString(&(pfn->name));
		pfn->name.Buffer = us.Buffer;
		pfn->name.Length = us.Length;
		pfn->name.MaximumLength = us.MaximumLength;
	}
	pfn->PathBuilt = TRUE;
	
	#ifdef DETAILED_LOGGING
	DebugPrint("-Ultradfg- FULL PATH =\n",pfn->name.Buffer);
	#endif
	
build_path_done:
	Nt_ExFreePool(buffer1);
	Nt_ExFreePool(buffer2);
	return;
	
path_is_too_long:
	DebugPrint("-Ultradfg- BuildPath2(): path is too long:\n",buffer1);
	Nt_ExFreePool(buffer1);
	Nt_ExFreePool(buffer2);
}

BOOLEAN GetFileNameAndParentMftId(UDEFRAG_DEVICE_EXTENSION *dx,
		ULONGLONG mft_id,ULONGLONG *parent_mft_id,WCHAR *buffer,ULONG length)
{
	PFILENAME pfn;
	BOOLEAN FullPathRetrieved = FALSE;
	
	/* initialize data */
	buffer[0] = 0;
	*parent_mft_id = FILE_root;

	/* find an appropriate pfn structure */
	pfn = FindDirectoryByMftId(dx,mft_id);
	
	if(pfn == NULL){
		DbgPrint("%I64u directory not found!\n",mft_id);
		return FullPathRetrieved;
	}
	
	/* update data */
	*parent_mft_id = pfn->ParentDirectoryMftId;
	wcsncpy(buffer,pfn->name.Buffer,length);
	buffer[length-1] = 0;
	FullPathRetrieved = pfn->PathBuilt;
	return FullPathRetrieved;
}

void AddResidentDirectoryToFileList(UDEFRAG_DEVICE_EXTENSION *dx,PMY_FILE_INFORMATION pmfi)
{
	PFILENAME pfn;
	
	pfn = (PFILENAME)InsertItem((PLIST *)&dx->filelist,NULL,sizeof(FILENAME),PagedPool);
	if(pfn == NULL){
		DebugPrint2("-Ultradfg- no enough memory for AddResidentDirectoryToFileList()!\n",NULL);
		return;
	}
	
	/* fill a name member of the created structure */
	if(!RtlCreateUnicodeString(&pfn->name,pmfi->Name)){
		DebugPrint2("-Ultradfg- AddResidentDirectoryToFileList():\n",NULL);
		DebugPrint2("-Ultradfg- no enough memory for pfn->name initialization!\n",NULL);
		RemoveItem((PLIST *)&dx->filelist,(LIST *)pfn);
		return;
	}
	pfn->blockmap = NULL; /* !!! */
	pfn->BaseMftId = pmfi->BaseMftId;
	pfn->ParentDirectoryMftId = pmfi->ParentDirectoryMftId;
	pfn->PathBuilt = TRUE;
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

PFILENAME FindDirectoryByMftId(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG mft_id)
{
	PFILENAME pfn;
	ULONG lim, i, k;
	signed long m;
	BOOLEAN ascending_order;

	if(mf_allocated == FALSE){ /* use slow search */
		for(pfn = dx->filelist; pfn != NULL; pfn = pfn->next_ptr){
			if(pfn->BaseMftId == mft_id){
				if(wcsstr(pfn->name.Buffer,L":$") == NULL)
					return pfn;
			}
			if(pfn->next_ptr == dx->filelist) break;
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
				DbgPrint("FUCK 1\n");
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
		DbgPrint("FUCK 2\n");
		return NULL;
	}
}
