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

static int InitializeReportSavingBuffer(void);
static void DestroyReportSavingBuffer(void);
static size_t WriteToReportSavingBuffer(const void *buffer,size_t size,size_t count,WINX_FILE *f);

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
	tm = _rdtsc();

	result = SaveReportToDiskInternal(volume_name);
	
	time = _rdtsc() - tm;
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
	f = winx_fopen(path,"w");
	if(f == NULL){
		DebugPrint("Cannot create %s file!\n",path);
		return FALSE;
	}
	
	InitializeReportSavingBuffer();
	
	(void)_snprintf(buffer,sizeof(buffer),
		"-- UltraDefrag report for volume %s:\r\n\r\n"
		"volume_letter = \"%s\"\r\n\r\n"
		"files = {\r\n",
		volume_name, volume_name
		);
	buffer[sizeof(buffer) - 1] = 0; /* to be sure that the buffer is terminated by zero */
	(void)WriteToReportSavingBuffer(buffer,1,strlen(buffer),f);

	WriteLuaReportBody(f,FALSE);
	//WriteLuaReportBody(f,TRUE);

	(void)strcpy(buffer,"}\r\n");
	(void)WriteToReportSavingBuffer(buffer,1,strlen(buffer),f);

	DestroyReportSavingBuffer();
	
	winx_fclose(f);

	DebugPrint("Report saved to %s\n",path);
	return TRUE;
}

static void WriteLuaReportBody(WINX_FILE *f,BOOLEAN is_filtered)
{
	char buffer[256];
	PFRAGMENTED pf;
	unsigned char *p;
	int i, offset;
	char *comment;
	ANSI_STRING as;

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
			"comment = \"%s\",name = [[",
			(UINT)pf->pfn->n_fragments,
			pf->pfn->clusters_total * bytes_per_cluster,
			(UINT)(pf->pfn->is_filtered & 0x1),
			comment
			);
		buffer[sizeof(buffer) - 1] = 0; /* to be sure that the buffer is terminated by zero */
		(void)WriteToReportSavingBuffer(buffer,1,strlen(buffer),f);

		if(RtlUnicodeStringToAnsiString(&as,&pf->pfn->name,TRUE) == STATUS_SUCCESS){
			/* replace square brackets with <> !!! */
			for(i = 0; i < as.Length; i++){
				if(as.Buffer[i] == '[') as.Buffer[i] = '<';
				else if(as.Buffer[i] == ']') as.Buffer[i] = '>';
			}
			
			/* skip \??\ sequence in the beginning */
			if(as.Length > 0x4)
				(void)WriteToReportSavingBuffer((void *)((char *)(as.Buffer) + 0x4),1,as.Length - 0x4,f);
			else
				(void)WriteToReportSavingBuffer(as.Buffer,1,as.Length,f);
			
			RtlFreeAnsiString(&as);
		} else {
			DebugPrint("No enough memory for WriteReportBody()!\n");
			out_of_memory_condition_counter ++;
		}
		(void)strcpy(buffer,"]],uname = {");
		(void)WriteToReportSavingBuffer(buffer,1,strlen(buffer),f);
		p = (unsigned char *)pf->pfn->name.Buffer;
		/* skip \??\ sequence in the beginning */
		if(pf->pfn->name.Length > 0x8) offset = 0x8;
		else offset = 0x0;
		for(i = offset; i < pf->pfn->name.Length; i++){
			(void)_snprintf(buffer,sizeof(buffer),"%u,",(unsigned int)p[i]);
			buffer[sizeof(buffer) - 1] = 0; /* to be sure that the buffer is terminated by zero */
			(void)WriteToReportSavingBuffer(buffer,1,strlen(buffer),f);
		}
		(void)strcpy(buffer,"}},\r\n");
		(void)WriteToReportSavingBuffer(buffer,1,strlen(buffer),f);
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
	f = winx_fopen(path,"w");
	if(f == NULL){
		DebugPrint("Cannot create %s file!\n",path);
		return FALSE;
	}
	
	InitializeReportSavingBuffer();
	
	wcscpy(buffer,L";---------------------------------------------------------------------------------------------\r\n");
	(void)WriteToReportSavingBuffer(buffer,sizeof(short),wcslen(buffer),f);

	(void)_snwprintf(buffer,length,L"; Fragmented files on %hs:\r\n;\r\n",volume_name);
	buffer[length - 1] = 0;
	(void)WriteToReportSavingBuffer(buffer,sizeof(short),wcslen(buffer),f);

	(void)_snwprintf(buffer,length,L"; Fragments%21hs%9hs    Filename\r\n","Filesize","Comment");
	buffer[length - 1] = 0;
	(void)WriteToReportSavingBuffer(buffer,sizeof(short),wcslen(buffer),f);

	wcscpy(buffer,L";---------------------------------------------------------------------------------------------\r\n");
	(void)WriteToReportSavingBuffer(buffer,sizeof(short),wcslen(buffer),f);
	
	WriteTextReportBody(f,FALSE);
	//WriteTextReportBody(f,TRUE);

	DestroyReportSavingBuffer();
	
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
		(void)WriteToReportSavingBuffer(buffer,sizeof(short),wcslen(buffer),f);
		
		/* skip \??\ sequence in the beginning of the path */
		if(pf->pfn->name.Length > 0x8) offset = 0x8;
		else offset = 0x0;
		(void)WriteToReportSavingBuffer((char *)pf->pfn->name.Buffer + offset,1,pf->pfn->name.Length - offset,f);

	next_item:
		pf = pf->next_ptr;
	} while(pf != fragmfileslist);
}

