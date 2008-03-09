/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* Functions for file dump manipulations.
*/

#include "driver.h"

BLOCKMAP *InsertBlock(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,
					  ULONGLONG startVcn,ULONGLONG startLcn,ULONGLONG length);

/*
* Dump File()
* Dumps the clusters belonging to the specified file until the
* end of the file.
*
* NOTES: 
* 1. On NTFS volumes files smaller then 1 kb are placed in MFT.
*    And we exclude their from defragmenting process.
* 2. On NTFS we skip 0-filled virtual clusters of compressed files.
*/

BOOLEAN DumpFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK ioStatus;
	PGET_RETRIEVAL_DESCRIPTOR fileMappings;
	NTSTATUS Status;
	HANDLE hFile;
	ULONGLONG startLcn,length;
	int i,cnt = 0;

	/* Data initialization */
	pfn->clusters_total = pfn->n_fragments = 0;
	pfn->is_fragm = FALSE;
	pfn->blockmap = NULL;
	dx->lastblock = NULL;
	/* Open the file */
	InitializeObjectAttributes(&ObjectAttributes,&pfn->name,0,NULL,NULL);
	Status = ZwCreateFile(&hFile,FILE_GENERIC_READ,&ObjectAttributes,&ioStatus,
			  NULL,0,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_OPEN,
			  pfn->is_dir ? FILE_OPEN_FOR_BACKUP_INTENT : FILE_NO_INTERMEDIATE_BUFFERING,
			  NULL,0);
	if(Status != STATUS_SUCCESS){
		DebugPrint1("-Ultradfg- System file found: %ls %x\n",pfn->name.Buffer,(UINT)Status);
		hFile = NULL;
		goto dump_success; /* System file! */
	}

	/* Start dumping the mapping information. Go until we hit the end of the file. */
	*dx->pstartVcn = 0;
	fileMappings = (PGET_RETRIEVAL_DESCRIPTOR)(dx->FileMap);
	do {
		Status = ZwFsControlFile(hFile, NULL, NULL, 0, &ioStatus, \
						FSCTL_GET_RETRIEVAL_POINTERS, \
						dx->pstartVcn, sizeof(ULONGLONG), \
						fileMappings, FILEMAPSIZE * sizeof(LARGE_INTEGER));
		if(Status == STATUS_PENDING){
			NtWaitForSingleObject(hFile,FALSE,NULL);
			Status = ioStatus.Status;
		}
		if(Status != STATUS_SUCCESS && Status != STATUS_BUFFER_OVERFLOW){
dump_fail:
			/*
			DebugPrint1("-Ultradfg- Dump failed for %ws %x\n",pfn->name.Buffer,(UINT)Status);
			*/
			DeleteBlockmap(pfn);
			pfn->clusters_total = pfn->n_fragments = 0;
			pfn->is_fragm = FALSE;
			ZwClose(hFile);
			return FALSE;
		}
		/* Loop through the buffer of number/cluster pairs. */
		*dx->pstartVcn = fileMappings->StartVcn;
		for(i = 0; i < (ULONGLONG) fileMappings->NumberOfPairs; i++){
			/*
			* On NT 4.0 (and later NT versions),
			* a compressed virtual run (0-filled) is
			* identified with a cluster offset of -1.
			*/
			if(fileMappings->Pair[i].Lcn == LLINVALID)
				goto next_run;
			if(fileMappings->Pair[i].Vcn == 0)
				goto next_run; /* only for some 3.99 Gb files on FAT32 */
			startLcn = fileMappings->Pair[i].Lcn;
			length = fileMappings->Pair[i].Vcn - *dx->pstartVcn;
			dx->processed_clusters += length;
			if(!InsertBlock(dx,pfn,*dx->pstartVcn,startLcn,length))
				goto dump_fail;
			cnt ++;	/* block counter */
next_run:
			*dx->pstartVcn = fileMappings->Pair[i].Vcn;
		}
	} while(Status != STATUS_SUCCESS);

	if(!cnt) goto dump_fail; /* file is placed in MFT or some error */
	ZwClose(hFile);

	MarkSpace(dx,pfn,SYSTEM_SPACE);

	pfn->n_fragments = cnt;
	if(pfn->is_fragm){
		dx->fragmfilecounter ++; /* file is true fragmented */
	}
	dx->fragmcounter += cnt;
dump_success:
	return TRUE; /* success */
}

/* inserts the specified block in file's blockmap */
BLOCKMAP *InsertBlock(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,
					ULONGLONG startVcn,ULONGLONG startLcn,ULONGLONG length)
{
	PBLOCKMAP block;

	/* for compressed files it's neccessary: */
	if(dx->lastblock){
		if(startLcn != dx->lastblock->lcn + dx->lastblock->length)
			pfn->is_fragm = TRUE;
	}
	block = (PBLOCKMAP)InsertLastItem((PLIST *)&pfn->blockmap,
		(PLIST *)&dx->lastblock,sizeof(BLOCKMAP));
	if(block){
		block->lcn = startLcn;
		block->length = length;
		block->vcn = startVcn;
		/* change file statistic */
		pfn->clusters_total += length;
	}
	return block;
}

void DeleteBlockmap(PFILENAME pfn)
{
	DestroyList((PLIST *)&pfn->blockmap);
}
