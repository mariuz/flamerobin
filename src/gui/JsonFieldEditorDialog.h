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

#ifndef FR_JSONFIELDEDITORDIALOG_H
#define FR_JSONFIELDEDITORDIALOG_H

#include "gui/BaseDialog.h"

class JsonFieldEditorDialog: public BaseDialog
{
private:
    wxString columnNameM;

    wxStaticText* label_editor;
    wxTextCtrl* textctrl_json;
    wxButton* button_format;
    wxButton* button_validate;
    wxStaticText* label_status;

    wxStaticText* label_path;
    wxTextCtrl* textctrl_path;
    wxStaticText* label_expr_type;
    wxChoice* choice_expr_type;
    wxStaticText* label_expr_preview;
    wxTextCtrl* textctrl_expr_preview;

    wxButton* button_ok;
    wxButton* button_cancel;

    void createControls();
    void layoutControls();
    void setControlsProperties();
    void updateExpressionPreview();

protected:
    virtual const wxString getName() const override;

public:
    JsonFieldEditorDialog(wxWindow* parent, const wxString& columnName, const wxString& initialJson = "");
    wxString getJsonText() const;

private:
    enum {
        ID_button_format = 300,
        ID_button_validate,
        ID_textctrl_json,
        ID_textctrl_path,
        ID_choice_expr_type
    };

    void OnFormatClick(wxCommandEvent& event);
    void OnValidateClick(wxCommandEvent& event);
    void OnControlChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif
