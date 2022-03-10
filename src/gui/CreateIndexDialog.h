/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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

#ifndef FR_CREATEINDEXDIALOG_H
#define FR_CREATEINDEXDIALOG_H

#include <wx/wx.h>

#include "core/Observer.h"
#include "gui/BaseDialog.h"

class Table;

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

#endif // FR_CREATEINDEXDIALOG_H
