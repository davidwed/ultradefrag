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
* Main header.
*/

#ifndef _DRIVER_H_
#define _DRIVER_H_

/* compiler specific definitions */
#if !defined(__GNUC__)
#ifndef USE_WINDDK
#define __MSVC__
#define _PCR
#endif
#pragma warning(disable:4103) /* used #pragma pack to change alignment */
#endif

#define DBG 1 /* it's very useful! */

/*
* Alter's DbgPrint logger is much better than my implementation,
* so NT4_DBG should be always undefined.
* The latest version of logger can be downloaded from
* http://alter.org.ua/en/soft/win/dbgdump/
* To collect messages during console/gui execution click on 
* DbgPrintLog.exe icon; to do this at boot time, use the following:
* DbgPrintLog.exe --full -T DTN -wd c:\ --drv:inst 1 
* 	--svc:inst A --drv:opt DoNotPassMessagesDown 1 UDefrag.log
*/
/*
#ifdef NT4_TARGET
#define NT4_DBG
#else
#undef NT4_DBG
#endif
*/
#undef NT4_DBG

#ifdef NT4_DBG
void __stdcall OpenLog();
void __stdcall CloseLog();
void __cdecl WriteLogMessage(char *format, ...);
#define DebugPrint WriteLogMessage
#define RING_BUFFER_SIZE (512 * 1024) 
#else
#define DebugPrint DbgPrint
#endif

#if 0 /* because windows ddk isn't compatible with ANSI C Standard */
#define DebugPrint0(...) DebugPrint(__VA_ARGS__)
#define DebugPrint1(...) { if(dbg_level > 0) DebugPrint(__VA_ARGS__); }
#define DebugPrint2(...) { if(dbg_level > 1) DebugPrint(__VA_ARGS__); }
#else
#define DebugPrint0 DebugPrint
#define DebugPrint1 if(dbg_level < 1) {} else DebugPrint
#define DebugPrint2 if(dbg_level < 2) {} else DebugPrint
#endif

#define DbgPrintNoMem() DebugPrint(no_mem)
#define DbgPrintNoMem0() DebugPrint0(no_mem)
#define DbgPrintNoMem1() DebugPrint1(no_mem)
#define DbgPrintNoMem2() DebugPrint2(no_mem)

#if defined(__GNUC__)
#include <ddk/ntddk.h>
#include <ddk/ntdddisk.h>
#else
#include <ntddk.h>
#include <ntdddisk.h>
#endif
#include <ctype.h>
#include "../include/ultradfg.h"

typedef unsigned int UINT;

#if defined(__GNUC__)
#define PLACE_IN_SECTION(s)	__attribute__((section (s)))
#define INIT_FUNCTION       PLACE_IN_SECTION("INIT")
#define PAGED_OUT_FUNCTION  PLACE_IN_SECTION("PAGE")
#else
#define INIT_FUNCTION
#define PAGED_OUT_FUNCTION
#endif

#include "globals.h"

/* Safe versions of some frequently used functions. */
#define ZwCloseSafe(h) if(h) { ZwClose(h); h = NULL; }
#define ExFreePoolSafe(p) if(p) { ExFreePool(p); p = NULL; }

/* exclude requests for 64-bit driver from 32-bit apps */
#ifdef _WIN64
#define Is32bitProcess(irp) IoIs32bitProcess(irp)
#define CheckIrp(irp) (!IoIs32bitProcess(irp))
#else
#define Is32bitProcess(irp) TRUE
#define CheckIrp(irp) TRUE
#endif

#define CHECK_IRP(irp) { \
if(!CheckIrp(Irp)){ \
	DebugPrint(invalid_request); \
	return CompleteIrp(Irp,STATUS_INVALID_DEVICE_REQUEST,0); \
} \
}

/* Unfortunately, usually headers don't contains some important prototypes. */
#ifdef USE_WINDDK
NTSTATUS NTAPI ZwDeviceIoControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,
		PVOID,PIO_STATUS_BLOCK,ULONG,PVOID,ULONG,PVOID,ULONG);
