/*
 *  ZenWINX - WIndows Native eXtended library.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

/**
 * @file ftw_ntfs.c
 * @brief Fast file tree walk for NTFS.
 * @addtogroup File
 * @{
 */

#include "ntndk.h"
#include "ntfs.h"
#include "zenwinx.h"

/**
 * @brief winx_scan_disk analog, but
 * optimized for fastest NTFS scan.
 */
winx_file_info * __stdcall ntfs_scan_disk(char volume_letter,
	int flags,ftw_callback cb,ftw_terminator t)
{
	return NULL;
}

/** @} */
