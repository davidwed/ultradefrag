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

#if defined(__GNUC__)
#include <ddk/ntddk.h>
#include <ddk/ntdddisk.h>
#else
#include <ntddk.h>
#include <ntdddisk.h>
#endif

#include <windef.h> /* for MAX_PATH constant */
#include <ctype.h>
#include <stdio.h> /* for _snwprintf */
#include "../include/ultradfg.h"

#ifdef USE_WINDDK
#include <ntimage.h>
#endif

typedef unsigned int UINT;

#if defined(__GNUC__)
#define PLACE_IN_SECTION(s)	__attribute__((section (s)))
#define INIT_FUNCTION       PLACE_IN_SECTION("INIT")
#define PAGED_OUT_FUNCTION  PLACE_IN_SECTION("PAGE")
#else
#define INIT_FUNCTION
#define PAGED_OUT_FUNCTION
#endif

/************************** DEBUGGING ISSUES ***************************/

/*
* Alter's DbgPrint logger is a good alternative for DbgView
* program, especially on NT 4.0 system.
* The latest version of logger can be downloaded from
* http://alter.org.ua/en/soft/win/dbgdump/
* To collect messages during console/gui execution click on 
* DbgPrintLog.exe icon; to do this at boot time, use the following:
* DbgPrintLog.exe --full -T DTN -wd c:\ --drv:inst 1 
* 	--svc:inst A --drv:opt DoNotPassMessagesDown 1 udefrag.log
*/

/*
* 21 Dec. 2008
* LOG file on disk is better than unsaved messages after a crash.
*/

void __stdcall RegisterBugCheckCallbacks(void);
void __stdcall DeregisterBugCheckCallbacks(void);

BOOLEAN __stdcall OpenLog();
void __stdcall CloseLog();

/*
* Fucked nt 4.0 and w2k kernels don't have _vsnwprintf() call, 
* fucked MS C compiler is not compatible with ANSI C Standard - 
* __VA_ARGS__ support is missing. Therefore we need something like 
* this imperfect definition.
*/
void __cdecl DebugPrint(char *format, short *ustring, ...);

BOOLEAN CheckForSystemVolume(void);

//#define FLUSH_DBG_CACHE() DebugPrint("FLUSH_DBG_CACHE\n",NULL);

#if 0 /* because windows ddk isn't compatible with ANSI C Standard */
#define DebugPrint0(...) DebugPrint(__VA_ARGS__)
#define DebugPrint1(...) { if(dbg_level > 0) DebugPrint(__VA_ARGS__); }
#define DebugPrint2(...) { if(dbg_level > 1) DebugPrint(__VA_ARGS__); }
#else
#define DebugPrint0 DebugPrint
#define DebugPrint1 if(dbg_level < 1) {} else DebugPrint
#define DebugPrint2 if(dbg_level < 2) {} else DebugPrint
#endif

#if defined(__GNUC__)
typedef enum _KBUGCHECK_CALLBACK_REASON {
    KbCallbackInvalid,
    KbCallbackReserved1,
    KbCallbackSecondaryDumpData,
    KbCallbackDumpIo,
} KBUGCHECK_CALLBACK_REASON;

typedef struct _KBUGCHECK_REASON_CALLBACK_RECORD {
    LIST_ENTRY Entry;
    VOID *CallbackRoutine;
    PUCHAR Component;
    ULONG_PTR Checksum;
    KBUGCHECK_CALLBACK_REASON Reason;
    UCHAR State;
} KBUGCHECK_REASON_CALLBACK_RECORD, *PKBUGCHECK_REASON_CALLBACK_RECORD;

typedef
VOID
(*PKBUGCHECK_REASON_CALLBACK_ROUTINE) (
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN struct _KBUGCHECK_REASON_CALLBACK_RECORD* Record,
    IN OUT PVOID ReasonSpecificData,
    IN ULONG ReasonSpecificDataLength
    );
	
#endif /* __GNUC__ */

#ifndef USE_WINDDK
typedef struct _KBUGCHECK_SECONDARY_DUMP_DATA {
    IN PVOID InBuffer;
    IN ULONG InBufferLength;
    IN ULONG MaximumAllowed;
    OUT GUID Guid;
    OUT PVOID OutBuffer;
    OUT ULONG OutBufferLength;
} KBUGCHECK_SECONDARY_DUMP_DATA, *PKBUGCHECK_SECONDARY_DUMP_DATA;
#endif /* USE_WINDDK */

/*********************** END OF DEBUGGING ISSUES ***********************/

#define TEMP_BUFFER_CHARS 32768

/*
#ifdef NT4_TARGET
#define Nt_MmGetSystemAddressForMdl(addr) MmGetSystemAddressForMdl(addr)
#else
#define Nt_MmGetSystemAddressForMdl(addr) MmGetSystemAddressForMdlSafe(addr,NormalPagePriority)
#endif
*/

