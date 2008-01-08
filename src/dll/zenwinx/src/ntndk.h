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

#ifndef _NTNDK_H_
#define _NTNDK_H_

/* define base types */
#define ULONG_PTR unsigned long*
typedef int BOOL;
typedef const char *PCSZ;

#ifndef USE_WINDDK
typedef ULONG_PTR KAFFINITY;
typedef KAFFINITY *PKAFFINITY;
typedef ULONG (NTAPI *PTHREAD_START_ROUTINE)(PVOID Parameter);
#endif

/* define status codes */
/* ifndef directives are used to prevent warnings when gcc on mingw is used */
typedef LONG NTSTATUS;
#define STATUS_SUCCESS                ((NTSTATUS)0x00000000)
#ifndef STATUS_TIMEOUT
#define STATUS_TIMEOUT                ((NTSTATUS)0x00000102)
#endif
#ifndef STATUS_PENDING
#define STATUS_PENDING                ((NTSTATUS)0x00000103)
#endif
#ifndef STATUS_INVALID_HANDLE
#define STATUS_INVALID_HANDLE         ((NTSTATUS)0xC0000008)
#endif
#define STATUS_IMAGE_ALREADY_LOADED   ((NTSTATUS)0xC000010E)
#define STATUS_NOT_ALL_ASSIGNED       ((NTSTATUS)0x00000106)
#define STATUS_UNSUCCESSFUL           ((NTSTATUS)0xC0000001)
#define STATUS_NOT_IMPLEMENTED        ((NTSTATUS)0xC0000002)
#define STATUS_INVALID_INFO_CLASS     ((NTSTATUS)0xC0000003)
#define STATUS_INFO_LENGTH_MISMATCH   ((NTSTATUS)0xC0000004)
#ifndef STATUS_ACCESS_VIOLATION
#define STATUS_ACCESS_VIOLATION       ((NTSTATUS)0xC0000005)
#endif
#ifndef STATUS_INVALID_HANDLE
#define STATUS_INVALID_HANDLE         ((NTSTATUS)0xC0000008)
#endif
#define STATUS_INVALID_PARAMETER      ((NTSTATUS)0xC000000D)
#define STATUS_NO_SUCH_DEVICE         ((NTSTATUS)0xC000000E)
#define STATUS_NO_SUCH_FILE           ((NTSTATUS)0xC000000F)
#define STATUS_INVALID_DEVICE_REQUEST ((NTSTATUS)0xC0000010)
#define STATUS_END_OF_FILE            ((NTSTATUS)0xC0000011)
#define STATUS_WRONG_VOLUME           ((NTSTATUS)0xC0000012)
#define STATUS_NO_MEDIA_IN_DEVICE     ((NTSTATUS)0xC0000013)
#ifndef STATUS_NO_MEMORY
#define STATUS_NO_MEMORY              ((NTSTATUS)0xC0000017)
#endif
#define STATUS_ACCESS_DENIED          ((NTSTATUS)0xC0000022)
#define STATUS_BUFFER_TOO_SMALL       ((NTSTATUS)0xC0000023)
#define STATUS_OBJECT_NAME_INVALID    ((NTSTATUS)0xC0000033)
#define STATUS_OBJECT_NAME_NOT_FOUND  ((NTSTATUS)0xC0000034)
#define STATUS_OBJECT_NAME_COLLISION  ((NTSTATUS)0xC0000035)
#define STATUS_OBJECT_PATH_INVALID    ((NTSTATUS)0xC0000039)
#define STATUS_OBJECT_PATH_NOT_FOUND  ((NTSTATUS)0xC000003A)
#define STATUS_OBJECT_PATH_SYNTAX_BAD ((NTSTATUS)0xC000003B)
#define STATUS_INVALID_INFO_CLASS     ((NTSTATUS)0xC0000003)
#ifndef NT_SUCCESS
#define NT_SUCCESS(x) ((x)>=0)
#endif

