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
#include "ExecuteSqlFrame.h"
#include "PrivilegesDialog.h"

//-----------------------------------------------------------------------------
PrivilegesDialog::PrivilegesDialog(wxWindow *parent, MetadataItem *object,
    const wxString& title)
    :wxDialog(parent, -1, title)
{
    // on some plaforms, some of control's constructor triggers the events
    // since not all objects are created by that time - event handling code
    // crashes
    inConstructor = true;
    databaseM = object->getDatabase();

    wxBoxSizer *mainSizer;
    mainSizer = new wxBoxSizer(wxVERTICAL);
    mainPanel = new wxPanel(this);

    wxBoxSizer *innerSizer;
    innerSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *topSizer;
    topSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *topLeftSizer;
    topLeftSizer = new wxBoxSizer(wxVERTICAL);
    wxString choices1[] = { _("Grant"), _("Revoke") };
    int nchoices1 = sizeof(choices1) / sizeof(wxString);
    radiobox_action = new wxRadioBox(mainPanel, ID_radiobox_action,
        _("Action"), wxDefaultPosition, wxDefaultSize, nchoices1, choices1,
        1, wxRA_SPECIFY_COLS);
    topLeftSizer->Add(radiobox_action, 0, wxALL|wxEXPAND, 5);

    checkbox_grant_option = new wxCheckBox(mainPanel, ID_checkbox,
        _("Grant/Admin option"));
    topLeftSizer->Add(checkbox_grant_option, 0, wxALL|wxEXPAND, 5);

    granteePanel = new wxPanel(mainPanel);
    wxStaticBoxSizer *granteeSizer = new wxStaticBoxSizer(wxVERTICAL,
        granteePanel, _("Grantee"));
    wxFlexGridSizer *fgSizer3;
    fgSizer3 = new wxFlexGridSizer(2, 2, 0, 0);
    fgSizer3->AddGrowableCol(1);

    radiobtn_user = new wxRadioButton(granteePanel, ID_radiobtn,
        _("User/Role"));
    fgSizer3->Add(radiobtn_user, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_user = new wxTextCtrl(granteePanel, ID_textctrl_user,
        wxT("PUBLIC"));
    fgSizer3->Add(textctrl_user, 0, wxALL|wxEXPAND, 5);

    radiobtn_trigger = new wxRadioButton(granteePanel, ID_radiobtn,
        _("Trigger"));
    fgSizer3->Add(radiobtn_trigger, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    {
        MetadataCollection<Trigger>* tc = databaseM->getCollection<Trigger>();
        wxArrayString choices;
        for (MetadataCollection<Trigger>::iterator it = tc->begin();
            it != tc->end(); ++it)
        {
            choices.Add((*it).getName_());
        }
        choice_trigger = new wxChoice(granteePanel, ID_choice,
            wxDefaultPosition, wxDefaultSize, choices);
        if (!choices.IsEmpty())
            choice_trigger->SetSelection(0);
    }
    choice_trigger->Enable(false);

    fgSizer3->Add(choice_trigger, 0, wxALL|wxEXPAND, 5);
    radiobtn_procedure = new wxRadioButton(granteePanel, ID_radiobtn,
         _("Procedure"));
    fgSizer3->Add(radiobtn_procedure, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    {
        MetadataCollection<Procedure>* tc =
            databaseM->getCollection<Procedure>();
        wxArrayString choices;
        for (MetadataCollection<Procedure>::iterator it = tc->begin();
            it != tc->end(); ++it)
        {
            choices.Add((*it).getName_());
        }
        choice_procedure = new wxChoice(granteePanel, ID_choice,
            wxDefaultPosition, wxDefaultSize, choices);
        if (!choices.IsEmpty())
            choice_procedure->SetSelection(0);
    }
    choice_procedure->Enable(false);
    fgSizer3->Add(choice_procedure, 0, wxALL|wxEXPAND, 5);

    radiobtn_view = new wxRadioButton(granteePanel, ID_radiobtn, _("View"));
    fgSizer3->Add(radiobtn_view, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    {
        MetadataCollection<View>* tc = databaseM->getCollection<View>();
        wxArrayString choices;
        for (MetadataCollection<View>::iterator it = tc->begin();
            it != tc->end(); ++it)
        {
            choices.Add((*it).getName_());
        }
        choice_view = new wxChoice(granteePanel, ID_choice, wxDefaultPosition,
            wxDefaultSize, choices);
        if (!choices.IsEmpty())
            choice_view->SetSelection(0);
    }
    choice_view->Enable(false);
    fgSizer3->Add(choice_view, 0, wxALL|wxEXPAND, 5);

    granteeSizer->Add(fgSizer3, 1, wxEXPAND, 5);
    granteePanel->SetSizer(granteeSizer);
    topLeftSizer->Add(granteePanel, 1, wxEXPAND | wxALL, 5);
    topSizer->Add(topLeftSizer, 0, wxEXPAND, 0);
    topSizer->Add(12, 12, 0, wxALL, 5); // separate left and right parts
    privilegesPanel = new wxPanel(mainPanel);
    wxBoxSizer *privilegesSizer;
    privilegesSizer = new wxBoxSizer(wxVERTICAL);
    m_staticText2 = new wxStaticText(privilegesPanel, wxID_ANY,
        _("Privileges"));
    wxFont font2(m_staticText2->GetFont());
    font2.SetWeight(wxBOLD);
    m_staticText2->SetFont(font2);
    privilegesSizer->Add(m_staticText2, 0, wxALL|wxEXPAND, 4);

    wxBoxSizer *relationPrivilegesSizer;
    relationPrivilegesSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *relPrivLeftSizer;
    relPrivLeftSizer = new wxBoxSizer(wxVERTICAL);
    checkbox_all = new wxCheckBox(privilegesPanel, ID_checkbox, wxT("All"));
    relPrivLeftSizer->Add(checkbox_all, 0, wxALL|wxALIGN_CENTER_VERTICAL, 4);

    checkbox_select = new wxCheckBox(privilegesPanel, ID_checkbox,
        wxT("Select"));
    relPrivLeftSizer->Add(checkbox_select, 0,
        wxALL|wxALIGN_CENTER_VERTICAL, 4);

    checkbox_insert = new wxCheckBox(privilegesPanel, ID_checkbox,
        wxT("Insert"));
    relPrivLeftSizer->Add(checkbox_insert, 0,
        wxALL|wxALIGN_CENTER_VERTICAL, 4);

    checkbox_update = new wxCheckBox(privilegesPanel, ID_checkbox,
        wxT("Update"));
    relPrivLeftSizer->Add(checkbox_update, 0,
        wxALL|wxALIGN_CENTER_VERTICAL, 4);

    checkbox_delete = new wxCheckBox(privilegesPanel, ID_checkbox,
        wxT("Delete"));
    relPrivLeftSizer->Add(checkbox_delete, 0,
        wxALL|wxALIGN_CENTER_VERTICAL, 4);

    checkbox_references = new wxCheckBox(privilegesPanel, ID_checkbox,
        wxT("References"));
    relPrivLeftSizer->Add(checkbox_references, 0,
        wxALL|wxALIGN_CENTER_VERTICAL, 4);
    relationPrivilegesSizer->Add(relPrivLeftSizer, 0, wxEXPAND, 0);
    wxBoxSizer *relPrivRightSizer;
    relPrivRightSizer = new wxBoxSizer(wxVERTICAL);

    {
        wxArrayString choices;
        MetadataCollection<Table>* tt = databaseM->getCollection<Table>();
        size_t relation_to_select = 0;
        for (MetadataCollection<Table>::iterator it = tt->begin();
            it != tt->end(); ++it)
        {
            if (dynamic_cast<Table *>(object) == &(*it))
                relation_to_select = choices.GetCount();
            choices.Add((*it).getName_());
        }
        MetadataCollection<View>* tv = databaseM->getCollection<View>();
        for (MetadataCollection<View>::iterator it = tv->begin();
            it != tv->end(); ++it)
        {
            if (dynamic_cast<View *>(object) == &(*it))
                relation_to_select = choices.GetCount();
            choices.Add((*it).getName_());
        }
        choice_relations = new wxChoice(privilegesPanel, ID_choice,
            wxDefaultPosition, wxDefaultSize, choices);
        if (!choices.IsEmpty())
            choice_relations->SetSelection(relation_to_select);
    }
    choice_relations->Enable(false);
    relPrivRightSizer->Add(choice_relations, 0, wxALL|wxEXPAND, 5);
    listbox_columns = new wxListBox(privilegesPanel, ID_listbox,
        wxDefaultPosition, wxDefaultSize, 0, 0, wxLB_MULTIPLE);
    relPrivRightSizer->Add(listbox_columns, 1, wxALL|wxEXPAND, 5);
    relationPrivilegesSizer->Add(relPrivRightSizer, 1, wxEXPAND, 5);
    privilegesSizer->Add(relationPrivilegesSizer, 0, wxEXPAND, 0);
    wxFlexGridSizer *fgSizer2;
    fgSizer2 = new wxFlexGridSizer(2, 2, 0, 0);
    fgSizer2->AddGrowableCol(1);

    checkbox_execute = new wxCheckBox(privilegesPanel, ID_checkbox,
        wxT("Execute"));
    fgSizer2->Add(checkbox_execute, 0, wxALL|wxALIGN_CENTER_VERTICAL, 4);
    {
        MetadataCollection<Procedure>* tc =
            databaseM->getCollection<Procedure>();
        wxArrayString choices;
        size_t to_select = 0;
        for (MetadataCollection<Procedure>::iterator it = tc->begin();
            it != tc->end(); ++it)
        {
            if (dynamic_cast<Procedure *>(object) == &(*it))
                to_select = choices.GetCount();
            choices.Add((*it).getName_());
        }
        choice_execute = new wxChoice(privilegesPanel, ID_choice,
            wxDefaultPosition, wxDefaultSize, choices);
        if (!choices.IsEmpty())
            choice_execute->SetSelection(to_select);
    }
    choice_execute->Enable(false);
    fgSizer2->Add(choice_execute, 0, wxEXPAND|wxALL, 5);

    checkbox_memberof = new wxCheckBox(privilegesPanel, ID_checkbox,
        wxT("Member of"));
    fgSizer2->Add(checkbox_memberof, 0, wxALL|wxALIGN_CENTER_VERTICAL, 4);
    {
        MetadataCollection<Role>* tc = databaseM->getCollection<Role>();
        wxArrayString choices;
        size_t to_select = 0;
        for (MetadataCollection<Role>::iterator it = tc->begin();
            it != tc->end(); ++it)
        {
            if (dynamic_cast<Role *>(object) == &(*it))
                to_select = choices.GetCount();
            choices.Add((*it).getName_());
        }
        choice_memberof = new wxChoice(privilegesPanel, ID_choice,
            wxDefaultPosition, wxDefaultSize, choices);
        if (!choices.IsEmpty())
            choice_memberof->SetSelection(to_select);
    }
    choice_memberof->Enable(false);
    fgSizer2->Add(choice_memberof, 0, wxALL|wxEXPAND, 5);

    privilegesSizer->Add(fgSizer2, 1, wxEXPAND, 5);
    privilegesPanel->SetSizer(privilegesSizer);
    topSizer->Add(privilegesPanel, 1, wxEXPAND | wxALL, 5);
    innerSizer->Add(topSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10);
    innerSizer->Add(10, 5, 0, wxALL, 0);
    wxBoxSizer *previewSqlSizer;
    previewSqlSizer = new wxBoxSizer(wxHORIZONTAL);
    label_sql = new wxStaticText(mainPanel, wxID_ANY, wxT("SQL"));
    previewSqlSizer->Add(label_sql, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textbox_current_sql = new wxTextCtrl(mainPanel, wxID_ANY, wxT(""),
        wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    textbox_current_sql->SetBackgroundColour(
        wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    textbox_current_sql->Enable(false);
    previewSqlSizer->Add(textbox_current_sql, 1,
        wxALL|wxALIGN_CENTER_VERTICAL, 5);
    button_add = new wxButton(mainPanel, ID_button_add, _("&Add to list"));
    previewSqlSizer->Add(button_add, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    innerSizer->Add(previewSqlSizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 10);

    m_staticline2 = new wxStaticLine(mainPanel, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxLI_HORIZONTAL);
    innerSizer->Add(m_staticline2, 0, wxALL|wxEXPAND, 6);

    label_statements = new wxStaticText(mainPanel, wxID_ANY,
        _("List of SQL statements to execute"));
    innerSizer->Add(label_statements, 0,
        wxALIGN_BOTTOM|wxTOP|wxLEFT|wxEXPAND, 10);
    listbox_statements = new wxListBox(mainPanel, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, 0, 0, 0);
    innerSizer->Add(listbox_statements, 1, wxEXPAND|wxRIGHT|wxLEFT, 10);

    wxBoxSizer *sizer_buttons;
    sizer_buttons = new wxBoxSizer(wxHORIZONTAL);
    button_remove = new wxButton(mainPanel, ID_button_remove,
        _("Remove selected"), wxDefaultPosition, wxDefaultSize, 0);
    sizer_buttons->Add(button_remove, 0, wxALL, 5);
    sizer_buttons->Add(2, 2, 1, wxALL, 0);
    button_execute = new wxButton(mainPanel, wxID_OK, _("Execute all"));
    sizer_buttons->Add(button_execute, 0, wxALL, 5);
    button_close = new wxButton(mainPanel, wxID_CANCEL, _("Close"));
    sizer_buttons->Add(button_close, 0, wxALL, 5);
    innerSizer->Add(sizer_buttons, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5);
    mainPanel->SetSizer(innerSizer);
    mainSizer->Add(mainPanel, 1, wxEXPAND, 0);
    SetSizerAndFit(mainSizer);

    inConstructor = false;
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
void PrivilegesDialog::loadRelationColumns()
{
    static wxString lastRelation = wxT("-");
    if (lastRelation != choice_relations->GetStringSelection())
    {
        lastRelation = choice_relations->GetStringSelection();
        listbox_columns->Clear();
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
}
//-----------------------------------------------------------------------------
void PrivilegesDialog::updateControls()
{
    if (inConstructor)
        return;

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
    bool isRole = checkbox_memberof->IsChecked();

    choice_relations->Enable(isRelationChecked);
    listbox_columns->Enable(isUpdRefChecked);
    if (isUpdRefChecked)
        loadRelationColumns();
    checkbox_execute->Enable(!(isRelationChecked||isRole));
    checkbox_memberof->Enable(!(isRelationChecked||isExecuteChecked));
    enableRelationCheckboxes(!(isExecuteChecked||isRole), true);
    choice_execute->Enable(isExecuteChecked);
    choice_memberof->Enable(isRole);

    // if ALL is selected, turn the rest off...
    if (checkbox_all->IsChecked())
        enableRelationCheckboxes(false, false);
    else if (isRelationChecked) // ...and vice-versa
        checkbox_all->Enable(false);

    // disable role granting for non-user grantees, part2/2 (see below)
    if (checkbox_memberof->IsEnabled() && !radiobtn_user->GetValue())
        checkbox_memberof->Enable(false);

    /* CONSTRUCT SQL STATEMENT.   Various combinations covered:
    GRANT            ... ON table/view     TO   grantee [WGO]
    REVOKE [GOF]     ... ON table/view     FROM grantee
    GRANT            ... ON PROCEDURE proc TO   grantee [WGO]
    REVOKE [GOF] EXECUTE ON PROCEDURE proc FROM grantee
    GRANT        rolename                  TO   grantee [WAO]
    REVOKE [AOF] rolename                  FROM grantee
    */

    bool hasSomething = isRelationChecked || isExecuteChecked || isRole;
    button_add->Enable(hasSomething);
    if (!hasSomething)
    {
        textbox_current_sql->SetValue(wxEmptyString);
        return;
    }

    wxString grantee;
    if (radiobtn_user->GetValue())
        grantee = textctrl_user->GetValue();
    else if (radiobtn_trigger->GetValue())
        grantee = wxT("TRIGGER ") + choice_trigger->GetStringSelection();
    else if (radiobtn_procedure->GetValue())
        grantee = wxT("PROCEDURE ") + choice_procedure->GetStringSelection();
    else if (radiobtn_view->GetValue())
        grantee = wxT("VIEW ") + choice_view->GetStringSelection();
    bool grant = radiobox_action->GetSelection() == 0;
    bool grantoption = checkbox_grant_option->IsChecked();
    wxString sql(grant ? wxT("GRANT ") : wxT("REVOKE "));
    if (!grant && grantoption)
        sql << (isRole ? wxT("ADMIN") : wxT("GRANT")) << wxT(" OPTION FOR ");
    if (isRole)
        sql << choice_memberof->GetStringSelection();
    else if (isRelationChecked)
    {
        wxString priv;
        if (checkbox_all->IsChecked())
            priv = wxT("ALL");
        else
        {
            wxCheckBox* boxes[5] = { checkbox_select, checkbox_insert,
                checkbox_update, checkbox_delete, checkbox_references };
            for (int i = 0; i < sizeof(boxes)/sizeof(wxCheckBox *); i++)
            {
                if (boxes[i]->IsChecked())
                {
                    if (!priv.IsEmpty())
                        priv += wxT(",");
                    priv += boxes[i]->GetLabel().Upper();
                    wxArrayInt ai;
                    listbox_columns->GetSelections(ai);
                    if ((boxes[i] == checkbox_update || boxes[i] ==
                        checkbox_references) && !ai.IsEmpty())
                    {
                        priv += wxT("(");
                        for (size_t i = 0; i < ai.GetCount(); i++)
                        {
                            if (i != 0)
                                priv += wxT(",");
                            priv += listbox_columns->GetString(ai.Item(i));
                        }
                        priv += wxT(")");
                    }
                }
            }
        }
        sql << priv << wxT(" ON ") << choice_relations->GetStringSelection();
    }
    else if (isExecuteChecked)
    {
        sql << wxT("EXECUTE ON PROCEDURE ")
            << choice_execute->GetStringSelection();
    }

    sql << (grant ? wxT(" TO ") : wxT(" FROM ")) << grantee;
    if (grant && grantoption)
    {
        sql << wxT(" WITH ") << (isRole ? wxT("ADMIN") : wxT("GRANT"))
            << wxT(" OPTION");
    }

    textbox_current_sql->SetValue(sql);
}
//-----------------------------------------------------------------------------
wxString PrivilegesDialog::getSqlStatements()
{
    wxString stmt;
    for (size_t i = 0; i<listbox_statements->GetCount(); i++)
        stmt << listbox_statements->GetString(i) << wxT(";\n");
    return stmt;
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(PrivilegesDialog, wxDialog)
    EVT_BUTTON(PrivilegesDialog::ID_button_add,
        PrivilegesDialog::OnButtonAddClick)
    EVT_BUTTON(PrivilegesDialog::ID_button_remove,
        PrivilegesDialog::OnButtonRemoveClick)
    EVT_CHECKBOX(PrivilegesDialog::ID_checkbox,
        PrivilegesDialog::OnSettingChanged)
    EVT_CHOICE(PrivilegesDialog::ID_choice, PrivilegesDialog::OnSettingChanged)
    EVT_LISTBOX(PrivilegesDialog::ID_listbox,
        PrivilegesDialog::OnSettingChanged)
    EVT_RADIOBOX(PrivilegesDialog::ID_radiobox_action,
        PrivilegesDialog::OnSettingChanged)
    EVT_RADIOBUTTON(PrivilegesDialog::ID_radiobtn,
        PrivilegesDialog::OnSettingChanged)
    EVT_TEXT(PrivilegesDialog::ID_textctrl_user,
        PrivilegesDialog::OnSettingChanged)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void PrivilegesDialog::OnButtonAddClick(wxCommandEvent& WXUNUSED(event))
{
    listbox_statements->Append(textbox_current_sql->GetValue());
}
//-----------------------------------------------------------------------------
void PrivilegesDialog::OnButtonRemoveClick(wxCommandEvent& WXUNUSED(event))
{
    int sel = listbox_statements->GetSelection();
    if (sel != wxNOT_FOUND)
        listbox_statements->Delete(sel);
}
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
        wxString statements(pd.getSqlStatements());
        // nothing to be done
        if (!statements.IsEmpty())
        {
            // create ExecuteSqlFrame with option to close at once
            ExecuteSqlFrame *esf = new ExecuteSqlFrame(w, -1,
                _("Grant and revoke privileges"));
            esf->setDatabase(m->getDatabase());
            esf->setSql(statements);
            esf->executeAllStatements(false);
            esf->Show();
        }
    }
    return true;
}
//-----------------------------------------------------------------------------

