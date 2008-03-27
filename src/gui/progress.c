/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* GUI - progress bar stuff.
*/

#include "main.h"

extern HWND hWindow;
HWND hProgressMsg,hProgressBar;

void InitProgress(void)
{
	hProgressMsg = GetDlgItem(hWindow,IDC_PROGRESSMSG);
	hProgressBar = GetDlgItem(hWindow,IDC_PROGRESS1);
	HideProgress();
}

void ShowProgress(void)
{
	ShowWindow(hProgressMsg,SW_SHOWNORMAL);
	ShowWindow(hProgressBar,SW_SHOWNORMAL);
}

void HideProgress(void)
{
	ShowWindow(hProgressMsg,SW_HIDE);
	ShowWindow(hProgressBar,SW_HIDE);
	SetWindowText(hProgressMsg,"0 %");
	SendMessage(hProgressBar,PBM_SETPOS,0,0);
}

void SetProgress(char *message, int percentage)
{
	SetWindowText(hProgressMsg,message);
	SendMessage(hProgressBar,PBM_SETPOS,(WPARAM)percentage,0);
}
