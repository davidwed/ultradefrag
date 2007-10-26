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
 *  Udefrag.dll interface header.
 */

#ifndef _UDEFRAG_H_
#define _UDEFRAG_H_

#include "ultradfg.h"

typedef struct _ud_settings
{
	short *in_filter;
	short *ex_filter;
	short *boot_in_filter;
	short *boot_ex_filter;
	ULONGLONG sizelimit;
	DWORD skip_removable;
	int update_interval;
	BOOL show_progress;
	UCHAR report_type;
	UCHAR report_format;
	DWORD dbgprint_level;
	short *sched_letters;
	DWORD every_boot;
	DWORD next_boot;
	DWORD only_reg_and_pagefile;
} ud_settings;

char * __cdecl udefrag_init(int native_mode);
char * __cdecl udefrag_unload(void);
HANDLE get_device_handle();

char * __cdecl udefrag_analyse(unsigned char letter);
char * __cdecl udefrag_defragment(unsigned char letter);
char * __cdecl udefrag_optimize(unsigned char letter);
char * __cdecl udefrag_stop(void);
char * __cdecl udefrag_get_progress(STATISTIC *pstat, double *percentage);

char * __cdecl udefrag_load_settings(int argc, short **argv);
ud_settings * __cdecl udefrag_get_settings(void);
char * __cdecl udefrag_apply_settings(void);
char * __cdecl udefrag_save_settings(void);

int __cdecl fbsize(char *s,ULONGLONG n);
#endif /* _UDEFRAG_H_ */