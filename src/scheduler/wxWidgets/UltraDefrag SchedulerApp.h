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
// Name:        UltraDefrag SchedulerApp.h
// Author:      Justin Dearing  <zippy1981@gmail.com>
// Created:     9/14/2007 7:45:52 AM
// Description:
//
//---------------------------------------------------------------------------

#ifndef __ULTRADEFRAG_SCHEDULERDLGApp_h__
#define __ULTRADEFRAG_SCHEDULERDLGApp_h__

#ifdef __BORLANDC__
	#pragma hdrstop
#endif

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#else
	#include <wx/wxprec.h>
#endif

class UltraDefrag_SchedulerDlgApp : public wxApp
{
	public:
		bool OnInit();
		int OnExit();
};

#endif
