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
extern HWND hList;
extern int shutdown_flag;

char global_cluster_map[N_BLOCKS];
DWORD thr_id;
BOOL busy_flag = 0;
char current_operation;
BOOL stop_pressed, exit_pressed = FALSE;

NEW_VOLUME_LIST_ENTRY *processed_entry = NULL;

DWORD WINAPI ThreadProc(LPVOID);
void DisplayLastError(char *caption);
void DisplayDefragError(int error_code,char *caption);
void DisplayStopDefragError(int error_code,char *caption);
NEW_VOLUME_LIST_ENTRY * vlist_get_first_selected_entry(void);
NEW_VOLUME_LIST_ENTRY * vlist_get_entry(char *name);
void ProcessSingleVolume(NEW_VOLUME_LIST_ENTRY *v_entry,char command);
void VolListUpdateStatusField(NEW_VOLUME_LIST_ENTRY *v_entry,int status);
void VolListRefreshItem(NEW_VOLUME_LIST_ENTRY *v_entry);
BOOL FillBitMap(char *cluster_map,NEW_VOLUME_LIST_ENTRY *v_entry);

void DoJob(char job_type)
{
	HANDLE h = create_thread(ThreadProc,(LPVOID)(DWORD_PTR)job_type,&thr_id);
	if(h == NULL){
		switch(job_type){
		case 'a':
			DisplayLastError("Cannot create thread starting volume analysis!");
			break;
		case 'd':
			DisplayLastError("Cannot create thread starting volume defragmentation!");
			break;
		default:
			DisplayLastError("Cannot create thread starting volume optimization!");
		}
		
	}
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
	NEW_VOLUME_LIST_ENTRY *v_entry;
	STATISTIC *pst;
	double percentage;
	char progress_msg[32];

	/* due to the following line of code we have obsolete statistics when we have stopped */
	if(stop_pressed) return 0; /* it's neccessary: see comment in main.h file */
	
	v_entry = processed_entry;
	if(v_entry == NULL) return 0;

	pst = &(v_entry->stat);

	if(udefrag_get_progress(pst,&percentage) >= 0){
		UpdateStatusBar(pst);
		current_operation = pst->current_operation;
		if(current_operation)
			(void)sprintf(progress_msg,"%c %6.2lf %%",current_operation,percentage);
		else
			(void)sprintf(progress_msg,"A %6.2lf %%",percentage);
		SetProgress(progress_msg,(int)percentage);
	}

	if(udefrag_get_map(global_cluster_map,N_BLOCKS) >= 0){
		FillBitMap(global_cluster_map,v_entry);
		RedrawMap(v_entry);
	}
	
	if(df == FALSE) return 0;
	if(!stop_pressed){
		(void)sprintf(progress_msg,"%c 100.00 %%",current_operation);
		SetProgress(progress_msg,100);
	}
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	NEW_VOLUME_LIST_ENTRY *v_entry;
	LRESULT SelectedItem;
	LV_ITEM lvi;
	char buffer[128];
	int index;
	
	/* return immediately if we are busy */
	if(busy_flag) return 0;
	busy_flag = 1;
	stop_pressed = FALSE;
	
	/* return immediately if there are no volumes selected */
	if(SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_SELECTED) == -1){
		busy_flag = 0;
		return 0;
	}

	/* disable buttons */
	WgxDisableWindows(hWindow,IDC_ANALYSE,
		IDC_DEFRAGM,IDC_OPTIMIZE,IDC_SHOWFRAGMENTED,
		IDC_RESCAN,IDC_SETTINGS,0);
	WgxEnableWindows(hWindow,IDC_PAUSE,IDC_STOP,0);

	/* process all selected volumes */
	index = -1;
	while(1){
		SelectedItem = SendMessage(hList,LVM_GETNEXTITEM,(WPARAM)index,LVNI_SELECTED);
		if(SelectedItem == -1 || SelectedItem == index) break;
		if(stop_pressed || exit_pressed) break;
		lvi.iItem = (int)SelectedItem;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT;
		lvi.pszText = buffer;
		lvi.cchTextMax = 127;
		if(SendMessage(hList,LVM_GETITEM,0,(LRESULT)&lvi)){
			buffer[2] = 0;
			v_entry = vlist_get_entry(buffer);
			ProcessSingleVolume(v_entry,(char)(LONG_PTR)lpParameter);
		}
		index = (int)SelectedItem;
	}

	/* enable buttons */
	busy_flag = 0;
	if(!exit_pressed){
		WgxEnableWindows(hWindow,IDC_ANALYSE,
			IDC_DEFRAGM,IDC_OPTIMIZE,IDC_SHOWFRAGMENTED,
			IDC_RESCAN,IDC_SETTINGS,0);
		WgxDisableWindows(hWindow,IDC_PAUSE,IDC_STOP,0);
	}

	/* check the shutdown after a job box state */
	if(!exit_pressed && !stop_pressed){
		if(SendMessage(GetDlgItem(hWindow,IDC_SHUTDOWN),
			BM_GETCHECK,0,0) == BST_CHECKED){
				shutdown_flag = TRUE;
				(void)SendMessage(hWindow,WM_CLOSE,0,0);
		}
	}
	return 0;
}

void ProcessSingleVolume(NEW_VOLUME_LIST_ENTRY *v_entry,char command)
{
	char letter;
	int error_code;
	int Status = STATUS_UNDEFINED;

	if(v_entry == NULL) return;

	/* refresh selected volume information (bug #2036873) */
	VolListRefreshItem(v_entry);
	
	ClearMap();
	letter = v_entry->name[0];

	VolListUpdateStatusField(v_entry,STATUS_RUNNING);

	ShowProgress();
	SetProgress("A 0.00 %",0);

	/* validate the volume before any processing */
	error_code = udefrag_validate_volume(letter,FALSE);
	if(error_code < 0){
		/* handle error */
		DisplayInvalidVolumeError(error_code);
	} else {
		/* process the volume */
		processed_entry = v_entry;
		switch(command){
		case 'a':
			error_code = udefrag_analyse(letter,update_stat);
			Status = STATUS_ANALYSED;
			break;
		case 'd':
			error_code = udefrag_defragment(letter,update_stat);
			Status = STATUS_DEFRAGMENTED;
			break;
		default:
			error_code = udefrag_optimize(letter,update_stat);
			Status = STATUS_OPTIMIZED;
		}
		if(error_code < 0 && !exit_pressed){
			DisplayDefragError(error_code,"Analysis/Defragmentation failed!");
			VolListUpdateStatusField(v_entry,STATUS_UNDEFINED);
			ClearMap();
		}
	}
	
	VolListUpdateStatusField(v_entry,Status);
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
