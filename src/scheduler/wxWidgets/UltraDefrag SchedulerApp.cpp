/*
 *  UltraDefragScheduler
 *  Copyright (c) 2007 by:
 *		Justin Dearing (zippy1981@gmail.com)
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
 
//---------------------------------------------------------------------------
//
// Name:        UltraDefrag SchedulerApp.cpp
// Author:      Justin Dearing <zippy1981@gmail.com>
// Created:     9/14/2007 7:45:52 AM
// Description: 
//
//---------------------------------------------------------------------------

#include "UltraDefrag SchedulerApp.h"
#include "UltraDefrag SchedulerDlg.h"

IMPLEMENT_APP(UltraDefrag_SchedulerDlgApp)

bool UltraDefrag_SchedulerDlgApp::OnInit()
{
	UltraDefrag_SchedulerDlg* dialog = new UltraDefrag_SchedulerDlg(NULL);
	SetTopWindow(dialog);
	dialog->Show(true);		
	return true;
}
 
int UltraDefrag_SchedulerDlgApp::OnExit()
{
	return 0;
}
