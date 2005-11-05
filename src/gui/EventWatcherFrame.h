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

//-----------------------------------------------------------------------------
#ifndef FR_EVENT_FRAME_H
#define FR_EVENT_FRAME_H

#include <wx/wx.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/panel.h>

#include "ibpp.h"

#include "core/Observer.h"
#include "controls/LogTextControl.h"
#include "gui/BaseFrame.h"

class Database;
class EventLogControl;
//-----------------------------------------------------------------------------
class EventWatcherFrame : public BaseFrame, public IBPP::EventInterface, public Observer
{
private:
    Database* databaseM;
    wxTimer timerM;
    bool monitoringM;
    bool skipEventsM;

    wxPanel* panel_controls;
    wxStaticText* static_text_monitored;
    wxStaticText* static_text_received;
    wxListBox* listbox_monitored;
    EventLogControl* eventlog_received;
    wxButton *button_add;
    wxButton *button_remove;
    wxButton *button_load;
    wxButton *button_save;
    wxButton *button_monitor;
    void createControls();
    void layoutControls();
    void updateControls();

    static wxString getFrameId(Database* db);

    void addEvents(wxString& s);    // multiline allowed
    void defineMonitoredEvents();
    virtual void ibppEventHandler(IBPP::IDatabase*, const std::string& name, 
        int count);
protected:
    virtual const wxString getName() const;
public:
    EventWatcherFrame(wxWindow* parent, Database* db);

    void removeSubject(Subject* subject);
    void update();

    static EventWatcherFrame* findFrameFor(Database* db);
private:
    // event handling
    enum
    {
        ID_listbox_monitored = 101,
        ID_log_received,
        ID_button_add,
        ID_button_remove,
        ID_button_load,
        ID_button_save,
        ID_button_monitor,
        ID_timer
    };

    void OnButtonAddClick(wxCommandEvent& event);
    void OnButtonRemoveClick(wxCommandEvent& event);
    void OnButtonLoadClick(wxCommandEvent& event);
    void OnButtonSaveClick(wxCommandEvent& event);
    void OnButtonStartStopClick(wxCommandEvent& event);
    void OnListBoxSelected(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif
