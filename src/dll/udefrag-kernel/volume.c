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
 * @brief Generic volume related routines.
 * @addtogroup Volume
 * @{
 */

#include "globals.h"

/**
 * @brief Opens the volume handle.
 * @param[in] volume_name the name of the volume.
 * @return Zero for success, negative value otherwise.
 */
int OpenVolume(char *volume_name)
{
	char path[64];
	char flags[2];
	#define FLAG 'r'
	
	CloseVolume();
	(void)_snprintf(path,64,"\\??\\%s:",volume_name);
	path[63] = 0;
#if FLAG != 'r'
#error Volume must be opened for read access!
#endif
	flags[0] = FLAG; flags[1] = 0;
	fVolume = winx_fopen(path,flags);
	if(!fVolume){
		DebugPrint("Cannot open volume %s!\n",path);
		return (-1);
	}
	return 0;
}

/**
 * @brief Flushes all file buffers.
 * @param[in] volume_name the name of the volume.
 */
void FlushAllFileBuffers(char *volume_name)
{
	char path[64];
	WINX_FILE *f;
	
	(void)_snprintf(path,64,"\\??\\%s:",volume_name);
	path[63] = 0;

	f = winx_fopen(path,"r+");
	if(f){
		(void)winx_fflush(f);
		winx_fclose(f);
	}
}

/**
 * @brief Closes the volume handle.
 */
void CloseVolume(void)
{
	winx_fclose(fVolume);
	fVolume = NULL;
}

/** @} */
