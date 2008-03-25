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
* GUI - i18n functions.
*/

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* TODO: error handling */

struct pair {
	struct pair *next;
	short *name;
	short *value;
};

short line_buffer[8192];
short param_buffer[8192];
short value_buffer[8192];

struct pair *pairs = NULL;

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

void AddResourceEntry()
{
	struct pair *p;
	short first_char = line_buffer[0];
	short *eq_pos;
	int param_len, value_len;
	
	/* skip comments and empty lines */
 	if(first_char == ';' || first_char == '#')
		return;
	eq_pos = wcschr(line_buffer,'=');
	if(!eq_pos) return;
	/* extract a parameter-value pair */
	param_buffer[0] = value_buffer[0] = 0;
	param_len = (int)/*(LONG_PTR)*/(eq_pos - line_buffer);
	value_len = (int)/*(LONG_PTR)*/(line_buffer + wcslen(line_buffer) - eq_pos - 1);
	ExtractToken(param_buffer,line_buffer,param_len);
	ExtractToken(value_buffer,eq_pos + 1,value_len);
	_wcsupr(param_buffer);
	/* allocate pair structure and fill it */
	p = malloc(sizeof(struct pair));
	if(!p) return;
	p->next = pairs;
	pairs = p;
	p->name = malloc((wcslen(param_buffer) + 1) * sizeof(short));
	if(p->name) wcscpy(p->name, param_buffer);
	p->value = malloc((wcslen(value_buffer) + 1) * sizeof(short));
	if(p->value) wcscpy(p->value, value_buffer);
}

void BuildResourceTable(void)
{
	FILE *f = fopen(".\\ud_i18n.lng","rb");
	if(!f) return;
	while(fgetws(line_buffer,sizeof(line_buffer) / sizeof(short),f)){
		line_buffer[sizeof(line_buffer) / sizeof(short) - 1] = 0;
		AddResourceEntry();
	}
	fclose(f);
}

void DestroyResourceTable(void)
{
	struct pair *next, *p = pairs;

	while(p){
		next = p->next;
		if(p->name) free(p->name);
		if(p->value) free(p->value);
		free(p);
		p = next;
	}
}

/* zero for success, negative value otherwise */
int  GetResourceString(short *id,short *buf,int maxchars)
{
	struct pair *p;

	if(!pairs) return -1;
	
	_wcsupr(id);
	for(p = pairs; p != NULL; p = p->next){
		if(wcscmp(id,p->name) == 0){
			wcsncpy(buf,p->value,maxchars);
			buf[maxchars - 1] = 0;
			return 0;
		}
	}
	buf[0] = 0;
	return -1;
}
