/*
Copyright (c) 2006 The FlameRobin Development Team

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

#include <wx/gbsizer.h>

#include "frutils.h"
#include "gui/ProgressDialog.h"
#include "gui/StyleGuide.h"
#include "gui/UserDialog.h"
#include "metadata/server.h"
#include "urihandler.h"
//-----------------------------------------------------------------------------
UserDialog::UserDialog(wxWindow* parent, const wxString& title, bool isNewUser)
    : BaseDialog(parent, wxID_ANY, title), isNewUserM(isNewUser), userM(0)
{
    createControls();
    setControlsProperties();
    layoutControls();
    updateButtons();
}
//-----------------------------------------------------------------------------
void UserDialog::createControls()
{
    labelUserNameM = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("User name:"));
    textUserNameM = new wxTextCtrl(getControlsPanel(), ID_textctrl_username);
    labelFirstNameM = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("First name:"));
    textFirstNameM = new wxTextCtrl(getControlsPanel(), wxID_ANY);
    labelMiddleNameM = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("Middle name:"));
    textMiddleNameM = new wxTextCtrl(getControlsPanel(), wxID_ANY);
    labelLastNameM = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("Last name:"));
    textLastNameM = new wxTextCtrl(getControlsPanel(), wxID_ANY);

    labelPasswordM = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("Password:"));
    textPasswordM = new wxTextCtrl(getControlsPanel(), ID_textctrl_password,
        wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    labelConfirmPasswordM = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("Confirm password:"));
    textConfirmPasswordM = new wxTextCtrl(getControlsPanel(),
        ID_textctrl_confirmpw, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        wxTE_PASSWORD);
    labelUserIdM = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("Unix user ID:"));
    spinctrlUserIdM = new wxSpinCtrl(getControlsPanel(), wxID_ANY, wxT("0"),
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 30000, 0);
    labelGroupIdM = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("Unix group ID:"));
    spinctrlGroupIdM = new wxSpinCtrl(getControlsPanel(), wxID_ANY, wxT("0"),
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 30000, 0);

    buttonOkM = new wxButton(getControlsPanel(), wxID_SAVE,
        (isNewUserM) ? _("Create") : _("Save"));
    buttonCancelM = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
}
//-----------------------------------------------------------------------------
const wxString UserDialog::getName() const
{
    return wxT("UserDialog");
}
//-----------------------------------------------------------------------------
void UserDialog::layoutControls()
{
    int dx = styleguide().getUnrelatedControlMargin(wxHORIZONTAL)
        - styleguide().getControlLabelMargin();
    if (dx < 0)
        dx = 0;

    // create sizer for controls
    wxGridBagSizer* controlsSizer = new wxGridBagSizer(
        styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin());

    controlsSizer->Add(labelUserNameM, wxGBPosition(0, 0), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL);
    controlsSizer->Add(textUserNameM, wxGBPosition(0, 1), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL | wxEXPAND);
    controlsSizer->Add(labelFirstNameM, wxGBPosition(1, 0), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL);
    controlsSizer->Add(textFirstNameM, wxGBPosition(1, 1), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL | wxEXPAND);
    controlsSizer->Add(labelMiddleNameM, wxGBPosition(2, 0), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL);
    controlsSizer->Add(textMiddleNameM, wxGBPosition(2, 1), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL | wxEXPAND);
    controlsSizer->Add(labelLastNameM, wxGBPosition(3, 0), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL);
    controlsSizer->Add(textLastNameM, wxGBPosition(3, 1), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL | wxEXPAND);

    controlsSizer->Add(labelPasswordM, wxGBPosition(0, 2), wxDefaultSpan,
        wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    controlsSizer->Add(textPasswordM, wxGBPosition(0, 3), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL | wxEXPAND);
    controlsSizer->Add(labelConfirmPasswordM, wxGBPosition(1, 2),
        wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    controlsSizer->Add(textConfirmPasswordM, wxGBPosition(1, 3),
        wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    controlsSizer->Add(labelUserIdM, wxGBPosition(2, 2),
        wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    controlsSizer->Add(spinctrlUserIdM, wxGBPosition(2, 3),
        wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    controlsSizer->Add(labelGroupIdM, wxGBPosition(3, 2),
        wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    controlsSizer->Add(spinctrlGroupIdM, wxGBPosition(3, 3),
        wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);

    controlsSizer->AddGrowableCol(1);

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* buttonSizer = styleguide().createButtonSizer(buttonOkM,
        buttonCancelM);
    // use method in base class to set everything up
    layoutSizers(controlsSizer, buttonSizer);
}
//-----------------------------------------------------------------------------
void UserDialog::setControlsProperties()
{
    textUserNameM->SetEditable(isNewUserM);
    buttonOkM->SetDefault();
}
//-----------------------------------------------------------------------------
void UserDialog::setUser(User* u)
{
    wxASSERT(u != 0);
    userM = u;
    if (!isNewUserM)
    {
        textUserNameM->SetValue(u->usernameM);
        textFirstNameM->SetValue(u->firstnameM);
        textMiddleNameM->SetValue(u->middlenameM);
        textLastNameM->SetValue(u->lastnameM);
        textPasswordM->SetValue(u->passwordM);
        textConfirmPasswordM->SetValue(u->passwordM);
        spinctrlGroupIdM->SetValue(u->groupidM);
        spinctrlUserIdM->SetValue(u->useridM);
    }

    updateButtons();
    updateColors();
}
//-----------------------------------------------------------------------------
void UserDialog::updateButtons()
{
    wxString passwd = textPasswordM->GetValue();
    wxString confirm = textConfirmPasswordM->GetValue();
    buttonOkM->Enable(!textUserNameM->GetValue().IsEmpty()
        && !passwd.IsEmpty() && passwd == confirm);
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(UserDialog, BaseDialog)
    EVT_TEXT(UserDialog::ID_textctrl_username, UserDialog::OnSettingsChange)
    EVT_TEXT(UserDialog::ID_textctrl_password, UserDialog::OnSettingsChange)
    EVT_TEXT(UserDialog::ID_textctrl_confirmpw, UserDialog::OnSettingsChange)
    EVT_BUTTON(wxID_SAVE, UserDialog::OnOkButtonClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void UserDialog::OnSettingsChange(wxCommandEvent& WXUNUSED(event))
{
    // will allready be called in constructor!
    if (IsShown())
        updateButtons();
}
//-----------------------------------------------------------------------------
void UserDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    userM->usernameM = textUserNameM->GetValue();
    userM->firstnameM = textFirstNameM->GetValue();
    userM->middlenameM = textMiddleNameM->GetValue();
    userM->lastnameM = textLastNameM->GetValue();
    userM->passwordM = textPasswordM->GetValue();
    userM->groupidM = spinctrlGroupIdM->GetValue();
    userM->useridM = spinctrlUserIdM->GetValue();
    EndModal(wxID_OK);
}
//-----------------------------------------------------------------------------
class UserPropertiesHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const UserPropertiesHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const UserPropertiesHandler UserPropertiesHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool UserPropertiesHandler::handleURI(URI& uri)
{
    bool addUser = uri.action == wxT("add_user");
    bool editUser = uri.action == wxT("edit_user");
    if (!addUser && !editUser)
        return false;

    wxWindow* w = getWindow(uri);
    User* u = 0;
    Server* s;
    wxString title(_("Modify User"));
    if (addUser)
    {
        s = (Server *)getObject(uri);
        title = _("Create New User");
    }
    else
    {
        u = (User *)getObject(uri);
        if (!u)
            return true;
        s = dynamic_cast<Server *>(u->getParent());
    }
    if (!s)
        return true;

    User tempusr(s);
    if (addUser)
        u = &tempusr;

    UserDialog d(w, title, addUser);
    d.setUser(u);
    if (d.ShowModal() == wxID_OK)
    {
        ProgressDialog pd(w, _("Connecting to Server..."), 1);
        IBPP::Service svc;
        if (!getService(s, svc, &pd, true)) // true = need SYSDBA password
            return true;

        try
        {
            IBPP::User user;
            u->setIBPP(user);
            if (addUser)
                svc->AddUser(user);
            else
                svc->ModifyUser(user);
            s->notifyObservers();
        }
        catch(IBPP::Exception& e)
        {
            wxMessageBox(std2wx(e.ErrorMessage()), _("Error"),
                wxOK | wxICON_WARNING);
        }
    }
    return true;
}
//-----------------------------------------------------------------------------
class DropUserHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const DropUserHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const DropUserHandler DropUserHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool DropUserHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("drop_user"))
        return false;

    wxWindow* w = getWindow(uri);
    User* u = (User*)getObject(uri);
    if (!u || !w)
        return true;
    Server* s = dynamic_cast<Server*>(u->getParent());
    if (!s)
        return true;

    if (wxNO == wxMessageBox(_("Are you sure?"), _("Removing User"),
        wxYES_NO|wxICON_QUESTION))
        return true;

    ProgressDialog pd(w, _("Connecting to Server..."), 1);
    IBPP::Service svc;
    if (!getService(s, svc, &pd, true)) // true = need SYSDBA password
        return true;

    try
    {
        svc->RemoveUser(wx2std(u->usernameM));
        s->notifyObservers();
    }
    catch(IBPP::Exception& e)
    {
        wxMessageBox(std2wx(e.ErrorMessage()), _("Error"),
            wxOK | wxICON_WARNING);
    }
    return true;
}
//-----------------------------------------------------------------------------
