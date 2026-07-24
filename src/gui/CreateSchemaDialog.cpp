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

#include "gui/CreateSchemaDialog.h"
#include "gui/ExecuteSqlFrame.h"
#include "gui/StyleGuide.h"
#include "sql/Identifier.h"

BEGIN_EVENT_TABLE(CreateSchemaDialog, BaseDialog)
    EVT_TEXT(CreateSchemaDialog::ID_textcontrol_name, CreateSchemaDialog::OnControlChange)
    EVT_TEXT(CreateSchemaDialog::ID_textcontrol_owner, CreateSchemaDialog::OnControlChange)
END_EVENT_TABLE()

CreateSchemaDialog::CreateSchemaDialog(wxWindow* parent, DatabasePtr database, const wxString& existingSchemaName)
    : BaseDialog(parent, -1, existingSchemaName.IsEmpty() ? _("Create Schema") : _("Alter Schema")),
      databaseM(database), oldSchemaNameM(existingSchemaName), isAlterM(!existingSchemaName.IsEmpty())
{
    createControls();
    layoutControls();
    setControlsProperties();
    updatePreview();
    updateButtons();
}

void CreateSchemaDialog::createControls()
{
    label_name = new wxStaticText(this, wxID_ANY, _("Schema Name:"));
    textctrl_name = new wxTextCtrl(this, ID_textcontrol_name, oldSchemaNameM);

    label_owner = new wxStaticText(this, wxID_ANY, _("Owner / Authorization:"));
    textctrl_owner = new wxTextCtrl(this, ID_textcontrol_owner, wxEmptyString);

    label_preview = new wxStaticText(this, wxID_ANY, _("Generated DDL Statement:"));
    textctrl_preview = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

    button_ok = new wxButton(this, wxID_OK, _("Execute"));
    button_cancel = new wxButton(this, wxID_CANCEL, _("Cancel"));
}

void CreateSchemaDialog::layoutControls()
{
    wxFlexGridSizer* sizerInputs = new wxFlexGridSizer(2, 2, styleguide().getRelatedControlMargin(wxVERTICAL), styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerInputs->AddGrowableCol(1, 1);

    sizerInputs->Add(label_name, 0, wxALIGN_CENTER_VERTICAL);
    sizerInputs->Add(textctrl_name, 1, wxEXPAND);

    sizerInputs->Add(label_owner, 0, wxALIGN_CENTER_VERTICAL);
    sizerInputs->Add(textctrl_owner, 1, wxEXPAND);

    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);
    sizerMain->Add(sizerInputs, 0, wxEXPAND);
    sizerMain->AddSpacer(styleguide().getUnrelatedControlMargin(wxVERTICAL));

    sizerMain->Add(label_preview, 0, wxEXPAND);
    sizerMain->AddSpacer(styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerMain->Add(textctrl_preview, 1, wxEXPAND);

    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok, button_cancel);
    layoutSizers(sizerMain, sizerButtons, true);
}

void CreateSchemaDialog::setControlsProperties()
{
    button_ok->SetDefault();
}

const wxString CreateSchemaDialog::getName() const
{
    return "CreateSchemaDialog";
}

const wxString CreateSchemaDialog::getStatementsToExecute() const
{
    wxString name = textctrl_name->GetValue().Trim();
    wxString owner = textctrl_owner->GetValue().Trim();
    Identifier idName(name);
    Identifier idOwner(owner);

    if (isAlterM)
    {
        wxString sql;
        Identifier idOld(oldSchemaNameM);
        if (!name.IsEmpty() && name != oldSchemaNameM)
        {
            sql += "ALTER SCHEMA " + idOld.getQuoted() + " RENAME TO " + idName.getQuoted() + ";\n";
        }
        if (!owner.IsEmpty())
        {
            sql += "ALTER SCHEMA " + (name.IsEmpty() ? idOld.getQuoted() : idName.getQuoted()) + " OWNER TO " + idOwner.getQuoted() + ";\n";
        }
        return sql;
    }
    else
    {
        wxString sql = "CREATE SCHEMA " + idName.getQuoted();
        if (!owner.IsEmpty())
            sql += " AUTHORIZATION " + idOwner.getQuoted();
        sql += ";\n";
        return sql;
    }
}

void CreateSchemaDialog::updatePreview()
{
    textctrl_preview->SetValue(getStatementsToExecute());
}

void CreateSchemaDialog::updateButtons()
{
    button_ok->Enable(!textctrl_name->GetValue().Trim().IsEmpty());
}

void CreateSchemaDialog::OnControlChange(wxCommandEvent& WXUNUSED(event))
{
    updatePreview();
    updateButtons();
}
