//---------------------------------------------------------------------------
//
// Name:        UltraDefrag SchedulerDlg.cpp
// Author:      Justin Dearing <zippy1981@gmail.com>
// Created:     9/14/2007 7:45:52 AM
// Description: UltraDefrag_SchedulerDlg class implementation
//
//---------------------------------------------------------------------------

#include "UltraDefrag SchedulerDlg.h"

//Do not add custom headers
//wxDev-C++ designer will remove them
////Header Include Start
#include "Images/Self_UltraDefrag_SchedulerDlg_XPM.xpm"
////Header Include End

//----------------------------------------------------------------------------
// UltraDefrag_SchedulerDlg
//----------------------------------------------------------------------------
//Add Custom Events only in the appropriate block.
//Code added in other places will be removed by wxDev-C++
////Event Table Start
BEGIN_EVENT_TABLE(UltraDefrag_SchedulerDlg,wxDialog)
	////Manual Code Start
	////Manual Code End
	
	EVT_CLOSE(UltraDefrag_SchedulerDlg::OnClose)
	EVT_BUTTON(ID_WXCMDEXIT,UltraDefrag_SchedulerDlg::WxCmdExitClick)
	EVT_BUTTON(ID_WXCMDOK,UltraDefrag_SchedulerDlg::WxCmdOkClick)
	EVT_RADIOBUTTON(ID_WXRADDAILY,UltraDefrag_SchedulerDlg::WxRadDailyClick)
	EVT_RADIOBUTTON(ID_WXRADWEEKLY,UltraDefrag_SchedulerDlg::WxRadWeeklyClick)
END_EVENT_TABLE()
////Event Table End

UltraDefrag_SchedulerDlg::UltraDefrag_SchedulerDlg(wxWindow *parent, wxWindowID id, const wxString &title, const wxPoint &position, const wxSize& size, long style)
: wxDialog(parent, id, title, position, size, style)
{
	CreateGUIControls();
}

UltraDefrag_SchedulerDlg::~UltraDefrag_SchedulerDlg()
{
} 

void UltraDefrag_SchedulerDlg::CreateGUIControls()
{
	//Do not add custom code between
	//GUI Items Creation Start and GUI Items Creation End.
	//wxDev-C++ designer will remove them.
	//Add the custom code before or after the blocks
	////GUI Items Creation Start

	SetTitle(wxT("UltraDefrag Scheduler"));
	SetIcon(Self_UltraDefrag_SchedulerDlg_XPM);
	SetSize(8,8,302,169);
	Center();
	

	WxCmdExit = new wxButton(this, ID_WXCMDEXIT, wxT("&Exit"), wxPoint(208,104), wxSize(75,25), 0, wxDefaultValidator, wxT("WxCmdExit"));
	WxCmdExit->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	WxCmdOk = new wxButton(this, ID_WXCMDOK, wxT("&Ok"), wxPoint(128,104), wxSize(75,25), 0, wxDefaultValidator, wxT("WxCmdOk"));
	WxCmdOk->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	WxCkSaturday = new wxCheckBox(this, ID_WXCKSATURDAY, wxT("Saturday"), wxPoint(200,80), wxSize(82,17), 0, wxDefaultValidator, wxT("WxCkSaturday"));
	WxCkSaturday->Enable(false);
	WxCkSaturday->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	WxCkFriday = new wxCheckBox(this, ID_WXCKFRIDAY, wxT("Friday"), wxPoint(104,80), wxSize(82,17), 0, wxDefaultValidator, wxT("WxCkFriday"));
	WxCkFriday->Enable(false);
	WxCkFriday->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	WxCkThursday = new wxCheckBox(this, ID_WXCKTHURSDAY, wxT("Thursday"), wxPoint(8,80), wxSize(82,17), 0, wxDefaultValidator, wxT("WxCkThursday"));
	WxCkThursday->Enable(false);
	WxCkThursday->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	WxCkSunday = new wxCheckBox(this, ID_WXCKSUNDAY, wxT("Sunday"), wxPoint(8,104), wxSize(82,17), 0, wxDefaultValidator, wxT("WxCkSunday"));
	WxCkSunday->Enable(false);
	WxCkSunday->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	WxCkWednesday = new wxCheckBox(this, ID_WXCKWEDNESDAY, wxT("Wednesday"), wxPoint(200,56), wxSize(82,17), 0, wxDefaultValidator, wxT("WxCkWednesday"));
	WxCkWednesday->Enable(false);
	WxCkWednesday->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	WxCkTuesday = new wxCheckBox(this, ID_WXCKTUESDAY, wxT("Tuesday"), wxPoint(104,56), wxSize(82,17), 0, wxDefaultValidator, wxT("WxCkTuesday"));
	WxCkTuesday->Enable(false);
	WxCkTuesday->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	WxCkMonday = new wxCheckBox(this, ID_WXCKMONDAY, wxT("Monday"), wxPoint(8,56), wxSize(82,17), 0, wxDefaultValidator, wxT("WxCkMonday"));
	WxCkMonday->Enable(false);
	WxCkMonday->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	WxRadDaily = new wxRadioButton(this, ID_WXRADDAILY, wxT("&Daily"), wxPoint(8,8), wxSize(113,17), 0, wxDefaultValidator, wxT("WxRadDaily"));
	WxRadDaily->SetValue(true);
	WxRadDaily->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));

	WxRadWeekly = new wxRadioButton(this, ID_WXRADWEEKLY, wxT("Weekly"), wxPoint(8,32), wxSize(113,17), 0, wxDefaultValidator, wxT("WxRadWeekly"));
	WxRadWeekly->SetFont(wxFont(8, wxSWISS, wxNORMAL,wxNORMAL, false, wxT("Tahoma")));
	////GUI Items Creation End
}

