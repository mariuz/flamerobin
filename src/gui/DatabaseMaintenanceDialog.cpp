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
    #include "wx/wx.h"
#endif

#include "gui/DatabaseMaintenanceDialog.h"
#include "gui/StyleGuide.h"
#include "engine/db/IDatabase.h"
#include "engine/db/ITransaction.h"
#include "engine/db/IStatement.h"
#include "core/StringUtils.h"

enum {
    ID_button_recalc_all_indices = 4001,
    ID_button_rebuild_all_indices,
    ID_button_recalc_selected_index,
    ID_button_run_sweep,
    ID_list_indices
};

BEGIN_EVENT_TABLE(DatabaseMaintenanceDialog, BaseDialog)
    EVT_BUTTON(ID_button_recalc_all_indices, DatabaseMaintenanceDialog::OnRecalcAllIndices)
    EVT_BUTTON(ID_button_rebuild_all_indices, DatabaseMaintenanceDialog::OnRebuildAllIndices)
    EVT_BUTTON(ID_button_recalc_selected_index, DatabaseMaintenanceDialog::OnRecalcSelectedIndex)
    EVT_BUTTON(ID_button_run_sweep, DatabaseMaintenanceDialog::OnRunSweep)
END_EVENT_TABLE()

DatabaseMaintenanceDialog::DatabaseMaintenanceDialog(wxWindow* parent, Database* db)
    : BaseDialog(parent, -1, wxEmptyString), databaseM(db)
{
    if (databaseM)
        SetTitle(wxString::Format(_("Database Maintenance & Health Dashboard - %s"), databaseM->getName_().c_str()));

    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);

    wxNotebook* notebook = new wxNotebook(this, -1);

    // --- Tab 1: Index Maintenance ---
    wxPanel* panelIndices = new wxPanel(notebook, -1);
    wxBoxSizer* sizerIndices = new wxBoxSizer(wxVERTICAL);

    list_indices = new wxListCtrl(panelIndices, ID_list_indices, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    list_indices->InsertColumn(0, _("Index Name"), wxLIST_FORMAT_LEFT, 180);
    list_indices->InsertColumn(1, _("Table Name"), wxLIST_FORMAT_LEFT, 180);
    list_indices->InsertColumn(2, _("Unique"), wxLIST_FORMAT_LEFT, 70);
    list_indices->InsertColumn(3, _("Selectivity"), wxLIST_FORMAT_LEFT, 110);
    list_indices->InsertColumn(4, _("Status"), wxLIST_FORMAT_LEFT, 80);

    wxBoxSizer* sizerBtns = new wxBoxSizer(wxHORIZONTAL);
    button_recalc_all_indices = new wxButton(panelIndices, ID_button_recalc_all_indices, _("Recalculate All Selectivities"));
    button_rebuild_all_indices = new wxButton(panelIndices, ID_button_rebuild_all_indices, _("Rebuild / Defrag All Indices"));
    button_recalc_selected_index = new wxButton(panelIndices, ID_button_recalc_selected_index, _("Recalculate Selected Index"));

    sizerBtns->Add(button_recalc_all_indices, 0, wxRIGHT, 6);
    sizerBtns->Add(button_rebuild_all_indices, 0, wxRIGHT, 6);
    sizerBtns->Add(button_recalc_selected_index, 0);

    sizerIndices->Add(list_indices, 1, wxEXPAND | wxALL, 6);
    sizerIndices->Add(sizerBtns, 0, wxALL | wxALIGN_RIGHT, 6);
    panelIndices->SetSizer(sizerIndices);
    notebook->AddPage(panelIndices, _("Index Maintenance"));

    // --- Tab 2: Database Health & Sweep ---
    wxPanel* panelHealth = new wxPanel(notebook, -1);
    wxFlexGridSizer* sizerGrid = new wxFlexGridSizer(2, 6, 12);

    sizerGrid->Add(new wxStaticText(panelHealth, -1, _("ODS Version:")), 0, wxALIGN_CENTER_VERTICAL);
    text_ods = new wxStaticText(panelHealth, -1, _("Unknown"));
    sizerGrid->Add(text_ods, 0, wxALIGN_CENTER_VERTICAL);

    sizerGrid->Add(new wxStaticText(panelHealth, -1, _("Page Size:")), 0, wxALIGN_CENTER_VERTICAL);
    text_page_size = new wxStaticText(panelHealth, -1, _("Unknown"));
    sizerGrid->Add(text_page_size, 0, wxALIGN_CENTER_VERTICAL);

    sizerGrid->Add(new wxStaticText(panelHealth, -1, _("Oldest Active Transaction:")), 0, wxALIGN_CENTER_VERTICAL);
    text_oldest_tx = new wxStaticText(panelHealth, -1, _("Unknown"));
    sizerGrid->Add(text_oldest_tx, 0, wxALIGN_CENTER_VERTICAL);

    sizerGrid->Add(new wxStaticText(panelHealth, -1, _("Next Transaction ID:")), 0, wxALIGN_CENTER_VERTICAL);
    text_next_tx = new wxStaticText(panelHealth, -1, _("Unknown"));
    sizerGrid->Add(text_next_tx, 0, wxALIGN_CENTER_VERTICAL);

    button_run_sweep = new wxButton(panelHealth, ID_button_run_sweep, _("Trigger Database Sweep Now"));

    wxBoxSizer* sizerHealth = new wxBoxSizer(wxVERTICAL);
    sizerHealth->Add(sizerGrid, 0, wxALL, 12);
    sizerHealth->Add(button_run_sweep, 0, wxLEFT | wxBOTTOM, 12);
    panelHealth->SetSizer(sizerHealth);
    notebook->AddPage(panelHealth, _("Database Health & Sweep"));

    sizerMain->Add(notebook, 1, wxEXPAND | wxALL, 6);

    // Bottom Execution Log Console
    sizerMain->Add(new wxStaticText(this, -1, _("Maintenance Console Output:")), 0, wxLEFT | wxTOP, 6);
    text_log = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxSize(-1, 140), wxTE_MULTILINE | wxTE_READONLY);
    sizerMain->Add(text_log, 0, wxEXPAND | wxALL, 6);

    progress_gauge = new wxGauge(this, -1, 100, wxDefaultPosition, wxSize(-1, 14));
    sizerMain->Add(progress_gauge, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    wxSizer* sizerButtons = CreateButtonSizer(wxCLOSE);
    sizerMain->Add(sizerButtons, 0, wxEXPAND | wxALL, 6);

    SetSizerAndFit(sizerMain);
    SetSize(wxSize(680, 560));

    loadIndicesInfo();
    loadHealthInfo();
}

