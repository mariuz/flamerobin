/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "gui/ServerRegistrationDialog.h"
#include "gui/StyleGuide.h"
#include "metadata/server.h"

// constructor for registration of a new server
ServerRegistrationDialog::ServerRegistrationDialog(wxWindow* parent,
        const wxString& title)
    : BaseDialog(parent, wxID_ANY, title), serverM(new Server())
{
    createControls(true);
    setControlsProperties();
    layoutControls();
    connectEvents();
}

// constructor for showing / editing the registration of a registered server
ServerRegistrationDialog::ServerRegistrationDialog(wxWindow* parent,
        const wxString& title, ServerPtr server)
    : BaseDialog(parent, wxID_ANY, title), serverM(server)
{
    createControls(false);
    setControlsProperties();
    layoutControls();
    connectEvents();
}

ServerPtr ServerRegistrationDialog::getServer() const
{
    return serverM;
}

//! implementation details
const wxString ServerRegistrationDialog::buildDefaultName() const
{
    return Server::makeConnectionString(text_ctrl_hostname->GetValue(),
        text_ctrl_portnumber->GetValue());
}

void ServerRegistrationDialog::connectEvents()
{
    Connect(text_ctrl_name->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
        wxCommandEventHandler(ServerRegistrationDialog::OnNameChange));
    Connect(text_ctrl_hostname->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
        wxCommandEventHandler(ServerRegistrationDialog::OnSettingsChange));
    Connect(text_ctrl_portnumber->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
        wxCommandEventHandler(ServerRegistrationDialog::OnSettingsChange));
    Connect(button_ok->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
        wxCommandEventHandler(ServerRegistrationDialog::OnOkButtonClick));
}

void ServerRegistrationDialog::createControls(bool registerServer)
{
    label_name = new wxStaticText(getControlsPanel(), wxID_ANY,
        _("Display name:"));
    text_ctrl_name = new wxTextCtrl(getControlsPanel(), wxID_ANY);
    label_hostname = new wxStaticText(getControlsPanel(), wxID_ANY,
        _("Hostname:"));
    text_ctrl_hostname = new wxTextCtrl(getControlsPanel(), wxID_ANY);
    label_portnumber = new wxStaticText(getControlsPanel(), wxID_ANY,
        _("Port number:"));
    text_ctrl_portnumber = new wxTextCtrl(getControlsPanel(), wxID_ANY);

    button_ok = new wxButton(getControlsPanel(), wxID_SAVE,
        (registerServer) ? _("Register") : _("Save"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
}

const wxString ServerRegistrationDialog::getName() const
{
    return "ServerRegistrationDialog";
}

void ServerRegistrationDialog::layoutControls()
{
    // create sizer for controls
    wxFlexGridSizer* sizerControls = new wxFlexGridSizer(3, 2,
        styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin()
    );
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

void ServerRegistrationDialog::setControlsProperties()
{
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

    button_ok->SetDefault();
}

void ServerRegistrationDialog::updateButtons()
{
    button_ok->Enable(!text_ctrl_name->GetValue().IsEmpty());
}

void ServerRegistrationDialog::updateIsDefaultName()
{
    wxString name(text_ctrl_name->GetValue());
    isDefaultNameM = (name.empty() || name == buildDefaultName());
}

void ServerRegistrationDialog::OnNameChange(wxCommandEvent& WXUNUSED(event))
{
    updateIsDefaultName();
    updateButtons();
}

void ServerRegistrationDialog::OnSettingsChange(wxCommandEvent& WXUNUSED(event))
{
    if (isDefaultNameM)
        text_ctrl_name->SetValue(buildDefaultName());
    updateIsDefaultName();
    updateButtons();
}

void ServerRegistrationDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    serverM->setName_(text_ctrl_name->GetValue());
    serverM->setHostname(text_ctrl_hostname->GetValue());
    serverM->setPort(text_ctrl_portnumber->GetValue());
    EndModal(wxID_OK);
}

