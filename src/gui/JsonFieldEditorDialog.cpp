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

#include "core/JsonExpressionHelper.h"
#include "gui/JsonFieldEditorDialog.h"
#include "gui/StyleGuide.h"

BEGIN_EVENT_TABLE(JsonFieldEditorDialog, BaseDialog)
    EVT_BUTTON(JsonFieldEditorDialog::ID_button_format, JsonFieldEditorDialog::OnFormatClick)
    EVT_BUTTON(JsonFieldEditorDialog::ID_button_validate, JsonFieldEditorDialog::OnValidateClick)
    EVT_TEXT(JsonFieldEditorDialog::ID_textctrl_json, JsonFieldEditorDialog::OnControlChange)
    EVT_TEXT(JsonFieldEditorDialog::ID_textctrl_path, JsonFieldEditorDialog::OnControlChange)
    EVT_CHOICE(JsonFieldEditorDialog::ID_choice_expr_type, JsonFieldEditorDialog::OnControlChange)
END_EVENT_TABLE()

JsonFieldEditorDialog::JsonFieldEditorDialog(wxWindow* parent, const wxString& columnName, const wxString& initialJson)
    : BaseDialog(parent, -1, _("JSON & Document Field Editor")),
      columnNameM(columnName)
{
    createControls();
    layoutControls();
    setControlsProperties();
    if (!initialJson.IsEmpty())
        textctrl_json->SetValue(initialJson);
    updateExpressionPreview();
}

void JsonFieldEditorDialog::createControls()
{
    label_editor = new wxStaticText(this, wxID_ANY, _("JSON Document Editor:"));
    textctrl_json = new wxTextCtrl(this, ID_textctrl_json, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);

    button_format = new wxButton(this, ID_button_format, _("Format JSON"));
    button_validate = new wxButton(this, ID_button_validate, _("Validate JSON"));
    label_status = new wxStaticText(this, wxID_ANY, _("Status: Ready"));

    label_path = new wxStaticText(this, wxID_ANY, _("JSON Path Expression:"));
    textctrl_path = new wxTextCtrl(this, ID_textctrl_path, "$.id");

    label_expr_type = new wxStaticText(this, wxID_ANY, _("Firebird 6 Expression:"));
    wxArrayString exprChoices;
    exprChoices.Add("JSON_VALUE");
    exprChoices.Add("JSON_QUERY");
    exprChoices.Add("JSON_EXISTS");
    choice_expr_type = new wxChoice(this, ID_choice_expr_type, wxDefaultPosition, wxDefaultSize, exprChoices);
    choice_expr_type->SetSelection(0);

    label_expr_preview = new wxStaticText(this, wxID_ANY, _("Generated SQL Expression:"));
    textctrl_expr_preview = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);

    button_ok = new wxButton(this, wxID_OK, _("Save"));
    button_cancel = new wxButton(this, wxID_CANCEL, _("Cancel"));
}

void JsonFieldEditorDialog::layoutControls()
{
    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);

    sizerMain->Add(label_editor, 0, wxEXPAND);
    sizerMain->AddSpacer(styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerMain->Add(textctrl_json, 1, wxEXPAND);
    sizerMain->AddSpacer(styleguide().getRelatedControlMargin(wxVERTICAL));

    wxBoxSizer* sizerToolBar = new wxBoxSizer(wxHORIZONTAL);
    sizerToolBar->Add(button_format, 0, wxRIGHT, styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerToolBar->Add(button_validate, 0, wxRIGHT, styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerToolBar->Add(label_status, 1, wxALIGN_CENTER_VERTICAL);
    sizerMain->Add(sizerToolBar, 0, wxEXPAND);
    sizerMain->AddSpacer(styleguide().getUnrelatedControlMargin(wxVERTICAL));

    wxFlexGridSizer* sizerExpr = new wxFlexGridSizer(2, 2, styleguide().getRelatedControlMargin(wxVERTICAL), styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerExpr->AddGrowableCol(1, 1);
    sizerExpr->Add(label_path, 0, wxALIGN_CENTER_VERTICAL);
    sizerExpr->Add(textctrl_path, 1, wxEXPAND);
    sizerExpr->Add(label_expr_type, 0, wxALIGN_CENTER_VERTICAL);
    sizerExpr->Add(choice_expr_type, 1, wxEXPAND);
    sizerMain->Add(sizerExpr, 0, wxEXPAND);
    sizerMain->AddSpacer(styleguide().getRelatedControlMargin(wxVERTICAL));

    sizerMain->Add(label_expr_preview, 0, wxEXPAND);
    sizerMain->AddSpacer(styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerMain->Add(textctrl_expr_preview, 0, wxEXPAND);

    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok, button_cancel);
    layoutSizers(sizerMain, sizerButtons, true);
}

void JsonFieldEditorDialog::setControlsProperties()
{
    button_ok->SetDefault();
}

const wxString JsonFieldEditorDialog::getName() const
{
    return "JsonFieldEditorDialog";
}

wxString JsonFieldEditorDialog::getJsonText() const
{
    return textctrl_json->GetValue();
}

void JsonFieldEditorDialog::updateExpressionPreview()
{
    wxString path = textctrl_path->GetValue();
    int sel = choice_expr_type->GetSelection();
    wxString expr;
    if (sel == 0)
        expr = JsonExpressionHelper::generateJsonValue(columnNameM, path);
    else if (sel == 1)
        expr = JsonExpressionHelper::generateJsonQuery(columnNameM, path);
    else
        expr = JsonExpressionHelper::generateJsonExists(columnNameM, path);

    textctrl_expr_preview->SetValue(expr);
}

void JsonFieldEditorDialog::OnFormatClick(wxCommandEvent& WXUNUSED(event))
{
    wxString formatted = JsonExpressionHelper::formatJson(textctrl_json->GetValue());
    textctrl_json->SetValue(formatted);
    label_status->SetLabel(_("Status: Formatted JSON"));
}

void JsonFieldEditorDialog::OnValidateClick(wxCommandEvent& WXUNUSED(event))
{
    wxString msg;
    bool valid = JsonExpressionHelper::validateJson(textctrl_json->GetValue(), msg);
    if (valid)
        label_status->SetLabel(_("Status: Valid JSON"));
    else
        label_status->SetLabel(_("Status Error: ") + msg);
}

void JsonFieldEditorDialog::OnControlChange(wxCommandEvent& WXUNUSED(event))
{
    updateExpressionPreview();
}
