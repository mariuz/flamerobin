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

#ifndef DATABASEREGISTRATIONDIALOG_H
#define DATABASEREGISTRATIONDIALOG_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>

#include "gui/BaseDialog.h"

class Server;
class Database;
//-----------------------------------------------------------------------------
class DatabaseRegistrationDialog: public BaseDialog {
private:
    Server* serverM;
    Database* databaseM;
    bool createM;
    bool connectAsM;
    bool isDefaultNameM;

    wxStaticText* label_name;
    wxTextCtrl* text_ctrl_name;
    wxStaticText* label_dbpath;
    wxTextCtrl* text_ctrl_dbpath;
    wxButton* button_browse;
    wxStaticText* label_username;
    wxTextCtrl* text_ctrl_username;
    wxStaticText* label_password;
    wxTextCtrl* text_ctrl_password;
    wxStaticText* label_charset;
    wxComboBox* combobox_charset;
    wxStaticText* label_role;
    wxTextCtrl* text_ctrl_role;
    wxStaticText* label_pagesize;
    wxChoice* choice_pagesize;
    wxStaticText* label_dialect;
    wxChoice* choice_dialect;
    wxCheckBox* checkbox_encrypted;
    wxButton* button_ok;
    wxButton* button_cancel;

    const wxString buildName(const wxString& dbPath) const;
    void createControls();
    void layoutControls();
    void setControlsProperties();
    void updateButtons();
    void updateIsDefaultName();
protected:
    virtual const wxString getName() const;
    virtual bool getConfigStoresHeight() const;
public:
    DatabaseRegistrationDialog(wxWindow* parent, const wxString& title,
        bool createDB = false,
        // a temporary solution, as we'll change the entire login scheme soon
        bool connectAs = false);

    void setServer(Server* s); // needed to create new db
    void setDatabase(Database* db);
private:
    // event handling
    enum {
        ID_textcontrol_dbpath = 101,
        ID_textcontrol_name,
        ID_textcontrol_username,
        ID_textcontrol_password,
        ID_button_browse,
        ID_button_ok,
        ID_button_cancel = wxID_CANCEL
    };

    void OnBrowseButtonClick(wxCommandEvent& event);
    void OnNameChange(wxCommandEvent& event);
    void OnOkButtonClick(wxCommandEvent& event);
    void OnSettingsChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // DATABASEREGISTRATIONDIALOG_H
