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
 * @file settings.c
 * @brief UltraDefrag settings code.
 * @addtogroup Settings
 * @{
 */

#include "../../include/ntndk.h"

#include "../../include/udefrag.h"
#include "../zenwinx/zenwinx.h"

/*
* http://sourceforge.net/tracker/index.php?func=
* detail&aid=2886353&group_id=199532&atid=969873
*/
ULONGLONG time_limit = 0;
int refresh_interval  = DEFAULT_REFRESH_INTERVAL;

/**
 * @brief Reloads udefrag.dll specific options.
 * @note Internal use only.
 */
void udefrag_reload_settings(void)
{
	#define ENV_BUFFER_LENGTH 128
	short env_buffer[ENV_BUFFER_LENGTH];
	char buf[ENV_BUFFER_LENGTH];
	ULONGLONG i;
	
	/* reset all parameters */
	refresh_interval  = DEFAULT_REFRESH_INTERVAL;
	time_limit = 0;

	if(winx_query_env_variable(L"UD_TIME_LIMIT",env_buffer,ENV_BUFFER_LENGTH) >= 0){
		(void)_snprintf(buf,ENV_BUFFER_LENGTH - 1,"%ws",env_buffer);
		buf[ENV_BUFFER_LENGTH - 1] = 0;
		time_limit = winx_str2time(buf);
	}
	DebugPrint("Time limit = %I64u seconds\n",time_limit);

	if(winx_query_env_variable(L"UD_REFRESH_INTERVAL",env_buffer,ENV_BUFFER_LENGTH) >= 0)
		refresh_interval = _wtoi(env_buffer);
	DebugPrint("Refresh interval = %u msec\n",refresh_interval);
	
	(void)strcpy(buf,"");
	(void)winx_dfbsize(buf,&i); /* to force MinGW export udefrag_dfbsize */
	(void)i;
}

/** @} */
