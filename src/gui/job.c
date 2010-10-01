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
 * @file job.c
 * @brief Volume processing jobs.
 * @addtogroup Job
 * @{
 */

#include "main.h"

/* each volume letter may have a single job assigned */
#define NUMBER_OF_JOBS ('z' - 'a' + 1)

volume_processing_job jobs[NUMBER_OF_JOBS];

/* synchronizes access to internal map representation */
HANDLE hMapEvent = NULL;

/**
 * @brief Initializes structures belonging to all jobs.
 */
int init_jobs(void)
{
	char event_name[64];
	int i;
	
	_snprintf(event_name,64,"udefrag-gui-%u",(int)GetCurrentProcessId());
	event_name[63] = 0;
	hMapEvent = CreateEvent(NULL,FALSE,TRUE,event_name);
	if(hMapEvent == NULL){
		WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,
			"UltraDefrag: cannot create %s event",
			event_name);
		return (-1);
	}
	
	for(i = 0; i < NUMBER_OF_JOBS; i++){
		memset(&jobs[i],0,sizeof(volume_processing_job));
		jobs[i].pi.completion_status = 1; /* not running */
		jobs[i].job_type = NEVER_EXECUTED_JOB;
		jobs[i].volume_letter = 'a' + i;
	}
	return 0;
}

/**
 * @brief Get job assigned to volume letter.
 */
volume_processing_job *get_job(char volume_letter)
{
	/* validate volume letter */
	volume_letter = udefrag_tolower(volume_letter);
	if(volume_letter < 'a' || volume_letter > 'z')
		return NULL;
	
	return &jobs[volume_letter - 'a'];
}

/**
 * @brief Frees resources allocated for all jobs.
 */
void release_jobs(void)
{
	volume_processing_job *j;
	int i;
	
	for(i = 0; i < NUMBER_OF_JOBS; i++){
		j = &jobs[i];
		if(j->map.hdc)
			(void)DeleteDC(j->map.hdc);
		if(j->map.hbitmap)
			(void)DeleteObject(j->map.hbitmap);
		if(j->map.buffer)
			free(j->map.buffer);
		if(j->map.scaled_buffer)
			free(j->map.scaled_buffer);
	}
	if(hMapEvent)
		CloseHandle(hMapEvent);
}







extern int map_blocks_per_line;
extern int map_lines;
extern HWND hWindow;
extern HWND hList;
extern int shutdown_flag;
extern WGX_I18N_RESOURCE_ENTRY i18n_table[];

BOOL busy_flag = 0;
char current_operation;
int stop_pressed, exit_pressed = 0;

volume_processing_job *current_job = NULL;

DWORD WINAPI ThreadProc(LPVOID);
void DisplayDefragError(int error_code,char *caption);
void ProcessSingleVolume(volume_processing_job *job);
void VolListUpdateStatusField(volume_processing_job *job);
void VolListRefreshItem(volume_processing_job *job);

extern HANDLE hMapEvent;

void DoJob(udefrag_job_type job_type)
{
	DWORD id;
	HANDLE h;
	char *action = "analysis";

	h = create_thread(ThreadProc,(LPVOID)(DWORD_PTR)job_type,&id);
	if(h == NULL){
		if(job_type == DEFRAG_JOB)
			action = "defragmentation";
		else if(job_type == OPTIMIZER_JOB)
			action = "optimization";
		WgxDisplayLastError(hWindow,MB_OK | MB_ICONHAND,
			"UltraDefrag: cannot create thread starting volume %s!",
			action);
	} else {
		CloseHandle(h);
	}
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

void DisplayDefragError(int error_code,char *caption)
{
	char buffer[512];
	
	(void)_snprintf(buffer,sizeof(buffer),"%s\n%s",
			udefrag_get_error_description(error_code),
			"Use DbgView program to get more information.");
	buffer[sizeof(buffer) - 1] = 0;
	MessageBoxA(NULL,buffer,caption,MB_OK | MB_ICONHAND);
}

/* callback function */
void __stdcall update_progress(udefrag_progress_info *pi, void *p)
{
	volume_processing_job *job;
	static wchar_t progress_msg[128];
    static wchar_t *ProcessCaption;
    char WindowCaption[256];

	job = current_job;
	if(job == NULL) return;

	memcpy(&job->pi,pi,sizeof(udefrag_progress_info));
	
	VolListUpdateStatusField(job);

	UpdateStatusBar(pi);
	current_operation = pi->current_operation;
	if(current_operation == 'C') current_operation = 'O';
	
	ProcessCaption = WgxGetResourceString(i18n_table,L"ANALYSE");
		
	if(current_operation) {
		switch(current_operation){
			case 'D':
				ProcessCaption = WgxGetResourceString(i18n_table,L"DEFRAGMENT");
				break;
			case 'O':
				ProcessCaption = WgxGetResourceString(i18n_table,L"OPTIMIZE");
		}
	}
		
	_snwprintf(progress_msg,sizeof(progress_msg),L"%ls %6.2lf %%",ProcessCaption,pi->percentage);
	progress_msg[sizeof(progress_msg) - 1] = 0;
	
	SetProgress(progress_msg,(int)pi->percentage);
	
	(void)sprintf(WindowCaption, "UD - %c %6.2lf %%", current_operation, pi->percentage);
	(void)SetWindowText(hWindow, WindowCaption);

	if(WaitForSingleObject(hMapEvent,INFINITE) != WAIT_OBJECT_0){
		WgxDbgPrintLastError("Wait on hMapEvent failed in update_progress");
		return;
	}
	if(pi->cluster_map){
		if(job->map.buffer == NULL || pi->cluster_map_size != job->map.size){
			if(job->map.buffer)
				free(job->map.buffer);
			job->map.buffer = malloc(pi->cluster_map_size);
		}
		if(job->map.buffer == NULL){
			// TODO
			job->map.size = 0;
		} else {
			job->map.size = pi->cluster_map_size;
			memcpy(job->map.buffer,pi->cluster_map,pi->cluster_map_size);
		}
	}
	SetEvent(hMapEvent);

	if(pi->cluster_map && job->map.buffer)
		RedrawMap(job);
	
	if(pi->completion_status == 0) return;
	if(!stop_pressed){
        ProcessCaption = WgxGetResourceString(i18n_table,L"ANALYSE");
            
		if(current_operation) {
            switch(current_operation){
                case 'D':
                    ProcessCaption = WgxGetResourceString(i18n_table,L"DEFRAGMENT");
                    break;
                case 'O':
                    ProcessCaption = WgxGetResourceString(i18n_table,L"OPTIMIZE");
            }
        }
            
		_snwprintf(progress_msg,sizeof(progress_msg),L"%ls 100.00 %%",ProcessCaption);
        progress_msg[sizeof(progress_msg) - 1] = 0;

		SetProgress(progress_msg,100);
        
        (void)SetWindowText(hWindow, VERSIONINTITLE);
	}
	return;
}

int __stdcall terminator(void *p)
{
	return stop_pressed;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	volume_processing_job *job;
	LRESULT SelectedItem;
	LV_ITEM lvi;
	char buffer[128];
	int index;
	udefrag_job_type job_type = (udefrag_job_type)(DWORD_PTR)lpParameter;
	
	/* return immediately if we are busy */
	if(busy_flag) return 0;
	busy_flag = 1;
	stop_pressed = 0;
	
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
			job = get_job(buffer[0]);
			if(job){
				job->job_type = job_type;
				ProcessSingleVolume(job);
			}
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

void ProcessSingleVolume(volume_processing_job *job)
{
	int error_code;

	if(job == NULL) return;

	/* refresh selected volume information (bug #2036873) */
	VolListRefreshItem(job);
	
	//ClearMap();

	ShowProgress();
	SetProgress(L"A 0.00 %",0);

	/* validate the volume before any processing */
	error_code = udefrag_validate_volume(job->volume_letter,FALSE);
	if(error_code < 0){
		/* handle error */
		DisplayInvalidVolumeError(error_code);
	} else {
		/* process the volume */
		current_job = job;
		error_code = udefrag_start_job(job->volume_letter, job->job_type,
				map_blocks_per_line * map_lines,
				update_progress, terminator, NULL);
		if(error_code < 0 && !exit_pressed){
			DisplayDefragError(error_code,"Analysis/Defragmentation failed!");
			//ClearMap();
		}
	}
}

void stop(void)
{
	stop_pressed = 1;
}

/** @} */