void UltraDefrag_SchedulerDlg::OnClose(wxCloseEvent& /*event*/)
{
	Destroy();
}

/*
 * WxCmdOkClick
 */
void UltraDefrag_SchedulerDlg::WxCmdOkClick(wxCommandEvent& event)
{
    LPDWORD JobId = new DWORD;
    AT_INFO AtInfo;
    LPWSTR  Command = L"udefrag c:";

    AtInfo.JobTime = 0;
    AtInfo.DaysOfMonth = 0;
    AtInfo.Flags = JOB_RUN_PERIODICALLY | JOB_NONINTERACTIVE;
    AtInfo.Command = Command;
    AtInfo.DaysOfWeek = 0;
	
    if (WxRadDaily->GetValue())
	{
        AtInfo.DaysOfWeek = 127;
    }
    else if (WxRadWeekly->GetValue())
    {
        if (WxCkMonday->GetValue()) AtInfo.DaysOfWeek |= 0x1;
        if (WxCkTuesday->GetValue()) AtInfo.DaysOfWeek |= 0x2;
        if (WxCkWednesday->GetValue()) AtInfo.DaysOfWeek |= 0x4;
        if (WxCkThursday->GetValue()) AtInfo.DaysOfWeek |= 0x8;
        if (WxCkFriday->GetValue()) AtInfo.DaysOfWeek |= 0x10;
        if (WxCkSaturday->GetValue()) AtInfo.DaysOfWeek |= 0x20;
        if (WxCkSunday->GetValue()) AtInfo.DaysOfWeek |= 0x40;
    }
    else
    {
        wxMessageBox("ERROR: Code execution should never reach this point.");
    }
    
    NetScheduleJobAdd(NULL, (LPBYTE)&AtInfo, JobId);
    free (JobId);
    Close();
}

/*
 * WxCmdExitClick
 */
void UltraDefrag_SchedulerDlg::WxCmdExitClick(wxCommandEvent& event)
{
	Close();
}

/*
 * WxRadDailyClick
 */
void UltraDefrag_SchedulerDlg::WxRadDailyClick(wxCommandEvent& event)
{
	WxCkMonday->Disable();
	WxCkTuesday->Disable();
	WxCkWednesday->Disable();
	WxCkThursday->Disable();
	WxCkFriday->Disable();
	WxCkSaturday->Disable();
	WxCkSunday->Disable();
}

/*
 * WxRadWeeklyClick
 */
void UltraDefrag_SchedulerDlg::WxRadWeeklyClick(wxCommandEvent& event)
{
	WxCkMonday->Enable();
	WxCkTuesday->Enable();
	WxCkWednesday->Enable();
	WxCkThursday->Enable();
	WxCkFriday->Enable();
	WxCkSaturday->Enable();
	WxCkSunday->Enable();
}
