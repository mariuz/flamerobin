/*
  Copyright (c) 2004-2011 The FlameRobin Development Team

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


  $Id$

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

#include <wx/filename.h>
#include <wx/gbsizer.h>

#include "core/StringUtils.h"
#include "gui/controls/DndTextControls.h"
#include "gui/DatabaseRegistrationDialog.h"
#include "gui/StyleGuide.h"
#include "metadata/database.h"
#include "metadata/server.h"
//-----------------------------------------------------------------------------
DatabaseRegistrationDialog::DatabaseRegistrationDialog(wxWindow* parent,
        const wxString& title, bool createDB, bool connectAs)
    : BaseDialog(parent, wxID_ANY, title)
{
    createM = createDB;
    connectAsM = connectAs;
    isDefaultNameM = true;

    createControls();
    setControlsProperties();
    layoutControls();
    updateButtons();
}
//-----------------------------------------------------------------------------
//! implementation details
const wxString DatabaseRegistrationDialog::buildName(const wxString& dbPath) const
{
    Database helper;
    helper.setPath(dbPath);
    return helper.extractNameFromConnectionString();
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::createControls()
{
    label_name = new wxStaticText(getControlsPanel(), -1, _("Display name:"));
    text_ctrl_name = new wxTextCtrl(getControlsPanel(),
        ID_textcontrol_name, wxEmptyString);
    label_dbpath = new wxStaticText(getControlsPanel(), -1,
        _("Database path:"));
    text_ctrl_dbpath = new FileTextControl(getControlsPanel(),
        ID_textcontrol_dbpath, wxEmptyString);
    button_browse = new wxButton(getControlsPanel(), ID_button_browse,
        wxT("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

    label_authentication = new wxStaticText(getControlsPanel(), -1,
        _("Authentication:"));
    choice_authentication = new wxChoice(getControlsPanel(),
        ID_choice_authentication, wxDefaultPosition, wxDefaultSize,
        getAuthenticationChoices());

    label_username = new wxStaticText(getControlsPanel(), -1, _("User name:"));
    text_ctrl_username = new wxTextCtrl(getControlsPanel(),
        ID_textcontrol_username, wxEmptyString);
    label_password = new wxStaticText(getControlsPanel(), -1, _("Password:"));
    text_ctrl_password = new wxTextCtrl(getControlsPanel(),
        ID_textcontrol_password, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxTE_PASSWORD);

    label_charset = new wxStaticText(getControlsPanel(), -1, _("Charset:"));
    combobox_charset = new wxComboBox(getControlsPanel(), -1, wxT("NONE"),
        wxDefaultPosition, wxDefaultSize, getDatabaseCharsetChoices(),
        wxCB_DROPDOWN | wxCB_SORT);

    label_role = new wxStaticText(getControlsPanel(), -1, _("Role:"));
    text_ctrl_role = new wxTextCtrl(getControlsPanel(), -1, wxT(""));

    if (createM)
    {
        label_pagesize = new wxStaticText(getControlsPanel(), -1,
            _("Page size:"));
        choice_pagesize = new wxChoice(getControlsPanel(), -1,
            wxDefaultPosition, wxDefaultSize, getDatabasePagesizeChoices());
        label_dialect = new wxStaticText(getControlsPanel(), -1,
            _("SQL dialect:"));
        choice_dialect = new wxChoice(getControlsPanel(), -1,
            wxDefaultPosition, wxDefaultSize, getDatabaseDialectChoices());
    }

    button_ok = new wxButton(getControlsPanel(), wxID_SAVE,
        (createM ? _("Create") : _("Save")));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
}
//-----------------------------------------------------------------------------
wxArrayString DatabaseRegistrationDialog::getAuthenticationChoices() const
{
    wxArrayString choices;
    if (connectAsM)
        choices.Add(_("Enter user name and password"));
    else
    {
        choices.Add(_("Use saved user name and password"));
        choices.Add(_("Use saved user name and encrypted password"));
        choices.Add(_("Use saved user name, but always enter password"));
        choices.Add(_("Use trusted user authentication"));
    }
    return choices;
}
//-----------------------------------------------------------------------------
wxArrayString DatabaseRegistrationDialog::getDatabaseCharsetChoices() const
{
    const wxString charset_choices[] = {
        wxT("NONE"), wxT("ASCII"), wxT("BIG_5"), wxT("CYRL"),

        wxT("DOS437"), wxT("DOS737"), wxT("DOS775"), wxT("DOS850"),
        wxT("DOS852"), wxT("DOS857"), wxT("DOS858"), wxT("DOS860"),
        wxT("DOS861"), wxT("DOS862"), wxT("DOS863"), wxT("DOS864"),
        wxT("DOS865"), wxT("DOS866"), wxT("DOS869"),

        wxT("EUCJ_0208"), wxT("GB_2312"),

        wxT("ISO8859_1"), wxT("ISO8859_2"), wxT("ISO8859_3"), wxT("ISO8859_4"),
        wxT("ISO8859_5"), wxT("ISO8859_6"), wxT("ISO8859_7"), wxT("ISO8859_8"),
        wxT("ISO8859_9"), wxT("ISO8859_13"),

        wxT("KSC_5601"), wxT("NEXT"), wxT("OCTETS"), wxT("SJIS_0208"),

        wxT("UNICODE_FSS"), wxT("UTF8"),

        wxT("WIN1250"), wxT("WIN1251"), wxT("WIN1252"), wxT("WIN1253"),
        wxT("WIN1254"), wxT("WIN1255"), wxT("WIN1256"), wxT("WIN1257")
    };
    const size_t cnt = sizeof(charset_choices) / sizeof(wxString);

    wxArrayString choices;
    choices.Alloc(cnt);
    for (size_t i = 0; i < cnt; i++)
        choices.Add(charset_choices[i]);
    return choices;
}
//-----------------------------------------------------------------------------
wxArrayString DatabaseRegistrationDialog::getDatabaseDialectChoices() const
{
    wxArrayString choices;
    choices.Alloc(2);
    // IBPP allows only dialects 1 and 3 for database creation
    choices.Add(_("1"));
    choices.Add(wxT("3"));
    return choices;
}
//-----------------------------------------------------------------------------
wxArrayString DatabaseRegistrationDialog::getDatabasePagesizeChoices() const
{
    wxArrayString choices;
    choices.Alloc(6);
    choices.Add(_("Default"));
    choices.Add(wxT("1024"));
    choices.Add(wxT("2048"));
    choices.Add(wxT("4096"));
    choices.Add(wxT("8192"));
    choices.Add(wxT("16384"));
    return choices;
}
//-----------------------------------------------------------------------------
const wxString DatabaseRegistrationDialog::getName() const
{
    // don't use different names here, force minimal height instead
    // this way it will work for all combinations of control visibility
    return wxT("DatabaseRegistrationDialog");
}
//-----------------------------------------------------------------------------
bool DatabaseRegistrationDialog::getConfigStoresHeight() const
{
    return false;
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::layoutControls()
{
    // create sizer for controls
    wxGridBagSizer* sizerControls = new wxGridBagSizer(styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin());

    sizerControls->Add(label_name, wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(text_ctrl_name, wxGBPosition(0, 1), wxGBSpan(1, 3), wxALIGN_CENTER_VERTICAL | wxEXPAND);

    sizerControls->Add(label_dbpath, wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* sizer_r1c1_3 = new wxBoxSizer(wxHORIZONTAL);
    sizer_r1c1_3->Add(text_ctrl_dbpath, 1, wxALIGN_CENTER_VERTICAL);
    sizer_r1c1_3->Add(button_browse, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, styleguide().getBrowseButtonMargin());
    sizerControls->Add(sizer_r1c1_3, wxGBPosition(1, 1), wxGBSpan(1, 3), wxEXPAND);

    sizerControls->Add(label_authentication, wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(choice_authentication, wxGBPosition(2, 1), wxGBSpan(1, 3), wxALIGN_CENTER_VERTICAL | wxEXPAND);

    int dx = styleguide().getUnrelatedControlMargin(wxHORIZONTAL) - styleguide().getControlLabelMargin();
    if (dx < 0)
        dx = 0;

    sizerControls->Add(label_username, wxGBPosition(3, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(text_ctrl_username, wxGBPosition(3, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerControls->Add(label_password, wxGBPosition(3, 2), wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerControls->Add(text_ctrl_password, wxGBPosition(3, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);

    sizerControls->Add(label_charset, wxGBPosition(4, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerControls->Add(combobox_charset, wxGBPosition(4, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerControls->Add(label_role, wxGBPosition(4, 2), wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerControls->Add(text_ctrl_role, wxGBPosition(4, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);

    if (createM)
    {
        sizerControls->Add(label_pagesize, wxGBPosition(5, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
        sizerControls->Add(choice_pagesize, wxGBPosition(5, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
        sizerControls->Add(label_dialect, wxGBPosition(5, 2), wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
        sizerControls->Add(choice_dialect, wxGBPosition(5, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    }

    sizerControls->AddGrowableCol(1);
    sizerControls->AddGrowableCol(3);

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok, button_cancel);
    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons);
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::setControlsProperties()
{
    int wh = text_ctrl_dbpath->GetMinHeight();
    button_browse->SetSize(wh, wh);
    choice_authentication->SetSelection(0);
    if (createM)
    {
        choice_pagesize->SetSelection(
            choice_pagesize->FindString(wxT("4096")));
        choice_dialect->SetSelection(choice_dialect->FindString(wxT("3")));
    }
    button_ok->SetDefault();
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::setDatabase(DatabasePtr db)
{
    wxASSERT(db);
    databaseM = db;
    /* this could be reactivated if there is a dialog with "Don't show me again"
    if (databaseM->isConnected())
    ::wxMessageBox(_("Properties of connected database cannot be changed."), _("Warning"), wxOK |wxICON_INFORMATION );
    */
    text_ctrl_name->SetValue(databaseM->getName_());
    text_ctrl_dbpath->SetValue(databaseM->getPath());
    text_ctrl_username->SetValue(databaseM->getUsername());
    text_ctrl_password->SetValue(databaseM->getDecryptedPassword());
    text_ctrl_role->SetValue(databaseM->getRole());
    wxString charset(databaseM->getConnectionCharset());
    if (charset.IsEmpty())
        charset = wxT("NONE");
    int selection = combobox_charset->FindString(charset);
    if (selection < 0)
        combobox_charset->SetValue(charset);
    else
        combobox_charset->SetSelection(selection);
    // see whether the database has an empty or default name; knowing that will be
    // useful to keep the name in sync when other attributes change.
    updateIsDefaultName();

    // enable controls depending on operation and database connection status
    // use SetEditable() for edit controls to allow copying text to clipboard
    bool isConnected = databaseM->isConnected();
    text_ctrl_name->SetEditable(!connectAsM);
    text_ctrl_dbpath->SetEditable(!connectAsM && !isConnected);
    button_browse->Enable(!connectAsM && !isConnected);
    choice_authentication->Enable(!connectAsM && !isConnected);
    combobox_charset->Enable(!isConnected);
    text_ctrl_role->SetEditable(!isConnected);
    if (connectAsM)
        button_ok->SetLabel(_("Connect"));
    else
    {
        choice_authentication->SetSelection(
            databaseM->getAuthenticationMode().getMode());
    }
    updateAuthenticationMode();
    updateButtons();
    updateColors();
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::setServer(ServerPtr s)
{
    wxASSERT(s);
    serverM = s;
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::updateAuthenticationMode()
{
    bool isConnected = databaseM->isConnected();
    int sel = choice_authentication->GetSelection();
    // user name not for trusted user authentication
    text_ctrl_username->SetEditable(!isConnected && sel < 3);
    // password not if always to be entered
    // password not for trusted user authentication
    text_ctrl_password->SetEditable(!isConnected && sel < 2);
    updateButtons();
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::updateButtons()
{
    if (button_ok->IsShown())
    {
        bool missingUserName = text_ctrl_username->IsEditable()
            && text_ctrl_username->GetValue().IsEmpty();
        button_ok->Enable(!text_ctrl_dbpath->GetValue().IsEmpty()
            && !text_ctrl_name->GetValue().IsEmpty()
            && !missingUserName);
    }
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::updateIsDefaultName()
{
    isDefaultNameM = ((text_ctrl_name->GetValue().IsEmpty() ||
        text_ctrl_name->GetValue() == buildName(text_ctrl_dbpath->GetValue())));
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(DatabaseRegistrationDialog, BaseDialog)
    EVT_BUTTON(DatabaseRegistrationDialog::ID_button_browse, DatabaseRegistrationDialog::OnBrowseButtonClick)
    EVT_BUTTON(wxID_SAVE, DatabaseRegistrationDialog::OnOkButtonClick)
    EVT_TEXT(DatabaseRegistrationDialog::ID_textcontrol_dbpath, DatabaseRegistrationDialog::OnSettingsChange)
    EVT_TEXT(DatabaseRegistrationDialog::ID_textcontrol_name, DatabaseRegistrationDialog::OnNameChange)
    EVT_TEXT(DatabaseRegistrationDialog::ID_textcontrol_username, DatabaseRegistrationDialog::OnSettingsChange)
    EVT_CHOICE(DatabaseRegistrationDialog::ID_choice_authentication, DatabaseRegistrationDialog::OnAuthenticationChange)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::OnBrowseButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxString path = ::wxFileSelector(_("Select database file"), wxT(""), wxT(""), wxT(""),
        _("Firebird database files (*.fdb, *.gdb)|*.fdb;*.gdb|All files (*.*)|*.*"),
        wxFD_OPEN, this);
    if (!path.empty())
        text_ctrl_dbpath->SetValue(path);
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
    // FIXME: implement a better interface than separate setServer()
    // and setDatabase() methods
    if (serverM)
        databaseM->setServer(serverM);

    // TODO: This needs to be reworked. If the order of method calls is important
    //       then they must not be provided as independent methods !!!

    // Please note that the order of calls is important here:
    // setPath and setUsername and setStoreEncryptedPassword
    // must come before setEncryptedPassword.
    // The reason is that setEncryptedPassword uses those 3 values to determine
    // whether the password needs to be encrypted, and if it does need, it uses
    // them to calculate the key (using master key)
    if (!connectAsM)
    {
        int sel = choice_authentication->GetSelection();
        databaseM->getAuthenticationMode().setMode(sel);
    }
    databaseM->setName_(text_ctrl_name->GetValue());
    databaseM->setPath(text_ctrl_dbpath->GetValue());
    databaseM->setUsername(text_ctrl_username->GetValue());
    databaseM->setEncryptedPassword(text_ctrl_password->GetValue());

    wxBusyCursor wait;
    // for some reason GetValue didn't work correctly before
    // I can't remember the exact issue, perhaps it was platform specific (Gtk1 maybe?)
    // so we replaced all of those with GetStringSelection()
    // the problem is that we are now using a *real* combo, not just a
    // dropdown list. So, this needs testing
    databaseM->setConnectionCharset(combobox_charset->GetValue());
    //databaseM->setConnectionCharset(combobox_charset->GetStringSelection());

    databaseM->setRole(text_ctrl_role->GetValue());

    try
    {
        if (createM)    // create new database
        {
            long ps = 0;
            if (!choice_pagesize->GetStringSelection().ToLong(&ps))
                ps = 0;
            long dialect = 3;
            if (choice_dialect->GetStringSelection() == wxT("1"))
                dialect = 1;
            serverM->createDatabase(databaseM, ps, dialect);
        }
        EndModal(wxID_OK);
    }
    catch (IBPP::Exception &e)
    {
        wxMessageBox(std2wx(e.ErrorMessage()), _("Error"), wxOK|wxICON_ERROR);
    }
    catch (...)
    {
        wxMessageBox(_("SYSTEM ERROR!"), _("Error"), wxOK|wxICON_ERROR);
    }
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::OnSettingsChange(
    wxCommandEvent& WXUNUSED(event))
{
    if (IsShown())
    {
        if (isDefaultNameM)
            text_ctrl_name->SetValue(buildName(text_ctrl_dbpath->GetValue()));
        updateIsDefaultName();
        updateButtons();
    }
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::OnNameChange(wxCommandEvent& WXUNUSED(event))
{
    if (IsShown())
    {
        updateIsDefaultName();
        updateButtons();
    }
}
//-----------------------------------------------------------------------------
void DatabaseRegistrationDialog::OnAuthenticationChange(
    wxCommandEvent& WXUNUSED(event))
{
    if (IsShown())
    {
        updateAuthenticationMode();
        updateColors();
    }
}
//-----------------------------------------------------------------------------
