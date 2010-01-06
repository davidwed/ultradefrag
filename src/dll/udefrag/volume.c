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
#include "../../include/ultradfg.h"
#include "../zenwinx/zenwinx.h"

volume_info v[MAX_DOS_DRIVES + 1];

int internal_validate_volume(unsigned char letter,int skip_removable,
		int *is_removable,char *fsname,LARGE_INTEGER *ptotal,LARGE_INTEGER *pfree);

/**
 * @brief Retrieves a list of volumes
 *        available for defragmentation.
 * @param[in] vol_info pointer to variable receiving
 *                     the volume list array address.
 * @param[in] skip_removable the boolean value defining
 *                           must removable drives
 *                           be skipped or not.
 * @return Zero for success, negative value otherwise.
 * @note if(skip_removable == FALSE && you have a
 *       floppy drive without floppy disk)
 *       then you will hear noise :))
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
	*vol_info = v;
	/* set error mode to ignore missing removable drives */
	if(winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS) < 0)
		return (-1);
	index = 0;
	for(i = 0; i < MAX_DOS_DRIVES; i++){
		letter = 'A' + (char)i;
		if(internal_validate_volume(letter, skip_removable,
			 &(v[index].is_removable), v[index].fsname,
			 &(v[index].total_space), &(v[index].free_space)) < 0) continue;
		v[index].letter = letter;
		index ++;
	}
	v[index].letter = 0;
	/* try to restore error mode to default state */
	winx_set_system_error_mode(1); /* equal to SetErrorMode(0) */
	return 0;
}

/**
 * @brief Checks a volume for the defragmentation possibility.
 * @param[in] letter the volume letter.
 * @param[in] skip_removable the boolean value defining
 *                           must removable drives be skipped or not.
 * @return Zero for success, negative value otherwise.
 * @note if(skip_removable == FALSE && you want 
 *       to validate a floppy drive without floppy disk)
 *       then you will hear noise :))
 */
int __stdcall udefrag_validate_volume(unsigned char letter,int skip_removable)
{
	int is_removable;
	/*
	* The following parameters are required 
	* to exclude missing floppies.
	*/
	char fsname[MAXFSNAME];
	LARGE_INTEGER total, free;

	/* set error mode to ignore missing removable drives */
	if(winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS) < 0)
		return (-1);
	if(internal_validate_volume(letter,skip_removable,&is_removable,
		fsname,&total,&free) < 0) return (-1);
	/* try to restore error mode to default state */
	winx_set_system_error_mode(1); /* equal to SetErrorMode(0) */
	return 0;
}

/**
 * @brief Retrieves a volume parameters.
 * @param[in] letter the volume letter.
 * @param[in] skip_removable the boolean value defining
 *                           must removable drives be treated
 *                           as invalid or not.
 * @param[out] is_removable pointer to the variable receiving
 *                          boolean value defining is volume
 *                          removable or not.
 * @param[out] fsname pointer to the buffer receiving
 *                    the name of the filesystem
 *                    containing on the volume.
 * @param[out] ptotal pointer to a variable receiving
 *                    a size of the volume.
 * @param[out] pfree pointer to a variable receiving
 *                   the amount of free space.
 * @return Zero for success, negative value otherwise.
 * @note
 * - Internal use only.
 * - if(skip_removable == FALSE && you want 
 *   to validate a floppy drive without floppy disk)
 *   then you will hear noise :))
 */
int internal_validate_volume(unsigned char letter,int skip_removable,
		int *is_removable,char *fsname,LARGE_INTEGER *ptotal,LARGE_INTEGER *pfree)
{
	int type;

	*is_removable = FALSE;
	type = winx_get_drive_type(letter);
	if(type < 0) return (-1);
	if(type == DRIVE_CDROM){
		DebugPrint("Volume %c: is on cdrom drive.",letter);
		return (-1);
	}
	if(type == DRIVE_REMOTE){
		DebugPrint("Volume %c: is on remote drive.",letter);
		return (-1);
	}
	if(type == DRIVE_ASSIGNED_BY_SUBST_COMMAND){
		DebugPrint("It seems that %c: volume letter is assigned by \'subst\' command.",letter);
		return (-1);
	}
	if(type == DRIVE_REMOVABLE){
		*is_removable = TRUE;
		if(skip_removable){
			DebugPrint("Volume %c: is on removable media.",letter);
			return (-1);
		}
	}
	/* get volume information */
	if(ptotal && pfree){
		if(winx_get_volume_size(letter,ptotal,pfree) < 0)
			return (-1);
	}
	if(fsname){
		if(winx_get_filesystem_name(letter,fsname,MAXFSNAME) < 0)
			return (-1);
	}
	return 0;
}

/** @} */
