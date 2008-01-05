/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 *  Main header.
 */

#ifndef _DRIVER_H_
#define _DRIVER_H_

// gcc ???
#ifndef USE_WINDDK
#define __MSVC__
#define _PCR
#endif

#if !defined(__GNUC__)
#pragma warning(disable:4103) /* used #pragma pack to change alignment */
#endif

#define DBG 1 /* it's very useful! */

#ifdef NT4_TARGET
#define NT4_DBG
#else
#undef NT4_DBG
#endif

///#define NT4_DBG

#ifdef NT4_DBG
void __stdcall OpenLog();
void __stdcall CloseLog();
void __cdecl WriteLogMessage(char *format, ...);
#define DebugPrint WriteLogMessage
#define RING_BUFFER_SIZE (512 * 1024) 
#else
#define DebugPrint DbgPrint
#endif

#if defined(__GNUC__)
#include <ddk/ntddk.h>
#include <ddk/ntdddisk.h>
#else
#include <ntddk.h>
#include <ntdddisk.h>
#endif
#include <ctype.h>
#include "../include/ultradfg.h"

typedef unsigned int        UINT;

#if defined(__GNUC__)
#define PLACE_IN_SECTION(s)	__attribute__((section (s)))
#define INIT_FUNCTION       PLACE_IN_SECTION("INIT")
#define PAGED_OUT_FUNCTION  PLACE_IN_SECTION("PAGE")
#else
#define INIT_FUNCTION
#define PAGED_OUT_FUNCTION
#endif

#include "globals.h"

#define ZwCloseSafe(h) if(h) { ZwClose(h); h = NULL; }
#define ExFreePoolSafe(p) if(p) { ExFreePool(p); p = NULL; }

#ifdef USE_WINDDK
NTSTATUS
NTAPI
ZwDeviceIoControlFile(
  IN HANDLE  DeviceHandle,
  IN HANDLE  Event  OPTIONAL,
  IN PIO_APC_ROUTINE  UserApcRoutine  OPTIONAL,
  IN PVOID  UserApcContext  OPTIONAL,
  OUT PIO_STATUS_BLOCK  IoStatusBlock,
  IN ULONG  IoControlCode,
  IN PVOID  InputBuffer,
  IN ULONG  InputBufferSize,
  OUT PVOID  OutputBuffer,
  IN ULONG  OutputBufferSize);

NTSTATUS
NTAPI
NtWaitForSingleObject(
  IN HANDLE  ObjectHandle,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  TimeOut  OPTIONAL);

PDEVICE_OBJECT
NTAPI
IoGetAttachedDevice(
  IN PDEVICE_OBJECT  DeviceObject);

#define MAX_PATH 260
#endif

#ifndef NOMINMAX
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#endif

ULONGLONG _rdtsc(void);

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

BOOLEAN  NTAPI RtlCreateUnicodeString(PUNICODE_STRING,LPCWSTR);
NTSTATUS NTAPI ZwQueryDirectoryFile(HANDLE,HANDLE,PIO_APC_ROUTINE,
				     PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,
				     FILE_INFORMATION_CLASS,BOOLEAN,
				     PUNICODE_STRING,BOOLEAN);
NTSTATUS NTAPI ZwFsControlFile(HANDLE FileHandle,
				HANDLE Event, /* optional */
				PIO_APC_ROUTINE ApcRoutine, /* optional */
				PVOID ApcContext, /* optional */
				PIO_STATUS_BLOCK IoStatusBlock,	
				ULONG FsControlCode,
				PVOID InputBuffer, /* optional */
				ULONG InputBufferLength,
				PVOID OutputBuffer, /* optional */
				ULONG OutputBufferLength
				);
NTSTATUS NTAPI ZwQueryVolumeInformationFile(HANDLE,PIO_STATUS_BLOCK,
					     PVOID,ULONG,FS_INFORMATION_CLASS);
NTSTATUS NTAPI ZwDeleteFile(IN POBJECT_ATTRIBUTES ObjectAttributes);

char *__cdecl _itoa(int value,char *str,int radix);

#if defined(__GNUC__)
ULONGLONG __stdcall _aulldiv(ULONGLONG n, ULONGLONG d);
ULONGLONG __stdcall _alldiv(ULONGLONG n, ULONGLONG d);
ULONGLONG __stdcall _aullrem(ULONGLONG u, ULONGLONG v);
#endif

/*
 * NOTE! NEXT_PTR MUST BE FIRST MEMBER OF STRUCTURES!
 */
typedef struct _LIST {
	struct _LIST *next_ptr;
} LIST, *PLIST;

typedef struct _tagBLOCKMAP {
	struct _tagBLOCKMAP *next_ptr;
	ULONGLONG vcn; /* useful for compressed files */
	ULONGLONG lcn;
	ULONGLONG length;
} BLOCKMAP, *PBLOCKMAP;

typedef struct _tagFREEBLOCKMAP {
	struct _tagFREEBLOCKMAP *next_ptr;
	ULONGLONG lcn;
	ULONGLONG length;
} FREEBLOCKMAP, *PFREEBLOCKMAP;

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

typedef struct _FRAGMENTED {
	struct _FRAGMENTED *next_ptr;
	FILENAME *pfn;
} FRAGMENTED, *PFRAGMENTED;

#define NTFS_PARTITION 0x7

#define QUEUE_WAIT_INTERVAL (-1 * 10000) /* in 100 ns intervals */
#define CHECKPOINT_WAIT_INTERVAL 30 /* sec */ //-300 * 10000 /* in 100 ns intervals */
#define WAIT_CMD_INTERVAL (-200 * 10000) /* 200 ms */

#define _256K    (256 * 1024)

typedef struct _PROCESS_BLOCK_STRUCT
{
	struct _PROCESS_BLOCK_STRUCT *next_ptr;
	ULONGLONG start;
	ULONGLONG len;
} PROCESS_BLOCK_STRUCT, *PPROCESS_BLOCK_STRUCT;

typedef struct _OFFSET
{
	struct _OFFSET *next_ptr;
	int offset;
} OFFSET, *POFFSET;

typedef struct _FILTER
{
	short *buffer;
	POFFSET offsets;
} FILTER, *PFILTER;

/* Define UDEFRAG_DEVICE_EXTENSION structure. Include pointer
 * to FDO (for simple realization of UnloadRoutine) and 
 * symbolic link name in format UNOCODE_STRING.
 */
typedef struct _UDEFRAG_DEVICE_EXTENSION
{
	PDEVICE_OBJECT fdo;
	UNICODE_STRING log_path;
	PFREEBLOCKMAP free_space_map;
	PFILENAME filelist;
	PFRAGMENTED fragmfileslist;
	PBLOCKMAP lastblock;
	PFREEBLOCKMAP lastfreeblock;
	ULONG filecounter;
	ULONG dircounter;
	ULONG compressedcounter;
	ULONG fragmfilecounter;
	ULONG fragmcounter;
	UCHAR current_operation;
	ULONGLONG clusters_to_move_initial;
	ULONGLONG clusters_to_move_tmp;
	ULONGLONG clusters_to_move;
	ULONGLONG clusters_to_compact_initial;
	ULONGLONG clusters_to_compact;
	ULONGLONG clusters_to_compact_tmp;
	/* Cluster map: bytes are set to FREE_SPACE, SYSTEM_SPACE etc. */
//	UCHAR *cluster_map;
	ULONGLONG clusters_total;
	ULONGLONG clusters_per_cell;
	ULONGLONG clusters_per_last_cell; /* last block can represent more clusters */
	BOOLEAN opposite_order; /* if true then number of clusters is less than number of blocks */
	ULONGLONG cells_per_cluster;
	ULONGLONG cells_per_last_cluster;
	ULONGLONG total_space;
	ULONGLONG free_space; /* in bytes */
	HANDLE hVol;
	UCHAR letter;
	/* Buffer to read file mapping information into */
	ULONGLONG *FileMap;
	UCHAR *BitMap;
	NTSTATUS status;
	KEVENT sync_event;
	KEVENT stop_event;
	KSPIN_LOCK spin_lock;
	unsigned char partition_type;
	ULONG mft_size;
	ULONGLONG bytes_per_cluster;
	ULONGLONG sizelimit;
	BOOLEAN compact_flag;
	BOOLEAN xp_compatible; /* true for NT 5.1 and later versions */
	PROCESS_BLOCK_STRUCT *no_checked_blocks;
	ULONG unprocessed_blocks; /* number of no-checked blocks */
	FILTER in_filter;
	FILTER ex_filter;
	POFFSET in_offsets;
	POFFSET ex_offsets;
	REPORT_TYPE report_type;
	ULONGLONG processed_clusters;
	short *tmp_buf;
} UDEFRAG_DEVICE_EXTENSION, *PUDEFRAG_DEVICE_EXTENSION;

