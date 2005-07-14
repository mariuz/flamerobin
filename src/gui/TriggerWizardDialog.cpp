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
  are Copyright (C) 2005 Milan Babuskov.

  All Rights Reserved.

  Contributor(s):
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

#include "metadata/metadataitem.h"
#include "ExecuteSqlFrame.h"
#include "ugly.h"
#include "styleguide.h"
#include "urihandler.h"
#include "TriggerWizardDialog.h"
//-----------------------------------------------------------------------------
TriggerWizardDialog::TriggerWizardDialog(wxWindow* parent, YxMetadataItem *item):
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
}
//-----------------------------------------------------------------------------
void TriggerWizardDialog::set_properties()
{
    SetTitle(_("Creating new trigger for: ") + std2wx(relationM->getName()));
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
const std::string TriggerWizardDialog::getName() const
{
    return "TriggerWizardDialog";
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(TriggerWizardDialog, BaseDialog)
	EVT_BUTTON(wxID_OK, TriggerWizardDialog::OnOkButtonClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void TriggerWizardDialog::OnOkButtonClick(wxCommandEvent& event)
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

	bool isBefore = (radio_box_1_copy->GetSelection() == 0);
	wxString name = text_ctrl_1->GetValue();
	wxString relationName = std2wx(relationM->getName());
	if (name.IsEmpty())
	{
		name = relationName + wxT("_") + (isBefore ? wxT("B") : wxT("A")) + suffix
			+ wxString::Format(wxT("%d"), spin_ctrl_1->GetValue());
	}

	wxString sql = wxT("SET TERM ^ ;\n\nCREATE TRIGGER ") + name + wxT(" FOR ") + relationName + wxT("\n") +
		(checkbox_1_copy->IsChecked() ? wxEmptyString : wxT("IN")) + wxT("ACTIVE ") +
		(isBefore ? wxT("BEFORE") : wxT("AFTER")) +
		insupdel + wxString::Format(wxT(" POSITION %d"), spin_ctrl_1->GetValue()) +
		wxT("\nAS \nBEGIN \n\t/* enter trigger code here */ \nEND^\n\nSET TERM ; ^ \n");

	ExecuteSqlFrame *eff = new ExecuteSqlFrame(GetParent(), -1, wxString(_("Creating new trigger")));
	eff->setDatabase(relationM->getDatabase());
	eff->setSql(sql);
	eff->Show();
	event.Skip();	// let the dialog close
}
//-----------------------------------------------------------------------------
class CreateTriggerHandler: public YxURIHandler
{
public:
	bool handleURI(std::string& uriStr);
private:
    static const CreateTriggerHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const CreateTriggerHandler CreateTriggerHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool CreateTriggerHandler::handleURI(std::string& uriStr)
{
    YURI uriObj(uriStr);
	if (uriObj.action != "create_trigger")
		return false;

	YTable *t = (YTable *)getObject(uriObj);
	wxWindow *w = getWindow(uriObj);
	if (!t || !w)
		return true;

	TriggerWizardDialog *tw = new TriggerWizardDialog(w, t);
	tw->Show();
	return true;
}
//-----------------------------------------------------------------------------
