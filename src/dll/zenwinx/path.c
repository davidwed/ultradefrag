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

/**
 * @file path.c
 * @brief Path manipulation.
 * @addtogroup Path
 * @{
 */

#include "zenwinx.h"

/**
 * @brief removes the extension from a path.
 * @details If path contains as input "\\??\\C:\\Windows\\Test.txt",
 * then path contains as output "\\??\\C:\\Windows\\Test".
 * If the file name contains no dot or is starting with a dot,
 * then path keeps unchanged.
 * @param[in,out] path the native ANSI path to be processed.
 * @note path must be MAX_PATH characters long.
 */
void __stdcall winx_path_remove_extension(char *path)
{
    char buffer[MAX_PATH + 1] = {0};
    int i;
    
    if(path == NULL || strlen(path) == 0)
        return;
    
    (void)strcpy(buffer,path);

    i = strlen(buffer);
    while(i > 0) {
        if(buffer[i] == '\\')
            break;
        if(buffer[i] == '.'){
            buffer[i] = 0;
            break;
        }
        i--;
    }
    if(i>0 && buffer[i-1] != '\\')
        (void)strcpy(path,buffer);
}

/**
 * @brief removes the file name from a path.
 * @details If path contains as input "\\??\\C:\\Windows\\Test.txt",
 * then path contains as output "\\??\\C:\\Windows".
 * If the path has a trailing backslash, then only that is removed.
 * @param[in,out] path the native ANSI path to be processed.
 * @note path must be MAX_PATH characters long.
 */
void __stdcall winx_path_remove_filename(char *path)
{
    char buffer[MAX_PATH + 1] = {0};
    int i;
    
    if(path == NULL || strlen(path) == 0)
        return;
    
    (void)strcpy(buffer,path);

    i = strlen(buffer);
    while(i > 0) {
        if(buffer[i] == '\\'){
            buffer[i] = 0;
            break;
        }
        i--;
    }
    if(i>0)
        (void)strcpy(path,buffer);
}

/**
 * @brief extracts the file name from a path.
 * @details If path contains as input "\\??\\C:\\Windows\\Test.txt",
 * path contains as output "Test.txt".
 * If path contains as input "\\??\\C:\\Windows\\",
 * path contains as output "Windows\\".
 * @param[in,out] path the native ANSI path to be processed.
 * @note path must be MAX_PATH characters long.
 */
void __stdcall winx_path_extract_filename(char *path)
{
    char buffer[MAX_PATH + 1] = {0};
    int i,j;
    
    if(path == NULL || strlen(path) == 0)
        return;
    
    (void)strcpy(buffer,path);

    i = strlen(buffer);
    while(i > 0) {
        i--;
        if(buffer[i] == '\\'){
            i++;
            break;
        }
    }
    if(i>0){
        j = 0;
        while((path[j] = buffer[i])){
            j++;
            i++;
        }
    }
}

/**
 * @brief Gets the fully quallified path of the current module.
 * @details This routine is the native equivalent of GetModuleFileName.
 * @param[out] path receives the native path of the current executable.
 * @note path must be MAX_PATH characters long.
 */
void __stdcall winx_get_module_filename(char *path)
{
	NTSTATUS Status;
    PROCESS_BASIC_INFORMATION ProcessInformation;
    ANSI_STRING as = {0};
    
    if(path == NULL)
        return;
    
    path[0] = 0;
    
    /* retrieve process executable path */
    RtlZeroMemory(&ProcessInformation,sizeof(ProcessInformation));
    Status = NtQueryInformationProcess(NtCurrentProcess(),
                    ProcessBasicInformation,&ProcessInformation,
                    sizeof(ProcessInformation),
                    NULL);
    /* extract path and file name */
    if(NT_SUCCESS(Status)){
        /* convert Unicode path to ANSI */
        if(RtlUnicodeStringToAnsiString(&as,&ProcessInformation.PebBaseAddress->ProcessParameters->ImagePathName,TRUE) == STATUS_SUCCESS){
            /* avoid buffer overflow */
            if(as.Length < (MAX_PATH-4)){
                /* add native path prefix to path */
                (void)strcpy(path,"\\??\\");
                (void)strcat(path,as.Buffer);
            } else {
                DebugPrint("winx_get_module_filename: path is too long");
            }
            RtlFreeAnsiString(&as);
        } else {
            DebugPrint("winx_get_module_filename: cannot convert unicode to ansi path: not enough memory");
        }
    } else {
        DebugPrint("winx_get_module_filename: cannot query process basic information");
    }
}

/** @} */
