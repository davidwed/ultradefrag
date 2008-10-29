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
* Volume validation routines.
*/

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> /* for toupper() function for mingw */
#include "../../include/ntndk.h"
#include "../../include/udefrag.h"
#include "../../include/ultradfg.h"
#include "../zenwinx/zenwinx.h"

volume_info v[MAX_DOS_DRIVES + 1];

int internal_validate_volume(unsigned char letter,int skip_removable,
							  int *is_removable,char *fsname,
							  LARGE_INTEGER *ptotal, LARGE_INTEGER *pfree);

/****f* udefrag.volume/udefrag_get_avail_volumes
* NAME
*    udefrag_get_avail_volumes
* SYNOPSIS
*    error = udefrag_get_avail_volumes(ppvol_info, skip_removable);
* FUNCTION
*    Retrieves the list of available volumes.
* INPUTS
*    ppvol_info     - pointer to variable of volume_info* type
*    skip_removable - true if we need to skip removable drives,
*                     false otherwise
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    volume_info *v;
*    char buffer[ERR_MSG_SIZE];
*    int i;
*
*    if(udefrag_get_avail_volumes(&v,TRUE) < 0){
*        udefrag_pop_error(buffer,sizeof(buffer));
*    } else {
*        for(i = 0;;i++){
*            if(!v[i].letter) break;
*            // ...
*        }
*    }
* NOTES
*    if(skip_removable == FALSE && you have 
*      floppy drive without floppy disk)
*       then you will hear noise :))
* SEE ALSO
*    udefrag_validate_volume, scheduler_get_avail_letters
******/
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
			 &(v[index].total_space), &(v[index].free_space)) < 0){
				winx_pop_error(NULL,0);
				continue;
		}
		v[index].letter = letter;
		index ++;
	}
	v[index].letter = 0;
	/* try to restore error mode to default state */
	if(winx_set_system_error_mode(1) < 0){ /* equal to SetErrorMode(0) */
		winx_pop_error(NULL,0);
	}
	return 0;
}

/****f* udefrag.volume/udefrag_validate_volume
* NAME
*    udefrag_validate_volume
* SYNOPSIS
*    error = udefrag_validate_volume(letter, skip_removable);
* FUNCTION
*    Checks specified volume to be valid for defragmentation.
* INPUTS
*    letter         - volume letter
*    skip_removable - true if we need to skip removable drives,
*                     false otherwise
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    if(udefrag_validate_volume("C",TRUE) < 0){
*        udefrag_pop_error(buffer,sizeof(buffer));
*        // handle error
*    }
* NOTES
*    if(skip_removable == FALSE && you want 
*      to validate floppy drive without floppy disk)
*       then you will hear noise :))
* SEE ALSO
*    udefrag_get_avail_volumes, scheduler_get_avail_letters
******/
int __stdcall udefrag_validate_volume(unsigned char letter,int skip_removable)
{
	int is_removable;

	/* set error mode to ignore missing removable drives */
	if(winx_set_system_error_mode(INTERNAL_SEM_FAILCRITICALERRORS) < 0)
		return (-1);
	if(internal_validate_volume(letter,skip_removable,&is_removable,
		NULL,NULL,NULL) < 0) return (-1);
	/* try to restore error mode to default state */
	if(winx_set_system_error_mode(1) < 0){ /* equal to SetErrorMode(0) */
		winx_pop_error(NULL,0);
	}
	return 0;
}

int internal_validate_volume(unsigned char letter,int skip_removable,
							  int *is_removable,char *fsname,
							  LARGE_INTEGER *ptotal, LARGE_INTEGER *pfree)
{
	int type;

	*is_removable = FALSE;
	type = winx_get_drive_type(letter);
	if(type < 0) return (-1);
	if(type == DRIVE_CDROM || type == DRIVE_REMOTE){
		winx_push_error("Volume must be on non-cdrom local drive, but it's %u!",type);
		return (-1);
	}
	if(type == DRIVE_ASSIGNED_BY_SUBST_COMMAND){
		winx_push_error("It seems that volume letter is assigned by \'subst\' command!");
		return (-1);
	}
	if(type == DRIVE_REMOVABLE){
		*is_removable = TRUE;
		if(skip_removable){
			winx_push_error("It's removable volume!");
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

/*
* This call is practically unuseful, especially on NT 4 system.
* Removed at 29 Oct 2008.
*/
/*BOOL get_drive_map(PROCESS_DEVICEMAP_INFORMATION *pProcessDeviceMapInfo);*/
