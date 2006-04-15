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


  $Id: $

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

#include "frutils.h"
#include "gui/ProgressDialog.h"
#include "metadata/server.h"
#include "styleguide.h"
#include "urihandler.h"
#include "UserDialog.h"
//-----------------------------------------------------------------------------
UserDialog::UserDialog(wxWindow* parent,
        const wxString& title, bool newUser)
    : BaseDialog(parent, wxID_ANY, title), isNewUserM(newUser)
{
    userM = 0;

    createControls();
    setControlsProperties();
    layoutControls();
    updateButtons();
}
//-----------------------------------------------------------------------------
void UserDialog::createControls()
{
    m_staticText17 = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("User name"));
    textctrl_username = new wxTextCtrl(getControlsPanel(), ID_textctrl);
    m_staticText18 = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("Password"));
    textctrl_password = new wxTextCtrl(getControlsPanel(), ID_textctrl,
        wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    m_staticText19 = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("Confirm password"), wxDefaultPosition, wxDefaultSize, 0);
    textctrl_confirm = new wxTextCtrl(getControlsPanel(), ID_textctrl,
        wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);

    m_staticText20 = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("Unix user ID"));
    spinctrl_userid = new wxSpinCtrl(getControlsPanel(), wxID_ANY, wxT("0"),
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 30000, 0);

    m_staticText21 = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("First name"));
    textctrl_firstname = new wxTextCtrl(getControlsPanel(), wxID_ANY, wxT(""));
    m_staticText22 = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("Middle name"));
    textctrl_middlename = new wxTextCtrl(getControlsPanel(), wxID_ANY,
        wxEmptyString);
    m_staticText23 = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("Last name"));
    textctrl_lastname = new wxTextCtrl(getControlsPanel(), wxID_ANY, wxT(""));

    m_staticText24 = new wxStaticText(getControlsPanel(), wxID_ANY,
        wxT("Unix group ID"));
    spinctrl_groupid = new wxSpinCtrl(getControlsPanel(), wxID_ANY, wxT("0"),
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 30000, 0);

    button_ok = new wxButton(getControlsPanel(), ID_button_ok,
        (isNewUserM) ? _("Create") : _("Save"));
    button_cancel = new wxButton(getControlsPanel(), ID_button_cancel,
        _("Cancel"));
}
//-----------------------------------------------------------------------------
const wxString UserDialog::getName() const
{
    return wxT("UserDialog");
}
//-----------------------------------------------------------------------------
void UserDialog::layoutControls()
{
    // create sizer for controls
    wxBoxSizer *controlsSizer = new wxBoxSizer(wxHORIZONTAL);
    wxFlexGridSizer *fgSizer3 = new wxFlexGridSizer(2, 2, 0, 0);
    fgSizer3->AddGrowableCol(1);
    fgSizer3->Add(m_staticText17, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    fgSizer3->Add(textctrl_username, 0, wxALL|wxEXPAND, 5);
    fgSizer3->Add(m_staticText18, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    fgSizer3->Add(textctrl_password, 0, wxALL|wxEXPAND, 5);
    fgSizer3->Add(m_staticText19, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    fgSizer3->Add(textctrl_confirm, 0, wxALL|wxEXPAND, 5);
    fgSizer3->Add(10, 10, 0, wxALL, 5);
    fgSizer3->Add(1, 1, 0, wxALL, 5);   // dummy
    fgSizer3->Add(m_staticText20, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    fgSizer3->Add(spinctrl_userid, 0, wxALL|wxEXPAND, 5);

    controlsSizer->Add(fgSizer3, 0, wxEXPAND, 5);
    controlsSizer->Add(10, 10, 0, wxALL, 5);

    wxFlexGridSizer *fgSizer4;
    fgSizer4 = new wxFlexGridSizer(2, 2, 0, 0);
    fgSizer4->AddGrowableCol(1);
    fgSizer4->Add(m_staticText21, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    fgSizer4->Add(textctrl_firstname, 0, wxALL|wxEXPAND, 5);
    fgSizer4->Add(m_staticText22, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    fgSizer4->Add(textctrl_middlename, 0, wxALL|wxEXPAND, 5);
    fgSizer4->Add(m_staticText23, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    fgSizer4->Add(textctrl_lastname, 0, wxALL|wxEXPAND, 5);
    fgSizer4->Add(10, 10, 0, wxALL, 5);
    fgSizer4->Add(1, 1, 0, wxALL, 5); // dummy
    fgSizer4->Add(m_staticText24, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    fgSizer4->Add(spinctrl_groupid, 0, wxALL|wxEXPAND, 5);
    controlsSizer->Add(fgSizer4, 1, wxEXPAND, 5);

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok,
        button_cancel);
    // use method in base class to set everything up
    layoutSizers(controlsSizer, sizerButtons);
}
//-----------------------------------------------------------------------------
void UserDialog::setControlsProperties()
{
    button_ok->SetDefault();
}
//-----------------------------------------------------------------------------
void UserDialog::setUser(User *u)
{
    userM = u;
    if (!isNewUserM)
    {
        textctrl_username->SetValue(u->usernameM);
        textctrl_password->SetValue(u->passwordM);
        textctrl_confirm->SetValue(u->passwordM);
        textctrl_firstname->SetValue(u->firstnameM);
        textctrl_middlename->SetValue(u->middlenameM);
        textctrl_lastname->SetValue(u->lastnameM);
        spinctrl_userid->SetValue(u->useridM);
        spinctrl_groupid->SetValue(u->groupidM);
    }

    updateButtons();
    updateColors();
}
//-----------------------------------------------------------------------------
void UserDialog::updateButtons()
{
    wxString passwd = textctrl_password->GetValue();
    wxString confirm = textctrl_confirm->GetValue();
    button_ok->Enable(
        !textctrl_username->GetValue().IsEmpty() &&
        !passwd.IsEmpty() && passwd == confirm
    );
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(UserDialog, BaseDialog)
    EVT_TEXT(UserDialog::ID_textctrl, UserDialog::OnSettingsChange)
    EVT_BUTTON(UserDialog::ID_button_ok, UserDialog::OnOkButtonClick)
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
    userM->usernameM = textctrl_username->GetValue();
    userM->passwordM = textctrl_password->GetValue();
    userM->firstnameM = textctrl_firstname->GetValue();
    userM->middlenameM = textctrl_middlename->GetValue();
    userM->lastnameM = textctrl_lastname->GetValue();
    userM->useridM = spinctrl_userid->GetValue();
    userM->groupidM = spinctrl_groupid->GetValue();
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
    if (uri.action != wxT("edit_user") && uri.action != wxT("add_user"))
        return false;

    wxWindow* w = getWindow(uri);
    User *u;
    Server *s;
    wxString title(_("Modify user"));
    if (uri.action == wxT("add_user"))
    {
        s = (Server *)getObject(uri);
        title = _("New user");
    }
    else
    {
        u = (User *)getObject(uri);
        if (!u)
            return true;
        s = dynamic_cast<Server *>(u->getParent());
    }
    User tempusr(s);
    if (uri.action == wxT("add_user"))
        u = &tempusr;
    if (!s)
        return true;

    UserDialog d(w, title, uri.action == wxT("add_user"));
    d.setUser(u);
    if (d.ShowModal() == wxID_OK)
    {
        ProgressDialog pd(w, _("Connecting to server..."), 1);
        IBPP::Service svc;
        if (!getService(s, svc, &pd))
            return true;

        try
        {
            IBPP::User user;
            u->setIBPP(user);
            if (uri.action == wxT("add_user"))
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
    User *u = (User *)getObject(uri);
    if (!u || !w)
        return true;
    Server *s = dynamic_cast<Server *>(u->getParent());
    if (!s)
        return true;
    ProgressDialog pd(w, _("Connecting to server..."), 1);
    IBPP::Service svc;
    if (!getService(s, svc, &pd))
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
