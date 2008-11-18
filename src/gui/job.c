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
* GUI - analyse/defrag job code.
*/

#include "main.h"

extern HWND hWindow;
extern char map[MAX_DOS_DRIVES][N_BLOCKS];

DWORD thr_id;
BOOL busy_flag = 0;
char current_operation;
BOOL stop_pressed, exit_pressed = FALSE;
STATISTIC stat[MAX_DOS_DRIVES];

DWORD WINAPI ThreadProc(LPVOID);

void analyse(void)
{
	create_thread(ThreadProc,(DWORD)'a',&thr_id);
}

void defragment(void)
{
	create_thread(ThreadProc,(DWORD)'d',&thr_id);
}

void optimize(void)
{
	create_thread(ThreadProc,(DWORD)'c',&thr_id);
}

/* callback function */
int __stdcall update_stat(int df)
{
	LRESULT iItem;
	int index;
	char *cl_map;
	STATISTIC *pst;
	double percentage;
	char progress_msg[32];

	if(stop_pressed) return 0; /* it's neccessary: see above one comment in Stop() */
	iItem = VolListGetSelectedItemIndex();
	if(iItem == -1) return 0;
	index = VolListGetLetterNumber(iItem);
	cl_map = map[index];
	pst = &stat[index];
	if(udefrag_get_progress(pst,&percentage) >= 0){
		UpdateStatusBar(&stat[index]);
		sprintf(progress_msg,"%c %u %%",
			current_operation = pst->current_operation,(int)percentage);
		SetProgress(progress_msg,(int)percentage);
	}

	if(udefrag_get_map(cl_map,N_BLOCKS) >= 0){
		FillBitMap(index);
		RedrawMap();
	}
	if(df == FALSE) return 0;
	if(!stop_pressed){
		sprintf(progress_msg,"%c 100 %%",current_operation);
		SetProgress(progress_msg,100);
	}
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	LRESULT iItem;
	UCHAR command;
	int status;

	/* return immediately if we are busy */
	if(busy_flag) return 0;
	busy_flag = 1;
	stop_pressed = FALSE;

	iItem = VolListGetSelectedItemIndex();
	if(iItem == -1){
		busy_flag = 0;
		return 0;
	}

	//VolListUpdateStatusField(STAT_WORK,iItem);
	DisableButtonsBeforeTask();
	ClearMap();

	/* LONG_PTR cast removes warnings both on mingw and winddk */
	command = (UCHAR)(LONG_PTR)lpParameter;

	if(command == 'a')
		VolListUpdateStatusField(STAT_AN,iItem);
	else
		VolListUpdateStatusField(STAT_DFRG,iItem);

	ShowProgress();
	SetProgress("0 %",0);
	switch(command){
	case 'a':
		status = udefrag_analyse((UCHAR)VolListGetLetter(iItem),update_stat);
		break;
	case 'd':
		status = udefrag_defragment((UCHAR)VolListGetLetter(iItem),update_stat);
		break;
	default:
		status = udefrag_optimize((UCHAR)VolListGetLetter(iItem),update_stat);
	}
	if(status < 0){
		VolListUpdateStatusField(STAT_CLEAR,iItem);
		ClearMap();
	}
	EnableButtonsAfterTask();
	busy_flag = 0;
	if(exit_pressed) EndDialog(hWindow,0);
	return 0;
}

void stop(void)
{
	stop_pressed = TRUE;
	udefrag_stop();
}
