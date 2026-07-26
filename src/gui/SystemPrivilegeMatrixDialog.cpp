/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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
#include <algorithm>

#include "engine/MetadataLoader.h"
#include "gui/ExecuteSql.h"
#include "gui/StyleGuide.h"
#include "gui/SystemPrivilegeMatrixDialog.h"
#include "metadata/role.h"
#include "metadata/User.h"
#include "sql/Identifier.h"

namespace {
struct SystemPrivInfo {
    const char* name;
    const char* description;
};

const SystemPrivInfo systemPrivilegesList[] = {
    {"ACCESS_ANY_OBJECT_IN_DATABASE", "Access all objects ignoring normal object privileges"},
    {"ACCESS_SHUTDOWN_DATABASE", "Access database even when in single-user shutdown mode"},
    {"CHANGE_HEADER_SETTINGS", "Modify database header settings via ALTER DATABASE"},
    {"CHANGE_MAPPING_RULES", "Create, alter, or drop authentication mapping rules"},
    {"CHANGE_SHUTDOWN_MODE", "Change database shutdown or online state"},
    {"CREATE_DATABASE", "Create new database files on server"},
    {"CREATE_PRIVILEGED_ROLES", "Create system-privileged roles"},
    {"CREATE_USER_TYPES", "Define custom user data types / domains"},
    {"DROP_DATABASE", "Drop database files"},
    {"GET_DBCRYPT_INFO", "View database encryption statistics and keys"},
    {"GRANT_REVOKE_ANY_DDL_RIGHT", "Grant or revoke DDL privileges to/from any user"},
    {"GRANT_REVOKE_ON_ANY_OBJECT", "Grant or revoke object privileges on any table, view, or procedure"},
    {"IGNORE_DB_TRIGGERS", "Bypass database-level connection/transaction triggers"},
    {"MODIFY_ANY_OBJECT_IN_DATABASE", "Modify or drop any database object regardless of ownership"},
    {"MODIFY_EXT_CONN_POOL", "Configure external connection pool parameters"},
    {"MONITOR_ANY_ATTACHMENT", "Monitor attachments and execution threads of all users"},
    {"PROFILE_ANY_ATTACHMENT", "Profile execution of queries for any attachment"},
    {"READ_RAW_PAGES", "Read raw database physical disk pages"},
    {"REPLICATE_INTO_DATABASE", "Execute replication applier streams"},
    {"SELECT_ANY_OBJECT_IN_DATABASE", "Select data from any table or view"},
    {"TRACE_ANY_ATTACHMENT", "Start trace sessions monitoring any database connection"},
    {"USE_GBAK_UTILITY", "Perform backup/restore operations via gbak service"},
    {"USE_GFIX_UTILITY", "Perform database repair and sweep operations via gfix service"},
    {"USE_GRANTED_BY_CLAUSE", "Specify GRANTED BY clause when granting privileges"},
    {"USE_GSTAT_UTILITY", "Analyze database header and page statistics via gstat service"},
    {"USE_NBACKUP_UTILITY", "Perform incremental physical backups via nbackup service"},
    {"USER_MANAGEMENT", "Create, modify, and drop user accounts and passwords"}
};
}

BEGIN_EVENT_TABLE(SystemPrivilegeMatrixDialog, BaseDialog)
    EVT_COMBOBOX(SystemPrivilegeMatrixDialog::ID_combo_grantee, SystemPrivilegeMatrixDialog::OnGranteeChanged)
    EVT_TEXT(SystemPrivilegeMatrixDialog::ID_combo_grantee, SystemPrivilegeMatrixDialog::OnGranteeChanged)
    EVT_RADIOBUTTON(SystemPrivilegeMatrixDialog::ID_radio_user, SystemPrivilegeMatrixDialog::OnTypeChanged)
    EVT_RADIOBUTTON(SystemPrivilegeMatrixDialog::ID_radio_role, SystemPrivilegeMatrixDialog::OnTypeChanged)
    EVT_TEXT(SystemPrivilegeMatrixDialog::ID_search_priv, SystemPrivilegeMatrixDialog::OnSearchChanged)
    EVT_GRID_CELL_LEFT_CLICK(SystemPrivilegeMatrixDialog::OnGridCellClick)
    EVT_BUTTON(SystemPrivilegeMatrixDialog::ID_button_grant, SystemPrivilegeMatrixDialog::OnButtonGrant)
    EVT_BUTTON(SystemPrivilegeMatrixDialog::ID_button_grant_admin, SystemPrivilegeMatrixDialog::OnButtonGrantWithAdmin)
    EVT_BUTTON(SystemPrivilegeMatrixDialog::ID_button_revoke, SystemPrivilegeMatrixDialog::OnButtonRevoke)
    EVT_BUTTON(SystemPrivilegeMatrixDialog::ID_button_add_stmt, SystemPrivilegeMatrixDialog::OnButtonAddStmt)
    EVT_BUTTON(SystemPrivilegeMatrixDialog::ID_button_remove_stmt, SystemPrivilegeMatrixDialog::OnButtonRemoveStmt)
    EVT_BUTTON(SystemPrivilegeMatrixDialog::ID_button_execute_all, SystemPrivilegeMatrixDialog::OnButtonExecuteAll)
    EVT_LISTBOX(SystemPrivilegeMatrixDialog::ID_listbox_stmts, SystemPrivilegeMatrixDialog::OnListBoxSelected)
END_EVENT_TABLE()

SystemPrivilegeMatrixDialog::SystemPrivilegeMatrixDialog(wxWindow* parent, DatabasePtr database)
    : BaseDialog(parent, -1, _("Granular System Privilege Security Matrix")), databaseM(database)
{
    initPrivilegeDefinitions();
    createControls();
    layoutControls();
    loadData();
    updateGrid();
    updateButtons();
}

SystemPrivilegeMatrixDialog::~SystemPrivilegeMatrixDialog()
{
}

const wxString SystemPrivilegeMatrixDialog::getName() const
{
    return "SystemPrivilegeMatrixDialog";
}

void SystemPrivilegeMatrixDialog::initPrivilegeDefinitions()
{
    privilegeNamesM.clear();
    privilegeDescriptionsM.clear();
    for (const auto& item : systemPrivilegesList)
    {
        privilegeNamesM.push_back(item.name);
        privilegeDescriptionsM.push_back(item.description);
    }
}

