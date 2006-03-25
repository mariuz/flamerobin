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

  The Initial Developer of the Original Code is Milan Babuskov

  Portions created by the original developer
  are Copyright (C) 2006 Milan Babuskov

  All Rights Reserved.

  $Id:  $

  Contributor(s):
*/

//-----------------------------------------------------------------------------
#ifndef FR_StatementHistoryFrame_H
#define FR_StatementHistoryFrame_H

#include "BaseFrame.h"

class StatementHistory;
//-----------------------------------------------------------------------------
class StatementHistoryFrame : public BaseFrame
{
private:
    bool isSearchingM;
    StatementHistory *historyM;
    wxPanel *m_panel1;
    wxStaticText *m_staticText2;
    wxTextCtrl *textctrl_search;
    wxButton *button_search;
    wxButton *button_delete;
    wxListBox *listbox_search;

    enum    // event handling
    {
        ID_button_search = 101,
        ID_button_delete,
        ID_listbox_search
    };
    void OnButtonSearchClick(wxCommandEvent& event);
    void OnButtonDeleteClick(wxCommandEvent& event);
    void OnListBoxSearchDoubleClick(wxCommandEvent& event);
public:
    StatementHistoryFrame(wxWindow *parent, StatementHistory *history,
        const wxString& title = wxT("SQL Statement History"),
        wxSize size = wxSize(620,400));

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif
