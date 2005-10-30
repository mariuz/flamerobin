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

  Contributor(s): Nando Dessena, Michael Hieke
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/gbsizer.h>

#include "gui/FieldPropertiesDialog.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "styleguide.h"
#include "ugly.h"
#include "urihandler.h"
//-----------------------------------------------------------------------------
FieldPropertiesDialog::FieldPropertiesDialog(wxWindow* parent, Table* table, 
        Column* column)
    : BaseDialog(parent, wxID_ANY, wxEmptyString)
{
    // can't do anything if no table is given
    wxASSERT(table);

    tableM = 0;
    columnM = 0;

    createControls();
    setTableM(table);
    setColumnM(column);
    setControlsProperties();
    layoutControls();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::createControls()
{
    label_fieldname = new wxStaticText(getControlsPanel(), wxID_ANY,
        _("Field name:"));
    textctrl_fieldname = new wxTextCtrl(getControlsPanel(), 
        ID_textctrl_fieldname, wxEmptyString);
    
    label_domain = new wxStaticText(getControlsPanel(), wxID_ANY,
        _("Domain:"));
    choice_domain = new wxChoice(getControlsPanel(), ID_choice_domain, 
        wxDefaultPosition, wxDefaultSize, 0, 0);
    button_edit_domain = new wxButton(getControlsPanel(), 
        ID_button_edit_domain, _("Edit domain"));

    label_datatype = new wxStaticText(getControlsPanel(), wxID_ANY, 
        _("Datatype:"));
    const wxString datatypes_choices[] = {
        wxT("Char"), wxT("Varchar"), wxT("Integer"), wxT("Smallint"),
        wxT("Numeric"), wxT("Decimal"), wxT("BigInt"), wxT("Float"),
        wxT("Double precision"), wxT("Date"), wxT("Time"), wxT("Timestamp"),
        wxT("Array"), wxT("Blob")
    };
    choice_datatype = new wxChoice(getControlsPanel(), ID_choice_datatype, 
        wxDefaultPosition, wxDefaultSize, 
        sizeof(datatypes_choices) / sizeof(wxString), datatypes_choices);
    label_size = new wxStaticText(getControlsPanel(), wxID_ANY, _("Size:"));
    textctrl_size = new wxTextCtrl(getControlsPanel(), wxID_ANY, wxEmptyString);
    label_scale = new wxStaticText(getControlsPanel(), wxID_ANY, _("Scale:"));
    textctrl_scale = new wxTextCtrl(getControlsPanel(), wxID_ANY, wxEmptyString);

    checkbox_notnull = new wxCheckBox(getControlsPanel(), wxID_ANY, _("Not null"));
    label_charset = new wxStaticText(getControlsPanel(), wxID_ANY, _("Charset:"));
    const wxString charset_choices[] = { 
        wxT("NONE")
    };
    choice_charset = new wxChoice(getControlsPanel(), ID_choice_charset, 
        wxDefaultPosition, wxDefaultSize, 
        sizeof(charset_choices) / sizeof(wxString), charset_choices);
    label_collate = new wxStaticText(getControlsPanel(), wxID_ANY, _("Collate:"));
    choice_collate = new wxChoice(getControlsPanel(), ID_choice_collate, 
        wxDefaultPosition, wxDefaultSize, 0, 0);

    static_line_autoinc = new wxStaticLine(getControlsPanel());
    label_autoinc = new wxStaticText(getControlsPanel(), wxID_ANY, _("Autoincrement"));

    radio_generator_new = new wxRadioButton(getControlsPanel(), 
        ID_radio_generator_new, _("Create new generator:"));
    textctrl_generator_name = new wxTextCtrl(getControlsPanel(), 
        ID_textctrl_generator_name, wxEmptyString);

    radio_generator_existing = new wxRadioButton(getControlsPanel(), 
        ID_radio_generator_existing, _("Use existing generator:"));
    choice_generator = new wxChoice(getControlsPanel(), ID_choice_generator,
        wxDefaultPosition, wxDefaultSize, 0, 0);

    checkbox_trigger = new wxCheckBox(getControlsPanel(), ID_checkbox_trigger,
        _("Create trigger"));
    textctrl_sql = new wxTextCtrl(getControlsPanel(), wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

    button_ok = new wxButton(getControlsPanel(), ID_button_ok, _("Save"));
    button_cancel = new wxButton(getControlsPanel(), ID_button_cancel, 
        _("Cancel"));
}
//-----------------------------------------------------------------------------
const wxString FieldPropertiesDialog::getName() const
{
    return wxT("FieldPropertiesDialog");
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::layoutControls()
{
    // create sizer for controls
    wxGridBagSizer* sizerTop = new wxGridBagSizer(styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin());

    sizerTop->Add(label_fieldname, wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerTop->Add(textctrl_fieldname, wxGBPosition(0, 1), wxGBSpan(1, 5), wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_domain, wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* sizerDomain = new wxBoxSizer(wxHORIZONTAL);
    sizerDomain->Add(choice_domain, 1, wxEXPAND);
    sizerDomain->Add(styleguide().getBrowseButtonMargin(), 0);
    sizerDomain->Add(button_edit_domain);
    sizerTop->Add(sizerDomain, wxGBPosition(1, 1), wxGBSpan(1, 5), wxALIGN_CENTER_VERTICAL | wxEXPAND);

    int dx = styleguide().getUnrelatedControlMargin(wxHORIZONTAL) - styleguide().getControlLabelMargin();
    if (dx < 0)
        dx = 0;

    sizerTop->Add(label_datatype, wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerTop->Add(choice_datatype, wxGBPosition(2, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_size, wxGBPosition(2, 2), wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerTop->Add(textctrl_size, wxGBPosition(2, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_scale, wxGBPosition(2, 4), wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerTop->Add(textctrl_scale, wxGBPosition(2, 5), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_charset, wxGBPosition(3, 2), wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerTop->Add(choice_charset, wxGBPosition(3, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_collate, wxGBPosition(3, 4), wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerTop->Add(choice_collate, wxGBPosition(3, 5), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);

    sizerTop->AddGrowableCol(1);
    sizerTop->AddGrowableCol(3);
    sizerTop->AddGrowableCol(5);

    wxGridBagSizer* sizerGenerator = new wxGridBagSizer(styleguide().getRelatedControlMargin(wxHORIZONTAL),
        styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerGenerator->Add(radio_generator_new, wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerGenerator->Add(textctrl_generator_name, wxGBPosition(0, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerGenerator->Add(radio_generator_existing, wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerGenerator->Add(choice_generator, wxGBPosition(1, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerGenerator->AddGrowableCol(1);

    // stack everything vertically
    wxBoxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    sizerControls->Add(sizerTop, 0, wxEXPAND);
    sizerControls->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerControls->Add(checkbox_notnull);
    sizerControls->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerControls->Add(static_line_autoinc, 0, wxEXPAND);
    sizerControls->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerControls->Add(label_autoinc);
    sizerControls->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerControls->Add(sizerGenerator, 0, wxEXPAND);
    sizerControls->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerControls->Add(checkbox_trigger);
    sizerControls->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerControls->Add(textctrl_sql, 1, wxEXPAND);

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok, button_cancel);
    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons, true);
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::removeSubject(Subject* subject)
{
    Observer::removeSubject(subject);
    if ((subject) && ((subject == tableM) || (subject == columnM)))
    {
        if (subject == tableM)
            setTableM(0);
        if (subject == columnM)
            setColumnM(0);
        EndModal(wxID_CANCEL);
    }
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::setColumnM(Column* column)
{
    if (columnM != column)
    {
        if (columnM)
            columnM->detachObserver(this);
        columnM = column;
        if (columnM)
            columnM->attachObserver(this);
        updateControlsFromColumn();
    }
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::setControlsProperties()
{
    // set dialog title
    wxString fmt;
    if (columnM)
        fmt = _("Table %s: Field Properties");
    else
        fmt = _("Table %s: Create New Field");
    SetTitle(wxString::Format(fmt, tableM->getName().c_str()));

    // bold font for "Autoincrement" label
    wxFont font(label_autoinc->GetFont());
    font.SetWeight(wxBOLD);
    label_autoinc->SetFont(font);
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::setTableM(Table* table)
{
    if (tableM != table)
    {
        if (tableM)
            tableM->detachObserver(this);
        tableM = table;
        if (tableM)
            tableM->attachObserver(this);
        updateControlsFromTable();
    }
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::update()
{
    updateControlsFromTable();
    updateControlsFromColumn();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::updateControlsFromColumn()
{
    if (!(columnM))
        return;
// TODO
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::updateControlsFromTable()
{
    if (!(tableM))
        return;
// TODO
}
//-----------------------------------------------------------------------------
