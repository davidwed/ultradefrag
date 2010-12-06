/*
 *  ZenWINX - WIndows Native eXtended library.
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
 * dbg.c definitions.
 */

#ifndef _DBG_H_
#define _DBG_H_

typedef enum _DBG_LOG_PATH_TYPE {
    DbgLogPathUseExe = 0,           /* uses DBG_LOG_PATH_FMT format string, replaces .exe by .log */
    DbgLogPathWinDir,               /* uses DBG_LOG_PATH_WINDIR_FMT format string, prepends %WinDir% */
    DbgLogPathWinDirAndExe,         /* uses DBG_LOG_PATH_WINDIR_EXE_FMT format string, prepends %WinDir% */
    DbgLogPathUseExeSubDir,         /* uses DBG_LOG_PATH_SUBDIR_FMT format string, prepends exe folder */
} DBG_LOG_PATH_TYPE, *PDBG_LOG_PATH_TYPE;

#define DBG_LOG_PATH_FMT            "%s.log"
/* Sample resulting path for "C:\Windows\UltraDefrag\ultradefrag.exe":
        \??\C:\Windows\UltraDefrag\ultradefrag.log */

#define DBG_LOG_PATH_WINDIR_FMT     "%s\\UltraDefrag\\Logs\\temp_defrag.log"
/* Sample resulting path:
        \??\C:\Windows\UltraDefrag\Logs\temp_defrag.log */

#define DBG_LOG_PATH_WINDIR_EXE_FMT "%s\\UltraDefrag\\Logs\\%s.log"
/* Sample resulting path for "C:\Windows\System32\defrag_native.exe":
        \??\C:\Windows\UltraDefrag\Logs\defrag_native.log */

#define DBG_LOG_PATH_SUBDIR_FMT     "%s\\Logs\\%s.log"
/* Sample resulting path for "C:\Windows\UltraDefrag\ultradefrag.exe":
        \??\C:\Windows\UltraDefrag\Logs\ultradefrag.log */

void __cdecl winx_dbg_print(char *format, ...);
void __cdecl winx_dbg_print_ex(unsigned long status,char *format, ...);
void __cdecl winx_dbg_print_header(char ch, int width, char *format, ...);
void __stdcall winx_enable_dbg_log(DBG_LOG_PATH_TYPE PathType,int CreateFolder);
void __stdcall winx_disable_dbg_log(void);

#endif /* _DBG_H_ */
