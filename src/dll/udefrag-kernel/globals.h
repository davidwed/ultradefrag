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
* User mode driver - global objects and ancillary routines definitions.
*/

#ifndef _UDEFRAG_KERNEL_GLOBALS_H_
#define _UDEFRAG_KERNEL_GLOBALS_H_

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
///#include <winioctl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> /* for toupper() on mingw */
#include "../../include/ntndk.h"
#include "../../include/udefrag.h"
#include "../../include/ultradfg.h"
#include "../zenwinx/zenwinx.h"
#include "../../include/udefrag-kernel.h"
#include "../../include/ultradfgver.h"

#define DebugPrint winx_dbg_print
#define DebugPrint1 if(dbgprint_level < 1) {} else DebugPrint
#define DebugPrint2 if(dbgprint_level < 2) {} else DebugPrint

/* UltraDefrag internal structures */

/*
* NOTE! NEXT_PTR MUST BE THE FIRST MEMBER OF THESE STRUCTURES!
* PREV_PTR MUST BE THE SECOND MEMBER!
*/
/* generic LIST structure definition */
typedef struct _LIST {
	struct _LIST *next_ptr;
	struct _LIST *prev_ptr;
} LIST, *PLIST;

/* structure to store information about blocks of file */
typedef struct _tagBLOCKMAP {
	struct _tagBLOCKMAP *next_ptr;
	struct _tagBLOCKMAP *prev_ptr;
	ULONGLONG vcn; /* useful for compressed files */
	ULONGLONG lcn;
	ULONGLONG length;
} BLOCKMAP, *PBLOCKMAP;

/* structure to store information about free space blocks */
typedef struct _tagFREEBLOCKMAP {
	struct _tagFREEBLOCKMAP *next_ptr;
	struct _tagFREEBLOCKMAP *prev_ptr;
	ULONGLONG lcn;
	ULONGLONG length;
} FREEBLOCKMAP, *PFREEBLOCKMAP;

/* structure to store information about file itself */
typedef struct _tagFILENAME {
	struct _tagFILENAME *next_ptr;
	struct _tagFILENAME *prev_ptr;
	UNICODE_STRING name;
	BOOLEAN is_fragm;
	ULONG n_fragments;
	ULONGLONG clusters_total;
	PBLOCKMAP blockmap;
	BOOLEAN is_dir;
	BOOLEAN is_compressed;
	BOOLEAN is_overlimit;
	BOOLEAN is_filtered;
	BOOLEAN is_reparse_point;
	BOOLEAN is_dirty; /* for ntfs scan it means: not all members of the structure are set */
	ULONGLONG BaseMftId; /* ancillary field - valid on NTFS volumes only */
	ULONGLONG ParentDirectoryMftId;
	BOOLEAN PathBuilt; /* ntfs specific */
} FILENAME, *PFILENAME;

/* structure to store fragmented item */
typedef struct _FRAGMENTED {
	struct _FRAGMENTED *next_ptr;
	struct _FRAGMENTED *prev_ptr;
	FILENAME *pfn;
} FRAGMENTED, *PFRAGMENTED;

/* structure to store offset of filter string in multiline filter */
typedef struct _OFFSET
{
	struct _OFFSET *next_ptr;
	struct _OFFSET *prev_ptr;
	int offset;
} OFFSET, *POFFSET;

/* structure that represents multiline filter */
typedef struct _FILTER
{
	short *buffer;
	POFFSET offsets;
} FILTER, *PFILTER;

/***************** Global variables ********************/
extern int nt4_system;
extern int w2k_system;

extern FILTER in_filter;
extern FILTER ex_filter;
extern ULONGLONG sizelimit;
extern ULONGLONG fraglimit;

extern ULONG disable_reports;
extern ULONG dbgprint_level;

extern BOOLEAN context_menu_handler;

extern HANDLE hSynchEvent;
extern HANDLE hStopEvent;
extern HANDLE hMapEvent;

extern STATISTIC Stat;

