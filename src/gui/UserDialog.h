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

#ifndef FR_USERDIALOG_H
#define FR_USERDIALOG_H

#include <wx/wx.h>
#include <wx/spinctrl.h>

#include "gui/BaseDialog.h"
#include "metadata/MetadataClasses.h"

class UserDialog: public BaseDialog
{
private:
    UserPtr userM;
    bool isNewUserM;

    wxStaticText* labelUserNameM;
    wxTextCtrl* textUserNameM;
    wxStaticText* labelFirstNameM;
    wxTextCtrl* textFirstNameM;
    wxStaticText* labelMiddleNameM;
    wxTextCtrl* textMiddleNameM;
    wxStaticText* labelLastNameM;
    wxTextCtrl* textLastNameM;
    wxStaticText* labelPasswordM;
    wxTextCtrl* textPasswordM;
    wxStaticText* labelConfirmPasswordM;
    wxTextCtrl* textConfirmPasswordM;
    wxStaticText* labelUserIdM;
    wxSpinCtrl* spinctrlUserIdM;
    wxStaticText* labelGroupIdM;
    wxSpinCtrl* spinctrlGroupIdM;
    wxButton* buttonOkM;
    wxButton* buttonCancelM;

    void createControls();
    void layoutControls();
    void setControlsProperties();
    void updateButtons();
protected:
    virtual const wxString getName() const;
public:
    UserDialog(wxWindow* parent, const wxString& title, bool isNewUser);

    void setUser(UserPtr user);
private:
    // event handling
    enum {
        ID_textctrl_username = 1000,
        ID_textctrl_password,
        ID_textctrl_confirmpw
    };

    void OnOkButtonClick(wxCommandEvent& event);
    void OnSettingsChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif
