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

#ifndef FR_REORDERFIELDSDIALOG_H
#define FR_REORDERFIELDSDIALOG_H

#include <wx/wx.h>

#include "core/Observer.h"
#include "gui/BaseDialog.h"

class Table;

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

    // observer stuff
    virtual void subjectRemoved(Subject* subject);
    virtual void update();
protected:
    virtual const wxString getName() const;
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

#endif // FR_REORDERFIELDSDIALOG_H
