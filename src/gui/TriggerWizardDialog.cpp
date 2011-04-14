/*
  Copyright (c) 2004-2011 The FlameRobin Development Team

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


  $Id$

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

#include "core/ArtProvider.h"
#include "core/URIProcessor.h"
#include "gui/ExecuteSql.h"
#include "gui/GUIURIHandlerHelper.h"
#include "gui/StyleGuide.h"
#include "gui/TriggerWizardDialog.h"
#include "metadata/database.h"
#include "metadata/MetadataItemURIHandlerHelper.h"
#include "metadata/table.h"
//-----------------------------------------------------------------------------
TriggerWizardDialog::TriggerWizardDialog(wxWindow* parent, MetadataItem *item):
    BaseDialog(parent, -1, wxEmptyString)
{
    relationM = item;
    label_1 = new wxStaticText(getControlsPanel(), -1, _("Trigger name"));
    text_ctrl_1 = new wxTextCtrl(getControlsPanel(), -1, wxT(""));
    checkbox_1_copy = new wxCheckBox(getControlsPanel(), -1, wxT("Active"));
    const wxString radio_box_1_copy_choices[] = { wxT("Before"), wxT("After") };
    radio_box_1_copy = new wxRadioBox(getControlsPanel(), -1, _("Trigger type"), wxDefaultPosition, wxDefaultSize, 2, radio_box_1_copy_choices, 0, wxRA_SPECIFY_ROWS);
    checkbox_insert = new wxCheckBox(getControlsPanel(), -1, wxT("INSERT"));
    checkbox_update = new wxCheckBox(getControlsPanel(), -1, wxT("UPDATE"));
    checkbox_delete = new wxCheckBox(getControlsPanel(), -1, wxT("DELETE"));
    label_2 = new wxStaticText(getControlsPanel(), -1, _("Position"));
    spin_ctrl_1 = new wxSpinCtrl(getControlsPanel(), -1, wxT("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100);
    button_ok = new wxButton(getControlsPanel(), wxID_OK, _("&OK"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("&Cancel"));

    set_properties();
    do_layout();

    SetIcon(wxArtProvider::GetIcon(ART_Trigger, wxART_FRAME_ICON));
    text_ctrl_1->SetFocus();
}
//-----------------------------------------------------------------------------
void TriggerWizardDialog::set_properties()
{
    SetTitle(_("Creating new trigger for: ") + relationM->getName_());
    radio_box_1_copy->SetSelection(0);
    checkbox_1_copy->SetValue(true);
    button_ok->SetDefault();
}
//-----------------------------------------------------------------------------
void TriggerWizardDialog::do_layout()
{
    wxBoxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* sizer_3 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_5 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* sizer_position = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_4 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* sizer_2 = new wxBoxSizer(wxHORIZONTAL);
    sizer_2->Add(label_1, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, styleguide().getControlLabelMargin());
    sizer_2->Add(text_ctrl_1, 1, wxALL|wxFIXED_MINSIZE, styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizer_4->Add(checkbox_1_copy, 0, wxALL|wxFIXED_MINSIZE, styleguide().getCheckboxSpacing());
    sizer_4->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizer_4->Add(radio_box_1_copy, 0, wxALL|wxEXPAND|wxFIXED_MINSIZE, 0);
    sizer_position->Add(label_2, 0, wxALL|wxFIXED_MINSIZE, styleguide().getControlLabelMargin());
    sizer_position->Add(spin_ctrl_1, 1, wxFIXED_MINSIZE, 0);
    sizer_5->Add(checkbox_insert, 0, wxALL|wxFIXED_MINSIZE, 0);
    sizer_5->Add(0, styleguide().getCheckboxSpacing());
    sizer_5->Add(checkbox_update, 0, wxALL|wxFIXED_MINSIZE, 0);
    sizer_5->Add(0, styleguide().getCheckboxSpacing());
    sizer_5->Add(checkbox_delete, 0, wxALL|wxFIXED_MINSIZE, 0);
    sizer_5->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizer_5->Add(sizer_position, 1, wxEXPAND, 0);
    sizer_3->Add(sizer_4, 1, wxEXPAND, 0);
    sizer_3->Add(styleguide().getUnrelatedControlMargin(wxHORIZONTAL), 0);
    sizer_3->Add(sizer_5, 1, wxEXPAND, 0);
    sizerControls->Add(sizer_2, 0, wxEXPAND, 0);
    sizerControls->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerControls->Add(sizer_3, 0, wxEXPAND, 0);

    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok, button_cancel);
    layoutSizers(sizerControls, sizerButtons);
}
//-----------------------------------------------------------------------------
const wxString TriggerWizardDialog::getName() const
{
    return wxT("TriggerWizardDialog");
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(TriggerWizardDialog, BaseDialog)
    EVT_BUTTON(wxID_OK, TriggerWizardDialog::OnOkButtonClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void TriggerWizardDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    if (getSqlStatement().IsEmpty())
    {
        wxMessageBox(_("Please select INSERT, UPDATE or DELETE checkbox."),
            _("No action specified."), wxOK|wxICON_INFORMATION);
    }
    else
        EndModal(wxID_OK);
}
//-----------------------------------------------------------------------------
wxString TriggerWizardDialog::getSqlStatement() const
{
    wxString insupdel, suffix;
    if (checkbox_insert->IsChecked())
    {
        insupdel = wxT(" INSERT");
        suffix = wxT("I");
    }
    if (checkbox_update->IsChecked())
    {
        if (!insupdel.IsEmpty())
            insupdel += wxT(" OR");
        insupdel += wxT(" UPDATE");
        suffix += wxT("U");
    }
    if (checkbox_delete->IsChecked())
    {
        if (!insupdel.IsEmpty())
            insupdel += wxT(" OR");
        insupdel += wxT(" DELETE");
        suffix += wxT("D");
    }
    if (insupdel.IsEmpty())
        return wxEmptyString;

    bool isBefore = (radio_box_1_copy->GetSelection() == 0);
    wxString name(Identifier::userString(text_ctrl_1->GetValue()));
    if (name.IsEmpty())
    {
        wxString sname = relationM->getName_() + wxT("_")
            + (isBefore ? wxT("B") : wxT("A")) + suffix
            + wxString::Format(wxT("%d"), spin_ctrl_1->GetValue());
        Identifier iname(sname);
        name = iname.getQuoted();
    }

    wxString sql = wxT("SET TERM ^ ;\n\nCREATE TRIGGER ") + name +
        wxT(" FOR ") + relationM->getQuotedName() + wxT("\n") +
        (checkbox_1_copy->IsChecked() ? wxEmptyString : wxT("IN")) +
        wxT("ACTIVE ") + (isBefore ? wxT("BEFORE") : wxT("AFTER")) +
        insupdel + wxString::Format(wxT(" POSITION %d"),
        spin_ctrl_1->GetValue()) + wxT("\nAS \nBEGIN \n\t/* enter trigger code here */ \nEND^\n\nSET TERM ; ^ \n");
    return sql;
}
//-----------------------------------------------------------------------------
class CreateTriggerHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    CreateTriggerHandler() {};
    bool handleURI(URI& uri);
private:
    static const CreateTriggerHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const CreateTriggerHandler CreateTriggerHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool CreateTriggerHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("create_trigger"))
        return false;

    Table* t = extractMetadataItemFromURI<Table>(uri);
    wxWindow* w = getParentWindow(uri);
    if (!t || !w)
        return true;

    TriggerWizardDialog twd(w, t);
    // NOTE: this has been moved here from OnOkButtonClick() to make frame
    //       activation work properly.  Basically activation of another
    //       frame has to happen outside wxDialog::ShowModal(), because it
    //       does at the end re-focus the last focused control, raising
    //       the parent frame over the newly created sql execution frame
    if (twd.ShowModal() == wxID_OK)
    {
        wxString statement(twd.getSqlStatement());
        if (!statement.IsEmpty())
            showSql(w, _("Creating new trigger"), t->getDatabase(), statement);
    }
    return true;
}
//-----------------------------------------------------------------------------
