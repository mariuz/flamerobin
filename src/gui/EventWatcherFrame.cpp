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

  Contributor(s): Michael Hieke
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

#include <wx/datetime.h>
#include <wx/ffile.h>
#include <wx/file.h>

#include "controls/LogTextControl.h"
#include "gui/EventWatcherFrame.h"
#include "gui/MultilineEnterDialog.h"
#include "metadata/database.h"
#include "styleguide.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
class EventLogControl: public LogTextControl
{
public:
    EventLogControl(wxWindow* parent, wxWindowID id = wxID_ANY,
        long style = wxSUNKEN_BORDER);
    void logAction(const wxString& action);
    void logEvent(const wxString& name, int count);
};
//-----------------------------------------------------------------------------
EventLogControl::EventLogControl(wxWindow* parent, wxWindowID id,
        long style)
    : LogTextControl(parent, id, style)
{
}
//-----------------------------------------------------------------------------
void EventLogControl::logAction(const wxString& action)
{
    wxString now(wxDateTime::Now().Format(wxT("%H:%M:%S  ")));
    addStyledText(now, logStyleImportant);
    logMsg(action + wxT("\n"));
}
//-----------------------------------------------------------------------------
void EventLogControl::logEvent(const wxString& name, int count)
{
    wxString now(wxDateTime::Now().Format(wxT("%H:%M:%S  ")));
    addStyledText(now, logStyleImportant);
    logMsg(name);
    addStyledText(wxString::Format(wxT(" (%d)\n"), count), logStyleError);
}
//-----------------------------------------------------------------------------
EventWatcherFrame::EventWatcherFrame(wxWindow *parent, Database *db)
    :BaseFrame(parent, -1, wxEmptyString), databaseM(db), skipEventsM(false)
{
    monitoringM = false;
    timerM.SetOwner(this, ID_timer);
    setIdString(this, getFrameId(db));
    db->attachObserver(this);    // observe database object
    SetTitle(wxString::Format(_("Event Monitor for Database: %s"),
        db->getName().c_str()));

    createControls();
    layoutControls();
    updateControls();

    #include "new.xpm"
    wxBitmap bmp(new_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::createControls()
{
    panel_controls = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL | wxCLIP_CHILDREN | wxNO_FULL_REPAINT_ON_RESIZE);
    static_text_monitored = new wxStaticText(panel_controls, wxID_ANY,
        _("Monitored events"));
    static_text_received = new wxStaticText(panel_controls, wxID_ANY,
        _("Received events"));
    listbox_monitored = new wxListBox(panel_controls, ID_listbox_monitored,
        wxDefaultPosition, wxDefaultSize, 0, 0, wxLB_EXTENDED);
    eventlog_received = new EventLogControl(panel_controls,
        ID_log_received);
    button_add = new wxButton(panel_controls, ID_button_add, _("&Add event"));
    button_remove = new wxButton(panel_controls, ID_button_remove,
        _("&Remove selected"));
    button_load = new wxButton(panel_controls, ID_button_load, _("&Load"));
    button_save = new wxButton(panel_controls, ID_button_save, _("&Save"));
    button_monitor = new wxButton(panel_controls, ID_button_monitor,
        _("Start &monitoring"));
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::layoutControls()
{
    wxBoxSizer* sizerList = new wxBoxSizer(wxVERTICAL);
    sizerList->Add(static_text_monitored);
    sizerList->Add(0, styleguide().getControlLabelMargin());
    sizerList->Add(listbox_monitored, 1, wxEXPAND);

    wxBoxSizer* sizerLog = new wxBoxSizer(wxVERTICAL);
    sizerLog->Add(static_text_received);
    sizerLog->Add(0, styleguide().getControlLabelMargin());
    sizerLog->Add(eventlog_received, 1, wxEXPAND);

    wxBoxSizer* sizerTop = new wxBoxSizer(wxHORIZONTAL);
    sizerTop->Add(sizerList, 2, wxEXPAND);
    sizerTop->Add(styleguide().getUnrelatedControlMargin(wxHORIZONTAL), 0);
    sizerTop->Add(sizerLog, 3, wxEXPAND);

    wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(button_add);
    sizerButtons->Add(styleguide().getBetweenButtonsMargin(wxHORIZONTAL), 0);
    sizerButtons->Add(button_remove);
    sizerButtons->Add(styleguide().getUnrelatedControlMargin(wxHORIZONTAL), 0);
    sizerButtons->Add(button_load);
    sizerButtons->Add(styleguide().getBetweenButtonsMargin(wxHORIZONTAL), 0);
    sizerButtons->Add(button_save);
    sizerButtons->Add(styleguide().getUnrelatedControlMargin(wxHORIZONTAL), 0,
        1, wxEXPAND);
    sizerButtons->Add(button_monitor);

    wxBoxSizer* sizerPanelV = new wxBoxSizer(wxVERTICAL);
    sizerPanelV->Add(0, styleguide().getFrameMargin(wxTOP));
    sizerPanelV->Add(sizerTop, 1, wxEXPAND);
    sizerPanelV->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerPanelV->Add(sizerButtons, 0, wxEXPAND);
    sizerPanelV->Add(0, styleguide().getFrameMargin(wxBOTTOM));

    wxBoxSizer* sizerPanelH = new wxBoxSizer(wxHORIZONTAL);
    sizerPanelH->Add(styleguide().getFrameMargin(wxLEFT), 0);
    sizerPanelH->Add(sizerPanelV, 1, wxEXPAND);
    sizerPanelH->Add(styleguide().getFrameMargin(wxRIGHT), 0);

    wxBoxSizer* sizerAll = new wxBoxSizer(wxHORIZONTAL);
    sizerAll->Add(sizerPanelH, 1, wxEXPAND);

    panel_controls->SetSizer(sizerAll);
    sizerAll->Fit(this);
    sizerAll->SetSizeHints(this);
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::updateControls()
{
    bool isSelected = false;
    bool hasEvents = !listbox_monitored->IsEmpty();
    if (hasEvents)
    {
        wxArrayInt sel;
        isSelected = listbox_monitored->GetSelections(sel) > 0;
    }
    button_remove->Enable(isSelected);
    button_save->Enable(hasEvents);
    button_monitor->Enable(hasEvents || timerM.IsRunning());
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::defineMonitoredEvents()
{
    // prevent timer from messing our business
    bool timerRunning = timerM.IsRunning();
    if (timerRunning)
    {
        timerM.Stop();
        wxSafeYield();
    }

    skipEventsM = true;         // used to catch phantom notifications
    IBPP::Database& idb = databaseM->getIBPPDatabase();
    idb->ClearEvents();
    for (int ix = 0; ix < listbox_monitored->GetCount(); ++ix)
    {
        idb->DefineEvent(wx2std(listbox_monitored->GetString(ix)), this);
        idb->DispatchEvents();  // let that phantom notification loose
    }
    skipEventsM = false;

    updateControls();
    if (timerRunning)
        timerM.Start(50);
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::ibppEventHandler(IBPP::IDatabase*,
    const std::string& name, int count)
{
    if (!skipEventsM)
        eventlog_received->logEvent(std2wx(name), count);
    // else
    //      TODO: should we inform user anyway that he got those events?
    //      perhaps display in log control, with gray/silver color?
    //      I decided not to put anything as it can give a lot of output
    //      when many events are registered
}
//-----------------------------------------------------------------------------
//! closes window if database is removed (unregistered)
void EventWatcherFrame::removeSubject(Subject* subject)
{
    Observer::removeSubject(subject);
    if (subject == databaseM)
        Close();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::update()
{
    if (!databaseM->isConnected())
        Close();
}
//-----------------------------------------------------------------------------
const wxString EventWatcherFrame::getName() const
{
    return wxT("EventWatcherFrame");
}
//-----------------------------------------------------------------------------
wxString EventWatcherFrame::getFrameId(Database* db)
{
    if (db)
        return wxString(wxT("EventWatcherFrame/") + db->getItemPath());
    else
        return wxEmptyString;
}
//-----------------------------------------------------------------------------
EventWatcherFrame* EventWatcherFrame::findFrameFor(Database* db)
{
    BaseFrame* bf = frameFromIdString(getFrameId(db));
    if (!bf)
        return 0;
    return dynamic_cast<EventWatcherFrame*>(bf);
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(EventWatcherFrame, wxFrame)
    EVT_BUTTON(EventWatcherFrame::ID_button_add, EventWatcherFrame::OnButtonAddClick)
    EVT_BUTTON(EventWatcherFrame::ID_button_remove, EventWatcherFrame::OnButtonRemoveClick)
    EVT_BUTTON(EventWatcherFrame::ID_button_load, EventWatcherFrame::OnButtonLoadClick)
    EVT_BUTTON(EventWatcherFrame::ID_button_save, EventWatcherFrame::OnButtonSaveClick)
    EVT_BUTTON(EventWatcherFrame::ID_button_monitor, EventWatcherFrame::OnButtonStartStopClick)
    EVT_LISTBOX(EventWatcherFrame::ID_listbox_monitored, EventWatcherFrame::OnListBoxSelected)
    EVT_TIMER(EventWatcherFrame::ID_timer, EventWatcherFrame::OnTimer)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnButtonLoadClick(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Select file to load"), wxT(""), wxT(""),
        _("Text files (*.txt)|*.sql|All files (*.*)|*.*"),
        wxOPEN|wxCHANGE_DIR);
    if (wxID_OK != fd.ShowModal())
        return;

    wxFFile f(fd.GetPath());
    if (!f.IsOpened())
    {
        wxMessageBox(_("Cannot open file."), _("Error"), wxOK|wxICON_ERROR);
        return;
    }
    wxBusyCursor wait;
    wxString s;
    f.ReadAll(&s);
    f.Close();
    addEvents(s);
    defineMonitoredEvents();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnButtonSaveClick(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog fd(this, _("Select file to save"), wxT(""), wxT(""),
        _("Text files (*.txt)|*.txt|All files (*.*)|*.*"),
        wxSAVE |wxCHANGE_DIR | wxOVERWRITE_PROMPT);

    if (wxID_OK != fd.ShowModal())
        return;

    wxBusyCursor wait;
    wxString s;
    for (int i = 0; i < listbox_monitored->GetCount(); ++i)
        s += listbox_monitored->GetString(i) + wxT("\n");

    wxFile f;
    if (!f.Open(fd.GetPath(), wxFile::write) || !f.Write(s))
    {
        wxMessageBox(_("Cannot write to file."), _("Error"), wxOK|wxICON_ERROR);
        return;
    }

    if (f.IsOpened())
        f.Close();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnButtonAddClick(wxCommandEvent& WXUNUSED(event))
{
    wxString s;
    if (GetMultilineTextFromUser(_("Add event(s)"), s, this,
        _("You can add multiple events by adding one per line.")))
    {
        addEvents(s);
    }
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::addEvents(wxString& s)
{
    while (true)
    {
        int p = s.Find(wxT("\n"));
        wxString s2;
        if (p == -1)
            s2 = s.Strip();
        else
        {
            s2 = s.Left(p).Strip(wxString::both);
            s.Remove(0, p);
            s.Trim(false);
        }
        if (!s2.IsEmpty() && listbox_monitored->FindString(s2) == wxNOT_FOUND)
        {
            databaseM->getIBPPDatabase()->DefineEvent(wx2std(s2), this);
            listbox_monitored->Select(listbox_monitored->Append(s2));
        }
        if (p == -1)
            break;
    }
    updateControls();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnButtonRemoveClick(wxCommandEvent& WXUNUSED(event))
{
    wxArrayInt sel;
    if (listbox_monitored->GetSelections(sel) == 0)
        return;

    wxBusyCursor wait;
    // going backwards to keep indexes valid
    for (int ix = sel.GetCount() - 1; ix >= 0; --ix)
        listbox_monitored->Delete(sel.Item(ix));
    defineMonitoredEvents();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnButtonStartStopClick(wxCommandEvent& WXUNUSED(event))
{
    if (monitoringM)
    {
        timerM.Stop();
        monitoringM = false;
        button_monitor->SetLabel(_("Start &monitoring"));
        eventlog_received->logAction(_("Monitoring stopped"));
    }
    else
    {
        if (!timerM.Start(50))
        {
            wxMessageBox(_("Cannot start timer"), _("Error"), wxOK|wxICON_ERROR);
            return;
        }
        monitoringM = true;
        button_monitor->SetLabel(_("Stop &monitoring"));
        eventlog_received->logAction(_("Monitoring started"));
    }
    updateControls();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnListBoxSelected(wxCommandEvent& WXUNUSED(event))
{
    updateControls();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnTimer(wxTimerEvent& WXUNUSED(event))
{
    databaseM->getIBPPDatabase()->DispatchEvents();
}
//-----------------------------------------------------------------------------
