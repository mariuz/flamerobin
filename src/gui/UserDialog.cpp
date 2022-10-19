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

#include <wx/gbsizer.h>

#include "core/StringUtils.h"
#include "frutils.h"
#ifdef __WXGTK__
    #include "gui/AdvancedMessageDialog.h"
#endif

#include "gui/GUIURIHandlerHelper.h"
#include "gui/ProgressDialog.h"
#include "gui/StyleGuide.h"
#include "gui/UserDialog.h"
#include "metadata/MetadataItemURIHandlerHelper.h"
#include "metadata/server.h"
#include "core/URIProcessor.h"

UserDialog::UserDialog(wxWindow* parent, const wxString& title, bool isNewUser)
    : BaseDialog(parent, wxID_ANY, title), isNewUserM(isNewUser)
{
    createControls();
    setControlsProperties();
    layoutControls();
    updateButtons();
}

void UserDialog::createControls()
{
    labelUserNameM = new wxStaticText(getControlsPanel(), wxID_ANY,
        "User name:");
    textUserNameM = new wxTextCtrl(getControlsPanel(), ID_textctrl_username);
    labelFirstNameM = new wxStaticText(getControlsPanel(), wxID_ANY,
        "First name:");
    textFirstNameM = new wxTextCtrl(getControlsPanel(), wxID_ANY);
    labelMiddleNameM = new wxStaticText(getControlsPanel(), wxID_ANY,
        "Middle name:");
    textMiddleNameM = new wxTextCtrl(getControlsPanel(), wxID_ANY);
    labelLastNameM = new wxStaticText(getControlsPanel(), wxID_ANY,
        "Last name:");
    textLastNameM = new wxTextCtrl(getControlsPanel(), wxID_ANY);

    labelPasswordM = new wxStaticText(getControlsPanel(), wxID_ANY,
        "Password:");
    textPasswordM = new wxTextCtrl(getControlsPanel(), ID_textctrl_password,
        wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    labelConfirmPasswordM = new wxStaticText(getControlsPanel(), wxID_ANY,
        "Confirm password:");
    textConfirmPasswordM = new wxTextCtrl(getControlsPanel(),
        ID_textctrl_confirmpw, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        wxTE_PASSWORD);
    labelUserIdM = new wxStaticText(getControlsPanel(), wxID_ANY,
        "Unix user ID:");
    spinctrlUserIdM = new wxSpinCtrl(getControlsPanel(), wxID_ANY, "0",
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 30000, 0);
    labelGroupIdM = new wxStaticText(getControlsPanel(), wxID_ANY,
        "Unix group ID:");
    spinctrlGroupIdM = new wxSpinCtrl(getControlsPanel(), wxID_ANY, "0",
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 30000, 0);

    buttonOkM = new wxButton(getControlsPanel(), wxID_SAVE,
        (isNewUserM) ? _("Create") : _("Save"));
    buttonCancelM = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
}

const wxString UserDialog::getName() const
{
    return "UserDialog";
}

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

void UserDialog::setControlsProperties()
{
    textUserNameM->SetEditable(isNewUserM);
    buttonOkM->SetDefault();
}

void UserDialog::setUser(UserPtr user)
{
    wxASSERT(user);
    userM = user;
    if (!isNewUserM)
    {
        textUserNameM->SetValue(user->getUsername());
        textFirstNameM->SetValue(user->getFirstName());
        textMiddleNameM->SetValue(user->getMiddleName());
        textLastNameM->SetValue(user->getLastName());
        textPasswordM->SetValue(user->getPassword());
        textConfirmPasswordM->SetValue(user->getPassword());
        spinctrlGroupIdM->SetValue(user->getGroupId());
        spinctrlUserIdM->SetValue(user->getUserId());
    }

    updateButtons();
    updateColors();
}

void UserDialog::updateButtons()
{
    wxString passwd = textPasswordM->GetValue();
    wxString confirm = textConfirmPasswordM->GetValue();
    buttonOkM->Enable(!textUserNameM->GetValue().IsEmpty()
        && !passwd.IsEmpty() && passwd == confirm);
}

//! event handling
BEGIN_EVENT_TABLE(UserDialog, BaseDialog)
    EVT_TEXT(UserDialog::ID_textctrl_username, UserDialog::OnSettingsChange)
    EVT_TEXT(UserDialog::ID_textctrl_password, UserDialog::OnSettingsChange)
    EVT_TEXT(UserDialog::ID_textctrl_confirmpw, UserDialog::OnSettingsChange)
    EVT_BUTTON(wxID_SAVE, UserDialog::OnOkButtonClick)
END_EVENT_TABLE()

void UserDialog::OnSettingsChange(wxCommandEvent& WXUNUSED(event))
{
    // will allready be called in constructor!
    if (IsShown())
        updateButtons();
}

void UserDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    SubjectLocker locker(userM.get());
    userM->setUsername(textUserNameM->GetValue());
    userM->setFirstName(textFirstNameM->GetValue());
    userM->setMiddleName(textMiddleNameM->GetValue());
    userM->setLastName(textLastNameM->GetValue());
    userM->setPassword(textPasswordM->GetValue());
    userM->setGroupId(spinctrlGroupIdM->GetValue());
    userM->setUserId(spinctrlUserIdM->GetValue());
    EndModal(wxID_OK);
}

class UserPropertiesHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    UserPropertiesHandler() {};
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const UserPropertiesHandler handlerInstance;
};

const UserPropertiesHandler UserPropertiesHandler::handlerInstance;

bool UserPropertiesHandler::handleURI(URI& uri)
{
    bool addUser = uri.action == "add_user";
    bool editUser = uri.action == "edit_user";
    if (!addUser && !editUser)
        return false;

    wxWindow* w = getParentWindow(uri);
    ServerPtr server;
    UserPtr user;
    wxString title(_("Modify User"));
    if (addUser)
    {
        server = extractMetadataItemPtrFromURI<Server>(uri);
        if (!server)
            return true;
        title = _("Create New User");
        user.reset(new User(server));
    }
    else
    {
        user = extractMetadataItemPtrFromURI<User>(uri);
        if (!user)
            return true;
#ifdef __WXGTK__
        if (user->getUsername() == "SYSDBA")
        {
            showWarningDialog(w, _("The password for the SYSDBA user should not be changed here."),
                _("The appropriate way to change the password of the SYSDBA user is to run the changeDBAPassword.sh script in Firebird's bin directory.\n\nOtherwise the scripts will not be updated."),
                AdvancedMessageDialogButtonsOk(), config(), "DIALOG_warn_sysdba_change",
                _("Do not show this information again"));
        }
#endif
        server = user->getServer();
        if (!server)
            return true;
    }

    UserDialog d(w, title, addUser);
    d.setUser(user);
    if (d.ShowModal() == wxID_OK)
    {
        ProgressDialog pd(w, _("Connecting to Server..."), 1);
        pd.doShow();
        IBPP::Service svc;
        if (!getService(server.get(), svc, &pd, true)) // true = need SYSDBA password
            return true;

        try
        {
            IBPP::User u;
            user->assignTo(u);
            if (addUser)
                svc->AddUser(u);
            else
                svc->ModifyUser(u);
            server->notifyObservers();
        }
        catch(IBPP::Exception& e)
        {
            wxMessageBox(e.what(), _("Error"),
                wxOK | wxICON_WARNING);
        }
    }
    return true;
}

class DropUserHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    DropUserHandler() {};
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const DropUserHandler handlerInstance;
};

const DropUserHandler DropUserHandler::handlerInstance;

bool DropUserHandler::handleURI(URI& uri)
{
    if (uri.action != "drop_user")
        return false;

    wxWindow* w = getParentWindow(uri);
    UserPtr u = extractMetadataItemPtrFromURI<User>(uri);
    if (!u || !w)
        return true;
    ServerPtr s = u->getServer();
    if (!s)
        return true;

    if (wxNO == wxMessageBox(_("Are you sure?"), _("Removing User"),
        wxYES_NO|wxICON_QUESTION))
        return true;

    ProgressDialog pd(w, _("Connecting to Server..."), 1);
    pd.doShow();
    IBPP::Service svc;
    if (!getService(s.get(), svc, &pd, true)) // true = need SYSDBA password
        return true;

    try
    {
        svc->RemoveUser(wx2std(u->getUsername()));
        s->notifyObservers();
    }
    catch(IBPP::Exception& e)
    {
        wxMessageBox(e.what(), _("Error"),
            wxOK | wxICON_WARNING);
    }
    return true;
}

