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
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Michael Hieke
*/

#ifndef FR_REORDERFIELDSDIALOG_H
#define FR_REORDERFIELDSDIALOG_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>

#include "core/Observer.h"
#include "gui/BaseDialog.h"

class Table;
//-----------------------------------------------------------------------------
class ReorderFieldsDialog: public BaseDialog, public Observer {
private:
    Table* tableM;

    wxListBox* list_box_fields;
    wxBitmapButton* button_first;
    wxBitmapButton* button_up;
    wxBitmapButton* button_down;
    wxBitmapButton* button_last;
    wxButton* button_ok;
    wxButton* button_cancel;

    void createControls();
    void layoutControls();

    void moveSelected(int moveby);
    void updateButtons();
protected:
    virtual const wxString getName() const;
    virtual void removeSubject(Subject* subject);
    virtual void update();
public:
    ReorderFieldsDialog(wxWindow* parent, Table *table);
    // creation of statement execution frame outside of dialog class
    const wxString getStatementsToExecute();
private:
    // event handling
    enum {
        ID_list_box_fields = 100,
        ID_button_up,
        ID_button_down,
        ID_button_first,
        ID_button_last
    };
    void OnListBoxSelChange(wxCommandEvent& event);
    void OnDownButtonClick(wxCommandEvent& event);
    void OnFirstButtonClick(wxCommandEvent& event);
    void OnLastButtonClick(wxCommandEvent& event);
    void OnUpButtonClick(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // FR_REORDERFIELDSDIALOG_H
