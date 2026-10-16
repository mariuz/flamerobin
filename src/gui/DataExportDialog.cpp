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

#include "gui/DataExportDialog.h"
#include "gui/ProgressDialog.h"
#include "gui/StyleGuide.h"
#include "gui/controls/DataGrid.h"
#include "gui/controls/DataGridTable.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/relation.h"

#include <wx/checklst.h>
#include <wx/filedlg.h>
#include <wx/filename.h>

BEGIN_EVENT_TABLE(DataExportDialog, BaseDialog)
    EVT_RADIOBOX(DataExportDialog::ID_radio_format, DataExportDialog::OnFormatChanged)
    EVT_BUTTON(DataExportDialog::ID_button_browse, DataExportDialog::OnBrowse)
    EVT_BUTTON(DataExportDialog::ID_button_select_all, DataExportDialog::OnSelectAll)
    EVT_BUTTON(DataExportDialog::ID_button_deselect_all, DataExportDialog::OnDeselectAll)
    EVT_BUTTON(DataExportDialog::ID_button_export, DataExportDialog::OnExport)
END_EVENT_TABLE()

DataExportDialog::DataExportDialog(wxWindow* parent, Database* db, Relation* relation)
    : BaseDialog(parent, -1, _("Export Table Data")),
      dbM(db), relationM(relation), gridM(nullptr)
{
    createControls();
    layoutControls();
    setControlProperties();
    updateFormatUI();
}

DataExportDialog::DataExportDialog(wxWindow* parent, DataGrid* grid)
    : BaseDialog(parent, -1, _("Export Query Result Data")),
      dbM(nullptr), relationM(nullptr), gridM(grid)
{
    createControls();
    layoutControls();
    setControlProperties();
    updateFormatUI();
}

const wxString DataExportDialog::getName() const
{
    return "DataExportDialog";
}