DatabaseMaintenanceDialog::~DatabaseMaintenanceDialog()
{
}

void DatabaseMaintenanceDialog::logMessage(const wxString& msg)
{
    text_log->AppendText(msg + "\n");
}

void DatabaseMaintenanceDialog::loadHealthInfo()
{
    if (!databaseM) return;
    if (!databaseM->isConnected())
    {
        try { databaseM->connect(databaseM->getRawPassword()); }
        catch (...) { return; }
    }

    auto dalDb = databaseM->getDALDatabase();
    if (!dalDb || !dalDb->isConnected()) return;

    try
    {
        auto tr = dalDb->createTransaction();
        tr->start();
        auto st = dalDb->createStatement(tr);
        st->prepare("SELECT MON$ODS_MAJOR || '.' || MON$ODS_MINOR AS ods_ver, MON$PAGE_SIZE, MON$OLDEST_TRANSACTION, MON$NEXT_TRANSACTION FROM MON$DATABASE;");
        st->execute();

        if (st->fetch())
        {
            text_ods->SetLabel(wxString::FromUTF8(st->getString(0).c_str()));
            text_page_size->SetLabel(wxString::Format("%d bytes", st->getInt32(1)));
            text_oldest_tx->SetLabel(wxString::Format("%lld", (long long)st->getInt64(2)));
            text_next_tx->SetLabel(wxString::Format("%lld", (long long)st->getInt64(3)));
        }
        tr->commit();
    }
    catch (...) {}
}

void DatabaseMaintenanceDialog::loadIndicesInfo()
{
    if (!databaseM) return;
    if (!databaseM->isConnected())
    {
        try { databaseM->connect(databaseM->getRawPassword()); }
        catch (...) { return; }
    }

    auto dalDb = databaseM->getDALDatabase();
    if (!dalDb || !dalDb->isConnected()) return;

    indicesM.clear();
    try
    {
        auto tr = dalDb->createTransaction();
        tr->start();
        auto st = dalDb->createStatement(tr);
        st->prepare("SELECT RDB$INDEX_NAME, RDB$RELATION_NAME, RDB$UNIQUE_FLAG, RDB$STATISTICS, RDB$INDEX_INACTIVE "
                    "FROM RDB$INDICES WHERE (RDB$SYSTEM_FLAG = 0 OR RDB$SYSTEM_FLAG IS NULL) "
                    "ORDER BY RDB$RELATION_NAME, RDB$INDEX_NAME;");
        st->execute();

        while (st->fetch())
        {
            IndexHealthInfo idx;
            idx.name = wxString::FromUTF8(st->getString(0).c_str()).Trim();
            idx.relationName = wxString::FromUTF8(st->getString(1).c_str()).Trim();
            idx.isUnique = st->isNull(2) ? false : (st->getInt32(2) != 0);
            idx.selectivity = st->isNull(3) ? 0.0 : st->getDouble(3);
            idx.isActive = st->isNull(4) ? true : (st->getInt32(4) == 0);
            indicesM.push_back(idx);
        }
        tr->commit();
    }
    catch (const std::exception& e)
    {
        logMessage(_("Error loading index information: ") + wxString::FromUTF8(e.what()));
    }

    list_indices->DeleteAllItems();
    for (size_t i = 0; i < indicesM.size(); ++i)
    {
        const auto& idx = indicesM[i];
        long itemIdx = list_indices->InsertItem(i, idx.name);
        list_indices->SetItem(itemIdx, 1, idx.relationName);
        list_indices->SetItem(itemIdx, 2, idx.isUnique ? _("Yes") : _("No"));
        list_indices->SetItem(itemIdx, 3, wxString::Format("%.6f", idx.selectivity));
        list_indices->SetItem(itemIdx, 4, idx.isActive ? _("Active") : _("Inactive"));
    }
}

