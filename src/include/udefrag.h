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

#define MAX_DOS_DRIVES 26
#define MAXFSNAME      32  /* I think that's enough. */

typedef struct _ud_options
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
} ud_options;

typedef struct _volume_info
{
	char letter;
	char fsname[MAXFSNAME];
	LARGE_INTEGER total_space;
	LARGE_INTEGER free_space;
} volume_info;

short * __stdcall udefrag_init(int argc, short **argv,int native_mode);
short * __stdcall udefrag_unload(BOOL save_options);

short * __stdcall udefrag_analyse(unsigned char letter);
short * __stdcall udefrag_defragment(unsigned char letter);
short * __stdcall udefrag_optimize(unsigned char letter);
short * __stdcall udefrag_stop(void);
short * __stdcall udefrag_get_progress(STATISTIC *pstat, double *percentage);
short * __stdcall udefrag_get_map(char *buffer,int size);
char *  __stdcall udefrag_get_default_formatted_results(STATISTIC *pstat);

ud_options * __stdcall udefrag_get_options(void);
short * __stdcall udefrag_set_options(ud_options *ud_opts);
short * __stdcall udefrag_clean_registry(void);
short * __stdcall udefrag_native_clean_registry(void);

short * __stdcall udefrag_get_avail_volumes(volume_info **vol_info,int skip_removable);
short * __stdcall udefrag_validate_volume(unsigned char letter,int skip_removable);
short * __stdcall scheduler_get_avail_letters(char *letters);

void   __stdcall nsleep(int msec);
char * __stdcall create_thread(LPTHREAD_START_ROUTINE start_addr,HANDLE *phandle);
void   __stdcall exit_thread(void);
int    __stdcall get_os_version(void);

int    __stdcall fbsize(char *s,ULONGLONG n);
int    __stdcall fbsize2(char *s,ULONGLONG n);
int    __stdcall dfbsize2(char *s,ULONGLONG *pn);

/* interface for scripting languages */
char * __stdcall udefrag_s_init(void);
char * __stdcall udefrag_s_unload(BOOL save_options);

char * __stdcall udefrag_s_analyse(unsigned char letter);
char * __stdcall udefrag_s_defragment(unsigned char letter);
char * __stdcall udefrag_s_optimize(unsigned char letter);
char * __stdcall udefrag_s_stop(void);
char * __stdcall udefrag_s_get_progress(void);
char * __stdcall udefrag_s_get_map(int size);

char * __stdcall udefrag_s_get_options(void);
char * __stdcall udefrag_s_set_options(char *string);

char * __stdcall udefrag_s_get_avail_volumes(int skip_removable);
char * __stdcall udefrag_s_validate_volume(unsigned char letter,int skip_removable);
char * __stdcall scheduler_s_get_avail_letters(void);

/* because perl/Tk is incompatible with threads 
 * we should provide callback functions
 */
typedef int (__stdcall *STATUPDATEPROC)(int done_flag);
short * __stdcall udefrag_analyse_ex(unsigned char letter,STATUPDATEPROC sproc);
short * __stdcall udefrag_defragment_ex(unsigned char letter,STATUPDATEPROC sproc);
short * __stdcall udefrag_optimize_ex(unsigned char letter,STATUPDATEPROC sproc);
short * __stdcall udefrag_get_ex_command_result(void);

char * __stdcall udefrag_s_analyse_ex(unsigned char letter,STATUPDATEPROC sproc);
char * __stdcall udefrag_s_defragment_ex(unsigned char letter,STATUPDATEPROC sproc);
char * __stdcall udefrag_s_optimize_ex(unsigned char letter,STATUPDATEPROC sproc);
char * __stdcall udefrag_s_get_ex_command_result(void);

#endif /* _UDEFRAG_H_ */
