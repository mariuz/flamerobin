/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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

#include "gui/ServerRegistrationDialog.h"
#include "metadata/server.h"
#include "styleguide.h"
//-----------------------------------------------------------------------------
ServerRegistrationDialog::ServerRegistrationDialog(wxWindow* parent,
        const wxString& title, bool registerServer)
    : BaseDialog(parent, wxID_ANY, title)
{
    serverM = 0;
    isDefaultNameM = true;
    isNewServerM = registerServer;

    createControls();
    setControlsProperties();
    layoutControls();
    updateButtons();
}
//-----------------------------------------------------------------------------
//! implementation details
const wxString ServerRegistrationDialog::buildDefaultName() const
{
    Server helper;
    helper.setHostname(text_ctrl_hostname->GetValue());
    helper.setPort(text_ctrl_portnumber->GetValue());
    return helper.getConnectionString();
}
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::createControls()
{
    label_name = new wxStaticText(getControlsPanel(), -1, _("Display name:"));
    text_ctrl_name = new wxTextCtrl(getControlsPanel(), ID_textctrl_name,
        wxEmptyString);
    label_hostname = new wxStaticText(getControlsPanel(), -1, _("Hostname:"));
    text_ctrl_hostname = new wxTextCtrl(getControlsPanel(),
        ID_textctrl_hostname, wxT("localhost"));
    label_portnumber = new wxStaticText(getControlsPanel(), -1,
        _("Port number:"));
    text_ctrl_portnumber = new wxTextCtrl(getControlsPanel(),
        ID_textctrl_portnumber, wxEmptyString);
    button_ok = new wxButton(getControlsPanel(), wxID_SAVE,
        (isNewServerM) ? _("Register") : _("Save"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
}
//-----------------------------------------------------------------------------
const wxString ServerRegistrationDialog::getName() const
{
    return wxT("ServerRegistrationDialog");
}
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::layoutControls()
{
    // create sizer for controls
    wxFlexGridSizer* sizerControls = new wxFlexGridSizer(3, 2,
        styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin());
    sizerControls->AddGrowableCol(1);

    sizerControls->Add(label_name, 0, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(text_ctrl_name, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(label_hostname, 0, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(text_ctrl_hostname, 1,
        wxEXPAND | wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(label_portnumber, 0, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(text_ctrl_portnumber, 1,
        wxEXPAND | wxALIGN_CENTER_VERTICAL);

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok,
        button_cancel);
    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons);
}
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::setControlsProperties()
{
    button_ok->SetDefault();
}
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::setServer(Server *s)
{
    serverM = s;

    text_ctrl_name->SetValue(serverM->getName_());
    text_ctrl_hostname->SetValue(serverM->getHostname());
    text_ctrl_portnumber->SetValue(serverM->getPort());

    // see whether the server has an empty or default name; knowing that will be
    // useful to keep the name in sync when other attributes change.
    updateIsDefaultName();

    // enable controls depending on database connection status
    // use SetEditable() for edit controls to allow copying text to clipboard
    bool isConnected = serverM->hasConnectedDatabase();
    text_ctrl_hostname->SetEditable(!isConnected);
    text_ctrl_portnumber->SetEditable(!isConnected);
    updateButtons();
    updateColors();
}
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::updateButtons()
{
    button_ok->Enable(!text_ctrl_name->GetValue().IsEmpty());
}
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::updateIsDefaultName()
{
    isDefaultNameM = (text_ctrl_name->GetValue().IsEmpty()
        || text_ctrl_name->GetValue() == buildDefaultName());
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(ServerRegistrationDialog, BaseDialog)
    EVT_TEXT(ServerRegistrationDialog::ID_textctrl_name, ServerRegistrationDialog::OnNameChange)
    EVT_TEXT(ServerRegistrationDialog::ID_textctrl_hostname, ServerRegistrationDialog::OnSettingsChange)
    EVT_TEXT(ServerRegistrationDialog::ID_textctrl_portnumber, ServerRegistrationDialog::OnSettingsChange)
    EVT_BUTTON(wxID_SAVE, ServerRegistrationDialog::OnOkButtonClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::OnNameChange(wxCommandEvent& WXUNUSED(event))
{
    // will allready be called in constructor!
    if (IsShown())
    {
        updateIsDefaultName();
        updateButtons();
    }
}
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::OnSettingsChange(wxCommandEvent& WXUNUSED(event))
{
    // will allready be called in constructor!
    if (IsShown())
    {
        if (isDefaultNameM)
            text_ctrl_name->SetValue(buildDefaultName());
        updateIsDefaultName();
        updateButtons();
    }
}
//-----------------------------------------------------------------------------
void ServerRegistrationDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    serverM->setName_(text_ctrl_name->GetValue());
    serverM->setHostname(text_ctrl_hostname->GetValue());
    serverM->setPort(text_ctrl_portnumber->GetValue());
    EndModal(wxID_OK);
}
//-----------------------------------------------------------------------------
