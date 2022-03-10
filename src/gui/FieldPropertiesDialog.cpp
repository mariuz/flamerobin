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

#include <wx/gbsizer.h>
#include <wx/wupdlock.h>

#include "core/ArtProvider.h"
#include "core/StringUtils.h"
#include "core/URIProcessor.h"
#include "engine/MetadataLoader.h"
#include "gui/ExecuteSql.h"
#include "gui/FieldPropertiesDialog.h"
#include "gui/GUIURIHandlerHelper.h"
#include "gui/StyleGuide.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/generator.h"
#include "metadata/MetadataItemURIHandlerHelper.h"
#include "metadata/table.h"

struct DatatypeProperties
{
    wxString name;
    //wxChar name[];    // doesn't work with MSVC
    //const wxChar* name;
    bool hasSize;
    bool hasScale;
    bool isChar;
    bool identity;
};

static const DatatypeProperties datatypes[] = {
    { "Char", true, false, true, false },
    { "Boolean", false, false, true, false }, // Firebird v3
    { "Varchar", true, false, true, false },
    { "Integer", false, false, false, true},
    { "Smallint", false, false, false, true },
    { "Numeric", true, true, false, true },
    { "Decimal", true, true, false, true },
    { "BigInt", false, false, false, true },
    { "Float" },
    { "Double precision" },
    { "Date" },
    { "Time" },
    { "Timestamp" },
    { "Array" },
    { "Blob" }
};

const size_t datatypescnt = sizeof(datatypes) / sizeof(DatatypeProperties);

