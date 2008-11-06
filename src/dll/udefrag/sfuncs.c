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
* udefrag.dll - middle layer between driver and user interfaces:
* simple interface for scripting languages.
*/

/*
* This interface is practically unuseful now,
* therefore it will be disabled by default.
* To enable them define PERL_SUPPORT and uncomment 
* appropriate entries in *.def files.
*/
#undef PERL_SUPPORT

#ifdef PERL_SUPPORT

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../include/ntndk.h"
#include "../../include/udefrag.h"
#include "../../include/ultradfg.h"
#include "../zenwinx/zenwinx.h"

char vlist[4096];
char map[32768];

/****f* udefrag.volume/scheduler_get_avail_letters
* NAME
*    scheduler_get_avail_letters
* SYNOPSIS
*    error = scheduler_get_avail_letters(letters);
* FUNCTION
*    Retrieves the string containing available letters.
* INPUTS
*    letters - buffer to store resulting string into
* RESULT
*    error - zero for success; negative value otherwise.
* EXAMPLE
*    char letters[32];
*    if(scheduler_get_avail_letters(letters) < 0){
*        udefrag_pop_error(buffer,sizeof(buffer));
*        // handle error
*    }
* NOTES
*    This function skips all removable drives.
* SEE ALSO
*    udefrag_get_avail_volumes, udefrag_validate_volume
******/
int __stdcall scheduler_get_avail_letters(char *letters)
{
	volume_info *v;
	int i;

	if(udefrag_get_avail_volumes(&v,TRUE)) return (-1);
	for(i = 0;;i++){
		letters[i] = v[i].letter;
		if(v[i].letter == 0) break;
	}
	return 0;
}

/****f* udefrag.scripting/udefrag_s_get_map
* NAME
*    udefrag_s_get_map
* SYNOPSIS
*    map = udefrag_s_get_map(size);
* FUNCTION
*    Retrieves the cluster map.
* INPUTS
*    size - size of cluster map.
* RESULT
*    map - string containing the cluster map.
*          Or "ERROR: ..." string if failure.
* EXAMPLE
*    my $map_buffer = udefrag_s_get_map($x * $y + 1);
*    $_ = $map_buffer;
*    if(m/ERROR/o){
*        # handle error
*    }else{
*        # draw map on the screen
*    }
* NOTES
*    Color codes aren't the same as udefrag_get_map()
*    returns.
******/
char * __stdcall udefrag_s_get_map(int size)
{
	char buffer[ERR_MSG_SIZE];
	int i;

	if(size > sizeof(map) - 1)
		return "ERROR: Map resolution is too high!";
	if(udefrag_get_map(map,size - 1) < 0){
		udefrag_pop_error(buffer,ERR_MSG_SIZE);
		sprintf(map,"ERROR: %s",buffer);
		return map;
	}
	/* remove zeroes from the map and terminate string */
	for(i = 0; i < size - 1; i++)
		map[i] ++;
	map[i] = 0;
	return map;
}

/****f* udefrag.scripting/udefrag_s_get_avail_volumes
* NAME
*    udefrag_s_get_avail_volumes
* SYNOPSIS
*    vlist = udefrag_s_get_avail_volumes(skip_removable);
* FUNCTION
*    Retrieves the list of available volumes.
* INPUTS
*    skip_removable - true if we need to skip removable drives,
*                     false otherwise
* RESULT
*    vlist - string containing the list of available volumes.
*            Or "ERROR: ..." string if failure.
* EXAMPLE
*    my $volumes = udefrag_s_get_avail_volumes($skip_rem);
*    $_ = $volumes;
*    if(m/ERROR/o){
*        # handle error
*    }else{
*        # display list of available volumes
*    }
* NOTES
*    if(skip_removable == FALSE && you have 
*      floppy drive without floppy disk)
*       then you will hear noise :))
******/
char * __stdcall udefrag_s_get_avail_volumes(int skip_removable)
{
	char buffer[ERR_MSG_SIZE];
	volume_info *v;
	int i;
	char chr;
	char t[256];
	int p;
	double d;
	

	if(udefrag_get_avail_volumes(&v,skip_removable) < 0){
		udefrag_pop_error(buffer,ERR_MSG_SIZE);
		sprintf(vlist,"ERROR: %s",buffer);
		return vlist;
	}
	strcpy(vlist,"");
	for(i = 0;;i++){
		chr = v[i].letter;
		if(!chr) break;
		sprintf(t,"%c:%s:",chr,v[i].fsname);
		strcat(vlist,t);
		fbsize(t,(ULONGLONG)(v[i].total_space.QuadPart));
		strcat(vlist,t);
		strcat(vlist,":");
		fbsize(t,(ULONGLONG)(v[i].free_space.QuadPart));
		strcat(vlist,t);
		strcat(vlist,":");
		d = (double)(signed __int64)(v[i].free_space.QuadPart);
		d /= ((double)(signed __int64)(v[i].total_space.QuadPart) + 0.1);
		p = (int)(100 * d);
		sprintf(t,"%u %%:",p);
		strcat(vlist,t);
	}
	/* remove last ':' */
	if(strlen(vlist) > 0)
		vlist[strlen(vlist) - 1] = 0;
	return vlist;
}

int __stdcall udefrag_s_analyse(unsigned char letter,STATUPDATEPROC sproc)
{
	return udefrag_send_command_ex('a',letter,sproc);
}

int __stdcall udefrag_s_defragment(unsigned char letter,STATUPDATEPROC sproc)
{
	return udefrag_send_command_ex('d',letter,sproc);
}

int __stdcall udefrag_s_optimize(unsigned char letter,STATUPDATEPROC sproc)
{
	return udefrag_send_command_ex('c',letter,sproc);
}

#endif /* PERL_SUPPORT */
