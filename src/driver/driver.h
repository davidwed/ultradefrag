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

/*
* VERY IMPORTANT NOTES:
* 1. Stack can be paged out.
* 2. RtlCreateUnicodeString allocates memory from paged pool.
* 3. Never specify OBJ_KERNEL_HANDLE flag in
*    InitializeObjectAttributes() call before ZwCreateFile()
*    on NT 4.0. On w2k and later systems always set this flag!
*/

/* compiler specific definitions */
#if !defined(__GNUC__)
#ifndef USE_WINDDK
#define __MSVC__
#define _PCR
#endif
#pragma warning(disable:4103) /* used #pragma pack to change alignment */
#endif

#define DBG 1 /* it's very useful and must always be specified */

/* include standard DDK headers */
#if defined(__GNUC__)
#include <ddk/ntddk.h>
#include <ddk/ntdddisk.h>
#else
#include <ntddk.h>
#include <ntdddisk.h>
#endif

/* include additional standard headers */
#include <windef.h> /* for MAX_PATH constant */
#include <ctype.h>
#include <stdio.h> /* for _snwprintf */
#include "../include/ultradfg.h"

#ifdef USE_WINDDK
#include <ntimage.h>
#endif

/*
* We cannot include standard header here 
* due to some compilation problems.
*/
typedef unsigned int UINT;

/* section attributes definition */
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
* 12 Sept. 2009
* Due to cool DbgView program simple DbgPrint() 
* calls are absolutely enough for debugging purposes.
* Read more about debugging techniques in 
* \doc\html\handbook\reporting_bugs.html
*/

#if 0 /* because windows ddk isn't compatible with ANSI C Standard */
#define DebugPrint(...) DbgPrint(__VA_ARGS__)
#define DebugPrint0(...) DbgPrint(__VA_ARGS__)
#define DebugPrint1(...) { if(dbg_level > 0) DbgPrint(__VA_ARGS__); }
#define DebugPrint2(...) { if(dbg_level > 1) DbgPrint(__VA_ARGS__); }
#else
#define DebugPrint DbgPrint
#define DebugPrint0 DbgPrint
#define DebugPrint1 if(dbg_level < 1) {} else DbgPrint
#define DebugPrint2 if(dbg_level < 2) {} else DbgPrint
#endif

void __stdcall RegisterBugCheckCallbacks(void);
void __stdcall DeregisterBugCheckCallbacks(void);

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

/* UltraDefrag device extension and other internal structures */

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
	ULONGLONG mft_size;
	ULONG ntfs_record_size;
	ULONGLONG max_mft_entries;
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
	FILTER in_filter;
	FILTER ex_filter;
	HANDLE hVol;
	ULONGLONG sizelimit;
	ULONGLONG fraglimit;
	UCHAR current_operation;
	UCHAR letter;
	BOOLEAN compact_flag;
	ULONG disable_reports;
	ULONGLONG bytes_per_cluster;
	ULONG bytes_per_sector;
	ULONG sectors_per_cluster;
	ULONG FatCountOfClusters; /* FAT specific */
 	/*
	* End of the data with initial zero state.
	*/
	MARKER z0_end;
	/* nt 4.0 specific */
	ULONGLONG nextLcn;
	ULONGLONG *pnextLcn;
	MOVEFILE_DESCRIPTOR moveFile;
	MOVEFILE_DESCRIPTOR *pmoveFile;
	ULONGLONG startVcn;
	ULONGLONG *pstartVcn;
} UDEFRAG_DEVICE_EXTENSION, *PUDEFRAG_DEVICE_EXTENSION;

