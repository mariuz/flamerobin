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
    label_fieldname = new wxStaticText(getControlsPanel(), -1, 
        _("Field name:"));
    text_ctrl_fieldname = new wxTextCtrl(getControlsPanel(), 
        ID_textctrl_fieldname, wxEmptyString);
// TODO

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
    wxGridBagSizer* sizerControls = new wxGridBagSizer(styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin());

    sizerControls->Add(label_fieldname, wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(text_ctrl_fieldname, wxGBPosition(0, 1), wxGBSpan(1, 5), wxALIGN_CENTER_VERTICAL | wxEXPAND);

    sizerControls->AddGrowableCol(1);
    sizerControls->AddGrowableCol(3);
    sizerControls->AddGrowableCol(5);

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok, button_cancel);
    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons);
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
