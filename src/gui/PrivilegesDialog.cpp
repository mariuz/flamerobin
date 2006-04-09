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
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "metadata/collection.h"
#include "metadata/database.h"
#include "metadata/metadataitem.h"
#include "metadata/trigger.h"
#include "urihandler.h"

#include "PrivilegesDialog.h"

//-----------------------------------------------------------------------------
PrivilegesDialog::PrivilegesDialog(wxWindow *parent, MetadataItem *object,
    const wxString& title)
    :wxDialog(parent, -1, title)
{
    databaseM = object->getDatabase();

    wxBoxSizer *mainSizer;
    mainSizer = new wxBoxSizer(wxVERTICAL);
    mainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

    wxBoxSizer *innerSizer;
    innerSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *topSizer;
    topSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *topLeftSizer;
    topLeftSizer = new wxBoxSizer(wxVERTICAL);
    {
    wxString __choices[] = { _("Grant"), _("Revoke") };
    int __nchoices = sizeof(__choices) / sizeof(wxString);
    radiobox_action = new wxRadioBox(mainPanel, ID_radiobox_action, _("Action"), wxDefaultPosition, wxDefaultSize, __nchoices, __choices, 1, wxRA_SPECIFY_COLS);
    }

    topLeftSizer->Add(radiobox_action, 0, wxALL|wxEXPAND, 5);
    checkbox_grant_option = new wxCheckBox(mainPanel, ID_checkbox, _("Grant/Admin option"), wxDefaultPosition, wxDefaultSize, 0);

    topLeftSizer->Add(checkbox_grant_option, 0, wxALL|wxEXPAND, 5);
    granteePanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL);

    wxBoxSizer *granteeSizer;
    granteeSizer = new wxBoxSizer(wxVERTICAL);
    m_staticText3 = new wxStaticText(granteePanel, wxID_ANY, _("GRANTEE"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
    wxFont font(m_staticText3->GetFont());
    font.SetWeight(wxBOLD);
    m_staticText3->SetFont(font);

    granteeSizer->Add(m_staticText3, 0, wxALL|wxEXPAND, 5);
    wxFlexGridSizer *fgSizer3;
    fgSizer3 = new wxFlexGridSizer(2, 2, 0, 0);
    fgSizer3->AddGrowableCol(1);
    radiobtn_user = new wxRadioButton(granteePanel, ID_radiobtn, _("User/Role"), wxDefaultPosition, wxDefaultSize, 0);

    fgSizer3->Add(radiobtn_user, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_user = new wxTextCtrl(granteePanel, ID_textctrl_user, wxT("PUBLIC"), wxDefaultPosition, wxDefaultSize, 0);

    fgSizer3->Add(textctrl_user, 0, wxALL|wxEXPAND, 5);
    radiobtn_trigger = new wxRadioButton(granteePanel, ID_radiobtn, _("Trigger"), wxDefaultPosition, wxDefaultSize, 0);

    fgSizer3->Add(radiobtn_trigger, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    {
        MetadataCollection<Trigger>* tc = databaseM->getCollection<Trigger>();
        wxArrayString choices;
        for (MetadataCollection<Trigger>::iterator it = tc->begin(); it != tc->end(); ++it)
            choices.Add((*it).getName_());
        choice_trigger = new wxChoice(granteePanel, ID_choice, wxDefaultPosition, wxDefaultSize, choices);
    }
    choice_trigger->Enable(false);

    fgSizer3->Add(choice_trigger, 0, wxALL|wxEXPAND, 5);
    radiobtn_procedure = new wxRadioButton(granteePanel, ID_radiobtn, _("Procedure"), wxDefaultPosition, wxDefaultSize, 0);

    fgSizer3->Add(radiobtn_procedure, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    {
        MetadataCollection<Procedure>* tc = databaseM->getCollection<Procedure>();
        wxArrayString choices;
        for (MetadataCollection<Procedure>::iterator it = tc->begin(); it != tc->end(); ++it)
            choices.Add((*it).getName_());
        choice_procedure = new wxChoice(granteePanel, ID_choice, wxDefaultPosition, wxDefaultSize, choices);
    }
    choice_procedure->Enable(false);

    fgSizer3->Add(choice_procedure, 0, wxALL|wxEXPAND, 5);
    radiobtn_view = new wxRadioButton(granteePanel, ID_radiobtn, _("View"), wxDefaultPosition, wxDefaultSize, 0);

    fgSizer3->Add(radiobtn_view, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    {
        MetadataCollection<View>* tc = databaseM->getCollection<View>();
        wxArrayString choices;
        for (MetadataCollection<View>::iterator it = tc->begin(); it != tc->end(); ++it)
            choices.Add((*it).getName_());
        choice_view = new wxChoice(granteePanel, ID_choice, wxDefaultPosition, wxDefaultSize, choices);
    }
    choice_view->Enable(false);

    fgSizer3->Add(choice_view, 0, wxALL|wxEXPAND, 5);
    granteeSizer->Add(fgSizer3, 1, wxEXPAND, 5);
    granteePanel->SetSizer(granteeSizer);
    topLeftSizer->Add(granteePanel, 1, wxEXPAND | wxALL, 5);
    topSizer->Add(topLeftSizer, 0, wxEXPAND, 0);
    privilegesPanel = new wxPanel(mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL);

    wxBoxSizer *privilegesSizer;
    privilegesSizer = new wxBoxSizer(wxVERTICAL);
    m_staticText2 = new wxStaticText(privilegesPanel, wxID_ANY, _("PRIVILEGES"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
    wxFont font2(m_staticText2->GetFont());
    font2.SetWeight(wxBOLD);
    m_staticText2->SetFont(font);

    privilegesSizer->Add(m_staticText2, 0, wxALL|wxEXPAND, 5);
    wxBoxSizer *relationPrivilegesSizer;
    relationPrivilegesSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *relPrivLeftSizer;
    relPrivLeftSizer = new wxBoxSizer(wxVERTICAL);
    checkbox_all = new wxCheckBox(privilegesPanel, ID_checkbox, wxT("All"), wxDefaultPosition, wxDefaultSize, 0);

    relPrivLeftSizer->Add(checkbox_all, 0, wxALL|wxALIGN_CENTER_VERTICAL, 4);
    checkbox_select = new wxCheckBox(privilegesPanel, ID_checkbox, wxT("Select"), wxDefaultPosition, wxDefaultSize, 0);

    relPrivLeftSizer->Add(checkbox_select, 0, wxALL|wxALIGN_CENTER_VERTICAL, 4);
    checkbox_insert = new wxCheckBox(privilegesPanel, ID_checkbox, wxT("Insert"), wxDefaultPosition, wxDefaultSize, 0);

    relPrivLeftSizer->Add(checkbox_insert, 0, wxALL|wxALIGN_CENTER_VERTICAL, 4);
    checkbox_update = new wxCheckBox(privilegesPanel, ID_checkbox, wxT("Update"), wxDefaultPosition, wxDefaultSize, 0);

    relPrivLeftSizer->Add(checkbox_update, 0, wxALL|wxALIGN_CENTER_VERTICAL, 4);
    checkbox_delete = new wxCheckBox(privilegesPanel, ID_checkbox, wxT("Delete"), wxDefaultPosition, wxDefaultSize, 0);

    relPrivLeftSizer->Add(checkbox_delete, 0, wxALL|wxALIGN_CENTER_VERTICAL, 4);
    checkbox_references = new wxCheckBox(privilegesPanel, ID_checkbox, wxT("References"), wxDefaultPosition, wxDefaultSize, 0);

    relPrivLeftSizer->Add(checkbox_references, 0, wxALL|wxALIGN_CENTER_VERTICAL, 4);
    relationPrivilegesSizer->Add(relPrivLeftSizer, 0, wxEXPAND, 0);
    wxBoxSizer *relPrivRightSizer;
    relPrivRightSizer = new wxBoxSizer(wxVERTICAL);
    {
        wxArrayString choices;
        MetadataCollection<Table>* tt = databaseM->getCollection<Table>();
        for (MetadataCollection<Table>::iterator it = tt->begin(); it != tt->end(); ++it)
            choices.Add((*it).getName_());
        MetadataCollection<View>* tv = databaseM->getCollection<View>();
        for (MetadataCollection<View>::iterator it = tv->begin(); it != tv->end(); ++it)
            choices.Add((*it).getName_());
        choice_relations = new wxChoice(privilegesPanel, ID_choice, wxDefaultPosition, wxDefaultSize, choices);
    }
    choice_relations->Enable(false);

    relPrivRightSizer->Add(choice_relations, 0, wxALL|wxEXPAND, 5);
    listbox_columns = new wxListBox(privilegesPanel, ID_listbox,
        wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_MULTIPLE);

    relPrivRightSizer->Add(listbox_columns, 1, wxALL|wxEXPAND, 5);
    relationPrivilegesSizer->Add(relPrivRightSizer, 1, wxEXPAND, 5);
    privilegesSizer->Add(relationPrivilegesSizer, 0, wxEXPAND, 0);
    wxFlexGridSizer *fgSizer2;
    fgSizer2 = new wxFlexGridSizer(2, 2, 0, 0);
    fgSizer2->AddGrowableCol(1);
    checkbox_execute = new wxCheckBox(privilegesPanel, ID_checkbox,
        wxT("Execute"), wxDefaultPosition, wxDefaultSize, 0);

    fgSizer2->Add(checkbox_execute, 0, wxALL|wxALIGN_CENTER_VERTICAL, 4);
    {
        MetadataCollection<Procedure>* tc = databaseM->getCollection<Procedure>();
        wxArrayString choices;
        for (MetadataCollection<Procedure>::iterator it = tc->begin(); it != tc->end(); ++it)
            choices.Add((*it).getName_());
        choice_execute = new wxChoice(privilegesPanel, ID_choice, wxDefaultPosition, wxDefaultSize, choices);
    }
    choice_execute->Enable(false);

    fgSizer2->Add(choice_execute, 0, wxEXPAND|wxALL, 5);
    checkbox_memberof = new wxCheckBox(privilegesPanel, ID_checkbox, wxT("Member of"), wxDefaultPosition, wxDefaultSize, 0);

    fgSizer2->Add(checkbox_memberof, 0, wxALL|wxALIGN_CENTER_VERTICAL, 4);
    {
        MetadataCollection<Role>* tc = databaseM->getCollection<Role>();
        wxArrayString choices;
        for (MetadataCollection<Role>::iterator it = tc->begin(); it != tc->end(); ++it)
            choices.Add((*it).getName_());
        choice_memberof = new wxChoice(privilegesPanel, ID_choice, wxDefaultPosition, wxDefaultSize, choices);
    }
    choice_memberof->Enable(false);

    fgSizer2->Add(choice_memberof, 0, wxALL|wxEXPAND, 5);
    privilegesSizer->Add(fgSizer2, 1, wxEXPAND, 5);
    privilegesPanel->SetSizer(privilegesSizer);
    topSizer->Add(privilegesPanel, 1, wxEXPAND | wxALL, 5);
    innerSizer->Add(topSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10);
    wxBoxSizer *previewSqlSizer;
    previewSqlSizer = new wxBoxSizer(wxHORIZONTAL);
    label_sql = new wxStaticText(mainPanel, wxID_ANY, wxT("SQL"), wxDefaultPosition, wxDefaultSize, 0);

    previewSqlSizer->Add(label_sql, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textbox_current_sql = new wxTextCtrl(mainPanel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    textbox_current_sql->SetBackgroundColour(wxColour(192,192,192));
    textbox_current_sql->Enable(false);

    previewSqlSizer->Add(textbox_current_sql, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    button_add = new wxButton(mainPanel, ID_button_add, _("&Add to list"), wxDefaultPosition, wxDefaultSize, 0);

    previewSqlSizer->Add(button_add, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    innerSizer->Add(previewSqlSizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 10);
    m_staticline2 = new wxStaticLine(mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);

    innerSizer->Add(m_staticline2, 0, wxALL|wxEXPAND, 6);
    label_statements = new wxStaticText(mainPanel, wxID_ANY, _("List of SQL statements to execute"), wxDefaultPosition, wxDefaultSize, 0);

    innerSizer->Add(label_statements, 0, wxALIGN_BOTTOM|wxTOP|wxLEFT|wxEXPAND, 10);
    listbox_statements = new wxListBox(mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0);

    innerSizer->Add(listbox_statements, 1, wxEXPAND|wxRIGHT|wxLEFT, 10);
    wxBoxSizer *sizer_buttons;
    sizer_buttons = new wxBoxSizer(wxHORIZONTAL);
    button_remove = new wxButton(mainPanel, ID_button_remove, _("Remove selected"), wxDefaultPosition, wxDefaultSize, 0);

    sizer_buttons->Add(button_remove, 0, wxALL, 5);
    sizer_buttons->Add(2, 2, 1, wxALL, 0);
    button_execute = new wxButton(mainPanel, wxID_OK, _("Execute all"), wxDefaultPosition, wxDefaultSize, 0);

    sizer_buttons->Add(button_execute, 0, wxALL, 5);
    button_close = new wxButton(mainPanel, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0);

    sizer_buttons->Add(button_close, 0, wxALL, 5);
    innerSizer->Add(sizer_buttons, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5);
    mainPanel->SetSizer(innerSizer);
    mainSizer->Add(mainPanel, 1, wxEXPAND, 0);
    SetSizerAndFit(mainSizer);

    updateControls();
}
//-----------------------------------------------------------------------------
void PrivilegesDialog::enableRelationCheckboxes(bool enable, bool all)
{
    if (all)
        checkbox_all->Enable(enable);
    checkbox_select->Enable(enable);
    checkbox_insert->Enable(enable);
    checkbox_update->Enable(enable);
    checkbox_delete->Enable(enable);
    checkbox_references->Enable(enable);
}
//-----------------------------------------------------------------------------
void PrivilegesDialog::updateControls()
{
    // enable left-size choices
    textctrl_user->Enable(radiobtn_user->GetValue());
    choice_trigger->Enable(radiobtn_trigger->GetValue());
    choice_procedure->Enable(radiobtn_procedure->GetValue());
    choice_view->Enable(radiobtn_view->GetValue());

    // disable role granting for non-user grantees, part1/2 (see below)
    if (!radiobtn_user->GetValue())
        checkbox_memberof->SetValue(false);

    // enable right-side choices
    bool isUpdRefChecked = (checkbox_update->IsChecked() ||
        checkbox_references->IsChecked());
    bool isRelationChecked = ( isUpdRefChecked ||
        checkbox_all->IsChecked()    ||  checkbox_select->IsChecked() ||
        checkbox_insert->IsChecked() ||  checkbox_delete->IsChecked());
    bool isExecuteChecked = checkbox_execute->IsChecked();
    bool isMemberOfChecked = checkbox_memberof->IsChecked();

    choice_relations->Enable(isRelationChecked);
    listbox_columns->Enable(isUpdRefChecked);
    checkbox_execute->Enable(!(isRelationChecked||isMemberOfChecked));
    checkbox_memberof->Enable(!(isRelationChecked||isExecuteChecked));
    enableRelationCheckboxes(!(isExecuteChecked||isMemberOfChecked), true);
    choice_execute->Enable(isExecuteChecked);
    choice_memberof->Enable(isMemberOfChecked);

    // if ALL is selected, turn the rest off...
    if (checkbox_all->IsChecked())
        enableRelationCheckboxes(false, false);
    else if (isRelationChecked) // ...and vice-versa
        checkbox_all->Enable(false);

    // disable role granting for non-user grantees, part2/2 (see below)
    if (checkbox_memberof->IsEnabled() && !radiobtn_user->GetValue())
        checkbox_memberof->Enable(false);

    // if selected table/view has changed -> reload column list
    static wxString lastRelation = choice_relations->GetStringSelection();
    if (choice_relations->IsEnabled() &&
        lastRelation != choice_relations->GetStringSelection())
    {
        listbox_columns->Clear();
        lastRelation = choice_relations->GetStringSelection();
        Relation *r = databaseM->findRelation(Identifier(lastRelation));
        if (r)
        {
            r->checkAndLoadColumns();
            for (MetadataCollection<Column>::const_iterator ci = r->begin();
                ci != r->end(); ++ci)
            {
                listbox_columns->Append((*ci).getName_());
            }
        }
    }

    // construct SQL statement
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(PrivilegesDialog, wxDialog)
//    EVT_BUTTON(FieldPropertiesDialog::ID_button_edit_domain,
//        FieldPropertiesDialog::OnButtonEditDomainClick)
    EVT_CHECKBOX(PrivilegesDialog::ID_checkbox,
        PrivilegesDialog::OnSettingChanged)
    EVT_CHOICE(PrivilegesDialog::ID_choice, PrivilegesDialog::OnSettingChanged)
    EVT_RADIOBOX(PrivilegesDialog::ID_radiobox_action,
        PrivilegesDialog::OnSettingChanged)
    EVT_RADIOBUTTON(PrivilegesDialog::ID_radiobtn,
        PrivilegesDialog::OnSettingChanged)
//    EVT_TEXT(FieldPropertiesDialog::ID_textctrl_fieldname,
//        FieldPropertiesDialog::OnTextFieldnameUpdate)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void PrivilegesDialog::OnSettingChanged(wxCommandEvent& WXUNUSED(event))
{
    updateControls();
}
//-----------------------------------------------------------------------------
class ManagePrivilegesHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const ManagePrivilegesHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const ManagePrivilegesHandler ManagePrivilegesHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool ManagePrivilegesHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("manage_privileges"))
        return false;

    wxWindow* w = getWindow(uri);
    MetadataItem *m = (MetadataItem *)getObject(uri);
    if (!m || !w)
        return true;

    PrivilegesDialog pd(w, m);
    // NOTE: this has been moved here from OnOkButtonClick() to make frame
    //       activation work properly.  Basically activation of another
    //       frame has to happen outside wxDialog::ShowModal(), because it
    //       does at the end re-focus the last focused control, raising
    //       the parent frame over the newly created sql execution frame
    if (pd.ShowModal() == wxID_OK)
    {
        /*
        wxString statements(fpd.getStatementsToExecute());
        // nothing to be done
        if (!statements.IsEmpty())
        {
            // create ExecuteSqlFrame with option to close at once
            ExecuteSqlFrame *esf = new ExecuteSqlFrame(w, -1,
                fpd.getStatementTitle());
            esf->setDatabase(t->getDatabase());
            esf->setSql(statements);
            esf->executeAllStatements(false);   // statement may contain the
            esf->Show();                        // COMMIT, so let's give user
        }                                       // a chance to cancel
        */
    }
    return true;
}
//-----------------------------------------------------------------------------

