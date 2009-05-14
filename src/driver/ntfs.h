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
* NTFS related definitions and structures.
*/

#ifndef _NTFS_H_
#define _NTFS_H_

/*
* Source: http://www.apnilife.com/E-Books_apnilife/Windows%20Programming_apnilife/
*         Windows%20NT%20Undocumented%20APIs/1996%20AppE_apnilife.pdf
*/

typedef struct {
	ULONGLONG FileReferenceNumber;
} NTFS_FILE_RECORD_INPUT_BUFFER, *PNTFS_FILE_RECORD_BUFFER;

typedef struct {
	ULONGLONG FileReferenceNumber;
	ULONG FileRecordLength;
	UCHAR FileRecordBuffer[1];
} NTFS_FILE_RECORD_OUTPUT_BUFFER, *PNTFS_FILE_RECORD_OUTPUT_BUFFER;

typedef struct {
	ULONG Type;       /* The type of NTFS record, possible values are 'FILE', 'INDX', 'BAAD', 'HOLE', CHKD'. */
	USHORT UsaOffset; /* The offset, in bytes, from the start of the structure to the Update Sequence Array. */
	USHORT UsaCount;  /* The number of values in the Update Sequence Array. */
	USN Usn;          /* The Update Sequence Number of the NTFS record. */
} NTFS_RECORD_HEADER, *PNTFS_RECORD_HEADER;

typedef struct {
	NTFS_RECORD_HEADER Ntfs;    /* It has 'FILE' type. */
	USHORT SequenceNumber;      /* The number of times that the MFT entry has been reused. */
	USHORT LinkCount;           /* The number of directory links to the MFT entry. */
	USHORT AttributesOffset;    /* The offset, in bytes, from the start of the structure to the first attribute of the MFT entry. */
	USHORT Flags;               /* A bit array of flags specifying properties of the MFT entry: 0x1 - InUse, 0x2 - Directory, ... */
	ULONG BytesInUse;           /* The number of bytes used by the MFT entry. */
	ULONG BytesAllocated;       /* The number of bytes allocated for the MFT entry. */
	ULONGLONG BaseFileRecord;   /* If the MFT entry contains attributes that overflowed a base MFT entry,
								 * this member contains the FRN of the base entry; otherwise it contains zero. */
	USHORT NextAttributeNumber; /* The number that will be assigned to the next attribute added to the MFT entry. */
} FILE_RECORD_HEADER, *PFILE_RECORD_HEADER;

/* MFT entry consists of FILE_RECORD_HEADER followed by a sequence of attributes. */

typedef enum {
	AttributeStandardInformation = 0x10,
	AttributeAttributeList = 0x20,
	AttributeFileName = 0x30,
	AttributeObjectId = 0x40,
	AttributeSecurityDescriptor = 0x50,
	AttributeVolumeName = 0x60,
	AttributeVolumeInformation = 0x70,
	AttributeData = 0x80,
	AttributeIndexRoot = 0x90,
	AttributeIndexAllocation = 0xA0,
	AttributeBitmap = 0xB0,
	AttributeReparsePoint = 0xC0,
	AttributeEAInformation = 0xD0,
	AttributeEA = 0xE0,
	AttributePropertySet = 0xF0,
	AttributeLoggedUtulityStream = 0x100
} ATTRIBUTE_TYPE, *PATTRIBUTE_TYPE;

typedef struct {
	ATTRIBUTE_TYPE AttributeType; /* The type of the attribute. */
	ULONG Length;                 /* The size, in bytes, of the resident part of the attribute. */
	BOOLEAN Nonresident;          /* Specifies, when true, that the attribute value is nonresident. */
	UCHAR NameLength;             /* The size, in characters, of the name (if any) of the attribute. */
	USHORT NameOffset;            /* The offset, in bytes, from the start of the structure to the attribute name 
								   * (stored as a Unicode string). */
	USHORT Flags;                 /* A bit array of flags specifying properties of the attribute. 0x1 - Compressed, ... */
	USHORT AttributeNumber;       /* A numeric identifier for the instance of the attribute. */
} ATTRIBUTE, *PATTRIBUTE;


#endif /* _NTFS_H_ */