NTSTATUS NTAPI NtWaitForSingleObject(HANDLE,BOOLEAN,PLARGE_INTEGER);
PDEVICE_OBJECT NTAPI IoGetAttachedDevice(PDEVICE_OBJECT);
#define MAX_PATH 260
#endif
BOOLEAN  NTAPI RtlCreateUnicodeString(PUNICODE_STRING,LPCWSTR);
NTSTATUS NTAPI ZwQueryDirectoryFile(HANDLE,HANDLE,PIO_APC_ROUTINE,
		PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS,BOOLEAN,
		PUNICODE_STRING,BOOLEAN);
NTSTATUS NTAPI ZwFsControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,
		PIO_STATUS_BLOCK,ULONG,PVOID,ULONG,PVOID,ULONG);
NTSTATUS NTAPI ZwQueryVolumeInformationFile(HANDLE,PIO_STATUS_BLOCK,
		PVOID,ULONG,FS_INFORMATION_CLASS);
NTSTATUS NTAPI ZwDeleteFile(IN POBJECT_ATTRIBUTES);
char *__cdecl _itoa(int value,char *str,int radix);
#if defined(__GNUC__)
ULONGLONG __stdcall _aulldiv(ULONGLONG n, ULONGLONG d);
ULONGLONG __stdcall _alldiv(ULONGLONG n, ULONGLONG d);
ULONGLONG __stdcall _aullrem(ULONGLONG u, ULONGLONG v);
#endif

#ifndef NOMINMAX
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#endif

#ifndef TAG
#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#endif

#if DBG
#define UDEFRAG_TAG TAG('U','L','T','R')
#define AllocatePool(type,size) ExAllocatePoolWithTag((type),(size),UDEFRAG_TAG)
#else
#define AllocatePool(type,size) ExAllocatePool((type),(size))
#endif

#ifdef NT4_TARGET
#ifdef USE_WINDDK
#undef ExFreePool
DECLSPEC_IMPORT VOID NTAPI ExFreePool(IN PVOID P);
#endif /* USE_WINDDK */
#else /* NT4_TARGET */
#ifndef USE_WINDDK
#undef ExFreePool
#define ExFreePool(a) ExFreePoolWithTag(a,0)
#endif /* USE_WINDDK */
#endif /* NT4_TARGET */

/*
* NOTE! NEXT_PTR MUST BE FIRST MEMBER OF THESE STRUCTURES!
*/
/* generic LIST structure definition */
typedef struct _LIST {
	struct _LIST *next_ptr;
} LIST, *PLIST;

/* structure to store information about blocks of file */
typedef struct _tagBLOCKMAP {
	struct _tagBLOCKMAP *next_ptr;
	ULONGLONG vcn; /* useful for compressed files */
	ULONGLONG lcn;
	ULONGLONG length;
} BLOCKMAP, *PBLOCKMAP;

/* structure to store information about free space blocks */
typedef struct _tagFREEBLOCKMAP {
	struct _tagFREEBLOCKMAP *next_ptr;
	ULONGLONG lcn;
	ULONGLONG length;
} FREEBLOCKMAP, *PFREEBLOCKMAP;

/* structure to store information about file itself */
typedef struct _tagFILENAME {
	struct _tagFILENAME *next_ptr;
	UNICODE_STRING name;
	BOOLEAN is_fragm;
	ULONG n_fragments;
	ULONGLONG clusters_total;
	PBLOCKMAP blockmap;
	BOOLEAN is_dir;
	BOOLEAN is_compressed;
	BOOLEAN is_overlimit;
	BOOLEAN is_filtered;
} FILENAME, *PFILENAME;

/* structure to store fragmented item */
typedef struct _FRAGMENTED {
	struct _FRAGMENTED *next_ptr;
	FILENAME *pfn;
} FRAGMENTED, *PFRAGMENTED;

#define FLOPPY_FAT12_PARTITION 0x0 /* really not defined */
#define FAT16_PARTITION        0x6
#define NTFS_PARTITION         0x7
#define FAT32_PARTITION        0xB

#define _256K (256 * 1024)

/* structure to store information about block with unknown state */
typedef struct _PROCESS_BLOCK_STRUCT
{
	struct _PROCESS_BLOCK_STRUCT *next_ptr;
	ULONGLONG start;
	ULONGLONG len;
} PROCESS_BLOCK_STRUCT, *PPROCESS_BLOCK_STRUCT;

/* structure to store offset of filter string in multiline filter */
typedef struct _OFFSET
{
	struct _OFFSET *next_ptr;
	int offset;
} OFFSET, *POFFSET;

/* structure that represents multiline filter */
typedef struct _FILTER
{
	short *buffer;
	POFFSET offsets;
} FILTER, *PFILTER;

/*
* This is the definition for the data structure that is passed in to
* FSCTL_MOVE_FILE
*/
#ifndef _WIN64
typedef struct {
     HANDLE            FileHandle; 
     ULONG             Reserved;   
     LARGE_INTEGER     StartVcn; 
     LARGE_INTEGER     TargetLcn;
     ULONG             NumVcns; 
     ULONG             Reserved1;	
} MOVEFILE_DESCRIPTOR, *PMOVEFILE_DESCRIPTOR;
#else
typedef struct {
     HANDLE            FileHandle; 
     LARGE_INTEGER     StartVcn; 
     LARGE_INTEGER     TargetLcn;
     ULONGLONG         NumVcns; 
} MOVEFILE_DESCRIPTOR, *PMOVEFILE_DESCRIPTOR;
#endif

#define MARKER int

/*
* Define UDEFRAG_DEVICE_EXTENSION structure. Include pointer
* to FDO (for simple realization of UnloadRoutine) and 
* symbolic link name in format UNOCODE_STRING.
*/
typedef struct _UDEFRAG_DEVICE_EXTENSION
{
	PDEVICE_OBJECT fdo;
	UNICODE_STRING log_path;
	/*
	* All fields between markers will be set
	* to zero state before each analysis.
	*/
	MARKER z_start;
	PFREEBLOCKMAP free_space_map;
	PFREEBLOCKMAP lastfreeblock;
	PFILENAME filelist;
	PFRAGMENTED fragmfileslist;
	ULONG filecounter;
	ULONG dircounter;
	ULONG compressedcounter;
	ULONG fragmfilecounter;
	ULONG fragmcounter;
	ULONG mft_size;
	unsigned char partition_type;
	ULONGLONG total_space;
	ULONGLONG free_space; /* in bytes */
	ULONGLONG clusters_total;
	ULONGLONG clusters_per_256k;
	ULONGLONG clusters_per_cell;
	ULONGLONG clusters_per_last_cell; /* last block can represent more clusters */
	BOOLEAN opposite_order; /* if true then number of clusters is less than number of blocks */
	ULONGLONG cells_per_cluster;
	ULONGLONG cells_per_last_cluster;
	ULONGLONG processed_clusters;
	ULONGLONG clusters_to_move;
	ULONGLONG clusters_to_move_initial;
	PROCESS_BLOCK_STRUCT *no_checked_blocks;
	ULONG unprocessed_blocks; /* number of no-checked blocks */
	NTSTATUS status;
	ULONG invalid_movings;
	/*
	* End of the data with default zero state.
	*/
	MARKER z_end;
	ULONGLONG *FileMap; /* Buffer to read file mapping information into */
	UCHAR *BitMap;
	short *tmp_buf;
	FILTER in_filter;
	FILTER ex_filter;
	HANDLE hVol;
	LONG working_rq;
 	/*
	* End of the data with initial zero state.
	*/
	MARKER z0_end;
	PBLOCKMAP lastblock;
	UCHAR current_operation;
	UCHAR letter;
	KEVENT sync_event;
	KEVENT stop_event;
	KEVENT unload_event;
	KSPIN_LOCK spin_lock;
	ULONGLONG bytes_per_cluster;
	ULONGLONG sizelimit;
	BOOLEAN compact_flag;
	BOOLEAN xp_compatible; /* true for NT 5.1 and later versions */
	REPORT_TYPE report_type;
	/* nt 4.0 specific */
	ULONGLONG nextLcn;
	ULONGLONG *pnextLcn;
	MOVEFILE_DESCRIPTOR moveFile;
	MOVEFILE_DESCRIPTOR *pmoveFile;
	ULONGLONG startVcn;
	ULONGLONG *pstartVcn;
} UDEFRAG_DEVICE_EXTENSION, *PUDEFRAG_DEVICE_EXTENSION;

