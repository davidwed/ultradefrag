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
+ Control an amount of debugging information 
* through this parameter.
* NOTE: Designed especially for development stage.
*/
//#define DETAILED_LOGGING

NTSTATUS GetMftRecord(UDEFRAG_DEVICE_EXTENSION *dx,PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,
					  ULONG nfrob_size,ULONGLONG mft_id);
void AnalyseMftRecord(UDEFRAG_DEVICE_EXTENSION *dx,PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,
					  ULONG nfrob_size,PMY_FILE_INFORMATION pmfi);
void AnalyseAttribute(UDEFRAG_DEVICE_EXTENSION *dx,PATTRIBUTE pattr,PMY_FILE_INFORMATION pmfi);
void AnalyseResidentAttribute(UDEFRAG_DEVICE_EXTENSION *dx,PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi);
void AnalyseNonResidentAttribute(UDEFRAG_DEVICE_EXTENSION *dx,PNONRESIDENT_ATTRIBUTE pnr_attr,PMY_FILE_INFORMATION pmfi);
void AnalyseResidentAttributeList(UDEFRAG_DEVICE_EXTENSION *dx,PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi);
void AnalyseAttributeFromAttributeList(UDEFRAG_DEVICE_EXTENSION *dx,PATTRIBUTE_LIST attr_list_entry,PMY_FILE_INFORMATION pmfi);
void GetFileFlags(UDEFRAG_DEVICE_EXTENSION *dx,PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi);
void GetFileName(UDEFRAG_DEVICE_EXTENSION *dx,PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi);
void UpdateFileName(UDEFRAG_DEVICE_EXTENSION *dx,PMY_FILE_INFORMATION pmfi,WCHAR *name,UCHAR name_type);
void BuildPath(UDEFRAG_DEVICE_EXTENSION *dx,PMY_FILE_INFORMATION pmfi);
void GetVolumeInformation(UDEFRAG_DEVICE_EXTENSION *dx,PRESIDENT_ATTRIBUTE pr_attr);
void CheckReparsePointResident(UDEFRAG_DEVICE_EXTENSION *dx,PRESIDENT_ATTRIBUTE pr_attr,PMY_FILE_INFORMATION pmfi);

void UpdateMaxMftEntriesNumber(UDEFRAG_DEVICE_EXTENSION *dx,
		PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,ULONG nfrob_size);

void GetFileNameAndParentMftIdFromMftRecord(UDEFRAG_DEVICE_EXTENSION *dx,
		ULONGLONG mft_id,ULONGLONG *parent_mft_id,WCHAR *buffer,ULONG length);

void ProcessRunList(UDEFRAG_DEVICE_EXTENSION *dx,WCHAR *full_path,PNONRESIDENT_ATTRIBUTE pnr_attr,PMY_FILE_INFORMATION pmfi);

/* extracts low 48 bits of File Reference Number */
#define GetMftIdFromFRN(n) ((n) & 0xffffffffffffLL)

BOOLEAN ScanMFT(UDEFRAG_DEVICE_EXTENSION *dx)
{
	PMY_FILE_INFORMATION pmfi;
	PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob = NULL;
	ULONG nfrob_size;
	ULONGLONG mft_id, ret_mft_id;
	NTSTATUS status;
	
	DebugPrint("-Ultradfg- MFT scan started!\n",NULL);

	/* allocate memory for NTFS record */
	nfrob_size = sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + dx->ntfs_record_size - 1;
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
	UpdateMaxMftEntriesNumber(dx,pnfrob,nfrob_size);
	mft_id = dx->max_mft_entries - 1;
	DebugPrint("\n",NULL);
	while(1){
		if(KeReadStateEvent(&stop_event) == 0x1) break;
		status = GetMftRecord(dx,pnfrob,nfrob_size,mft_id);
		if(!NT_SUCCESS(status)){
			if(mft_id == 0){
				DebugPrint("-Ultradfg- FSCTL_GET_NTFS_FILE_RECORD failed: %x!\n",NULL,status);
				Nt_ExFreePool(pnfrob);
				Nt_ExFreePool(pmfi);
				DebugPrint("-Ultradfg- MFT scan finished!\n",NULL);
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
	
	/* free allocated memory */
	Nt_ExFreePool(pnfrob);
	Nt_ExFreePool(pmfi);
	
	DebugPrint("-Ultradfg- MFT scan finished!\n",NULL);
	return TRUE;
}

void UpdateMaxMftEntriesNumber(UDEFRAG_DEVICE_EXTENSION *dx,
		PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,ULONG nfrob_size)
{
	NTSTATUS status;
	PFILE_RECORD_HEADER pfrh;
	PATTRIBUTE pattr;
	ULONG attr_length;
	USHORT attr_offset;
	PNONRESIDENT_ATTRIBUTE pnr_attr;

	/* Get record for FILE_MFT. */
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

		if(pattr->Nonresident && pattr->AttributeType == AttributeData){
			pnr_attr = (PNONRESIDENT_ATTRIBUTE)pattr;
			dx->max_mft_entries = pnr_attr->DataSize / dx->ntfs_record_size;
			DebugPrint("-Ultradfg- MFT contains no more than %I64u records (more accurately)\n",NULL,
				dx->max_mft_entries);
			break;
		}
		
		/* go to the next attribute */
		attr_length = pattr->Length;
		attr_offset += attr_length;
		pattr = (PATTRIBUTE)((char *)pattr + attr_length);
	}
}

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
	ULONGLONG mft_id;
	PFILE_RECORD_HEADER pfrh;
	ULONG type;
	USHORT Flags;
//	char a,b,c,d;
	PATTRIBUTE pattr;
//	short *name;
	ULONG attr_length;
	USHORT attr_offset;
	BOOLEAN path_built = FALSE;
	
	/* allocate memory */
//	name = (short *)AllocatePool(NonPagedPool,(MAX_PATH + 1) * sizeof(short));
//	if(!name){}
	
	/* get Mft entry ID */
	mft_id = GetMftIdFromFRN(pnfrob->FileReferenceNumber);

	/* analyse record's header */
	pfrh = (PFILE_RECORD_HEADER)pnfrob->FileRecordBuffer;
	type = pfrh->Ntfs.Type;
	Flags = pfrh->Flags;
/*	a = (char)(type & 0xff);
	b = (char)((type & 0xff00) >> 8);
	c = (char)((type & 0xff0000) >> 16);
	d = (char)(type >> 24);
	DebugPrint("-Ultradfg- Type = %c%c%c%c\n",NULL,a,b,c,d);
*/	
	if(type != TAG('F','I','L','E')){
//		Nt_ExFreePool(name);
		return;
	}
	
	/* analyse file record */
//	DebugPrint("-Ultradfg- SequenceNumber = %hu, LinkCount = %hu, Flags = 0x%hx\n",
//		NULL,pfrh->SequenceNumber,pfrh->LinkCount,pfrh->Flags);
//	if(Flags & 0x1) DebugPrint("-Ultradfg- In Use\n",NULL);
	#ifdef DETAILED_LOGGING
	if(Flags & 0x2) DebugPrint("-Ultradfg- Directory\n",NULL); /* May be wrong? */
	#endif

	if(pfrh->BaseFileRecord){ /* skip these records, we will analyse them later */
		DebugPrint("-Ultradfg- BaseFileRecord (id) = %I64u\n",
			NULL,GetMftIdFromFRN(pfrh->BaseFileRecord));
		DebugPrint("\n",NULL);
		//Nt_ExFreePool(name);
		return;
	}
	
	/* analyse attributes of MFT entry */
	pmfi->BaseMftId = mft_id;
	pmfi->ParentDirectoryMftId = FILE_root;
	pmfi->Flags = 0x0;
	pmfi->NameType = 0x0; /* Assume FILENAME_POSIX */
	memset(pmfi->Name,0,MAX_NTFS_PATH);
//	DebugPrint("-Ultradfg- Attributes\n",NULL);

	/* skip AttributeList attributes */
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

		if(pattr->AttributeType > AttributeFileName && path_built == FALSE){
			BuildPath(dx,pmfi);
			path_built = TRUE;
		}

		if(pattr->AttributeType != AttributeAttributeList)
			AnalyseAttribute(dx,pattr,pmfi);
		
		/* go to the next attribute */
		attr_length = pattr->Length;
		attr_offset += attr_length;
		pattr = (PATTRIBUTE)((char *)pattr + attr_length);
	}
	
	if(!path_built){
		BuildPath(dx,pmfi);
		path_built = TRUE;
	}

	/* analyse AttributeList attributes */
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

		if(pattr->AttributeType == AttributeAttributeList)
			AnalyseAttribute(dx,pattr,pmfi);
		
		/* go to the next attribute */
		attr_length = pattr->Length;
		attr_offset += attr_length;
		pattr = (PATTRIBUTE)((char *)pattr + attr_length);
	}

