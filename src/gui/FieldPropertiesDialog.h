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

#ifndef FR_FIELDPROPERTIESDIALOG_H
#define FR_FIELDPROPERTIESDIALOG_H

#include <wx/wx.h>
#include <wx/statline.h>

#include "core/Observer.h"
#include "gui/BaseDialog.h"

class Database;
class Table;
class Column;

class FieldPropertiesDialog: public BaseDialog, public Observer {
private:
    DatabasePtr databaseM;
    Table* tableM;
    Column* columnM;

    wxStaticText* label_fieldname;
    wxTextCtrl* textctrl_fieldname;
    wxStaticText* label_domain;
    wxChoice* choice_domain;
    wxButton* button_edit_domain;
    wxStaticText* label_datatype;
    wxChoice* choice_datatype;
    wxStaticText* label_size;
    wxTextCtrl* textctrl_size;
    wxStaticText* label_scale;
    wxTextCtrl* textctrl_scale;
    wxCheckBox* checkbox_notnull;
    wxStaticText* label_charset;
    wxChoice* choice_charset;
    wxStaticText* label_collate;
    wxChoice* choice_collate;
    wxStaticLine* static_line_autoinc;
    wxStaticText* label_autoinc;
    wxRadioButton* radio_generator_new;
    wxTextCtrl* textctrl_generator_name;
    wxRadioButton* radio_generator_existing;
    wxChoice* choice_generator;
    wxCheckBox* checkbox_trigger;
    wxTextCtrl* textctrl_sql;
    wxCheckBox* checkbox_identity;
    wxStaticText* label_initialValue;
    wxTextCtrl* textctrl_initialValue;
    wxStaticText* label_incrementalValue;
    wxTextCtrl* textctrl_incrementalValue;

    wxButton* button_ok;
    wxButton* button_cancel;

    void createControls();

    bool getDomainInfo(const wxString& domain,
        wxString& type, wxString& size, wxString& scale, wxString& charset);
    bool getIsNewDomainSelected();
    bool getNotNullConstraintName(const wxString& fieldName,
        wxString& constraintName);
    bool getStatementsToExecute(wxString& statements, bool justCheck);
    void layoutControls();
    void loadCharsets();
    void loadCollations();
    void loadDomains();
    void loadGeneratorNames();
    void setControlsProperties();
    void updateColumnControls();
    void updateControls();
    void updateDatatypeInfo();
    void updateDomainControls();
    void updateDomainInfo(const wxString& domain);
    void updateSqlStatement();

    // observer stuff
    virtual void subjectRemoved(Subject* subject);
    virtual void update();
protected:
    virtual const wxString getName() const;
public:
    // Database is required so that domains, charsets, generators can be loaded
    FieldPropertiesDialog(wxWindow* parent, Table* table, Column* column = 0);
    wxString getStatementTitle() const;
    const wxString getStatementsToExecute();
private:
    // event handling
    enum {
        ID_textctrl_fieldname = 101,
        ID_choice_domain,
        ID_button_edit_domain,
        ID_choice_datatype,
        ID_choice_charset,
        ID_choice_collate,
        ID_radio_generator_new,
        ID_textctrl_generator_name,
        ID_radio_generator_existing,
        ID_choice_generator,
        ID_checkbox_trigger,
        ID_checkbox_identity
    };

    void OnButtonEditDomainClick(wxCommandEvent& event);
    void OnButtonOkClick(wxCommandEvent& event);
    void OnChoiceCharsetClick(wxCommandEvent& event);
    void OnChoiceDatatypeClick(wxCommandEvent& event);
    void OnChoiceDomainClick(wxCommandEvent& event);
    void OnNeedsUpdateSql(wxCommandEvent& event);
    void OnRadioGeneratorClick(wxCommandEvent& event);
    void OnTextFieldnameUpdate(wxCommandEvent& event);
    void OnCheckBoxidentityClick(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // FR_FIELDPROPERTIESDIALOG_H