PVOID NTAPI Nt_MmGetSystemAddressForMdl(PMDL addr);

#include "globals.h"

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
	DebugPrint(invalid_request,NULL); \
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
NTSTATUS NTAPI NtNotifyChangeDirectoryFile(HANDLE,HANDLE,PIO_APC_ROUTINE,
		PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,ULONG,BOOLEAN);

//
//  System Information Classes for NtQuerySystemInformation
//
typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemBasicInformation,
    SystemProcessorInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation, /// Obsolete: Use KUSER_SHARED_DATA
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
    SystemFlagsInformation,
    SystemCallTimeInformation,
    SystemModuleInformation,
    SystemLocksInformation,
    SystemStackTraceInformation,
    SystemPagedPoolInformation,
    SystemNonPagedPoolInformation,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPageFileInformation,
    SystemVdmInstemulInformation,
    SystemVdmBopInformation,
    SystemFileCacheInformation,
    SystemPoolTagInformation,
    SystemInterruptInformation,
    SystemDpcBehaviorInformation,
    SystemFullMemoryInformation,
    SystemLoadGdiDriverInformation,
    SystemUnloadGdiDriverInformation,
    SystemTimeAdjustmentInformation,
    SystemSummaryMemoryInformation,
    SystemNextEventIdInformation,
    SystemEventIdsInformation,
    SystemCrashDumpInformation,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemExtendServiceTableInformation,
    SystemPrioritySeperation,
    SystemPlugPlayBusInformation,
    SystemDockInformation,
    SystemPowerInformationNative,
    SystemProcessorSpeedInformation,
    SystemCurrentTimeZoneInformation,
    SystemLookasideInformation,
    SystemTimeSlipNotification,
    SystemSessionCreate,
    SystemSessionDetach,
    SystemSessionInformation,
    SystemRangeStartInformation,
    SystemVerifierInformation,
    SystemAddVerifier,
    SystemSessionProcessesInformation,
    SystemInformationClassMax
} SYSTEM_INFORMATION_CLASS;

