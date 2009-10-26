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
#include <winioctl.h>
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

extern ULONG disable_reports;
extern ULONG dbgprint_level;

extern BOOLEAN context_menu_handler;

extern HANDLE hSynchEvent;
extern HANDLE hStopEvent;

extern STATISTIC Stat;

/*************** Functions prototypes ******************/
void InitDriverResources(void);
void FreeDriverResources(void);

int AllocateMap(int size);
void FreeMap(void);
void GetMap(char *dest,int cluster_map_size);

void InitializeOptions(void);
void InitializeFilter(void);
void DestroyFilter(void);
BOOLEAN IsStringInFilter(short *str,PFILTER pf);
BOOLEAN CheckForContextMenuHandler(void);

LIST * NTAPI InsertItem(PLIST *phead,PLIST prev,ULONG size);
void NTAPI RemoveItem(PLIST *phead,PLIST item);
void NTAPI DestroyList(PLIST *phead);

int CheckForSynchObjects(void);

#endif /* _UDEFRAG_KERNEL_GLOBALS_H_ */