/* Function Prototypes */
NTSTATUS Analyse(UDEFRAG_DEVICE_EXTENSION *dx);
void ProcessMFT(UDEFRAG_DEVICE_EXTENSION *dx);
BOOLEAN FindFiles(UDEFRAG_DEVICE_EXTENSION *dx,UNICODE_STRING *path,BOOLEAN is_root);
BOOLEAN DumpFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);
void RedumpFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);
void ProcessBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG len, int space_state);
void ProcessFreeBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG len);
void ProcessUnfragmentedBlock(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,ULONGLONG start, ULONGLONG len);
void MarkSpace(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);
BOOLEAN FillFreeClusterMap(UDEFRAG_DEVICE_EXTENSION *dx);
BOOLEAN GetTotalClusters(UDEFRAG_DEVICE_EXTENSION *dx);
void FreeAllBuffers(UDEFRAG_DEVICE_EXTENSION *dx);
void Defragment(UDEFRAG_DEVICE_EXTENSION *dx);
NTSTATUS __MoveFile(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,ULONGLONG startVcn, ULONGLONG targetLcn, ULONGLONG n_clusters);
NTSTATUS MoveBlocksOfFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,HANDLE hFile,ULONGLONG target,ULONGLONG clusters_per_block);
void DeleteBlockmap(PFILENAME pfn);
BOOLEAN DefragmentFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);
void DefragmentFreeSpace(UDEFRAG_DEVICE_EXTENSION *dx);
BOOLEAN validate_letter(char letter);
void PrepareDataFields(UDEFRAG_DEVICE_EXTENSION *dx);
void InsertFreeSpaceBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG length);
FREEBLOCKMAP *InsertNewFreeBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG length);
BOOLEAN InsertFragmFileBlock(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);

BOOLEAN SaveFragmFilesListToDisk(UDEFRAG_DEVICE_EXTENSION *dx);
void DeleteLogFile(UDEFRAG_DEVICE_EXTENSION *dx);

void ApplyFilter(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);

LIST* NTAPI InsertFirstItem(PLIST *phead,ULONG size);
LIST* NTAPI InsertMiddleItem(PLIST *pprev,PLIST *pnext,ULONG size);
LIST* NTAPI InsertLastItem(PLIST *phead,PLIST *plast,ULONG size);
LIST* NTAPI RemoveItem(PLIST *phead,PLIST *pprev,PLIST *pcurrent);
void NTAPI DestroyList(PLIST *phead);

NTSTATUS NTAPI __NtFlushBuffersFile(HANDLE hFile);

void DbgPrintNoMem();

void MarkAllSpaceAsSystem0(UDEFRAG_DEVICE_EXTENSION *dx);
void MarkAllSpaceAsSystem1(UDEFRAG_DEVICE_EXTENSION *dx);

NTSTATUS OpenVolume(UDEFRAG_DEVICE_EXTENSION *dx);
NTSTATUS GetVolumeInfo(UDEFRAG_DEVICE_EXTENSION *dx);
void CloseVolume(UDEFRAG_DEVICE_EXTENSION *dx);

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

/* This is the definition for the data structure that is passed in to
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

/* Invalid longlong number */
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

#endif /* _DRIVER_H_ */