void DataExportDialog::createControls()
{
    wxArrayString formats;
    formats.Add("JSON");
    formats.Add("XML");
    radio_format = new wxRadioBox(getControlsPanel(), ID_radio_format, _("Format"),
        wxDefaultPosition, wxDefaultSize, formats, 1, wxRA_SPECIFY_ROWS);

    check_list_columns = new wxCheckListBox(getControlsPanel(), ID_check_list_columns,
        wxDefaultPosition, wxSize(-1, 160));

    std::vector<wxString> colNames;
    std::vector<bool> checked;

    if (relationM)
    {
        relationM->ensureChildrenLoaded();
        for (auto it = relationM->begin(); it != relationM->end(); ++it)
        {
            colNames.push_back((*it)->getName_());
            checked.push_back(true);
        }
    }
    else if (gridM)
    {
        std::vector<bool> selCols = gridM->getColumnsWithSelectedCells();
        bool hasSelection = (std::find(selCols.begin(), selCols.end(), true) != selCols.end());

        for (int c = 0; c < gridM->GetNumberCols(); ++c)
        {
            colNames.push_back(gridM->GetColLabelValue(c));
            checked.push_back(hasSelection ? selCols[c] : true);
        }
    }

    for (size_t i = 0; i < colNames.size(); ++i)
    {
        check_list_columns->Append(colNames[i]);
        if (i < checked.size() && checked[i])
            check_list_columns->Check((unsigned int)i, true);
    }

    button_select_all = new wxButton(getControlsPanel(), ID_button_select_all, _("Select All"));
    button_deselect_all = new wxButton(getControlsPanel(), ID_button_deselect_all, _("Deselect All"));

    wxString defaultFile;
    if (relationM)
        defaultFile = relationM->getName_() + ".json";
    else
        defaultFile = "query_result.json";

    textctrl_file = new wxTextCtrl(getControlsPanel(), ID_textctrl_file, defaultFile);
    button_browse = new wxButton(getControlsPanel(), ID_button_browse, _("Browse..."));

    wxArrayString encodings;
    encodings.Add("UTF-8");
    encodings.Add("ISO-8859-1");
    encodings.Add("ASCII");
    choice_encoding = new wxChoice(getControlsPanel(), ID_choice_encoding, wxDefaultPosition, wxDefaultSize, encodings);
    choice_encoding->SetSelection(0);

    checkbox_pretty = new wxCheckBox(getControlsPanel(), ID_checkbox_pretty, _("Pretty-print formatted output"));
    checkbox_pretty->SetValue(true);

    checkbox_base64 = new wxCheckBox(getControlsPanel(), ID_checkbox_base64, _("Encode binary BLOBs as Base64"));
    checkbox_base64->SetValue(true);

    wxArrayString nullOptions;
    nullOptions.Add(_("Output null (JSON null / XML xsi:nil=\"true\")"));
    nullOptions.Add(_("Omit element / property"));
    nullOptions.Add(_("Empty string (\"\")"));
    choice_null_handling = new wxChoice(getControlsPanel(), ID_choice_null_handling, wxDefaultPosition, wxDefaultSize, nullOptions);
    choice_null_handling->SetSelection(0);

    wxString defRoot = relationM ? fr::DataTransferHelper::sanitizeXmlTagName(relationM->getName_()) : "query_results";
    label_root_tag = new wxStaticText(getControlsPanel(), wxID_ANY, _("XML Root tag:"));
    textctrl_root_tag = new wxTextCtrl(getControlsPanel(), ID_textctrl_root_tag, defRoot);

    label_row_tag = new wxStaticText(getControlsPanel(), wxID_ANY, _("XML Row tag:"));
    textctrl_row_tag = new wxTextCtrl(getControlsPanel(), ID_textctrl_row_tag, "row");

    button_export = new wxButton(getControlsPanel(), ID_button_export, _("Export"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
}

void DataExportDialog::layoutControls()
{
    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);

    // Format selection
    sizerMain->Add(radio_format, 0, wxEXPAND | wxBOTTOM, styleguide().getRelatedControlMargin(wxVERTICAL));

    // Target file
    wxStaticBoxSizer* sizerFile = new wxStaticBoxSizer(wxVERTICAL, getControlsPanel(), _("Target File"));
    wxBoxSizer* sizerFileRow = new wxBoxSizer(wxHORIZONTAL);
    sizerFileRow->Add(textctrl_file, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerFileRow->Add(button_browse, 0, wxALIGN_CENTER_VERTICAL);
    sizerFile->Add(sizerFileRow, 0, wxEXPAND | wxALL, styleguide().getControlLabelMargin());
    sizerMain->Add(sizerFile, 0, wxEXPAND | wxBOTTOM, styleguide().getRelatedControlMargin(wxVERTICAL));

    // Columns
    wxStaticBoxSizer* sizerCols = new wxStaticBoxSizer(wxVERTICAL, getControlsPanel(), _("Columns to Export"));
    sizerCols->Add(check_list_columns, 1, wxEXPAND | wxBOTTOM, styleguide().getRelatedControlMargin(wxVERTICAL));

    wxBoxSizer* sizerColBtns = new wxBoxSizer(wxHORIZONTAL);
    sizerColBtns->Add(button_select_all, 0, wxRIGHT, styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerColBtns->Add(button_deselect_all, 0);
    sizerCols->Add(sizerColBtns, 0, wxALIGN_RIGHT);
    sizerMain->Add(sizerCols, 1, wxEXPAND | wxBOTTOM, styleguide().getRelatedControlMargin(wxVERTICAL));

    // Options
    wxStaticBoxSizer* sizerOpts = new wxStaticBoxSizer(wxVERTICAL, getControlsPanel(), _("Options"));
    sizerOpts->Add(checkbox_pretty, 0, wxBOTTOM, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerOpts->Add(checkbox_base64, 0, wxBOTTOM, styleguide().getRelatedControlMargin(wxVERTICAL));

    wxFlexGridSizer* sizerGridOpts = new wxFlexGridSizer(4, 2, styleguide().getRelatedControlMargin(wxVERTICAL), styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerGridOpts->AddGrowableCol(1, 1);

    sizerGridOpts->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("NULL handling:")), 0, wxALIGN_CENTER_VERTICAL);
    sizerGridOpts->Add(choice_null_handling, 1, wxEXPAND);

    sizerGridOpts->Add(new wxStaticText(getControlsPanel(), wxID_ANY, _("Encoding:")), 0, wxALIGN_CENTER_VERTICAL);
    sizerGridOpts->Add(choice_encoding, 1, wxEXPAND);

    sizerGridOpts->Add(label_root_tag, 0, wxALIGN_CENTER_VERTICAL);
    sizerGridOpts->Add(textctrl_root_tag, 1, wxEXPAND);

    sizerGridOpts->Add(label_row_tag, 0, wxALIGN_CENTER_VERTICAL);
    sizerGridOpts->Add(textctrl_row_tag, 1, wxEXPAND);

    sizerOpts->Add(sizerGridOpts, 0, wxEXPAND | wxTOP, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerMain->Add(sizerOpts, 0, wxEXPAND | wxBOTTOM, styleguide().getRelatedControlMargin(wxVERTICAL));

    // Dialog buttons
    wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(button_export, 0, wxRIGHT, styleguide().getRelatedControlMargin(wxHORIZONTAL));
    sizerButtons->Add(button_cancel, 0);

    layoutSizers(sizerMain, sizerButtons, true);
}

void DataExportDialog::setControlProperties()
{
    button_export->SetDefault();
}

void DataExportDialog::updateFormatUI()
{
    bool isXml = (radio_format->GetSelection() == 1);
    label_root_tag->Enable(isXml);
    textctrl_root_tag->Enable(isXml);
    label_row_tag->Enable(isXml);
    textctrl_row_tag->Enable(isXml);

    wxString currentPath = textctrl_file->GetValue();
    if (!currentPath.empty())
    {
        wxFileName fn(currentPath);
        wxString ext = fn.GetExt().Lower();
        if (isXml && ext == "json")
        {
            fn.SetExt("xml");
            textctrl_file->SetValue(fn.GetFullPath());
        }
        else if (!isXml && ext == "xml")
        {
            fn.SetExt("json");
            textctrl_file->SetValue(fn.GetFullPath());
        }
    }
}

void DataExportDialog::OnFormatChanged(wxCommandEvent& WXUNUSED(event))
{
    updateFormatUI();
}

void DataExportDialog::OnBrowse(wxCommandEvent& WXUNUSED(event))
{
    bool isXml = (radio_format->GetSelection() == 1);
    wxString wildcard = isXml
        ? _("XML files (*.xml)|*.xml|All files (*.*)|*.*")
        : _("JSON files (*.json)|*.json|All files (*.*)|*.*");

    wxString defaultExt = isXml ? "*.xml" : "*.json";

    wxString fname = ::wxFileSelector(
        _("Select Export File"),
        wxEmptyString,
        textctrl_file->GetValue(),
        defaultExt,
        wildcard,
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
        this
    );

    if (!fname.empty())
    {
        textctrl_file->SetValue(fname);
    }
}

void DataExportDialog::OnSelectAll(wxCommandEvent& WXUNUSED(event))
{
    for (unsigned int i = 0; i < check_list_columns->GetCount(); ++i)
        check_list_columns->Check(i, true);
}

void DataExportDialog::OnDeselectAll(wxCommandEvent& WXUNUSED(event))
{
    for (unsigned int i = 0; i < check_list_columns->GetCount(); ++i)
        check_list_columns->Check(i, false);
}

void DataExportDialog::OnExport(wxCommandEvent& WXUNUSED(event))
{
    wxString filePath = textctrl_file->GetValue().Trim();
    if (filePath.empty())
    {
        wxMessageBox(_("Please specify an export file path."), _("Error"), wxOK | wxICON_ERROR, this);
        return;
    }

    std::vector<wxString> selCols;
    for (unsigned int i = 0; i < check_list_columns->GetCount(); ++i)
    {
        if (check_list_columns->IsChecked(i))
            selCols.push_back(check_list_columns->GetString(i));
    }

    if (selCols.empty())
    {
        wxMessageBox(_("Please select at least one column to export."), _("Error"), wxOK | wxICON_ERROR, this);
        return;
    }

    fr::DataExportOptions options;
    options.format = (radio_format->GetSelection() == 1)
        ? fr::DataTransferFormat::Xml
        : fr::DataTransferFormat::Json;

    options.columns = selCols;
    options.prettyPrint = checkbox_pretty->GetValue();
    options.base64Blobs = checkbox_base64->GetValue();

    int nullSel = choice_null_handling->GetSelection();
    if (nullSel == 1)
        options.nullHandling = fr::NullHandling::Omit;
    else if (nullSel == 2)
        options.nullHandling = fr::NullHandling::EmptyString;
    else
        options.nullHandling = fr::NullHandling::NullValue;

    options.xmlRootTag = textctrl_root_tag->GetValue().Trim();
    options.xmlRowTag = textctrl_row_tag->GetValue().Trim();
    options.encoding = choice_encoding->GetStringSelection();

    ProgressDialog pd(this, _("Exporting Data"));
    pd.doShow();

    wxString errorMsg;
    bool success = false;

    if (relationM)
    {
        success = fr::DataTransferHelper::exportRelation(dbM, relationM, filePath, options, &pd, &errorMsg);
    }
    else if (gridM)
    {
        success = fr::DataTransferHelper::exportDataGrid(gridM, filePath, options, &pd, &errorMsg);
    }

    pd.doHide();

    if (pd.isCanceled())
    {
        wxMessageBox(_("Export canceled by user."), _("Export Data"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    if (success)
    {
        wxMessageBox(_("Data exported successfully!"), _("Export Data"), wxOK | wxICON_INFORMATION, this);
        EndModal(wxID_OK);
    }
    else
    {
        wxMessageBox(wxString::Format(_("Export failed: %s"), errorMsg.c_str()), _("Export Error"), wxOK | wxICON_ERROR, this);
    }
}
