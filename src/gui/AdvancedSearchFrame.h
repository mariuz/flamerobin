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
  are Copyright (C) 2006 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/
#ifndef FR_ADVANCEDSEARCHFRAME_H
#define FR_ADVANCEDSEARCHFRAME_H

#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>

#include "BaseFrame.h"
//-----------------------------------------------------------------------------
class AdvancedSearchFrame : public BaseFrame
{
private:

protected:
    wxPanel *mainPanel;
    wxStaticText *m_staticText1;
    wxChoice *choice_type;
    wxButton *button_add_type;
    wxStaticText *m_staticText2;
    wxTextCtrl *textctrl_name;
    wxButton *button_add_name;
    wxStaticText *m_staticText3;
    wxTextCtrl *textctrl_description;
    wxButton *button_add_description;
    wxStaticText *m_staticText4;
    wxTextCtrl *textctrl_ddl;
    wxButton *button_add_ddl;
    wxStaticText *m_staticText5;
    wxTextCtrl *textctrl_field;
    wxButton *button_add_field;
    wxStaticText *m_staticText6;
    wxChoice *choice_database;
    wxButton *button_add_database;
    wxStaticText *label_search_criteria;
    wxListCtrl *listctrl_criteria;
    wxButton *button_remove;
    wxButton *button_search;
    wxStaticText *label_search_results;
    wxCheckBox *checkbox_ddl;
    wxSplitterWindow *splitter1;
    wxPanel *top_splitter_panel;
    wxListCtrl *listctrl_results;
    wxPanel *bottom_splitter_panel;
    wxTextCtrl *stc_ddl;

public:
    enum
    {
        ID_button_remove=100,
        ID_button_start,
        ID_checkbox_ddl,
        ID_listctrl_results
    };

    AdvancedSearchFrame(wxWindow *parent);
};
//-----------------------------------------------------------------------------
#endif