void DatabaseMaintenanceDialog::OnRecalcAllIndices(wxCommandEvent& WXUNUSED(event))
{
    if (indicesM.empty()) return;

    logMessage(_("Starting index selectivity recalculation for all user indices..."));
    progress_gauge->SetValue(0);

    auto dalDb = databaseM->getDALDatabase();
    int count = 0;

    for (size_t i = 0; i < indicesM.size(); ++i)
    {
        const auto& idx = indicesM[i];
        try
        {
            auto tr = dalDb->createTransaction();
            tr->start();
            auto st = dalDb->createStatement(tr);
            std::string sql = "SET STATISTICS INDEX \"" + wx2std(idx.name) + "\";";
            st->prepare(sql);
            st->execute();
            tr->commit();

            count++;
            logMessage(wxString::Format(_("Updated statistics for index %s (Table: %s)"), idx.name.c_str(), idx.relationName.c_str()));
        }
        catch (const std::exception& e)
        {
            logMessage(wxString::Format(_("Error setting statistics for %s: %s"), idx.name.c_str(), wxString::FromUTF8(e.what()).c_str()));
        }

        progress_gauge->SetValue((int)(((i + 1) * 100) / indicesM.size()));
        ::wxYield();
    }

    logMessage(wxString::Format(_("Finished index selectivity recalculation. Recalculated %d indices."), count));
    loadIndicesInfo();
}

void DatabaseMaintenanceDialog::OnRebuildAllIndices(wxCommandEvent& WXUNUSED(event))
{
    if (indicesM.empty()) return;

    int res = ::wxMessageBox(_("Rebuilding all indices deactivates and reactivates each index to optimize B-tree balance. Continue?"),
        _("Confirm Index Rebuild"), wxYES_NO | wxICON_QUESTION, this);
    if (res != wxYES) return;

    logMessage(_("Starting rebuild/defragmentation for all user indices..."));
    progress_gauge->SetValue(0);

    auto dalDb = databaseM->getDALDatabase();
    int count = 0;

    for (size_t i = 0; i < indicesM.size(); ++i)
    {
        const auto& idx = indicesM[i];
        try
        {
            auto tr = dalDb->createTransaction();
            tr->start();
            auto st = dalDb->createStatement(tr);

            st->prepare("ALTER INDEX \"" + wx2std(idx.name) + "\" INACTIVE;");
            st->execute();
            st->prepare("ALTER INDEX \"" + wx2std(idx.name) + "\" ACTIVE;");
            st->execute();

            tr->commit();

            count++;
            logMessage(wxString::Format(_("Rebuilt index %s (Table: %s)"), idx.name.c_str(), idx.relationName.c_str()));
        }
        catch (const std::exception& e)
        {
            logMessage(wxString::Format(_("Error rebuilding index %s: %s"), idx.name.c_str(), wxString::FromUTF8(e.what()).c_str()));
        }

        progress_gauge->SetValue((int)(((i + 1) * 100) / indicesM.size()));
        ::wxYield();
    }

    logMessage(wxString::Format(_("Finished index rebuild/defragmentation. Rebuilt %d indices."), count));
    loadIndicesInfo();
}

void DatabaseMaintenanceDialog::OnRecalcSelectedIndex(wxCommandEvent& WXUNUSED(event))
{
    long itemIdx = list_indices->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (itemIdx < 0 || itemIdx >= (long)indicesM.size())
    {
        wxMessageBox(_("Please select an index from the list to recalculate."), _("Info"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    const auto& idx = indicesM[itemIdx];
    try
    {
        auto dalDb = databaseM->getDALDatabase();
        auto tr = dalDb->createTransaction();
        tr->start();
        auto st = dalDb->createStatement(tr);
        std::string sql = "SET STATISTICS INDEX \"" + wx2std(idx.name) + "\";";
        st->prepare(sql);
        st->execute();
        tr->commit();

        logMessage(wxString::Format(_("Updated statistics for index %s"), idx.name.c_str()));
        loadIndicesInfo();
    }
    catch (const std::exception& e)
    {
        logMessage(wxString::Format(_("Error setting statistics for %s: %s"), idx.name.c_str(), wxString::FromUTF8(e.what()).c_str()));
    }
}

void DatabaseMaintenanceDialog::OnRunSweep(wxCommandEvent& WXUNUSED(event))
{
    logMessage(_("Initiating database sweep..."));

    try
    {
        auto dalDb = databaseM->getDALDatabase();
        auto tr = dalDb->createTransaction();
        tr->start();
        auto st = dalDb->createStatement(tr);
        st->prepare("ALTER DATABASE SWEEP;");
        st->execute();
        tr->commit();

        logMessage(_("Database sweep completed successfully."));
        loadHealthInfo();
    }
    catch (const std::exception& e)
    {
        logMessage(wxString::Format(_("Database sweep failed: %s"), wxString::FromUTF8(e.what()).c_str()));
    }
}
