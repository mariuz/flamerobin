/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Michael Hieke, Nando Dessena
*/

#ifndef SERVERREGISTRATIONDIALOG_H
#define SERVERREGISTRATIONDIALOG_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>

#include "gui/BaseDialog.h"

class Server;
//-----------------------------------------------------------------------------
class ServerRegistrationDialog: public BaseDialog {
private:
    Server* serverM;
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
    void createControls();
    void layoutControls();
    void setControlsProperties();
    void updateButtons();
    void updateIsDefaultName();
protected:
    virtual const wxString getName() const;
public:
    ServerRegistrationDialog(wxWindow* parent, int id, const wxString& title);

    void setServer(Server* s);
private:
    // event handling
    enum {
        ID_textctrl_name = 100,
        ID_textctrl_hostname,
        ID_textctrl_portnumber,
        ID_button_ok = wxID_OK,
        ID_button_cancel = wxID_CANCEL
    };

    void OnNameChange(wxCommandEvent& event);
    void OnOkButtonClick(wxCommandEvent& event);
    void OnSettingsChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // SERVERREGISTRATIONDIALOG_H
