/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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
    bool isNewServerM;

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
    ServerRegistrationDialog(wxWindow* parent, const wxString& title,
        bool registerServer = false);

    void setServer(Server* s);
private:
    // event handling
    enum {
        ID_textctrl_name = 100,
        ID_textctrl_hostname,
        ID_textctrl_portnumber
    };

    void OnNameChange(wxCommandEvent& event);
    void OnOkButtonClick(wxCommandEvent& event);
    void OnSettingsChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // SERVERREGISTRATIONDIALOG_H
