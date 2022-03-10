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

#ifndef SERVERREGISTRATIONDIALOG_H
#define SERVERREGISTRATIONDIALOG_H

#include <wx/wx.h>

#include "gui/BaseDialog.h"
#include "metadata/MetadataClasses.h"

class ServerRegistrationDialog: public BaseDialog {
private:
    ServerPtr serverM;
    bool isDefaultNameM;

    wxStaticText* label_name;
    wxTextCtrl* text_ctrl_name;
    wxStaticText* label_hostname;
    wxTextCtrl* text_ctrl_hostname;
    wxStaticText* label_portnumber;
    wxTextCtrl* text_ctrl_portnumber;
    wxButton* button_ok;
    wxButton* button_cancel;

    const wxString buildDefaultName() const;
    void connectEvents();
    void createControls(bool registerServer);
    void layoutControls();
    void setControlsProperties();
    void updateButtons();
    void updateIsDefaultName();
protected:
    virtual const wxString getName() const;
public:
    ServerRegistrationDialog(wxWindow* parent, const wxString& title);
    ServerRegistrationDialog(wxWindow* parent, const wxString& title,
        ServerPtr server);

    ServerPtr getServer() const;
private:
    void OnNameChange(wxCommandEvent& event);
    void OnOkButtonClick(wxCommandEvent& event);
    void OnSettingsChange(wxCommandEvent& event);
};

#endif // SERVERREGISTRATIONDIALOG_H
