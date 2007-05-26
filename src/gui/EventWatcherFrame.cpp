/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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


  $Id$

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
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "gui/EventWatcherFrame.h"
#include "gui/MultilineEnterDialog.h"
#include "gui/StyleGuide.h"
#include "metadata/database.h"
//-----------------------------------------------------------------------------
using namespace std;
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
    : BaseFrame(parent, -1, wxEmptyString), databaseM(db)
{
    timerM.SetOwner(this, ID_timer);
    eventsM = 0;

    setIdString(this, getFrameId(db));
    db->attachObserver(this);    // observe database object
    SetTitle(wxString::Format(_("Event Monitor for Database: %s"),
        db->getName_().c_str()));

    createControls();
    layoutControls();
    updateControls();

    button_add->SetFocus();

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
    button_add = new wxButton(panel_controls, ID_button_add, _("&Add Events"));
    button_remove = new wxButton(panel_controls, ID_button_remove,
        _("&Remove Selected"));
    button_load = new wxButton(panel_controls, ID_button_load, _("&Load"));
    button_save = new wxButton(panel_controls, ID_button_save, _("&Save"));
    button_monitor = new wxButton(panel_controls, ID_button_monitor,
        _("Start &Monitoring"));
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::layoutControls()
{
    wxBoxSizer* sizerList = new wxBoxSizer(wxVERTICAL);
    sizerList->Add(static_text_monitored);
    sizerList->AddSpacer(styleguide().getControlLabelMargin());
    sizerList->Add(listbox_monitored, 1, wxEXPAND);

    wxBoxSizer* sizerLog = new wxBoxSizer(wxVERTICAL);
    sizerLog->Add(static_text_received);
    sizerLog->AddSpacer(styleguide().getControlLabelMargin());
    sizerLog->Add(eventlog_received, 1, wxEXPAND);

    wxBoxSizer* sizerTop = new wxBoxSizer(wxHORIZONTAL);
    sizerTop->Add(sizerList, 2, wxEXPAND);
    sizerTop->AddSpacer(styleguide().getUnrelatedControlMargin(wxHORIZONTAL));
    sizerTop->Add(sizerLog, 3, wxEXPAND);

    wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(button_add);
    sizerButtons->AddSpacer(styleguide().getBetweenButtonsMargin(wxHORIZONTAL));
    sizerButtons->Add(button_remove);
    sizerButtons->AddSpacer(styleguide().getUnrelatedControlMargin(wxHORIZONTAL));
    sizerButtons->Add(button_load);
    sizerButtons->AddSpacer(styleguide().getBetweenButtonsMargin(wxHORIZONTAL));
    sizerButtons->Add(button_save);
    sizerButtons->Add(styleguide().getUnrelatedControlMargin(wxHORIZONTAL), 0,
        1, wxEXPAND);
    sizerButtons->Add(button_monitor);

    wxBoxSizer* sizerPanelV = new wxBoxSizer(wxVERTICAL);
    sizerPanelV->AddSpacer(styleguide().getFrameMargin(wxTOP));
    sizerPanelV->Add(sizerTop, 1, wxEXPAND);
    sizerPanelV->AddSpacer(styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerPanelV->Add(sizerButtons, 0, wxEXPAND);
    sizerPanelV->AddSpacer(styleguide().getFrameMargin(wxBOTTOM));

    wxBoxSizer* sizerPanelH = new wxBoxSizer(wxHORIZONTAL);
    sizerPanelH->AddSpacer(styleguide().getFrameMargin(wxLEFT));
    sizerPanelH->Add(sizerPanelV, 1, wxEXPAND);
    sizerPanelH->AddSpacer(styleguide().getFrameMargin(wxRIGHT));

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
void EventWatcherFrame::addEvents(wxString& s)
{
    // deselect all items so user can cleanly see what is added
    for (int ix = 0; ix < (int)listbox_monitored->GetCount(); ++ix)
    {
        if (listbox_monitored->IsSelected(ix))
            listbox_monitored->Deselect(ix);
    }
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
            listbox_monitored->Select(listbox_monitored->Append(s2));
        if (p == -1)
            break;
    }
    updateControls();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::defineMonitoredEvents()
{
    if (eventsM != 0)
    {
        // prevent timer from messing our business
        bool timerRunning = timerM.IsRunning();
        setTimerActive(false);

        // get a list of events to be monitored
        vector<string> events;
        for (int i = 0; i < (int)listbox_monitored->GetCount(); i++)
            events.push_back(wx2std(listbox_monitored->GetString(i)));

        eventsM->Clear();
        vector<string>::const_iterator it;
        for (it = events.begin(); it != events.end(); it++)
        {
            eventsM->Add(*it, this);
            // make IBPP::Events pick up the initial event count
            eventsM->Dispatch();
        }

        updateControls();
        if (timerRunning)
            setTimerActive(true);
    }
}
//-----------------------------------------------------------------------------
bool EventWatcherFrame::setTimerActive(bool active)
{
    if (active && !timerM.Start(100))
        wxMessageBox(_("Can not start timer"), _("Error"), wxOK | wxICON_ERROR);
        
    if (!active && timerM.IsRunning())
    {
        timerM.Stop();
        wxSafeYield();
    }
    return active == timerM.IsRunning();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::updateMonitoringActive()
{
    if (eventsM != 0)
    {
        setTimerActive(true);
        button_monitor->SetLabel(_("Stop &Monitoring"));
        eventlog_received->logAction(_("Monitoring started"));
    }
    else
    {
        timerM.Stop();
        button_monitor->SetLabel(_("Start &Monitoring"));
        eventlog_received->logAction(_("Monitoring stopped"));
    }
    updateControls();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::ibppEventHandler(IBPP::Events events,
    const std::string& name, int count)
{
    eventlog_received->logEvent(std2wx(name), count);
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
        _("Text files (*.txt)|*.txt|All files (*.*)|*.*"),
#if wxCHECK_VERSION(2, 8, 0)
        wxFD_OPEN | wxFD_CHANGE_DIR);
#else
        wxOPEN | wxCHANGE_DIR);
#endif
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
#if wxCHECK_VERSION(2, 8, 0)
        wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT);
#else
        wxSAVE |wxCHANGE_DIR | wxOVERWRITE_PROMPT);
#endif

    if (wxID_OK != fd.ShowModal())
        return;

    wxBusyCursor wait;
    wxString s;
    for (int i = 0; i < (int)listbox_monitored->GetCount(); ++i)
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
    if (GetMultilineTextFromUser(this, _("Add Events for Monitoring"), s,
        _("You can add multiple events by adding one per line."),
        _("Add Events")))
    {
        addEvents(s);
        defineMonitoredEvents();
    }
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
    FR_TRY

    if (eventsM != 0)
        eventsM.clear();
    else
    {
        IBPP::Database db(databaseM->getIBPPDatabase());
        eventsM = IBPP::EventsFactory(db);
        defineMonitoredEvents();
    }
    updateMonitoringActive();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnListBoxSelected(wxCommandEvent& WXUNUSED(event))
{
    updateControls();
}
//-----------------------------------------------------------------------------
void EventWatcherFrame::OnTimer(wxTimerEvent& WXUNUSED(event))
{
    if (eventsM != 0)
        eventsM->Dispatch();
    else // stop timer, update UI
        updateMonitoringActive();
}
//-----------------------------------------------------------------------------
