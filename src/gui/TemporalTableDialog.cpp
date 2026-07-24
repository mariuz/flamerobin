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

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "gui/StyleGuide.h"
#include "gui/TemporalTableDialog.h"
#include "metadata/TemporalTableHelper.h"

BEGIN_EVENT_TABLE(TemporalTableDialog, BaseDialog)
    EVT_CHOICE(TemporalTableDialog::ID_choice_mode, TemporalTableDialog::OnControlChange)
    EVT_TEXT(TemporalTableDialog::ID_textctrl_start_col, TemporalTableDialog::OnControlChange)
    EVT_TEXT(TemporalTableDialog::ID_textctrl_end_col, TemporalTableDialog::OnControlChange)
    EVT_TEXT(TemporalTableDialog::ID_textctrl_timestamp1, TemporalTableDialog::OnControlChange)
    EVT_TEXT(TemporalTableDialog::ID_textctrl_timestamp2, TemporalTableDialog::OnControlChange)
END_EVENT_TABLE()

TemporalTableDialog::TemporalTableDialog(wxWindow* parent, Table* table)
    : BaseDialog(parent, -1, _("Temporal Table & Historical Query Generator")),
      tableM(table)
{
    createControls();
    layoutControls();
    setControlsProperties();
    updatePreview();
}

void TemporalTableDialog::createControls()
{
    label_mode = new wxStaticText(this, wxID_ANY, _("Operation Mode:"));
    wxArrayString modes;
    modes.Add("Define System-Time Period (DDL)");
    modes.Add("Historical Point-In-Time Query (AS OF)");
    modes.Add("Historical Range Query (BETWEEN)");
    choice_mode = new wxChoice(this, ID_choice_mode, wxDefaultPosition, wxDefaultSize, modes);
    choice_mode->SetSelection(0);

    label_start_col = new wxStaticText(this, wxID_ANY, _("Row Start Column:"));
    textctrl_start_col = new wxTextCtrl(this, ID_textctrl_start_col, "SYS_START");

    label_end_col = new wxStaticText(this, wxID_ANY, _("Row End Column:"));
    textctrl_end_col = new wxTextCtrl(this, ID_textctrl_end_col, "SYS_END");

    label_timestamp1 = new wxStaticText(this, wxID_ANY, _("Start Timestamp / As-Of:"));
    textctrl_timestamp1 = new wxTextCtrl(this, ID_textctrl_timestamp1, "2026-01-01 00:00:00");

    label_timestamp2 = new wxStaticText(this, wxID_ANY, _("End Timestamp:"));
    textctrl_timestamp2 = new wxTextCtrl(this, ID_textctrl_timestamp2, "2026-12-31 23:59:59");

    label_preview = new wxStaticText(this, wxID_ANY, _("Generated SQL Statement:"));
    textctrl_preview = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

    button_ok = new wxButton(this, wxID_OK, _("Execute"));
    button_cancel = new wxButton(this, wxID_CANCEL, _("Cancel"));
}

void TemporalTableDialog::layoutControls()
{
    wxFlexGridSizer* sizerInputs = new wxFlexGridSizer(5, 2, styleguide().getRelatedControlMargin(wxVERTICAL), styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerInputs->AddGrowableCol(1, 1);

    sizerInputs->Add(label_mode, 0, wxALIGN_CENTER_VERTICAL);
    sizerInputs->Add(choice_mode, 1, wxEXPAND);

    sizerInputs->Add(label_start_col, 0, wxALIGN_CENTER_VERTICAL);
    sizerInputs->Add(textctrl_start_col, 1, wxEXPAND);

    sizerInputs->Add(label_end_col, 0, wxALIGN_CENTER_VERTICAL);
    sizerInputs->Add(textctrl_end_col, 1, wxEXPAND);

    sizerInputs->Add(label_timestamp1, 0, wxALIGN_CENTER_VERTICAL);
    sizerInputs->Add(textctrl_timestamp1, 1, wxEXPAND);

    sizerInputs->Add(label_timestamp2, 0, wxALIGN_CENTER_VERTICAL);
    sizerInputs->Add(textctrl_timestamp2, 1, wxEXPAND);

    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);
    sizerMain->Add(sizerInputs, 0, wxEXPAND);
    sizerMain->AddSpacer(styleguide().getUnrelatedControlMargin(wxVERTICAL));

    sizerMain->Add(label_preview, 0, wxEXPAND);
    sizerMain->AddSpacer(styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerMain->Add(textctrl_preview, 1, wxEXPAND);

    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok, button_cancel);
    layoutSizers(sizerMain, sizerButtons, true);
}

void TemporalTableDialog::setControlsProperties()
{
    button_ok->SetDefault();
}

const wxString TemporalTableDialog::getName() const
{
    return "TemporalTableDialog";
}

wxString TemporalTableDialog::getGeneratedSql() const
{
    wxString tableName = tableM ? tableM->getName_() : "TABLE_NAME";
    int mode = choice_mode->GetSelection();

    if (mode == 0)
    {
        return TemporalTableHelper::generateTemporalTableDDL(tableName, textctrl_start_col->GetValue(), textctrl_end_col->GetValue());
    }
    else if (mode == 1)
    {
        return TemporalTableHelper::generateHistoricalAsOfQuery(tableName, textctrl_timestamp1->GetValue());
    }
    else
    {
        return TemporalTableHelper::generateHistoricalBetweenQuery(tableName, textctrl_timestamp1->GetValue(), textctrl_timestamp2->GetValue());
    }
}

void TemporalTableDialog::updatePreview()
{
    textctrl_preview->SetValue(getGeneratedSql());
}

void TemporalTableDialog::OnControlChange(wxCommandEvent& WXUNUSED(event))
{
    updatePreview();
}
