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
 * @file kmd.c
 * @brief Obsolete kernel mode driver interaction code.
 * @addtogroup Driver
 * @{
 */

#include "../../include/ntndk.h"

#include "../../include/udefrag.h"
#include "../../include/ultradfg.h"
#include "../zenwinx/zenwinx.h"

#ifdef KERNEL_MODE_DRIVER_SUPPORT

char user_mode_buffer[65536]; /* for nt 4.0 */
WINX_FILE *f_ud = NULL;
WINX_FILE *f_map = NULL, *f_stat = NULL, *f_stop = NULL;

int __stdcall kmd_init(long map_size)
{
	(void)winx_enable_privilege(SE_LOAD_DRIVER_PRIVILEGE);
	if(winx_load_driver(L"ultradfg") < 0){
		DebugPrint("------------------------------------------------------------\n");
		DebugPrint("UltraDefrag kernel mode driver is not installed\n");
		DebugPrint("or Windows denies it.\n");
		DebugPrint("------------------------------------------------------------\n");
		return (-1);
	}
	
	/* open our device */
	f_ud = winx_fopen("\\Device\\UltraDefrag","w");
	if(!f_ud) goto init_fail;
	f_map = winx_fopen("\\Device\\UltraDefragMap","r");
	if(!f_map) goto init_fail;
	f_stat = winx_fopen("\\Device\\UltraDefragStat","r");
	if(!f_stat) goto init_fail;
	f_stop = winx_fopen("\\Device\\UltraDefragStop","w");
	if(!f_stop) goto init_fail;
	/* set user mode buffer - nt 4.0 specific */
	if(winx_ioctl(f_ud,IOCTL_SET_USER_MODE_BUFFER,"User mode buffer setup",
		user_mode_buffer,0,NULL,0,NULL) < 0) goto init_fail;
	/* set cluster map size */
	if(winx_ioctl(f_ud,IOCTL_SET_CLUSTER_MAP_SIZE,"Cluster map buffer setup",
		&map_size,sizeof(long),NULL,0,NULL) < 0) goto init_fail;
	/* load settings */
	if(udefrag_reload_settings() < 0) goto init_fail;
	return 0;
init_fail:
	kmd_unload();
	DebugPrint("Cannot initialize the kernel mode driver!\n");
	return (-1);
}

int __stdcall kmd_unload(void)
{
	if(kernel_mode_driver){
		/* close device handle */
		if(f_ud) winx_fclose(f_ud);
		if(f_map) winx_fclose(f_map);
		if(f_stat) winx_fclose(f_stat);
		if(f_stop) winx_fclose(f_stop);
		/* unload the driver */
		(void)winx_unload_driver(L"ultradfg");
	}
	return 0;
}

#else /* KERNEL_MODE_DRIVER_SUPPORT */

int __stdcall kmd_init(long map_size)
{
	(void)map_size;
	return (-1);
}

int __stdcall kmd_unload(void)
{
	return 0;
}

#endif /* KERNEL_MODE_DRIVER_SUPPORT */

/** @} */
