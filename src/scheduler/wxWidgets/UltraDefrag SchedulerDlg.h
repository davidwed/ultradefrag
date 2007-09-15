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
// Name:        UltraDefrag SchedulerDlg.h
// Author:      Justin Dearing  <zippy1981@gmail.com>
// Created:     9/14/2007 7:45:52 AM
// Description: UltraDefrag_SchedulerDlg class declaration
//
//---------------------------------------------------------------------------

#ifndef __ULTRADEFRAG_SCHEDULERDLG_h__
#define __ULTRADEFRAG_SCHEDULERDLG_h__

#define WIN32LEANANDMEAN
#include <windows.h>
#include <lmshare.h>
#include <lmat.h>

#ifdef __BORLANDC__
	#pragma hdrstop
#endif

#ifndef WX_PRECOMP
	#include <wx/wx.h>
	#include <wx/dialog.h>
#else
	#include <wx/wxprec.h>
#endif

//Do not add custom headers between 
//Header Include Start and Header Include End.
//wxDev-C++ designer will remove them. Add custom headers after the block.
////Header Include Start
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
////Header Include End

////Dialog Style Start
#undef UltraDefrag_SchedulerDlg_STYLE
#define UltraDefrag_SchedulerDlg_STYLE wxCAPTION | wxSYSTEM_MENU | wxDIALOG_NO_PARENT | wxMINIMIZE_BOX | wxCLOSE_BOX
////Dialog Style End

class UltraDefrag_SchedulerDlg : public wxDialog
{
	private:
		DECLARE_EVENT_TABLE();
		
	public:
		UltraDefrag_SchedulerDlg(wxWindow *parent, wxWindowID id = 1, const wxString &title = wxT("UltraDefrag Scheduler"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = UltraDefrag_SchedulerDlg_STYLE);
		virtual ~UltraDefrag_SchedulerDlg();
		void WxCmdOkClick(wxCommandEvent& event);
		void WxCmdExitClick(wxCommandEvent& event);
		void WxRadDailyClick(wxCommandEvent& event);
		void WxRadWeeklyClick(wxCommandEvent& event);
		
	private:
		//Do not add custom control declarations between 
		//GUI Control Declaration Start and GUI Control Declaration End.
		//wxDev-C++ will remove them. Add custom code after the block.
		////GUI Control Declaration Start
		wxButton *WxCmdExit;
		wxButton *WxCmdOk;
		wxCheckBox *WxCkSaturday;
		wxCheckBox *WxCkFriday;
		wxCheckBox *WxCkThursday;
		wxCheckBox *WxCkSunday;
		wxCheckBox *WxCkWednesday;
		wxCheckBox *WxCkTuesday;
		wxCheckBox *WxCkMonday;
		wxRadioButton *WxRadDaily;
		wxRadioButton *WxRadWeekly;
		////GUI Control Declaration End
		
	private:
		//Note: if you receive any error with these enum IDs, then you need to
		//change your old form code that are based on the #define control IDs.
		//#defines may replace a numeric value for the enum names.
		//Try copy and pasting the below block in your old form header files.
		enum
		{
			////GUI Enum Control ID Start
			ID_WXCMDEXIT = 1028,
			ID_WXCMDOK = 1026,
			ID_WXCKSATURDAY = 1025,
			ID_WXCKFRIDAY = 1024,
			ID_WXCKTHURSDAY = 1023,
			ID_WXCKSUNDAY = 1018,
			ID_WXCKWEDNESDAY = 1009,
			ID_WXCKTUESDAY = 1008,
			ID_WXCKMONDAY = 1007,
			ID_WXRADDAILY = 1002,
			ID_WXRADWEEKLY = 1001,
			////GUI Enum Control ID End
			ID_DUMMY_VALUE_ //don't remove this value unless you have other enum values
		};
	
	private:
		void OnClose(wxCloseEvent& event);
		void CreateGUIControls();
};

#endif
