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
* FAT related definitions and structures.
*/

/*
* Code based on:
* Microsoft Extensible Firmware Initiative
* FAT32 File System Specification
*/

#ifndef _FAT_H_
#define _FAT_H_

/* file attribute flags */
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN 	0x02
#define ATTR_SYSTEM 	0x04
#define ATTR_VOLUME_ID 	0x08
#define ATTR_DIRECTORY	0x10
#define ATTR_ARCHIVE  	0x20
#define ATTR_LONG_NAME 	(ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

#define FIRST_LONG_ENTRY      0x01
#define LAST_LONG_ENTRY_FLAG  0x40 /* the last long entry has Order == (N | 0x40) */

#define MAX_LONG_PATH (260 + 4) /* incl. trailing null and \??\ sequence */

#define FAT12_EOC 0x0FF8
#define FAT16_EOC 0xFFF8
#define FAT32_EOC 0x0FFFFFF8

/* first of all - BIOS parameter block */
#pragma pack(push, 1)
typedef struct _BPB {
    UCHAR    Jump[3];            //  0 jmp instruction
    UCHAR    OemName[8];         //  3 OEM name and version    // This portion is the BPB (BIOS Parameter Block)
    USHORT   BytesPerSec;        // 11 bytes per sector
    UCHAR    SecPerCluster;      // 13 sectors per cluster
    USHORT   ReservedSectors;    // 14 number of reserved sectors
    UCHAR    NumFATs;            // 16 number of file allocation tables
    USHORT   RootDirEnts;        // 17 number of root-directory entries
    USHORT   FAT16totalsectors;  // 19 if FAT16 total number of sectors
    UCHAR    Media;              // 21 media descriptor
    USHORT   FAT16sectors;       // 22 if FAT16 number of sectors per FATable, else 0
    USHORT   SecPerTrack;        // 24 number of sectors per track
    USHORT   NumHeads;           // 26 number of read/write heads
    ULONG    HiddenSectors;      // 28 number of hidden sectors
    ULONG    FAT32totalsectors;  // 32 if FAT32 number of sectors if Sectors==0    // End of BPB
    union {
        struct {
          UCHAR  BS_DrvNum;          // 36
          UCHAR  BS_Reserved1;       // 37
          UCHAR  BS_BootSig;         // 38
          ULONG  BS_VolID;           // 39
          UCHAR  BS_VolLab[11];      // 43
          UCHAR  BS_FilSysType[8];   // 54
          UCHAR  BS_Reserved2[448];  // 62
          } Fat1x;
        struct {
          ULONG  FAT32sectors;       // 36
          USHORT BPB_ExtFlags;       // 40
          USHORT BPB_FSVer;          // 42
          ULONG  BPB_RootClus;       // 44
          USHORT BPB_FSInfo;         // 48
          USHORT BPB_BkBootSec;      // 50
          UCHAR  BPB_Reserved[12];   // 52
          UCHAR  BS_DrvNum;          // 64
          UCHAR  BS_Reserved1;       // 65
          UCHAR  BS_BootSig;         // 66
          ULONG  BS_VolID;           // 67
          UCHAR  BS_VolLab[11];      // 71
          UCHAR  BS_FilSysType[8];   // 82
          UCHAR  BPB_Reserved2[420]; // 90
          } Fat32;
    };
    USHORT Signature;              // 510
} BPB;

typedef struct _DIRENTRY {
	UCHAR ShortName[11];   /* short name of the file */
	UCHAR Attr;            /* file attributes */
	UCHAR NtReserved;
	UCHAR CrtTimeTenth;
	USHORT CrtTime;
	USHORT CrtDate;
	USHORT LstAccDate;
	USHORT FirstClusterHI; /* high word of the entry's first cluster number,
							  always zero for a FAT12 and FAT16 volumes */
	USHORT WrtTime;
	USHORT WrtDate;
	USHORT FirstClusterLO; /* low word of the entry's first cluster number */
	ULONG FileSize;        /* file size, in bytes */
} DIRENTRY, *PDIRENTRY;

typedef struct _LONGDIRENTRY {
	UCHAR Order;     /* Entry's order in the sequence of long dir entries,
	                    associated with the short dir entry at the end of the long dir set. */
					 /* If masked with 0x40 (LAST_LONG_ENTRY), this indicates the entry 
	                    is the last long dir entry in a set of long dir entries. All valid sets 
	                    of long dir entries must begin with an entry having this mask. */
	                 /* Cannot have 0x00 or 0xE5 value. */
	WCHAR Name1[5];  /* Characters 1-5 of the long-name sub-component in this dir entry. */
	UCHAR Attr;      /* must be ATTR_LONG_NAME */
	UCHAR Type;      /* Zero indicates - dir entry is a sub-component of a long name. 
	                    Other values reserved for future extensions. */
	UCHAR Chksum;    /* Checksum of name in the short dir entry at the end of the long dir set. */
	WCHAR Name2[6]; /* Characters 6-11 of long name sub-component. */
	USHORT FirstClusterLO; /* This artifact must be zero. */
	WCHAR Name3[2];  /* Characters 12-13 of long name sub-component. */
} LONGDIRENTRY, *PLONGDIRENTRY;

#pragma pack(pop)

/* global variables */
extern BPB Bpb;
extern ULONGLONG FirstDataSector;

/* internal function prototypes */
unsigned char ChkSum (unsigned char *pFcbName);

#endif /* _FAT_H_ */
