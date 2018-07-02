/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2018 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @brief Disk validation.
 * @addtogroup Disks
 * @{
 */

#include "udefrag-internals.h"

/******************************************************************************/
/*                            Auxiliary functions                             */
/******************************************************************************/

/**
 * @internal
 * @brief Finds out whether a disk can be defragmented or not. 
 * If so, retrieves the complete information about it as well.
 * @param[in] volume_letter the volume letter.
 * @param[in] skip_removable defines whether
 * to skip removable media or not.
 * @param[out] v pointer to structure receiving the information.
 * @return Zero for success, a negative value otherwise.
 * @note If skip_removable is equal to zero and you want to
 * validate a floppy drive with no floppy disk inserted then
 * you'll hear noise :)
 */
static int internal_validate_volume(char volume_letter,int skip_removable,volume_info *v)
{
    winx_volume_information volume_info;
    int type;
    
    if(v == NULL)
        return (-1);

    /* convert volume letter to uppercase */
    volume_letter = winx_toupper(volume_letter);
    
    v->letter = volume_letter;
    v->is_removable = FALSE;
    v->is_dirty = FALSE;
    type = winx_get_drive_type(volume_letter);
    if(type < 0) return (-1);
    if(type == DRIVE_CDROM){
        itrace("Disk %c: is on cdrom drive.",volume_letter);
        return UDEFRAG_CDROM;
    }
    if(type == DRIVE_REMOTE){
        itrace("Disk %c: is on remote drive.",volume_letter);
        return UDEFRAG_REMOTE;
    }
    if(type == DRIVE_ASSIGNED_BY_SUBST_COMMAND){
        itrace("It seems that %c: drive letter is assigned by \'subst\' command.",volume_letter);
        return UDEFRAG_ASSIGNED_BY_SUBST;
    }
    if(type == DRIVE_REMOVABLE){
        v->is_removable = TRUE;
        if(skip_removable){
            itrace("Disk %c: is on removable media.",volume_letter);
            return UDEFRAG_REMOVABLE;
        }
    }

    /*
    * Get volume information; it is strongly 
    * required to exclude missing floppies.
    */
    if(winx_get_volume_information(volume_letter,&volume_info) < 0)
        return (-1);
    
    v->total_space.QuadPart = volume_info.total_bytes;
    v->free_space.QuadPart = volume_info.free_bytes;
    strncpy(v->fsname,volume_info.fs_name,MAXFSNAME - 1);
    v->fsname[MAXFSNAME - 1] = 0;
    wcsncpy(v->label,volume_info.label,MAX_PATH);
    v->label[MAX_PATH] = 0;
    v->is_dirty = volume_info.is_dirty;
    return 0;
}

/******************************************************************************/
/*                            Interface functions                             */
/******************************************************************************/

/**
 * @brief Enumerates disk volumes
 * available for defragmentation.
 * @param[in] skip_removable defines
 * whether to skip removable media or not.
 * @return The list of volumes.
 * NULL indicates failure.
 * @note If skip_removable is equal to zero
 * and you have a floppy drive with no disk
 * inserted, then you'll hear noise :)
 * @par Example:
 * @code
 * volume_info *v;
 * int i;
 *
 * v = udefrag_get_vollist(TRUE);
 * if(v != NULL){
 *     for(i = 0; v[i].letter != 0; i++){
 *         // process the volume
 *     }
 *     udefrag_release_vollist(v);
 * }
 * @endcode
 */
volume_info *udefrag_get_vollist(int skip_removable)
{
    volume_info *v;
    ULONG i, index;
    char letter;
    
    /* allocate memory */
    v = winx_malloc((MAX_DOS_DRIVES + 1) * sizeof(volume_info));

    /* set error mode to ignore missing removable drives */
    if(winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS) < 0){
        winx_free(v);
        return NULL;
    }

    /* cycle through drive letters */
    for(i = 0, index = 0; i < MAX_DOS_DRIVES; i++){
        letter = 'A' + (char)i;
        if(internal_validate_volume(letter,skip_removable,v + index) >= 0)
            index ++;
    }
    v[index].letter = 0;

    /* try to restore error mode to default state */
    winx_set_system_error_mode(1); /* equal to SetErrorMode(0) */
    return v;
}

/**
 * @brief Releases list of volumes
 * returned by udefrag_get_vollist.
 */
void udefrag_release_vollist(volume_info *v)
{
    winx_free(v);
}

/**
 * @brief Retrieves the complete information about a disk volume.
 * @param[in] volume_letter the volume letter.
 * @param[in] v pointer to structure receiving the information.
 * @return Zero for success, a negative value otherwise.
 */
int udefrag_get_volume_information(char volume_letter,volume_info *v)
{
    int result;
    
    /* set error mode to ignore missing removable drives */
    if(winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS) < 0)
        return (-1);

    result = internal_validate_volume(volume_letter,0,v);

    /* try to restore error mode to default state */
    winx_set_system_error_mode(1); /* equal to SetErrorMode(0) */
    return result;
}

/**
 * @brief Checks whether defragmentation is
 * possible for the specified disk volume or not.
 * @param[in] volume_letter the volume letter.
 * @param[in] skip_removable defines whether
 * to skip removable media or not.
 * @return Zero for success, a negative value otherwise.
 * @note If skip_removable is equal to zero and you want 
 * to validate a floppy drive with no floppy disk inserted
 * then you'll hear noise :)
 */
int udefrag_validate_volume(char volume_letter,int skip_removable)
{
    volume_info v;
    int result;

    /* set error mode to ignore missing removable drives */
    if(winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS) < 0)
        return (-1);
    result = internal_validate_volume(volume_letter,skip_removable,&v);
    /* try to restore error mode to default state */
    winx_set_system_error_mode(1); /* equal to SetErrorMode(0) */
    return result;
}

/** @} */
