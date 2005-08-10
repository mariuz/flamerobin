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

  Contributor(s): Michael Hieke
*/

#ifndef REORDERFIELDSDIALOG_H
#define REORDERFIELDSDIALOG_H

#include <wx/wx.h>
#include <wx/image.h>

#include <string>
#include "BaseDialog.h"
#include "observer.h"
#include "metadata/table.h"

class ReorderFieldsDialog: public BaseDialog, public Observer {
public:
    enum {
        ID_list_box_fields = 100,
        ID_button_up,
        ID_button_down,
        ID_button_first,
        ID_button_last,
        ID_button_ok,
        ID_button_cancel = wxID_CANCEL
    };

    void OnListBoxSelChange(wxCommandEvent& event);
    void OnOkButtonClick(wxCommandEvent& event);
    void OnDownButtonClick(wxCommandEvent& event);
    void OnFirstButtonClick(wxCommandEvent& event);
    void OnLastButtonClick(wxCommandEvent& event);
    void OnUpButtonClick(wxCommandEvent& event);

    ReorderFieldsDialog(wxWindow* parent, Table *table);

private:
	Table *tableM;

    void do_layout();
    void moveSelected(int moveby);
    void set_properties();
protected:
    wxListBox* list_box_fields;
    wxBitmapButton* button_first;
    wxBitmapButton* button_up;
    wxBitmapButton* button_down;
    wxBitmapButton* button_last;
    wxButton* button_ok;
    wxButton* button_cancel;

    virtual const std::string getName() const;
	void removeObservedObject(Subject *object);
	void update();
    void updateButtons();

    DECLARE_EVENT_TABLE()
};

#endif // REORDERFIELDSDIALOG_H
