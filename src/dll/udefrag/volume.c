/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
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
 * @file volume.c
 * @brief Disk validation code.
 * @addtogroup Disks
 * @{
 */

#include "../../include/ntndk.h"

#include "../../include/udefrag.h"
#include "../zenwinx/zenwinx.h"

volume_info global_v[MAX_DOS_DRIVES + 1];

static int internal_validate_volume(unsigned char letter,int skip_removable,volume_info *v);

/**
 * @brief Retrieves a list of volumes
 * available for defragmentation.
 * @param[in] vol_info pointer to variable 
 * receiving the volume list array address.
 * @param[in] skip_removable the boolean value defining,
 * must removable drives be skipped or not.
 * @return Zero for success, negative value otherwise.
 * @note if skip_removable is equal to FALSE and you have a
 * floppy drive without a floppy disk then you will hear noise :))
 * @par Example:
 * @code
 * volume_info *v;
 * int i;
 *
 * if(udefrag_get_avail_volumes(&v,TRUE) >= 0){
 *     for(i = 0;;i++){
 *         if(!v[i].letter) break;
 *         // ...
 *     }
 * }
 * @endcode
 */
int __stdcall udefrag_get_avail_volumes(volume_info **vol_info,int skip_removable)
{
	ULONG i, index;
	char letter;

	/* get full list of volumes */
	*vol_info = global_v;
	/* set error mode to ignore missing removable drives */
	if(winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS) < 0)
		return (-1);
	index = 0;
	for(i = 0; i < MAX_DOS_DRIVES; i++){
		letter = 'A' + (char)i;
		if(internal_validate_volume(letter, skip_removable,&(global_v[index])) >= 0)
			index ++;
	}
	global_v[index].letter = 0;
	/* try to restore error mode to default state */
	winx_set_system_error_mode(1); /* equal to SetErrorMode(0) */
	return 0;
}

/**
 * @brief Checks a volume for the defragmentation possibility.
 * @param[in] letter the volume letter.
 * @param[in] skip_removable the boolean value 
 * defining, must removable drives be skipped or not.
 * @return Zero for success, negative value otherwise.
 * @note if skip_removable is equal to FALSE and you want 
 * to validate a floppy drive without a floppy disk
 * then you will hear noise :))
 */
int __stdcall udefrag_validate_volume(unsigned char letter,int skip_removable)
{
	volume_info v;
	int error_code;

	/* set error mode to ignore missing removable drives */
	if(winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS) < 0)
		return (-1);
	error_code = internal_validate_volume(letter,skip_removable,&v);
	if(error_code < 0) return error_code;
	/* try to restore error mode to default state */
	winx_set_system_error_mode(1); /* equal to SetErrorMode(0) */
	return 0;
}

/**
 * @brief Retrieves a volume parameters.
 * @param[in] letter the volume letter.
 * @param[in] skip_removable the boolean value defining,
 * must removable drives be treated as invalid or not.
 * @param[out] v pointer to structure receiving volume
 * parameters.
 * @return Zero for success, negative value otherwise.
 * @note
 * - Internal use only.
 * - if skip_removable is equal to FALSE and you want 
 *   to validate a floppy drive without a floppy disk
 *   then you will hear noise :))
 */
static int internal_validate_volume(unsigned char letter,int skip_removable,volume_info *v)
{
	winx_volume_information volume_info;
	int type;
	
	if(v == NULL)
		return (-1);

	v->letter = letter;
	v->is_removable = FALSE;
	type = winx_get_drive_type(letter);
	if(type < 0) return (-1);
	if(type == DRIVE_CDROM){
		DebugPrint("Volume %c: is on cdrom drive.",letter);
		return UDEFRAG_CDROM;
	}
	if(type == DRIVE_REMOTE){
		DebugPrint("Volume %c: is on remote drive.",letter);
		return UDEFRAG_REMOTE;
	}
	if(type == DRIVE_ASSIGNED_BY_SUBST_COMMAND){
		DebugPrint("It seems that %c: volume letter is assigned by \'subst\' command.",letter);
		return UDEFRAG_ASSIGNED_BY_SUBST;
	}
	if(type == DRIVE_REMOVABLE){
		v->is_removable = TRUE;
		if(skip_removable){
			DebugPrint("Volume %c: is on removable media.",letter);
			return UDEFRAG_REMOVABLE;
		}
	}

	/*
	* Get volume information; it is strongly 
	* required to exclude missing floppies.
	*/
	volume_info.volume_letter = letter;
	if(winx_get_volume_information(&volume_info) < 0)
		return (-1);
	
	v->total_space.QuadPart = volume_info.total_bytes;
	v->free_space.QuadPart = volume_info.free_bytes;
	strncpy(v->fsname,volume_info.fs_name,MAXFSNAME - 1);
	v->fsname[MAXFSNAME - 1] = 0;
	return 0;
}

/** @} */
