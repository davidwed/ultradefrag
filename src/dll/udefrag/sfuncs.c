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
 *  udefrag.dll - middle layer between driver and user interfaces:
 *  simple interface functions set for scripting languages.
 */

#define WIN32_NO_STATUS
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../include/ntndk.h"
#include "../../include/udefrag.h"
#include "../../include/ultradfg.h"

#define MAX_DOS_DRIVES 26

char s_letters[MAX_DOS_DRIVES + 1];
char s_msg[4096];
char map[32768];

char * __stdcall udefrag_s_init(void)
{
	char *result;

	result = udefrag_init(FALSE);
	return result ? result : "";
}

char * __stdcall udefrag_s_unload(void)
{
	char *result;

	result = udefrag_unload();
	return result ? result : "";
}

char * __stdcall udefrag_s_analyse(unsigned char letter)
{
	char *result;

	result = udefrag_analyse(letter);
	return result ? result : "";
}

char * __stdcall udefrag_s_defragment(unsigned char letter)
{
	char *result;

	result = udefrag_defragment(letter);
	return result ? result : "";
}

char * __stdcall udefrag_s_optimize(unsigned char letter)
{
	char *result;

	result = udefrag_optimize(letter);
	return result ? result : "";
}

char * __stdcall udefrag_s_stop(void)
{
	char *result;

	result = udefrag_stop();
	return result ? result : "";
}

char * __stdcall udefrag_s_get_progress(void)
{
	return "";
}

char * __stdcall udefrag_s_get_map(int size)
{
	char *result;
	int i;

	if(size > sizeof(map) - 1)
		return "ERROR: Map resolution is too high!";
	result = udefrag_get_map(map,size - 1);
	if(result)
	{
		strcpy(s_msg,"ERROR: ");
		strcat(s_msg,result);
		return s_msg;
	}
	/* remove zeroes from the map and terminate string */
	for(i = 0; i < size - 1; i++)
		map[i] ++;
	map[i] = 0;
	return map;
}

char * __stdcall udefrag_s_load_settings(void)
{
	char *result;

	result = udefrag_load_settings(0,NULL);
	return result ? result : "";
}

char * __stdcall udefrag_s_get_settings(void)
{
	return "";
}

char * __stdcall udefrag_s_apply_settings(char *string)
{
	return "";
}

char * __stdcall udefrag_s_save_settings(void)
{
	char *result;

	result = udefrag_save_settings();
	return result ? result : "";
}

char * __stdcall udefrag_s_get_avail_volumes(int skip_removable)
{
	char *result;
	volume_info *v;
	int i;
	char chr;
	char t[256];
	int p;

	result = udefrag_get_avail_volumes(&v,skip_removable);
	if(result)
	{
		strcpy(s_msg,"ERROR: ");
		strcat(s_msg,result);
		return s_msg;
	}
	strcpy(s_msg,"");
	for(i = 0;;i++)
	{
		chr = v[i].letter;
		if(!chr) break;
		sprintf(t,"%c:%s:",chr,v[i].fsname);
		strcat(s_msg,t);
		fbsize(t,(ULONGLONG)(v[i].total_space.QuadPart));
		strcat(s_msg,t);
		strcat(s_msg,":");
		fbsize(t,(ULONGLONG)(v[i].free_space.QuadPart));
		strcat(s_msg,t);
		strcat(s_msg,":");
		p = (int)(100 * \
			((double)(signed __int64)(v[i].free_space.QuadPart) / (double)(signed __int64)(v[i].total_space.QuadPart)));
		sprintf(t,"%u %%:",p);
		strcat(s_msg,t);
	}
	/* remove last ':' */
	if(strlen(s_msg) > 0)
		s_msg[strlen(s_msg) - 1] = 0;
	return s_msg;
}

char * __stdcall udefrag_s_validate_volume(unsigned char letter,int skip_removable)
{
	char *result;

	result = udefrag_validate_volume(letter,skip_removable);
	return result ? result : "";
}

char * __stdcall scheduler_s_get_avail_letters(void)
{
	char *result;

	result = scheduler_get_avail_letters(s_letters);
	if(!result) return s_letters;
	strcpy(s_msg,"ERROR: ");
	strcat(s_msg,result);
	return s_msg;
}

char * __stdcall udefrag_s_analyse_ex(unsigned char letter,STATUPDATEPROC sproc)
{
	char *result;

	result = udefrag_analyse_ex(letter,sproc);
	return result ? result : "";
}

char * __stdcall udefrag_s_defragment_ex(unsigned char letter,STATUPDATEPROC sproc)
{
	char *result;

	result = udefrag_defragment_ex(letter,sproc);
	return result ? result : "";
}

char * __stdcall udefrag_s_optimize_ex(unsigned char letter,STATUPDATEPROC sproc)
{
	char *result;

	result = udefrag_optimize_ex(letter,sproc);
	return result ? result : "";
}
