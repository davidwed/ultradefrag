/*
 *  WGX - Windows GUI Extended Library.
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
 * @file i18n.c
 * @brief Internationalization.
 * @addtogroup Internationalization
 * @{
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "wgx.h"

void ExtractToken(short *dest, short *src, int max_chars)
{
	signed int i,cnt;
	short ch;
	
	cnt = 0;
	for(i = 0; i < max_chars; i++){
		ch = src[i];
		/* skip spaces and tabs in the beginning */
		if((ch != 0x20 && ch != '\t') || cnt){
			dest[cnt] = ch;
			cnt++;
		}
	}
	dest[cnt] = 0;
	/* remove spaces, tabs and \r\n from the end */
	if(cnt == 0) return;
	for(i = (cnt - 1); i >= 0; i--){
		ch = dest[i];
		if(ch != 0x20 && ch != '\t' && ch != '\r' && ch != '\n') break;
		dest[i] = 0;
	}
}

void AddResourceEntry(PWGX_I18N_RESOURCE_ENTRY table,short *line_buffer)
{
	short first_char = line_buffer[0];
	short *eq_pos;
	int param_len, value_len;
	int i;
	short *param_buffer;
	short *value_buffer;

	/* skip comments and empty lines */
 	if(first_char == ';' || first_char == '#')
		return;
	eq_pos = wcschr(line_buffer,'=');
	if(!eq_pos) return;

	param_buffer = malloc(8192 * sizeof(short));
	if(!param_buffer) return;
	value_buffer = malloc(8192 * sizeof(short));
	if(!value_buffer){ free(param_buffer); return; }

	/* extract a parameter-value pair */
	param_buffer[0] = value_buffer[0] = 0;
	param_len = (int)/*(LONG_PTR)*/(eq_pos - line_buffer);
	value_len = (int)/*(LONG_PTR)*/(line_buffer + wcslen(line_buffer) - eq_pos - 1);
	ExtractToken(param_buffer,line_buffer,param_len);
	ExtractToken(value_buffer,eq_pos + 1,value_len);
	(void)_wcsupr(param_buffer);

	/* search for table entry */
	for(i = 0;; i++){
		if(table[i].Key == NULL) break;
		if(!wcscmp(table[i].Key,param_buffer)){
			table[i].LoadedString = malloc((wcslen(value_buffer) + 1) * sizeof(short));
			if(table[i].LoadedString) (void)wcscpy(table[i].LoadedString, value_buffer);
			/* break; // the same text may be used for few GUI controls */
		}
	}
	free(param_buffer); free(value_buffer);
}

/**
 * @brief Adds localization strings from the file to the table.
 * @param[in,out] table pointer to the i18n table.
 * @param[in] lng_file_path the path of the i18n file.
 * @return Boolean value. TRUE indicates success.
 * @note All lines in i18n file must be no longer than 8189 characters.
 * Otherwise they will be truncated before an analysis.
 */
BOOL __stdcall WgxBuildResourceTable(PWGX_I18N_RESOURCE_ENTRY table,short *lng_file_path)
{
	FILE *f;
	short *line_buffer;
	
	/* parameters validation */
	if(!table || !lng_file_path) return FALSE;

	line_buffer = malloc(8192 * sizeof(short));
	if(!line_buffer) return FALSE;
	
	/* open lng file */
	f = _wfopen(lng_file_path,L"rb"); /* binary mode required! */
	if(!f){ free(line_buffer); return FALSE; }

	/* read lines and applies them to specified table */
	while(fgetws(line_buffer,8192,f)){
		line_buffer[8192 - 1] = 0;
		AddResourceEntry(table,line_buffer);
	}
	fclose(f); free(line_buffer);
	return TRUE;
}

/**
 * @brief Applies a i18n table to the dialog window.
 * @param[in] table pointer to the i18n table.
 * @param[in] hWindow handle to the window.
 */
void __stdcall WgxApplyResourceTable(PWGX_I18N_RESOURCE_ENTRY table,HWND hWindow)
{
	int i;
	HWND hChild;

	if(!table || !hWindow) return;
	
	for(i = 0;; i++){
		if(table[i].Key == NULL) break;
		hChild = GetDlgItem(hWindow,table[i].ControlID);
		if(table[i].LoadedString) (void)SetWindowTextW(hChild,table[i].LoadedString);
		else (void)SetWindowTextW(hChild,table[i].DefaultString);
	}
}

/**
 * @brief Retrieves a localization string.
 * @param[in] table pointer to the i18n table.
 * @param[in] key pointer to the key string.
 * @return A pointer to the localized string if
 * available or to the default string otherwise.
 */
short * __stdcall WgxGetResourceString(PWGX_I18N_RESOURCE_ENTRY table,short *key)
{
	int i;
	
	if(!table || !key) return NULL;
	
	for(i = 0;; i++){
		if(table[i].Key == NULL) break;
		if(!wcscmp(table[i].Key,key)){
			if(table[i].LoadedString) return table[i].LoadedString;
			return table[i].DefaultString;
		}
	}
	return NULL;
}

/**
 * @brief Destroys an i18n table.
 * @param[in] table pointer to the i18n table.
 */
void __stdcall WgxDestroyResourceTable(PWGX_I18N_RESOURCE_ENTRY table)
{
	int i;
	
	if(!table) return;
	
	for(i = 0;; i++){
		if(table[i].Key == NULL) break;
		if(table[i].LoadedString) free(table[i].LoadedString);
	}
}

/** @} */
