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

#ifndef FR_TEMPORALTABLEDIALOG_H
#define FR_TEMPORALTABLEDIALOG_H

#include "gui/BaseDialog.h"
#include "metadata/table.h"

class TemporalTableDialog: public BaseDialog
{
private:
    Table* tableM;

    wxStaticText* label_mode;
    wxChoice* choice_mode;

    wxStaticText* label_start_col;
    wxTextCtrl* textctrl_start_col;
    wxStaticText* label_end_col;
    wxTextCtrl* textctrl_end_col;
    wxStaticText* label_timestamp1;
    wxTextCtrl* textctrl_timestamp1;
    wxStaticText* label_timestamp2;
    wxTextCtrl* textctrl_timestamp2;

    wxStaticText* label_preview;
    wxTextCtrl* textctrl_preview;

    wxButton* button_ok;
    wxButton* button_cancel;

    void createControls();
    void layoutControls();
    void setControlsProperties();
    void updatePreview();

protected:
    virtual const wxString getName() const override;

public:
    TemporalTableDialog(wxWindow* parent, Table* table);
    wxString getGeneratedSql() const;

private:
    enum {
        ID_choice_mode = 400,
        ID_textctrl_start_col,
        ID_textctrl_end_col,
        ID_textctrl_timestamp1,
        ID_textctrl_timestamp2
    };

    void OnControlChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif
