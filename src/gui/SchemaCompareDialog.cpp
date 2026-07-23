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

#include <wx/clipbrd.h>
#include <wx/notebook.h>
#include "gui/SchemaCompareDialog.h"
#include "gui/ExecuteSqlFrame.h"
#include "gui/StyleGuide.h"
#include "metadata/root.h"
#include "metadata/server.h"

enum {
    ID_choice_source_db = 2001,
    ID_choice_target_db,
    ID_button_compare,
    ID_list_diffs,
    ID_button_open_editor,
    ID_button_copy_script
};

BEGIN_EVENT_TABLE(SchemaCompareDialog, BaseDialog)
    EVT_BUTTON(ID_button_compare, SchemaCompareDialog::OnCompareButtonClick)
    EVT_BUTTON(ID_button_open_editor, SchemaCompareDialog::OnOpenEditorClick)
    EVT_BUTTON(ID_button_copy_script, SchemaCompareDialog::OnCopyScriptClick)
END_EVENT_TABLE()

SchemaCompareDialog::SchemaCompareDialog(wxWindow* parent, Database* sourceDb)
    : BaseDialog(parent, -1, _("Database Schema Comparison & Migration Generator"),
        wxDefaultPosition, wxSize(800, 600)), initialSourceDbM(sourceDb)
{
    wxPanel* p = getControlsPanel();

    wxBoxSizer* sizerTop = new wxBoxSizer(wxVERTICAL);

    // Database selection sizer
    wxFlexGridSizer* sizerDbs = new wxFlexGridSizer(2, 2, 5, 10);
    sizerDbs->AddGrowableCol(1, 1);

    sizerDbs->Add(new wxStaticText(p, -1, _("Source Database (Reference):")), 0, wxALIGN_CENTER_VERTICAL);
    choice_source_db = new wxChoice(p, ID_choice_source_db);
    sizerDbs->Add(choice_source_db, 1, wxEXPAND);

    sizerDbs->Add(new wxStaticText(p, -1, _("Target Database (To Migrate):")), 0, wxALIGN_CENTER_VERTICAL);
    choice_target_db = new wxChoice(p, ID_choice_target_db);
    sizerDbs->Add(choice_target_db, 1, wxEXPAND);

    sizerTop->Add(sizerDbs, 0, wxEXPAND | wxALL, 6);

    // Options sizer
    wxStaticBoxSizer* sizerOptions = new wxStaticBoxSizer(wxHORIZONTAL, p, _("Comparison Options"));
    check_tables = new wxCheckBox(p, -1, _("Tables")); check_tables->SetValue(true);
    check_columns = new wxCheckBox(p, -1, _("Columns")); check_columns->SetValue(true);
    check_indices = new wxCheckBox(p, -1, _("Indices")); check_indices->SetValue(true);
    check_views_procs = new wxCheckBox(p, -1, _("Views/Procs/Triggers")); check_views_procs->SetValue(true);
    check_domains_gens = new wxCheckBox(p, -1, _("Domains/Sequences")); check_domains_gens->SetValue(true);
    check_drop_extra = new wxCheckBox(p, -1, _("Generate DROP statements")); check_drop_extra->SetValue(false);

    sizerOptions->Add(check_tables, 0, wxRIGHT, 8);
    sizerOptions->Add(check_columns, 0, wxRIGHT, 8);
    sizerOptions->Add(check_indices, 0, wxRIGHT, 8);
    sizerOptions->Add(check_views_procs, 0, wxRIGHT, 8);
    sizerOptions->Add(check_domains_gens, 0, wxRIGHT, 8);
    sizerOptions->Add(check_drop_extra, 0);

    sizerTop->Add(sizerOptions, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    button_compare = new wxButton(p, ID_button_compare, _("Compare Schemas"));
    button_compare->SetFont(button_compare->GetFont().Bold());
    sizerTop->Add(button_compare, 0, wxALIGN_CENTER | wxBOTTOM, 6);

    // Splitter or notebook for diff list & migration script
    wxNotebook* notebookResults = new wxNotebook(p, -1);

    list_diffs = new wxListCtrl(notebookResults, ID_list_diffs, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    list_diffs->InsertColumn(0, _("Type"), wxLIST_FORMAT_LEFT, 100);
    list_diffs->InsertColumn(1, _("Object"), wxLIST_FORMAT_LEFT, 180);
    list_diffs->InsertColumn(2, _("Difference Description"), wxLIST_FORMAT_LEFT, 450);

    notebookResults->AddPage(list_diffs, _("Differences List"));

    editor_sql = new SqlEditor(notebookResults, wxID_ANY);
    notebookResults->AddPage(editor_sql, _("Migration DDL Script"));

    sizerTop->Add(notebookResults, 1, wxEXPAND | wxALL, 6);

    button_open_editor = new wxButton(p, ID_button_open_editor, _("Open in SQL Editor"));
    button_copy_script = new wxButton(p, ID_button_copy_script, _("Copy Script"));
    button_close = new wxButton(p, wxID_CANCEL, _("Close"));

    wxSizer* sizerButtons = styleguide().createButtonSizer(button_open_editor, button_copy_script, button_close);

    layoutSizers(sizerTop, sizerButtons, true);

    populateDatabaseChoices();
}

SchemaCompareDialog::~SchemaCompareDialog()
{
}

void SchemaCompareDialog::populateDatabaseChoices()
{
    choice_source_db->Clear();
    choice_target_db->Clear();
    availableDatabasesM.clear();

    if (!initialSourceDbM || !initialSourceDbM->getServer())
        return;

    MetadataItem* parent = initialSourceDbM->getServer()->getParent();
    Root* root = dynamic_cast<Root*>(parent);
    if (!root)
        return;

    int srcSel = 0;
    int tgtSel = 0;
    ServerPtrs servers(root->getServers());
    for (auto srv : servers)
    {
        DatabasePtrs databases(srv->getDatabases());
        for (auto dbPtr : databases)
        {
            if (!dbPtr) continue;

            wxString label = dbPtr->getName_() + " (" + srv->getHostname() + ")";
            choice_source_db->Append(label);
            choice_target_db->Append(label);
            availableDatabasesM.push_back(dbPtr);

            if (dbPtr.get() == initialSourceDbM)
                srcSel = (int)availableDatabasesM.size() - 1;
        }
    }

    if (availableDatabasesM.size() > 1)
        tgtSel = (srcSel == 0) ? 1 : 0;

    if (!availableDatabasesM.empty())
    {
        choice_source_db->SetSelection(srcSel);
        choice_target_db->SetSelection(tgtSel);
    }
}

void SchemaCompareDialog::updateDiffList(const std::vector<SchemaDiffItem>& items)
{
    list_diffs->DeleteAllItems();
    for (size_t i = 0; i < items.size(); ++i)
    {
        const auto& item = items[i];
        long idx = list_diffs->InsertItem(i, item.objectType);
        list_diffs->SetItem(idx, 1, item.objectName);
        list_diffs->SetItem(idx, 2, item.description);
    }
}

void SchemaCompareDialog::OnCompareButtonClick(wxCommandEvent& WXUNUSED(event))
{
    int srcIdx = choice_source_db->GetSelection();
    int tgtIdx = choice_target_db->GetSelection();

    if (srcIdx < 0 || tgtIdx < 0 || srcIdx >= (int)availableDatabasesM.size() || tgtIdx >= (int)availableDatabasesM.size())
    {
        wxMessageBox(_("Please select valid source and target databases for schema comparison."), _("Selection Required"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    if (srcIdx == tgtIdx)
    {
        wxMessageBox(_("Source and Target databases must be different!"), _("Invalid Selection"), wxOK | wxICON_WARNING, this);
        return;
    }

    DatabasePtr srcDb = availableDatabasesM[srcIdx];
    DatabasePtr tgtDb = availableDatabasesM[tgtIdx];

    if (!srcDb->isConnected())
    {
        try { srcDb->connect(srcDb->getRawPassword()); }
        catch (const std::exception& e)
        {
            wxMessageBox(wxString::Format(_("Failed to connect to source database '%s': %s"), srcDb->getName_().c_str(), e.what()), _("Error"), wxOK | wxICON_ERROR, this);
            return;
        }
    }

    if (!tgtDb->isConnected())
    {
        try { tgtDb->connect(tgtDb->getRawPassword()); }
        catch (const std::exception& e)
        {
            wxMessageBox(wxString::Format(_("Failed to connect to target database '%s': %s"), tgtDb->getName_().c_str(), e.what()), _("Error"), wxOK | wxICON_ERROR, this);
            return;
        }
    }

    wxBusyCursor wait;

    SchemaDiffOptions opts;
    opts.compareTables = check_tables->IsChecked();
    opts.compareColumns = check_columns->IsChecked();
    opts.compareIndices = check_indices->IsChecked();
    opts.compareViews = check_views_procs->IsChecked();
    opts.compareProcedures = check_views_procs->IsChecked();
    opts.compareTriggers = check_views_procs->IsChecked();
    opts.compareDomains = check_domains_gens->IsChecked();
    opts.compareGenerators = check_domains_gens->IsChecked();
    opts.compareExceptions = check_domains_gens->IsChecked();
    opts.generateDropStatements = check_drop_extra->IsChecked();

    lastDiffItemsM = SchemaDiff::compareDatabases(srcDb.get(), tgtDb.get(), opts);
    generatedScriptM = SchemaDiff::generateMigrationScript(lastDiffItemsM, srcDb.get(), tgtDb.get());

    updateDiffList(lastDiffItemsM);
    editor_sql->SetText(generatedScriptM);

    wxMessageBox(wxString::Format(_("Schema comparison completed!\nFound %d difference(s)."), (int)lastDiffItemsM.size()), _("Comparison Complete"), wxOK | wxICON_INFORMATION, this);
}

void SchemaCompareDialog::OnOpenEditorClick(wxCommandEvent& WXUNUSED(event))
{
    if (generatedScriptM.IsEmpty())
    {
        wxMessageBox(_("No migration script available. Please run 'Compare Schemas' first."), _("Info"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    int tgtIdx = choice_target_db->GetSelection();
    DatabasePtr tgtDb = (tgtIdx >= 0 && tgtIdx < (int)availableDatabasesM.size()) ? availableDatabasesM[tgtIdx] : DatabasePtr();

    ExecuteSqlFrame* eff = new ExecuteSqlFrame(GetParent(), -1, _("Migration Script"), tgtDb);
    eff->setSql(generatedScriptM);
    eff->Show(true);
}

void SchemaCompareDialog::OnCopyScriptClick(wxCommandEvent& WXUNUSED(event))
{
    if (generatedScriptM.IsEmpty())
        return;

    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(generatedScriptM));
        wxTheClipboard->Close();
        wxMessageBox(_("Migration DDL script copied to clipboard."), _("Copied"), wxOK | wxICON_INFORMATION, this);
    }
}