/* internal function prototypes */
void ApplyFilter(UDEFRAG_DEVICE_EXTENSION *dx);
void CheckForFatPartition(UDEFRAG_DEVICE_EXTENSION *dx);
void CheckForNtfsPartition(UDEFRAG_DEVICE_EXTENSION *dx);
void CleanupFreeSpaceList(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG len);
void CloseVolume(UDEFRAG_DEVICE_EXTENSION *dx);
int  Defragment(UDEFRAG_DEVICE_EXTENSION *dx);
void DefragmentFreeSpace(UDEFRAG_DEVICE_EXTENSION *dx);
void DeleteLogFile(UDEFRAG_DEVICE_EXTENSION *dx);
void DestroyFilter(UDEFRAG_DEVICE_EXTENSION *dx);
void DbgPrintFreeSpaceList(UDEFRAG_DEVICE_EXTENSION *dx);
void FreeAllBuffers(UDEFRAG_DEVICE_EXTENSION *dx);
void GenerateFragmentedFilesList(UDEFRAG_DEVICE_EXTENSION *dx);
void GetMap(char *dest);
void InitDX(UDEFRAG_DEVICE_EXTENSION *dx);
void InitDX_0(UDEFRAG_DEVICE_EXTENSION *dx);
void InsertFreeSpaceBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG length,UCHAR old_space_state);
void MarkAllSpaceAsSystem0(UDEFRAG_DEVICE_EXTENSION *dx);
void MarkAllSpaceAsSystem1(UDEFRAG_DEVICE_EXTENSION *dx);
void MarkSpace(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,int old_space_state);
void Optimize(UDEFRAG_DEVICE_EXTENSION *dx);
void ProcessBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG len, int space_state,int old_space_state);
void ProcessFreeBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG len,UCHAR old_space_state);
void TruncateFreeSpaceBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG length);
void UpdateFilter(UDEFRAG_DEVICE_EXTENSION *dx,PFILTER pf,short *buffer,int length);
void UpdateFragmentedFilesList(UDEFRAG_DEVICE_EXTENSION *dx);

BOOLEAN CheckForContextMenuHandler(UDEFRAG_DEVICE_EXTENSION *dx);
BOOLEAN ConsoleUnwantedStuffDetected(UDEFRAG_DEVICE_EXTENSION *dx,WCHAR *Path,ULONG *InsideFlag);
BOOLEAN DefragmentFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);
BOOLEAN DumpFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);
BOOLEAN FindFiles(UDEFRAG_DEVICE_EXTENSION *dx,WCHAR *ParentDirectoryPath);
BOOLEAN InsertFragmentedFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn);
BOOLEAN IsStringInFilter(short *str,PFILTER pf);
BOOLEAN SaveFragmFilesListToDisk(UDEFRAG_DEVICE_EXTENSION *dx);
BOOLEAN ScanFat12Partition(UDEFRAG_DEVICE_EXTENSION *dx);
BOOLEAN ScanFat16Partition(UDEFRAG_DEVICE_EXTENSION *dx);
BOOLEAN ScanFat32Partition(UDEFRAG_DEVICE_EXTENSION *dx);
BOOLEAN ScanMFT(UDEFRAG_DEVICE_EXTENSION *dx);

ULONGLONG _rdtsc(void);
ULONGLONG _rdtsc_1(void);

NTSTATUS AllocateMap(ULONG size);
NTSTATUS Analyse(UDEFRAG_DEVICE_EXTENSION *dx);
NTSTATUS AnalyseFreeSpace(UDEFRAG_DEVICE_EXTENSION *dx);
NTSTATUS FillFreeSpaceMap(UDEFRAG_DEVICE_EXTENSION *dx);
NTSTATUS MoveBlocksOfFile(UDEFRAG_DEVICE_EXTENSION *dx,PFILENAME pfn,HANDLE hFile,ULONGLONG target);
NTSTATUS MovePartOfFile(UDEFRAG_DEVICE_EXTENSION *dx,HANDLE hFile,ULONGLONG startVcn, ULONGLONG targetLcn, ULONGLONG n_clusters);
NTSTATUS OpenTheFile(PFILENAME pfn,HANDLE *phFile);
NTSTATUS OpenVolume(UDEFRAG_DEVICE_EXTENSION *dx);
NTSTATUS ReadSectors(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG lsn,PVOID buffer,ULONG length);

LIST * NTAPI InsertItem(PLIST *phead,PLIST prev,ULONG size,POOL_TYPE pool_type);
void NTAPI RemoveItem(PLIST *phead,PLIST item);
void NTAPI DestroyList(PLIST *phead);

