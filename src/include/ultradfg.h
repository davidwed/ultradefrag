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
* Driver interface header.
*/

#ifndef _ULTRADFG_H_
#define _ULTRADFG_H_

/* this numbers MUST BE THE SAME */
#define FREE_SPACE                  0
#define SYSTEM_SPACE                1
#define SYSTEM_OVERLIMIT_SPACE      2
#define FRAGM_SPACE                 3
#define FRAGM_OVERLIMIT_SPACE       4
#define UNFRAGM_SPACE               5
#define UNFRAGM_OVERLIMIT_SPACE     6
#define MFT_SPACE                   7
#define DIR_SPACE                   8
#define DIR_OVERLIMIT_SPACE         9
#define COMPRESSED_SPACE            10
#define COMPRESSED_OVERLIMIT_SPACE  11
#define NO_CHECKED_SPACE            12

#define NUM_OF_SPACE_STATES         13

#define IOCTL_GET_STATISTIC CTL_CODE( \
	FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GET_CLUSTER_MAP CTL_CODE(\
	FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ULTRADEFRAG_STOP CTL_CODE(\
	FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SET_INCLUDE_FILTER CTL_CODE(\
	FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SET_EXCLUDE_FILTER CTL_CODE(\
	FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SET_DBGPRINT_LEVEL CTL_CODE(\
	FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SET_REPORT_TYPE CTL_CODE(\
	FILE_DEVICE_UNKNOWN, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* nt 4.0 specific */
#define IOCTL_SET_USER_MODE_BUFFER CTL_CODE(\
	FILE_DEVICE_UNKNOWN, 0x808, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_SET_CLUSTER_MAP_SIZE CTL_CODE(\
	FILE_DEVICE_UNKNOWN, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define STATUS_BEFORE_PROCESSING  0x0
#define STATUS_ANALYSED           0x1
#define STATUS_DEFRAGMENTED       0x2

#define HTML_REPORT   'H'
#define NO_REPORT     'N'

#define UTF_FORMAT    'U'
#define ASCII_FORMAT  'A'

#define DBG_NORMAL     0
#define DBG_DETAILED   1
#define DBG_PARANOID   2

typedef struct _REPORT_TYPE {
	UCHAR		type;
	UCHAR		format;
} REPORT_TYPE, *PREPORT_TYPE;

typedef struct _STATISTIC {
	ULONG		filecounter;
	ULONG		dircounter;
	ULONG		compressedcounter;
	ULONG		fragmfilecounter;
	ULONG		fragmcounter;
	ULONGLONG	total_space;
	ULONGLONG	free_space; /* in bytes */
	ULONG		mft_size;
	ULONGLONG	processed_clusters;
	ULONGLONG	bytes_per_cluster;
	UCHAR		current_operation;
	ULONGLONG	clusters_to_move_initial;
	ULONGLONG	clusters_to_move;
	ULONGLONG	clusters_to_compact_initial;
	ULONGLONG	clusters_to_compact;
} STATISTIC, *PSTATISTIC;

typedef struct _ULTRADFG_COMMAND {
	UCHAR		command;
	UCHAR		letter;
	ULONGLONG	sizelimit;
} ULTRADFG_COMMAND, *PULTRADFG_COMMAND;
#endif /* _ULTRADFG_H_ */
