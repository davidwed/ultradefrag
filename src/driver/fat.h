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

#ifndef _FAT_H_
#define _FAT_H_

/* first of all - BIOS parameter block */
#pragma pack(push, 1)
typedef struct BPB {
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
          } Fat12;
        struct {
          UCHAR  BS_DrvNum;          // 36
          UCHAR  BS_Reserved1;       // 37
          UCHAR  BS_BootSig;         // 38
          ULONG  BS_VolID;           // 39
          UCHAR  BS_VolLab[11];      // 43
          UCHAR  BS_FilSysType[8];   // 54
          UCHAR  BS_Reserved2[448];  // 62
          } Fat16;
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
#pragma pack(pop)

/* internal function prototypes */
NTSTATUS ReadSector(UDEFRAG_DEVICE_EXTENSION *dx,ULONGLONG lsn,PVOID buffer,ULONG length);

#endif /* _FAT_H_ */
