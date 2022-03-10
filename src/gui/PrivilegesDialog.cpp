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

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/gbsizer.h>

#include "core/URIProcessor.h"
#include "frutils.h"
#include "gui/ExecuteSql.h"
#include "gui/GUIURIHandlerHelper.h"
#include "gui/PrivilegesDialog.h"
#include "gui/StyleGuide.h"
#include "metadata/collection.h"
#include "metadata/database.h"
#include "metadata/MetadataItemURIHandlerHelper.h"
#include "metadata/procedure.h"
#include "metadata/role.h"
#include "metadata/table.h"
#include "metadata/trigger.h"
#include "metadata/view.h"


PrivilegesDialog::PrivilegesDialog(wxWindow *parent, MetadataItem *object,
    const wxString& title)
    :BaseDialog(parent, -1, title)
{
    // on some plaforms, some of control's constructor triggers the events
    // since not all objects are created by that time - event handling code
    // crashes
    inConstructor = true;
    databaseM = object->getDatabase().get();

    wxBoxSizer *innerSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *topSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *topLeftSizer = new wxBoxSizer(wxVERTICAL);
    wxString choices1[] =
    {
        _("Grant privilege"),
        _("Grant privilege with grant/admin option"),
        _("Revoke privilege"),
        _("Revoke grant/admin option")
    };
    int nchoices1 = sizeof(choices1) / sizeof(wxString);
    radiobox_action = new wxRadioBox(getControlsPanel(), ID_radiobox_action,
        _("Action"), wxDefaultPosition, wxDefaultSize, nchoices1, choices1,
        1, wxRA_SPECIFY_COLS);
    topLeftSizer->Add(radiobox_action, 0, wxBOTTOM|wxEXPAND,
        styleguide().getUnrelatedControlMargin(wxVERTICAL));

    granteePanel = new wxPanel(getControlsPanel());
    wxStaticBoxSizer *granteeSizer = new wxStaticBoxSizer(wxVERTICAL,
        granteePanel, _("Grantee"));
    wxFlexGridSizer *fgSizer3 = new wxFlexGridSizer(2, 2, 0, 0);
    fgSizer3->AddGrowableCol(1);

    radiobtn_user = new wxRadioButton(granteePanel, ID_radiobtn,
        _("User/Role:"));
    radiobtn_user->SetValue(true);  // not needed on GTK, but doesn't hurt
    fgSizer3->Add(radiobtn_user, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_user = new wxTextCtrl(granteePanel, ID_textctrl, "PUBLIC");
    fgSizer3->Add(textctrl_user, 0, wxALL|wxEXPAND, 5);

    radiobtn_trigger = new wxRadioButton(granteePanel, ID_radiobtn,
        _("Trigger:"));
    fgSizer3->Add(radiobtn_trigger, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    {
        DMLTriggersPtr tc(databaseM->getDMLTriggers());
        wxArrayString choices;
        for (DMLTriggers::iterator it = tc->begin(); it != tc->end(); ++it)
            choices.Add((*it)->getName_());
        choice_trigger = new wxChoice(granteePanel, ID_choice,
            wxDefaultPosition, wxDefaultSize, choices);
        if (!choices.IsEmpty())
            choice_trigger->SetSelection(0);
    }
    choice_trigger->Enable(false);
    fgSizer3->Add(choice_trigger, 0, wxALL|wxEXPAND, 5);

    radiobtn_procedure = new wxRadioButton(granteePanel, ID_radiobtn,
         _("Procedure:"));
    fgSizer3->Add(radiobtn_procedure, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    {
        ProceduresPtr tc(databaseM->getProcedures());
        wxArrayString choices;
        for (Procedures::iterator it = tc->begin(); it != tc->end(); ++it)
            choices.Add((*it)->getName_());
        choice_procedure = new wxChoice(granteePanel, ID_choice,
            wxDefaultPosition, wxDefaultSize, choices);
        if (!choices.IsEmpty())
            choice_procedure->SetSelection(0);
    }
    choice_procedure->Enable(false);
    fgSizer3->Add(choice_procedure, 0, wxALL|wxEXPAND, 5);

    radiobtn_view = new wxRadioButton(granteePanel, ID_radiobtn, _("View:"));
    fgSizer3->Add(radiobtn_view, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    {
        ViewsPtr tc(databaseM->getViews());
        wxArrayString choices;
        for (Views::iterator it = tc->begin(); it != tc->end(); ++it)
            choices.Add((*it)->getName_());
        choice_view = new wxChoice(granteePanel, ID_choice, wxDefaultPosition,
            wxDefaultSize, choices);
        if (!choices.IsEmpty())
            choice_view->SetSelection(0);
    }
    choice_view->Enable(false);
    fgSizer3->Add(choice_view, 0, wxALL|wxEXPAND, 5);

    granteeSizer->Add(fgSizer3, 1, wxEXPAND, 0);
    granteePanel->SetSizer(granteeSizer);
    topLeftSizer->Add(granteePanel, 1, wxEXPAND | wxALL, 0);
    topSizer->Add(topLeftSizer, 0, wxEXPAND, 0);
    // separate left and right parts
    topSizer->Add(styleguide().getUnrelatedControlMargin(wxHORIZONTAL), 1);

    // PRIVILEGES: ---------------------------------------------------------
    wxPanel *privilegesPanel = new wxPanel(getControlsPanel());
    wxStaticBoxSizer *privilegesSizer = new wxStaticBoxSizer(wxVERTICAL,
        privilegesPanel, _("Privileges"));
    wxFlexGridSizer *fgSizer4 = new wxFlexGridSizer(2, 2,
        styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin());
    fgSizer4->AddGrowableCol(1);

    radiobtn_relation = new wxRadioButton(privilegesPanel, ID_radiobtn,
        _("Table/View"));
    radiobtn_relation->SetValue(true);  // not needed on GTK, but doesn't hurt
    fgSizer4->Add(radiobtn_relation, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

    // relations
    {
        wxArrayString choices;
        TablesPtr tt(databaseM->getTables());
        size_t to_select = 0;
        for (Tables::iterator it = tt->begin(); it != tt->end(); ++it)
        {
            if (dynamic_cast<Table *>(object) == (*it).get())
                to_select = choices.GetCount();
            choices.Add((*it)->getName_());
        }
        ViewsPtr tv(databaseM->getViews());
        for (Views::iterator it = tv->begin(); it != tv->end(); ++it)
        {
            if (dynamic_cast<View *>(object) == (*it).get())
                to_select = choices.GetCount();
            choices.Add((*it)->getName_());
        }
        choice_relations = new wxChoice(privilegesPanel, ID_choice,
            wxDefaultPosition, wxDefaultSize, choices);
        if (!choices.IsEmpty())
            choice_relations->SetSelection(static_cast<int>(to_select));
    }
    choice_relations->Enable(false);
    fgSizer4->Add(choice_relations, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 0);

    checkbox_all = new wxCheckBox(privilegesPanel, ID_checkbox, "All");
    checkbox_select = new wxCheckBox(privilegesPanel, ID_checkbox,
        "Select");
    checkbox_insert = new wxCheckBox(privilegesPanel, ID_checkbox,
        "Insert");
    checkbox_update = new wxCheckBox(privilegesPanel, ID_checkbox,
        "Update");
    checkbox_delete = new wxCheckBox(privilegesPanel, ID_checkbox,
        "Delete");
    checkbox_references = new wxCheckBox(privilegesPanel, ID_checkbox,
        "References");

    int indentation = 20;
    fgSizer4->Add(checkbox_all, 0, wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND,
        indentation);
    fgSizer4->AddSpacer(1);
    fgSizer4->Add(checkbox_select, 0, wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND,
        indentation);
    fgSizer4->AddSpacer(1);
    fgSizer4->Add(checkbox_insert, 0, wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND,
        indentation);
    fgSizer4->AddSpacer(1);

    fgSizer4->Add(checkbox_update, 0, wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND,
        indentation);
    wxBoxSizer *bSizer2 = new wxBoxSizer(wxHORIZONTAL);
    textctrl_update = new wxTextCtrl(privilegesPanel, ID_textctrl, "",
        wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    textctrl_update->SetBackgroundColour(
        wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    bSizer2->Add(textctrl_update, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 0);
    button_update_browse = new wxButton(privilegesPanel, ID_button_browse,
        "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    bSizer2->Add(button_update_browse, 0, wxLEFT | wxALIGN_CENTER_VERTICAL,
        styleguide().getBrowseButtonMargin());
    fgSizer4->Add(bSizer2, 1, wxEXPAND, 0);

    fgSizer4->Add(checkbox_delete, 0, wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND,
        indentation);
    fgSizer4->AddSpacer(1);
    fgSizer4->Add(checkbox_references, 0,
        wxLEFT|wxALIGN_CENTER_VERTICAL|wxEXPAND, indentation);
    wxBoxSizer *bSizer3 = new wxBoxSizer(wxHORIZONTAL);
    textctrl_references = new wxTextCtrl(privilegesPanel, ID_textctrl,
        "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    textctrl_references->SetBackgroundColour(
        wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    bSizer3->Add(textctrl_references, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 0);
    button_references_browse = new wxButton(privilegesPanel,
        ID_button_browse, "...", wxDefaultPosition,
        wxDefaultSize, wxBU_EXACTFIT);
    bSizer3->Add(button_references_browse, 0, wxLEFT | wxALIGN_CENTER_VERTICAL,
        styleguide().getBrowseButtonMargin());
    fgSizer4->Add(bSizer3, 1, wxEXPAND, 0);

    // procedure
    radiobtn_execute = new wxRadioButton(privilegesPanel, ID_radiobtn,
        _("Procedure"));
    fgSizer4->Add(radiobtn_execute, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    {
        ProceduresPtr tc = databaseM->getProcedures();
        wxArrayString choices;
        size_t to_select = 0;
        for (Procedures::iterator it = tc->begin(); it != tc->end(); ++it)
        {
            if (dynamic_cast<Procedure *>(object) == (*it).get())
                to_select = choices.GetCount();
            choices.Add((*it)->getName_());
        }
        choice_execute = new wxChoice(privilegesPanel, ID_choice,
            wxDefaultPosition, wxDefaultSize, choices);
        if (!choices.IsEmpty())
            choice_execute->SetSelection(static_cast<int>(to_select));
    }
    choice_execute->Enable(false);
    fgSizer4->Add(choice_execute, 0, wxEXPAND, 0);

    // roles
    radiobtn_memberof = new wxRadioButton(privilegesPanel, ID_radiobtn,
        _("Role"));
    fgSizer4->Add(radiobtn_memberof, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    {
        RolesPtr tc(databaseM->getRoles());
        wxArrayString choices;
        size_t to_select = 0;
        for (Roles::iterator it = tc->begin(); it != tc->end(); ++it)
        {
            if (dynamic_cast<Role *>(object) == (*it).get())
                to_select = choices.GetCount();
            choices.Add((*it)->getName_());
        }
        choice_memberof = new wxChoice(privilegesPanel, ID_choice,
            wxDefaultPosition, wxDefaultSize, choices);
        if (!choices.IsEmpty())
            choice_memberof->SetSelection(static_cast<int>(to_select));
    }
    choice_memberof->Enable(false);
    fgSizer4->Add(choice_memberof, 0, wxEXPAND, 0);
    privilegesSizer->Add(fgSizer4, 1, wxEXPAND|wxALL, 5);
    privilegesPanel->SetSizer(privilegesSizer);
    topSizer->Add(privilegesPanel, 1, wxEXPAND, 0);
    // PRIVILEGES done

    innerSizer->Add(topSizer, 0, wxEXPAND, 0);
    innerSizer->Add(styleguide().getUnrelatedControlMargin(wxVERTICAL),
        1, 0, wxALL, 0);
    wxBoxSizer *previewSqlSizer = new wxBoxSizer(wxHORIZONTAL);
    label_sql = new wxStaticText(getControlsPanel(), wxID_ANY,
        _("Preview SQL:"));
    previewSqlSizer->Add(label_sql, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL,
        styleguide().getControlLabelMargin());
    textbox_current_sql = new wxTextCtrl(getControlsPanel(), wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    textbox_current_sql->SetBackgroundColour(
        wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    textbox_current_sql->Enable(false);
    previewSqlSizer->Add(textbox_current_sql, 1, wxALIGN_CENTER_VERTICAL, 0);
    button_add = new wxButton(getControlsPanel(), ID_button_add,
        _("&Add To List"));
    previewSqlSizer->Add(button_add, 0, wxLEFT|wxALIGN_CENTER_VERTICAL,
        styleguide().getRelatedControlMargin(wxHORIZONTAL));
    innerSizer->Add(previewSqlSizer, 0, wxEXPAND|wxTOP,
        styleguide().getRelatedControlMargin(wxVERTICAL));

    m_staticline2 = new wxStaticLine(getControlsPanel(), wxID_ANY,
        wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
    innerSizer->Add(m_staticline2, 0, wxTOP|wxEXPAND,
        styleguide().getRelatedControlMargin(wxVERTICAL));

    label_statements = new wxStaticText(getControlsPanel(), wxID_ANY,
        _("List of SQL statements to execute:"));
    innerSizer->Add(label_statements, 0, wxTOP,
        styleguide().getUnrelatedControlMargin(wxVERTICAL));
    listbox_statements = new wxListBox(getControlsPanel(),
        ID_listbox_statements, wxDefaultPosition, wxDefaultSize, 0, 0, 0);
    innerSizer->Add(listbox_statements, 1, wxEXPAND|wxTOP,
        styleguide().getRelatedControlMargin(wxVERTICAL));

    button_remove = new wxButton(getControlsPanel(), ID_button_remove,
        _("&Remove Selected"), wxDefaultPosition, wxDefaultSize, 0);
    button_remove->Enable(false);
    button_execute = new wxButton(getControlsPanel(), wxID_OK,
        _("&Execute All"));
    button_execute->Enable(false);
    button_close = new wxButton(getControlsPanel(), wxID_CANCEL, _("&Close"));

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_execute,
        button_close);
    sizerButtons->Prepend(button_remove);
    // use method in base class to set everything up
    layoutSizers(innerSizer, sizerButtons, true);

    inConstructor = false;
    updateControls();
}

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

const wxString PrivilegesDialog::getName() const
{
    return "PrivilegesDialog";
}

void PrivilegesDialog::updateControls()
{
    if (inConstructor)
        return;

    // enable left-size choices
    textctrl_user->Enable(radiobtn_user->GetValue());
    choice_trigger->Enable(radiobtn_trigger->GetValue());
    choice_procedure->Enable(radiobtn_procedure->GetValue());
    choice_view->Enable(radiobtn_view->GetValue());

    // disable role granting for non-user grantees
    if (!radiobtn_user->GetValue() && radiobtn_memberof->GetValue())
        radiobtn_relation->SetValue(true);
    radiobtn_memberof->Enable(radiobtn_user->GetValue());

    // enable right-side choices
    bool isRelation = radiobtn_relation->GetValue();
    bool isProcedure = radiobtn_execute->GetValue();
    bool isRole = radiobtn_memberof->GetValue();
    choice_relations->Enable(isRelation);
    enableRelationCheckboxes(isRelation, true);
    choice_execute->Enable(isProcedure);
    choice_memberof->Enable(isRole);

    bool hasRelPriv = false;
    for (const auto box : {
            checkbox_select, checkbox_insert, checkbox_all,
            checkbox_update, checkbox_delete, checkbox_references
        })
    {
        if (box->IsChecked())
            hasRelPriv = true;
    }

    // if ALL is selected, turn the rest off...
    if (checkbox_all->IsChecked())
        enableRelationCheckboxes(false, false);
    else if (hasRelPriv) // ...and vice-versa
        checkbox_all->Enable(false);

    bool update = (checkbox_update->IsChecked() && isRelation);
    bool refer = (checkbox_references->IsChecked() && isRelation);
    textctrl_update->Enable(update);
    textctrl_references->Enable(refer);
    button_update_browse->Enable(update);
    button_references_browse->Enable(refer);

    /* CONSTRUCT SQL STATEMENT.   Various combinations covered:
    GRANT            ... ON table/view     TO   grantee [WGO]
    REVOKE [GOF]     ... ON table/view     FROM grantee
    GRANT            ... ON PROCEDURE proc TO   grantee [WGO]
    REVOKE [GOF] EXECUTE ON PROCEDURE proc FROM grantee
    GRANT        rolename                  TO   grantee [WAO]
    REVOKE [AOF] rolename                  FROM grantee
    */

    bool hasSomething = (isRelation && hasRelPriv) || isProcedure || isRole;
    button_add->Enable(hasSomething);
    if (!hasSomething)
    {
        textbox_current_sql->SetValue(wxEmptyString);
        return;
    }

    wxString grantee;
    // some usernames need quoting
    if (radiobtn_user->GetValue())
    {
        grantee = Identifier::userString(textctrl_user->GetValue());
    }
    else if (radiobtn_trigger->GetValue())
    {
        grantee = "TRIGGER " + Identifier(
                  choice_trigger->GetStringSelection()).getQuoted();
    }
    else if (radiobtn_procedure->GetValue())
    {
        grantee = "PROCEDURE " + Identifier(
                  choice_procedure->GetStringSelection()).getQuoted();
    }
    else if (radiobtn_view->GetValue())
    {
        grantee = "VIEW " + Identifier(
                  choice_view->GetStringSelection()).getQuoted();
    }
    bool grant = radiobox_action->GetSelection() < 2;
    bool grantoption = (radiobox_action->GetSelection() % 2 == 1);
    wxString sql(grant ? "GRANT " : "REVOKE ");
    if (!grant && grantoption)
        sql << (isRole ? "ADMIN" : "GRANT") << " OPTION FOR ";
    if (isRole)
        sql << Identifier(choice_memberof->GetStringSelection()).getQuoted();
    else if (isRelation)
    {
        wxString priv;
        if (checkbox_all->IsChecked())
            priv = "ALL";
        else
        {
            wxString upd = textctrl_update->GetValue();
            wxString ref = textctrl_references->GetValue();
            for (const auto box : {
                    checkbox_select, checkbox_insert,
                    checkbox_update, checkbox_delete, checkbox_references
                })
            {
                if (box->IsChecked())
                {
                    if (!priv.IsEmpty())
                        priv += ",";
                    priv += box->GetLabel().Upper();
                    if (box == checkbox_update && !upd.IsEmpty())
                        priv += "(" + upd + ")";
                    if (box == checkbox_references && !ref.IsEmpty())
                        priv += "(" + ref + ")";
                }
            }
        }
        sql << priv << " ON " << Identifier(
            choice_relations->GetStringSelection()).getQuoted();
    }
    else if (isProcedure)
    {
        sql << "EXECUTE ON PROCEDURE "
            << Identifier(choice_execute->GetStringSelection()).getQuoted();
    }

    sql << (grant ? " TO " : " FROM ") << grantee;
    if (grant && grantoption)
    {
        sql << " WITH " << (isRole ? "ADMIN" : "GRANT")
            << " OPTION";
    }

    textbox_current_sql->SetValue(sql);
}

wxString PrivilegesDialog::getSqlStatements()
{
    wxString stmt;
    for (int i = 0; i < (int)listbox_statements->GetCount(); i++)
        stmt << listbox_statements->GetString(i) << ";\n";
    return stmt;
}

//! event handling
BEGIN_EVENT_TABLE(PrivilegesDialog, BaseDialog)
    EVT_BUTTON(PrivilegesDialog::ID_button_add,
        PrivilegesDialog::OnButtonAddClick)
    EVT_BUTTON(PrivilegesDialog::ID_button_remove,
        PrivilegesDialog::OnButtonRemoveClick)
    EVT_BUTTON(PrivilegesDialog::ID_button_browse,
        PrivilegesDialog::OnButtonBrowseClick)
    EVT_CHECKBOX(PrivilegesDialog::ID_checkbox,
        PrivilegesDialog::OnSettingChanged)
    EVT_CHOICE(PrivilegesDialog::ID_choice, PrivilegesDialog::OnSettingChanged)
    EVT_LISTBOX(PrivilegesDialog::ID_listbox,
        PrivilegesDialog::OnSettingChanged)
    EVT_LISTBOX(PrivilegesDialog::ID_listbox_statements,
        PrivilegesDialog::OnListBoxStatementsSelected)
    EVT_RADIOBOX(PrivilegesDialog::ID_radiobox_action,
        PrivilegesDialog::OnSettingChanged)
    EVT_RADIOBUTTON(PrivilegesDialog::ID_radiobtn,
        PrivilegesDialog::OnSettingChanged)
    EVT_TEXT(PrivilegesDialog::ID_textctrl,
        PrivilegesDialog::OnSettingChanged)
END_EVENT_TABLE()

void PrivilegesDialog::OnListBoxStatementsSelected(wxCommandEvent&
    WXUNUSED(event))
{
    button_remove->Enable(listbox_statements->GetSelection() != wxNOT_FOUND);
}

void PrivilegesDialog::OnButtonAddClick(wxCommandEvent& WXUNUSED(event))
{
    listbox_statements->Append(textbox_current_sql->GetValue());
    button_execute->Enable();
}

void PrivilegesDialog::OnButtonRemoveClick(wxCommandEvent& WXUNUSED(event))
{
    int sel = listbox_statements->GetSelection();
    if (sel != wxNOT_FOUND)
        listbox_statements->Delete(sel);
    button_remove->Enable(false);
    if (listbox_statements->GetCount() == 0)
        button_execute->Enable(false);
}

void PrivilegesDialog::OnButtonBrowseClick(wxCommandEvent& event)
{
    Identifier id(choice_relations->GetStringSelection());
    Relation* r = databaseM->findRelation(id);
    if (!r)
        return;
    // load list of columns and let user select
    wxString cols = selectRelationColumns(r, this);
    // display in text ctrl
    wxTextCtrl* tc = 0;
    if (event.GetEventObject() == button_update_browse)
        tc = textctrl_update;
    if (event.GetEventObject() == button_references_browse)
        tc = textctrl_references;
    if (tc)
        tc->SetValue(cols);
}

void PrivilegesDialog::OnSettingChanged(wxCommandEvent& WXUNUSED(event))
{
    updateControls();
}

class ManagePrivilegesHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    ManagePrivilegesHandler() {};
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const ManagePrivilegesHandler handlerInstance;
};

const ManagePrivilegesHandler ManagePrivilegesHandler::handlerInstance;

bool ManagePrivilegesHandler::handleURI(URI& uri)
{
    if (uri.action != "manage_privileges")
        return false;

    wxWindow* w = getParentWindow(uri);
    MetadataItem *m = extractMetadataItemFromURI<MetadataItem>(uri);
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
            execSql(w, _("Grant And Revoke Privileges"), m->getDatabase(),
                statements, true);
        }
    }
    return true;
}


