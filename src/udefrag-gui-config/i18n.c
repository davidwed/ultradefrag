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
* GUI Configurator - i18n information.
*/

#define WIN32_NO_STATUS
#include <windows.h>

#include "resource.h"

#include "../dll/wgx/wgx.h"

WGX_I18N_RESOURCE_ENTRY i18n_table[] = {
	{IDC_GUI,           L"GRAPHICAL_INTERFACE", L"Graphical interface",    NULL},
	{IDC_FONT,          L"FONT",                L"Font",                   NULL},
	{IDC_GUI_SCRIPT,    L"OPTIONS",             L"Options",                NULL},
	{IDC_GUI_HELP,      L"HELP",                L"Help",                   NULL},
	{IDC_BOOT_TIME,     L"BOOT_TIME_SCAN",      L"Windows boot time scan", NULL},
	{IDC_ENABLE,        L"ENABLE",              L"Enable",                 NULL},
	{IDC_BOOT_SCRIPT,   L"SCRIPT",              L"Script",                 NULL},
	{IDC_BOOT_HELP,     L"HELP",                L"Help",                   NULL},
	{IDC_REPORTS,       L"REPORTS",             L"Reports",                NULL},
	{IDC_REPORT_OPTIONS,L"OPTIONS",             L"Options",                NULL},
	{IDC_REPORT_HELP,   L"HELP",                L"Help",                   NULL},

	{0,                 NULL,                   NULL,                      NULL}
};

void SetText(HWND hWnd, short *key)
{
	SetWindowTextW(hWnd,WgxGetResourceString(i18n_table,key));
}
