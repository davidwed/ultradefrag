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
#include "../include/version.h"

extern HWND hWindow;

#define VERSION_URL "http://ultradefrag.sourceforge.net/version.ini"
char version_ini_path[MAX_PATH + 1];
#define MAX_VERSION_FILE_LEN 32
char version_number[MAX_VERSION_FILE_LEN + 1];
char error_msg[256];
#define MAX_ANNOUNCEMENT_LEN 128
short announcement[MAX_ANNOUNCEMENT_LEN];

#define IBindStatusCallback LPVOID
typedef HRESULT (__stdcall *URLMON_PROCEDURE)(
	LPUNKNOWN,
	LPCSTR,
	LPTSTR,
	DWORD,
	DWORD,
	IBindStatusCallback
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
		DbgDisplayLastError("UltraDefrag: LoadLibrary(urlmon.dll) failed! ");
		return NULL;
	}
	
	/* get an address of procedure downloading a file */
	pURLDownloadToCacheFile = (URLMON_PROCEDURE)GetProcAddress(hUrlmonDLL,"URLDownloadToCacheFileA");
	if(pURLDownloadToCacheFile == NULL){
		DbgDisplayLastError("UltraDefrag: URLDownloadToCacheFile not found in urlmon.dll! ");
		return NULL;
	}
	
	/* download a file */
	result = pURLDownloadToCacheFile(NULL,VERSION_URL,version_ini_path,MAX_PATH,0,NULL);
	version_ini_path[MAX_PATH] = 0;
	if(result != S_OK){
		if(result == E_OUTOFMEMORY) OutputDebugString("UltraDefrag: no enough memory for URLDownloadToCacheFile!");
		else OutputDebugString("UltraDefrag: URLDownloadToCacheFile failed!");
		return NULL;
	}
	
	/* open the file */
	f = fopen(version_ini_path,"rb");
	if(f == NULL){
		(void)_snprintf(error_msg,sizeof(error_msg) - 1,
			"Cannot open %s! %s",
			version_ini_path,_strerror(NULL));
		error_msg[sizeof(error_msg) - 1] = 0;
		OutputDebugString(error_msg);
		return NULL;
	}
	
	/* read version string */
	res = fread(version_number,1,MAX_VERSION_FILE_LEN,f);
	fclose(f);
	if(res == 0){
		OutputDebugString("UltraDefrag: version.ini file reading failed! ");
		OutputDebugString(version_ini_path);
		if(feof(f)) OutputDebugString(" File seems to be empty.");
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

	lv = GetLatestVersion();
	if(lv == NULL) return NULL;
	
	//lv[2] = '4';
	if(sscanf(lv,"%u.%u.%u",&lmj,&lmn,&li) != 3) return NULL;
	if(sscanf(cv,"UltraDefrag %u.%u.%u",&cmj,&cmn,&ci) != 3) return NULL;
	
	if(lmj > cmj || (lmj == cmj && lmn > cmn) || (lmj == cmj && lmn == cmn && li > ci)){
		_snwprintf(announcement,MAX_ANNOUNCEMENT_LEN,L"%hs%ws",
			lv,L" release is available for download!");
		announcement[MAX_ANNOUNCEMENT_LEN - 1] = 0;
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
	
	h = create_thread(CheckForTheNewVersionThreadProc,NULL,&id);
	if(h == NULL)
		DisplayLastError("Cannot create thread checking the latest version of the program!");
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