/* Function Prototypes */
NTSTATUS Analyse(UDEFRAG_DEVICE_EXTENSION *dx);
NTSTATUS RedumpSpace(UDEFRAG_DEVICE_EXTENSION *dx);
void ProcessMFT(UDEFRAG_DEVICE_EXTENSION *dx);
BOOLEAN FindFiles(UDEFRAG_DEVICE_EXTENSION *dx,UNICODE_STRING *path,BOOLEAN is_root);
BOOLEAN DumpFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);
void ProcessBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG len, int space_state,int old_space_state);
void ProcessFreeBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG len,UCHAR old_space_state);
void MarkSpace(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,int old_space_state);
NTSTATUS FillFreeSpaceMap(UDEFRAG_DEVICE_EXTENSION *dx);
BOOLEAN GetTotalClusters(UDEFRAG_DEVICE_EXTENSION *dx);
void FreeAllBuffers(UDEFRAG_DEVICE_EXTENSION *dx);
void Defragment(UDEFRAG_DEVICE_EXTENSION *dx);
NTSTATUS MovePartOfFile(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,ULONGLONG startVcn, ULONGLONG targetLcn, ULONGLONG n_clusters);
NTSTATUS MoveBlocksOfFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,HANDLE hFile,ULONGLONG target);
void DeleteBlockmap(PFILENAME pfn);
BOOLEAN DefragmentFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);
void DefragmentFreeSpace(UDEFRAG_DEVICE_EXTENSION *dx);
void InitDX(UDEFRAG_DEVICE_EXTENSION *dx);
void InitDX_0(UDEFRAG_DEVICE_EXTENSION *dx);
void InsertFreeSpaceBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG length,UCHAR old_space_state);
FREEBLOCKMAP *InsertLastFreeBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG length);

BOOLEAN SaveFragmFilesListToDisk(UDEFRAG_DEVICE_EXTENSION *dx);
void DeleteLogFile(UDEFRAG_DEVICE_EXTENSION *dx);

void ApplyFilter(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);

LIST* NTAPI InsertFirstItem(PLIST *phead,ULONG size);
LIST* NTAPI InsertMiddleItem(PLIST *pprev,PLIST *pnext,ULONG size);
LIST* NTAPI InsertLastItem(PLIST *phead,PLIST *plast,ULONG size);
LIST* NTAPI RemoveItem(PLIST *phead,PLIST *pprev,PLIST *pcurrent);
void NTAPI DestroyList(PLIST *phead);

NTSTATUS NTAPI IoFlushBuffersFile(HANDLE hFile);

NTSTATUS AllocateMap(ULONG size);
void MarkAllSpaceAsSystem0(UDEFRAG_DEVICE_EXTENSION *dx);
void MarkAllSpaceAsSystem1(UDEFRAG_DEVICE_EXTENSION *dx);
void GetMap(char *dest);

NTSTATUS OpenVolume(UDEFRAG_DEVICE_EXTENSION *dx);
NTSTATUS GetVolumeInfo(UDEFRAG_DEVICE_EXTENSION *dx);
/*__inline */void CloseVolume(UDEFRAG_DEVICE_EXTENSION *dx);

BOOLEAN CheckFreeSpace(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG len);
void CheckPendingBlocks(UDEFRAG_DEVICE_EXTENSION *dx);
void TruncateFreeSpaceBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG length);

