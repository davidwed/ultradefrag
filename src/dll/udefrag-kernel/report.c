/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* User mode driver - fragmentation report related code.
*/

#include "globals.h"

/*
* Standard Edition of the program will generate lua reports,
* Micro Edition - text reports,
* Portable Edition - both reports.
*/

/* function prototypes */
void RemoveLuaReportFromDisk(char *volume_name);
void RemoveTextReportFromDisk(char *volume_name);
void RemoveHtmlReportFromDisk(char *volume_name);
BOOLEAN SaveLuaReportToDisk(char *volume_name);
BOOLEAN SaveTextReportToDisk(char *volume_name);
void WriteLuaReportBody(WINX_FILE *f,BOOLEAN is_filtered);
void WriteTextReportBody(WINX_FILE *f,BOOLEAN is_filtered);

/* reports cleanup */
void RemoveReportFromDisk(char *volume_name)
{
	RemoveTextReportFromDisk(volume_name);
	RemoveLuaReportFromDisk(volume_name);
	RemoveHtmlReportFromDisk(volume_name);
}

void RemoveLuaReportFromDisk(char *volume_name)
{
	char path[64];
	
	_snprintf(path,64,"\\??\\%s:\\fraglist.luar",volume_name);
	path[63] = 0;
	winx_delete_file(path);
}

void RemoveTextReportFromDisk(char *volume_name)
{
	char path[64];
	
	_snprintf(path,64,"\\??\\%s:\\fraglist.txt",volume_name);
	path[63] = 0;
	winx_delete_file(path);
}

void RemoveHtmlReportFromDisk(char *volume_name)
{
	char path[64];
	
	_snprintf(path,64,"\\??\\%s:\\fraglist.htm",volume_name);
	path[63] = 0;
	winx_delete_file(path);
}

/* reports saving */
BOOLEAN SaveReportToDisk(char *volume_name)
{
#ifndef UDEFRAG_PORTABLE
	#ifdef MICRO_EDITION
	return SaveTextReportToDisk(volume_name);
	#else
	return SaveLuaReportToDisk(volume_name);
	#endif
#else /* UDEFRAG_PORTABLE */
	if(!SaveTextReportToDisk(volume_name)) return FALSE;
	#ifdef MICRO_EDITION
	return TRUE;
	#else
	return SaveLuaReportToDisk(volume_name);
	#endif
#endif
}

BOOLEAN SaveLuaReportToDisk(char *volume_name)
{
	char buffer[512];
	char path[64];
	WINX_FILE *f;
	
	if(disable_reports) return TRUE;

	_snprintf(path,64,"\\??\\%s:\\fraglist.luar",volume_name);
	path[63] = 0;
	f = winx_fopen(path,"w");
	if(f == NULL){
		DebugPrint("Can't create %s file!\n",path);
		return FALSE;
	}
	
	_snprintf(buffer,sizeof(buffer),
		"-- UltraDefrag report for volume %s:\r\n\r\n"
		"volume_letter = \"%s\"\r\n\r\n"
		"files = {\r\n",
		volume_name, volume_name
		);
	buffer[sizeof(buffer) - 1] = 0; /* to be sure that the buffer is terminated by zero */
	winx_fwrite(buffer,1,strlen(buffer),f);

	WriteLuaReportBody(f,FALSE);
	//WriteLuaReportBody(f,TRUE);

	strcpy(buffer,"}\r\n");
	winx_fwrite(buffer,1,strlen(buffer),f);

	winx_fclose(f);

	DebugPrint("-Ultradfg- Report saved to %s\n",path);
	return TRUE;
}

