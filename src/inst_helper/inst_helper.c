/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
 *  installation helper for all targets - it create custom preset.
 */

#include <windows.h>
#include <winreg.h>
#include <stdio.h>

short value[1024 + 4];

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR lpCmdLine, int nShowCmd)
{
	FILE *pf;
	DWORD len;
	HKEY hKey;

	pf = _wfopen(lpCmdLine,L"rb");
	if(pf)
	{ /* custom preset exist */
		fclose(pf);
		return 1;
	}
	pf = _wfopen(lpCmdLine,L"wb");
	if(!pf)
	{ /* can't create file */
		return 2;
	}
	/* read data from registry and write them to file */
	if(RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\DASoft\\NTDefrag",0,
		KEY_QUERY_VALUE,&hKey) == ERROR_SUCCESS)
	{
		len = 1024; value[0] = 0;
		RegQueryValueExW(hKey,L"include filter",NULL,NULL,
			(BYTE*)value,&len);
		fwrite(value,sizeof(short),wcslen(value) + 1,pf);
		len = 1024; value[0] = 0;
		RegQueryValueExW(hKey,L"exclude filter",NULL,NULL,
			(BYTE*)value,&len);
		fwrite(value,sizeof(short),wcslen(value) + 1,pf);
		RegCloseKey(hKey);
		fclose(pf);
		return 0;
	}
	else
	{
		value[0] = 0;
		fwrite(value,sizeof(short),1,pf);
		fwrite(value,sizeof(short),1,pf);
		fclose(pf);
		return 3;
	}
}
