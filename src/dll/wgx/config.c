/*
 *  WGX - Windows GUI Extended Library.
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
 * @file config.c
 * @brief Configuration files I/O code.
 * @addtogroup Config
 * @{
 */

#define WIN32_NO_STATUS
#include <windows.h>

#include "wgx.h"

/* Uses Lua */
#define lua_c
#include "../../lua5.1/lua.h"
#include "../../lua5.1/lauxlib.h"
#include "../../lua5.1/lualib.h"

char err_msg[1024];

/* returns 0 if variable is not defined */
static int getint(lua_State *L, char *variable)
{
	int ret;
	
	lua_getglobal(L, variable);
	ret = (int)lua_tointeger(L, lua_gettop(L));
	lua_pop(L, 1);
	return ret;
}

/**
 * @brief Retrieves a LOGFONT structure from a file.
 * @param[in] path the path to the configuration file.
 * @param[out] lf pointer to the LOGFONT structure.
 * @return Boolean value. TRUE indicates success.
 * @par File contents example:
@verbatim
height = -12
width = 0
escapement = 0
orientation = 0
weight = 400
italic = 0
underline = 0
strikeout = 0
charset = 0
outprecision = 3
clipprecision = 2
quality = 1
pitchandfamily = 34
facename = "Arial Black"
@endverbatim
 */
BOOL __stdcall WgxGetLogFontStructureFromFile(char *path,LOGFONT *lf)
{
	lua_State *L;
	int status;
	char *string;

	L = lua_open();  /* create state */
	if(!L) return FALSE;
	lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
	luaL_openlibs(L);  /* open libraries */
	lua_gc(L, LUA_GCRESTART, 0);

	status = luaL_dofile(L,path);
	if(!status){ /* successful */
		lf->lfHeight = getint(L,"height");
		lf->lfWidth = getint(L,"width");
		lf->lfEscapement = getint(L,"escapement");
		lf->lfOrientation = getint(L,"orientation");
		lf->lfWeight = getint(L,"weight");
		lf->lfItalic = (BYTE)getint(L,"italic");
		lf->lfUnderline = (BYTE)getint(L,"underline");
		lf->lfStrikeOut = (BYTE)getint(L,"strikeout");
		lf->lfCharSet = (BYTE)getint(L,"charset");
		lf->lfOutPrecision = (BYTE)getint(L,"outprecision");
		lf->lfClipPrecision = (BYTE)getint(L,"clipprecision");
		lf->lfQuality = (BYTE)getint(L,"quality");
		lf->lfPitchAndFamily = (BYTE)getint(L,"pitchandfamily");
		lua_getglobal(L, "facename");
		string = (char *)lua_tostring(L, lua_gettop(L));
		if(string){
			(void)strncpy(lf->lfFaceName,string,LF_FACESIZE);
			lf->lfFaceName[LF_FACESIZE - 1] = 0;
		}
		lua_pop(L, 1);
	}
	lua_close(L);
	return TRUE;
}

/**
 * @brief Saves a LOGFONT structure to a file.
 * @param[in] path the path to the configuration file.
 * @param[out] lf pointer to the LOGFONT structure.
 * @return Boolean value. TRUE indicates success.
 * @note This function shows a message box in case when
 * some error has been occured.
 * @bug This function is not thread safe.
 */
BOOL __stdcall WgxSaveLogFontStructureToFile(char *path,LOGFONT *lf)
{
	FILE *pf;
	int result;

	pf = fopen(path,"wt");
	if(!pf){
		(void)_snprintf(err_msg,sizeof(err_msg) - 1,
			"Can't save font preferences to %s!\n%s",
			path,_strerror(NULL));
		err_msg[sizeof(err_msg) - 1] = 0;
		MessageBox(0,err_msg,"Warning!",MB_OK | MB_ICONWARNING);
		return FALSE;
	}

	result = fprintf(pf,
		"height = %li\n"
		"width = %li\n"
		"escapement = %li\n"
		"orientation = %li\n"
		"weight = %li\n"
		"italic = %i\n"
		"underline = %i\n"
		"strikeout = %i\n"
		"charset = %i\n"
		"outprecision = %i\n"
		"clipprecision = %i\n"
		"quality = %i\n"
		"pitchandfamily = %i\n"
		"facename = \"%s\"\n",
		lf->lfHeight,
		lf->lfWidth,
		lf->lfEscapement,
		lf->lfOrientation,
		lf->lfWeight,
		lf->lfItalic,
		lf->lfUnderline,
		lf->lfStrikeOut,
		lf->lfCharSet,
		lf->lfOutPrecision,
		lf->lfClipPrecision,
		lf->lfQuality,
		lf->lfPitchAndFamily,
		lf->lfFaceName
		);
	fclose(pf);
	if(result < 0){
		(void)_snprintf(err_msg,sizeof(err_msg) - 1,
			"Can't write gui preferences to %s!\n%s",
			path,_strerror(NULL));
		err_msg[sizeof(err_msg) - 1] = 0;
		MessageBox(0,err_msg,"Warning!",MB_OK | MB_ICONWARNING);
		return FALSE;
	}
	return TRUE;
}

/** @} */