void WriteLuaReportBody(WINX_FILE *f,BOOLEAN is_filtered)
{
	char buffer[256];
	PFRAGMENTED pf;
	char *p;
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
		_snprintf(buffer, sizeof(buffer),
			"\t{fragments = %u,size = %I64u,filtered = %u,"
			"comment = \"%s\",name = [[",
			(UINT)pf->pfn->n_fragments,
			pf->pfn->clusters_total * bytes_per_cluster,
			(UINT)(pf->pfn->is_filtered & 0x1),
			comment
			);
		buffer[sizeof(buffer) - 1] = 0; /* to be sure that the buffer is terminated by zero */
		winx_fwrite(buffer,1,strlen(buffer),f);

		if(RtlUnicodeStringToAnsiString(&as,&pf->pfn->name,TRUE) == STATUS_SUCCESS){
			/* replace square brackets with <> !!! */
			for(i = 0; i < as.Length; i++){
				if(as.Buffer[i] == '[') as.Buffer[i] = '<';
				else if(as.Buffer[i] == ']') as.Buffer[i] = '>';
			}
			
			/* skip \??\ sequence in the beginning */
			if(as.Length > 0x4)
				winx_fwrite((void *)((char *)(as.Buffer) + 0x4),1,as.Length - 0x4,f);
			else
				winx_fwrite(as.Buffer,1,as.Length,f);
			
			RtlFreeAnsiString(&as);
		} else {
			DebugPrint("No enough memory for WriteReportBody()!\n");
		}
		strcpy(buffer,"]],uname = {");
		winx_fwrite(buffer,1,strlen(buffer),f);
		p = (char *)pf->pfn->name.Buffer;
		/* skip \??\ sequence in the beginning */
		if(pf->pfn->name.Length > 0x8) offset = 0x8;
		else offset = 0x0;
		for(i = offset; i < (pf->pfn->name.Length - offset); i++){
			_snprintf(buffer,sizeof(buffer),/*"%03u,"*/"%u,",(UINT)p[i]);
			buffer[sizeof(buffer) - 1] = 0; /* to be sure that the buffer is terminated by zero */
			winx_fwrite(buffer,1,strlen(buffer),f);
		}
		strcpy(buffer,"}},\r\n");
		winx_fwrite(buffer,1,strlen(buffer),f);
	next_item:
		pf = pf->next_ptr;
	} while(pf != fragmfileslist);
}

BOOLEAN SaveTextReportToDisk(char *volume_name)
{
	short unicode_buffer[256];
	//unsigned short line[] = L"\r\n;-----------------------------------------------------------------\r\n\r\n";
	char path[64];
	WINX_FILE *f;
	
	if(disable_reports) return TRUE;

	_snprintf(path,64,"\\??\\%s:\\fraglist.txt",volume_name);
	path[63] = 0;
	f = winx_fopen(path,"w");
	if(f == NULL){
		DebugPrint("Can't create %s file!\n",path);
		return FALSE;
	}
	
	_snwprintf(unicode_buffer,sizeof(unicode_buffer),
		L"\r\nFragmented files on %hs:\r\n\r\n",
		volume_name
		);
	/* to be sure that the buffer is terminated by zero */
	unicode_buffer[sizeof(unicode_buffer)/sizeof(short) - 1] = 0;
	winx_fwrite(unicode_buffer,2,wcslen(unicode_buffer),f);

	WriteTextReportBody(f,FALSE);
	//winx_fwrite(line,2,wcslen(line),f);
	//WriteTextReportBody(f,TRUE);

	winx_fclose(f);
	
	DebugPrint("-Ultradfg- Report saved to %s\n",path);
	return TRUE;
}

void WriteTextReportBody(WINX_FILE *f,BOOLEAN is_filtered)
{
	char buffer[128];
	PFRAGMENTED pf;
	ANSI_STRING as;
	UNICODE_STRING us;
	unsigned short e1[] = L"\t";
	unsigned short e2[] = L"\r\n";

	pf = fragmfileslist; if(!pf) return;
	do {
		if(pf->pfn->is_filtered != is_filtered)
			goto next_item;
		/* because on NT 4.0 we don't have _itow: */
		_itoa(pf->pfn->n_fragments,buffer,10);
		RtlInitAnsiString(&as,buffer);
		if(RtlAnsiStringToUnicodeString(&us,&as,TRUE) != STATUS_SUCCESS){
			DebugPrint("No enough memory for WriteReportBody()!\n");
			return;
		}
		winx_fwrite(us.Buffer,1,us.Length,f);
		RtlFreeUnicodeString(&us);
		winx_fwrite(e1,2,wcslen(e1),f);
		/* skip \??\ sequence in the beginning */
		if(pf->pfn->name.Length > 0x8)
			winx_fwrite((void *)((char *)(pf->pfn->name.Buffer) + 0x8),1,pf->pfn->name.Length - 0x8,f);
		else
			winx_fwrite(pf->pfn->name.Buffer,1,pf->pfn->name.Length,f);
		winx_fwrite(e2,2,wcslen(e2),f);
	next_item:
		pf = pf->next_ptr;
	} while(pf != fragmfileslist);
}