#if defined(__GNUC__)
#define MAX_WAIT_INTERVAL -0x7FFFFFFFFFFFFFFFLL
#else
/* c compiler from ms visual studio 6.0 don't supports LL suffix */
#define MAX_WAIT_INTERVAL -0x7FFFFFFFFFFFFFFF
#endif

/* define base nt structures */
typedef struct _STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING, *PSTRING;

typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;

typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
  ULONG Length;
  HANDLE RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;
  PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

#define OBJ_INHERIT          0x00000002
#define OBJ_PERMANENT        0x00000010
#define OBJ_EXCLUSIVE        0x00000020
#define OBJ_CASE_INSENSITIVE 0x00000040
#define OBJ_OPENIF           0x00000080
#define OBJ_OPENLINK         0x00000100
#define OBJ_KERNEL_HANDLE    0x00000200
#define OBJ_VALID_ATTRIBUTES (OBJ_KERNEL_HANDLE | OBJ_OPENLINK | \
		OBJ_OPENIF | OBJ_CASE_INSENSITIVE | OBJ_EXCLUSIVE | \
		OBJ_PERMANENT | OBJ_INHERIT)
#define InitializeObjectAttributes(p,n,a,r,s) \
    do { \
        (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
        (p)->RootDirectory = r; \
        (p)->Attributes = a; \
        (p)->ObjectName = n; \
        (p)->SecurityDescriptor = s; \
        (p)->SecurityQualityOfService = NULL; \
    } while (0)

typedef struct _IO_STATUS_BLOCK {
    NTSTATUS Status;
    ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef VOID(*PIO_APC_ROUTINE) (
				PVOID ApcContext,
				PIO_STATUS_BLOCK IoStatusBlock,
				ULONG Reserved
			);

/* additional nt structures */
typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_HEAP_DEFINITION {
    ULONG Length; /* = sizeof(RTL_HEAP_DEFINITION) */
    ULONG Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation = 0,
    SystemCpuInformation = 1,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3, /* was SystemTimeInformation */
    Unknown4,
    SystemProcessInformation = 5,
    Unknown6,
    Unknown7,
    SystemProcessorPerformanceInformation = 8,
    Unknown9,
    Unknown10,
    SystemModuleInformation = 11,
    Unknown12,
    Unknown13,
    Unknown14,
    Unknown15,
    SystemHandleInformation = 16,
    Unknown17,
    SystemPageFileInformation = 18,
    Unknown19,
    Unknown20,
    SystemCacheInformation = 21,
    Unknown22,
    SystemInterruptInformation = 23,
    SystemDpcBehaviourInformation = 24,
    SystemFullMemoryInformation = 25,
    SystemNotImplemented6 = 25,
    SystemLoadImage = 26,
    SystemUnloadImage = 27,
    SystemTimeAdjustmentInformation = 28,
    SystemTimeAdjustment = 28,
    SystemSummaryMemoryInformation = 29,
    SystemNotImplemented7 = 29,
    SystemNextEventIdInformation = 30,
    SystemNotImplemented8 = 30,
    SystemEventIdsInformation = 31,
    SystemCrashDumpInformation = 32,
    SystemExceptionInformation = 33,
    SystemCrashDumpStateInformation = 34,
    SystemKernelDebuggerInformation = 35,
    SystemContextSwitchInformation = 36,
    SystemRegistryQuotaInformation = 37,
    SystemCurrentTimeZoneInformation = 44,
    SystemTimeZoneInformation = 44,
    SystemLookasideInformation = 45,
    SystemSetTimeSlipEvent = 46,
    SystemCreateSession = 47,
    SystemDeleteSession = 48,
    SystemInvalidInfoClass4 = 49,
    SystemRangeStartInformation = 50,
    SystemVerifierInformation = 51,
    SystemAddVerifier = 52,
    SystemSessionProcessesInformation	= 53,
    SystemInformationClassMax
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_BASIC_INFORMATION
{
    ULONG Reserved;
    ULONG TimerResolution;
    ULONG PageSize;
    ULONG NumberOfPhysicalPages;
    ULONG LowestPhysicalPageNumber;
    ULONG HighestPhysicalPageNumber;
    ULONG AllocationGranularity;
    ULONG MinimumUserModeAddress;
    ULONG MaximumUserModeAddress;
    KAFFINITY ActiveProcessorsAffinityMask;
    CCHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleProcessTime;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
    ULONG IoReadOperationCount;
    ULONG IoWriteOperationCount;
    ULONG IoOtherOperationCount;
    ULONG AvailablePages;
    ULONG CommittedPages;
    ULONG CommitLimit;
    ULONG PeakCommitment;
    ULONG PageFaultCount;
    ULONG CopyOnWriteCount;
    ULONG TransitionCount;
    ULONG CacheTransitionCount;
    ULONG DemandZeroCount;
    ULONG PageReadCount;
    ULONG PageReadIoCount;
    ULONG CacheReadCount;
    ULONG CacheIoCount;
    ULONG DirtyPagesWriteCount;
    ULONG DirtyWriteIoCount;
    ULONG MappedPagesWriteCount;
    ULONG MappedWriteIoCount;
    ULONG PagedPoolPages;
    ULONG NonPagedPoolPages;
    ULONG PagedPoolAllocs;
    ULONG PagedPoolFrees;
    ULONG NonPagedPoolAllocs;
    ULONG NonPagedPoolFrees;
    ULONG FreeSystemPtes;
    ULONG ResidentSystemCodePage;
    ULONG TotalSystemDriverPages;
    ULONG TotalSystemCodePages;
    ULONG NonPagedPoolLookasideHits;
    ULONG PagedPoolLookasideHits;
    ULONG Spare3Count;
    ULONG ResidentSystemCachePage;
    ULONG ResidentPagedPoolPage;
    ULONG ResidentSystemDriverPage;
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadResourceMiss;
    ULONG CcFastReadNotPossible;
    ULONG CcFastMdlReadNoWait;
    ULONG CcFastMdlReadWait;
    ULONG CcFastMdlReadResourceMiss;
    ULONG CcFastMdlReadNotPossible;
    ULONG CcMapDataNoWait;
    ULONG CcMapDataWait;
    ULONG CcMapDataNoWaitMiss;
    ULONG CcMapDataWaitMiss;
    ULONG CcPinMappedDataCount;
    ULONG CcPinReadNoWait;
    ULONG CcPinReadWait;
    ULONG CcPinReadNoWaitMiss;
    ULONG CcPinReadWaitMiss;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    ULONG CcCopyReadWaitMiss;
    ULONG CcMdlReadNoWait;
    ULONG CcMdlReadWait;
    ULONG CcMdlReadNoWaitMiss;
    ULONG CcMdlReadWaitMiss;
    ULONG CcReadAheadIos;
    ULONG CcLazyWriteIos;
    ULONG CcLazyWritePages;
    ULONG CcDataFlushes;
    ULONG CcDataPages;
    ULONG ContextSwitches;
    ULONG FirstLevelTbFills;
    ULONG SecondLevelTbFills;
    ULONG SystemCalls;
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

#define DEVICE_TYPE DWORD

/* DEVICE_OBJECT.Characteristics */

#define FILE_REMOVABLE_MEDIA            0x00000001
#define FILE_READ_ONLY_DEVICE           0x00000002
#define FILE_FLOPPY_DISKETTE            0x00000004
#define FILE_WRITE_ONCE_MEDIA           0x00000008
#define FILE_REMOTE_DEVICE              0x00000010
#define FILE_DEVICE_IS_MOUNTED          0x00000020
#define FILE_VIRTUAL_VOLUME             0x00000040
#define FILE_AUTOGENERATED_DEVICE_NAME  0x00000080
#define FILE_DEVICE_SECURE_OPEN         0x00000100

typedef struct _FILE_FS_DEVICE_INFORMATION {
  DEVICE_TYPE  DeviceType;
  ULONG  Characteristics;
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation = 1,
    FileFsLabelInformation,
    FileFsSizeInformation,
    FileFsDeviceInformation,
    FileFsAttributeInformation,
    FileFsControlInformation,
    FileFsFullSizeInformation,
    FileFsObjectIdInformation,
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation = 0,
    ProcessQuotaLimits = 1,
    ProcessIoCounters = 2,
    ProcessVmCounters = 3,
    ProcessTimes = 4,
    ProcessBasePriority = 5,
    ProcessRaisePriority = 6,
    ProcessDebugPort = 7,
    ProcessExceptionPort = 8,
    ProcessAccessToken = 9,
    ProcessLdtInformation = 10,
    ProcessLdtSize = 11,
    ProcessDefaultHardErrorMode = 12,
    ProcessIoPortHandlers = 13,
    ProcessPooledUsageAndLimits = 14,
    ProcessWorkingSetWatch = 15,
    ProcessUserModeIOPL = 16,
    ProcessEnableAlignmentFaultFixup = 17,
    ProcessPriorityClass = 18,
    ProcessWx86Information = 19,
    ProcessHandleCount = 20,
    ProcessAffinityMask = 21,
    ProcessPriorityBoost = 22,
    ProcessDeviceMap = 23,
    ProcessSessionInformation = 24,
    ProcessForegroundInformation = 25,
    ProcessWow64Information = 26,
    ProcessImageFileName = 27,
    ProcessLUIDDeviceMapsEnabled = 28,
    ProcessBreakOnTermination = 29,
    ProcessDebugObjectHandle = 30,
    ProcessDebugFlags = 31,
    ProcessHandleTracing = 32,
    MaxProcessInfoClass
} PROCESSINFOCLASS, PROCESS_INFORMATION_CLASS;

#define SYMBOLIC_LINK_QUERY       0x0001
#define SYMBOLIC_LINK_ALL_ACCESS  (STANDARD_RIGHTS_REQUIRED | 0x1)

#ifndef FILE_OPEN
#define FILE_OPEN                 0x1
#endif

typedef struct RTL_DRIVE_LETTER_CURDIR
{
    USHORT              Flags;
    USHORT              Length;
    ULONG               TimeStamp;
    UNICODE_STRING      DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG               AllocationSize;
    ULONG               Size;
    ULONG               Flags;
    ULONG               DebugFlags;
    HANDLE              ConsoleHandle;
    ULONG               ConsoleFlags;
    HANDLE              hStdInput;
    HANDLE              hStdOutput;
    HANDLE              hStdError;
    CURDIR              CurrentDirectory;
    UNICODE_STRING      DllPath;
    UNICODE_STRING      ImagePathName;
    UNICODE_STRING      CommandLine;
    PWSTR               Environment;
    ULONG               dwX;
    ULONG               dwY;
    ULONG               dwXSize;
    ULONG               dwYSize;
    ULONG               dwXCountChars;
    ULONG               dwYCountChars;
    ULONG               dwFillAttribute;
    ULONG               dwFlags;
    ULONG               wShowWindow;
    UNICODE_STRING      WindowTitle;
    UNICODE_STRING      Desktop;
    UNICODE_STRING      ShellInfo;
    UNICODE_STRING      RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct tagRTL_BITMAP {
    ULONG  SizeOfBitMap; /* Number of bits in the bitmap */
    PULONG Buffer; /* Bitmap data, assumed sized to a DWORD boundary */
} RTL_BITMAP, *PRTL_BITMAP;

typedef struct _PEB_LDR_DATA
{
    ULONG               Length;
    BOOLEAN             Initialized;
    PVOID               SsHandle;
    LIST_ENTRY          InLoadOrderModuleList;
    LIST_ENTRY          InMemoryOrderModuleList;
    LIST_ENTRY          InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _GDI_TEB_BATCH
{
    ULONG  Offset;
    HANDLE HDC;
    ULONG  Buffer[0x136];
} GDI_TEB_BATCH;

/***********************************************************************
 * PEB data structure
 */
typedef struct _PEB
{
    BOOLEAN                      InheritedAddressSpace;             /*  00 */
    BOOLEAN                      ReadImageFileExecOptions;          /*  01 */
    BOOLEAN                      BeingDebugged;                     /*  02 */
    BOOLEAN                      SpareBool;                         /*  03 */
    HANDLE                       Mutant;                            /*  04 */
    HMODULE                      ImageBaseAddress;                  /*  08 */
    PPEB_LDR_DATA                LdrData;                           /*  0c */
    RTL_USER_PROCESS_PARAMETERS *ProcessParameters;                 /*  10 */
    PVOID                        SubSystemData;                     /*  14 */
    HANDLE                       ProcessHeap;                       /*  18 */
    PRTL_CRITICAL_SECTION        FastPebLock;                       /*  1c */
    PVOID /*PPEBLOCKROUTINE*/    FastPebLockRoutine;                /*  20 */
    PVOID /*PPEBLOCKROUTINE*/    FastPebUnlockRoutine;              /*  24 */
    ULONG                        EnvironmentUpdateCount;            /*  28 */
    PVOID                        KernelCallbackTable;               /*  2c */
    PVOID                        EventLogSection;                   /*  30 */
    PVOID                        EventLog;                          /*  34 */
    PVOID /*PPEB_FREE_BLOCK*/    FreeList;                          /*  38 */
    ULONG                        TlsExpansionCounter;               /*  3c */
    PRTL_BITMAP                  TlsBitmap;                         /*  40 */
    ULONG                        TlsBitmapBits[2];                  /*  44 */
    PVOID                        ReadOnlySharedMemoryBase;          /*  4c */
    PVOID                        ReadOnlySharedMemoryHeap;          /*  50 */
    PVOID                       *ReadOnlyStaticServerData;          /*  54 */
    PVOID                        AnsiCodePageData;                  /*  58 */
    PVOID                        OemCodePageData;                   /*  5c */
    PVOID                        UnicodeCaseTableData;              /*  60 */
    ULONG                        NumberOfProcessors;                /*  64 */
    ULONG                        NtGlobalFlag;                      /*  68 */
    BYTE                         Spare2[4];                         /*  6c */
    LARGE_INTEGER                CriticalSectionTimeout;            /*  70 */
    ULONG                        HeapSegmentReserve;                /*  78 */
    ULONG                        HeapSegmentCommit;                 /*  7c */
    ULONG                        HeapDeCommitTotalFreeThreshold;    /*  80 */
    ULONG                        HeapDeCommitFreeBlockThreshold;    /*  84 */
    ULONG                        NumberOfHeaps;                     /*  88 */
    ULONG                        MaximumNumberOfHeaps;              /*  8c */
    PVOID                       *ProcessHeaps;                      /*  90 */
    PVOID                        GdiSharedHandleTable;              /*  94 */
    PVOID                        ProcessStarterHelper;              /*  98 */
    PVOID                        GdiDCAttributeList;                /*  9c */
    PVOID                        LoaderLock;                        /*  a0 */
    ULONG                        OSMajorVersion;                    /*  a4 */
    ULONG                        OSMinorVersion;                    /*  a8 */
    ULONG                        OSBuildNumber;                     /*  ac */
    ULONG                        OSPlatformId;                      /*  b0 */
    ULONG                        ImageSubSystem;                    /*  b4 */
    ULONG                        ImageSubSystemMajorVersion;        /*  b8 */
    ULONG                        ImageSubSystemMinorVersion;        /*  bc */
    ULONG                        ImageProcessAffinityMask;          /*  c0 */
    ULONG                        GdiHandleBuffer[34];               /*  c4 */
    ULONG                        PostProcessInitRoutine;            /* 14c */
    PRTL_BITMAP                  TlsExpansionBitmap;                /* 150 */
    ULONG                        TlsExpansionBitmapBits[32];        /* 154 */
    ULONG                        SessionId;                         /* 1d4 */
} PEB, *PPEB;

/***********************************************************************
 * TEB data structure
 */
typedef struct _TEB
{
    NT_TIB          Tib;                        /* 000 */
    PVOID           EnvironmentPointer;         /* 01c */
    CLIENT_ID       ClientId;                   /* 020 */
    PVOID           ActiveRpcHandle;            /* 028 */
    PVOID           ThreadLocalStoragePointer;  /* 02c */
    PPEB            Peb;                        /* 030 */
    ULONG           LastErrorValue;             /* 034 */
    ULONG           CountOfOwnedCriticalSections;/* 038 */
    PVOID           CsrClientThread;            /* 03c */
    PVOID           Win32ThreadInfo;            /* 040 */
    ULONG           Win32ClientInfo[31];        /* 044 used for user32 private data in Wine */
    PVOID           WOW32Reserved;              /* 0c0 */
    ULONG           CurrentLocale;              /* 0c4 */
    ULONG           FpSoftwareStatusRegister;   /* 0c8 */
    PVOID           SystemReserved1[54];        /* 0cc used for kernel32 private data in Wine */
    PVOID           Spare1;                     /* 1a4 */
    LONG            ExceptionCode;              /* 1a8 */
    BYTE            SpareBytes1[40];            /* 1ac */
    PVOID           SystemReserved2[10];        /* 1d4 used for ntdll private data in Wine */
    GDI_TEB_BATCH   GdiTebBatch;                /* 1fc */
    ULONG           gdiRgn;                     /* 6dc */
    ULONG           gdiPen;                     /* 6e0 */
    ULONG           gdiBrush;                   /* 6e4 */
    CLIENT_ID       RealClientId;               /* 6e8 */
    HANDLE          GdiCachedProcessHandle;     /* 6f0 */
    ULONG           GdiClientPID;               /* 6f4 */
    ULONG           GdiClientTID;               /* 6f8 */
    PVOID           GdiThreadLocaleInfo;        /* 6fc */
    PVOID           UserReserved[5];            /* 700 */
    PVOID           glDispachTable[280];        /* 714 */
    ULONG           glReserved1[26];            /* b74 */
    PVOID           glReserved2;                /* bdc */
    PVOID           glSectionInfo;              /* be0 */
    PVOID           glSection;                  /* be4 */
    PVOID           glTable;                    /* be8 */
    PVOID           glCurrentRC;                /* bec */
    PVOID           glContext;                  /* bf0 */
    ULONG           LastStatusValue;            /* bf4 */
    UNICODE_STRING  StaticUnicodeString;        /* bf8 used by advapi32 */
    WCHAR           StaticUnicodeBuffer[261];   /* c00 used by advapi32 */
    PVOID           DeallocationStack;          /* e0c */
    PVOID           TlsSlots[64];               /* e10 */
    LIST_ENTRY      TlsLinks;                   /* f10 */
    PVOID           Vdm;                        /* f18 */
    PVOID           ReservedForNtRpc;           /* f1c */
    PVOID           DbgSsReserved[2];           /* f20 */
    ULONG           HardErrorDisabled;          /* f28 */
    PVOID           Instrumentation[16];        /* f2c */
    PVOID           WinSockData;                /* f6c */
    ULONG           GdiBatchCount;              /* f70 */
    ULONG           Spare2;                     /* f74 */
    ULONG           Spare3;                     /* f78 */
    ULONG           Spare4;                     /* f7c */
    PVOID           ReservedForOle;             /* f80 */
    ULONG           WaitingOnLoaderLock;        /* f84 */
    PVOID           Reserved5[3];               /* f88 */
    PVOID          *TlsExpansionSlots;          /* f94 */
} TEB, *PTEB;

#define NtCurrentProcess() ((HANDLE)-1)
/*
 * NtCurrentTeb() is imported from ntdll.dll if we use ms c compiler.
 * Otherwise (on mingw) this is inline function, defined in one of the
 * mingw headers.
 */
//#if defined(__GNUC__)
//struct _TEB *NtCurrentTeb(void);
//#else
//struct _TEB *NtCurrentTeb(void);
//#endif

#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE  0x3
#define SE_AUDIT_PRIVILEGE               0x15
#define SE_BACKUP_PRIVILEGE              0x11
#define SE_CREATE_PAGEFILE_PRIVILEGE     0x0f
#define SE_CREATE_PERMANENT_PRIVILEGE    0x10
#define SE_CREATE_TOKEN_PRIVILEGE        0x2
#define SE_DEBUG_PRIVILEGE               0x14
#define SE_IMPERSONATE_PRIVILEGE
#define SE_INC_BASE_PRIORITY_PRIVILEGE   0x0e
#define SE_INCREASE_QUOTA_PRIVILEGE      0x5
#define SE_LOAD_DRIVER_PRIVILEGE         0x0a
#define SE_LOCK_MEMORY_PRIVILEGE         0x4
#define SE_MANAGE_VOLUME_PRIVILEGE       0x1c
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE 0x0d
#define SE_RESTORE_PRIVILEGE             0x12
#define SE_SECURITY_PRIVILEGE            0x8
#define SE_SHUTDOWN_PRIVILEGE            0x13
#define SE_SYSTEM_PROFILE_PRIVILEGE      0x0b
#define SE_SYSTEMTIME_PRIVILEGE          0x0c
#define SE_TAKE_OWNERSHIP_PRIVILEGE      0x9
#define SE_TCB_PRIVILEGE                 0x7

typedef enum _SHUTDOWN_ACTION
{
    ShutdownNoReboot,
    ShutdownReboot,
    ShutdownPowerOff
} SHUTDOWN_ACTION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataLength;
    UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef enum _KEY_VALUE_INFORMATION_CLASS
{
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;

typedef enum _EVENT_TYPE {
  NotificationEvent,
  SynchronizationEvent
} EVENT_TYPE, *PEVENT_TYPE;

#define FILE_DIRECTORY_FILE		0x00000001
#define FILE_RESERVE_OPFILTER	0x00100000

/* DriveMap member must be declared as unsigned int and alignment must be 1 
 * because without that it don't works on Windows XP x64 !!!
 */
#pragma pack(push,1)
typedef struct _PROCESS_DEVICEMAP_INFORMATION
{
    union
    {
        struct
        {
            HANDLE DirectoryHandle;
        } Set;
        struct
        {
            //ULONG DriveMap;
            UINT DriveMap;
            UCHAR DriveType[32];
        } Query;
    };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;
#pragma pack(pop)

typedef struct _FILE_FS_SIZE_INFORMATION {
    LARGE_INTEGER   TotalAllocationUnits;
    LARGE_INTEGER   AvailableAllocationUnits;
    ULONG           SectorsPerAllocationUnit;
    ULONG           BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
    ULONG   FileSystemAttributes;
    ULONG   MaximumComponentNameLength;
    ULONG   FileSystemNameLength;
    WCHAR   FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

typedef struct _KBD_RECORD {
	WORD	wVirtualScanCode;
	DWORD	dwControlKeyState;
	UCHAR	AsciiChar;
	BOOL	bKeyDown;
} KBD_RECORD, *PKBD_RECORD;

typedef LPOSVERSIONINFOW PRTL_OSVERSIONINFOW;

/* keyboard related structures */
/* KEYBOARD_INPUT_DATA.Flags constants */
#define KEY_MAKE                          0
#define KEY_BREAK                         1
#define KEY_E0                            2
#define KEY_E1                            4

typedef struct _KEYBOARD_INPUT_DATA {
  USHORT  UnitId;
  USHORT  MakeCode;
  USHORT  Flags;
  USHORT  Reserved;
  ULONG  ExtraInformation;
} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;

/* native functions prototypes */
NTSTATUS	NTAPI	NtCreateFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG);
NTSTATUS	NTAPI	NtReadFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,PLARGE_INTEGER,PULONG);
NTSTATUS	NTAPI	NtWriteFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,PLARGE_INTEGER,PULONG);
NTSTATUS	NTAPI	NtClose(HANDLE);
VOID		NTAPI	RtlInitUnicodeString(PUNICODE_STRING,PCWSTR);
NTSTATUS	NTAPI	NtWaitForSingleObject(HANDLE,BOOLEAN,const LARGE_INTEGER*);
NTSTATUS	NTAPI	NtDisplayString(PUNICODE_STRING); 
NTSTATUS	NTAPI	NtInitializeRegistry(BOOLEAN);
NTSTATUS	NTAPI	ZwQuerySystemInformation(IN SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS	NTAPI	NtQueryVolumeInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FS_INFORMATION_CLASS);
NTSTATUS	NTAPI	NtCreateEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *,BOOLEAN,BOOLEAN);
NTSTATUS	NTAPI	NtDeviceIoControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,PVOID,ULONG,PVOID,ULONG);
NTSTATUS	NTAPI	NtOpenEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *);
NTSTATUS	NTAPI	NtQueryInformationProcess(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);
NTSTATUS	NTAPI	NtSetInformationProcess(HANDLE,PROCESS_INFORMATION_CLASS,PVOID,ULONG);
NTSTATUS	NTAPI	NtOpenSymbolicLinkObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS	NTAPI	NtQuerySymbolicLinkObject(HANDLE,PUNICODE_STRING,PULONG);
NTSTATUS	NTAPI	NtCancelIoFile(HANDLE,PIO_STATUS_BLOCK);
PRTL_USER_PROCESS_PARAMETERS NTAPI RtlNormalizeProcessParams(RTL_USER_PROCESS_PARAMETERS*);
VOID		NTAPI	DbgBreakPoint(VOID);
NTSTATUS	NTAPI	NtTerminateProcess(HANDLE,LONG);
NTSTATUS	NTAPI	NtOpenProcessToken(HANDLE,ACCESS_MASK,PHANDLE);
NTSTATUS	NTAPI	NtAdjustPrivilegesToken(HANDLE,BOOLEAN,PTOKEN_PRIVILEGES,DWORD,PTOKEN_PRIVILEGES,PDWORD);
NTSTATUS	NTAPI	NtDelayExecution(BOOLEAN,const LARGE_INTEGER*);
NTSTATUS	NTAPI	NtShutdownSystem(SHUTDOWN_ACTION);
NTSTATUS	NTAPI	NtLoadDriver(PUNICODE_STRING);
NTSTATUS	NTAPI	NtUnloadDriver(PUNICODE_STRING);
NTSTATUS	NTAPI	NtOpenKey(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS	NTAPI	NtCreateKey(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,ULONG,PUNICODE_STRING,ULONG,PULONG);
NTSTATUS	NTAPI	NtQueryValueKey(HANDLE,PUNICODE_STRING,KEY_VALUE_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS	NTAPI	NtSetValueKey(HANDLE,PUNICODE_STRING,ULONG,ULONG,PVOID,ULONG);
NTSTATUS	NTAPI	NtClose(HANDLE);
NTSTATUS	NTAPI	RtlCreateUserThread(HANDLE,PSECURITY_DESCRIPTOR,BOOLEAN,LONG,ULONG,ULONG,PTHREAD_START_ROUTINE,PVOID,PHANDLE,PCLIENT_ID);
VOID		NTAPI	RtlExitUserThread(NTSTATUS);
VOID		NTAPI	RtlInitAnsiString(PANSI_STRING,PCSZ);
NTSTATUS	NTAPI	RtlAnsiStringToUnicodeString(PUNICODE_STRING,PANSI_STRING,BOOL);
NTSTATUS	NTAPI	RtlUnicodeStringToAnsiString(PANSI_STRING,PUNICODE_STRING,BOOL);
VOID		NTAPI	RtlFreeUnicodeString(PUNICODE_STRING);
ULONG		NTAPI	RtlNtStatusToDosError(NTSTATUS);
NTSTATUS	NTAPI	LdrGetDllHandle(ULONG,ULONG,const UNICODE_STRING*,HMODULE*);
NTSTATUS	NTAPI	LdrGetProcedureAddress(PVOID,PANSI_STRING,ULONG,PVOID *);
NTSTATUS	NTAPI	NtAllocateVirtualMemory(HANDLE,PVOID*,ULONG,PULONG,ULONG,ULONG);
NTSTATUS	NTAPI	NtFreeVirtualMemory(HANDLE,PVOID*,PULONG,ULONG);

#endif /* _NTNDK_H_ */
