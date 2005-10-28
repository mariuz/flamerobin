/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2005 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/file.h>
#include "metadata/database.h"
#include "controls/LogTextControl.h"
#include "EventWatcherFrame.h"
//-----------------------------------------------------------------------------
EventWatcherFrame::EventWatcherFrame(wxWindow *parent, Database *db)
    :BaseFrame(parent, -1, wxEmptyString), databaseM(db)
{
    SetTitle(wxString::Format(_("Event monitor for database: %s"), db->getName().c_str()));
    layoutControls();
    updateControls();

    #include "new.xpm"
    wxBitmap bmp(new_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::updateControls()
{
    bool hasEvents = !listbox_events->IsEmpty();
    button_save->Enable(hasEvents);
    button_start->Enable(hasEvents);
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::layoutControls()
{
    wxBoxSizer *bSizer1;
    bSizer1 = new wxBoxSizer(wxVERTICAL);
    m_panel1 = new wxPanel(this,-1,wxDefaultPosition,wxDefaultSize,wxTAB_TRAVERSAL);
    wxBoxSizer *bSizer2;
    bSizer2 = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer *fgSizer2;
    fgSizer2 = new wxFlexGridSizer(2,2,0,0);
    fgSizer2->AddGrowableCol(0);
    fgSizer2->AddGrowableCol(1);
    fgSizer2->AddGrowableRow(1);
    m_staticText1 = new wxStaticText(m_panel1,-1,wxT("Subscribed to"),wxDefaultPosition,wxDefaultSize,0);
    m_staticText2 = new wxStaticText(m_panel1,-1,wxT("Received events"),wxDefaultPosition,wxDefaultSize,0);
    fgSizer2->Add(m_staticText1, 0, wxALL, 5);
    fgSizer2->Add(m_staticText2, 0, wxALL, 5);
    listbox_events = new wxListBox(m_panel1,-1);
    text_ctrl_log = new LogTextControl(m_panel1);
    fgSizer2->Add(listbox_events, 1, wxALL|wxEXPAND, 5);
    fgSizer2->Add(text_ctrl_log, 1, wxALL|wxEXPAND, 5);
    bSizer2->Add(fgSizer2, 1, wxEXPAND, 5);

    wxBoxSizer *bSizer3 = new wxBoxSizer(wxHORIZONTAL);
    button_add = new wxButton(m_panel1,   ID_button_add,   wxT("Subscribe &new"),wxDefaultPosition,wxDefaultSize,0);
    button_remove = new wxButton(m_panel1,ID_button_remove,wxT("&Unsubscribe selected"),wxDefaultPosition,wxDefaultSize,0);
    button_load = new wxButton(m_panel1,  ID_button_load,  wxT("&Load"),wxDefaultPosition,wxDefaultSize,0);
    button_save = new wxButton(m_panel1,  ID_button_save,  wxT("&Save"),wxDefaultPosition,wxDefaultSize,0);
    button_start = new wxButton(m_panel1, ID_button_start, wxT("Start &polling"),wxDefaultPosition,wxDefaultSize,0);
    bSizer3->Add(button_add, 0, wxALL, 5);
    bSizer3->Add(button_remove, 0, wxALL, 5);
    bSizer3->Add(10, 10, 0, wxALL, 5);
    bSizer3->Add(button_load, 0, wxALL, 5);
    bSizer3->Add(button_save, 0, wxALL, 5);
    bSizer3->Add(10, 10, 1, wxALL, 5);
    bSizer3->Add(button_start, 0, wxALL, 5);
    bSizer2->Add(bSizer3, 0, wxEXPAND, 5);
    m_panel1->SetSizer(bSizer2);
    m_panel1->SetAutoLayout(true);
    m_panel1->Layout();
    bSizer1->Add(m_panel1, 1, wxEXPAND, 0);
    SetSizer(bSizer1);
    SetAutoLayout(true);
    Layout();
}
//-----------------------------------------------------------------------------
const wxString EventWatcherFrame::getName() const
{
    return wxT("EventWatcherFrame");
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(EventWatcherFrame, wxFrame)
    EVT_BUTTON(EventWatcherFrame::ID_button_add, EventWatcherFrame::OnButtonAddClick)
    EVT_BUTTON(EventWatcherFrame::ID_button_remove, EventWatcherFrame::OnButtonRemoveClick)
    EVT_BUTTON(EventWatcherFrame::ID_button_load, EventWatcherFrame::OnButtonLoadClick)
    EVT_BUTTON(EventWatcherFrame::ID_button_save, EventWatcherFrame::OnButtonSaveClick)
    EVT_BUTTON(EventWatcherFrame::ID_button_start, EventWatcherFrame::OnButtonStartClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnButtonLoadClick(wxCommandEvent& WXUNUSED(event))
{
    //...
    updateControls();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnButtonSaveClick(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Select file to save"), wxT(""), wxT(""),
        _("Text files (*.txt)|*.txt|All files (*.*)|*.*"),
        wxSAVE |wxCHANGE_DIR | wxOVERWRITE_PROMPT);

    if (wxID_OK != fd.ShowModal())
        return;

    wxString s;
    for (int i = 0; i < listbox_events->GetCount(); ++i)
        s += listbox_events->GetString(i) + wxT("\n");

    wxFile f;
    if (!f.Open(fd.GetPath(), wxFile::write) || !f.Write(s))
    {
        wxMessageBox(_("Cannot write to file."), _("Error"), wxICON_ERROR);
        return;
    }

    if (f.IsOpened())
        f.Close();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnButtonAddClick(wxCommandEvent& WXUNUSED(event))
{
    wxString s = wxGetTextFromUser(_("Enter event name"), _("Subscribing to event"));
    if (s.IsEmpty())
        return;

    // subscribe to event "s" here

    listbox_events->Append(s);
    updateControls();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnButtonRemoveClick(wxCommandEvent& WXUNUSED(event))
{
    wxArrayInt sel;
    if (listbox_events->GetSelections(sel) == 0)
    {
        wxMessageBox(_("Please select some event from the list first."), _("No selection"), wxICON_ERROR);
        return;
    }

    // going backwards to keep indexes valid
    for (int ix = sel.GetCount() - 1; ix >= 0; --ix)
    {
        int to_remove = sel.Item(ix);

        // unsubscribe event

        listbox_events->Delete(to_remove);
    }
    updateControls();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnButtonStartClick(wxCommandEvent& WXUNUSED(event))
{
    if (button_start->GetLabel() == _("Start &polling"))
    {
        // start the actuall polling
        button_start->SetLabel(_("Stop polling"));
    }
    else
    {
        // stop the polling
        button_start->SetLabel(_("Start &polling"));
    }
}
//-----------------------------------------------------------------------------