void UpdateFilter(UDEFRAG_DEVICE_EXTENSION *dx,PFILTER pf,short *buffer,int length);
void DestroyFilter(UDEFRAG_DEVICE_EXTENSION *dx);
void UpdateFragmentedFilesList(UDEFRAG_DEVICE_EXTENSION *dx);
unsigned char GetSpaceState(PFILENAME pfn);
void FreeAllBuffersInIdleState(UDEFRAG_DEVICE_EXTENSION *dx);
void wait_for_idle_state(UDEFRAG_DEVICE_EXTENSION *dx);

#define FIND_DATA_SIZE	(16*1024)

#define IS_DIR(pFileInfo) \
(((pFileInfo)->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE)

#define IS_COMPRESSED(pFileInfo) \
(((pFileInfo)->FileAttributes & FILE_ATTRIBUTE_COMPRESSED) ? TRUE : FALSE)

#define IS_REPARSE_POINT(pFileInfo) \
(((pFileInfo)->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ? TRUE : FALSE)

typedef struct _FILE_BOTH_DIRECTORY_INFORMATION {
    ULONG               NextEntryOffset;
    ULONG               FileIndex;
    LARGE_INTEGER       CreationTime;
    LARGE_INTEGER       LastAccessTime;
    LARGE_INTEGER       LastWriteTime;
    LARGE_INTEGER       ChangeTime;
    LARGE_INTEGER       EndOfFile;
    LARGE_INTEGER       AllocationSize;
    ULONG               FileAttributes;
    ULONG               FileNameLength;
    ULONG               EaSize;
    CHAR                ShortNameLength;
    WCHAR               ShortName[12];
    WCHAR               FileName[ANYSIZE_ARRAY];
} FILE_BOTH_DIRECTORY_INFORMATION, *PFILE_BOTH_DIRECTORY_INFORMATION,
  FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

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

/* File System Control commands related to defragging */
#define	FSCTL_READ_MFT_RECORD         0x90068
#define FSCTL_GET_VOLUME_BITMAP       0x9006F
#define FSCTL_GET_RETRIEVAL_POINTERS  0x90073
#define FSCTL_MOVE_FILE               0x90074

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

typedef struct _FILE_FS_SIZE_INFORMATION {
    LARGE_INTEGER   TotalAllocationUnits;
    LARGE_INTEGER   AvailableAllocationUnits;
    ULONG           SectorsPerAllocationUnit;
    ULONG           BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

/* This is the definition for the data structure that is passed in to
 * FSCTL_GET_NTFS_VOLUME_DATA
 */
typedef struct _NTFS_DATA {
    LARGE_INTEGER VolumeSerialNumber;
    LARGE_INTEGER NumberSectors;
    LARGE_INTEGER TotalClusters;
    LARGE_INTEGER FreeClusters;
    LARGE_INTEGER TotalReserved;
    ULONG BytesPerSector;
    ULONG BytesPerCluster;
    ULONG BytesPerFileRecordSegment;
    ULONG ClustersPerFileRecordSegment;
    LARGE_INTEGER MftValidDataLength;
    LARGE_INTEGER MftStartLcn;
    LARGE_INTEGER Mft2StartLcn;
    LARGE_INTEGER MftZoneStart;
    LARGE_INTEGER MftZoneEnd;
} NTFS_DATA, *PNTFS_DATA;

#define FSCTL_GET_NTFS_VOLUME_DATA      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 25, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_NTFS_FILE_RECORD      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 26, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Function prototypes */
NTSTATUS NTAPI DeviceControlRoutine(PDEVICE_OBJECT fdo,PIRP Irp);
VOID     NTAPI UnloadRoutine(PDRIVER_OBJECT DriverObject);
NTSTATUS NTAPI Read_IRPhandler(PDEVICE_OBJECT fdo,PIRP Irp);
NTSTATUS NTAPI Write_IRPhandler(PDEVICE_OBJECT fdo,PIRP Irp);
NTSTATUS NTAPI Create_File_IRPprocessing(PDEVICE_OBJECT fdo,PIRP Irp);
NTSTATUS NTAPI Close_HandleIRPprocessing(PDEVICE_OBJECT fdo,PIRP Irp);

ULONGLONG _rdtsc(void);

#endif /* _DRIVER_H_ */
