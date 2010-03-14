/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2009-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* Scheduler - i18n information.
*/

#define WIN32_NO_STATUS
#include <windows.h>

#include "resource.h"

#include "../../dll/wgx/wgx.h"

WGX_I18N_RESOURCE_ENTRY i18n_table[] = {
	{IDC_MONDAY,    L"MONDAY",      L"Monday",      NULL},
	{IDC_TUESDAY,   L"TUESDAY",     L"Tuesday",     NULL},
	{IDC_WEDNESDAY, L"WEDNESDAY",   L"Wednesday",   NULL},
	{IDC_THURSDAY,  L"THURSDAY",    L"Thursday",    NULL},
	{IDC_FRIDAY,    L"FRIDAY",      L"Friday",      NULL},
	{IDC_SATURDAY,  L"SATURDAY",    L"Saturday",    NULL},
	{IDC_SUNDAY,    L"SUNDAY",      L"Sunday",      NULL},

	{IDC_DAILY,     L"DAILY",       L"Daily",       NULL},
	{IDC_WEEKLY,    L"WEEKLY",      L"Weekly",      NULL},

	{IDOK,          L"CREATE_TASK", L"Create Task", NULL},

	{0,             NULL,           NULL,           NULL}
};

void SetText(HWND hWnd, short *key)
{
	(void)SetWindowTextW(hWnd,WgxGetResourceString(i18n_table,key));
}
