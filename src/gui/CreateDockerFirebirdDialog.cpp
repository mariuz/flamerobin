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

// For compilers that support precompilation, includes "wx/wxprec.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/gbsizer.h>
#include "gui/CreateDockerFirebirdDialog.h"
#include "gui/StyleGuide.h"

CreateDockerFirebirdDialog::CreateDockerFirebirdDialog(wxWindow* parent, RootPtr root)
    : BaseDialog(parent, wxID_ANY, _("Create Firebird in Docker")), rootM(root)
{
    // Create controls
    wxPanel* panel = getControlsPanel();

    textCtrlContainerNameM = new wxTextCtrl(panel, wxID_ANY, "firebird-server");
    textCtrlHostPortM = new wxTextCtrl(panel, wxID_ANY, "3050");
    textCtrlPasswordM = new wxTextCtrl(panel, wxID_ANY, "masterkey");

    choiceVersionM = new wxChoice(panel, wxID_ANY);
    choiceVersionM->Append("5.0");
    choiceVersionM->Append("4.0");
    choiceVersionM->Append("3.0");
    choiceVersionM->SetSelection(0);

    dirPickerVolumeM = new wxDirPickerCtrl(panel, wxID_ANY, "", 
        _("Select host volume directory (Optional)"));

    checkBoxAutoRegisterM = new wxCheckBox(panel, wxID_ANY, _("Auto-register Server"));
    checkBoxAutoRegisterM->SetValue(true);

    buttonOkM = new wxButton(panel, wxID_SAVE, _("Create"));
    buttonCancelM = new wxButton(panel, wxID_CANCEL, _("Cancel"));

    // Connect events
    Connect(textCtrlContainerNameM->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
        wxCommandEventHandler(CreateDockerFirebirdDialog::OnTextChange));
    Connect(textCtrlHostPortM->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
        wxCommandEventHandler(CreateDockerFirebirdDialog::OnTextChange));
    Connect(textCtrlPasswordM->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
        wxCommandEventHandler(CreateDockerFirebirdDialog::OnTextChange));
    Connect(buttonOkM->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
        wxCommandEventHandler(CreateDockerFirebirdDialog::OnOkButtonClick));

    layoutControls();
    updateButtons();
    updateColors();

    buttonOkM->SetDefault();
}

CreateDockerFirebirdDialog::~CreateDockerFirebirdDialog()
{
}

const wxString CreateDockerFirebirdDialog::getName() const
{
    return "CreateDockerFirebirdDialog";
}

