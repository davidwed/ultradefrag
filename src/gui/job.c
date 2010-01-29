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
void DisplayLastError(char *caption);
void DisplayDefragError(int error_code,char *caption);
void DisplayStopDefragError(int error_code,char *caption);

void analyse(void)
{
	HANDLE h = create_thread(ThreadProc,(DWORD)'a',&thr_id);
	if(h == NULL)
		DisplayLastError("Cannot create thread starting volume analysis!");
	if(h) CloseHandle(h);
}

void defragment(void)
{
	HANDLE h = create_thread(ThreadProc,(DWORD)'d',&thr_id);
	if(h == NULL)
		DisplayLastError("Cannot create thread starting volume defragmentation!");
	if(h) CloseHandle(h);
}

void optimize(void)
{
	HANDLE h = create_thread(ThreadProc,(DWORD)'c',&thr_id);
	if(h == NULL)
		DisplayLastError("Cannot create thread starting volume optimization!");
	if(h) CloseHandle(h);
}

void DisplayInvalidVolumeError(int error_code)
{
	char buffer[512];

	if(error_code == UDEFRAG_UNKNOWN_ERROR){
		MessageBoxA(NULL,"Volume is missing or some error has been encountered.\n"
		                 "Use DbgView program to get more information.",
		                 "The volume cannot be processed!",MB_OK | MB_ICONHAND);
	} else {
		(void)_snprintf(buffer,sizeof(buffer),"%s\n%s",
				udefrag_get_error_description(error_code),
				"Use DbgView program to get more information.");
		buffer[sizeof(buffer) - 1] = 0;
		MessageBoxA(NULL,buffer,"The volume cannot be processed!",MB_OK | MB_ICONHAND);
	}
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
			(void)sprintf(progress_msg,"%c %u %%",current_operation,(int)percentage);
		else
			(void)sprintf(progress_msg,"A %u %%",(int)percentage);
		SetProgress(progress_msg,(int)percentage);
	}

	if(udefrag_get_map(global_cluster_map,N_BLOCKS) >= 0){
		FillBitMap(global_cluster_map);
		RedrawMap();
	}
	
	if(df == FALSE) return 0;
	if(!stop_pressed){
		(void)sprintf(progress_msg,"%c 100 %%",current_operation);
		SetProgress(progress_msg,100);
	}
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	PVOLUME_LIST_ENTRY vl;
	UCHAR command;
	int error_code;
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
		IDC_DEFRAGM,IDC_OPTIMIZE,IDC_SHOWFRAGMENTED,
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

	/* validate the volume before any processing */
	error_code = udefrag_validate_volume(letter,FALSE);
	if(error_code < 0){
		/* handle error */
		DisplayInvalidVolumeError(error_code);
	} else {
		/* process the volume */
		switch(command){
		case 'a':
			error_code = udefrag_analyse(letter,update_stat);
			break;
		case 'd':
			error_code = udefrag_defragment(letter,update_stat);
			break;
		default:
			error_code = udefrag_optimize(letter,update_stat);
		}
		if(error_code < 0 && !exit_pressed){
			DisplayDefragError(error_code,"Analysis/Defragmentation failed!");
			VolListUpdateSelectedStatusField(STAT_CLEAR);
			ClearMap();
		}
	}
	
	if(!exit_pressed){
		WgxEnableWindows(hWindow,IDC_ANALYSE,
			IDC_DEFRAGM,IDC_OPTIMIZE,IDC_SHOWFRAGMENTED,
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
				(void)SendMessage(hWindow,WM_CLOSE,0,0);
		}
	}
	
	return 0;
}

void stop(void)
{
	int error_code;
	
	stop_pressed = TRUE;
	error_code = udefrag_stop();
	if(error_code < 0)
		DisplayStopDefragError(error_code,
			"Analysis/Defragmentation cannot be stopped!");
}
