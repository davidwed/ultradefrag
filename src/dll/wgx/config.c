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
 * @brief Configuration.
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

/**
 * @brief Reads options from configuration file.
 * @details Look at WGX_OPTION structure definition
 * in wgx.h file for details. Configuration files
 * must be written in Lua language to be accepted.
 * @return TRUE indicates that configuration file
 * has been successfully processed.
 * @note 
 * - NULL can be passed as first parameter to
 * initialize options by their default values.
 * - Option name equal to NULL inicates the end of the table.
 * - If option type is WGX_CFG_EMPTY, an empty string
 * will be inserted on WgxSaveOptions call. All other
 * fields of WGX_OPTION structure are ignored.
 * - If option type is WGX_CFG_COMMENT, option name
 * is used as a body of comment, all other fields are ignored.
 * - If option type is WGX_CFG_INT value buffer must point to
 * variable of type int. Default value is interpreted as
 * integer, not as pointer. value_length field is ignored.
 * - If otion type is WGX_CFG_STING, all fields of the structure
 * have an obvious meaning.
 */
BOOL __stdcall WgxGetOptions(char *config_file_path,WGX_OPTION *opts_table)
{
	lua_State *L;
	int status;
	int i;
	char *s;
	
	if(opts_table == NULL)
		return FALSE;
	
	/* set defaults */
	for(i = 0; opts_table[i].name; i++){
		if(opts_table[i].type == WGX_CFG_INT){
			/* copy default value to the buffer */
			*((int *)opts_table[i].value) = (int)(DWORD_PTR)opts_table[i].default_value;
		} else if(opts_table[i].type == WGX_CFG_STRING){
			/* copy default string to the buffer */
			strncpy((char *)opts_table[i].value,
				(char *)opts_table[i].default_value,
				opts_table[i].value_length);
			((char *)opts_table[i].value)[opts_table[i].value_length - 1] = 0;
		}
	}
	
	if(config_file_path == NULL)
		return TRUE; /* nothing to update */
	
	L = lua_open();  /* create state */
	if(L == NULL){
		WgxDbgPrint("WgxGetOptions: cannot initialize Lua library\n");
		return FALSE;
	}
	
	lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
	luaL_openlibs(L);  /* open libraries */
	lua_gc(L, LUA_GCRESTART, 0);

	status = luaL_dofile(L,config_file_path);
	if(status != 0){
		WgxDbgPrint("WgxGetOptions: cannot interprete %s\n",config_file_path);
		lua_close(L);
		return FALSE;
	}
	
	/* search for variables */
	for(i = 0; opts_table[i].name; i++){
		lua_getglobal(L, opts_table[i].name);
		if(!lua_isnil(L, lua_gettop(L))){
			if(opts_table[i].type == WGX_CFG_INT){
				*((int *)opts_table[i].value) = (int)lua_tointeger(L, lua_gettop(L));
			} else if(opts_table[i].type == WGX_CFG_STRING){
				s = (char *)lua_tostring(L, lua_gettop(L));
				if(s != NULL){
					strncpy((char *)opts_table[i].value,s,opts_table[i].value_length);
					((char *)opts_table[i].value)[opts_table[i].value_length - 1] = 0;
				} else {
					strcpy((char *)opts_table[i].value,"");
				}
			}
		}
		/* important: remove received variable from stack */
		lua_pop(L, 1);
	}
	
	/*for(i = 0; opts_table[i].name; i++){
		if(opts_table[i].type == WGX_CFG_EMPTY){
			WgxDbgPrint("\n");
		} else if(opts_table[i].type == WGX_CFG_COMMENT){
			WgxDbgPrint("-- %s\n",opts_table[i].name);
		} else if(opts_table[i].type == WGX_CFG_INT){
			WgxDbgPrint("%s = %i\n",opts_table[i].name,*((int *)opts_table[i].value));
		} else if(opts_table[i].type == WGX_CFG_STRING){
			WgxDbgPrint("%s = \"%s\"\n",opts_table[i].name,(char *)opts_table[i].value);
		}
	}*/
	
	/* cleanup */
	lua_close(L);
	return TRUE;
}

/**
 * @brief Saves options to configuration file.
 * @details Look at WGX_OPTION structure definition
 * in wgx.h file for details. Configuration files
 * produced by this routine are written in Lua language.
 */
BOOL __stdcall WgxSaveOptions(char *config_file_path,WGX_OPTION *opts_table,WGX_SAVE_OPTIONS_CALLBACK cb)
{
	char err_msg[1024];
	FILE *f;
	int i, result = 0;
	
	if(config_file_path == NULL || opts_table == NULL){
		if(cb != NULL)
			cb("WgxSaveOptions: invalid parameter");
		return FALSE;
	}
	
	f = fopen(config_file_path,"wt");
	if(f == NULL){
		(void)_snprintf(err_msg,sizeof(err_msg) - 1,
			"Cannot open %s file:\n%s",
			config_file_path,_strerror(NULL));
		err_msg[sizeof(err_msg) - 1] = 0;
		WgxDbgPrint("%s\n",err_msg);
		if(cb != NULL)
			cb(err_msg);
		return FALSE;
	}

	for(i = 0; opts_table[i].name; i++){
		if(opts_table[i].type == WGX_CFG_EMPTY){
			//WgxDbgPrint("\n");
			result = fprintf(f,"\n");
		} else if(opts_table[i].type == WGX_CFG_COMMENT){
			//WgxDbgPrint("-- %s\n",opts_table[i].name);
			result = fprintf(f,"-- %s\n",opts_table[i].name);
		} else if(opts_table[i].type == WGX_CFG_INT){
			//WgxDbgPrint("%s = %i\n",opts_table[i].name,*((int *)opts_table[i].value));
			result = fprintf(f,"%s = %i\n",opts_table[i].name,*((int *)opts_table[i].value));
		} else if(opts_table[i].type == WGX_CFG_STRING){
			//WgxDbgPrint("%s = \"%s\"\n",opts_table[i].name,(char *)opts_table[i].value);
			result = fprintf(f,"%s = \"%s\"\n",opts_table[i].name,(char *)opts_table[i].value);
		}
		if(result < 0){
			fclose(f);
			(void)_snprintf(err_msg,sizeof(err_msg) - 1,
				"Cannot write to %s file:\n%s",
				config_file_path,_strerror(NULL));
			err_msg[sizeof(err_msg) - 1] = 0;
			WgxDbgPrint("%s\n",err_msg);
			if(cb != NULL)
				cb(err_msg);
			return FALSE;
		}
	}

	/* cleanup */
	fclose(f);
	return TRUE;
}

/** @} */
