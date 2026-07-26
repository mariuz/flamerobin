/*
  Copyright (c) 2004-2026 FlameRobin Development Team

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
#include "gui/VectorDataEditorDialog.h"
#include "gui/StyleGuide.h"
#include <cmath>

VectorDataEditorDialog::VectorDataEditorDialog(wxWindow* parent, const wxString& columnName, const wxString& initialVector)
    : BaseDialog(parent, wxID_ANY, _("Vector & AI Embedding Data Viewer")),
      columnNameM(columnName)
{
    createControls();
    layoutControls();

    if (!initialVector.IsEmpty())
    {
        textCtrlVectorTextM->SetValue(initialVector);
    }
    else
    {
        textCtrlVectorTextM->SetValue("[0.1500, 0.4200, -0.8700, 0.3100]");
    }
    updateVectorStats();
}

VectorDataEditorDialog::~VectorDataEditorDialog()
{
}

const wxString VectorDataEditorDialog::getName() const
{
    return "VectorDataEditorDialog";
}

void VectorDataEditorDialog::createControls()
{
    wxPanel* panel = getControlsPanel();

    textCtrlVectorTextM = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
    labelDimensionsM = new wxStaticText(panel, wxID_ANY, _("Dimensions: 0"));
    labelNormM = new wxStaticText(panel, wxID_ANY, _("L2 Norm: 0.0000"));

    gridValuesM = new wxGrid(panel, wxID_ANY);
    gridValuesM->CreateGrid(0, 2);
    gridValuesM->SetColLabelValue(0, _("Index"));
    gridValuesM->SetColLabelValue(1, _("Value"));

    buttonFormatM = new wxButton(panel, wxID_ANY, _("Format Vector"));
    buttonNormalizeM = new wxButton(panel, wxID_ANY, _("Normalize (L2)"));

    textCtrlQueryVectorM = new wxTextCtrl(panel, wxID_ANY, "[0.1000, 0.4000, -0.8000, 0.3000]");
    
    wxArrayString metrics;
    metrics.Add("Cosine Distance (COSINE_DISTANCE)");
    metrics.Add("L2 Euclidean Distance (L2_DISTANCE)");
    metrics.Add("Inner Product (INNER_PRODUCT)");
    metrics.Add("Manhattan Distance (L1_DISTANCE)");
    choiceMetricM = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, metrics);
    choiceMetricM->SetSelection(0);

    labelDistanceM = new wxStaticText(panel, wxID_ANY, _("Similarity Distance: N/A"));
    textCtrlSimilaritySqlM = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

    buttonGenerateQueryM = new wxButton(panel, wxID_ANY, _("Generate Similarity Query"));
    buttonOkM = new wxButton(panel, wxID_OK, _("Save"));
    buttonCancelM = new wxButton(panel, wxID_CANCEL, _("Cancel"));

    textCtrlVectorTextM->Bind(wxEVT_TEXT, &VectorDataEditorDialog::OnTextChange, this);
    textCtrlQueryVectorM->Bind(wxEVT_TEXT, &VectorDataEditorDialog::OnTextChange, this);
    choiceMetricM->Bind(wxEVT_CHOICE, &VectorDataEditorDialog::OnTextChange, this);

    buttonFormatM->Bind(wxEVT_BUTTON, &VectorDataEditorDialog::OnFormatClick, this);
    buttonNormalizeM->Bind(wxEVT_BUTTON, &VectorDataEditorDialog::OnNormalizeClick, this);
    buttonGenerateQueryM->Bind(wxEVT_BUTTON, &VectorDataEditorDialog::OnGenerateQueryClick, this);
}

void VectorDataEditorDialog::layoutControls()
{
    wxPanel* panel = getControlsPanel();
    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);

    sizerMain->Add(new wxStaticText(panel, wxID_ANY, wxString::Format(_("Vector Field Editor [%s]:"), columnNameM)), 0, wxALL, 4);
    sizerMain->Add(textCtrlVectorTextM, 1, wxEXPAND | wxALL, 4);

    wxBoxSizer* sizerInfo = new wxBoxSizer(wxHORIZONTAL);
    sizerInfo->Add(labelDimensionsM, 0, wxRIGHT, 12);
    sizerInfo->Add(labelNormM, 0, wxRIGHT, 12);
    sizerInfo->Add(buttonFormatM, 0, wxRIGHT, 6);
    sizerInfo->Add(buttonNormalizeM, 0);
    sizerMain->Add(sizerInfo, 0, wxEXPAND | wxALL, 4);

    sizerMain->Add(new wxStaticText(panel, wxID_ANY, _("Vector Element Inspector:")), 0, wxLEFT | wxRIGHT | wxTOP, 4);
    sizerMain->Add(gridValuesM, 1, wxEXPAND | wxALL, 4);

    wxStaticBoxSizer* sizerQuery = new wxStaticBoxSizer(wxVERTICAL, panel, _("Vector Similarity Query Helper"));
    sizerQuery->Add(new wxStaticText(panel, wxID_ANY, _("Target Query Vector:")), 0, wxALL, 2);
    sizerQuery->Add(textCtrlQueryVectorM, 0, wxEXPAND | wxALL, 2);

    sizerQuery->Add(new wxStaticText(panel, wxID_ANY, _("Metric Function:")), 0, wxALL, 2);
    sizerQuery->Add(choiceMetricM, 0, wxEXPAND | wxALL, 2);

    sizerQuery->Add(labelDistanceM, 0, wxALL, 2);
    sizerQuery->Add(textCtrlSimilaritySqlM, 1, wxEXPAND | wxALL, 2);
    sizerQuery->Add(buttonGenerateQueryM, 0, wxALIGN_RIGHT | wxALL, 2);

    sizerMain->Add(sizerQuery, 1, wxEXPAND | wxALL, 4);

    wxBoxSizer* sizerBtn = new wxBoxSizer(wxHORIZONTAL);
    sizerBtn->AddStretchSpacer(1);
    sizerBtn->Add(buttonOkM, 0, wxRIGHT, 6);
    sizerBtn->Add(buttonCancelM, 0);
    sizerMain->Add(sizerBtn, 0, wxEXPAND | wxALL, 4);

    panel->SetSizer(sizerMain);
    sizerMain->SetSizeHints(this);
}

void VectorDataEditorDialog::updateVectorStats()
{
    std::string text = std::string(textCtrlVectorTextM->GetValue().mb_str());
    fr::VectorHelper::parseVectorString(text, vectorValuesM);

    labelDimensionsM->SetLabel(wxString::Format(_("Dimensions: %d"), (int)vectorValuesM.size()));

    float l2 = 0.0f;
    for (float f : vectorValuesM) l2 += f * f;
    l2 = std::sqrt(l2);

    labelNormM->SetLabel(wxString::Format(_("L2 Norm: %.4f"), l2));

    if (gridValuesM->GetNumberRows() > 0)
        gridValuesM->DeleteRows(0, gridValuesM->GetNumberRows());

    if (!vectorValuesM.empty())
    {
        gridValuesM->AppendRows((int)vectorValuesM.size());
        for (size_t i = 0; i < vectorValuesM.size(); ++i)
        {
            gridValuesM->SetCellValue((int)i, 0, wxString::Format("%d", (int)i));
            gridValuesM->SetCellValue((int)i, 1, wxString::Format("%.6f", vectorValuesM[i]));
        }
    }

    updateQueryPreview();
}

void VectorDataEditorDialog::updateQueryPreview()
{
    std::vector<float> qVec;
    std::string qText = std::string(textCtrlQueryVectorM->GetValue().mb_str());
    fr::VectorHelper::parseVectorString(qText, qVec);

    if (!vectorValuesM.empty() && !qVec.empty() && vectorValuesM.size() == qVec.size())
    {
        int sel = choiceMetricM->GetSelection();
        float dist = 0.0f;
        if (sel == 0)
            dist = fr::VectorHelper::calculateCosineDistance(vectorValuesM, qVec);
        else if (sel == 1)
            dist = fr::VectorHelper::calculateL2Distance(vectorValuesM, qVec);
        else
            dist = fr::VectorHelper::calculateInnerProduct(vectorValuesM, qVec);

        labelDistanceM->SetLabel(wxString::Format(_("Similarity Distance / Score: %.6f"), dist));
    }
    else
    {
        labelDistanceM->SetLabel(_("Similarity Distance: Dim Mismatch"));
    }

    fr::VectorMetric metric = fr::VectorMetric::CosineDistance;
    int sel = choiceMetricM->GetSelection();
    if (sel == 1) metric = fr::VectorMetric::L2Distance;
    else if (sel == 2) metric = fr::VectorMetric::InnerProduct;
    else if (sel == 3) metric = fr::VectorMetric::ManhattanDistance;

    std::string sql = fr::VectorHelper::generateSimilarityQuery("DOCUMENTS", std::string(columnNameM.mb_str()), qVec.empty() ? vectorValuesM : qVec, metric, 10);
    textCtrlSimilaritySqlM->SetValue(wxString::FromUTF8(sql.c_str()));
}

void VectorDataEditorDialog::OnTextChange(wxCommandEvent& WXUNUSED(event))
{
    updateVectorStats();
}

void VectorDataEditorDialog::OnFormatClick(wxCommandEvent& WXUNUSED(event))
{
    if (!vectorValuesM.empty())
    {
        std::string formatted = fr::VectorHelper::formatVectorString(vectorValuesM, 4);
        textCtrlVectorTextM->SetValue(wxString::FromUTF8(formatted.c_str()));
    }
}

void VectorDataEditorDialog::OnNormalizeClick(wxCommandEvent& WXUNUSED(event))
{
    if (vectorValuesM.empty()) return;

    float l2 = 0.0f;
    for (float f : vectorValuesM) l2 += f * f;
    l2 = std::sqrt(l2);

    if (l2 > 0.0f)
    {
        for (float& f : vectorValuesM) f /= l2;
        std::string formatted = fr::VectorHelper::formatVectorString(vectorValuesM, 6);
        textCtrlVectorTextM->SetValue(wxString::FromUTF8(formatted.c_str()));
    }
}

void VectorDataEditorDialog::OnGenerateQueryClick(wxCommandEvent& WXUNUSED(event))
{
    wxString sql = textCtrlSimilaritySqlM->GetValue();
    if (!sql.IsEmpty() && wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(sql));
        wxTheClipboard->Close();
        wxMessageBox(_("Similarity search SQL query copied to clipboard!"), _("Query Copied"), wxOK | wxICON_INFORMATION, this);
    }
}

wxString VectorDataEditorDialog::getVectorText() const
{
    return textCtrlVectorTextM->GetValue();
}
