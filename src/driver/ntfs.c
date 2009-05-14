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

NTSTATUS GetMftRecord(UDEFRAG_DEVICE_EXTENSION *dx,PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,
					  ULONG nfrob_size,ULONGLONG mft_id);
void AnalyseMftRecord(UDEFRAG_DEVICE_EXTENSION *dx,PNTFS_FILE_RECORD_OUTPUT_BUFFER pnfrob,
					  ULONG nfrob_size);

/* extracts low 48 bits of File Reference Number */
#define GetMftIdFromFRN(n) ((n) & 0xffffffffffffLL)

BOOLEAN ScanMFT(UDEFRAG_DEVICE_EXTENSION *dx)
{
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
	
	/* read all MFT records sequentially */
	/* TODO: determine maximum mft_id more finely */
	mft_id = dx->max_mft_entries - 1;
	while(1){
		status = GetMftRecord(dx,pnfrob,nfrob_size,mft_id);
		if(!NT_SUCCESS(status)){
			if(mft_id == 0){
				DebugPrint("-Ultradfg- FSCTL_GET_NTFS_FILE_RECORD failed: %x!\n",NULL,status);
				Nt_ExFreePool(pnfrob);
				DebugPrint("-Ultradfg- MFT scan finished!\n",NULL);
				return FALSE;
			}
			/* it returns 0xc000000d (invalid parameter) for non existing records */
			mft_id --; /* try to retrieve a previous record */
			continue;
		}

		/* analyse retrieved mft record */
		ret_mft_id = GetMftIdFromFRN(pnfrob->FileReferenceNumber);
		DebugPrint("-Ultradfg- NTFS record found, id = %I64u\n",NULL,ret_mft_id);
		AnalyseMftRecord(dx,pnfrob,nfrob_size);
		
		/* go to the next record */
		if(ret_mft_id == 0) break;
		mft_id = ret_mft_id - 1;
	}
	
	/* free allocated memory */
	Nt_ExFreePool(pnfrob);
	
	DebugPrint("-Ultradfg- MFT scan finished!\n",NULL);
	return TRUE;
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
					  ULONG nfrob_size)
{
	ULONGLONG mft_id;
	PFILE_RECORD_HEADER pfrh;
	ULONG type;
	char a,b,c,d;
	
	/* get Mft entry ID */
	mft_id = GetMftIdFromFRN(pnfrob->FileReferenceNumber);

	/* analyse record's header */
	pfrh = (PFILE_RECORD_HEADER)pnfrob->FileRecordBuffer;
	type = pfrh->Ntfs.Type;
	a = (char)(type & 0xff);
	b = (char)((type & 0xff00) >> 8);
	c = (char)((type & 0xff0000) >> 16);
	d = (char)(type >> 24);
	DebugPrint("-Ultradfg- Type = %c%c%c%c\n",NULL,a,b,c,d);
	
	if(type != TAG('F','I','L','E')) return;
	
	/* analyse file record */
	DebugPrint("-Ultradfg- SequenceNumber = %hu, LinkCount = %hu, Flags = 0x%hx\n",
		NULL,pfrh->SequenceNumber,pfrh->LinkCount,pfrh->Flags);
	DebugPrint("-Ultradfg- BaseFileRecord = %I64u\n",NULL,pfrh->BaseFileRecord);
}
