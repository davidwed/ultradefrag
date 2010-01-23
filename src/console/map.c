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

/*
* UltraDefrag console interface - cluster map drawing code.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "../include/udefrag.h"
#include "../include/ultradfgver.h"

#define settextcolor(c) (void)SetConsoleTextAttribute(hOut,c)

#define BLOCKS_PER_HLINE  68//60//79
#define BLOCKS_PER_VLINE  10//8//16
#define N_BLOCKS          (BLOCKS_PER_HLINE * BLOCKS_PER_VLINE)

void display_error(char *string);
extern HANDLE hOut;
extern WORD console_attr;
extern int b_flag;
extern int m_flag;
extern char letter;
extern BOOL stop_flag;

char *cluster_map = NULL;

WORD colors[NUM_OF_SPACE_STATES] = {
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	FOREGROUND_GREEN,
	FOREGROUND_RED | FOREGROUND_INTENSITY,
	FOREGROUND_RED,
	FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_BLUE,
	FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	FOREGROUND_RED | FOREGROUND_GREEN,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	FOREGROUND_RED | FOREGROUND_GREEN,
	FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
};

BOOL map_completed = FALSE;

#define MAP_SYMBOL '%'
#define BORDER_COLOR (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)

short map_border_color = BORDER_COLOR;
char map_symbol = MAP_SYMBOL;
int map_rows = BLOCKS_PER_VLINE;
int map_symbols_per_line = BLOCKS_PER_HLINE;

void DisplayLastError(char *caption);
void clear_line(FILE *f);

void AllocateClusterMap(void)
{
	cluster_map = malloc(map_rows * map_symbols_per_line);
	if(!cluster_map){
		printf("Cannot allocate %i bytes of memory for cluster map!\n\n",
			map_rows * map_symbols_per_line);
		exit(1);
	}
	memset(cluster_map,0,map_rows * map_symbols_per_line);
}

void FreeClusterMap(void)
{
	if(cluster_map) free(cluster_map);
}

void RedrawMap(void)
{
	int i,j;
	WORD color, prev_color = 0x0;
	char c[2];
	WORD border_color = map_border_color;

	fprintf(stderr,"\n\n");
	
	settextcolor(border_color);
	prev_color = border_color;
	c[0] = 0xC9; c[1] = 0;
	fprintf(stderr,c);
	for(j = 0; j < map_symbols_per_line; j++){
		c[0] = 0xCD; c[1] = 0;
		fprintf(stderr,c);
	}
	c[0] = 0xBB; c[1] = 0;
	fprintf(stderr,c);
	fprintf(stderr,"\n");

	for(i = 0; i < map_rows; i++){
		if(border_color != prev_color) settextcolor(border_color);
		prev_color = border_color;
		c[0] = 0xBA; c[1] = 0;
		fprintf(stderr,c);
		for(j = 0; j < map_symbols_per_line; j++){
			color = colors[(int)cluster_map[i * map_symbols_per_line + j]];
			if(color != prev_color) settextcolor(color);
			prev_color = color;
			c[0] = map_symbol; c[1] = 0;
			fprintf(stderr,"%s",c);
		}
		if(border_color != prev_color) settextcolor(border_color);
		prev_color = border_color;
		c[0] = 0xBA; c[1] = 0;
		fprintf(stderr,c);
		fprintf(stderr,"\n");
	}

	if(border_color != prev_color) settextcolor(border_color);
	prev_color = border_color;
	c[0] = 0xC8; c[1] = 0;
	fprintf(stderr,c);
	for(j = 0; j < map_symbols_per_line; j++){
		c[0] = 0xCD; c[1] = 0;
		fprintf(stderr,c);
	}
	c[0] = 0xBC; c[1] = 0;
	fprintf(stderr,c);
	fprintf(stderr,"\n");

	fprintf(stderr,"\n");
	if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	else settextcolor(console_attr);
	map_completed = TRUE;
}

BOOL first_run = TRUE;
BOOL err_flag = FALSE;
BOOL err_flag2 = FALSE;

int __stdcall ProgressCallback(int done_flag)
{
	STATISTIC stat;
	char op; char *op_name = "";
	double percentage;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD cursor_pos;
	
	if(err_flag || err_flag2) return 0;
	
	if(first_run){
		/*
		* Ultra fast ntfs analysis contains one piece of code 
		* that heavily loads CPU for one-two seconds.
		*/
		(void)SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
		first_run = FALSE;
	}
	
	if(map_completed){
		if(!GetConsoleScreenBufferInfo(hOut,&csbi)){
			DisplayLastError("Cannot retrieve cursor position!");
			return 0;
		}
		cursor_pos.X = 0;
		cursor_pos.Y = csbi.dwCursorPosition.Y - map_rows - 3 - 2;
		if(!SetConsoleCursorPosition(hOut,cursor_pos)){
			DisplayLastError("Cannot set cursor position!");
			return 0;
		}
	}
	
	/* this function never raises warnings or errors */
	if(udefrag_get_progress(&stat,&percentage) >= 0){
		op = stat.current_operation;
		if(op == 'A' || op == 'a')      op_name = "analyse:  ";
		else if(op == 'D' || op == 'd') op_name = "defrag:   ";
		else                            op_name = "optimize: ";
		clear_line(stderr);
		fprintf(stderr,"\r%c: %s%3u%% complete, fragmented/total = %lu/%lu",
			letter,op_name,(int)percentage,stat.fragmfilecounter,stat.filecounter);
	} else {
		err_flag = TRUE;
	}
	
	if(err_flag) return 0;

	if(done_flag && !stop_flag){ /* set progress indicator to 100% state */
		clear_line(stderr);
		fprintf(stderr,"\r%c: %s100%% complete, fragmented/total = %lu/%lu",
			letter,op_name,stat.fragmfilecounter,stat.filecounter);
		if(!m_flag) fprintf(stderr,"\n");
	}
	
	if(m_flag){ /* display cluster map */
		if(udefrag_get_map(cluster_map,map_rows * map_symbols_per_line) >= 0)
			RedrawMap();
		else
			err_flag2 = TRUE;
	}
	
	return 0;
}