void SystemPrivilegeMatrixDialog::createControls()
{
    radioUserM = new wxRadioButton(getControlsPanel(), ID_radio_user, _("User"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    radioRoleM = new wxRadioButton(getControlsPanel(), ID_radio_role, _("Role"));
    radioRoleM->SetValue(true);

    comboGranteeM = new wxComboBox(getControlsPanel(), ID_combo_grantee, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN | wxCB_SORT);
    searchCtrlM = new wxSearchCtrl(getControlsPanel(), ID_search_priv, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    searchCtrlM->SetDescriptiveText(_("Filter privileges..."));
    searchCtrlM->ShowSearchButton(true);
    searchCtrlM->ShowCancelButton(true);

    gridMatrixM = new wxGrid(getControlsPanel(), ID_grid_matrix);
    gridMatrixM->CreateGrid((int)privilegeNamesM.size(), 3);
    gridMatrixM->SetColLabelValue(0, _("System Privilege"));
    gridMatrixM->SetColLabelValue(1, _("Granted Status"));
    gridMatrixM->SetColLabelValue(2, _("Description"));
    gridMatrixM->SetRowLabelSize(30);
    gridMatrixM->EnableEditing(false);
    gridMatrixM->SetSelectionMode(wxGrid::wxGridSelectRows);

    buttonGrantM = new wxButton(getControlsPanel(), ID_button_grant, _("&Grant"));
    buttonGrantWithAdminM = new wxButton(getControlsPanel(), ID_button_grant_admin, _("Grant &WITH ADMIN"));
    buttonRevokeM = new wxButton(getControlsPanel(), ID_button_revoke, _("&Revoke"));

    textPreviewM = new wxTextCtrl(getControlsPanel(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    textPreviewM->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    buttonAddStmtM = new wxButton(getControlsPanel(), ID_button_add_stmt, _("&Add Statement"));
    listboxStatementsM = new wxListBox(getControlsPanel(), ID_listbox_stmts, wxDefaultPosition, wxSize(-1, 100));
    buttonRemoveStmtM = new wxButton(getControlsPanel(), ID_button_remove_stmt, _("R&emove"));
    buttonExecuteM = new wxButton(getControlsPanel(), ID_button_execute_all, _("&Execute All"));
    buttonCloseM = new wxButton(getControlsPanel(), wxID_CANCEL, _("&Close"));
}

void SystemPrivilegeMatrixDialog::layoutControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Top Filter Bar
    wxBoxSizer* filterSizer = new wxBoxSizer(wxHORIZONTAL);
    filterSizer->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Target Type:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    filterSizer->Add(radioRoleM, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    filterSizer->Add(radioUserM, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 15);
    filterSizer->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Grantee (User / Role):")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    filterSizer->Add(comboGranteeM, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 15);
    filterSizer->Add(searchCtrlM, 1, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(filterSizer, 0, wxEXPAND | wxALL, 8);

    // Grid and Action buttons
    wxBoxSizer* gridContainerSizer = new wxBoxSizer(wxHORIZONTAL);
    gridContainerSizer->Add(gridMatrixM, 1, wxEXPAND | wxRIGHT, 8);

    wxBoxSizer* actionBtnSizer = new wxBoxSizer(wxVERTICAL);
    actionBtnSizer->Add(buttonGrantM, 0, wxEXPAND | wxBOTTOM, 6);
    actionBtnSizer->Add(buttonGrantWithAdminM, 0, wxEXPAND | wxBOTTOM, 6);
    actionBtnSizer->Add(buttonRevokeM, 0, wxEXPAND | wxBOTTOM, 12);

    gridContainerSizer->Add(actionBtnSizer, 0, wxALIGN_TOP);
    mainSizer->Add(gridContainerSizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // SQL Preview
    wxBoxSizer* previewSizer = new wxBoxSizer(wxHORIZONTAL);
    previewSizer->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Pending DDL:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    previewSizer->Add(textPreviewM, 1, wxEXPAND | wxRIGHT, 8);
    previewSizer->Add(buttonAddStmtM, 0, wxALIGN_CENTER_VERTICAL);
    mainSizer->Add(previewSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    mainSizer->Add(new wxStaticLine(getControlsPanel(), wxID_ANY), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    // Statements Queue
    mainSizer->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Queue of DDL Statements to Execute:")), 0, wxLEFT | wxRIGHT, 8);
    
    wxBoxSizer* queueSizer = new wxBoxSizer(wxHORIZONTAL);
    queueSizer->Add(listboxStatementsM, 1, wxEXPAND | wxRIGHT, 8);
    
    wxBoxSizer* queueBtnSizer = new wxBoxSizer(wxVERTICAL);
    queueBtnSizer->Add(buttonRemoveStmtM, 0, wxEXPAND | wxBOTTOM, 6);
    queueSizer->Add(queueBtnSizer, 0, wxALIGN_TOP);

    mainSizer->Add(queueSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Bottom buttons
    wxSizer* buttonSizer = styleguide().createButtonSizer(buttonExecuteM, buttonCloseM);
    layoutSizers(mainSizer, buttonSizer, true);

    SetSize(820, 640);
}

void SystemPrivilegeMatrixDialog::loadData()
{
    currentPrivilegeMapM.clear();
    comboGranteeM->Clear();

    if (!databaseM || !databaseM->isConnected())
        return;

    // Load roles & users into combo
    bool isRole = radioRoleM->GetValue();
    if (isRole)
    {
        RolesPtr roles(databaseM->getRoles());
        for (Roles::iterator it = roles->begin(); it != roles->end(); ++it)
            comboGranteeM->Append((*it)->getName_());
    }
    else
    {
        UsersPtr users(databaseM->getUsers());
        for (Users::iterator it = users->begin(); it != users->end(); ++it)
            comboGranteeM->Append((*it)->getName_());
    }

    if (comboGranteeM->GetCount() > 0)
        comboGranteeM->SetSelection(0);

    // Fetch privileges from RDB$USER_PRIVILEGES for system privileges
    try
    {
        MetadataLoader* loader = databaseM->getMetadataLoader();
        MetadataLoaderTransaction tr(loader);

        fr::IStatementPtr& st = loader->getStatement(
            "select RDB$USER, RDB$PRIVILEGE, RDB$GRANT_OPTION "
            "from RDB$USER_PRIVILEGES "
            "where RDB$OBJECT_TYPE = 13 or RDB$OBJECT_TYPE is null or RDB$RELATION_NAME is null "
            "order by RDB$USER"
        );
        st->execute();

        while (st->fetch())
        {
            wxString user = wxString::FromUTF8(st->getString(0).c_str()).Trim().Strip();
            wxString priv = wxString::FromUTF8(st->getString(1).c_str()).Trim().Strip();
            int grantOpt = st->isNull(2) ? 0 : st->getInt32(2);

            PrivState ps;
            ps.granted = true;
            ps.withAdminOption = (grantOpt != 0);

            currentPrivilegeMapM[user][priv] = ps;
        }
    }
    catch (...)
    {
        // Fail gracefully if schema version doesn't support query
    }
}

wxString SystemPrivilegeMatrixDialog::getSelectedGrantee() const
{
    wxString grantee = comboGranteeM->GetValue().Trim();
    if (grantee.IsEmpty())
        grantee = "PUBLIC";
    return grantee;
}

void SystemPrivilegeMatrixDialog::updateGrid()
{
    wxString grantee = getSelectedGrantee();
    wxString filterText = searchCtrlM->GetValue().Lower().Trim();

    gridMatrixM->BeginBatch();

    auto userPrivsIt = currentPrivilegeMapM.find(grantee);

    for (size_t i = 0; i < privilegeNamesM.size(); ++i)
    {
        wxString privName = privilegeNamesM[i];
        wxString desc = privilegeDescriptionsM[i];

        gridMatrixM->SetCellValue((int)i, 0, privName);
        gridMatrixM->SetCellValue((int)i, 2, desc);

        bool granted = false;
        bool withAdmin = false;

        if (userPrivsIt != currentPrivilegeMapM.end())
        {
            auto privIt = userPrivsIt->second.find(privName);
            if (privIt != userPrivsIt->second.end())
            {
                granted = privIt->second.granted;
                withAdmin = privIt->second.withAdminOption;
            }
        }

        wxString statusStr = _("NOT GRANTED");
        wxColour bgCol(240, 240, 240);
        wxColour fgCol(120, 120, 120);

        if (withAdmin)
        {
            statusStr = _("GRANTED (WITH ADMIN)");
            bgCol = wxColour(180, 230, 200);
            fgCol = wxColour(0, 100, 40);
        }
        else if (granted)
        {
            statusStr = _("GRANTED");
            bgCol = wxColour(210, 240, 220);
            fgCol = wxColour(0, 120, 0);
        }

        gridMatrixM->SetCellValue((int)i, 1, statusStr);
        gridMatrixM->SetCellBackgroundColour((int)i, 1, bgCol);
        gridMatrixM->SetCellTextColour((int)i, 1, fgCol);

        // Filtering visibility
        if (!filterText.IsEmpty() && !privName.Lower().Contains(filterText) && !desc.Lower().Contains(filterText))
            gridMatrixM->HideRow((int)i);
        else
            gridMatrixM->ShowRow((int)i);
    }

    gridMatrixM->AutoSizeColumns();
    gridMatrixM->EndBatch();
}

void SystemPrivilegeMatrixDialog::updateButtons()
{
    int selectedRow = gridMatrixM->GetGridCursorRow();
    bool hasSelection = (selectedRow >= 0 && selectedRow < (int)privilegeNamesM.size());
    
    buttonGrantM->Enable(hasSelection);
    buttonGrantWithAdminM->Enable(hasSelection);
    buttonRevokeM->Enable(hasSelection);
    buttonAddStmtM->Enable(!textPreviewM->GetValue().IsEmpty());
    
    buttonRemoveStmtM->Enable(listboxStatementsM->GetSelection() != wxNOT_FOUND);
    buttonExecuteM->Enable(listboxStatementsM->GetCount() > 0);
}

wxString SystemPrivilegeMatrixDialog::buildGrantSql(const wxString& priv, const wxString& grantee, bool withAdminOption)
{
    wxString sql = "GRANT " + priv + " TO " + Identifier(grantee).getQuoted();
    if (withAdminOption)
        sql += " WITH ADMIN OPTION";
    return sql;
}

wxString SystemPrivilegeMatrixDialog::buildRevokeSql(const wxString& priv, const wxString& grantee)
{
    return "REVOKE " + priv + " FROM " + Identifier(grantee).getQuoted();
}

void SystemPrivilegeMatrixDialog::OnGranteeChanged(wxCommandEvent& WXUNUSED(event))
{
    updateGrid();
}

void SystemPrivilegeMatrixDialog::OnTypeChanged(wxCommandEvent& WXUNUSED(event))
{
    loadData();
    updateGrid();
}

void SystemPrivilegeMatrixDialog::OnSearchChanged(wxCommandEvent& WXUNUSED(event))
{
    updateGrid();
}

void SystemPrivilegeMatrixDialog::OnGridCellClick(wxGridEvent& event)
{
    int row = event.GetRow();
    if (row >= 0 && row < (int)privilegeNamesM.size())
    {
        gridMatrixM->SelectRow(row);
        wxString priv = privilegeNamesM[row];
        wxString grantee = getSelectedGrantee();
        textPreviewM->SetValue(buildGrantSql(priv, grantee, false));
        updateButtons();
    }
    event.Skip();
}

void SystemPrivilegeMatrixDialog::OnButtonGrant(wxCommandEvent& WXUNUSED(event))
{
    int row = gridMatrixM->GetGridCursorRow();
    if (row >= 0 && row < (int)privilegeNamesM.size())
    {
        wxString priv = privilegeNamesM[row];
        wxString grantee = getSelectedGrantee();
        textPreviewM->SetValue(buildGrantSql(priv, grantee, false));
        updateButtons();
    }
}

void SystemPrivilegeMatrixDialog::OnButtonGrantWithAdmin(wxCommandEvent& WXUNUSED(event))
{
    int row = gridMatrixM->GetGridCursorRow();
    if (row >= 0 && row < (int)privilegeNamesM.size())
    {
        wxString priv = privilegeNamesM[row];
        wxString grantee = getSelectedGrantee();
        textPreviewM->SetValue(buildGrantSql(priv, grantee, true));
        updateButtons();
    }
}

void SystemPrivilegeMatrixDialog::OnButtonRevoke(wxCommandEvent& WXUNUSED(event))
{
    int row = gridMatrixM->GetGridCursorRow();
    if (row >= 0 && row < (int)privilegeNamesM.size())
    {
        wxString priv = privilegeNamesM[row];
        wxString grantee = getSelectedGrantee();
        textPreviewM->SetValue(buildRevokeSql(priv, grantee));
        updateButtons();
    }
}

void SystemPrivilegeMatrixDialog::OnButtonAddStmt(wxCommandEvent& WXUNUSED(event))
{
    wxString sql = textPreviewM->GetValue();
    if (!sql.IsEmpty())
    {
        listboxStatementsM->Append(sql);
        updateButtons();
    }
}

void SystemPrivilegeMatrixDialog::OnButtonRemoveStmt(wxCommandEvent& WXUNUSED(event))
{
    int sel = listboxStatementsM->GetSelection();
    if (sel != wxNOT_FOUND)
    {
        listboxStatementsM->Delete(sel);
        updateButtons();
    }
}

void SystemPrivilegeMatrixDialog::OnButtonExecuteAll(wxCommandEvent& WXUNUSED(event))
{
    if (listboxStatementsM->GetCount() == 0)
        return;

    wxString batchSql;
    for (size_t i = 0; i < listboxStatementsM->GetCount(); ++i)
        batchSql << listboxStatementsM->GetString((unsigned int)i) << ";\n";

    execSql(this, _("Executing System Privilege DDL"), databaseM, batchSql, true);
    EndModal(wxID_OK);
}

void SystemPrivilegeMatrixDialog::OnListBoxSelected(wxCommandEvent& WXUNUSED(event))
{
    updateButtons();
}
