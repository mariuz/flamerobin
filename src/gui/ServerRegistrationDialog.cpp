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

Contributor(s): Michael Hieke
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

#include "ServerRegistrationDialog.h"
#include "frutils.h"
#include "styleguide.h"
#include "ugly.h"

//-----------------------------------------------------------------------------
ServerRegistrationDialog::ServerRegistrationDialog(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    BaseDialog(parent, id, title, pos, size, style)
{
    label_hostname = new wxStaticText(this, -1, _("Hostname:"));
    text_ctrl_hostname = new wxTextCtrl(this, ID_textctrl_hostname, wxT("localhost"));
    label_portnumber = new wxStaticText(this, -1, _("Port number:"));
    text_ctrl_portnumber = new wxTextCtrl(this, -1, wxT("3050"));
    button_ok = new wxButton(this, ID_button_ok, _("Save"));
    button_cancel = new wxButton(this, ID_button_cancel, _("Cancel"));

    set_properties();
    do_layout();
    updateButtons();
}
//-----------------------------------------------------------------------------
//! implementation details
void ServerRegistrationDialog::do_layout()
{
#if wxCHECK_VERSION(2, 5, 3)
    // create sizer for controls
    wxFlexGridSizer* sizerControls = new wxFlexGridSizer(2, 2, 
        styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin());
    sizerControls->AddGrowableCol(1);

    sizerControls->Add(label_hostname, 0, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(text_ctrl_hostname, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(label_portnumber, 0, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(text_ctrl_portnumber, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL);
#else
    // make all labels have the same width to simulate a grid of controls
    std::list<wxWindow*> controls;
    controls.push_back(label_hostname);
    controls.push_back(label_portnumber);
    adjustControlsMinWidth(controls);
    controls.clear();

    // create sizers hierarchy for controls
    wxBoxSizer* sizerRow1 = new wxBoxSizer(wxHORIZONTAL);
    sizerRow1->Add(label_hostname, 0, wxALIGN_CENTER_VERTICAL);
    sizerRow1->Add(styleguide().getControlLabelMargin(), 0);
    sizerRow1->Add(text_ctrl_hostname, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* sizerRow2 = new wxBoxSizer(wxHORIZONTAL);
    sizerRow2->Add(label_portnumber, 0, wxALIGN_CENTER_VERTICAL);
    sizerRow2->Add(styleguide().getControlLabelMargin(), 0);
    sizerRow2->Add(text_ctrl_portnumber, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    sizerControls->Add(sizerRow1, 0, wxEXPAND);
    sizerControls->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerControls->Add(sizerRow2, 0, wxEXPAND);
#endif

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok, button_cancel);

    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons);
}
//-----------------------------------------------------------------------------
const std::string ServerRegistrationDialog::getName() const
{
    return "ServerRegistrationDialog";
}
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::set_properties()
{
    button_ok->SetDefault();
}
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::setServer(YServer *s)
{
    serverM = s;

    text_ctrl_hostname->SetValue(std2wx(serverM->getHostname()));
    text_ctrl_portnumber->SetValue(std2wx(serverM->getPort()));

    // enable controls depending on operation and database connection status
    // use SetEditable() for edit controls to allow copying text to clipboard
    bool isConnected = serverM->hasConnectedDatabase();
    text_ctrl_hostname->SetEditable(!isConnected);
    text_ctrl_portnumber->SetEditable(!isConnected);
    button_ok->Enable(!isConnected);
    if (isConnected)
    {
        button_cancel->SetLabel(_("Close"));
        button_cancel->SetDefault();
    }
    updateButtons();
}
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::updateButtons()
{
    button_ok->Enable(text_ctrl_hostname->IsEditable() 
        && !text_ctrl_hostname->GetValue().IsEmpty());
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(ServerRegistrationDialog, BaseDialog)
    EVT_TEXT(ServerRegistrationDialog::ID_textctrl_hostname, ServerRegistrationDialog::OnSettingsChange)
    EVT_BUTTON(ServerRegistrationDialog::ID_button_ok, ServerRegistrationDialog::OnOkButtonClick)
    EVT_BUTTON(ServerRegistrationDialog::ID_button_cancel, ServerRegistrationDialog::OnCancelButtonClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::OnSettingsChange(wxCommandEvent& WXUNUSED(event))
{
    // will allready be called in constructor!
    if (IsShown())
        updateButtons();
}
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    serverM->setHostname(wx2std(text_ctrl_hostname->GetValue()));
    serverM->setPort(wx2std(text_ctrl_portnumber->GetValue()));
    EndModal(wxOK);
}
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::OnCancelButtonClick(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxCANCEL);
}
//-----------------------------------------------------------------------------