PVOID KernelGetModuleBase(PCHAR pModuleName);
PVOID KernelGetProcAddress(PVOID ModuleBase,PCHAR pFunctionName);

FREEBLOCKMAP *InsertLastFreeBlock(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG start,ULONGLONG length);

unsigned char GetSpaceState(PFILENAME pfn);

void stop_all_requests(void);

#define DeleteBlockmap(pfn) DestroyList((PLIST *)&(pfn)->blockmap)

/* os version independent functions prototypes */
VOID NTAPI Nt_ExFreePool(PVOID);
ULONGLONG NTAPI Nt_KeQueryInterruptTime(VOID);
PVOID NTAPI Nt_MmGetSystemAddressForMdl(PMDL addr);

/* exported function prototypes */
NTSTATUS NTAPI DeviceControlRoutine(PDEVICE_OBJECT fdo,PIRP Irp);
VOID     NTAPI UnloadRoutine(PDRIVER_OBJECT DriverObject);
NTSTATUS NTAPI Read_IRPhandler(PDEVICE_OBJECT fdo,PIRP Irp);
NTSTATUS NTAPI Write_IRPhandler(PDEVICE_OBJECT fdo,PIRP Irp);
NTSTATUS NTAPI Create_File_IRPprocessing(PDEVICE_OBJECT fdo,PIRP Irp);
NTSTATUS NTAPI Close_HandleIRPprocessing(PDEVICE_OBJECT fdo,PIRP Irp);

/* definitions missing in standard headers */
#ifdef USE_WINDDK
NTSTATUS NTAPI ZwDeviceIoControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,
		PVOID,PIO_STATUS_BLOCK,ULONG,PVOID,ULONG,PVOID,ULONG);
NTSTATUS NTAPI NtWaitForSingleObject(HANDLE,BOOLEAN,PLARGE_INTEGER);
NTSTATUS NTAPI ZwWaitForSingleObject(HANDLE,BOOLEAN,PLARGE_INTEGER);
PDEVICE_OBJECT NTAPI IoGetAttachedDevice(PDEVICE_OBJECT);
#define MAX_PATH 260
#endif

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

/* ExFreePool must be always ExFreePool call, nothing more! */
#undef ExFreePool
#ifdef USE_WINDDK
DECLSPEC_IMPORT VOID NTAPI ExFreePool(IN PVOID P);
#endif /* USE_WINDDK */

#ifndef OBJ_KERNEL_HANDLE
#define OBJ_KERNEL_HANDLE    0x00000200
#endif

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

typedef struct _FILE_FS_SIZE_INFORMATION {
    LARGE_INTEGER   TotalAllocationUnits;
    LARGE_INTEGER   AvailableAllocationUnits;
    ULONG           SectorsPerAllocationUnit;
    ULONG           BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

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
NTSTATUS NTAPI ZwQuerySystemInformation(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI ZwDeleteFile(IN POBJECT_ATTRIBUTES);
char *__cdecl _itoa(int value,char *str,int radix);

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

/* UltraDefrag specific definitions */
#define _256K (256 * 1024)
#define FIND_DATA_SIZE	(16*1024)
#define TEMP_BUFFER_CHARS 32768

#define FAT12_PARTITION        0x0 /* really not defined */
#define FAT16_PARTITION        0x6
#define NTFS_PARTITION         0x7
#define FAT32_PARTITION        0xB
#define UDF_PARTITION          0xC /* really not defined */
#define UNKNOWN_PARTITION      0xF /* really not defined */

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

/* Safe versions of some frequently used functions. */
#define ZwCloseSafe(h) if(h) { ZwClose(h); h = NULL; }
#define ExFreePoolSafe(p) if(p) { Nt_ExFreePool(p); p = NULL; }

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

#include "globals.h"

wchar_t * __cdecl wcsistr(const wchar_t * wcs1,const wchar_t * wcs2);

#define CHECK_FOR_FRAGLIMIT(dx,pfn) { \
	if(dx->fraglimit && pfn->n_fragments < dx->fraglimit) pfn->is_filtered = TRUE; \
}

#endif /* _DRIVER_H_ */