extern PFREEBLOCKMAP free_space_map;
extern PFILENAME filelist;
extern PFRAGMENTED fragmfileslist;

extern WINX_FILE *fVolume;

extern unsigned char volume_letter;

extern ULONGLONG bytes_per_cluster;
extern ULONG bytes_per_sector;
extern ULONG sectors_per_cluster;
extern ULONGLONG total_space;
extern ULONGLONG free_space; /* in bytes */
extern ULONGLONG clusters_total;
extern ULONGLONG clusters_per_256k;

extern unsigned char partition_type;

extern ULONGLONG mft_size;
extern ULONG ntfs_record_size;
extern ULONGLONG max_mft_entries;

extern ULONGLONG (*new_cluster_map)[NUM_OF_SPACE_STATES];
extern ULONG map_size;

extern BOOLEAN optimize_flag;

/*************** Functions prototypes ******************/
void InitDriverResources(void);
void FreeDriverResources(void);

int AllocateMap(int size);
void FreeMap(void);
void GetMap(char *dest,int cluster_map_size);
void MarkAllSpaceAsSystem0(void);
void MarkAllSpaceAsSystem1(void);
unsigned char GetSpaceState(PFILENAME pfn);
void MarkSpace(PFILENAME pfn,int old_space_state);
void ProcessBlock(ULONGLONG start,ULONGLONG len,int space_state,int old_space_state);

void InitializeOptions(void);
void InitializeFilter(void);
void DestroyFilter(void);
BOOLEAN IsStringInFilter(short *str,PFILTER pf);
BOOLEAN CheckForContextMenuHandler(void);

LIST * NTAPI InsertItem(PLIST *phead,PLIST prev,ULONG size);
void NTAPI RemoveItem(PLIST *phead,PLIST item);
void NTAPI DestroyList(PLIST *phead);

int CheckForSynchObjects(void);

BOOLEAN SaveReportToDisk(char *volume_name);
void RemoveReportFromDisk(char *volume_name);

int  OpenVolume(char *volume_name);
void CloseVolume(void);
int GetDriveGeometry(char *volume_name);

int Analyze(char *volume_name);
int Defragment(char *volume_name);
int Optimize(char *volume_name);

int AnalyzeFreeSpace(char *volume_name);
void DestroyLists(void);
void DbgPrintFreeSpaceList(void);

BOOLEAN ScanMFT(void);
void CheckForNtfsPartition(void);
NTSTATUS ReadSectors(ULONGLONG lsn,PVOID buffer,ULONG length);

ULONGLONG _rdtsc(void);
ULONGLONG _rdtsc_1(void);

BOOLEAN CheckForStopEvent(void);

wchar_t * __cdecl wcsistr(const wchar_t * wcs1,const wchar_t * wcs2);

NTSTATUS FillFreeSpaceMap(void);
void ProcessFreeBlock(ULONGLONG start,ULONGLONG len,UCHAR old_space_state);
void InsertFreeSpaceBlock(ULONGLONG start,ULONGLONG length,UCHAR old_space_state);
FREEBLOCKMAP *InsertLastFreeBlock(ULONGLONG start,ULONGLONG length);
void CleanupFreeSpaceList(ULONGLONG start,ULONGLONG len);
void TruncateFreeSpaceBlock(ULONGLONG start,ULONGLONG length);

void GenerateFragmentedFilesList(void);
BOOLEAN FindFiles(WCHAR *ParentDirectoryPath);
BOOLEAN InsertFragmentedFile(PFILENAME pfn);
NTSTATUS OpenTheFile(PFILENAME pfn,HANDLE *phFile);
BOOLEAN DumpFile(PFILENAME pfn);
void UpdateFragmentedFilesList(void);

NTSTATUS MovePartOfFile(HANDLE hFile,ULONGLONG startVcn, ULONGLONG targetLcn, ULONGLONG n_clusters);
NTSTATUS MoveBlocksOfFile(PFILENAME pfn,HANDLE hFile,ULONGLONG targetLcn);

