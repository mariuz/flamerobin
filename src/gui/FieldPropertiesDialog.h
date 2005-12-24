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

  Contributor(s): Nando Dessena, Michael Hieke
*/

#ifndef FR_FIELDPROPERTIESDIALOG_H
#define FR_FIELDPROPERTIESDIALOG_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/statline.h>

#include "core/Observer.h"
#include "gui/BaseDialog.h"

class Database;
class Table;
class Column;
//-----------------------------------------------------------------------------
class FieldPropertiesDialog: public BaseDialog, public Observer {
private:
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

    wxButton* button_ok;
    wxButton* button_cancel;

    void createControls();
    bool getDomainInfo(const wxString& domain, 
        wxString& type, wxString& size, wxString& scale, wxString& charset);
    bool getIsNewDomainSelected();
    void layoutControls();
    void loadCharsets();
    void loadCollations();
    void loadDomains();
    void loadGeneratorNames();
    void setColumnM(Column* column);
    void setControlsProperties();
    void setTableM(Table* table);
    void updateColumnControls();
    void updateControls();
    void updateDatatypeInfo();
    void updateDomainControls();
    void updateDomainInfo(const wxString& domain);
    void updateSqlStatement();
protected:
    virtual const wxString getName() const;
    virtual void removeSubject(Subject* subject);
    virtual void update();
public:
    // Database is required so that domains, charsets, generators can be loaded
    FieldPropertiesDialog(wxWindow* parent, Table* table, Column* column = 0);
	wxString getStatementTitle() const;
    wxString getStatementsToExecute();
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

        ID_button_ok,
        ID_button_cancel = wxID_CANCEL
    };

    void OnButtonEditDomainClick(wxCommandEvent& event);
    void OnButtonOkClick(wxCommandEvent& event);
    void OnChoiceCharsetClick(wxCommandEvent& event);
    void OnChoiceDatatypeClick(wxCommandEvent& event);
    void OnChoiceDomainClick(wxCommandEvent& event);
    void OnNeedsUpdateSql(wxCommandEvent& event);
    void OnRadioGeneratorClick(wxCommandEvent& event);
    void OnTextFieldnameUpdate(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // FR_FIELDPROPERTIESDIALOG_H
