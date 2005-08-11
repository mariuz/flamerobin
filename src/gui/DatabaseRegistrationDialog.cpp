/*
The contents of this file are subject to the Initial Developer's Public
License Version 1.0 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License here:
http://www.flamerobin.org/license.html.

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations under
the License.

The Original Code is FlameRobin (TM).

The Initial Developer of the Original Code is Milan Babuskov.

Portions created by the original developer
are Copyright (C) 2004 Milan Babuskov.

All Rights Reserved.

$Id$

Contributor(s): Michael Hieke, Nando Dessena
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "DatabaseRegistrationDialog.h"
#include "styleguide.h"
#include "ugly.h"

#include <wx/filename.h>
#if wxCHECK_VERSION(2, 5, 3)
#include "wx/gbsizer.h"
#endif
//-----------------------------------------------------------------------------
DatabaseRegistrationDialog::DatabaseRegistrationDialog(wxWindow* parent, int id, const wxString& title, bool createDB, const wxPoint& pos, const wxSize& size, long style):
    BaseDialog(parent, id, title, pos, size, style)
{
    createM = createDB;
    label_name = new wxStaticText(getControlsPanel(), -1, _("Display name:"));
    text_ctrl_name = new wxTextCtrl(getControlsPanel(), ID_textcontrol_name, wxT(""));
    label_dbpath = new wxStaticText(getControlsPanel(), -1, _("Database path:"));
    text_ctrl_dbpath = new wxTextCtrl(getControlsPanel(), ID_textcontrol_dbpath, wxT(""));
    button_browse = new wxButton(getControlsPanel(), ID_button_browse, _("..."),
        wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    label_username = new wxStaticText(getControlsPanel(), -1, _("Username:"));
    text_ctrl_username = new wxTextCtrl(getControlsPanel(), ID_textcontrol_username, wxT("SYSDBA"));
    label_password = new wxStaticText(getControlsPanel(), -1, _("Password:"));
    text_ctrl_password = new wxTextCtrl(getControlsPanel(), -1, wxT("masterkey"),
        wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    text_ctrl_password->SetToolTip(_("Leave empty if you wish to be prompted for password every time"));
    label_charset = new wxStaticText(getControlsPanel(), -1, _("Charset:"));
    const wxString charset_choices[] = {
        wxT("NONE"),
        wxT("ASCII"),
        wxT("BIG_5"),
        wxT("CYRL"),
        wxT("DOS437"),
        wxT("DOS850"),
        wxT("DOS852"),
        wxT("DOS857"),
        wxT("DOS860"),
        wxT("DOS861"),
        wxT("DOS863"),
        wxT("DOS865"),
        wxT("EUCJ_0208"),
        wxT("GB_2312"),
        wxT("ISO8859_1"),
        wxT("ISO8859_2"),
        wxT("KSC_5601"),
        wxT("NEXT"),
        wxT("OCTETS"),
        wxT("SJIS_0208"),
        wxT("UNICODE_FSS"),
        wxT("WIN1250"),
        wxT("WIN1251"),
        wxT("WIN1252"),
        wxT("WIN1253"),
        wxT("WIN1254")
    };
    choice_charset = new wxChoice(getControlsPanel(), -1, wxDefaultPosition, wxDefaultSize,
        sizeof(charset_choices) / sizeof(wxString), charset_choices);
    label_role = new wxStaticText(getControlsPanel(), -1, _("Role:"));
    text_ctrl_role = new wxTextCtrl(getControlsPanel(), -1, wxT(""));

    if (createM)
    {
        label_pagesize = new wxStaticText(getControlsPanel(), -1, _("Page size:"));
        const wxString pagesize_choices[] = {
            wxT("1024"),
            wxT("2048"),
            wxT("4096"),
            wxT("8192"),
            wxT("16384")
        };
        choice_pagesize = new wxChoice(getControlsPanel(), -1, wxDefaultPosition, wxDefaultSize,
            sizeof(pagesize_choices) / sizeof(wxString), pagesize_choices);
        label_dialect = new wxStaticText(getControlsPanel(), -1, _("SQL Dialect:"));
        const wxString dialect_choices[] = {
            wxT("1"),
            wxT("2"),
            wxT("3")
        };
        choice_dialect = new wxChoice(getControlsPanel(), -1, wxDefaultPosition, wxDefaultSize,
            sizeof(dialect_choices) / sizeof(wxString), dialect_choices);
    }

    button_ok = new wxButton(getControlsPanel(), ID_button_ok, (createM ? _("Create") : _("Save")));
    button_cancel = new wxButton(getControlsPanel(), ID_button_cancel, _("Cancel"));

    set_properties();
    do_layout();
    updateButtons();
}
//-----------------------------------------------------------------------------
//! implementation details
void DatabaseRegistrationDialog::do_layout()
{
    // create sizer for controls
    wxGridBagSizer* sizerControls = new wxGridBagSizer(styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin());

    sizerControls->Add(label_name, wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(text_ctrl_name, wxGBPosition(0, 1), wxGBSpan(1, 3), wxALIGN_CENTER_VERTICAL|wxEXPAND);

    sizerControls->Add(label_dbpath, wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* sizer_r1c1_3 = new wxBoxSizer(wxHORIZONTAL);
    sizer_r1c1_3->Add(text_ctrl_dbpath, 1, wxALIGN_CENTER_VERTICAL);
    sizer_r1c1_3->Add(button_browse, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, styleguide().getBrowseButtonMargin());
    sizerControls->Add(sizer_r1c1_3, wxGBPosition(1, 1), wxGBSpan(1, 3), wxEXPAND);

    int dx = styleguide().getUnrelatedControlMargin(wxHORIZONTAL) - styleguide().getControlLabelMargin();
    if (dx < 0)
        dx = 0;

    sizerControls->Add(label_username, wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(text_ctrl_username, wxGBPosition(2, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxEXPAND);
    sizerControls->Add(label_password, wxGBPosition(2, 2), wxDefaultSpan, wxLEFT|wxALIGN_CENTER_VERTICAL, dx);
    sizerControls->Add(text_ctrl_password, wxGBPosition(2, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxEXPAND);

    sizerControls->Add(label_charset, wxGBPosition(3, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(choice_charset, wxGBPosition(3, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxEXPAND);
    sizerControls->Add(label_role, wxGBPosition(3, 2), wxDefaultSpan, wxLEFT|wxALIGN_CENTER_VERTICAL, dx);
    sizerControls->Add(text_ctrl_role, wxGBPosition(3, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxEXPAND);

    if (createM)
    {
        sizerControls->Add(label_pagesize, wxGBPosition(4, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
        sizerControls->Add(choice_pagesize, wxGBPosition(4, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxEXPAND);
        sizerControls->Add(label_dialect, wxGBPosition(4, 2), wxDefaultSpan, wxLEFT|wxALIGN_CENTER_VERTICAL, dx);
        sizerControls->Add(choice_dialect, wxGBPosition(4, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxEXPAND);
    }

    sizerControls->AddGrowableCol(1);
    sizerControls->AddGrowableCol(3);

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok, button_cancel);
    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons);
}
//-----------------------------------------------------------------------------
const std::string DatabaseRegistrationDialog::getName() const
{
    if (createM)
        return "CreateDatabaseDialog";
    else
        return "DatabaseRegistrationDialog";
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::set_properties()
{
    int wh = text_ctrl_dbpath->GetMinHeight();
    button_browse->SetSize(wh, wh);
    if (createM)
    {
        choice_pagesize->SetSelection(2);
        choice_dialect->SetSelection(2);
    }
    button_ok->SetDefault();
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::setDatabase(Database *db)
{
    databaseM = db;
    /* this could be reactivated if there is a dialog with "Don't show me again"
    if (databaseM->isConnected())
    ::wxMessageBox(_("Properties of connected database cannot be changed."), _("Warning"), wxOK |wxICON_INFORMATION );
    */
    text_ctrl_name->SetValue(std2wx(databaseM->getName()));
    text_ctrl_dbpath->SetValue(std2wx(databaseM->getPath()));
    text_ctrl_username->SetValue(std2wx(databaseM->getUsername()));
    text_ctrl_password->SetValue(std2wx(databaseM->getPassword()));
    text_ctrl_role->SetValue(std2wx(databaseM->getRole()));
    choice_charset->SetSelection(choice_charset->FindString(std2wx(databaseM->getCharset())));
    if (choice_charset->GetSelection() < 0)
        choice_charset->SetSelection(choice_charset->FindString(wxT("NONE")));

	hasNameM = !(databaseM->getName().empty());
	
    // enable controls depending on operation and database connection status
    // use SetEditable() for edit controls to allow copying text to clipboard
    bool isConnected = databaseM->isConnected();
    text_ctrl_dbpath->SetEditable(!isConnected);
    button_browse->Enable(!isConnected);
    text_ctrl_username->SetEditable(!isConnected);
    text_ctrl_password->SetEditable(!isConnected);
    choice_charset->Enable(!isConnected);
    text_ctrl_role->SetEditable(!isConnected);
    //button_ok->Enable(!isConnected);
    if (isConnected)
    {
        button_cancel->SetLabel(_("Close"));
        button_cancel->SetDefault();
    };
    updateButtons();
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::setServer(Server *s)
{
    serverM = s;
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::updateButtons()
{
    if (button_ok->IsShown())
    {
        button_ok->Enable(!text_ctrl_dbpath->GetValue().IsEmpty()
            && !text_ctrl_username->GetValue().IsEmpty()
            && !text_ctrl_name->GetValue().IsEmpty());
    }
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(DatabaseRegistrationDialog, BaseDialog)
    EVT_BUTTON(DatabaseRegistrationDialog::ID_button_browse, DatabaseRegistrationDialog::OnBrowseButtonClick)
    EVT_BUTTON(DatabaseRegistrationDialog::ID_button_ok, DatabaseRegistrationDialog::OnOkButtonClick)
    EVT_TEXT(DatabaseRegistrationDialog::ID_textcontrol_dbpath, DatabaseRegistrationDialog::OnSettingsChange)
    EVT_TEXT(DatabaseRegistrationDialog::ID_textcontrol_name, DatabaseRegistrationDialog::OnNameChange)
    EVT_TEXT(DatabaseRegistrationDialog::ID_textcontrol_username, DatabaseRegistrationDialog::OnSettingsChange)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::OnBrowseButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxString path = ::wxFileSelector(_("Select database file"), wxT(""), wxT(""), wxT(""),
        _("Firebird database files (*.fdb, *.gdb)|*.fdb;*.gdb|All files (*.*)|*.*"), 0, this);
    if (!path.empty())
        text_ctrl_dbpath->SetValue(path);
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxBusyCursor wait;
    databaseM->setName(wx2std(text_ctrl_name->GetValue()));
    databaseM->setPath(wx2std(text_ctrl_dbpath->GetValue()));
    databaseM->setUsername(wx2std(text_ctrl_username->GetValue()));
    databaseM->setPassword(wx2std(text_ctrl_password->GetValue()));
    databaseM->setCharset(wx2std(choice_charset->GetStringSelection()));
    databaseM->setRole(wx2std(text_ctrl_role->GetValue()));

    try
    {
        if (createM)	// create new database
        {
            long ps = 0;
            choice_pagesize->GetStringSelection().ToLong(&ps);

            int dialect = 3;
            if (choice_dialect->GetSelection() == 0)
                dialect = 1;

            serverM->createDatabase(databaseM, (ps) ? ps : 4096, dialect);
        }
        EndModal(wxID_OK);
    }
    catch (IBPP::Exception &e)
    {
        wxMessageBox(std2wx(e.ErrorMessage()), _("Error"), wxOK|wxICON_ERROR);
    }
    catch (...)
    {
        wxMessageBox(_("SYSTEM ERROR!\n"), _("Error"), wxOK|wxICON_ERROR);
    }
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::OnSettingsChange(wxCommandEvent& WXUNUSED(event))
{
    if (IsShown())
	{
		if (!hasNameM)
		{
			wxFileName fn(text_ctrl_dbpath->GetValue());
			text_ctrl_name->SetValue(fn.GetName());
		}
        updateButtons();
	}
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::OnNameChange(wxCommandEvent& WXUNUSED(event))
{
    if (IsShown())
	{
        updateButtons();
		wxFileName fn(text_ctrl_dbpath->GetValue());
		if (text_ctrl_name->GetValue() != fn.GetName())
			hasNameM = true;
	}
}
//-----------------------------------------------------------------------------