//	Nt_ExFreePool(name);
	#ifdef DETAILED_LOGGING
	DebugPrint("\n",NULL);
	#endif
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

	case AttributeAttributeList: /* always nonresident? */
		DebugPrint("Resident AttributeList found!\n",NULL);
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

/* build the full path after all names has been collected */
void BuildPath(UDEFRAG_DEVICE_EXTENSION *dx,PMY_FILE_INFORMATION pmfi)
{
	WCHAR *buffer1;
	WCHAR *buffer2;
	ULONG offset;
	ULONGLONG mft_id,parent_mft_id;
	ULONG name_length;
	WCHAR header[] = L"\\??\\A:";
	
	#ifdef DETAILED_LOGGING
	DebugPrint("-Ultradfg- parent id = %I64u, FILENAME =\n",
		pmfi->Name,pmfi->ParentDirectoryMftId);
	#endif
		
	if(pmfi->Name[0] == 0) return;
		
	/* allocate memory */
	buffer1 = (WCHAR *)AllocatePool(NonPagedPool,(MAX_NTFS_PATH) * sizeof(short));
	if(!buffer1){
		DebugPrint("-Ultradfg- BuildPath(): cannot allocate memory for buffer1\n",NULL);
		return;
	}
	buffer2 = (WCHAR *)AllocatePool(NonPagedPool,(MAX_NTFS_PATH) * sizeof(short));
	if(!buffer2){
		DebugPrint("-Ultradfg- BuildPath(): cannot allocate memory for buffer2\n",NULL);
		Nt_ExFreePool(buffer1);
		return;
	}

	/* terminate buffer1 with zero */
	offset = MAX_NTFS_PATH - 1;
	buffer1[offset] = 0; /* terminating zero */
	offset --;
	
	/* copy filename to the right side of buffer1 */
	name_length = wcslen(pmfi->Name);
	if(offset < (name_length - 1)){
		DebugPrint("-Ultradfg- BuildPath(): filename is too long (%u characters)\n",
			pmfi->Name,name_length);
		Nt_ExFreePool(buffer1);
		Nt_ExFreePool(buffer2);
		return;
	}

	offset -= (name_length - 1);
	wcsncpy(buffer1 + offset,pmfi->Name,name_length);

	if(offset == 0) goto path_is_too_long;
	offset --;
	
	/* add backslash */
	buffer1[offset] = '\\';
	offset --;
	
	if(offset == 0) goto path_is_too_long;

	parent_mft_id = pmfi->ParentDirectoryMftId;
	while(parent_mft_id != FILE_root){
		if(KeReadStateEvent(&stop_event) == 0x1) break;
		mft_id = parent_mft_id;
		GetFileNameAndParentMftIdFromMftRecord(dx,mft_id,
			&parent_mft_id,buffer2,MAX_NTFS_PATH);
		if(buffer2[0] == 0){
			DebugPrint("-Ultradfg- BuildPath(): cannot retrieve parent directory name!\n",NULL);
			goto build_path_done;
		}
		//DbgPrint("%ws\n",buffer2);
		/* append buffer2 contents to the right side of buffer1 */
		name_length = wcslen(buffer2);
		if(offset < (name_length - 1)) goto path_is_too_long;
		offset -= (name_length - 1);
		wcsncpy(buffer1 + offset,buffer2,name_length);
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
	
	/* replace pmfi->Name contents with full path */
	wcsncpy(pmfi->Name,buffer1 + offset,MAX_NTFS_PATH);
	pmfi->Name[MAX_NTFS_PATH - 1] = 0;
	
	#ifdef DETAILED_LOGGING
	DebugPrint("-Ultradfg- FULL PATH =\n",pmfi->Name);
	#endif
	
build_path_done:
	Nt_ExFreePool(buffer1);
	Nt_ExFreePool(buffer2);
	return;
	
path_is_too_long:
	DebugPrint("-Ultradfg- BuildPath(): path is too long:\n",buffer1);
	Nt_ExFreePool(buffer1);
	Nt_ExFreePool(buffer2);
}

void GetFileNameAndParentMftIdFromMftRecord(UDEFRAG_DEVICE_EXTENSION *dx,
		ULONGLONG mft_id,ULONGLONG *parent_mft_id,WCHAR *buffer,ULONG length)
{
	PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob = NULL;
	ULONG nfrob_size;
	NTSTATUS status;
	PFILE_RECORD_HEADER pfrh;
	PATTRIBUTE pattr;
	ULONG attr_length;
	USHORT attr_offset;
	PRESIDENT_ATTRIBUTE pr_attr;
	PMY_FILE_INFORMATION pmfi;

	/* initialize data */
	buffer[0] = 0;
	*parent_mft_id = FILE_root;
	
	/* allocate memory for NTFS record */
	nfrob_size = sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + dx->ntfs_record_size - 1;
	pnfrob = (PNTFS_FILE_RECORD_OUTPUT_BUFFER)AllocatePool(NonPagedPool,nfrob_size);
	if(!pnfrob){
		DebugPrint("-Ultradfg- GetFileNameAndParentMftIdFromMftRecord()\n",NULL);
		DebugPrint("-Ultradfg- No enough memory for NTFS_FILE_RECORD_OUTPUT_BUFFER!\n",NULL);
		return;
	}

	/* get record */
	status = GetMftRecord(dx,pnfrob,nfrob_size,mft_id);
	if(!NT_SUCCESS(status)){
		DebugPrint("-Ultradfg- GetFileNameAndParentMftIdFromMftRecord()\n",NULL);
		DebugPrint("-Ultradfg- FSCTL_GET_NTFS_FILE_RECORD failed: %x!\n",NULL,status);
		Nt_ExFreePool(pnfrob);
		return;
	}
	if(GetMftIdFromFRN(pnfrob->FileReferenceNumber) != mft_id){
		DebugPrint("-Ultradfg- GetFileNameAndParentMftIdFromMftRecord()\n",NULL);
		DebugPrint("-Ultradfg- Unable to get %I64u record.\n",NULL,mft_id);
		Nt_ExFreePool(pnfrob);
		return;
	}

	/* allocate memory for MY_FILE_INFORMATION structure */
	pmfi = (PMY_FILE_INFORMATION)AllocatePool(NonPagedPool,sizeof(MY_FILE_INFORMATION));
	if(!pmfi){
		DebugPrint("-Ultradfg- GetFileNameAndParentMftIdFromMftRecord()\n",NULL);
		DebugPrint("-Ultradfg- Cannot allocate memory for MY_FILE_INFORMATION structure.\n",NULL);
		Nt_ExFreePool(pnfrob);
		return;
	}
	
	/* analyse filename attributes */
	pfrh = (PFILE_RECORD_HEADER)pnfrob->FileRecordBuffer;
	attr_offset = pfrh->AttributeOffset;
	pattr = (PATTRIBUTE)((char *)pfrh + attr_offset);

	pmfi->BaseMftId = mft_id;
	pmfi->ParentDirectoryMftId = FILE_root;
	pmfi->Flags = 0x0;
	pmfi->NameType = 0x0; /* Assume FILENAME_POSIX */
	memset(pmfi->Name,0,MAX_NTFS_PATH);

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

		if(pattr->AttributeType == AttributeFileName){
			pr_attr = (PRESIDENT_ATTRIBUTE)pattr;
			GetFileName(dx,pr_attr,pmfi);
		}
		
		/* go to the next attribute */
		attr_length = pattr->Length;
		attr_offset += attr_length;
		pattr = (PATTRIBUTE)((char *)pattr + attr_length);
	}
	
	/* update data */
	*parent_mft_id = pmfi->ParentDirectoryMftId;
	wcsncpy(buffer,pmfi->Name,length);
	buffer[length-1] = 0;
	
	/* free allocated memory */
	Nt_ExFreePool(pnfrob);
	Nt_ExFreePool(pmfi);
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
	PATTRIBUTE pattr;
	ULONG attr_length;
	USHORT attr_offset;
	
	/* skip entries describing attributes stored in base mft record */
	if(GetMftIdFromFRN(attr_list_entry->FileReferenceNumber) == pmfi->BaseMftId){
		DebugPrint("Resident attribute found, type = 0x%x\n",NULL,attr_list_entry->AttributeType);
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

	if(pfrh->BaseFileRecord == 0){
		DebugPrint("-Ultradfg- AnalyseAttributeFromAttributeList() failed - %I64u is not a child record.\n",
			NULL,child_record_mft_id);
		Nt_ExFreePool(pnfrob);
		return;
	}

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

		if(pattr->Nonresident)
			AnalyseNonResidentAttribute(dx,
				(PNONRESIDENT_ATTRIBUTE)pattr,pmfi);
		
		/* go to the next attribute */
		attr_length = pattr->Length;
		attr_offset += attr_length;
		pattr = (PATTRIBUTE)((char *)pattr + attr_length);
	}

	/* free allocated memory */
	Nt_ExFreePool(pnfrob);
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
		default_attr_name = L"$Reparse";
		break;
	case AttributeLoggedUtulityStream:
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
	
	if(pnr_attr->Attribute.NameLength){
		/* append a name of attribute to filename */
		/* NameLength is always less than MAX_PATH :) */
		wcsncpy(attr_name,(short *)((char *)pnr_attr + pnr_attr->Attribute.NameOffset),
			pnr_attr->Attribute.NameLength);
		attr_name[pnr_attr->Attribute.NameLength] = 0;
		_snwprintf(full_path,MAX_NTFS_PATH,L"%s:%s",pmfi->Name,attr_name);
	} else {
		/* append default name of attribute to filename */
		if(wcscmp(default_attr_name,L"$DATA")){
			_snwprintf(full_path,MAX_NTFS_PATH,L"%s:%s",pmfi->Name,default_attr_name);
		} else {
			wcsncpy(full_path,pmfi->Name,MAX_NTFS_PATH);
		}
	}
	full_path[MAX_NTFS_PATH - 1] = 0;

	ProcessRunList(dx,full_path,pnr_attr,pmfi);

	/* free allocated memory */
	Nt_ExFreePool(attr_name);
	Nt_ExFreePool(full_path);
}

/* Saves information about VCN/LCN pairs of the attribute specified by full_path parameter. */
void ProcessRunList(UDEFRAG_DEVICE_EXTENSION *dx,WCHAR *full_path,PNONRESIDENT_ATTRIBUTE pnr_attr,PMY_FILE_INFORMATION pmfi)
{
	if(pnr_attr->Attribute.Flags & 0x1)
		DbgPrint("[CMP] %ws VCN %I64u - %I64u\n",full_path,pnr_attr->LowVcn,pnr_attr->HighVcn);
	else
		DbgPrint("[ORD] %ws VCN %I64u - %I64u\n",full_path,pnr_attr->LowVcn,pnr_attr->HighVcn);
}