// Class 11
typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY
{
    ULONG  Unknown1;
    ULONG  Unknown2;
#ifdef _WIN64
	ULONG Unknown3;
	ULONG Unknown4;
#endif
    PVOID  Base;
    ULONG  Size;
    ULONG  Flags;
    USHORT  Index;
    USHORT  NameLength;
    USHORT  LoadCount;
    USHORT  PathLength;
    CHAR  ImageName[256];
} SYSTEM_MODULE_INFORMATION_ENTRY, *PSYSTEM_MODULE_INFORMATION_ENTRY;
typedef struct _SYSTEM_MODULE_INFORMATION
{
    ULONG Count;
    SYSTEM_MODULE_INFORMATION_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

NTSTATUS NTAPI ZwQuerySystemInformation(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
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

/*
* Since v3.1.0 we are using paged pool when possible.
* Because Windows has a strong limitation: if all nonpaged 
* memory will be allocated then any Windows driver may crash system.
* With BAD_POOL_CALLER bug code or 0x24 (NTFS_FILE_SYSTEM) for ntfs.sys driver.
*/
#define AllocateNonPagedPool(size) AllocatePool(NonPagedPool,size)
#define AllocatePagedPool(size) AllocatePool(PagedPool,size)

/* FIXME: RtlCreateUnicodeString allocates memory from which pool??? */
/* It seems that system allocates memory from paged pool. :) */

//#ifdef NT4_TARGET

/* ExFreePool must be always ExFreePool call, nothing more! */
#undef ExFreePool
#ifdef USE_WINDDK
DECLSPEC_IMPORT VOID NTAPI ExFreePool(IN PVOID P);
#endif /* USE_WINDDK */

VOID NTAPI Nt_ExFreePool(PVOID);

/* Safe versions of some frequently used functions. */
#define ZwCloseSafe(h) if(h) { ZwClose(h); h = NULL; }
#define ExFreePoolSafe(p) if(p) { Nt_ExFreePool(p); p = NULL; }

//#else /* NT4_TARGET */

//#ifndef USE_WINDDK
//#undef ExFreePool
//#define ExFreePool(a) ExFreePoolWithTag(a,0)
//#endif /* USE_WINDDK */

//#endif /* NT4_TARGET */

/*
* NOTE! NEXT_PTR MUST BE FIRST MEMBER OF THESE STRUCTURES!
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
} FILENAME, *PFILENAME;

#define DeleteBlockmap(pfn) DestroyList((PLIST *)&(pfn)->blockmap)

/* structure to store fragmented item */
typedef struct _FRAGMENTED {
	struct _FRAGMENTED *next_ptr;
	struct _FRAGMENTED *prev_ptr;
	FILENAME *pfn;
} FRAGMENTED, *PFRAGMENTED;

#define FLOPPY_FAT12_PARTITION 0x0 /* really not defined */
#define FAT16_PARTITION        0x6
#define NTFS_PARTITION         0x7
#define FAT32_PARTITION        0xB

#define _256K (256 * 1024)

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
	int second_device;
	int map_device;
	int stat_device;
	int stop_device;
	struct _UDEFRAG_DEVICE_EXTENSION *main_dx;
	UNICODE_STRING log_path;
	/*
	* All fields between markers will be set
	* to zero state before each analysis.
	*/
	MARKER z_start;
	PFREEBLOCKMAP free_space_map;
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
	ULONGLONG clusters_to_process;
	ULONGLONG processed_clusters;
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
 	/*
	* End of the data with initial zero state.
	*/
	MARKER z0_end;
	UCHAR current_operation;
	UCHAR letter;
	ULONGLONG bytes_per_cluster;
	ULONGLONG sizelimit;
	BOOLEAN compact_flag;
	ULONG disable_reports;
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
void ProcessMFT(UDEFRAG_DEVICE_EXTENSION *dx);
BOOLEAN FindFiles(UDEFRAG_DEVICE_EXTENSION *dx,UNICODE_STRING *path);
BOOLEAN DumpFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);
void ProcessBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG len, int space_state,int old_space_state);
void ProcessFreeBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG len,UCHAR old_space_state);
void MarkSpace(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,int old_space_state);
NTSTATUS FillFreeSpaceMap(UDEFRAG_DEVICE_EXTENSION *dx);
void FreeAllBuffers(UDEFRAG_DEVICE_EXTENSION *dx);
void Defragment(UDEFRAG_DEVICE_EXTENSION *dx);
NTSTATUS MovePartOfFile(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,ULONGLONG startVcn, ULONGLONG targetLcn, ULONGLONG n_clusters);
NTSTATUS MoveBlocksOfFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,HANDLE hFile,ULONGLONG target);
BOOLEAN DefragmentFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);
void DefragmentFreeSpace(UDEFRAG_DEVICE_EXTENSION *dx);
void Optimize(UDEFRAG_DEVICE_EXTENSION *dx);
void InitDX(UDEFRAG_DEVICE_EXTENSION *dx);
void InitDX_0(UDEFRAG_DEVICE_EXTENSION *dx);
void InsertFreeSpaceBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG length,UCHAR old_space_state);
FREEBLOCKMAP *InsertLastFreeBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG length);
BOOLEAN SaveFragmFilesListToDisk(UDEFRAG_DEVICE_EXTENSION *dx);
void DeleteLogFile(UDEFRAG_DEVICE_EXTENSION *dx);
void ApplyFilter(UDEFRAG_DEVICE_EXTENSION *dx);
LIST * NTAPI InsertItem(PLIST *phead,PLIST prev,ULONG size,POOL_TYPE pool_type);
void NTAPI RemoveItem(PLIST *phead,PLIST item);
void NTAPI DestroyList(PLIST *phead);
NTSTATUS AllocateMap(ULONG size);
void MarkAllSpaceAsSystem0(UDEFRAG_DEVICE_EXTENSION *dx);
void MarkAllSpaceAsSystem1(UDEFRAG_DEVICE_EXTENSION *dx);
void GetMap(char *dest);
NTSTATUS OpenVolume(UDEFRAG_DEVICE_EXTENSION *dx);
NTSTATUS GetVolumeInfo(UDEFRAG_DEVICE_EXTENSION *dx);
void CloseVolume(UDEFRAG_DEVICE_EXTENSION *dx);
void TruncateFreeSpaceBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG length);
void UpdateFilter(UDEFRAG_DEVICE_EXTENSION *dx,PFILTER pf,short *buffer,int length);
void DestroyFilter(UDEFRAG_DEVICE_EXTENSION *dx);
void UpdateFragmentedFilesList(UDEFRAG_DEVICE_EXTENSION *dx);
unsigned char GetSpaceState(PFILENAME pfn);
void stop_all_requests(void);
NTSTATUS OpenTheFile(PFILENAME pfn,HANDLE *phFile);
PVOID KernelGetModuleBase(PCHAR pModuleName);
PVOID KernelGetProcAddress(PVOID ModuleBase,PCHAR pFunctionName);
BOOLEAN IsStringInFilter(short *str,PFILTER pf);
BOOLEAN CheckForContextMenuHandler(UDEFRAG_DEVICE_EXTENSION *dx);

#define FIND_DATA_SIZE	(16*1024)

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

/* cannot be retrieved from pFileInfo */
#define IS_HARD_LINK(pFileInfo) FALSE

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
