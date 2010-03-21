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
* GUI - i18n information.
*/

#include "main.h"

WGX_I18N_RESOURCE_ENTRY i18n_table[] = {
	{IDC_RESCAN,        L"RESCAN_DRIVES",        L"Rescan drives",            NULL},
	{IDC_SKIPREMOVABLE, L"SKIP_REMOVABLE_MEDIA", L"Skip removable media",     NULL},
	{IDC_CL_MAP_STATIC, L"CLUSTER_MAP",          L"Cluster map:",             NULL},
	{IDC_ANALYSE,       L"ANALYSE",              L"Analyze",                  NULL},
	{IDC_DEFRAGM,       L"DEFRAGMENT",           L"Defragment",               NULL},
	{IDC_OPTIMIZE,      L"OPTIMIZE",             L"Optimize",                 NULL},
	{IDC_PAUSE,         L"PAUSE",                L"Pause",                    NULL},
	{IDC_STOP,          L"STOP",                 L"Stop",                     NULL},
	{IDC_SHOWFRAGMENTED,L"REPORT",               L"Report",                   NULL},
	{IDC_SETTINGS,      L"SETTINGS",             L"Settings",                 NULL},
	{IDC_ABOUT,         L"ABOUT",                L"About",                    NULL},

	{0,                 L"VOLUME",               L"Volume",                   NULL},
	{0,                 L"STATUS",               L"Status",                   NULL},
	{0,                 L"FILESYSTEM",           L"File system",              NULL},
	{0,                 L"TOTAL",                L"Total space",              NULL},
	{0,                 L"FREE",                 L"Free space",               NULL},
	{0,                 L"PERCENT",              L"% free",                   NULL},
	
	{0,                 L"STATUS_RUNNING",       L"Executing",                NULL},
	{0,                 L"STATUS_ANALYSED",      L"Analyzed",                 NULL},
	{0,                 L"STATUS_DEFRAGMENTED",  L"Defragmented",             NULL},
	{0,                 L"STATUS_OPTIMIZED",     L"Optimized",                NULL},

	{0,                 L"DIRS",                 L"folders",                  NULL},
	{0,                 L"FILES",                L"files",                    NULL},
	{0,                 L"FRAGMENTED",           L"fragmented",               NULL},
	{0,                 L"COMPRESSED",           L"compressed",               NULL},
	{0,                 L"MFT",                  L"MFT",                      NULL},
	
	{0,                 L"ABOUT_WIN_TITLE",      L"About Ultra Defragmenter", NULL},
	{IDC_CREDITS,       L"CREDITS",              L"Credits",                  NULL},
	{IDC_LICENSE,       L"LICENSE",              L"License",                  NULL},
	
	{IDC_SHUTDOWN,      L"SHUTDOWN_PC_AFTER_A_JOB",  L"Shutdown PC when done",  NULL},
	{0,                 L"HIBERNATE_PC_AFTER_A_JOB", L"Hibernate PC when done", NULL},

	{0,                 L"PLEASE_CONFIRM",             L"Please Confirm",        NULL},
	{IDC_MESSAGE,       L"REALLY_SHUTDOWN_WHEN_DONE",  L"Do you really want to shutdown when done?", NULL},
	{0,                 L"REALLY_HIBERNATE_WHEN_DONE", L"Do you really want to hibernate when done?", NULL},
	{IDC_DELAY_MSG,     L"SECONDS_TILL_SHUTDOWN",      L"seconds till shutdown", NULL},
	{IDC_YES,           L"YES",                        L"Yes",                   NULL},
	{IDC_NO,            L"NO",                         L"No",                    NULL},

	{0,                 NULL,                    NULL,                        NULL}
};

void SetText(HWND hWnd, short *key)
{
	(void)SetWindowTextW(hWnd,WgxGetResourceString(i18n_table,key));
}