void CreateDockerFirebirdDialog::layoutControls()
{
    wxPanel* panel = getControlsPanel();

    wxStaticText* labelContainerName = new wxStaticText(panel, wxID_ANY, _("Container name:"));
    wxStaticText* labelHostPort = new wxStaticText(panel, wxID_ANY, _("Host port:"));
    wxStaticText* labelPassword = new wxStaticText(panel, wxID_ANY, _("SYSDBA password:"));
    wxStaticText* labelVersion = new wxStaticText(panel, wxID_ANY, _("Firebird version:"));
    wxStaticText* labelVolume = new wxStaticText(panel, wxID_ANY, _("Volume directory:"));

    wxGridBagSizer* sizerControls = new wxGridBagSizer(
        styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin()
    );

    sizerControls->Add(labelContainerName, wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(textCtrlContainerNameM, wxGBPosition(0, 1), wxGBSpan(1, 3), wxALIGN_CENTER_VERTICAL | wxEXPAND);

    sizerControls->Add(labelHostPort, wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(textCtrlHostPortM, wxGBPosition(1, 1), wxGBSpan(1, 3), wxALIGN_CENTER_VERTICAL | wxEXPAND);

    sizerControls->Add(labelPassword, wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(textCtrlPasswordM, wxGBPosition(2, 1), wxGBSpan(1, 3), wxALIGN_CENTER_VERTICAL | wxEXPAND);

    sizerControls->Add(labelVersion, wxGBPosition(3, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(choiceVersionM, wxGBPosition(3, 1), wxGBSpan(1, 3), wxALIGN_CENTER_VERTICAL | wxEXPAND);

    sizerControls->Add(labelVolume, wxGBPosition(4, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(dirPickerVolumeM, wxGBPosition(4, 1), wxGBSpan(1, 3), wxALIGN_CENTER_VERTICAL | wxEXPAND);

    sizerControls->Add(checkBoxAutoRegisterM, wxGBPosition(5, 1), wxGBSpan(1, 3), wxALIGN_CENTER_VERTICAL);

    sizerControls->AddGrowableCol(1);

    wxSizer* sizerButtons = styleguide().createButtonSizer(buttonOkM, buttonCancelM);
    layoutSizers(sizerControls, sizerButtons);
}

void CreateDockerFirebirdDialog::updateButtons()
{
    bool enabled = !textCtrlContainerNameM->GetValue().IsEmpty() &&
                   !textCtrlHostPortM->GetValue().IsEmpty() &&
                   !textCtrlPasswordM->GetValue().IsEmpty();

    if (enabled)
    {
        long portVal;
        if (!textCtrlHostPortM->GetValue().ToLong(&portVal) || portVal <= 0 || portVal > 65535)
        {
            enabled = false;
        }
    }

    buttonOkM->Enable(enabled);
}

void CreateDockerFirebirdDialog::OnTextChange(wxCommandEvent& WXUNUSED(event))
{
    updateButtons();
}

void CreateDockerFirebirdDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    if (runCreateContainer())
    {
        EndModal(wxID_OK);
    }
}

wxString CreateDockerFirebirdDialog::getContainerName() const
{
    return textCtrlContainerNameM->GetValue();
}

wxString CreateDockerFirebirdDialog::getHostPort() const
{
    return textCtrlHostPortM->GetValue();
}

wxString CreateDockerFirebirdDialog::getPassword() const
{
    return textCtrlPasswordM->GetValue();
}

wxString CreateDockerFirebirdDialog::getVersionTag() const
{
    return choiceVersionM->GetStringSelection();
}

wxString CreateDockerFirebirdDialog::getVolumeDir() const
{
    return dirPickerVolumeM->GetPath();
}

bool CreateDockerFirebirdDialog::getAutoRegister() const
{
    return checkBoxAutoRegisterM->GetValue();
}

bool CreateDockerFirebirdDialog::runCreateContainer()
{
    // 1. Verify Docker CLI is in PATH
    wxArrayString versionOutput, versionErrors;
    long versionExitCode = wxExecute("docker --version", versionOutput, versionErrors);
    if (versionExitCode != 0)
    {
        wxMessageBox(_("Docker CLI does not seem to be installed or is not in the system PATH.\n"
                       "Please ensure Docker is installed and running."),
                     _("Docker Command Error"), wxOK | wxICON_ERROR, this);
        return false;
    }

    // 2. Verify Docker daemon is running
    wxArrayString infoOutput, infoErrors;
    long infoExitCode = wxExecute("docker info", infoOutput, infoErrors);
    if (infoExitCode != 0)
    {
        wxMessageBox(_("Docker is installed, but the Docker daemon is not running.\n"
                       "Please start Docker Desktop and try again."),
                     _("Docker Daemon Error"), wxOK | wxICON_ERROR, this);
        return false;
    }

    // 3. Construct the run command
    wxString cmd = wxString::Format("docker run -d --name \"%s\" -p %s:3050 -e FIREBIRD_ROOT_PASSWORD=\"%s\"",
        getContainerName(), getHostPort(), getPassword());

    wxString vol = getVolumeDir();
    if (!vol.IsEmpty())
    {
        cmd += wxString::Format(" -v \"%s:/firebird/data\"", vol);
    }

    cmd += " firebirdsql/firebird:" + getVersionTag();

    // 4. Run the docker run command
    wxBusyCursor busy;
    wxArrayString runOutput, runErrors;
    long runExitCode = wxExecute(cmd, runOutput, runErrors);

    if (runExitCode != 0)
    {
        wxString errMsg = _("Failed to create and start the Firebird Docker container.\n\n"
                            "Docker command executed:\n") + cmd + _("\n\nError output:\n");
        for (size_t i = 0; i < runErrors.GetCount(); ++i)
        {
            errMsg += runErrors[i] + "\n";
        }
        for (size_t i = 0; i < runOutput.GetCount(); ++i)
        {
            errMsg += runOutput[i] + "\n";
        }
        wxMessageBox(errMsg, _("Docker Execution Error"), wxOK | wxICON_ERROR, this);
        return false;
    }

    return true;
}
