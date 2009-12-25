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

char global_cluster_map[N_BLOCKS];
DWORD thr_id;
BOOL busy_flag = 0;
char current_operation;
BOOL stop_pressed, exit_pressed = FALSE;

extern int shutdown_flag;

DWORD WINAPI ThreadProc(LPVOID);
void StopFailure_Handler(void);
void JobFailure_Handler(void);

void analyse(void)
{
	HANDLE h = create_thread(ThreadProc,(DWORD)'a',&thr_id);
	if(h) CloseHandle(h);
}

void defragment(void)
{
	HANDLE h = create_thread(ThreadProc,(DWORD)'d',&thr_id);
	if(h) CloseHandle(h);
}

void optimize(void)
{
	HANDLE h = create_thread(ThreadProc,(DWORD)'c',&thr_id);
	if(h) CloseHandle(h);
}

/* callback function */
int __stdcall update_stat(int df)
{
	PVOLUME_LIST_ENTRY vl;
	STATISTIC *pst;
	double percentage;
	char progress_msg[32];

	/* due to the following line of code we have obsolete statistics when we have stopped */
	if(stop_pressed) return 0; /* it's neccessary: see comment in main.h file */
	
	vl = VolListGetSelectedEntry();
	if(vl->VolumeName == NULL) return 0;

	pst = &(vl->Statistics);

	if(udefrag_get_progress(pst,&percentage) >= 0){
		UpdateStatusBar(pst);
		current_operation = pst->current_operation;
		if(current_operation)
			sprintf(progress_msg,"%c %u %%",current_operation,(int)percentage);
		else
			sprintf(progress_msg,"A %u %%",(int)percentage);
		SetProgress(progress_msg,(int)percentage);
	}

	if(udefrag_get_map(global_cluster_map,N_BLOCKS) >= 0){
		FillBitMap(global_cluster_map);
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
	PVOLUME_LIST_ENTRY vl;
	UCHAR command;
	int status;
	char letter;

	/* return immediately if we are busy */
	if(busy_flag) return 0;
	busy_flag = 1;
	stop_pressed = FALSE;

	vl = VolListGetSelectedEntry();
	if(vl->VolumeName == NULL){
		busy_flag = 0;
		return 0;
	}

	/* refresh selected volume information (bug #2036873) */
	VolListRefreshSelectedItem();
	
	//VolListUpdateSelectedStatusField(STAT_WORK);
	WgxDisableWindows(hWindow,IDC_ANALYSE,
		IDC_DEFRAGM,IDC_COMPACT,IDC_SHOWFRAGMENTED,
		IDC_RESCAN,IDC_SETTINGS,0);
	WgxEnableWindows(hWindow,IDC_PAUSE,IDC_STOP,0);
	ClearMap();

	/* LONG_PTR cast removes warnings both on mingw and winddk */
	command = (UCHAR)(LONG_PTR)lpParameter;
	letter = vl->VolumeName[0];

	if(command == 'a')
		VolListUpdateSelectedStatusField(STAT_AN);
	else
		VolListUpdateSelectedStatusField(STAT_DFRG);

	ShowProgress();
	SetProgress("A 0 %",0);
	switch(command){
	case 'a':
		status = udefrag_analyse(letter,update_stat);
		break;
	case 'd':
		status = udefrag_defragment(letter,update_stat);
		break;
	default:
		status = udefrag_optimize(letter,update_stat);
	}
	if(status < 0 && !exit_pressed){
		JobFailure_Handler();
		VolListUpdateSelectedStatusField(STAT_CLEAR);
		ClearMap();
	}

	if(!exit_pressed){
		WgxEnableWindows(hWindow,IDC_ANALYSE,
			IDC_DEFRAGM,IDC_COMPACT,IDC_SHOWFRAGMENTED,
			IDC_RESCAN,IDC_SETTINGS,0);
		WgxDisableWindows(hWindow,IDC_PAUSE,IDC_STOP,0);
	}
	
	busy_flag = 0;
//	if(exit_pressed) EndDialog(hWindow,0);
	
	/* check the shutdown after a job box state */
	if(!exit_pressed){
		if(SendMessage(GetDlgItem(hWindow,IDC_SHUTDOWN),
			BM_GETCHECK,0,0) == BST_CHECKED){
				shutdown_flag = TRUE;
				SendMessage(hWindow,WM_CLOSE,0,0);
		}
	}
	
	return 0;
}

void stop(void)
{
	stop_pressed = TRUE;
	if(udefrag_stop() < 0) StopFailure_Handler();
}