FieldPropertiesDialog::FieldPropertiesDialog(wxWindow* parent, Table* table,
        Column* column)
    : BaseDialog(parent, wxID_ANY, wxEmptyString), columnM(column),
        tableM(table)
{
    // can't do anything if no table is given
    wxASSERT(table);

    if (table)
        table->attachObserver(this, false);
    if (columnM)
        columnM->attachObserver(this, false);

    createControls();
    setControlsProperties();
    updateControls();
    layoutControls();

    SetIcon(wxArtProvider::GetIcon(ART_Column, wxART_FRAME_ICON));
}

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
        "NONE"
    };
    choice_charset = new wxChoice(getControlsPanel(), ID_choice_charset,
        wxDefaultPosition, wxDefaultSize,
        sizeof(charset_choices) / sizeof(wxString), charset_choices);
    label_collate = new wxStaticText(getControlsPanel(), wxID_ANY, _("Collate:"));
    choice_collate = new wxChoice(getControlsPanel(), ID_choice_collate,
        wxDefaultPosition, wxDefaultSize, 0, 0);

    checkbox_notnull = new wxCheckBox(getControlsPanel(), wxID_ANY, _("Not null"));
    {
        DatabasePtr db = tableM->getDatabase();
        if (db->getInfo().getODSVersionIsHigherOrEqualTo(12.0)) {
            checkbox_identity = new wxCheckBox(getControlsPanel(), ID_checkbox_identity, _("Identity"));
            label_initialValue = new wxStaticText(getControlsPanel(), wxID_ANY, _("Initial Value:"));
            textctrl_initialValue = new wxTextCtrl(getControlsPanel(), wxID_ANY, wxEmptyString);
            //label_incrementalValue = new wxStaticText(getControlsPanel(), wxID_ANY, _("Icremental Value:"));
            //textctrl_incrementalValue = new wxTextCtrl(getControlsPanel(), wxID_ANY, wxEmptyString);
        }
    }
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

    button_ok = new wxButton(getControlsPanel(), wxID_OK, _("Execute"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
}

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

    int dx = styleguide().getUnrelatedControlMargin(wxHORIZONTAL) -
        styleguide().getControlLabelMargin();
    if (dx < 0)
        dx = 0;

    sizerTop->Add(label_datatype, wxGBPosition(2, 0), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL);
    sizerTop->Add(choice_datatype, wxGBPosition(2, 1), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_size, wxGBPosition(2, 2), wxDefaultSpan,
        wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerTop->Add(textctrl_size, wxGBPosition(2, 3), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_scale, wxGBPosition(2, 4), wxDefaultSpan,
        wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerTop->Add(textctrl_scale, wxGBPosition(2, 5), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_charset, wxGBPosition(3, 2), wxDefaultSpan,
        wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerTop->Add(choice_charset, wxGBPosition(3, 3), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerTop->Add(label_collate, wxGBPosition(3, 4), wxDefaultSpan,
        wxLEFT | wxALIGN_CENTER_VERTICAL, dx);
    sizerTop->Add(choice_collate, wxGBPosition(3, 5), wxDefaultSpan,
        wxALIGN_CENTER_VERTICAL | wxEXPAND);
    {
        DatabasePtr db = tableM->getDatabase();
        if (db->getInfo().getODSVersionIsHigherOrEqualTo(12.0)) {
            sizerTop->Add(checkbox_identity, wxGBPosition(4, 0), wxDefaultSpan,
                wxALIGN_CENTER_VERTICAL | wxEXPAND);
            sizerTop->Add(label_initialValue, wxGBPosition(4, 2), wxDefaultSpan,
                wxALIGN_CENTER_VERTICAL | wxEXPAND);
            sizerTop->Add(textctrl_initialValue, wxGBPosition(4, 3), wxDefaultSpan,
                wxALIGN_CENTER_VERTICAL | wxEXPAND);
            //sizerTop->Add(label_incrementalValue, wxGBPosition(4, 4), wxDefaultSpan,
            //    wxALIGN_CENTER_VERTICAL | wxEXPAND);
            //sizerTop->Add(textctrl_incrementalValue, wxGBPosition(4, 5), wxDefaultSpan,
            //    wxALIGN_CENTER_VERTICAL | wxEXPAND);
        }
    }

    sizerTop->AddGrowableCol(1);
    sizerTop->AddGrowableCol(3);
    sizerTop->AddGrowableCol(5);

    wxGridBagSizer* sizerGenerator = new wxGridBagSizer(
        styleguide().getRelatedControlMargin(wxHORIZONTAL),
        styleguide().getRelatedControlMargin(wxVERTICAL));
    sizerGenerator->Add(radio_generator_new, wxGBPosition(0, 0),
        wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerGenerator->Add(textctrl_generator_name, wxGBPosition(0, 1),
        wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
    sizerGenerator->Add(radio_generator_existing, wxGBPosition(1, 0),
        wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    sizerGenerator->Add(choice_generator, wxGBPosition(1, 1),
        wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
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

const wxString FieldPropertiesDialog::getName() const
{
    return "FieldPropertiesDialog";
}

void FieldPropertiesDialog::subjectRemoved(Subject* subject)
{
    if ((subject) && ((subject == tableM) || (subject == columnM)))
    {
        subject->detachObserver(this);
        EndModal(wxID_CANCEL);
    }
}

void FieldPropertiesDialog::update()
{
    updateControls();
}

bool FieldPropertiesDialog::getDomainInfo(const wxString& domain,
    wxString& type, wxString& size, wxString& scale, wxString& charset)
{
    if (DatabasePtr db = tableM->getDatabase())
    {
        if (DomainPtr dm = db->getDomain(domain))
        {
            dm->getDatatypeParts(type, size, scale);
            charset = dm->getCharset();
            return true;
        }
    }
    return false;
}

bool FieldPropertiesDialog::getIsNewDomainSelected()
{
    // first item is "[Create new]"
    // Note: The following code does not work when there are no user defined
    // domains:
    // return choice_domain->GetSelection() == 0;
    return choice_domain->GetStringSelection() == _("[Create new]");
}

bool FieldPropertiesDialog::getNotNullConstraintName(const wxString& fieldName,
    wxString& constraintName)
{
    if (DatabasePtr db = tableM->getDatabase())
    {
        wxMBConv* conv = db->getCharsetConverter();
        MetadataLoader* loader = db->getMetadataLoader();
        MetadataLoaderTransaction tr(loader);

        IBPP::Statement& st1 = loader->getStatement(
            "SELECT rc.RDB$CONSTRAINT_NAME FROM RDB$RELATION_CONSTRAINTS rc "
            "JOIN RDB$CHECK_CONSTRAINTS cc "
            "ON rc.RDB$CONSTRAINT_NAME = cc.RDB$CONSTRAINT_NAME "
            "WHERE rc.RDB$CONSTRAINT_TYPE = 'NOT NULL' "
            "AND rc.RDB$RELATION_NAME = ?"
            "AND cc.RDB$TRIGGER_NAME = ?");

        st1->Set(1, wx2std(tableM->getName_(), conv));
        st1->Set(2, wx2std(fieldName, conv));
        st1->Execute();
        if (st1->Fetch())
        {
            std::string s;
            st1->Get(1, s);
            constraintName = std2wxIdentifier(s, conv);
            return true;
        }
    }

    return false;
}

// UDD = user defined domain
// AGD = auto generated domain (those starting with RDB$)
bool FieldPropertiesDialog::getStatementsToExecute(wxString& statements,
    bool justCheck)
{
    wxString colNameSql(Identifier::userString(textctrl_fieldname->GetValue()));
    Identifier selDomain(choice_domain->GetStringSelection());
    bool newDomain = getIsNewDomainSelected();
    wxString selDatatype = choice_datatype->GetStringSelection();
    wxString dtSize = textctrl_size->GetValue();
    wxString dtScale = textctrl_scale->GetValue();
    bool isNullable = !checkbox_notnull->IsChecked();
    bool isIdentity = tableM->getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(12.0) ? checkbox_identity->IsChecked() : false;
    wxString initialValue = tableM->getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(12.0) ? textctrl_initialValue->GetValue() : "";

    int n = choice_datatype->GetSelection();
    if (n >= 0 && n < datatypescnt)
    {
        if (!datatypes[n].hasSize)
            dtSize.Clear();
        if (!datatypes[n].hasScale)
            dtScale.Clear();
    }

    wxString alterTable = "ALTER TABLE " + tableM->getQuotedName() + " ";
    enum unn { unnNone, unnBefore, unnAfter } update_not_null = unnNone;

    // detect changes to existing field, create appropriate SQL actions
    if (columnM)
    {
        // field name changed ?
        // compare regardless of active quoting rules, so that name
        // will remain unchanged if edit field contents haven't changed
        // OR if the altered name would result in same SQL identifier
        if (textctrl_fieldname->GetValue() == columnM->getName_()
            || colNameSql == columnM->getQuotedName())
        {
            // no changes -> use original name for all other statements
            colNameSql = columnM->getQuotedName();
        }
        else
        {
            statements += alterTable + "ALTER " + columnM->getQuotedName()
                + " TO " + colNameSql + ";\n\n";
        }

        // domain changed ?
        wxString type, size, scale, charset;
        if (!getDomainInfo(columnM->getSource(), type, size, scale, charset))
        {
            ::wxMessageBox(_("Can not get domain info - aborting."),
                _("Error"), wxOK | wxICON_ERROR);
            return false;
        }
        if (columnM->getSource() != selDomain.get() && !newDomain)
        {   // UDD -> other UDD  or  AGD -> UDD
            statements += alterTable + "ALTER " + colNameSql +
                " TYPE " + selDomain.getQuoted() + ";\n\n";
        }
        else if (newDomain
            || type.CmpNoCase(selDatatype) || size != dtSize || scale != dtScale)
        {   // UDD -> AGD  or  AGD -> different AGD
            statements += alterTable + "ALTER " + colNameSql +
                " TYPE ";
            statements += selDatatype;
            if (!dtSize.IsEmpty())
            {
                statements += "(" + dtSize;
                if (!dtScale.IsEmpty())
                    statements += "," + dtScale;
                statements += ")";
            }
            statements += ";\n\n";
        }

        // not null option changed ?
        if (isNullable != columnM->isNullable(CheckDomainNullability))
        {
            if (!isNullable) // change from NULL to NOT NULL
                update_not_null = unnBefore;
            // direct change in RDB$RELATION_FIELDS needs unquoted field name
            Identifier id;
            id.setFromSql(colNameSql);
            wxString fnm = id.get();
            fnm.Replace("'", "''");
            wxString tnm = tableM->getName_();

            if (columnM->getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(12,0))
            {
                statements += "ALTER TABLE " + tableM->getQuotedName() + " ALTER " + colNameSql + " ";
                if (isNullable)
                    statements += "DROP";
                else
                    statements += "SET";
                statements += " NOT NULL ;\n\n";

            } else {
                statements += "UPDATE RDB$RELATION_FIELDS SET RDB$NULL_FLAG = ";
                if (isNullable)
                    statements += "NULL";
                else
                    statements += "1";

                statements += "\nWHERE RDB$FIELD_NAME = '" + fnm
                    + "' AND RDB$RELATION_NAME = '" + tnm
                    + "';\n\n";

                if (isNullable) // change from NOT NULL to NULL
                {
                    wxString constraintName;
                    //I'm not 100% sure this code runs with ODS>=12
                    if (getNotNullConstraintName(fnm, constraintName))
                    {
                        statements += alterTable + "DROP CONSTRAINT "
                            + constraintName + ";\n\n";
                    }
                }
            }
        }
    }
    else // create new field
    {
        wxString addCollate;
        statements += alterTable + "ADD \n" + colNameSql + " ";
        if (isIdentity) {
            statements += selDatatype + " GENERATED " + " BY DEFAULT "  + " AS IDENTITY "; // Todo: implemented ALWAYS
            if (!initialValue.IsEmpty())
                statements += "(START WITH " + initialValue + ")";
        }else
        if (newDomain)
        {
            statements += selDatatype;
            if (!dtSize.IsEmpty())
            {
                statements += "(" + dtSize;
                if (!dtScale.IsEmpty())
                    statements += "," + dtScale;
                statements += ")";
            }
            if (datatypes[n].isChar)
            {
                wxString charset = choice_charset->GetStringSelection();
                wxString collate = choice_collate->GetStringSelection();
                if (!charset.IsEmpty())
                {
                    statements += " CHARACTER SET " + charset;
                    if (!collate.IsEmpty())
                        addCollate = " COLLATE " + collate;
                }
            }
        }
        else
            statements += selDomain.getQuoted();

        if (!isNullable)
        {
            statements += " NOT NULL";
            update_not_null = unnAfter;
        }
        statements += addCollate + ";\n\n";
    }

    if (update_not_null != unnNone && !justCheck)
    {
        wxString s = ::wxGetTextFromUser(
            _("Enter value for existing fields containing NULL"),
            _("Update Existing NULL Values"), "", this);
        if (update_not_null == unnBefore)
        {
            wxString origColumnName = columnM->getQuotedName();
            statements = "UPDATE " + tableM->getQuotedName()
                + " \nSET " + origColumnName + " = '" + s
                + "' \nWHERE " + origColumnName + " IS NULL;\n"
                + statements;
        }
        else
        {
            statements = statements + "COMMIT;\n"
                + "UPDATE " + tableM->getQuotedName()
                + " \nSET " + colNameSql + " = '" + s
                + "' \nWHERE " + colNameSql + " IS NULL;\n";
        }
    }
    statements += textctrl_sql->GetValue();
    return !statements.IsEmpty();
}

const wxString FieldPropertiesDialog::getStatementsToExecute()
{
    wxString statements;
    getStatementsToExecute(statements, false);
    return statements;
}

void FieldPropertiesDialog::loadCharsets()
{
    wxWindowUpdateLocker freeze(choice_charset);

    choice_charset->Clear();
    choice_charset->Append("NONE");
    if (DatabasePtr db = tableM->getDatabase())
    {
        wxString stmt = "select rdb$character_set_name"
            " from rdb$character_sets order by 1";
        wxArrayString charsets(db->loadIdentifiers(stmt));
        charsets.Remove("NONE");
        choice_charset->Append(charsets);
    }
}

void FieldPropertiesDialog::loadCollations()
{
    wxWindowUpdateLocker freeze(choice_collate);

    choice_collate->Clear();
    if (DatabasePtr db = tableM->getDatabase())
    {
        wxString charset(choice_charset->GetStringSelection());
        choice_collate->Append(db->getCollations(charset));
    }
}

void FieldPropertiesDialog::loadDomains()
{
    wxWindowUpdateLocker freeze(choice_domain);

    choice_domain->Clear();
    if (columnM == 0 || !MetadataItem::hasSystemPrefix(columnM->getSource()))
    {
        choice_domain->Append(_("[Create new]"));
        if (!columnM)
            choice_domain->SetSelection(0);
    }

    if (DatabasePtr db = tableM->getDatabase())
    {
        if (columnM)
        {
            wxString name(columnM->getSource());
            if (!name.empty())
                choice_domain->Append(name);
        }

        DomainsPtr ds(db->getDomains());
        for (Domains::const_iterator it = ds->begin(); it != ds->end(); ++it)
            choice_domain->Append((*it)->getName_());
    }
}

void FieldPropertiesDialog::loadGeneratorNames()
{
    wxWindowUpdateLocker freeze(choice_generator);

    choice_generator->Clear();
    if (DatabasePtr db = tableM->getDatabase())
    {
        GeneratorsPtr gs(db->getGenerators());
        for (Generators::const_iterator it = gs->begin(); it != gs->end();
            ++it)
        {
            choice_generator->Append((*it)->getName_());
        }
    }
}

void FieldPropertiesDialog::setControlsProperties()
{
    // set dialog title
    wxString fmt;
    if (columnM)
        fmt = _("Table %s: Field Properties");
    else
        fmt = _("Table %s: Create New Field");
    SetTitle(wxString::Format(fmt, tableM->getName_().c_str()));

    // bold font for "Autoincrement" label
    wxFont font(label_autoinc->GetFont());
    font.MakeBold();
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

void FieldPropertiesDialog::updateColumnControls()
{
    if (columnM)
    {
        textctrl_fieldname->SetValue(columnM->getQuotedName());
        checkbox_notnull->SetValue(
            !columnM->isNullable(CheckDomainNullability));
        choice_domain->SetSelection(
            choice_domain->FindString(columnM->getSource()));
        updateDomainControls();
        loadCollations();
        choice_collate->SetSelection(
            choice_collate->FindString(columnM->getCollation()));
        if (tableM->getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(12.0)) {
            checkbox_identity->SetValue(columnM->isIdentity());
            if (columnM->isIdentity()) {
                textctrl_initialValue->SetValue(std::to_string(columnM->getInitialValue()));
            }
            else
                textctrl_initialValue->SetEditable(false);

        }
    }
    updateDatatypeInfo();
}

void FieldPropertiesDialog::updateControls()
{
    button_ok->Enable(!textctrl_fieldname->GetValue().IsEmpty());

    wxString genName;
    if (tableM)
        genName = "GEN_" + tableM->getName_() + "_ID";
    textctrl_generator_name->SetValue(genName);

    loadGeneratorNames();
    loadCharsets();
    loadDomains();
    updateColumnControls();
}

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
    if (tableM->getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(12.0)) {
        checkbox_identity->Enable(datatypes[n].identity);
        textctrl_initialValue->SetEditable(datatypes[n].identity);
        //textctrl_incrementalValue->SetEditable(datatypes[n].identity);
    }

    updateColors();
}

void FieldPropertiesDialog::updateDomainControls()
{
    wxString domain = choice_domain->GetStringSelection();
    bool newDomain = getIsNewDomainSelected();
    if (!newDomain)
        updateDomainInfo(domain);

    // data type, size, scale and collate are not editable for all other
    // already existing domains
    bool allowEdit = (newDomain || domain.Mid(0, 4) == "RDB$");
    choice_datatype->Enable(allowEdit);
    if (allowEdit)
        updateDatatypeInfo();
    else
    {
        textctrl_size->SetEditable(false);
        textctrl_scale->SetEditable(false);
        choice_collate->Enable(false);
        updateColors();
        // charset is editable only for new fields with new domain
        choice_charset->Enable(columnM == 0 && newDomain);
    }
}

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

void FieldPropertiesDialog::updateSqlStatement()
{
    wxString fNameSql(Identifier::userString(textctrl_fieldname->GetValue()));
    Identifier generator(choice_generator->GetStringSelection());

    wxString sql;
    if (radio_generator_new->GetValue())
    {
        generator.setText(textctrl_generator_name->GetValue());
        sql = "CREATE GENERATOR " + generator.getQuoted() + ";\n\n";
    }

    if (checkbox_trigger->IsChecked())
    {
        // TODO: we could use BIGINT for FB 1.5 and above
        Identifier triggername(tableM->getName_() + "_BI");
        sql += "SET TERM !! ;\n";
        sql += "CREATE TRIGGER " + triggername.getQuoted() + " FOR "
            + tableM->getQuotedName() + "\n"
            + "ACTIVE BEFORE INSERT POSITION 0\nAS\n"
            + "DECLARE VARIABLE tmp DECIMAL(18,0);\nBEGIN\n"
            + "  IF (NEW." + fNameSql + " IS NULL) THEN\n"
            + "    NEW." + fNameSql + " = GEN_ID("
            + generator.getQuoted() + ", 1);\n"
            + "  ELSE\n  BEGIN\n    tmp = GEN_ID("
            + generator.getQuoted() + ", 0);\n    if (tmp < new."
            + fNameSql + ") then\n      tmp = GEN_ID("
            + generator.getQuoted() + ", new." + fNameSql
            + "-tmp);\n  END\nEND!!\n";
        sql += "SET TERM ; !!\n";
    }

    textctrl_sql->SetValue(sql);
}

//! event handling
BEGIN_EVENT_TABLE(FieldPropertiesDialog, BaseDialog)
    EVT_BUTTON(FieldPropertiesDialog::ID_button_edit_domain,
        FieldPropertiesDialog::OnButtonEditDomainClick)
    EVT_BUTTON(wxID_OK, FieldPropertiesDialog::OnButtonOkClick)
    EVT_CHECKBOX(FieldPropertiesDialog::ID_checkbox_trigger,
        FieldPropertiesDialog::OnNeedsUpdateSql)
    EVT_CHOICE(FieldPropertiesDialog::ID_choice_charset,
        FieldPropertiesDialog::OnChoiceCharsetClick)
    EVT_CHOICE(FieldPropertiesDialog::ID_choice_datatype,
        FieldPropertiesDialog::OnChoiceDatatypeClick)
    EVT_CHOICE(FieldPropertiesDialog::ID_choice_domain,
        FieldPropertiesDialog::OnChoiceDomainClick)
    EVT_CHOICE(FieldPropertiesDialog::ID_choice_generator,
        FieldPropertiesDialog::OnNeedsUpdateSql)
    EVT_RADIOBUTTON(FieldPropertiesDialog::ID_radio_generator_existing,
        FieldPropertiesDialog::OnRadioGeneratorClick)
    EVT_RADIOBUTTON(FieldPropertiesDialog::ID_radio_generator_new,
        FieldPropertiesDialog::OnRadioGeneratorClick)
    EVT_TEXT(FieldPropertiesDialog::ID_textctrl_fieldname,
        FieldPropertiesDialog::OnTextFieldnameUpdate)
    EVT_TEXT(FieldPropertiesDialog::ID_textctrl_generator_name,
        FieldPropertiesDialog::OnNeedsUpdateSql)
    EVT_CHECKBOX(FieldPropertiesDialog::ID_checkbox_identity,
        FieldPropertiesDialog::OnCheckBoxidentityClick)
    END_EVENT_TABLE()

void FieldPropertiesDialog::OnButtonEditDomainClick(wxCommandEvent&
    WXUNUSED(event))
{
    if (getIsNewDomainSelected())
    {
        // no domain selected
        return;
    }

    // TODO:
    // create DomainPropertiesFrame & show it
    // when done, reload domain definition
    // updateDomainInfo(wx2std(cb_domains->GetValue()));

    // Currently we just open the SQL editor and close this dialog
    wxString domain = choice_domain->GetStringSelection();
    if (DatabasePtr db = tableM->getDatabase())
    {
        Domain* d = dynamic_cast<Domain*>(db->findByNameAndType(ntDomain,
            domain));
        if (d)
        {
            showSql(GetParent(), _("Alter domain"), db,
                d->getAlterSqlTemplate());
            EndModal(wxID_CANCEL);
        }
    }
}

void FieldPropertiesDialog::OnButtonOkClick(wxCommandEvent& WXUNUSED(event))
{
    updateSqlStatement();
    wxString statements;
    if (getStatementsToExecute(statements, true))
        EndModal(wxID_OK);
}

wxString FieldPropertiesDialog::getStatementTitle() const
{
    if (columnM)
        return _("Executing Field Modification Script");
    else
        return _("Executing Field Creation Script");
}

void FieldPropertiesDialog::OnChoiceCharsetClick(wxCommandEvent&
    WXUNUSED(event))
{
    wxString oldCol(choice_collate->GetStringSelection());
    loadCollations();
    choice_collate->SetSelection(choice_collate->FindString(oldCol));
}

void FieldPropertiesDialog::OnChoiceDatatypeClick(wxCommandEvent&
    WXUNUSED(event))
{
    updateDatatypeInfo();
}

void FieldPropertiesDialog::OnChoiceDomainClick(wxCommandEvent&
    WXUNUSED(event))
{
    updateDomainControls();
}

void FieldPropertiesDialog::OnNeedsUpdateSql(wxCommandEvent& WXUNUSED(event))
{
    updateSqlStatement();
}

void FieldPropertiesDialog::OnRadioGeneratorClick(wxCommandEvent&
    WXUNUSED(event))
{
    textctrl_generator_name->SetEditable(radio_generator_new->GetValue());
    updateColors();
    choice_generator->Enable(radio_generator_existing->GetValue());
    updateSqlStatement();
}

void FieldPropertiesDialog::OnTextFieldnameUpdate(wxCommandEvent&
    WXUNUSED(event))
{
    button_ok->Enable(!textctrl_fieldname->GetValue().IsEmpty());
    updateSqlStatement();
}

void FieldPropertiesDialog::OnCheckBoxidentityClick(wxCommandEvent& WXUNUSED(event))
{
    textctrl_initialValue->SetEditable(checkbox_identity->IsChecked());
    checkbox_notnull->SetValue(checkbox_identity->IsChecked());
}

class ColumnPropertiesHandler: public URIHandler,
    private MetadataItemURIHandlerHelper, private GUIURIHandlerHelper
{
public:
    ColumnPropertiesHandler() {};
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const ColumnPropertiesHandler handlerInstance;
};

const ColumnPropertiesHandler ColumnPropertiesHandler::handlerInstance;

bool ColumnPropertiesHandler::handleURI(URI& uri)
{
    bool addField = uri.action == "add_field";
    bool editField = uri.action == "edit_field";
    if (!addField && !editField)
        return false;

    wxWindow* w = getParentWindow(uri);
    MetadataItem* mo = extractMetadataItemFromURI<MetadataItem>(uri);
    if (!mo || !w)
        return true;

    Column* c = 0;
    Table* t;
    if (addField)
        t = (Table*)mo;
    else
    {
        c = (Column*)mo;
        t = c->getTable();
    }

    wxString statements, title;
    {   // we want FPD to go out of scope and get destroyed since the action in
        // ESF can destroy the field, and take down the FPD. Accessing FPD in turn
        // leads to mysterious crash
        FieldPropertiesDialog fpd(w, t, c);
        // NOTE: this has been moved here from OnOkButtonClick() to make frame
        //       activation work properly.  Basically activation of another
        //       frame has to happen outside wxDialog::ShowModal(), because it
        //       does at the end re-focus the last focused control, raising
        //       the parent frame over the newly created sql execution frame
        if (fpd.ShowModal() == wxID_OK)
        {
            statements = fpd.getStatementsToExecute();
            title = fpd.getStatementTitle();
        }
    }

    if (!statements.IsEmpty())
        execSql(w, title, t->getDatabase(), statements, true);
    return true;
}

