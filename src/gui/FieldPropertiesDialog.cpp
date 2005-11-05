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

  Contributor(s): Nando Dessena, Michael Hieke
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

#include <wx/gbsizer.h>

#include "gui/ExecuteSqlFrame.h"
#include "gui/FieldPropertiesDialog.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "styleguide.h"
#include "ugly.h"
#include "urihandler.h"
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
struct DatatypeProperties
{
    wxString name;
    bool hasSize;
    bool hasScale;
    bool isChar;
};

static const DatatypeProperties datatypes[] = {
    { wxT("Char"), true, false, true },
    { wxT("Varchar"), true, false, true },
    { wxT("Integer") },
    { wxT("Smallint") },
    { wxT("Numeric"), true, true, false },
    { wxT("Decimal"), true, true, false },
    { wxT("BigInt") },
    { wxT("Float") },
    { wxT("Double precision") },
    { wxT("Date") },
    { wxT("Time") },
    { wxT("Timestamp") },
    { wxT("Array") },
    { wxT("Blob") }
};

const size_t datatypescnt = sizeof(datatypes) / sizeof(DatatypeProperties);
//-----------------------------------------------------------------------------
FieldPropertiesDialog::FieldPropertiesDialog(wxWindow* parent, Table* table,
        Column* column)
    : BaseDialog(parent, wxID_ANY, wxEmptyString)
{
    // can't do anything if no table is given
    wxASSERT(table);

    tableM = 0;
    columnM = 0;

    createControls();
    setTableM(table);
    setColumnM(column);
    setControlsProperties();
    updateControls();
    layoutControls();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::createControls()
{
    label_fieldname = new wxStaticText(getControlsPanel(), wxID_ANY,
        _("Field name:"));
    textctrl_fieldname = new wxTextCtrl(getControlsPanel(),
        ID_textctrl_fieldname, wxEmptyString);

    label_domain = new wxStaticText(getControlsPanel(), wxID_ANY,
        _("Domain:"));
    choice_domain = new wxChoice(getControlsPanel(), ID_choice_domain,
        wxDefaultPosition, wxDefaultSize, 0, 0);
    button_edit_domain = new wxButton(getControlsPanel(),
        ID_button_edit_domain, _("Edit domain"));

    label_datatype = new wxStaticText(getControlsPanel(), wxID_ANY,
        _("Datatype:"));
    wxArrayString datatypes_choices;
    datatypes_choices.Alloc(datatypescnt);
    for (size_t n = 0; n < datatypescnt; n++)
        datatypes_choices.Add(datatypes[n].name);
    choice_datatype = new wxChoice(getControlsPanel(), ID_choice_datatype,
        wxDefaultPosition, wxDefaultSize, datatypes_choices);
    label_size = new wxStaticText(getControlsPanel(), wxID_ANY, _("Size:"));
    textctrl_size = new wxTextCtrl(getControlsPanel(), wxID_ANY, wxEmptyString);
    label_scale = new wxStaticText(getControlsPanel(), wxID_ANY, _("Scale:"));
    textctrl_scale = new wxTextCtrl(getControlsPanel(), wxID_ANY, wxEmptyString);

    label_charset = new wxStaticText(getControlsPanel(), wxID_ANY, _("Charset:"));
    const wxString charset_choices[] = {
        wxT("NONE")
    };
    choice_charset = new wxChoice(getControlsPanel(), ID_choice_charset,
        wxDefaultPosition, wxDefaultSize,
        sizeof(charset_choices) / sizeof(wxString), charset_choices);
    label_collate = new wxStaticText(getControlsPanel(), wxID_ANY, _("Collate:"));
    choice_collate = new wxChoice(getControlsPanel(), ID_choice_collate,
        wxDefaultPosition, wxDefaultSize, 0, 0);

    checkbox_notnull = new wxCheckBox(getControlsPanel(), wxID_ANY, _("Not null"));

    static_line_autoinc = new wxStaticLine(getControlsPanel());
    label_autoinc = new wxStaticText(getControlsPanel(), wxID_ANY, _("Autoincrement"));

    radio_generator_new = new wxRadioButton(getControlsPanel(),
        ID_radio_generator_new, _("Create new generator:"));
    textctrl_generator_name = new wxTextCtrl(getControlsPanel(),
        ID_textctrl_generator_name, wxEmptyString);

    radio_generator_existing = new wxRadioButton(getControlsPanel(),
        ID_radio_generator_existing, _("Use existing generator:"));
    choice_generator = new wxChoice(getControlsPanel(), ID_choice_generator,
        wxDefaultPosition, wxDefaultSize, 0, 0);

    checkbox_trigger = new wxCheckBox(getControlsPanel(), ID_checkbox_trigger,
        _("Create trigger"));
    textctrl_sql = new wxTextCtrl(getControlsPanel(), wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

    button_ok = new wxButton(getControlsPanel(), ID_button_ok, _("Execute"));
    button_cancel = new wxButton(getControlsPanel(), ID_button_cancel,
        _("Cancel"));
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::layoutControls()
{
    // create sizer for controls
    wxGridBagSizer* sizerTop = new wxGridBagSizer(styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin());

    sizerTop->Add(label_fieldname, wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerTop->Add(textctrl_fieldname, wxGBPosition(0, 1), wxGBSpan(1, 5), wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_domain, wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* sizerDomain = new wxBoxSizer(wxHORIZONTAL);
    sizerDomain->Add(choice_domain, 1, wxEXPAND);
    sizerDomain->Add(styleguide().getBrowseButtonMargin(), 0);
    sizerDomain->Add(button_edit_domain);
    sizerTop->Add(sizerDomain, wxGBPosition(1, 1), wxGBSpan(1, 5), wxALIGN_CENTER_VERTICAL | wxEXPAND);

    int dx = styleguide().getUnrelatedControlMargin(wxHORIZONTAL) - styleguide().getControlLabelMargin();
    if (dx < 0)
        dx = 0;

    sizerTop->Add(label_datatype, wxGBPosition(2, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerTop->Add(choice_datatype, wxGBPosition(2, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_size, wxGBPosition(2, 2), wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerTop->Add(textctrl_size, wxGBPosition(2, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_scale, wxGBPosition(2, 4), wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerTop->Add(textctrl_scale, wxGBPosition(2, 5), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_charset, wxGBPosition(3, 2), wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerTop->Add(choice_charset, wxGBPosition(3, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_collate, wxGBPosition(3, 4), wxDefaultSpan, wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerTop->Add(choice_collate, wxGBPosition(3, 5), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);

    sizerTop->AddGrowableCol(1);
    sizerTop->AddGrowableCol(3);
    sizerTop->AddGrowableCol(5);

    wxGridBagSizer* sizerGenerator = new wxGridBagSizer(styleguide().getRelatedControlMargin(wxHORIZONTAL),
        styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerGenerator->Add(radio_generator_new, wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerGenerator->Add(textctrl_generator_name, wxGBPosition(0, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerGenerator->Add(radio_generator_existing, wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerGenerator->Add(choice_generator, wxGBPosition(1, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerGenerator->AddGrowableCol(1);

    // stack everything vertically
    wxBoxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    sizerControls->Add(sizerTop, 0, wxEXPAND);
    sizerControls->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerControls->Add(checkbox_notnull);
    sizerControls->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerControls->Add(static_line_autoinc, 0, wxEXPAND);
    sizerControls->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerControls->Add(label_autoinc);
    sizerControls->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerControls->Add(sizerGenerator, 0, wxEXPAND);
    sizerControls->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerControls->Add(checkbox_trigger);
    sizerControls->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerControls->Add(textctrl_sql, 1, wxEXPAND);

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_ok, button_cancel);
    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons, true);
}
//-----------------------------------------------------------------------------
const wxString FieldPropertiesDialog::getName() const
{
    return wxT("FieldPropertiesDialog");
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::removeSubject(Subject* subject)
{
    Observer::removeSubject(subject);
    if ((subject) && ((subject == tableM) || (subject == columnM)))
    {
        if (subject == tableM)
            setTableM(0);
        if (subject == columnM)
            setColumnM(0);
        EndModal(wxID_CANCEL);
    }
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::update()
{
    updateControls();
}
//-----------------------------------------------------------------------------
bool FieldPropertiesDialog::getDomainInfo(const wxString& domain,
    wxString& type, wxString& size, wxString& scale, wxString& charset)
{
    Database* db = tableM->getDatabase();
    if (db)
    {
        MetadataCollection<Domain>::const_iterator it;
        for (it = db->domainsBegin(); it != db->domainsEnd(); ++it)
        {
            if (domain == (*it).getName())
            {
                Domain* d = (Domain*)&(*it);
                d->getDatatypeParts(type, size, scale);
                charset = d->getCharset();
                return true;
            }
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
bool FieldPropertiesDialog::getIsNewDomainSelected()
{
    // first item is "[Create new]"
    return choice_domain->GetSelection() == 0;
}
//-----------------------------------------------------------------------------
// UDD = user defined domain
// AGD = auto generated domain (those starting with RDB$)
bool FieldPropertiesDialog::getStatementsToExecute(wxString& sql)
{
    wxString fieldName = textctrl_fieldname->GetValue();
    wxString selDomain = choice_domain->GetStringSelection();
    bool newDomain = getIsNewDomainSelected();
    wxString selDatatype = choice_datatype->GetStringSelection();
    wxString dtSize = textctrl_size->GetValue();
    wxString dtScale = textctrl_scale->GetValue();
    bool isNullable = !checkbox_notnull->IsChecked();

    int n = choice_datatype->GetSelection();
    if (n >= 0 && n < datatypescnt)
    {
        if (!datatypes[n].hasSize)
            dtSize.Clear();
        if (!datatypes[n].hasScale)
            dtScale.Clear();
    }

    wxString alterTable = wxT("ALTER TABLE ") + tableM->getName() + wxT(" ");
    enum unn { unnNone, unnBefore, unnAfter } update_not_null = unnNone;
    sql = wxEmptyString;

    // detect changes to existing field, create appropriate SQL actions
    if (columnM)
    {
        // field name changed ?
        if (columnM->getName() != fieldName)
        {
            sql += alterTable + wxT("ALTER ") + columnM->getName()
                + wxT(" TO ") + fieldName + wxT(";\n\n");
        }

        // domain changed ?
        wxString type, size, scale, charset;
        if (!getDomainInfo(columnM->getSource(), type, size, scale, charset))
        {
            ::wxMessageBox(_("Can not get domain info - aborting."),
                _("Error"), wxOK | wxICON_ERROR);
            return false;
        }
        if (columnM->getSource() != selDomain && !newDomain)
        {   // UDD -> other UDD  or  AGD -> UDD
            sql += alterTable + wxT("ALTER ") + fieldName +
                wxT(" TYPE ") + selDomain + wxT(";\n\n");
        }
        else if (newDomain
            || type != selDatatype || size != dtSize || scale != dtScale)
        {   // UDD -> AGD  or  AGD -> different AGD
            sql += alterTable + wxT("ALTER ") + fieldName +
                wxT(" TYPE ");
            sql += selDatatype;
            if (!dtSize.IsEmpty())
            {
                sql += wxT("(") + dtSize;
                if (!dtScale.IsEmpty())
                    sql += wxT(",") + dtScale;
                sql += wxT(")");
            }
            sql += wxT(";\n\n");
        }

        // not null option changed ?
        if (isNullable != columnM->isNullable())
        {
            if (!isNullable) // change from NULL to NOT NULL
                update_not_null = unnBefore;

            sql += wxT("UPDATE RDB$RELATION_FIELDS SET RDB$NULL_FLAG = ");
            if (isNullable)
                sql += wxT("NULL");
            else
                sql += wxT("1");
            sql += wxT("\nWHERE RDB$FIELD_NAME = '") + fieldName
                + wxT("' AND RDB$RELATION_NAME = '") + tableM->getName()
                + wxT("';\n\n");
        }
    }
    else // create new field
    {
        sql += alterTable + wxT("ADD \n") + fieldName + wxT(" ");
        if (newDomain)
        {
            sql += selDatatype;
            if (!dtSize.IsEmpty())
            {
                sql += wxT("(") + dtSize;
                if (!dtScale.IsEmpty())
                    sql += wxT(",") + dtScale;
                sql += wxT(")");
            }
        }
        else
            sql += selDomain;

        if (!isNullable)
        {
            sql += wxT(" not null");
            update_not_null = unnAfter;
        }
        sql += wxT(";\n\n");
    }

    if (update_not_null != unnNone)
    {
        wxString s = ::wxGetTextFromUser(
            _("Enter value for existing fields containing NULL"),
            _("Update Existing NULL Values"), wxT(""), this);
        wxString sqlAdd = wxT("UPDATE ") + tableM->getName()
            + wxT(" \nSET ") + fieldName + wxT(" = '") + s
            + wxT("' \nWHERE ") + fieldName + wxT(" IS NULL;\n");
        if (update_not_null == unnBefore)
            sql = sqlAdd + sql;
        else
            sql += wxT("COMMIT;\n") + sqlAdd;
    }
    return true;
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::loadCharsets()
{
    choice_charset->Freeze();
    choice_charset->Clear();
    choice_charset->Append(wxT("NONE"));

    if (tableM && tableM->getDatabase())
    {
        vector<wxString> charsets;
        tableM->getDatabase()->fillVector(charsets,
            wxT("select rdb$character_set_name from rdb$character_sets order by 1"));
        for (vector<wxString>::iterator it = charsets.begin(); it != charsets.end(); ++it)
        {
            if ((*it) != wxT("NONE"))
                choice_charset->Append(*it);
        }
    }
    choice_charset->Thaw();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::loadCollations()
{
    choice_collate->Freeze();
    choice_collate->Clear();

    if (tableM && tableM->getDatabase())
    {
        wxString charset(choice_charset->GetStringSelection());
        vector<wxString> cols = tableM->getDatabase()->getCollations(charset);
        for (vector<wxString>::iterator it = cols.begin(); it != cols.end(); it++)
            choice_collate->Append(*it);
    }
    choice_collate->Thaw();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::loadDomains()
{
    choice_domain->Freeze();
    choice_domain->Clear();

    if (columnM == 0 ||columnM->getSource().substr(0,4) != wxT("RDB$"))
    {
        choice_domain->Append(wxT("[Create new]"));
        if (!columnM)
            choice_domain->SetSelection(0);
    }

    if (tableM && tableM->getDatabase())
    {
        Database* db = tableM->getDatabase();
        MetadataCollection<Domain>::const_iterator it;
        for (it = db->domainsBegin(); it != db->domainsEnd(); ++it)
        {
            wxString name = (*it).getName();
            // ignore RDB$XXX domains unless it's the one columnM uses
            bool addDomain = name.substr(0, 4) != wxT("RDB$")
                || (columnM && columnM->getSource() == name);
            if (addDomain)
                choice_domain->Append(name);
        }
    }
    choice_domain->Thaw();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::loadGeneratorNames()
{
    choice_generator->Freeze();
    choice_generator->Clear();

    if (tableM && tableM->getDatabase())
    {
        Database* db = tableM->getDatabase();
        MetadataCollection<Generator>::const_iterator it;
        for (it = db->generatorsBegin(); it != db->generatorsEnd(); ++it)
            choice_generator->Append((*it).getName());
    }
    choice_generator->Thaw();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::setColumnM(Column* column)
{
    if (columnM != column)
    {
        if (columnM)
            columnM->detachObserver(this);
        columnM = column;
        if (columnM)
            columnM->attachObserver(this);
    }
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::setControlsProperties()
{
    // set dialog title
    wxString fmt;
    if (columnM)
        fmt = _("Table %s: Field Properties");
    else
        fmt = _("Table %s: Create New Field");
    SetTitle(wxString::Format(fmt, tableM->getName().c_str()));

    // bold font for "Autoincrement" label
    wxFont font(label_autoinc->GetFont());
    font.SetWeight(wxBOLD);
    label_autoinc->SetFont(font);

    // select items in controls without selection, this is called after
    // database and field (if applicable) have been set
    if (choice_domain->GetCount() > 0 && choice_domain->GetSelection() == wxNOT_FOUND)
        choice_domain->SetSelection(0);
    if (choice_datatype->GetCount() > 0 && choice_datatype->GetSelection() == wxNOT_FOUND)
        choice_datatype->SetSelection(0);
    if (choice_charset->GetCount() > 0 && choice_charset->GetSelection() == wxNOT_FOUND)
        choice_charset->SetSelection(0);
    textctrl_generator_name->SetEditable(false);
    updateColors();
    radio_generator_existing->SetValue(true);
    if (choice_generator->GetCount() > 0 && choice_generator->GetSelection() == wxNOT_FOUND)
        choice_generator->SetSelection(0);

    button_ok->SetDefault();
    button_cancel->SetFocus();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::setTableM(Table* table)
{
    if (tableM != table)
    {
        if (tableM)
            tableM->detachObserver(this);
        tableM = table;
        if (tableM)
            tableM->attachObserver(this);
    }
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::updateColumnControls()
{
    if (columnM)
    {
        textctrl_fieldname->SetValue(columnM->getName());
        checkbox_notnull->SetValue(!columnM->isNullable());
        choice_domain->SetSelection(
            choice_domain->FindString(columnM->getSource()));
        updateDomainControls();
        loadCollations();
        choice_collate->SetSelection(
            choice_collate->FindString(columnM->getCollation()));
    }
    updateDatatypeInfo();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::updateControls()
{
    button_ok->Enable(!textctrl_fieldname->GetValue().IsEmpty());

    wxString genName;
    if (tableM)
        genName = wxT("GEN_") + tableM->getName() + wxT("_ID");
    textctrl_generator_name->SetValue(genName);

    loadGeneratorNames();
    loadCharsets();
    loadDomains();
    updateColumnControls();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::updateDatatypeInfo()
{
    int n = choice_datatype->GetSelection();
    bool indexOk = n >= 0 && n < datatypescnt;
    choice_charset->Enable(columnM == 0 && indexOk && datatypes[n].isChar);
    if (!choice_charset->IsEnabled())
        choice_charset->SetSelection(wxNOT_FOUND);
    textctrl_size->SetEditable(indexOk && datatypes[n].hasSize);
    if (!textctrl_size->IsEditable())
        textctrl_size->SetValue(wxEmptyString);
    textctrl_scale->SetEditable(indexOk && datatypes[n].hasScale);
    if (!textctrl_scale->IsEditable())
        textctrl_scale->SetValue(wxEmptyString);
    choice_collate->Enable(columnM == 0 && indexOk && datatypes[n].isChar);
    if (!choice_collate->IsEnabled())
        choice_collate->SetSelection(wxNOT_FOUND);
    updateColors();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::updateDomainControls()
{
    wxString domain = choice_domain->GetStringSelection();
    bool newDomain = getIsNewDomainSelected();
    if (!newDomain)
        updateDomainInfo(domain);

    // data type, size, scale and collate are not editable for all other
    // already existing domains
    bool allowEdit = (newDomain || domain.Mid(0, 4) == wxT("RDB$"));
    choice_datatype->Enable(allowEdit);
    if (allowEdit)
        updateDatatypeInfo();
    else
    {
        textctrl_size->SetEditable(false);
        textctrl_scale->SetEditable(false);
        choice_collate->Enable(false);
        updateColors();
    }

    // charset is editable only for new fields with new domain
    choice_charset->Enable(columnM == 0 && newDomain);
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::updateDomainInfo(const wxString& domain)
{
    wxString type, size, scale, charset;
    if (!getDomainInfo(domain, type, size, scale, charset))
        return;
    textctrl_scale->SetValue(scale);
    textctrl_size->SetValue(size);
    choice_datatype->SetSelection(choice_datatype->FindString(type));
    choice_charset->SetSelection(choice_charset->FindString(charset));
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::updateSqlStatement()
{
    wxString table = tableM->getName();
    wxString field = textctrl_fieldname->GetValue();
    wxString generator = choice_generator->GetStringSelection();

    wxString sql;
    if (radio_generator_new->GetValue())
    {
        generator = textctrl_generator_name->GetValue();
        sql = wxT("CREATE GENERATOR ") + generator + wxT(";\n\n");
    }

    if (checkbox_trigger->IsChecked())
    {
        sql += wxT("SET TERM !! ;\n");
        sql += wxT("CREATE TRIGGER ") + table + wxT("_BI FOR ") + table + wxT("\n");
        sql += wxT("ACTIVE BEFORE INSERT POSITION 0\nAS\nBEGIN\n");
        sql += wxT("  IF (NEW.") + field + wxT(" IS NULL) THEN\n");
        sql += wxT("    NEW.") + field + wxT(" = GEN_ID(") + generator + wxT(", 1);\n");
        sql += wxT("END!!\n");
        sql += wxT("SET TERM ; !!\n");
    }

    textctrl_sql->SetValue(sql);
}
//-----------------------------------------------------------------------------
//! event handling
BEGIN_EVENT_TABLE(FieldPropertiesDialog, BaseDialog)
    EVT_BUTTON(FieldPropertiesDialog::ID_button_edit_domain, FieldPropertiesDialog::OnButtonEditDomainClick)
    EVT_BUTTON(FieldPropertiesDialog::ID_button_ok, FieldPropertiesDialog::OnButtonOkClick)
    EVT_CHECKBOX(FieldPropertiesDialog::ID_checkbox_trigger, FieldPropertiesDialog::OnNeedsUpdateSql)
    EVT_CHOICE(FieldPropertiesDialog::ID_choice_charset, FieldPropertiesDialog::OnChoiceCharsetClick)
    EVT_CHOICE(FieldPropertiesDialog::ID_choice_datatype, FieldPropertiesDialog::OnChoiceDatatypeClick)
    EVT_CHOICE(FieldPropertiesDialog::ID_choice_domain, FieldPropertiesDialog::OnChoiceDomainClick)
    EVT_CHOICE(FieldPropertiesDialog::ID_choice_generator, FieldPropertiesDialog::OnNeedsUpdateSql)
    EVT_RADIOBUTTON(FieldPropertiesDialog::ID_radio_generator_existing, FieldPropertiesDialog::OnRadioGeneratorClick)
    EVT_RADIOBUTTON(FieldPropertiesDialog::ID_radio_generator_new, FieldPropertiesDialog::OnRadioGeneratorClick)
    EVT_TEXT(FieldPropertiesDialog::ID_textctrl_fieldname, FieldPropertiesDialog::OnTextFieldnameUpdate)
    EVT_TEXT(FieldPropertiesDialog::ID_textctrl_generator_name, FieldPropertiesDialog::OnNeedsUpdateSql)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::OnButtonEditDomainClick(wxCommandEvent& WXUNUSED(event))
{
    // create DomainPropertiesFrame & show it
    // when done, reload domain definition
    // updateDomainInfo(wx2std(cb_domains->GetValue()));
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::OnButtonOkClick(wxCommandEvent& WXUNUSED(event))
{
    updateSqlStatement();

    wxString sql;
    if (!getStatementsToExecute(sql))
        return;
    sql += textctrl_sql->GetValue();
    if (sql.IsEmpty())
    {
        EndModal(wxID_OK);
        return;
    }

    wxString title;
    if (columnM)
        title = _("Executing Field Modification Script");
    else
        title = _("Executing Field Creation Script");
    ExecuteSqlFrame* esf = new ExecuteSqlFrame(GetParent(), wxID_ANY, title);
    esf->setDatabase(tableM->getDatabase());
    esf->setSql(sql);

    // close dialog before showing the sql frame, otherwise parent frame of
    // this dialog is brought to top instead of ExecuteSqlFrame
    Close();
    esf->Show();
    esf->Raise();
    // user must commit/rollback + frame is closed at once
    esf->executeAllStatements(true);
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::OnChoiceCharsetClick(wxCommandEvent& WXUNUSED(event))
{
    wxString oldCol(choice_collate->GetStringSelection());
    loadCollations();
    choice_collate->SetSelection(choice_collate->FindString(oldCol));
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::OnChoiceDatatypeClick(wxCommandEvent& WXUNUSED(event))
{
    updateDatatypeInfo();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::OnChoiceDomainClick(wxCommandEvent& WXUNUSED(event))
{
    updateDomainControls();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::OnNeedsUpdateSql(wxCommandEvent& WXUNUSED(event))
{
    updateSqlStatement();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::OnRadioGeneratorClick(wxCommandEvent& WXUNUSED(event))
{
    textctrl_generator_name->SetEditable(radio_generator_new->GetValue());
    updateColors();
    choice_generator->Enable(radio_generator_existing->GetValue());
    updateSqlStatement();
}
//-----------------------------------------------------------------------------
void FieldPropertiesDialog::OnTextFieldnameUpdate(wxCommandEvent& WXUNUSED(event))
{
    button_ok->Enable(!textctrl_fieldname->GetValue().IsEmpty());
    updateSqlStatement();
}
//-----------------------------------------------------------------------------
