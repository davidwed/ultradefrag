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
 * @file report.c
 * @brief File fragmentation reports code.
 * @addtogroup Reports
 * @{
 */

#include "globals.h"

/**
 * @brief The size of the report saving buffer, in bytes.
 */
#define RSB_SIZE (512 * 1024)

/*
* Micro Edition generates text reports only,
* all other editions - both reports.
*/

/* function prototypes */
static void RemoveLuaReportFromDisk(char *volume_name);
static void RemoveTextReportFromDisk(char *volume_name);
static void RemoveHtmlReportFromDisk(char *volume_name);
static BOOLEAN SaveLuaReportToDisk(char *volume_name);
static void WriteLuaReportBody(WINX_FILE *f,BOOLEAN is_filtered);
static BOOLEAN SaveTextReportToDisk(char *volume_name);
static void WriteTextReportBody(WINX_FILE *f,BOOLEAN is_filtered);

static BOOLEAN SaveReportToDiskInternal(char *volume_name);

/**
 * @brief Removes all file fragmentation reports from the volume.
 * @param[in] volume_name the name of the volume.
 */
void RemoveReportFromDisk(char *volume_name)
{
	RemoveTextReportFromDisk(volume_name);
	RemoveLuaReportFromDisk(volume_name);
	RemoveHtmlReportFromDisk(volume_name);
}

static void RemoveLuaReportFromDisk(char *volume_name)
{
	char path[64];
	
	(void)_snprintf(path,64,"\\??\\%s:\\fraglist.luar",volume_name);
	path[63] = 0;
	(void)winx_delete_file(path);
}

static void RemoveTextReportFromDisk(char *volume_name)
{
	char path[64];
	
	(void)_snprintf(path,64,"\\??\\%s:\\fraglist.txt",volume_name);
	path[63] = 0;
	(void)winx_delete_file(path);
}

static void RemoveHtmlReportFromDisk(char *volume_name)
{
	char path[64];
	
	(void)_snprintf(path,64,"\\??\\%s:\\fraglist.htm",volume_name);
	path[63] = 0;
	(void)winx_delete_file(path);

	(void)_snprintf(path,64,"\\??\\%s:\\fraglist.html",volume_name);
	path[63] = 0;
	(void)winx_delete_file(path);
}

/**
 * @brief Saves a file fragmentation report on the volume.
 * @param[in] volume_name the name of the volume.
 */
BOOLEAN SaveReportToDisk(char *volume_name)
{
	ULONGLONG tm, time;
	BOOLEAN result;
	
	DebugPrint("SaveReportToDisk() started...\n");
	tm = winx_xtime();

	result = SaveReportToDiskInternal(volume_name);
	
	time = winx_xtime() - tm;
	DebugPrint("SaveReportToDisk() completed in %I64u ms.\n",time);
	return result;
}

static BOOLEAN SaveReportToDiskInternal(char *volume_name)
{
	if(!SaveTextReportToDisk(volume_name)) return FALSE;
#ifdef MICRO_EDITION
	return TRUE;
#else
	return SaveLuaReportToDisk(volume_name);
#endif
}

#ifndef MICRO_EDITION

/**
 * @brief Saves a Lua file fragmentation report on the volume.
 * @param[in] volume_name the name of the volume.
 */
static BOOLEAN SaveLuaReportToDisk(char *volume_name)
{
	char buffer[512];
	char path[64];
	WINX_FILE *f;
	
	if(disable_reports) return TRUE;

	(void)_snprintf(path,64,"\\??\\%s:\\fraglist.luar",volume_name);
	path[63] = 0;
	f = winx_fbopen(path,"w",RSB_SIZE);
	if(f == NULL){
		f = winx_fopen(path,"w");
		if(f == NULL){
			DebugPrint("Cannot create %s file!\n",path);
			return FALSE;
		}
	}
	
	(void)_snprintf(buffer,sizeof(buffer),
		"-- UltraDefrag report for volume %s:\r\n\r\n"
		"format_version = 2\r\n\r\n"
		"volume_letter = \"%s\"\r\n\r\n"
		"files = {\r\n",
		volume_name, volume_name
		);
	buffer[sizeof(buffer) - 1] = 0;
	(void)winx_fwrite(buffer,1,strlen(buffer),f);

	WriteLuaReportBody(f,FALSE);
	//WriteLuaReportBody(f,TRUE);

	(void)strcpy(buffer,"}\r\n");
	(void)winx_fwrite(buffer,1,strlen(buffer),f);
	winx_fclose(f);

	DebugPrint("Report saved to %s\n",path);
	return TRUE;
}

static void WriteLuaReportBody(WINX_FILE *f,BOOLEAN is_filtered)
{
	char buffer[256];
	PFRAGMENTED pf;
	char *comment;
	int i, offset, name_length;

	pf = fragmfileslist; if(!pf) return;
	do {
		if(pf->pfn->is_filtered != is_filtered)
			goto next_item;
		if(pf->pfn->is_dir) comment = "[DIR]";
		else if(pf->pfn->is_overlimit) comment = "[OVR]";
		else if(pf->pfn->is_compressed) comment = "[CMP]";
		else comment = " - ";
		(void)_snprintf(buffer, sizeof(buffer),
			"\t{fragments = %u,size = %I64u,filtered = %u,"
			"comment = \"%s\",uname = {",
			(UINT)pf->pfn->n_fragments,
			pf->pfn->clusters_total * bytes_per_cluster,
			(UINT)(pf->pfn->is_filtered & 0x1),
			comment
			);
		buffer[sizeof(buffer) - 1] = 0;
		(void)winx_fwrite(buffer,1,strlen(buffer),f);

		/* skip \??\ sequence in the beginning of the path */
		name_length = pf->pfn->name.Length / sizeof(short);
		if(name_length > 4) offset = 4;	else offset = 0;
		
		for(i = offset; i < name_length; i++){
			(void)_snprintf(buffer,sizeof(buffer),"%u,",(unsigned int)pf->pfn->name.Buffer[i]);
			buffer[sizeof(buffer) - 1] = 0;
			(void)winx_fwrite(buffer,1,strlen(buffer),f);
		}

		(void)strcpy(buffer,"}},\r\n");
		(void)winx_fwrite(buffer,1,strlen(buffer),f);
	next_item:
		pf = pf->next_ptr;
	} while(pf != fragmfileslist);
}

#endif /* MICRO_EDITION */

/**
 * @brief Saves a PlainText file fragmentation report on the volume.
 * @param[in] volume_name the name of the volume.
 */
static BOOLEAN SaveTextReportToDisk(char *volume_name)
{
	char path[64];
	WINX_FILE *f;
	short buffer[256];
	int length = sizeof(buffer) / sizeof(short);
	
	if(disable_reports) return TRUE;

	(void)_snprintf(path,64,"\\??\\%s:\\fraglist.txt",volume_name);
	path[63] = 0;
	f = winx_fbopen(path,"w",RSB_SIZE);
	if(f == NULL){
		f = winx_fopen(path,"w");
		if(f == NULL){
			DebugPrint("Cannot create %s file!\n",path);
			return FALSE;
		}
	}
	
	wcscpy(buffer,L";---------------------------------------------------------------------------------------------\r\n");
	(void)winx_fwrite(buffer,sizeof(short),wcslen(buffer),f);

	(void)_snwprintf(buffer,length,L"; Fragmented files on %hs:\r\n;\r\n",volume_name);
	buffer[length - 1] = 0;
	(void)winx_fwrite(buffer,sizeof(short),wcslen(buffer),f);

	(void)_snwprintf(buffer,length,L"; Fragments%21hs%9hs    Filename\r\n","Filesize","Comment");
	buffer[length - 1] = 0;
	(void)winx_fwrite(buffer,sizeof(short),wcslen(buffer),f);

	wcscpy(buffer,L";---------------------------------------------------------------------------------------------\r\n");
	(void)winx_fwrite(buffer,sizeof(short),wcslen(buffer),f);
	
	WriteTextReportBody(f,FALSE);
	//WriteTextReportBody(f,TRUE);

	winx_fclose(f);

	DebugPrint("Report saved to %s\n",path);
	return TRUE;
}

static void WriteTextReportBody(WINX_FILE *f,BOOLEAN is_filtered)
{
	PFRAGMENTED pf;
	short buffer[256];
	int length = sizeof(buffer) / sizeof(short);
	char *comment;
	int offset;

	pf = fragmfileslist; if(!pf) return;
	do {
		if(pf->pfn->is_filtered != is_filtered)
			goto next_item;
		if(pf->pfn->is_dir) comment = "[DIR]";
		else if(pf->pfn->is_overlimit) comment = "[OVR]";
		else if(pf->pfn->is_compressed) comment = "[CMP]";
		else comment = " - ";
		
		(void)_snwprintf(buffer,length,L"\r\n%11u%21I64u%9hs    ",
			(UINT)pf->pfn->n_fragments,
			pf->pfn->clusters_total * bytes_per_cluster,
			comment);
		buffer[length - 1] = 0;
		(void)winx_fwrite(buffer,sizeof(short),wcslen(buffer),f);
		
		/* skip \??\ sequence in the beginning of the path */
		if(pf->pfn->name.Length > 0x8) offset = 0x8;
		else offset = 0x0;
		(void)winx_fwrite((char *)pf->pfn->name.Buffer + offset,1,pf->pfn->name.Length - offset,f);

	next_item:
		pf = pf->next_ptr;
	} while(pf != fragmfileslist);
}

/** @} */
