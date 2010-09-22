/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* GUI - routines designed to check for the latest release.
*/

#include "main.h"
#include "../include/ultradfgver.h"

extern HWND hWindow;

#define VERSION_URL "http://ultradefrag.sourceforge.net/version.ini"
char version_ini_path[MAX_PATH + 1];
#define MAX_VERSION_FILE_LEN 32
char version_number[MAX_VERSION_FILE_LEN + 1];
char error_msg[256];
#define MAX_ANNOUNCEMENT_LEN 128
short announcement[MAX_ANNOUNCEMENT_LEN];

int disable_latest_version_check = 0;

typedef HRESULT (__stdcall *URLMON_PROCEDURE)(
	/* LPUNKNOWN */ void *lpUnkcaller,
	LPCSTR szURL,
	LPTSTR szFileName,
	DWORD cchFileName,
	DWORD dwReserved,
	/*IBindStatusCallback*/ void *pBSC
);
URLMON_PROCEDURE pURLDownloadToCacheFile;

DWORD WINAPI CheckForTheNewVersionThreadProc(LPVOID lpParameter);

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
* @brief Retrieves the latest version number from the project's website.
* @return Version number string. NULL indicates failure.
* @note Based on http://msdn.microsoft.com/en-us/library/ms775122(VS.85).aspx
*/
char *GetLatestVersion(void)
{
	HMODULE hUrlmonDLL = NULL;
	HRESULT result;
	FILE *f;
	int res;
	int i;
	
	/* load urlmon.dll library */
	hUrlmonDLL = LoadLibrary("urlmon.dll");
	if(hUrlmonDLL == NULL){
		DbgDisplayLastError("GetLatestVersion: LoadLibrary(urlmon.dll) failed! ");
		return NULL;
	}
	
	/* get an address of procedure downloading a file */
	pURLDownloadToCacheFile = (URLMON_PROCEDURE)GetProcAddress(hUrlmonDLL,"URLDownloadToCacheFileA");
	if(pURLDownloadToCacheFile == NULL){
		DbgDisplayLastError("GetLatestVersion: URLDownloadToCacheFile not found in urlmon.dll! ");
		return NULL;
	}
	
	/* download a file */
	result = pURLDownloadToCacheFile(NULL,VERSION_URL,version_ini_path,MAX_PATH,0,NULL);
	version_ini_path[MAX_PATH] = 0;
	if(result != S_OK){
		if(result == E_OUTOFMEMORY) OutputDebugString("GetLatestVersion: Not enough memory for URLDownloadToCacheFile!");
		else OutputDebugString("GetLatestVersion: URLDownloadToCacheFile failed!");
		OutputDebugString("\n");
		return NULL;
	}
	
	/* open the file */
	f = fopen(version_ini_path,"rb");
	if(f == NULL){
		(void)_snprintf(error_msg,sizeof(error_msg) - 1,
			"Cannot open %s! %s\n",
			version_ini_path,_strerror(NULL));
		error_msg[sizeof(error_msg) - 1] = 0;
		OutputDebugString(error_msg);
		return NULL;
	}
	
	/* read version string */
	res = fread(version_number,1,MAX_VERSION_FILE_LEN,f);
	(void)fclose(f);
	/* remove cached data, otherwise it may not be loaded next time */
	(void)remove(version_ini_path);
	if(res == 0){
		OutputDebugString("GetLatestVersion: version.ini file reading failed! ");
		OutputDebugString(version_ini_path);
		if(feof(f)) OutputDebugString(" File seems to be empty.");
		OutputDebugString("\n");
		return NULL;
	}
	
	/* remove trailing \r \n characters if they exists */
	version_number[res] = 0;
	for(i = res - 1; i >= 0; i--){
		if(version_number[i] != '\r' && version_number[i] != '\n') break;
		version_number[i] = 0;
	}
	
	return version_number;
}

/**
 * @brief Defines whether new version is available for download or not.
 * @return A string containing an announcement. NULL indicates that
 * there is no new version available.
 */
short *GetNewVersionAnnouncement(void)
{
	char *lv;
	char *cv = VERSIONINTITLE;
	int lmj, lmn, li; /* latest version numbers */
	int cmj, cmn, ci; /* current version numbers */
	int res;
	char buf[32];

	lv = GetLatestVersion();
	if(lv == NULL) return NULL;
	
	/*lv[2] = '4';*/
	res = sscanf(lv,"%u.%u.%u",&lmj,&lmn,&li);
	if(res != 3){
		OutputDebugString("GetNewVersionAnnouncement: the first sscanf call returned ");
		(void)_itoa(res,buf,10);
		OutputDebugString(buf);
		OutputDebugString("\n");
		return NULL;
	}
	res = sscanf(cv,"UltraDefrag %u.%u.%u",&cmj,&cmn,&ci);
	if(res != 3){
		OutputDebugString("GetNewVersionAnnouncement: the second sscanf call returned ");
		(void)_itoa(res,buf,10);
		OutputDebugString(buf);
		OutputDebugString("\n");
		return NULL;
	}
	
	/* 5.0.0 > 4.99.99 */
	if(lmj * 10000 + lmn * 100 + li > cmj * 10000 + cmn * 100 + ci){
		_snwprintf(announcement,MAX_ANNOUNCEMENT_LEN,L"%hs%ws",
			lv,L" release is available for download!");
		announcement[MAX_ANNOUNCEMENT_LEN - 1] = 0;
		
		_snprintf(buf, sizeof(buf), "Upgrade to %s!",lv);
		buf[sizeof(buf) - 1] = 0;
		OutputDebugString(buf);
		OutputDebugString("\n");
		
		return announcement;
	}
	
	return NULL;
}

/**
 * @brief Checks for the new version available for download.
 * @note Runs in a separate thread to speedup the GUI window displaying.
 */
void CheckForTheNewVersion(void)
{
	HANDLE h;
	DWORD id;
	
	if(disable_latest_version_check) return;
	
	h = create_thread(CheckForTheNewVersionThreadProc,NULL,&id);
	if(h == NULL)
		DbgDisplayLastError("Cannot create thread checking the latest version of the program! ");
	if(h) CloseHandle(h);
}

DWORD WINAPI CheckForTheNewVersionThreadProc(LPVOID lpParameter)
{
	short *s;
	
	s = GetNewVersionAnnouncement();
	if(s){
		if(MessageBoxW(hWindow,s,L"You can upgrade me ^-^",MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
			(void)WgxShellExecuteW(hWindow,L"open",L"http://ultradefrag.sourceforge.net",NULL,NULL,SW_SHOW);
	}
	
	return 0;
}
