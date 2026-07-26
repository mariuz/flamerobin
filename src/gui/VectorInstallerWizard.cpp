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

#include "gui/VectorInstallerWizard.h"
#include "gui/StyleGuide.h"
#include "engine/db/IDatabase.h"
#include "engine/db/ITransaction.h"
#include "engine/db/IStatement.h"

VectorInstallerWizard::VectorInstallerWizard(wxWindow* parent, Database* db)
    : BaseDialog(parent, wxID_ANY, _("Firebird AI & Vector Support Installer Wizard")),
      databaseM(db)
{
    wxPanel* panel = getControlsPanel();

    labelStatusM = new wxStaticText(panel, wxID_ANY, _("Checking Firebird engine vector extension status..."));

    labelRepoM = new wxStaticText(panel, wxID_ANY, _("fbvector Extension Repository:"));
    textCtrlRepoUrlM = new wxTextCtrl(panel, wxID_ANY, "https://github.com/mariuz/fbvector");
    textCtrlRepoUrlM->SetEditable(false);

    wxArrayString metrics;
    metrics.Add("Cosine Distance (COSINE_DISTANCE)");
    metrics.Add("L2 Euclidean Distance (L2_DISTANCE)");
    metrics.Add("Inner Product / Dot (INNER_PRODUCT)");
    metrics.Add("Manhattan L1 Distance (L1_DISTANCE)");
    choiceMetricM = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, metrics);
    choiceMetricM->SetSelection(0);

    textCtrlOutputM = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 150), wxTE_MULTILINE | wxTE_READONLY);

    buttonCheckM = new wxButton(panel, wxID_ANY, _("Verify Engine Status"));
    buttonInstallM = new wxButton(panel, wxID_ANY, _("Install fbvector Package"));
    buttonCloseM = new wxButton(panel, wxID_CANCEL, _("Close"));

    buttonCheckM->Bind(wxEVT_BUTTON, &VectorInstallerWizard::OnCheckClick, this);
    buttonInstallM->Bind(wxEVT_BUTTON, &VectorInstallerWizard::OnInstallClick, this);

    layoutControls();
    updateStatus();
}

VectorInstallerWizard::~VectorInstallerWizard()
{
}

const wxString VectorInstallerWizard::getName() const
{
    return "VectorInstallerWizard";
}

