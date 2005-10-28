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

#ifndef FR_EVENT_FRAME_H
#define FR_EVENT_FRAME_H
#include <wx/wx.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/listbox.h>

#include "ibpp.h"
#include "controls/LogTextControl.h"
#include "BaseFrame.h"

class Database;
// TODO: make it an observer of database, so it closes when connection is closed
//-----------------------------------------------------------------------------
class EventWatcherFrame : public BaseFrame, public IBPP::EventInterface
{
protected:
	virtual void ibppEventHandler(IBPP::IDatabase*, const std::string& name, int count);
    wxTimer timerM;

    Database *databaseM;
    virtual const wxString getName() const;
    void layoutControls();
    void updateControls();

    wxPanel *m_panel1;
    wxStaticText *m_staticText1;
    wxStaticText *m_staticText2;
    wxListBox *listbox_events;
    LogTextControl* text_ctrl_log;    // we want colors :)
    wxButton *button_add;
    wxButton *button_remove;
    wxButton *button_load;
    wxButton *button_save;
    wxButton *button_start;
    enum
    {
        ID_button_add = 101,
        ID_button_remove = 102,
        ID_button_load = 103,
        ID_button_save = 104,
        ID_button_start = 105,
        ID_timer
    };
    void OnButtonAddClick(wxCommandEvent &event);
    void OnButtonRemoveClick(wxCommandEvent &event);
    void OnButtonLoadClick(wxCommandEvent &event);
    void OnButtonSaveClick(wxCommandEvent &event);
    void OnButtonStartClick(wxCommandEvent &event);
    void OnTimer(wxTimerEvent &event);

public:
    EventWatcherFrame(wxWindow *parent, Database *db);
    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif

