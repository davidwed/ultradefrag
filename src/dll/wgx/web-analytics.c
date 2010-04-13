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
 * @file web-analytics.c
 * @brief Web Analytics API.
 * @addtogroup WebAnalytics
 * @{
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <stdio.h>

#include "wgx.h"

typedef HRESULT (__stdcall *URLMON_PROCEDURE)(
	/* LPUNKNOWN */ void *lpUnkcaller,
	LPCSTR szURL,
	LPTSTR szFileName,
	DWORD cchFileName,
	DWORD dwReserved,
	/*IBindStatusCallback*/ void *pBSC
);
URLMON_PROCEDURE pURLDownloadToCacheFile;

void DbgDisplayLastError(char *caption);
DWORD WINAPI IncreaseWebAnalyticsCounterThreadProc(LPVOID lpParameter);

/**
 * @brief Increases a Google Analytics counter
 * by loading the specified page.
 * @param[in] url the address of the page.
 * @return Boolean value indicating the status of
 * the operation. TRUE means success, FALSE - failure.
 * @note This function is designed especially to
 * collect statistics about the use of the program,
 * or about the frequency of some error, or about
 * similar things through Google Analytics service.
 *
 * How it works. It loads the specified page which
 * contains a Google Analytics counter code inside.
 * Thus it translates the frequency of some event
 * inside your application to the frequency of an 
 * access to some predefined webpage. Therefore
 * you will have a handy nice looking report of
 * application events frequencies. And moreover,
 * it will represent statistics globally, collected
 * from all instances of the program.
 *
 * It is not supposed to increase counters of your
 * website though! If you'll try to use it for 
 * illegal purposes, remember - you are forewarned
 * and we have no responsibility for your illegal 
 * actions.
 * @par Example 1:
 * @code
 * // collect statistics about the UltraDefrag GUI client use
 * IncreaseWebAnalyticsCounter("http://ultradefrag.sourceforge.net/appstat/gui.html");
 * @endcode
 * @par Example 2:
 * @code
 * if(cannot_define_windows_version){
 *     // collect statistics about this error frequency
 *     IncreaseWebAnalyticsCounter("http://ultradefrag.sourceforge.net/appstat/errors/winver.html");
 * }
 * @endcode
 */
BOOL __stdcall IncreaseWebAnalyticsCounter(char *url)
{
	HMODULE hUrlmonDLL = NULL;
	HRESULT result;
	char path[MAX_PATH + 1];
	
	/* load urlmon.dll library */
	hUrlmonDLL = LoadLibrary("urlmon.dll");
	if(hUrlmonDLL == NULL){
		DbgDisplayLastError("IncreaseWebAnalyticsCounter: LoadLibrary(urlmon.dll) failed! ");
		return FALSE;
	}
	
	/* get an address of procedure downloading a file */
	pURLDownloadToCacheFile = (URLMON_PROCEDURE)GetProcAddress(hUrlmonDLL,"URLDownloadToCacheFileA");
	if(pURLDownloadToCacheFile == NULL){
		DbgDisplayLastError("IncreaseWebAnalyticsCounter: URLDownloadToCacheFile not found in urlmon.dll! ");
		return FALSE;
	}
	
	/* download a file */
	result = pURLDownloadToCacheFile(NULL,url,path,MAX_PATH,0,NULL);
	path[MAX_PATH] = 0;
	if(result != S_OK){
		if(result == E_OUTOFMEMORY) OutputDebugString("Not enough memory for IncreaseWebAnalyticsCounter!");
		else OutputDebugString("IncreaseWebAnalyticsCounter: URLDownloadToCacheFile failed!");
		OutputDebugString("\n");
		return FALSE;
	}
	
	/* remove cached data, otherwise it may not be loaded next time */
	(void)remove(path);
	return TRUE;
}

/**
 */
DWORD WINAPI IncreaseWebAnalyticsCounterThreadProc(LPVOID lpParameter)
{
	(void)IncreaseWebAnalyticsCounter((char *)lpParameter);
	return 0;
}

/**
 * @brief An asynchronous equivalent 
 * of IncreaseWebAnalyticsCounter.
 * @note Runs in a separate thread.
 */
void __stdcall IncreaseWebAnalyticsCounterAsynch(char *url)
{
	HANDLE h;
	DWORD id;
	
	h = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)IncreaseWebAnalyticsCounterThreadProc,(void *)url,0,&id);
	if(h == NULL)
		DbgDisplayLastError("Cannot create thread for IncreaseWebAnalyticsCounterAsynch!" );
	if(h) CloseHandle(h);
}

/**
 */
void DbgDisplayLastError(char *caption)
{
	LPVOID lpMsgBuf;
	char buffer[128];
	DWORD error = GetLastError();

	OutputDebugString(caption);
	if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,error,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,0,NULL)){
				(void)_snprintf(buffer,sizeof(buffer),
						"Error code = 0x%x",(UINT)error);
				buffer[sizeof(buffer) - 1] = 0;
				OutputDebugString(buffer);
				return;
	} else {
		OutputDebugString((LPCTSTR)lpMsgBuf);
		LocalFree(lpMsgBuf);
	}
	OutputDebugString("\n");
}

/** @} */
