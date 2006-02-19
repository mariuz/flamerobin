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

  The Initial Developer of the Original Code is Michael Hieke.

  Portions created by the original developer
  are Copyright (C) 2006 Michael Hieke.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/
//-----------------------------------------------------------------------------
#ifndef FR_CREATEINDEXDIALOG_H
#define FR_CREATEINDEXDIALOG_H

#include <wx/wx.h>

#include "core/Observer.h"
#include "gui/BaseDialog.h"

class Table;
//-----------------------------------------------------------------------------
class CreateIndexDialog: public BaseDialog 
{
private:
    Table* tableM;

    wxStaticText* label_name;
    wxTextCtrl* textctrl_name;
    wxCheckBox* checkbox_unique;
    wxRadioBox* radiobox_order;
    wxStaticText* label_columns;
    wxListBox* listbox_columns;
    wxButton* button_ok;
    wxButton* button_cancel;

    void createControls();
    void layoutControls();
    void setControlsProperties();

    wxString getSelectedColumnsList();
    void updateButtons();
protected:
    virtual const wxString getName() const;
public:
    CreateIndexDialog(wxWindow* parent, Table *table);
    // creation of statement execution frame outside of dialog class
    const wxString getStatementsToExecute();
private:
    // event handling
    enum {
        ID_textcontrol_name = 100,
        ID_check_unique,
        ID_radio_order,
        ID_list_columns
    };
    void OnControlChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // FR_CREATEINDEXDIALOG_H
