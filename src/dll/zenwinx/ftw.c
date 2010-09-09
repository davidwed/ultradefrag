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
 * @file ftw.c
 * @brief File tree walk routines.
 * @addtogroup File
 * @{
 */

#include "ntndk.h"
#include "zenwinx.h"

/**
 * @brief Returns list of files contained
 * in directory, and all its subdirectories
 * if WINX_FTW_RECURSIVE flag is passed.
 * @param[in] path the native path of the
 * directory to be scanned.
 * @param[in] flags combination
 * of WINX_FTW_xxx flags, defined in zenwinx.h
 * @param[in] cb address of callback routine
 * to be called for each file; if it returns
 * nonzero value, the current file and all
 * its children should be skipped. If NULL pointer
 * is passed, no files will be skipped.
 * @return List of files, NULL indicates failure.
 * @note Optimized for little directories scanning.
 */
winx_file_info * __stdcall winx_ftw(short *path,int flags,ftw_callback cb)
{
	return NULL;
}

/**
 * @brief Returns list of files,
 * contained on the disk.
 * @param[in] volume_letter the volume letter.
 * @param[in] flags combination
 * of WINX_FTW_xxx flags, defined in zenwinx.h
 * @param[in] cb address of callback routine
 * to be called for each file; if it returns
 * nonzero value, the current file and all
 * its children should be skipped. If NULL pointer
 * is passed, no files will be skipped.
 * @return List of files, NULL indicates failure.
 * @note Optimized for entire disk scanning.
 */
winx_file_info * __stdcall winx_scan_disk(char volume_letter,int flags,ftw_callback cb)
{
	return NULL;
}

/** @} */
