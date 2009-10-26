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
* User mode driver - options initialization routine.
*/

#include "globals.h"

void InitializeOptions(void)
{
	short *env_buffer;
	#define ENV_BUFFER_SIZE 8192
	
	/* allocate memory */
	env_buffer = winx_virtual_alloc(ENV_BUFFER_SIZE * sizeof(short));
	if(!env_buffer){
		winx_dbg_print("Cannot allocate %u bytes for InitializeOptions()\n",
			ENV_BUFFER_SIZE * sizeof(short));
		return;
	}

	/* reset options */
	disable_reports = FALSE;
	dbgprint_level = DBG_NORMAL;
	
	/* read program's options from environment variables */
	if(winx_query_env_variable(L"UD_DISABLE_REPORTS",env_buffer,ENV_BUFFER_SIZE) >= 0) {
		if(!wcscmp(env_buffer,L"1")) disable_reports = TRUE;
	}
	if(winx_query_env_variable(L"UD_DBGPRINT_LEVEL",env_buffer,ENV_BUFFER_SIZE) >= 0){
		_wcsupr(env_buffer);
		if(!wcscmp(env_buffer,L"DETAILED"))
			dbgprint_level = DBG_DETAILED;
		else if(!wcscmp(env_buffer,L"PARANOID"))
			dbgprint_level = DBG_PARANOID;
		else if(!wcscmp(env_buffer,L"NORMAL"))
			dbgprint_level = DBG_NORMAL;
	}
	
	/* initialize filter */
	InitializeFilter();
	
	/* print options */
	if(disable_reports)
		DebugPrint("Disable reports: YES\n");
	else
		DebugPrint("Disable reports: NO\n");
	DebugPrint("dbg_level=%u\n",dbgprint_level);
	
	/* free memory */
	winx_virtual_free(env_buffer,ENV_BUFFER_SIZE * sizeof(short));
}
