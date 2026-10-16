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

#include "gui/DataImportDialog.h"
#include "gui/ProgressDialog.h"
#include "gui/StyleGuide.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/table.h"

#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/spinctrl.h>

BEGIN_EVENT_TABLE(DataImportDialog, BaseDialog)
    EVT_BUTTON(DataImportDialog::ID_button_browse, DataImportDialog::OnBrowse)
    EVT_TEXT(DataImportDialog::ID_textctrl_file, DataImportDialog::OnFileText)
    EVT_CHOICE(DataImportDialog::ID_choice_format, DataImportDialog::OnFormatChanged)
    EVT_BUTTON(DataImportDialog::ID_button_automap, DataImportDialog::OnAutoMap)
    EVT_BUTTON(DataImportDialog::ID_button_clear_mapping, DataImportDialog::OnClearMapping)
    EVT_BUTTON(DataImportDialog::ID_button_import, DataImportDialog::OnImport)
END_EVENT_TABLE()

DataImportDialog::DataImportDialog(wxWindow* parent, Database* db, Table* table)
    : BaseDialog(parent, -1, wxString::Format(_("Import Data into Table: %s"), table->getName_().c_str())),
      dbM(db), tableM(table)
{
    createControls();
    layoutControls();
    setControlProperties();
}

const wxString DataImportDialog::getName() const
{
    return "DataImportDialog";
}

