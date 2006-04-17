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
//-----------------------------------------------------------------------------
#ifndef FR_PRIVILEGESDIALOG_H
#define FR_PRIVILEGESDIALOG_H

#include <wx/statline.h>
#include "BaseDialog.h"

class Database;
//-----------------------------------------------------------------------------
class PrivilegesDialog : public BaseDialog
{
private:
    Database *databaseM;
    void enableRelationCheckboxes(bool enable, bool all);
    void loadRelationColumns();
    bool inConstructor;

protected:
    wxRadioBox *radiobox_action;
    wxCheckBox *checkbox_grant_option;
    wxPanel *granteePanel;
    wxStaticText *m_staticText3;
    wxRadioButton *radiobtn_user;
    wxTextCtrl *textctrl_user;
    wxRadioButton *radiobtn_trigger;
    wxChoice *choice_trigger;
    wxRadioButton *radiobtn_procedure;
    wxChoice *choice_procedure;
    wxRadioButton *radiobtn_view;
    wxChoice *choice_view;
    wxPanel *privilegesPanel;
    wxStaticText *m_staticText2;
    wxCheckBox *checkbox_all;
    wxCheckBox *checkbox_select;
    wxCheckBox *checkbox_insert;
    wxCheckBox *checkbox_update;
    wxCheckBox *checkbox_delete;
    wxCheckBox *checkbox_references;
    wxChoice *choice_relations;
    wxListBox *listbox_columns;
    wxCheckBox *checkbox_execute;
    wxChoice *choice_execute;
    wxCheckBox *checkbox_memberof;
    wxChoice *choice_memberof;
    wxStaticText *label_sql;
    wxTextCtrl *textbox_current_sql;
    wxButton *button_add;
    wxStaticLine *m_staticline2;
    wxStaticText *label_statements;
    wxListBox *listbox_statements;
    wxButton *button_remove;
    wxButton *button_execute;
    wxButton *button_close;

public:
    void updateControls();
    void OnSettingChanged(wxCommandEvent& event);
    void OnButtonAddClick(wxCommandEvent& event);
    void OnButtonRemoveClick(wxCommandEvent& event);
    void OnListBoxStatementsSelected(wxCommandEvent& event);

    enum
    {
        ID_button_add = 1000,
        ID_button_close,
        ID_button_execute,
        ID_button_remove,
        ID_checkbox,
        ID_choice,
        ID_listbox,
        ID_radiobox_action,
        ID_radiobtn,
        ID_textctrl_user,
        ID_listbox_statements
    };

    wxString getSqlStatements();
    PrivilegesDialog(wxWindow *parent, MetadataItem *object,
        const wxString& title = _("Grant And Revoke Privileges"));

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif
