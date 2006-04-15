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


  $Id:  $

*/

#ifndef FR_USERDIALOG_H
#define FR_USERDIALOG_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/spinctrl.h>

#include "gui/BaseDialog.h"

class User;
//-----------------------------------------------------------------------------
class UserDialog: public BaseDialog
{
private:
    bool isNewUserM;
    User *userM;
    wxStaticText *m_staticText17;
    wxTextCtrl *textctrl_username;
    wxStaticText *m_staticText18;
    wxTextCtrl *textctrl_password;
    wxStaticText *m_staticText19;
    wxTextCtrl *textctrl_confirm;
    wxStaticText *m_staticText20;
    wxSpinCtrl *spinctrl_userid;
    wxStaticText *m_staticText21;
    wxTextCtrl *textctrl_firstname;
    wxStaticText *m_staticText22;
    wxTextCtrl *textctrl_middlename;
    wxStaticText *m_staticText23;
    wxTextCtrl *textctrl_lastname;
    wxStaticText *m_staticText24;
    wxSpinCtrl *spinctrl_groupid;
    wxButton* button_ok;
    wxButton* button_cancel;

    void createControls();
    void layoutControls();
    void setControlsProperties();
    void updateButtons();
    void updateIsDefaultName();
protected:
    virtual const wxString getName() const;
public:
    UserDialog(wxWindow* parent, const wxString& title,
        bool newUser);

    void setUser(User* u);
private:
    // event handling
    enum {
        ID_textctrl = 1000,
        ID_button_ok = wxID_OK,
        ID_button_cancel = wxID_CANCEL
    };

    void OnOkButtonClick(wxCommandEvent& event);
    void OnSettingsChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif
