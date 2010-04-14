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
#include <time.h>

#include "wgx.h"

#define MAX_GA_REQUEST_LENGTH 1024
char ga_request[MAX_GA_REQUEST_LENGTH];

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
DWORD WINAPI SendWebAnalyticsRequestThreadProc(LPVOID lpParameter);

/**
 */
static BOOL __stdcall SendWebAnalyticsRequest(char *url)
{
	HMODULE hUrlmonDLL = NULL;
	HRESULT result;
	char path[MAX_PATH + 1];
	
	if(url == NULL){
		OutputDebugString("SendWebAnalyticsRequest: invalid URL (NULL)!\n");
		return FALSE;
	}
	
	OutputDebugString("SendWebAnalyticsRequest: URL = ");
	OutputDebugString(url);
	OutputDebugString("\n");
	
	/* load urlmon.dll library */
	hUrlmonDLL = LoadLibrary("urlmon.dll");
	if(hUrlmonDLL == NULL){
		DbgDisplayLastError("SendWebAnalyticsRequest: LoadLibrary(urlmon.dll) failed! ");
		return FALSE;
	}
	
	/* get an address of procedure downloading a file */
	pURLDownloadToCacheFile = (URLMON_PROCEDURE)GetProcAddress(hUrlmonDLL,"URLDownloadToCacheFileA");
	if(pURLDownloadToCacheFile == NULL){
		DbgDisplayLastError("SendWebAnalyticsRequest: URLDownloadToCacheFile not found in urlmon.dll! ");
		return FALSE;
	}
	
	/* download a file */
	result = pURLDownloadToCacheFile(NULL,url,path,MAX_PATH,0,NULL);
	path[MAX_PATH] = 0;
	if(result != S_OK){
		if(result == E_OUTOFMEMORY) OutputDebugString("Not enough memory for SendWebAnalyticsRequest!");
		else OutputDebugString("SendWebAnalyticsRequest: URLDownloadToCacheFile failed!");
		OutputDebugString("\n");
		return FALSE;
	}
	
	/* remove cached data, otherwise it may not be loaded next time */
	(void)remove(path);
	return TRUE;
}

/**
 */
DWORD WINAPI SendWebAnalyticsRequestThreadProc(LPVOID lpParameter)
{
	(void)SendWebAnalyticsRequest((char *)lpParameter);
	return 0;
}

/**
 */
static void __stdcall SendWebAnalyticsRequestAsynch(char *url)
{
	HANDLE h;
	DWORD id;
	
	h = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)SendWebAnalyticsRequestThreadProc,(void *)url,0,&id);
	if(h == NULL)
		DbgDisplayLastError("Cannot create thread for SendWebAnalyticsRequestAsynch!" );
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

/**
 * @note Based on http://www.vdgraaf.info/google-analytics-without-javascript.html
 * and http://code.google.com/apis/analytics/docs/tracking/gaTrackingTroubleshooting.html
 */
static char * __stdcall build_ga_request(char *hostname,char *path,char *account)
{
	int utmn, utmhid, cookie, random;
	__int64 today;
	
	srand((unsigned int)time(NULL));
	utmn = (rand() << 16) + rand();
	utmhid = (rand() << 16) + rand();
	cookie = (rand() << 16) + rand();
	random = (rand() << 16) + rand();
	today = (__int64)time(NULL);
	
	(void)_snprintf(ga_request,MAX_GA_REQUEST_LENGTH,
		"http://www.google-analytics.com/__utm.gif?utmwv=4.6.5"
		"&utmn=%u"
		"&utmhn=%s"
		"&utmhid=%u"
		"&utmr=-"
		"&utmp=%s"
		"&utmac=%s"
		"&utmcc=__utma%%3D%u.%u.%I64u.%I64u.%I64u.50%%3B%%2B__utmz%%3D%u.%I64u.27.2.utmcsr%%3Dgoogle.com%%7Cutmccn%%3D(referral)%%7Cutmcmd%%3Dreferral%%7Cutmcct%%3D%%2F%3B",
		utmn,hostname,utmhid,path,account,
		cookie,random,today,today,today,cookie,today
		);
	ga_request[MAX_GA_REQUEST_LENGTH - 1] = 0;
	return ga_request;
}

/**
 * @brief Increases a Google Analytics counter
 * by sending the request directly to the service.
 * @param[in] hostname the name of the host.
 * @param[in] path the relative path of the page.
 * @param[in] account the account string.
 * @return Boolean value indicating the status of
 * the operation. TRUE means success, FALSE - failure.
 * @note This function is designed especially to
 * collect statistics about the use of the program,
 * or about the frequency of some error, or about
 * similar things through Google Analytics service.
 *
 * How it works. It sends a request to Google 
 * Analytics service as described here: 
 * http://code.google.com/apis/analytics/docs/
 * (see Troubleshooting section).
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
 * IncreaseGoogleAnalyticsCounter("ultradefrag.sourceforge.net","/appstat/gui.html","UA-13022964-1");
 * @endcode
 * @par Example 2:
 * @code
 * if(cannot_define_windows_version){
 *     // collect statistics about this error frequency
 *     IncreaseGoogleAnalyticsCounter("ultradefrag.sourceforge.net","/appstat/errors/winver.html","UA-13022964-1");
 * }
 * @endcode
 */
BOOL __stdcall IncreaseGoogleAnalyticsCounter(char *hostname,char *path,char *account)
{
	return SendWebAnalyticsRequest(build_ga_request(hostname,path,account));
}

/**
 * @brief An asynchronous equivalent 
 * of IncreaseGoogleAnalyticsCounter.
 * @note Runs in a separate thread.
 */
void __stdcall IncreaseGoogleAnalyticsCounterAsynch(char *hostname,char *path,char *account)
{
	SendWebAnalyticsRequestAsynch(build_ga_request(hostname,path,account));
}

/** @} */