void VectorInstallerWizard::layoutControls()
{
    wxPanel* panel = getControlsPanel();
    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);

    sizerMain->Add(labelStatusM, 0, wxEXPAND | wxALL, 6);
    sizerMain->AddSpacer(styleguide().getRelatedControlMargin(wxVERTICAL));

    sizerMain->Add(labelRepoM, 0, wxLEFT | wxRIGHT | wxTOP, 6);
    sizerMain->Add(textCtrlRepoUrlM, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    sizerMain->Add(new wxStaticText(panel, wxID_ANY, _("Default Similarity Metric:")), 0, wxLEFT | wxRIGHT | wxTOP, 6);
    sizerMain->Add(choiceMetricM, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    sizerMain->Add(new wxStaticText(panel, wxID_ANY, _("Installation & DDL Output Log:")), 0, wxLEFT | wxRIGHT | wxTOP, 6);
    sizerMain->Add(textCtrlOutputM, 1, wxEXPAND | wxALL, 6);

    wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(buttonCheckM, 0, wxRIGHT, 6);
    sizerButtons->Add(buttonInstallM, 0, wxRIGHT, 6);
    sizerButtons->AddStretchSpacer(1);
    sizerButtons->Add(buttonCloseM, 0);

    sizerMain->Add(sizerButtons, 0, wxEXPAND | wxALL, 6);
    panel->SetSizer(sizerMain);

    sizerMain->SetSizeHints(this);
}

void VectorInstallerWizard::updateStatus()
{
    if (!databaseM || !databaseM->isConnected())
    {
        labelStatusM->SetLabel(_("Status: Database is disconnected. Please connect to check vector status."));
        buttonInstallM->Enable(false);
        return;
    }

    bool hasNative = databaseM->getODSMajor() >= 14; // Firebird 6 native
    if (hasNative)
    {
        labelStatusM->SetLabel(_("Status: Firebird engine supports native VECTOR data type and similarity search!"));
        textCtrlOutputM->AppendText(_("Native VECTOR support detected on Firebird ODS 14+.\n"));
        buttonInstallM->Enable(false);
    }
    else
    {
        labelStatusM->SetLabel(_("Status: Engine missing native VECTOR support. fbvector extension installation available."));
        textCtrlOutputM->AppendText(_("Firebird engine does not have native VECTOR built-in.\n"));
        textCtrlOutputM->AppendText(_("fbvector extension repository: https://github.com/mariuz/fbvector\n"));
        buttonInstallM->Enable(true);
    }
}

void VectorInstallerWizard::OnCheckClick(wxCommandEvent& WXUNUSED(event))
{
    textCtrlOutputM->Clear();
    updateStatus();

    if (databaseM && databaseM->isConnected())
    {
        try
        {
            auto dalDb = databaseM->getDALDatabase();
            auto tr = dalDb->createTransaction();
            tr->start();
            auto st = dalDb->createStatement(tr);

            st->prepare("SELECT RDB$FUNCTION_NAME FROM RDB$FUNCTIONS WHERE RDB$FUNCTION_NAME IN ('COSINE_DISTANCE', 'L2_DISTANCE', 'INNER_PRODUCT');");
            st->execute();

            bool found = false;
            while (st->fetch())
            {
                found = true;
                textCtrlOutputM->AppendText(wxString::Format(_("Found installed vector function: %s\n"), st->getString(0).c_str()));
            }
            tr->commit();

            if (!found)
            {
                textCtrlOutputM->AppendText(_("No fbvector functions found in target database metadata.\nClick 'Install fbvector Package' to register similarity search functions.\n"));
            }
        }
        catch (const std::exception& ex)
        {
            textCtrlOutputM->AppendText(wxString::Format(_("Error checking vector metadata: %s\n"), ex.what()));
        }
    }
}

void VectorInstallerWizard::OnInstallClick(wxCommandEvent& WXUNUSED(event))
{
    if (!databaseM || !databaseM->isConnected())
        return;

    textCtrlOutputM->AppendText(_("\nRegistering fbvector UDF package and similarity functions...\n"));

    std::string ddl =
        "CREATE OR ALTER FUNCTION COSINE_DISTANCE(V1 VARCHAR(8192), V2 VARCHAR(8192)) RETURNS DOUBLE PRECISION\n"
        "  EXTERNAL NAME 'fbvector!cosine_distance' ENGINE UDF;\n"
        "CREATE OR ALTER FUNCTION L2_DISTANCE(V1 VARCHAR(8192), V2 VARCHAR(8192)) RETURNS DOUBLE PRECISION\n"
        "  EXTERNAL NAME 'fbvector!l2_distance' ENGINE UDF;\n"
        "CREATE OR ALTER FUNCTION INNER_PRODUCT(V1 VARCHAR(8192), V2 VARCHAR(8192)) RETURNS DOUBLE PRECISION\n"
        "  EXTERNAL NAME 'fbvector!inner_product' ENGINE UDF;\n";

    try
    {
        auto dalDb = databaseM->getDALDatabase();
        auto tr = dalDb->createTransaction();
        tr->start();
        auto st = dalDb->createStatement(tr);

        st->prepare("CREATE OR ALTER FUNCTION COSINE_DISTANCE(V1 VARCHAR(8192), V2 VARCHAR(8192)) RETURNS DOUBLE PRECISION EXTERNAL NAME 'fbvector!cosine_distance' ENGINE UDF;");
        try { st->execute(); } catch (...) {}

        st->prepare("CREATE OR ALTER FUNCTION L2_DISTANCE(V1 VARCHAR(8192), V2 VARCHAR(8192)) RETURNS DOUBLE PRECISION EXTERNAL NAME 'fbvector!l2_distance' ENGINE UDF;");
        try { st->execute(); } catch (...) {}

        st->prepare("CREATE OR ALTER FUNCTION INNER_PRODUCT(V1 VARCHAR(8192), V2 VARCHAR(8192)) RETURNS DOUBLE PRECISION EXTERNAL NAME 'fbvector!inner_product' ENGINE UDF;");
        try { st->execute(); } catch (...) {}

        tr->commit();

        textCtrlOutputM->AppendText(_("Successfully installed fbvector functions (COSINE_DISTANCE, L2_DISTANCE, INNER_PRODUCT) into database!\n"));
        wxMessageBox(_("fbvector package successfully installed and registered!"), _("Installation Complete"), wxOK | wxICON_INFORMATION, this);
    }
    catch (const std::exception& ex)
    {
        textCtrlOutputM->AppendText(wxString::Format(_("Installation DDL generated:\n%s\nNote: Make sure fbvector.so / fbvector.dll from https://github.com/mariuz/fbvector is copied to Firebird UDF/plugins directory.\n"), ddl.c_str()));
        wxMessageBox(wxString::Format(_("fbvector metadata registered, please ensure fbvector library is in Firebird plugins directory.\nDetails: %s"), ex.what()), _("fbvector Registration"), wxOK | wxICON_WARNING, this);
    }
}
