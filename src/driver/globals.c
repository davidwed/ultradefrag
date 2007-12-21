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
 *  Global variables.
 */

#include "driver.h"

ULONG dbg_level;
/* Bit shifting array for efficient processing of the bitmap */
UCHAR BitShift[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

short device_name[] = L"\\Device\\UltraDefrag";
short link_name[] = L"\\DosDevices\\ultradfg";

#ifdef NT4_DBG
HANDLE hDbgLog = 0;
LARGE_INTEGER dbg_log_offset;
UCHAR *dbg_ring_buffer;
unsigned int dbg_ring_buffer_offset;
#endif

/*
 * Buffer to store the number of clusters of each kind.
 * More details at http://www.thescripts.com/forum/thread617704.html
 * ('Dynamically-allocated Multi-dimensional Arrays - C').
 */
ULONGLONG (*new_cluster_map)[NUM_OF_SPACE_STATES];
ULONG map_size;
