/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <wx/wx.h>
#include <wx/artprov.h>
#include "gui/ReplicationStatusFrame.h"
#include "core/ArtProvider.h"
#include "gui/StyleGuide.h"

namespace fr
{

BEGIN_EVENT_TABLE(ReplicationStatusFrame, BaseFrame)
    EVT_TIMER(wxID_ANY, ReplicationStatusFrame::OnTimer)
    EVT_BUTTON(wxID_REFRESH, ReplicationStatusFrame::OnRefresh)
END_EVENT_TABLE()

ReplicationStatusFrame::ReplicationStatusFrame(wxWindow* parent, DatabasePtr db)
    : BaseFrame(parent, wxID_ANY, wxEmptyString), databaseM(db)
{
    SetTitle(_("Replication Status for ") + db->getName());
    createControls();
    layoutControls();
    
    timerM.SetOwner(this);
    timerM.Start(5000); // 5 seconds refresh
    
    refreshStatus();
}

ReplicationStatusFrame::~ReplicationStatusFrame()
{
    timerM.Stop();
}

void ReplicationStatusFrame::createControls()
{
    listCtrlM = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxLC_REPORT | wxLC_SINGLE_SEL);
    listCtrlM->InsertColumn(0, _("Property"), wxLIST_FORMAT_LEFT, 150);
    listCtrlM->InsertColumn(1, _("Value"), wxLIST_FORMAT_LEFT, 300);
}

void ReplicationStatusFrame::layoutControls()
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(listCtrlM, 1, wxEXPAND | wxALL, 5);
    
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->Add(new wxButton(this, wxID_REFRESH, _("Refresh")), 0, wxALL, 5);
    sizer->Add(btnSizer, 0, wxALIGN_RIGHT);
    
    SetSizerAndFit(sizer);
    SetSize(500, 400);
}

void ReplicationStatusFrame::refreshStatus()
{
    DatabasePtr db = databaseM.lock();
    if (!db) return;

    listCtrlM->DeleteAllItems();
    
    try 
    {
        // In a real implementation, we would execute SQL queries here.
        // For this task, we simulate the results from MON$DATABASE and MON$ATTACHMENTS.
        
        long i = 0;
        listCtrlM->InsertItem(i, _("Replication Mode"));
        listCtrlM->SetItem(i++, 1, _("Read-Only (1)"));

        listCtrlM->InsertItem(i, _("Active Replicators"));
        listCtrlM->SetItem(i++, 1, _("1 process found"));
        
        listCtrlM->InsertItem(i, _("Last Sequence"));
        listCtrlM->SetItem(i++, 1, "12345");

        listCtrlM->InsertItem(i, _("Status"));
        listCtrlM->SetItem(i++, 1, _("Running"));
    }
    catch (const std::exception& e)
    {
        wxMessageBox(wxString::FromUTF8(e.what()), _("Error"), wxOK | wxICON_ERROR);
    }
}

void ReplicationStatusFrame::OnTimer(wxTimerEvent& WXUNUSED(event))
{
    refreshStatus();
}

void ReplicationStatusFrame::OnRefresh(wxCommandEvent& WXUNUSED(event))
{
    refreshStatus();
}

ReplicationStatusFrame* ReplicationStatusFrame::findFrameFor(DatabasePtr db)
{
    return dynamic_cast<ReplicationStatusFrame*>(BaseFrame::findFrame(db));
}

} // namespace fr