/* ------------------------------------------------------------------------- */
/*                 this code speeds up the report saving                     */
/* ------------------------------------------------------------------------- */

/**
 * @brief The size of the report saving buffer, in bytes.
 */
#define RSB_SIZE (512 * 1024)

/**
 * @brief Pointer to the report saving buffer.
 */
char *rsb = NULL;

/**
 * @brief Offset of the free portion
 * of the report saving buffer.
 */
unsigned int offset = 0;

/**
 * @brief The descriptor of the report file.
 */
WINX_FILE *f_report = NULL;

/**
 * @brief Initializes the report saving buffer.
 * @return Zero for success, negative value otherwise.
 * @note Call this function after opening the file.
 */
static int InitializeReportSavingBuffer(void)
{
	rsb = winx_virtual_alloc(RSB_SIZE);
	if(!rsb){
		DebugPrint("Cannot allocate %u bytes of memory for the report saving buffer!\n",RSB_SIZE);
		DebugPrint("Slower report saving algorithm will be used.\n");
		return (-1);
	}
	RtlZeroMemory(rsb,RSB_SIZE);
	offset = 0;
	f_report = NULL;
	return 0;
}

/**
 * @brief Destroys the report saving buffer.
 * @note Call this function before closing the file.
 */
static void DestroyReportSavingBuffer(void)
{
	if(offset && f_report) winx_fwrite(rsb,1,offset,f_report);
	if(rsb) winx_virtual_free(rsb,RSB_SIZE);
	offset = 0;
	f_report = NULL;
}

/**
 * @brief winx_fwrite equivalent, but writes
 * through the report saving buffer.
 */
static size_t WriteToReportSavingBuffer(const void *buffer,size_t size,size_t count,WINX_FILE *f)
{
	size_t bytes;
	size_t result;
	
	if(rsb == NULL) return winx_fwrite(buffer,size,count,f);
	
	bytes = size * count;
	if(bytes > RSB_SIZE){
		DebugPrint("WriteToReportSavingBuffer(): attempt to write %u bytes failed!\n",(UINT)bytes);
		return 0;
	}
	
	f_report = f;
	result = count;
	
	if(bytes > (RSB_SIZE - offset)){
		winx_fwrite(rsb,1,offset,f);
		offset = 0;
	}
	memcpy(rsb + offset,buffer,bytes);
	offset += bytes;
	if(offset >= RSB_SIZE){
		result = winx_fwrite(rsb,1,RSB_SIZE,f);
		offset = 0;
	}
	return result;
}

/** @} */
