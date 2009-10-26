/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2009 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* User mode driver - global objects and ancillary routines.
*/

#include "globals.h"

int nt4_system = 0;
int w2k_system = 0;

ULONG disable_reports = FALSE;
ULONG dbgprint_level = DBG_NORMAL;

BOOLEAN context_menu_handler = FALSE;

HANDLE hSynchEvent = NULL;
HANDLE hStopEvent = NULL;

STATISTIC Stat;

void InitSynchObjects(void);
void DestroySynchObjects(void);

void InitDriverResources(void)
{
	int os_version;
	int mj, mn;
	
	/* print driver version */
	winx_dbg_print("------------------------------------------------------------\n");
	DebugPrint("%s\n",VERSIONINTITLE);

	/* get windows version */
	os_version = winx_get_os_version();
	mj = os_version / 10;
	mn = os_version % 10;
	/* FIXME: windows 9x? */
	DebugPrint("Windows NT %u.%u\n",mj,mn);
	/*dx->xp_compatible = (((mj << 6) + mn) > ((5 << 6) + 0));*/
	nt4_system = (mj == 4) ? 1 : 0;
	w2k_system = (os_version == 50) ? 1 : 0;
	
	InitSynchObjects();
	
	memset(&Stat,0,sizeof(STATISTIC));
	
	DebugPrint("User mode driver loaded successfully\n");
}

void FreeDriverResources(void)
{
	FreeMap();
	DestroyFilter();
	DestroySynchObjects();
}

void __stdcall InitSynchObjectsErrorHandler(short *msg)
{
	winx_dbg_print("------------------------------------------------------------\n");
	winx_dbg_print("%ws\n",msg);
	winx_dbg_print("------------------------------------------------------------\n");
}

void InitSynchObjects(void)
{
	ERRORHANDLERPROC eh;
	eh = winx_set_error_handler(InitSynchObjectsErrorHandler);
	winx_create_event(L"\\udefrag_synch_event",
		SynchronizationEvent,&hSynchEvent);
	winx_create_event(L"\\udefrag_stop_event",
		NotificationEvent,&hStopEvent);
	winx_set_error_handler(eh);
	
	if(hSynchEvent) NtSetEvent(hSynchEvent,NULL);
	if(hStopEvent) NtClearEvent(hStopEvent);
}

int CheckForSynchObjects(void)
{
	if(!hSynchEvent || !hStopEvent) return (-1);
	return 0;
}

void DestroySynchObjects(void)
{
	winx_destroy_event(hSynchEvent);
}
