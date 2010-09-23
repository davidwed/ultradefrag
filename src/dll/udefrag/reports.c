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
 * @file reports.c
 * @brief File fragmentation reports.
 * @addtogroup Reports
 * @{
 */

#include "udefrag-internals.h"

/**
 * @brief The size of the report saving buffer, in bytes.
 */
#define RSB_SIZE (512 * 1024)

/*
* Micro Edition generates text reports only,
* all other editions - both reports.
*/

static int save_text_report(udefrag_job_parameters *jp)
{
	char path[] = "\\??\\A:\\fraglist.txt";
	WINX_FILE *f;
	short buffer[256];
	int size = sizeof(buffer) / sizeof(short);
	udefrag_fragmented_file *file;
	char *comment;
	int length, offset;
	
	path[4] = jp->volume_letter;
	f = winx_fbopen(path,"w",RSB_SIZE);
	if(f == NULL){
		f = winx_fopen(path,"w");
		if(f == NULL){
			return (-1);
		}
	}
	
	/* write 0xFEFF to be able to view report in boot time shell */
	buffer[0] = 0xFEFF;
	(void)winx_fwrite(buffer,sizeof(short),1,f);

	/* print header */
	wcscpy(buffer,L";---------------------------------------------------------------------------------------------\r\n");
	(void)winx_fwrite(buffer,sizeof(short),wcslen(buffer),f);

	(void)_snwprintf(buffer,size,L"; Fragmented files on %c:\r\n;\r\n",jp->volume_letter);
	buffer[size - 1] = 0;
	(void)winx_fwrite(buffer,sizeof(short),wcslen(buffer),f);

	(void)_snwprintf(buffer,size,L"; Fragments%21hs%9hs    Filename\r\n","Filesize","Comment");
	buffer[size - 1] = 0;
	(void)winx_fwrite(buffer,sizeof(short),wcslen(buffer),f);

	wcscpy(buffer,L";---------------------------------------------------------------------------------------------\r\n");
	(void)winx_fwrite(buffer,sizeof(short),wcslen(buffer),f);

	/* print body */
	for(file = jp->fragmented_files; file; file = file->next){
		if(!is_excluded(file->f)){
			if(is_directory(file->f))
				comment = "[DIR]";
			else if(is_compressed(file->f))
				comment = "[CMP]";
			else if(is_over_limit(file->f))
				comment = "[OVR]";
			else
				comment = " - ";
			
			(void)_snwprintf(buffer,size,L"\r\n%11u%21I64u%9hs    ",
				(UINT)file->f->disp.fragments,
				file->f->disp.clusters * jp->v_info.bytes_per_cluster,
				comment);
			buffer[size - 1] = 0;
			(void)winx_fwrite(buffer,sizeof(short),wcslen(buffer),f);
			
			if(file->f->path){
				/* skip \??\ sequence in the beginning of the path */
				length = wcslen(file->f->path);
				if(length > 4) offset = 4; else offset = 0;
				(void)winx_fwrite(file->f->path + offset,sizeof(short),length - offset,f);
			}
		}
		if(file->next == jp->fragmented_files) break;
	}
	
	DebugPrint("report saved to %s",path);
	winx_fclose(f);
	return 0;
}

static int save_lua_report(udefrag_job_parameters *jp)
{
	char path[] = "\\??\\A:\\fraglist.luar";
	WINX_FILE *f;
	char buffer[512];
	udefrag_fragmented_file *file;
	char *comment;
	int i, length, offset;
	
#ifdef MICRO_EDITION
	return 0;
#endif

	path[4] = jp->volume_letter;
	f = winx_fbopen(path,"w",RSB_SIZE);
	if(f == NULL){
		f = winx_fopen(path,"w");
		if(f == NULL){
			return (-1);
		}
	}

	/* print header */
	(void)_snprintf(buffer,sizeof(buffer),
		"-- UltraDefrag report for volume %c:\r\n\r\n"
		"format_version = 2\r\n\r\n"
		"volume_letter = \"%c\"\r\n\r\n"
		"files = {\r\n",
		jp->volume_letter, jp->volume_letter
		);
	buffer[sizeof(buffer) - 1] = 0;
	(void)winx_fwrite(buffer,1,strlen(buffer),f);
	
	/* print body */
	for(file = jp->fragmented_files; file; file = file->next){
		if(!is_excluded(file->f)){
			if(is_directory(file->f))
				comment = "[DIR]";
			else if(is_compressed(file->f))
				comment = "[CMP]";
			else if(is_over_limit(file->f))
				comment = "[OVR]";
			else
				comment = " - ";
			
			(void)_snprintf(buffer, sizeof(buffer),
				"\t{fragments = %u,size = %I64u,filtered = %u,"
				"comment = \"%s\",uname = {",
				(UINT)file->f->disp.fragments,
				file->f->disp.clusters * jp->v_info.bytes_per_cluster,
				(UINT)is_excluded(file->f),
				comment
				);
			buffer[sizeof(buffer) - 1] = 0;
			(void)winx_fwrite(buffer,1,strlen(buffer),f);

			if(file->f->path == NULL){
				strcpy(buffer,"0");
				(void)winx_fwrite(buffer,1,strlen(buffer),f);
			} else {
				/* skip \??\ sequence in the beginning of the path */
				length = wcslen(file->f->path);
				if(length > 4) offset = 4; else offset = 0;
				for(i = offset; i < length; i++){
					(void)_snprintf(buffer,sizeof(buffer),"%u,",(unsigned int)file->f->path[i]);
					buffer[sizeof(buffer) - 1] = 0;
					(void)winx_fwrite(buffer,1,strlen(buffer),f);
				}
			}

			(void)strcpy(buffer,"}},\r\n");
			(void)winx_fwrite(buffer,1,strlen(buffer),f);
		}
		if(file->next == jp->fragmented_files) break;
	}
	
	/* print footer */
	(void)strcpy(buffer,"}\r\n");
	(void)winx_fwrite(buffer,1,strlen(buffer),f);

	DebugPrint("report saved to %s",path);
	winx_fclose(f);
	return 0;
}

/**
 * @brief Saves fragmentation report on the volume.
 * @return Zero for success, negative value otherwise.
 */
int save_fragmentation_reports(udefrag_job_parameters *jp)
{
	int result = 0;
	ULONGLONG time;
	
	if(jp->udo.disable_reports)
		return 0;
	
	winx_dbg_print_header(0,0,"*");
	winx_dbg_print_header(0,0,"reports saving started");
	time = winx_xtime();

	result = save_text_report(jp);
	if(result >= 0)
		result = save_lua_report(jp);
	
	winx_dbg_print_header(0,0,"reports saved in %I64u ms",
		winx_xtime() - time);
	return result;
}

/**
 * @brief Removes all fragmentation reports from the volume.
 */
void remove_fragmentation_reports(udefrag_job_parameters *jp)
{
	char *paths[] = {
		"\\??\\%c:\\fraglist.luar",
		"\\??\\%c:\\fraglist.txt",
		"\\??\\%c:\\fraglist.htm",
		"\\??\\%c:\\fraglist.html",
		NULL
	};
	char path[64];
	int i;
	
	winx_dbg_print_header(0,0,"*");
	
	for(i = 0; paths[i]; i++){
		_snprintf(path,64,paths[i],jp->volume_letter);
		path[63] = 0;
		(void)winx_delete_file(path);
	}
}

/** @} */
