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
* All UltraDefrag editions produce
* both Lua and text reports by default.
*/

int winx_fwrite_test(char *filename);

static int save_text_report(udefrag_job_parameters *jp)
{
	char path[] = "\\??\\A:\\fraglist.txt";
	WINX_FILE *f;
	short buffer[256];
	const int size = sizeof(buffer) / sizeof(short);
	udefrag_fragmented_file *file;
	char *comment;
	char *status;
	int length, offset;
	char human_readable_size[32];
	int n1, n2;
	char s[32]; /* must be at least 32 chars long */
	winx_time t;
	
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

	(void)_snwprintf(buffer,size,L"; Fragmented files on %c: ",jp->volume_letter);
	buffer[size - 1] = 0;
	(void)winx_fwrite(buffer,sizeof(short),wcslen(buffer),f);

	memset(&t,0,sizeof(winx_time));
	(void)winx_get_local_time(&t);
	(void)_snwprintf(buffer,size,L"[%02i.%02i.%04i at %02i:%02i]\r\n;\r\n",
		(int)t.day,(int)t.month,(int)t.year,(int)t.hour,(int)t.minute);
	buffer[size - 1] = 0;
	(void)winx_fwrite(buffer,sizeof(short),wcslen(buffer),f);

	(void)_snwprintf(buffer,size,L"; Fragments%12hs%9hs%12hs    Filename\r\n","Filesize","Comment","Status");
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
			
			if(is_locked(file->f))
				status = "locked";
			else if(is_moving_failed(file->f)) /* before is_too_large() check */
				status = "move failed";
			else if(is_too_large(file->f))
				status = "too big";
			else
				status = " - ";
			
			(void)winx_fbsize(file->f->disp.clusters * jp->v_info.bytes_per_cluster,
				1,human_readable_size,sizeof(human_readable_size));
			if(sscanf(human_readable_size,"%u.%u %31s",&n1,&n2,s) == 3){
				//if(n2 == 0){ /* remove trailing zero */
					_snprintf(human_readable_size,sizeof(human_readable_size),"%u %s",n1,s);
					human_readable_size[sizeof(human_readable_size) - 1] = 0;
				//}
			}
			(void)_snwprintf(buffer,size,L"\r\n%11u%12hs%9hs%12hs    ",
				(UINT)file->f->disp.fragments,human_readable_size,comment,status);
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
	char *status;
	int i, length, offset;
	char human_readable_size[32];
	int n1, n2;
	char s[32]; /* must be at least 32 chars long */
	winx_time t;
	
	path[4] = jp->volume_letter;
	f = winx_fbopen(path,"w",RSB_SIZE);
	if(f == NULL){
		f = winx_fopen(path,"w");
		if(f == NULL){
			return (-1);
		}
	}

	/* print header */
	memset(&t,0,sizeof(winx_time));
	(void)winx_get_local_time(&t);
	(void)_snprintf(buffer,sizeof(buffer),
		"-- UltraDefrag report for volume %c:\r\n\r\n"
		"format_version = 4\r\n\r\n"
		"volume_letter = \"%c\"\r\n"
		/*"current_time = \"%02i.%02i.%04i at %02i:%02i\"\r\n"*/
		"current_time = {\r\n"
		"\tyear = %04i,\r\n"
		"\tmonth = %02i,\r\n"
		"\tday = %02i,\r\n"
		"\thour = %02i,\r\n"
		"\tmin = %02i,\r\n"
		"\tsec = %02i,\r\n"
		"\tisdst = false\r\n"
		"}\r\n\r\n"
		"files = {\r\n",
		jp->volume_letter, jp->volume_letter,
		/*(int)t.day,(int)t.month,(int)t.year,(int)t.hour,(int)t.minute*/
		(int)t.year,(int)t.month,(int)t.day,(int)t.hour,(int)t.minute,(int)t.second
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
			
			if(is_locked(file->f))
				status = "locked";
			else if(is_moving_failed(file->f)) /* before is_too_large() check */
				status = "file moving failed";
			else if(is_too_large(file->f))
				status = "block of free space too small";
			else
				status = " - ";
			
			(void)winx_fbsize(file->f->disp.clusters * jp->v_info.bytes_per_cluster,
				1,human_readable_size,sizeof(human_readable_size));
			if(sscanf(human_readable_size,"%u.%u %31s",&n1,&n2,s) == 3){
				//if(n2 == 0){ /* remove trailing zero */
					_snprintf(human_readable_size,sizeof(human_readable_size),"%u&nbsp;%s",n1,s);
					human_readable_size[sizeof(human_readable_size) - 1] = 0;
				//}
			}
			(void)_snprintf(buffer, sizeof(buffer),
				"\t{fragments = %u,size = %I64u,hrsize = \"%s\",filtered = %u,"
				"comment = \"%s\",status = \"%s\",uname = {",
				(UINT)file->f->disp.fragments,
				file->f->disp.clusters * jp->v_info.bytes_per_cluster,
				human_readable_size,
				(UINT)is_excluded(file->f),
				comment,status
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

	/*winx_fwrite_test("\\??\\L:\\test.txt");*/
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


/*
* test suite for winx_fwrite()
*/

static int winx_fwrite_test_buffered_io(char *filename,char **strings,int test_number,int buffer_size);
static int winx_fwrite_test_compare(char *filename,char **strings,int total_length,int test_number);

/*
* accepts native filenames like that:
* \??\C:\test.txt
* returns zero on success, negative value otherwise
*/
int winx_fwrite_test(char *filename)
{
	WINX_FILE *f;
	char *strings[] = {
		"Sherlock Holmes took his bottle from the corner of the mantelpiece, \r\n",
		"and his hypodermic syringe from its neat morocco case. With his long, \r\n",
		"white, nervous fingers he adjusted the delicate needle and rolled back \r\n",
		"his left shirtcuff. For some little time his eyes rested thoughtfully \r\n",
		"upon the sinewy forearm and wrist, all dotted and scarred with innumerable \r\n",
		NULL
	};
	int total_length;
	int i;
	
	DebugPrint("@winx_fwrite_test: tests started...\n");

	/* 1. test not buffered i/o */
	f = winx_fopen(filename,"w");
	if(f == NULL){
		DebugPrint("@winx_fwrite_test: winx_fopen failed in test 1\n");
		return (-1);
	}
	total_length = 0;
	for(i = 0; strings[i]; i++){
		total_length += strlen(strings[i]);
		if(!winx_fwrite(strings[i],sizeof(char),strlen(strings[i]),f)){
			DebugPrint("@winx_fwrite_test: winx_fwrite failed in test 1 for %s\n",strings[i]);
			winx_fclose(f);
			return (-1);
		}
	}
	winx_fclose(f);
	if(winx_fwrite_test_compare(filename,strings,total_length,1) < 0){
		return (-1);
	}
	
	/* 2. test buffered i/o with negative buffer size */
	if(winx_fwrite_test_buffered_io(filename,strings,2,-100) < 0)
		return (-1);
	
	/* 3. test buffered i/o with zero buffer size */
	if(winx_fwrite_test_buffered_io(filename,strings,3,0) < 0)
		return (-1);
	
	/* 4. test buffered i/o with small buffer size */
	if(winx_fwrite_test_buffered_io(filename,strings,4,10) < 0)
		return (-1);
	
	/* 5. test buffered i/o with medium buffer size */
	if(winx_fwrite_test_buffered_io(filename,strings,5,170) < 0)
		return (-1);
	
	/* 6. test buffered i/o with large buffer size */
	if(winx_fwrite_test_buffered_io(filename,strings,6,10000) < 0)
		return (-1);
	
	/* 7. test buffered i/o with buffer size equal to the length of the first string */
	if(winx_fwrite_test_buffered_io(filename,strings,7,strlen(strings[0])) < 0)
		return (-1);
	
	/* 8. test buffered i/o with buffer size equal to the length of the first two strings */
	if(winx_fwrite_test_buffered_io(filename,strings,8,strlen(strings[0]) + strlen(strings[1])) < 0)
		return (-1);
	
	/* 9. test buffered i/o with buffer size equal to the length of the first three strings */
	if(winx_fwrite_test_buffered_io(filename,strings,9,strlen(strings[0]) + strlen(strings[1]) + strlen(strings[2])) < 0)
		return (-1);
	
	/* 10. test buffered i/o with buffer size equal to the length of all strings */
	if(winx_fwrite_test_buffered_io(filename,strings,10,total_length) < 0)
		return (-1);
	
	DebugPrint("@winx_fwrite_test: all tests passed!\n");
	return 0;
}

static int winx_fwrite_test_buffered_io(char *filename,char **strings,int test_number,int buffer_size)
{
	WINX_FILE *f;
	int total_length;
	int i;

	f = winx_fbopen(filename,"w",buffer_size);
	if(f == NULL){
		DebugPrint("@winx_fwrite_test: winx_fbopen failed in test %i\n",test_number);
		return (-1);
	}
	total_length = 0;
	for(i = 0; strings[i]; i++){
		total_length += strlen(strings[i]);
		if(!winx_fwrite(strings[i],sizeof(char),strlen(strings[i]),f)){
			DebugPrint("@winx_fwrite_test: winx_fwrite failed in test %i for %s\n",test_number,strings[i]);
			winx_fclose(f);
			return (-1);
		}
	}
	winx_fclose(f);
	if(winx_fwrite_test_compare(filename,strings,total_length,test_number) < 0){
		return (-1);
	}
	return 0;
}

static int winx_fwrite_test_compare(char *filename,char **strings,int total_length,int test_number)
{
	int i;
	char *written_strings;
	size_t written_data_length;
	int j, n;
	
	/* read the entire file contents */
	written_strings = (char *)winx_get_file_contents(filename,&written_data_length);
	if(written_strings == NULL){
		DebugPrint("@winx_fwrite_test: winx_get_file_contents failed in test %i\n",test_number);
		return (-1);
	}
	if(written_data_length != total_length){
		DebugPrint("@winx_fwrite_test: size mismatch in test %i\n",test_number);
		winx_release_file_contents((void *)written_strings);
		return (-1);
	}
	/* compare the file contents with original data */
	j = 0;
	for(i = 0; strings[i]; i++){
		n = strlen(strings[i]);
		if(memcmp(written_strings + j,strings[i],n) != 0){
			DebugPrint("@winx_fwrite_test: contents mismatch in test %i\n",test_number);
			winx_release_file_contents((void *)written_strings);
			return (-1);
		}
		j += n;
	}
	winx_release_file_contents((void *)written_strings);
	DebugPrint("@winx_fwrite_test: test %i passed\n",test_number);
	return 0;
}

