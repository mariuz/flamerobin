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

#include "gui/StyleGuide.h"
#include "gui/UsernamePasswordDialog.h"
#include "metadata/database.h"

UsernamePasswordDialog::UsernamePasswordDialog(wxWindow* parentWindow,
    const wxString& description, const wxString& username, int flags)
    : BaseDialog(parentWindow, wxID_ANY, _("Connection Credentials"))
{
    wxWindow* panel = getControlsPanel();
    // create controls
    if (!description.empty())
        labelDescriptionM = new wxStaticText(panel, wxID_ANY, description);
    else
        labelDescriptionM = 0;
    radioUsernamePasswordM = new wxRadioButton(panel, wxID_ANY,
        _("Connect with the following &username and password"));
    labelUsernameM = new wxStaticText(panel, wxID_ANY, _("&Username:"));
    textUsernameM = new wxTextCtrl(panel, wxID_ANY, username);
    textUsernameM->SetEditable((flags & AllowOtherUsername) != 0);
    labelPasswordM = new wxStaticText(panel, wxID_ANY, _("&Password:"));
    textPasswordM = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    radioTrustedUserM = new wxRadioButton(panel, wxID_ANY,
        _("Connect as me (&trusted user authentication"));
    radioTrustedUserM->Enable((flags & AllowTrustedUser) != 0);
    buttonOkM = new wxButton(panel, wxID_OK, _("Connect"));
    buttonCancelM = new wxButton(panel, wxID_CANCEL, _("Cancel"));
    layoutControls();
    if (textUsernameM->IsEditable() && username.size() == 0)
        textUsernameM->SetFocus();
    else
        textPasswordM->SetFocus();
    updateColors();
    updateButton();
    Connect(wxEVT_COMMAND_RADIOBUTTON_SELECTED,
        wxCommandEventHandler(UsernamePasswordDialog::OnSettingsChange));
    Connect(wxEVT_COMMAND_TEXT_UPDATED,
        wxCommandEventHandler(UsernamePasswordDialog::OnSettingsChange));
}

wxString UsernamePasswordDialog::getUsername()
{
    if (radioTrustedUserM->GetValue())
        return wxEmptyString;
    return textUsernameM->GetValue();
}

wxString UsernamePasswordDialog::getPassword()
{
    if (radioTrustedUserM->GetValue())
        return wxEmptyString;
    return textPasswordM->GetValue();
}

void UsernamePasswordDialog::layoutControls()
{
    // create sizer for controls
    wxBoxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    if (labelDescriptionM)
    {
        sizerControls->Add(labelDescriptionM, 0, wxEXPAND);
        sizerControls->AddSpacer(
            styleguide().getUnrelatedControlMargin(wxVERTICAL));
    }

    sizerControls->Add(radioUsernamePasswordM, 0, wxEXPAND);
    sizerControls->AddSpacer(
        styleguide().getRelatedControlMargin(wxVERTICAL));

    wxFlexGridSizer* sizerGrid = new wxFlexGridSizer(2, 3,
        styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin());
    sizerGrid->AddGrowableCol(2);

    sizerGrid->AddSpacer(textUsernameM->GetSize().GetHeight());
    sizerGrid->Add(labelUsernameM, 1, wxALIGN_CENTER_VERTICAL);
    sizerGrid->Add(textUsernameM, 2, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    sizerGrid->AddSpacer(textPasswordM->GetSize().GetHeight());
    sizerGrid->Add(labelPasswordM, 1, wxALIGN_CENTER_VERTICAL);
    sizerGrid->Add(textPasswordM, 2, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(sizerGrid, 0, wxEXPAND);
    sizerControls->AddSpacer(
        styleguide().getUnrelatedControlMargin(wxVERTICAL));

    sizerControls->Add(radioTrustedUserM, 0, wxEXPAND);

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(buttonOkM,
        buttonCancelM);
    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons);
}

void UsernamePasswordDialog::updateButton()
{
    bool oldEnabled = buttonOkM->IsEnabled();
    bool newEnabled = ((textUsernameM->GetValue().size() > 0)
        && (textPasswordM->GetValue().size() > 0))
        || radioTrustedUserM->GetValue();
    buttonOkM->Enable(newEnabled);
    if (!oldEnabled && newEnabled)
        buttonOkM->SetDefault();
}

void UsernamePasswordDialog::OnSettingsChange(wxCommandEvent& WXUNUSED(event))
{
    updateButton();
}

bool getConnectionCredentials(wxWindow* parentWindow, DatabasePtr database,
    wxString& username, wxString& password, int flags)
{
    wxASSERT(database);
    username.clear();
    password.clear();
    DatabaseAuthenticationMode dam = database->getAuthenticationMode();
    if (dam.getIgnoreUsernamePassword())
        return true;

    username = database->getUsername();
    password = database->getDecryptedPassword();
    if (!dam.getAlwaysAskForPassword() && !password.empty())
        return true;
    return getConnectionCredentials(parentWindow, wxEmptyString,
        username, password, flags);
}

bool getConnectionCredentials(wxWindow* parentWindow,
    const wxString& description, wxString& username, wxString& password,
    int flags)
{
    UsernamePasswordDialog upd(parentWindow, description, username, flags);
    if (upd.ShowModal() != wxID_OK)
        return false;
    username = upd.getUsername();
    password = upd.getPassword();
    return true;
}

