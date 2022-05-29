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

#ifndef USERNAMEPASSWORDDIALOG_H
#define USERNAMEPASSWORDDIALOG_H

#include <wx/wx.h>

#include "gui/BaseDialog.h"
#include "metadata/MetadataClasses.h"

class UsernamePasswordDialog: public BaseDialog {
public:
    enum Flags
    {
        Default             = 0x00,
        AllowOtherUsername  = 0x01,
        AllowTrustedUser    = 0x02,
    };
private:
    wxStaticText* labelDescriptionM;
    wxRadioButton* radioUsernamePasswordM;
    wxStaticText* labelUsernameM;
    wxTextCtrl* textUsernameM;
    wxStaticText* labelPasswordM;
    wxTextCtrl* textPasswordM;
    wxRadioButton* radioTrustedUserM;
    wxButton* buttonOkM;
    wxButton* buttonCancelM;

    void layoutControls();
    void updateButton();

    void OnSettingsChange(wxCommandEvent& event);
public:
    UsernamePasswordDialog(wxWindow* parentWindow, const wxString& description,
        const wxString& username, int flags);

    wxString getUsername();
    wxString getPassword();
};

bool getConnectionCredentials(wxWindow* parentWindow, DatabasePtr database,
    wxString& username, wxString& password,
    int flags = UsernamePasswordDialog::Default);
bool getConnectionCredentials(wxWindow* parentWindow,
    const wxString& description, wxString& username, wxString& password,
    int flags = UsernamePasswordDialog::Default);

#endif // USERNAMEPASSWORDDIALOG_H
