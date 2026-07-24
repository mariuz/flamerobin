/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#ifndef FR_CREATESCHEMADIALOG_H
#define FR_CREATESCHEMADIALOG_H

#include "gui/BaseDialog.h"
#include "metadata/database.h"

class CreateSchemaDialog: public BaseDialog
{
private:
    DatabasePtr databaseM;
    wxString oldSchemaNameM;
    bool isAlterM;

    wxStaticText* label_name;
    wxTextCtrl* textctrl_name;
    wxStaticText* label_owner;
    wxTextCtrl* textctrl_owner;
    wxStaticText* label_preview;
    wxTextCtrl* textctrl_preview;

    wxButton* button_ok;
    wxButton* button_cancel;

    void createControls();
    void layoutControls();
    void setControlsProperties();
    void updatePreview();
    void updateButtons();

protected:
    virtual const wxString getName() const override;

public:
    CreateSchemaDialog(wxWindow* parent, DatabasePtr database, const wxString& existingSchemaName = "");
    const wxString getStatementsToExecute() const;

private:
    enum {
        ID_textcontrol_name = 200,
        ID_textcontrol_owner
    };
    void OnControlChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif
