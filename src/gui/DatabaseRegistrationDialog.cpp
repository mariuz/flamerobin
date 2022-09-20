/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/filename.h>
#include <wx/gbsizer.h>

#include "config/Config.h"
#include "core/StringUtils.h"
#include "gui/controls/DndTextControls.h"
#include "gui/DatabaseRegistrationDialog.h"
#include "gui/StyleGuide.h"
#include "metadata/database.h"
#include "metadata/server.h"

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

//! implementation details
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
        "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

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
    long comboStyle =  wxCB_DROPDOWN;
#ifndef __WXMAC__
    // Not supported on OSX/Cocoa presently
    comboStyle |= wxCB_SORT;
#endif
    combobox_charset = new wxComboBox(getControlsPanel(), -1, "NONE",
        wxDefaultPosition, wxDefaultSize, getDatabaseCharsetChoices(), comboStyle);

    label_role = new wxStaticText(getControlsPanel(), -1, _("Role:"));
    text_ctrl_role = new wxTextCtrl(getControlsPanel(), -1, "");
    /*
    Todo: Implement FB library per conexion
    label_library = new wxStaticText(getControlsPanel(), -1,
        _("Library path:"));
    text_ctrl_library = new FileTextControl(getControlsPanel(),
        ID_textcontrol_library, wxEmptyString);
    button_browse_library = new wxButton(getControlsPanel(), ID_button_browse_library,
        "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
     */


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

wxArrayString DatabaseRegistrationDialog::getDatabaseCharsetChoices() const
{
    const wxString charset_choices[] = {
        "NONE", "ASCII", "BIG_5", "CYRL",

        "DOS437", "DOS737", "DOS775", "DOS850",
        "DOS852", "DOS857", "DOS858", "DOS860",
        "DOS861", "DOS862", "DOS863", "DOS864",
        "DOS865", "DOS866", "DOS869",

        "EUCJ_0208", "GB_2312",

        "ISO8859_1", "ISO8859_2", "ISO8859_3", "ISO8859_4",
        "ISO8859_5", "ISO8859_6", "ISO8859_7", "ISO8859_8",
        "ISO8859_9", "ISO8859_13",

        "KSC_5601", "NEXT", "OCTETS", "SJIS_0208",

        "UNICODE_FSS", "UTF8",

        "WIN1250", "WIN1251", "WIN1252", "WIN1253",
        "WIN1254", "WIN1255", "WIN1256", "WIN1257"
    };
    const size_t cnt = sizeof(charset_choices) / sizeof(wxString);

    wxArrayString choices;
    choices.Alloc(cnt);
    for (size_t i = 0; i < cnt; i++)
        choices.Add(charset_choices[i]);
    return choices;
}

wxArrayString DatabaseRegistrationDialog::getDatabaseDialectChoices() const
{
    wxArrayString choices;
    choices.Alloc(2);
    // only dialects 1 and 3 are valid for database creation
    choices.Add("1");
    choices.Add("3");
    return choices;
}

wxArrayString DatabaseRegistrationDialog::getDatabasePagesizeChoices() const
{
    wxArrayString choices;
    choices.Alloc(7);
    choices.Add(_("Default"));
    choices.Add("1024");
    choices.Add("2048");
    choices.Add("4096");
    choices.Add("8192");
    choices.Add("16384");
    choices.Add("32768");
    return choices;
}

void DatabaseRegistrationDialog::doReadConfigSettings(const wxString& prefix)
{
    BaseDialog::doReadConfigSettings(prefix);

    if (createM)
    {
        int idx = config().get(
            prefix + Config::pathSeparator + "createDialogAuthentication",
            wxNOT_FOUND);
        bool indexValid = idx >= 0
            && idx < static_cast<int>(choice_authentication->GetCount());
        if (indexValid)
            choice_authentication->SetSelection(idx);

        wxString selection, empty;
        selection = config().get(
            prefix + Config::pathSeparator + "createDialogCharset",
            empty);
        if (!selection.empty())
            combobox_charset->SetStringSelection(selection);

        selection = config().get(
            prefix + Config::pathSeparator + "createDialogPageSize",
            empty);
        if (!selection.empty())
            choice_pagesize->SetStringSelection(selection);

        selection = config().get(
            prefix + Config::pathSeparator + "createDialogDialect",
            empty);
        if (!selection.empty())
            choice_dialect->SetStringSelection(selection);

        updateAuthenticationMode();
        updateButtons();
        updateColors();
    }
}

void DatabaseRegistrationDialog::doWriteConfigSettings(
    const wxString& prefix) const
{
    BaseDialog::doWriteConfigSettings(prefix);
    if (createM && GetReturnCode() == wxID_OK)
    {
        config().setValue(
            prefix + Config::pathSeparator + "createDialogAuthentication",
            choice_authentication->GetSelection());
        config().setValue(
            prefix + Config::pathSeparator + "createDialogCharset",
            combobox_charset->GetStringSelection());
        config().setValue(
            prefix + Config::pathSeparator + "createDialogPageSize",
            choice_pagesize->GetStringSelection());
        config().setValue(
            prefix + Config::pathSeparator + "createDialogDialect",
            choice_dialect->GetStringSelection());
    }
}

const wxString DatabaseRegistrationDialog::getName() const
{
    // don't use different names here, force minimal height instead
    // this way it will work for all combinations of control visibility
    return "DatabaseRegistrationDialog";
}

bool DatabaseRegistrationDialog::getConfigStoresHeight() const
{
    return false;
}

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
    
    /*
    * Todo: Implement FB library per conexion
    sizerControls->Add(label_library, wxGBPosition(5, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* sizer_r1c1_4 = new wxBoxSizer(wxHORIZONTAL);
    sizer_r1c1_4->Add(text_ctrl_library, 1, wxALIGN_CENTER_VERTICAL);
    sizer_r1c1_4->Add(button_browse_library, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, styleguide().getBrowseButtonMargin());
    sizerControls->Add(sizer_r1c1_4, wxGBPosition(5, 1), wxGBSpan(1, 3), wxEXPAND);
    */

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

void DatabaseRegistrationDialog::setControlsProperties()
{
    int wh = text_ctrl_dbpath->GetMinHeight();
    button_browse->SetSize(wh, wh);

    choice_authentication->SetSelection(0);
    combobox_charset->SetStringSelection("NONE");
    if (createM)
    {
        choice_pagesize->SetStringSelection("Default");
        choice_dialect->SetStringSelection("3");
    }

    button_ok->SetDefault();
}

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
    /*
    * Todo: Implement FB library per conexion
    text_ctrl_library->SetValue(databaseM->getClientLibrary());
    */
    wxString charset(databaseM->getConnectionCharset());
    if (charset.empty())
        charset = "NONE";
    combobox_charset->SetValue(charset);
    // see whether the database has an empty or default name; knowing that will be
    // useful to keep the name in sync when other attributes change.
    updateIsDefaultDatabaseName();

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

wxString DatabaseRegistrationDialog::getDefaultDatabaseName() const
{
    return Database::extractNameFromConnectionString(
        text_ctrl_dbpath->GetValue());
}

void DatabaseRegistrationDialog::updateIsDefaultDatabaseName()
{
    wxString name(text_ctrl_name->GetValue());
    isDefaultNameM = name.empty() || name == getDefaultDatabaseName();
}

//! event handling
BEGIN_EVENT_TABLE(DatabaseRegistrationDialog, BaseDialog)
    EVT_BUTTON(DatabaseRegistrationDialog::ID_button_browse, DatabaseRegistrationDialog::OnBrowseButtonClick)
    EVT_BUTTON(DatabaseRegistrationDialog::ID_button_browse_library, DatabaseRegistrationDialog::OnBrowseLibraryButtonClick)
    EVT_BUTTON(wxID_SAVE, DatabaseRegistrationDialog::OnOkButtonClick)
    EVT_TEXT(DatabaseRegistrationDialog::ID_textcontrol_dbpath, DatabaseRegistrationDialog::OnSettingsChange)
    EVT_TEXT(DatabaseRegistrationDialog::ID_textcontrol_name, DatabaseRegistrationDialog::OnNameChange)
    EVT_TEXT(DatabaseRegistrationDialog::ID_textcontrol_username, DatabaseRegistrationDialog::OnSettingsChange)
    EVT_CHOICE(DatabaseRegistrationDialog::ID_choice_authentication, DatabaseRegistrationDialog::OnAuthenticationChange)
    EVT_TEXT(DatabaseRegistrationDialog::ID_textcontrol_library, DatabaseRegistrationDialog::OnSettingsChange)
END_EVENT_TABLE()

void DatabaseRegistrationDialog::OnBrowseButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxString path = ::wxFileSelector(_("Select database file"), "", "", "",
        _("Firebird database files (*.fdb, *.gdb)|*.fdb;*.gdb|All files (*.*)|*.*"),
        wxFD_OPEN, this);
    if (!path.empty())
        text_ctrl_dbpath->SetValue(path);
}

void DatabaseRegistrationDialog::OnBrowseLibraryButtonClick(wxCommandEvent& WXUNUSED(event))
{
    /*
    * Todo: Implement FB library per conexion
    wxString path = ::wxFileSelector(_("Select library file"), "", "", "",
        _("Firebird library files (*.dll)|*.dll|All files (*.*)|*.*"),
        wxFD_OPEN, this);
    if (!path.empty())
        text_ctrl_library->SetValue(path);
     */
}

void DatabaseRegistrationDialog::OnOkButtonClick(wxCommandEvent& WXUNUSED(event))
{
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
    /*
    * Todo: Implement FB library per conexion
    databaseM->setClientLibrary(text_ctrl_library->GetValue());
    */

    wxBusyCursor wait;
    databaseM->setConnectionCharset(combobox_charset->GetValue());
    databaseM->setRole(text_ctrl_role->GetValue());

    try
    {
        if (createM)    // create new database
        {
            long ps = 0;
            if (!choice_pagesize->GetStringSelection().ToLong(&ps))
                ps = 0;
            long dialect = 3;
            if (choice_dialect->GetStringSelection() == "1")
                dialect = 1;
            databaseM->create(ps, dialect);
        }
        EndModal(wxID_OK);
    }
    catch (IBPP::Exception &e)
    {
        wxMessageBox(e.what(), _("Error"), wxOK|wxICON_ERROR);
    }
    catch (...)
    {
        wxMessageBox(_("SYSTEM ERROR!"), _("Error"), wxOK|wxICON_ERROR);
    }
}

void DatabaseRegistrationDialog::OnSettingsChange(
    wxCommandEvent& WXUNUSED(event))
{
    if (IsShown())
    {
        if (isDefaultNameM)
            text_ctrl_name->SetValue(getDefaultDatabaseName());
        updateIsDefaultDatabaseName();
        updateButtons();
    }
}

void DatabaseRegistrationDialog::OnNameChange(wxCommandEvent& WXUNUSED(event))
{
    if (IsShown())
    {
        updateIsDefaultDatabaseName();
        updateButtons();
    }
}

void DatabaseRegistrationDialog::OnAuthenticationChange(
    wxCommandEvent& WXUNUSED(event))
{
    if (IsShown())
    {
        updateAuthenticationMode();
        updateColors();
    }
}

