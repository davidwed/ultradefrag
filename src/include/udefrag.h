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

#include <ntddkbd.h>
#include "ultradfg.h"

#define MAXFSNAME 32  /* I think that's enough. */

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
	long x;
	long y;
} ud_settings;

typedef struct _volume_info
{
	char letter;
	char fsname[MAXFSNAME];
	LARGE_INTEGER total_space;
	LARGE_INTEGER free_space;
} volume_info;

char * __cdecl udefrag_init(int native_mode);
char * __cdecl udefrag_unload(void);

char * __cdecl udefrag_analyse(unsigned char letter);
char * __cdecl udefrag_defragment(unsigned char letter);
char * __cdecl udefrag_optimize(unsigned char letter);
char * __cdecl udefrag_stop(void);
char * __cdecl udefrag_get_progress(STATISTIC *pstat, double *percentage);
char * __cdecl udefrag_get_map(char *buffer,int size);
char * __cdecl get_default_formatted_results(STATISTIC *pstat);

char * __cdecl udefrag_load_settings(int argc, short **argv);
ud_settings * __cdecl udefrag_get_settings(void);
char * __cdecl udefrag_apply_settings(void);
char * __cdecl udefrag_save_settings(void);

char * __cdecl udefrag_get_avail_volumes(volume_info **vol_info,int skip_removable);
char * __cdecl udefrag_validate_volume(unsigned char letter,int skip_removable);
char * __stdcall scheduler_get_avail_letters(char *letters);

void   __cdecl nputch(char ch);
void   __cdecl nprint(char *str);

char * __cdecl kb_open(void);
int    __cdecl kb_hit(KEYBOARD_INPUT_DATA *pkbd,int msec_timeout);
char * __cdecl kb_gets(char *buffer,int max_chars);
char * __cdecl kb_close(void);

void   __cdecl nsleep(int msec);

int    __cdecl fbsize(char *s,ULONGLONG n);
int    __cdecl fbsize2(char *s,ULONGLONG n);
int    __cdecl dfbsize2(char *s,ULONGLONG *pn);

/* interface for scripting languages */
char * __stdcall udefrag_s_init(void);
char * __stdcall udefrag_s_unload(void);

char * __stdcall udefrag_s_analyse(unsigned char letter);
char * __stdcall udefrag_s_defragment(unsigned char letter);
char * __stdcall udefrag_s_optimize(unsigned char letter);
char * __stdcall udefrag_s_stop(void);
char * __stdcall udefrag_s_get_progress(void);
char * __stdcall udefrag_s_get_map(int size);

char * __stdcall udefrag_s_load_settings(void);
char * __stdcall udefrag_s_get_settings(void);
char * __stdcall udefrag_s_apply_settings(char *string);
char * __stdcall udefrag_s_save_settings(void);

char * __stdcall udefrag_s_get_avail_volumes(int skip_removable);
char * __stdcall udefrag_s_validate_volume(unsigned char letter,int skip_removable);
char * __stdcall scheduler_s_get_avail_letters(void);

#endif /* _UDEFRAG_H_ */