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
* Udefrag.dll interface header.
*/

#ifndef _UDEFRAG_H_
#define _UDEFRAG_H_

#include "ultradfg.h"

#define MAX_DOS_DRIVES 26
#define MAXFSNAME      32  /* I think that's enough. */

typedef struct _volume_info {
	char letter;
	char fsname[MAXFSNAME];
	LARGE_INTEGER total_space;
	LARGE_INTEGER free_space;
	int is_removable;
} volume_info;

int __stdcall udefrag_init(long map_size);
int __stdcall udefrag_unload(void);

int __stdcall udefrag_stop(void);
int __stdcall udefrag_get_progress(STATISTIC *pstat, double *percentage);
int __stdcall udefrag_get_map(char *buffer,int size);
char *  __stdcall udefrag_get_default_formatted_results(STATISTIC *pstat);

int __stdcall udefrag_reload_settings();

int __stdcall udefrag_get_avail_volumes(volume_info **vol_info,int skip_removable);
int __stdcall udefrag_validate_volume(unsigned char letter,int skip_removable);

/*
* This callback procedure was designed especially 
* to refresh progress indicator during defragmentation process.
*/
typedef int (__stdcall *STATUPDATEPROC)(int done_flag);
int __stdcall udefrag_send_command_ex(unsigned char command,unsigned char letter,STATUPDATEPROC sproc);

/* interface for scripting languages */
/*char * __stdcall udefrag_s_get_map(int size);
char * __stdcall udefrag_s_get_avail_volumes(int skip_removable);
*/
/*
* because perl/Tk is incompatible with threads 
* we should provide callback functions
*/
/*int __stdcall udefrag_s_analyse(unsigned char letter,STATUPDATEPROC sproc);
int __stdcall udefrag_s_defragment(unsigned char letter,STATUPDATEPROC sproc);
int __stdcall udefrag_s_optimize(unsigned char letter,STATUPDATEPROC sproc);
int __stdcall scheduler_get_avail_letters(char *letters);
*/

#define udefrag_analyse(letter,sproc) udefrag_send_command_ex('a',letter,sproc)
#define udefrag_defragment(letter,sproc) udefrag_send_command_ex('d',letter,sproc)
#define udefrag_optimize(letter,sproc) udefrag_send_command_ex('c',letter,sproc)

int __stdcall udefrag_fbsize(ULONGLONG number, int digits, char *buffer, int length);
int __stdcall udefrag_dfbsize(char *string,ULONGLONG *pnumber);

#ifndef _ZENWINX_H_
typedef void (__stdcall *ERRORHANDLERPROC)(short *msg);
#endif
ERRORHANDLERPROC __stdcall udefrag_set_error_handler(ERRORHANDLERPROC handler);

#endif /* _UDEFRAG_H_ */