void DataImportDialog::createControls()
{
    textctrl_file = new wxTextCtrl(getControlsPanel(), ID_textctrl_file, wxEmptyString);
    button_browse = new wxButton(getControlsPanel(), ID_button_browse, _("Browse..."));

    wxArrayString formats;
    formats.Add(_("Auto-detect"));
    formats.Add("JSON");
    formats.Add("XML");
    choice_format = new wxChoice(getControlsPanel(), ID_choice_format, wxDefaultPosition, wxDefaultSize, formats);
    choice_format->SetSelection(0);

    // Load table columns
    tableM->ensureChildrenLoaded();
    for (auto it = tableM->begin(); it != tableM->end(); ++it)
    {
        Column* col = (*it).get();
        columnNamesM.push_back(col->getName_());
        columnTypesM.push_back(col->getDatatype());
        columnComputedM.push_back(!col->getComputedSource().IsEmpty());
    }

    scroll_mappings = new wxScrolledWindow(getControlsPanel(), wxID_ANY,
        wxDefaultPosition, wxSize(550, 220), wxVSCROLL | wxSUNKEN_BORDER);
    scroll_mappings->SetScrollRate(0, 10);

    sizer_mappings = new wxFlexGridSizer(3, styleguide().getRelatedControlMargin(wxVERTICAL), styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizer_mappings->AddGrowableCol(0, 1);
    sizer_mappings->AddGrowableCol(1, 1);
    sizer_mappings->AddGrowableCol(2, 2);

    // Headers
    wxFont boldFont = scroll_mappings->GetFont();
    boldFont.SetWeight(wxFONTWEIGHT_BOLD);

    wxStaticText* h1 = new wxStaticText(scroll_mappings, wxID_ANY, _("Table Column"));
    h1->SetFont(boldFont);
    wxStaticText* h2 = new wxStaticText(scroll_mappings, wxID_ANY, _("Type"));
    h2->SetFont(boldFont);
    wxStaticText* h3 = new wxStaticText(scroll_mappings, wxID_ANY, _("Source Field"));
    h3->SetFont(boldFont);

    sizer_mappings->Add(h1, 0, wxALIGN_CENTER_VERTICAL | wxBOTTOM, 4);
    sizer_mappings->Add(h2, 0, wxALIGN_CENTER_VERTICAL | wxBOTTOM, 4);
    sizer_mappings->Add(h3, 0, wxALIGN_CENTER_VERTICAL | wxBOTTOM, 4);

    for (size_t i = 0; i < columnNamesM.size(); ++i)
    {
        wxStaticText* labelCol = new wxStaticText(scroll_mappings, wxID_ANY, columnNamesM[i]);
        wxStaticText* labelType = new wxStaticText(scroll_mappings, wxID_ANY, columnTypesM[i]);
        labelType->SetForegroundColour(*wxLIGHT_GREY);

        sizer_mappings->Add(labelCol, 0, wxALIGN_CENTER_VERTICAL);
        sizer_mappings->Add(labelType, 0, wxALIGN_CENTER_VERTICAL);

        if (columnComputedM[i])
        {
            wxStaticText* labelComputed = new wxStaticText(scroll_mappings, wxID_ANY, _("(Computed - skipped)"));
            labelComputed->SetForegroundColour(*wxLIGHT_GREY);
            sizer_mappings->Add(labelComputed, 0, wxALIGN_CENTER_VERTICAL);
            choiceMappingsM.push_back(nullptr);
        }
        else
        {
            wxChoice* choice = new wxChoice(scroll_mappings, wxID_ANY);
            choice->Append(_("(None)"));
            choice->SetSelection(0);
            sizer_mappings->Add(choice, 1, wxEXPAND);
            choiceMappingsM.push_back(choice);
        }
    }
    scroll_mappings->SetSizer(sizer_mappings);

    button_automap = new wxButton(getControlsPanel(), ID_button_automap, _("Auto-map by Name"));
    button_clear_mapping = new wxButton(getControlsPanel(), ID_button_clear_mapping, _("Clear Mappings"));

    spin_batch_size = new wxSpinCtrl(getControlsPanel(), ID_spin_batch_size, "500",
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 500);

    wxArrayString errorOptions;
    errorOptions.Add(_("Abort on error"));
    errorOptions.Add(_("Ignore errors and continue"));
    choice_error_handling = new wxChoice(getControlsPanel(), ID_choice_error_handling,
        wxDefaultPosition, wxDefaultSize, errorOptions);
    choice_error_handling->SetSelection(0);

    button_import = new wxButton(getControlsPanel(), ID_button_import, _("Import"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
}

void DataImportDialog::layoutControls()
{
    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);

    // File selection
    wxStaticBoxSizer* sizerFile = new wxStaticBoxSizer(wxVERTICAL, getControlsPanel(), _("Source File"));
    wxBoxSizer* sizerFileRow = new wxBoxSizer(wxHORIZONTAL);
    sizerFileRow->Add(textctrl_file, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerFileRow->Add(button_browse, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerFileRow->Add(choice_format, 0, wxALIGN_CENTER_VERTICAL);
    sizerFile->Add(sizerFileRow, 0, wxEXPAND | wxALL, styleguide().getControlLabelMargin());
    sizerMain->Add(sizerFile, 0, wxEXPAND | wxBOTTOM, styleguide().getRelatedControlMargin(wxVERTICAL));

    // Mappings
    wxStaticBoxSizer* sizerMapGroup = new wxStaticBoxSizer(wxVERTICAL, getControlsPanel(), _("Field to Column Mapping"));
    sizerMapGroup->Add(scroll_mappings, 1, wxEXPAND | wxBOTTOM, styleguide().getRelatedControlMargin(wxVERTICAL));

    wxBoxSizer* sizerMapBtns = new wxBoxSizer(wxHORIZONTAL);
    sizerMapBtns->Add(button_automap, 0, wxRIGHT, styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerMapBtns->Add(button_clear_mapping, 0);
    sizerMapGroup->Add(sizerMapBtns, 0, wxALIGN_RIGHT);
    sizerMain->Add(sizerMapGroup, 1, wxEXPAND | wxBOTTOM, styleguide().getRelatedControlMargin(wxVERTICAL));

    // Options
    wxStaticBoxSizer* sizerOpts = new wxStaticBoxSizer(wxVERTICAL, getControlsPanel(), _("Options"));
    wxFlexGridSizer* sizerGridOpts = new wxFlexGridSizer(2, 2, styleguide().getRelatedControlMargin(wxVERTICAL), styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerGridOpts->AddGrowableCol(1, 1);

    sizerGridOpts->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Commit batch size:")), 0, wxALIGN_CENTER_VERTICAL);
    sizerGridOpts->Add(spin_batch_size, 0);

    sizerGridOpts->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Error handling:")), 0, wxALIGN_CENTER_VERTICAL);
    sizerGridOpts->Add(choice_error_handling, 1, wxEXPAND);

    sizerOpts->Add(sizerGridOpts, 0, wxEXPAND | wxALL, styleguide().getControlLabelMargin());
    sizerMain->Add(sizerOpts, 0, wxEXPAND | wxBOTTOM, styleguide().getRelatedControlMargin(wxVERTICAL));

    // Buttons
    wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(button_import, 0, wxRIGHT, styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerButtons->Add(button_cancel, 0);

    layoutSizers(sizerMain, sizerButtons, true);
}

void DataImportDialog::setControlProperties()
{
    button_import->SetDefault();
}

void DataImportDialog::loadFileFields()
{
    wxString path = textctrl_file->GetValue().Trim();
    if (path.empty() || !wxFileExists(path))
        return;

    fr::DataTransferFormat fmt = fr::DataTransferFormat::Auto;
    int fmtSel = choice_format->GetSelection();
    if (fmtSel == 1)
        fmt = fr::DataTransferFormat::Json;
    else if (fmtSel == 2)
        fmt = fr::DataTransferFormat::Xml;

    currentFileFieldsM = fr::DataTransferHelper::extractFieldsFromFile(path, fmt);
    updateMappingChoices();
    runAutoMapping();
}

void DataImportDialog::updateMappingChoices()
{
    for (size_t i = 0; i < choiceMappingsM.size(); ++i)
    {
        wxChoice* c = choiceMappingsM[i];
        if (!c)
            continue;

        wxString prevSel = c->GetStringSelection();
        c->Clear();
        c->Append(_("(None)"));
        for (const auto& f : currentFileFieldsM)
            c->Append(f);

        int idx = c->FindString(prevSel);
        if (idx != wxNOT_FOUND)
            c->SetSelection(idx);
        else
            c->SetSelection(0);
    }
}

void DataImportDialog::runAutoMapping()
{
    if (currentFileFieldsM.empty())
        return;

    auto mapping = fr::DataTransferHelper::autoMapFields(columnNamesM, currentFileFieldsM);

    for (size_t i = 0; i < columnNamesM.size(); ++i)
    {
        wxChoice* c = choiceMappingsM[i];
        if (!c)
            continue;

        const wxString& col = columnNamesM[i];
        wxString matched = mapping[col];
        if (!matched.empty())
        {
            int idx = c->FindString(matched);
            if (idx != wxNOT_FOUND)
                c->SetSelection(idx);
        }
    }
}

void DataImportDialog::OnBrowse(wxCommandEvent& WXUNUSED(event))
{
    wxString fname = ::wxFileSelector(
        _("Select Data File to Import"),
        wxEmptyString,
        textctrl_file->GetValue(),
        wxEmptyString,
        _("Data files (*.json;*.xml)|*.json;*.xml|JSON files (*.json)|*.json|XML files (*.xml)|*.xml|All files (*.*)|*.*"),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST,
        this
    );

    if (!fname.empty())
    {
        textctrl_file->SetValue(fname);
        loadFileFields();
    }
}

void DataImportDialog::OnFileText(wxCommandEvent& WXUNUSED(event))
{
    wxString path = textctrl_file->GetValue().Trim();
    if (!path.empty() && wxFileExists(path))
    {
        loadFileFields();
    }
}

void DataImportDialog::OnFormatChanged(wxCommandEvent& WXUNUSED(event))
{
    loadFileFields();
}

void DataImportDialog::OnAutoMap(wxCommandEvent& WXUNUSED(event))
{
    runAutoMapping();
}

void DataImportDialog::OnClearMapping(wxCommandEvent& WXUNUSED(event))
{
    for (auto* c : choiceMappingsM)
    {
        if (c)
            c->SetSelection(0);
    }
}

void DataImportDialog::OnImport(wxCommandEvent& WXUNUSED(event))
{
    wxString filePath = textctrl_file->GetValue().Trim();
    if (filePath.empty() || !wxFileExists(filePath))
    {
        wxMessageBox(_("Please specify a valid source file to import."), _("Error"), wxOK | wxICON_ERROR, this);
        return;
    }

    std::map<wxString, wxString> mappings;
    bool hasAnyMapping = false;

    for (size_t i = 0; i < columnNamesM.size(); ++i)
    {
        wxChoice* c = choiceMappingsM[i];
        if (!c)
            continue;

        wxString sel = c->GetStringSelection();
        if (!sel.empty() && sel != _("(None)"))
        {
            mappings[columnNamesM[i]] = sel;
            hasAnyMapping = true;
        }
    }

    if (!hasAnyMapping)
    {
        wxMessageBox(_("Please map at least one column before importing."), _("Error"), wxOK | wxICON_ERROR, this);
        return;
    }

    fr::DataImportOptions options;
    int fmtSel = choice_format->GetSelection();
    if (fmtSel == 1)
        options.format = fr::DataTransferFormat::Json;
    else if (fmtSel == 2)
        options.format = fr::DataTransferFormat::Xml;
    else
        options.format = fr::DataTransferFormat::Auto;

    options.fieldMappings = mappings;
    options.commitBatchSize = spin_batch_size->GetValue();
    options.errorHandling = (choice_error_handling->GetSelection() == 1)
        ? fr::ErrorHandling::IgnoreErrors
        : fr::ErrorHandling::AbortOnError;

    ProgressDialog pd(this, _("Importing Data"));
    pd.doShow();

    fr::DataImportResult result;
    bool success = fr::DataTransferHelper::importTable(dbM, tableM, filePath, options, &pd, &result);

    pd.doHide();

    if (pd.isCanceled())
    {
        wxMessageBox(_("Import canceled by user."), _("Import Data"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxString summary;
    if (success && result.errorCount == 0)
    {
        summary = wxString::Format(
            _("Import completed successfully!\n\nRecords processed: %zu\nRecords imported: %zu"),
            result.totalRecords, result.importedRecords
        );
        wxMessageBox(summary, _("Import Data"), wxOK | wxICON_INFORMATION, this);
        EndModal(wxID_OK);
    }
    else
    {
        summary = wxString::Format(
            _("Import completed with warnings or errors.\n\nRecords processed: %zu\nRecords imported: %zu\nErrors: %zu\n\nFirst error:\n%s"),
            result.totalRecords, result.importedRecords, result.errorCount,
            (result.errorMessages.empty() ? _("Unknown error") : result.errorMessages.front()).c_str()
        );
        wxMessageBox(summary, _("Import Data"), wxOK | wxICON_WARNING, this);
        if (result.importedRecords > 0)
            EndModal(wxID_OK);
    }
}
