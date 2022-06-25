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

#ifndef DATABASEREGISTRATIONDIALOG_H
#define DATABASEREGISTRATIONDIALOG_H

#include <wx/wx.h>

#include "gui/BaseDialog.h"
#include "metadata/MetadataClasses.h"

class FileTextControl;

class DatabaseRegistrationDialog: public BaseDialog {
private:
    DatabasePtr databaseM;
    bool createM;
    bool connectAsM;
    bool isDefaultNameM;

    wxStaticText* label_name;
    wxTextCtrl* text_ctrl_name;

    wxStaticText* label_dbpath;
    FileTextControl* text_ctrl_dbpath;
    wxButton* button_browse;
    
    wxStaticText* label_authentication;
    wxChoice* choice_authentication;
    
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
    
    /*
    * Todo: Implement FB library per conexion
    wxStaticText* label_library;
    FileTextControl* text_ctrl_library;
    wxButton* button_browse_library;
    */

    wxButton* button_ok;
    wxButton* button_cancel;

    void createControls();
    void layoutControls();
    void setControlsProperties();
    void updateAuthenticationMode();
    void updateButtons();
    wxString getDefaultDatabaseName() const;
    void updateIsDefaultDatabaseName();

    wxArrayString getAuthenticationChoices() const;
    wxArrayString getDatabaseCharsetChoices() const;
    wxArrayString getDatabaseDialectChoices() const;
    wxArrayString getDatabasePagesizeChoices() const;
protected:
    virtual void doReadConfigSettings(const wxString& prefix);
    virtual void doWriteConfigSettings(const wxString& prefix) const;
    virtual const wxString getName() const;
    virtual bool getConfigStoresHeight() const;
public:
    DatabaseRegistrationDialog(wxWindow* parent, const wxString& title,
        bool createDB = false,
        // a temporary solution, as we'll change the entire login scheme soon
        bool connectAs = false);

    void setDatabase(DatabasePtr db);
private:
    // event handling
    enum {
        ID_textcontrol_dbpath = 101,
        ID_textcontrol_name,
        ID_textcontrol_username,
        ID_textcontrol_password,
        ID_button_browse,
        ID_choice_authentication,
        ID_textcontrol_library,
        ID_button_browse_library
    };

    void OnAuthenticationChange(wxCommandEvent& event);
    void OnBrowseButtonClick(wxCommandEvent& event);
    void OnBrowseLibraryButtonClick(wxCommandEvent& event);
    void OnNameChange(wxCommandEvent& event);
    void OnOkButtonClick(wxCommandEvent& event);
    void OnSettingsChange(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // DATABASEREGISTRATIONDIALOG_H
