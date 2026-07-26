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

#ifndef FR_SYSTEMPRIVILEGEMATRIXDIALOG_H
#define FR_SYSTEMPRIVILEGEMATRIXDIALOG_H

#include <vector>
#include <map>
#include <set>
#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/srchctrl.h>
#include <wx/statline.h>

#include "gui/BaseDialog.h"
#include "metadata/database.h"

class SystemPrivilegeMatrixDialog : public BaseDialog
{
private:
    DatabasePtr databaseM;

    wxComboBox* comboGranteeM;
    wxRadioButton* radioUserM;
    wxRadioButton* radioRoleM;
    wxGrid* gridMatrixM;
    wxSearchCtrl* searchCtrlM;
    wxListBox* listboxStatementsM;
    wxTextCtrl* textPreviewM;

    wxButton* buttonGrantM;
    wxButton* buttonGrantWithAdminM;
    wxButton* buttonRevokeM;
    wxButton* buttonAddStmtM;
    wxButton* buttonRemoveStmtM;
    wxButton* buttonExecuteM;
    wxButton* buttonCloseM;

    std::vector<wxString> privilegeNamesM;
    std::vector<wxString> privilegeDescriptionsM;

    struct PrivState {
        bool granted;
        bool withAdminOption;
    };
    std::map<wxString, std::map<wxString, PrivState>> currentPrivilegeMapM;

    void createControls();
    void layoutControls();
    void initPrivilegeDefinitions();
    void loadData();
    void updateGrid();
    void updateButtons();

    wxString getSelectedGrantee() const;
    wxString buildGrantSql(const wxString& priv, const wxString& grantee, bool withAdminOption);
    wxString buildRevokeSql(const wxString& priv, const wxString& grantee);

protected:
    virtual const wxString getName() const override;

public:
    SystemPrivilegeMatrixDialog(wxWindow* parent, DatabasePtr database);
    virtual ~SystemPrivilegeMatrixDialog();

    enum {
        ID_combo_grantee = 2000,
        ID_radio_user,
        ID_radio_role,
        ID_grid_matrix,
        ID_search_priv,
        ID_button_grant,
        ID_button_grant_admin,
        ID_button_revoke,
        ID_button_add_stmt,
        ID_button_remove_stmt,
        ID_button_execute_all,
        ID_listbox_stmts
    };

    void OnGranteeChanged(wxCommandEvent& event);
    void OnTypeChanged(wxCommandEvent& event);
    void OnSearchChanged(wxCommandEvent& event);
    void OnGridCellClick(wxGridEvent& event);
    void OnButtonGrant(wxCommandEvent& event);
    void OnButtonGrantWithAdmin(wxCommandEvent& event);
    void OnButtonRevoke(wxCommandEvent& event);
    void OnButtonAddStmt(wxCommandEvent& event);
    void OnButtonRemoveStmt(wxCommandEvent& event);
    void OnButtonExecuteAll(wxCommandEvent& event);
    void OnListBoxSelected(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // FR_SYSTEMPRIVILEGEMATRIXDIALOG_H
