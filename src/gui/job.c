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

extern int map_blocks_per_line;
extern int map_lines;

typedef struct _job_parameters {
	char volume_letter;
	udefrag_job_type job_type;
} job_parameters;
	
typedef struct _volume_processing_job {
	job_parameters jp;
	int termination_flag;
	udefrag_progress_info pi;
} volume_processing_job;

/* each volume letter may have a single job assigned */
#define NUMBER_OF_JOBS ('z' - 'a' + 1)

volume_processing_job jobs[NUMBER_OF_JOBS];

/**
 * @brief Initializes structures belonging to all jobs.
 */
void init_jobs(void)
{
	int i;
	
	for(i = 0; i < NUMBER_OF_JOBS; i++){
		memset(&jobs[i],0,sizeof(volume_processing_job));
		jobs[i].pi.completion_status = 1; /* not running */
	}
}

/**
 * @brief Get job assigned to volume letter.
 */
static volume_processing_job *get_job(char volume_letter)
{
	/* validate volume letter */
	volume_letter = (char)tolower((int)volume_letter);
	if(volume_letter < 'a' || volume_letter > 'z')
		return NULL;
	
	return &jobs[volume_letter - 'a'];
}

static void __stdcall _update_progress(udefrag_progress_info *pi, void *p)
{
	volume_processing_job *j = (volume_processing_job *)p;
	
	if(pi == NULL || j == NULL)
		return;
	
	memcpy(&j->pi,pi,sizeof(udefrag_progress_info));
}

static int __stdcall _terminator(void *p)
{
	volume_processing_job *j = (volume_processing_job *)p;
	
	if(j == NULL)
		return 0;
	
	return j->termination_flag;
}

static DWORD WINAPI run_job(LPVOID lpParameter)
{
	job_parameters *jp = (job_parameters *)lpParameter;
	volume_processing_job *j;
	int result;

	if(jp == NULL)
		goto done;
	
	j = get_job(jp->volume_letter);
	if(j == NULL)
		goto done;
	
	/* initialize job */
	memcpy(&j->jp,jp,sizeof(job_parameters));
	j->termination_flag = 0;
	
	/* start the job */
	result = udefrag_start_job(j->jp.volume_letter, 
				j->jp.job_type,	map_blocks_per_line * map_lines,
				_update_progress, _terminator, (void *)j);
	if(result < 0){
		// TODO: display defrag error
	}

done:	
	return 0;
}

/**
 * @brief Starts the volume processing job.
 * @return Zero for success, negative value otherwise.
 */
int start_job(char volume_letter,udefrag_job_type job_type)
{
	volume_processing_job *j = get_job(volume_letter);
	job_parameters jp;
	DWORD id;

	if(j == NULL)
		return (-1);
	
	if(j->pi.completion_status == 0)
		return (-1); /* already running */
	
	/* run job in separate thread */
	jp.volume_letter = volume_letter;
	jp.job_type = job_type;
	if(CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)run_job,(void *)&jp,0,&id) == NULL){
		// TODO: print last error
		return (-1);
	}
	
	return 0;
}

/**
 * @brief Defines whether the job is running or not.
 */
int is_job_running(char volume_letter)
{
	volume_processing_job *j = get_job(volume_letter);
	if(j == NULL)
		return 0;
	return (j->pi.completion_status == 0) ? 1 : 0;
}

/**
 * @brief Stops the running volume processing job.
 */
void stop_job(char volume_letter)
{
	volume_processing_job *j = get_job(volume_letter);
	if(j != NULL)
		j->termination_flag = 1;
}

/** @} */
















































extern HWND hWindow;
extern HWND hList;
extern int shutdown_flag;
extern WGX_I18N_RESOURCE_ENTRY i18n_table[];

extern char *global_cluster_map;
DWORD thr_id;
BOOL busy_flag = 0;
char current_operation;
int stop_pressed, exit_pressed = 0;

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
void __stdcall update_progress(udefrag_progress_info *pi, void *p)
{
	NEW_VOLUME_LIST_ENTRY *v_entry;
	static wchar_t progress_msg[128];
    static wchar_t *ProcessCaption;
    char WindowCaption[256];

	/* due to the following line of code we have obsolete statistics when we have stopped */
	if(stop_pressed) {
        (void)SetWindowText(hWindow, VERSIONINTITLE);

        return; /* it's neccessary: see comment in main.h file */
    }
	
	v_entry = processed_entry;
	if(v_entry == NULL) return;

	memcpy(&v_entry->pi,pi,sizeof(udefrag_progress_info));

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

	if(pi->cluster_map && pi->cluster_map_size == map_blocks_per_line * map_lines){
		memcpy(global_cluster_map,pi->cluster_map,pi->cluster_map_size);
		FillBitMap(global_cluster_map,v_entry);
		RedrawMap(v_entry);
	}
	
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
	NEW_VOLUME_LIST_ENTRY *v_entry;
	LRESULT SelectedItem;
	LV_ITEM lvi;
	char buffer[128];
	int index;
	LONG main_win_style;
	
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
	
	/* make the main window not resizable */
	/* this obvious code fails on x64 editions of Windows */
#ifndef _WIN64
	main_win_style = GetWindowLong(hWindow,GWL_STYLE);
	main_win_style &= ~WS_MAXIMIZEBOX;
	SetWindowLong(hWindow,GWL_STYLE,main_win_style);
	SetWindowPos(hWindow,0,0,0,0,0,SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
#endif
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

	/* make the main window resizable again */
	/* this obvious code fails on x64 editions of Windows */
#ifndef _WIN64
	main_win_style |= WS_MAXIMIZEBOX;
	SetWindowLong(hWindow,GWL_STYLE,main_win_style);
	SetWindowPos(hWindow,0,0,0,0,0,SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
#endif
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
	SetProgress(L"A 0.00 %",0);

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
			error_code = udefrag_start_job(letter, ANALYSIS_JOB,
					map_blocks_per_line * map_lines,
					update_progress, terminator, NULL);
			Status = STATUS_ANALYSED;
			break;
		case 'd':
			error_code = udefrag_start_job(letter, DEFRAG_JOB,
					map_blocks_per_line * map_lines,
					update_progress, terminator, NULL);
			Status = STATUS_DEFRAGMENTED;
			break;
		default:
			error_code = udefrag_start_job(letter, OPTIMIZER_JOB,
					map_blocks_per_line * map_lines,
					update_progress, terminator, NULL);
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
	stop_pressed = 1;
}