/*--------------------------------------------------------------------
 *       F S C T L  S P E C I F I C   T Y P E D E F S  
 *--------------------------------------------------------------------
 */

/* This is the definition for a VCN/LCN (virtual cluster/logical cluster)
 * mapping pair that is returned in the buffer passed to 
 * FSCTL_GET_RETRIEVAL_POINTERS
 */
typedef struct {
	ULONGLONG			Vcn;
	ULONGLONG			Lcn;
} MAPPING_PAIR, *PMAPPING_PAIR;

/* This is the definition for the buffer that FSCTL_GET_RETRIEVAL_POINTERS
 * returns. It consists of a header followed by mapping pairs
 */
typedef struct {
	ULONG				NumberOfPairs;
	ULONGLONG			StartVcn;
	MAPPING_PAIR		Pair[1];
} GET_RETRIEVAL_DESCRIPTOR, *PGET_RETRIEVAL_DESCRIPTOR;

/* This is the definition of the buffer that FSCTL_GET_VOLUME_BITMAP
 * returns. It consists of a header followed by the actual bitmap data
 */
typedef struct {
	ULONGLONG			StartLcn;
	ULONGLONG			ClustersToEndOfVol;
	UCHAR				Map[1];
} BITMAP_DESCRIPTOR, *PBITMAP_DESCRIPTOR; 

/* Size of the buffer we read file mapping information into.
 * The buffer is big enough to hold the 16 bytes that 
 * come back at the head of the buffer (the number of entries 
 * and the starting virtual cluster), as well as 512 pairs
 * of [virtual cluster, logical cluster] pairs.
 */
#define	FILEMAPSIZE		(512+2)

/* Size of the bitmap buffer we pass in. Its large enough to
 * hold information for the 16-byte header that's returned
 * plus the indicated number of bytes, each of which has 8 bits 
 * (imagine that!)
 */
#define BITMAPBYTES		4096
#define BITMAPSIZE		(BITMAPBYTES+2*sizeof(ULONGLONG))

/* "Invalid longlong number" - indicates that cluster is virtual */
#define LLINVALID		((ULONGLONG) -1)

/* UltraDefrag specific definitions */
#define _256K (256 * 1024)
#define FIND_DATA_SIZE	(16*1024)

#define FAT12_PARTITION        0x0 /* really not defined */
#define FAT16_PARTITION        0x6
#define NTFS_PARTITION         0x7
#define FAT32_PARTITION        0xB
#define UDF_PARTITION          0xC /* really not defined */
#define UNKNOWN_PARTITION      0xF /* really not defined */

#define CHECK_FOR_FRAGLIMIT(pfn) { \
	if(fraglimit && pfn->n_fragments < fraglimit) pfn->is_filtered = TRUE; \
}

#define IS_DIR(pFileInfo) \
(((pFileInfo)->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE)

#define IS_COMPRESSED(pFileInfo) \
(((pFileInfo)->FileAttributes & FILE_ATTRIBUTE_COMPRESSED) ? TRUE : FALSE)

#define IS_REPARSE_POINT(pFileInfo) \
(((pFileInfo)->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ? TRUE : FALSE)

#define IS_SPARSE_FILE(pFileInfo) \
(((pFileInfo)->FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) ? TRUE : FALSE)

#define IS_TEMPORARY_FILE(pFileInfo) \
(((pFileInfo)->FileAttributes & FILE_ATTRIBUTE_TEMPORARY) ? TRUE : FALSE)

#define IS_ENCRYPTED_FILE(pFileInfo) \
(((pFileInfo)->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ? TRUE : FALSE)

#define IS_NOT_CONTENT_INDEXED(pFileInfo) \
(((pFileInfo)->FileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) ? TRUE : FALSE)

#define DeleteBlockmap(pfn) DestroyList((PLIST *)&(pfn)->blockmap)

#endif /* _UDEFRAG_KERNEL_GLOBALS_H_ */
