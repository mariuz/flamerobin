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

  Contributor(s): Nando Dessena
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

#include "ExecuteSqlFrame.h"
#include "FieldPropertiesFrame.h"
#include "images.h"
#include "metadata/database.h"
#include "ugly.h"
#include "urihandler.h"

//-----------------------------------------------------------------------------
FieldPropertiesFrame::FieldPropertiesFrame(wxWindow* parent, int id, wxString title, Table *table, const wxPoint& pos, const wxSize& size, long style):
    BaseFrame(parent, id, title, pos, size, style)
{
    tableM = table;
    if (table)
        table->attachObserver(this);
    fieldM = 0;

    // begin wxGlade: FieldPropertiesFrame::FieldPropertiesFrame
    panel_1 = new wxPanel(this, -1);
    label_8 = new wxStaticText(panel_1, -1, _("Field"));
    textctrl_fieldname = new wxTextCtrl(panel_1, ID_textctrl_fieldname, wxT(""));
    label_1 = new wxStaticText(panel_1, -1, _("Domain"));
    const wxString ch_domains_choices[] = {
        wxT(""),    // ugly hack. The control doesn't want to show big list for selection if initial list is short
        wxT(""),    //            so it looks really ugly on the screen
        wxT(""),
        wxT(""),
        wxT(""),
        wxT(""),
        wxT(""),
        wxT(""),
        wxT(""),
        wxT("")
    };

    ch_domains = new wxChoice(panel_1, ID_ch_domains, wxDefaultPosition, wxDefaultSize, 10, ch_domains_choices);
    button_edit_domain = new wxButton(panel_1, ID_button_edit_domain, _("Edit domain"));
    label_2 = new wxStaticText(panel_1, -1, _("Datatype"));
    const wxString ch_datatypes_choices[] = {
        wxT("Char"),
        wxT("Varchar"),
        wxT("Integer"),
        wxT("Smallint"),
        wxT("Numeric"),
        wxT("Decimal"),
        wxT("Float"),
        wxT("Double precision"),
        wxT("Date"),
        wxT("Time"),
        wxT("Timestamp"),
        wxT("Array"),
        wxT("Blob")
    };

    ch_datatypes = new wxChoice(panel_1, ID_ch_datatypes, wxDefaultPosition, wxDefaultSize, 13, ch_datatypes_choices);
    label_3 = new wxStaticText(panel_1, -1, _("Size"));
    textctrl_size = new wxTextCtrl(panel_1, -1, wxT(""));
    label_4 = new wxStaticText(panel_1, -1, _("Scale"));
    textctrl_scale = new wxTextCtrl(panel_1, -1, wxT(""));
    cb_notnull = new wxCheckBox(panel_1, -1, _("Not null"));
    label_5 = new wxStaticText(panel_1, -1, _("Charset"));
    const wxString ch_charset_choices[] = {
        wxT("NONE")
    };
    ch_charset = new wxChoice(panel_1, ID_ch_charset, wxDefaultPosition, wxDefaultSize, 1, ch_charset_choices);
    Database *d = table->getDatabase();
    if (d)
    {
        std::vector<std::string> charsets;
        d->fillVector(charsets, "select rdb$character_set_name from rdb$character_sets order by 1");
        for (std::vector<std::string>::iterator it = charsets.begin(); it != charsets.end(); ++it)
        {
            if ((*it) == "NONE")
                continue;
            ch_charset->Append(std2wx(*it));
        }
    }
    
    label_6 = new wxStaticText(panel_1, -1, _("Collate"));
    const wxString ch_collate_choices[] = {
        wxT(""),    // ugly hack. The control doesn't want to show big list for selection if initial list is short
        wxT(""),    //            so it looks really ugly on the screen
        wxT(""),
        wxT(""),
        wxT(""),
        wxT(""),
        wxT(""),
        wxT(""),
        wxT(""),
        wxT("")
    };
    ch_collate = new wxChoice(panel_1, -1, wxDefaultPosition, wxDefaultSize, 10, ch_collate_choices);
    static_line_1 = new wxStaticLine(panel_1, -1);
    label_7 = new wxStaticText(panel_1, -1, _("Autoincrement"));
    radio_new = new wxRadioButton(panel_1, ID_radio_new, _("Create new generator, named: "));
    textctrl_generatorname = new wxTextCtrl(panel_1, ID_textctrl_generatorname, wxT(""));
    radio_existing = new wxRadioButton(panel_1, ID_radio_existing, _("Use existing generator"));
    const wxString ch_generators_choices[] = {
        wxT(""),    // ugly hack. The control doesn't want to show big list for selection if initial list is short
        wxT(""),    //            so it looks really ugly on the screen
        wxT(""),
        wxT(""),
        wxT(""),
        wxT(""),
        wxT(""),
        wxT(""),
        wxT(""),
        wxT("")
    };
    ch_generators = new wxChoice(panel_1, ID_ch_generators, wxDefaultPosition, wxDefaultSize, 10, ch_generators_choices);
    cb_trigger = new wxCheckBox(panel_1, ID_cb_trigger, _("Create trigger"));
    textctrl_sql = new wxTextCtrl(panel_1, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    button_ok = new wxButton(panel_1, ID_button_ok, _("OK"));
    button_cancel = new wxButton(panel_1, ID_button_cancel, _("Cancel"));

    set_properties();
    do_layout();
    // end wxGlade

    wxBitmap bmp = getImage32(ntColumn);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);

    button_cancel->SetFocus();
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::set_properties()
{
    // begin wxGlade: FieldPropertiesFrame::set_properties
    SetSize(wxSize(397, 386));
    ch_domains->SetSelection(0);
    ch_datatypes->SetSelection(0);
    ch_charset->SetSelection(0);
    label_7->SetFont(wxFont(9, wxDEFAULT, wxNORMAL, wxBOLD, 0, wxT("")));
    radio_existing->SetValue(1);
    ch_generators->SetSelection(0);
    textctrl_sql->SetSize(wxSize(383, 150));
    // end wxGlade
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::do_layout()
{
    // begin wxGlade: FieldPropertiesFrame::do_layout
    wxBoxSizer* sizer_1 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* sizer_2 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* sizer_3 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_8 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_9 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_10 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_7 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_6 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_5 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* sizer_4 = new wxBoxSizer(wxHORIZONTAL);
    sizer_4->Add(label_8, 0, wxALL|wxALIGN_CENTER_VERTICAL, 4);
    sizer_4->Add(textctrl_fieldname, 1, wxALL, 3);
    sizer_2->Add(sizer_4, 0, wxALL|wxEXPAND, 2);
    sizer_5->Add(label_1, 0, wxALL, 4);
    sizer_5->Add(ch_domains, 0, wxALL, 1);
    sizer_5->Add(button_edit_domain, 0, wxLEFT, 2);
    sizer_2->Add(sizer_5, 0, wxALL, 2);
    sizer_6->Add(label_2, 0, wxALL, 4);
    sizer_6->Add(ch_datatypes, 0, 0, 0);
    sizer_6->Add(label_3, 0, wxALL, 4);
    sizer_6->Add(textctrl_size, 0, 0, 0);
    sizer_6->Add(label_4, 0, wxALL, 4);
    sizer_6->Add(textctrl_scale, 0, 0, 0);
    sizer_2->Add(sizer_6, 0, wxALL, 2);
    sizer_7->Add(cb_notnull, 0, wxALL, 4);
    sizer_7->Add(35, 10, 0, 0, 0);
    sizer_7->Add(label_5, 0, wxALL, 4);
    sizer_7->Add(ch_charset, 0, wxRIGHT, 10);
    sizer_7->Add(label_6, 0, wxALL, 4);
    sizer_7->Add(ch_collate, 0, 0, 0);
    sizer_2->Add(sizer_7, 0, wxALL, 2);
    sizer_2->Add(static_line_1, 0, wxALL|wxEXPAND, 5);
    sizer_2->Add(label_7, 0, wxALL, 4);
    sizer_10->Add(radio_new, 0, wxALL, 3);
    sizer_10->Add(textctrl_generatorname, 1, 0, 0);
    sizer_2->Add(sizer_10, 0, wxALL|wxEXPAND, 2);
    sizer_9->Add(radio_existing, 0, wxALL, 3);
    sizer_9->Add(ch_generators, 1, 0, 0);
    sizer_2->Add(sizer_9, 0, wxALL|wxEXPAND, 2);
    sizer_8->Add(cb_trigger, 0, wxLEFT, 3);
    sizer_2->Add(sizer_8, 0, wxALL, 2);
    sizer_2->Add(textctrl_sql, 1, wxALL|wxEXPAND, 3);
    sizer_3->Add(button_ok, 0, wxALL, 3);
    sizer_3->Add(button_cancel, 0, wxALL, 3);
    sizer_2->Add(sizer_3, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    panel_1->SetAutoLayout(true);
    panel_1->SetSizer(sizer_2);
    sizer_2->Fit(panel_1);
    sizer_2->SetSizeHints(panel_1);
    sizer_1->Add(panel_1, 1, wxEXPAND, 0);
    SetAutoLayout(true);
    SetSizer(sizer_1);
    sizer_1->Fit(this);
    sizer_1->SetSizeHints(this);
    Layout();
    // end wxGlade
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(FieldPropertiesFrame, wxFrame)
    EVT_BUTTON(FieldPropertiesFrame::ID_button_edit_domain, FieldPropertiesFrame::OnButtonEditDomainClick)
    EVT_BUTTON(FieldPropertiesFrame::ID_button_ok,          FieldPropertiesFrame::OnButtonOkClick)
    EVT_BUTTON(FieldPropertiesFrame::ID_button_cancel,      FieldPropertiesFrame::OnButtonCancelClick)
    EVT_RADIOBUTTON(FieldPropertiesFrame::ID_radio_new,      FieldPropertiesFrame::OnRadioNewClick)
    EVT_RADIOBUTTON(FieldPropertiesFrame::ID_radio_existing, FieldPropertiesFrame::OnRadioExistingClick)
    EVT_TEXT(FieldPropertiesFrame::ID_textctrl_generatorname, FieldPropertiesFrame::OnTextctrlGeneratornameChange)
    EVT_TEXT(FieldPropertiesFrame::ID_textctrl_fieldname,     FieldPropertiesFrame::OnTextctrlFieldnameChange)
    EVT_CHOICE(FieldPropertiesFrame::ID_ch_generators, FieldPropertiesFrame::OnChGeneratorsClick)
    EVT_CHOICE(FieldPropertiesFrame::ID_ch_domains,    FieldPropertiesFrame::OnChDomainsClick)
    EVT_CHOICE(FieldPropertiesFrame::ID_ch_charset,    FieldPropertiesFrame::OnChCharsetClick)
    EVT_CHOICE(FieldPropertiesFrame::ID_ch_datatypes,  FieldPropertiesFrame::OnChDatatypesClick)
    EVT_CHECKBOX(FieldPropertiesFrame::ID_cb_trigger, FieldPropertiesFrame::OnCbTriggerClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::OnButtonEditDomainClick(wxCommandEvent& WXUNUSED(event))
{
    // create DomainPropertiesFrame & show it
    // when done, reload domain definition
    //updateDomainInfo(wx2std(cb_domains->GetValue()));
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::OnRadioNewClick(wxCommandEvent& WXUNUSED(event))
{
    updateSqlWindow();
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::OnTextctrlGeneratornameChange(wxCommandEvent& WXUNUSED(event))
{
    updateSqlWindow();
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::OnTextctrlFieldnameChange(wxCommandEvent& WXUNUSED(event))
{
    updateSqlWindow();
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::OnRadioExistingClick(wxCommandEvent& WXUNUSED(event))
{
    updateSqlWindow();
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::OnChGeneratorsClick(wxCommandEvent& WXUNUSED(event))
{
    updateSqlWindow();
}
//-----------------------------------------------------------------------------
//! change collation list
void FieldPropertiesFrame::OnChCharsetClick(wxCommandEvent& WXUNUSED(event))
{
    loadCollations(wx2std(ch_collate->GetStringSelection()));
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::loadCollations(std::string desired)
{
    Database *d = tableM->getDatabase();    // get list of collations from Ydatabase
    if (!d)
        return;

    std::vector<std::string> list = d->getCollations(wx2std(ch_charset->GetStringSelection()));
    int to_select = 0;
    int counter = 0;
    ch_collate->Clear();
    for (std::vector<std::string>::iterator it = list.begin(); it != list.end(); ++it)
    {
        ch_collate->Append(std2wx(*it));
        if ((*it) == desired)
            to_select = counter;
        counter++;
    }

    ch_collate->SetSelection(to_select);
}
//-----------------------------------------------------------------------------
//! enable/disable datatype properties
void FieldPropertiesFrame::OnChDatatypesClick(wxCommandEvent& WXUNUSED(event))
{
    updateEditBoxes();
}
//-----------------------------------------------------------------------------
//! enable/disable datatype properties
void FieldPropertiesFrame::OnChDomainsClick(wxCommandEvent& WXUNUSED(event))
{
    wxString domain = ch_domains->GetStringSelection();
    if (domain != wxT("[new]"))
        updateDomainInfo(wx2std(ch_domains->GetStringSelection()));
    updateEditBoxes();

    bool allowEdit = (domain == wxT("[new]") || domain.Mid(0, 4) == wxT("RDB$"));
    ch_datatypes->Enable(allowEdit);
    textctrl_size->Enable(allowEdit);
    textctrl_scale->Enable(allowEdit);

    ch_charset->Enable(fieldM == 0 && domain == wxT("[new]"));  // only for new fields with new domain
}
//-----------------------------------------------------------------------------
bool datatypeHasSize(const wxString& type)
{
    return (type == wxT("Char") || type == wxT("Varchar") || type == wxT("Numeric") || type == wxT("Decimal"));
}
//-----------------------------------------------------------------------------
bool datatypeHasScale(const wxString& type)
{
    return (type == wxT("Numeric") || type == wxT("Decimal"));
}
//-----------------------------------------------------------------------------
bool datatypeHasCollate(const wxString& type)
{
    return (type == wxT("Char") || type == wxT("Varchar"));
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::updateEditBoxes()
{
    wxString type = ch_datatypes->GetStringSelection();
    textctrl_size->Enable(datatypeHasSize(type));
    textctrl_scale->Enable(datatypeHasScale(type));
    ch_collate->Enable(fieldM == 0 && datatypeHasCollate(type));
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::OnCbTriggerClick(wxCommandEvent& WXUNUSED(event))
{
    updateSqlWindow();
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::OnButtonOkClick(wxCommandEvent& WXUNUSED(event))
{
    updateSqlWindow();  // just in case, so we can copy from it

    wxString selectedDomain = ch_domains->GetStringSelection();
    wxString selectedDatatype = ch_datatypes->GetStringSelection();
    wxString tsize = textctrl_size->GetValue();
    wxString tscale = textctrl_scale->GetValue();
    if (!datatypeHasSize(selectedDatatype))
        tsize = wxEmptyString;
    if (!datatypeHasScale(selectedDatatype))
        tscale = wxEmptyString;
    wxString fieldName = textctrl_fieldname->GetValue();

    std::string sql;
    enum unn { unnNone, unnBefore, unnAfter } update_not_null = unnNone;
    if (fieldM)         // detect changes and do appropriate SQL actions
    {
        if (std2wx(fieldM->getName()) != fieldName)     // field name changed
            sql += "ALTER TABLE " + tableM->getName() + " ALTER "
                + fieldM->getName() + " TO " + wx2std(fieldName) + ";\n\n";

        std::string type, size, scale, charset;
        if (!getDomainInfo(fieldM->getSource(), type, size, scale, charset))
        {
            ::wxMessageBox(_("Cannot get info for domain. Aborting."), _("Warning!"), wxICON_WARNING);
            return;
        }

        // changed domain
        if (std2wx(fieldM->getSource()) != selectedDomain && selectedDomain != wxT("[new]"))
        {
            sql += "ALTER TABLE " + tableM->getName() + " ALTER " + wx2std(fieldName) + " TYPE "
                + wx2std(selectedDomain) + ";\n\n";
        }
        else if (selectedDomain == wxT("[new]")     // user domain -> autogenerated domain
            || std2wx(type) != selectedDatatype     // OR changed datatype, size or scale
            || std2wx(size) != tsize
            || std2wx(scale) != tscale)
        {
            sql += "ALTER TABLE " + tableM->getName() + " ALTER " + wx2std(fieldName) + " TYPE ";
            sql += wx2std(selectedDatatype);
            if (!tsize.IsEmpty())
            {
                sql += "(" + wx2std(tsize);
                if (!tscale.IsEmpty())
                    sql += "," + wx2std(tscale);
                sql += ")";
            }
            sql += ";\n\n";
        }

        // check for null option
        if (cb_notnull->IsChecked() == fieldM->isNullable())    // watch for double negation!
        {
            if (cb_notnull->IsChecked())    // change from NULL to NOT NULL
                update_not_null = unnBefore;

            sql += "UPDATE RDB$RELATION_FIELDS SET RDB$NULL_FLAG = ";
            if (cb_notnull->IsChecked())
                sql += "1";
            else
                sql += "NULL";
            sql += " \nWHERE RDB$FIELD_NAME = '" + wx2std(fieldName)
                + "' AND RDB$RELATION_NAME = '" + tableM->getName() + "';\n\n";
        }
    }
    else    // new field
    {
        sql += "ALTER TABLE " + tableM->getName() + " ADD \n" + wx2std(fieldName) + " ";
        if (selectedDomain == _("[new]"))
        {
            sql += wx2std(selectedDatatype);
            if (!tsize.IsEmpty())
            {
                sql += "(" + wx2std(tsize);
                if (!tscale.IsEmpty())
                    sql += "," + wx2std(tscale);
                sql += ")";
            }
        }
        else
            sql += wx2std(selectedDomain);

        if (cb_notnull->IsChecked())
        {
            sql += " not null";
            update_not_null = unnAfter;
        }
        sql += ";\n\n";
    }

    if (update_not_null != unnNone)
    {
        wxString s = ::wxGetTextFromUser(_("Enter value for existing fields with NULLs"),
            _("NOT NULL field"), wxT(""), this);
        std::string sqladd = "UPDATE " + tableM->getName() + " \nSET "
            + wx2std(fieldName) + " = '" + wx2std(s) + "' \nWHERE "
            + wx2std(fieldName) + " IS NULL;\n";
        if (update_not_null == unnBefore)
            sql = sqladd + sql;
        else
            sql += "COMMIT;\n" + sqladd;
    }

    wxString wxsql = std2wx(sql);
    wxsql += textctrl_sql->GetValue();  // execute autoinc sql (gen+trigger),

    ExecuteSqlFrame *eff = 0;
    if (!wxsql.Trim().IsEmpty())
    {
        eff = new ExecuteSqlFrame(GetParent(), -1, _("Executing change script"));
        eff->setDatabase(tableM->getDatabase());
    }

    Close();

    if (eff)            // this piece of code has to be after Close()
    {                   // otherwise ExecuteSqlFrame is shown underneath the html-properties window
        eff->Show();
        eff->Raise();   // make sure it's on top (otherwise it gets hidden behind)
        eff->setSql(wxsql);
        eff->executeAllStatements(true);        // true = user must commit/rollback + frame is closed at once
    }
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::OnButtonCancelClick(wxCommandEvent& WXUNUSED(event))
{
    Close();
}
//-----------------------------------------------------------------------------
//! setup visual controls
void FieldPropertiesFrame::setProperties()
{
    if (!tableM)
        return;

    std::string genname = "GEN_" + tableM->getName() + "_ID";
    textctrl_generatorname->SetValue(std2wx(genname));

    Database *d = dynamic_cast<Database *>(tableM->getDatabase());
    if (!d)
        return;
    ch_generators->Clear();
    for (MetadataCollection<Generator>::const_iterator it = d->generatorsBegin(); it != d->generatorsEnd(); ++it)
        ch_generators->Append(std2wx((*it).getName()));
    ch_domains->Clear();
    if (fieldM == 0 || fieldM->getSource().substr(0,4) != "RDB$")
    {
        ch_domains->Append(wxT("[new]"));
        if (fieldM == 0)
            ch_domains->SetSelection(0);
    }
    for (MetadataCollection<Domain>::const_iterator it = d->domainsBegin(); it != d->domainsEnd(); ++it)
    {
        std::string name = (*it).getName();
        if (name.substr(0, 4) == "RDB$" && (!fieldM || fieldM->getSource() != name))
            continue;
        ch_domains->Append(std2wx(name));
    }

    if (fieldM)
    {
        // editing existing field...
        textctrl_fieldname->SetValue(std2wx(fieldM->getName()));
        cb_notnull->SetValue(!fieldM->isNullable());
        ch_domains->SetSelection(ch_domains->FindString(std2wx(fieldM->getSource())));

        wxCommandEvent dummy;
        OnChDomainsClick(dummy);    // loads list of domains
        loadCollations(fieldM->getCollation());
    }
    updateEditBoxes();
}
//-----------------------------------------------------------------------------
bool FieldPropertiesFrame::getDomainInfo(std::string domain, std::string& type, std::string& size,
    std::string& scale, std::string& charset)
{
    Database *db = dynamic_cast<Database *>(tableM->getDatabase());
    if (!db)
        return false;
    for (MetadataCollection<Domain>::const_iterator it = db->domainsBegin(); it != db->domainsEnd(); ++it)
    {
        if (domain == (*it).getName())
        {
            Domain *d = (Domain *)&(*it);
            d->getDatatypeParts(type,size,scale);
            charset = d->getCharset();
            return true;
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::updateDomainInfo(std::string domain)
{
    std::string type, size, scale, charset;
    if (!getDomainInfo(domain, type, size, scale, charset))
        return;
    textctrl_scale->SetValue(std2wx(scale));
    textctrl_size->SetValue(std2wx(size));
    ch_datatypes->SetSelection(ch_datatypes->FindString(std2wx(type)));
    ch_charset->SetSelection(ch_charset->FindString(std2wx(charset)));
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::update()
{
    setProperties();
}
//-----------------------------------------------------------------------------
//! closes window if field is removed (dropped/table dropped/disconnected,etc.)
void FieldPropertiesFrame::removeSubject(Subject* subject)
{
    Observer::removeSubject(subject);
    if (subject == fieldM)
        Close();
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::updateSqlWindow()
{
    std::string table = tableM->getName();
    std::string field = wx2std(textctrl_fieldname->GetValue());
    std::string sql;
    std::string generator = wx2std(ch_generators->GetStringSelection());
    if (radio_new->GetValue())
    {
        generator = wx2std(textctrl_generatorname->GetValue());
        sql = "CREATE GENERATOR " + generator + ";\n\n";
    }

    if (cb_trigger->IsChecked())
    {
        sql += "SET TERM !! ;\n";
        sql += "CREATE TRIGGER " + table + "_BI FOR " + table + "\n";
        sql += "ACTIVE BEFORE INSERT POSITION 0\nAS\nBEGIN\n";
        sql += "  IF (NEW." + field + " IS NULL) THEN\n";
        sql += "    NEW." + field + " = GEN_ID(" + generator + ",1);\n";
        sql += "END!!\n";
        sql += "SET TERM ; !!\n";
    }

    textctrl_sql->SetValue(std2wx(sql));
}
//-----------------------------------------------------------------------------
void FieldPropertiesFrame::setField(Column *field)
{
    fieldM = field;
    if (field)
    {
        field->attachObserver(this);
        setProperties();
    }
}
//-----------------------------------------------------------------------------
const std::string FieldPropertiesFrame::getName() const
{
    return "FieldPropertiesFrame";
}
//-----------------------------------------------------------------------------
class ColumnPropertiesHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    // singleton; registers itself on creation.
    static const ColumnPropertiesHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const ColumnPropertiesHandler ColumnPropertiesHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool ColumnPropertiesHandler::handleURI(URI& uri)
{
    if (uri.action != "edit_field" && uri.action != "add_field")
        return false;

    wxWindow *w = getWindow(uri);
    void *mo = getObject(uri);
    if (!mo || !w)
        return true;

    Column *c = 0;
    Table *t;
    if (uri.action == "add_field")
        t = (Table *)mo;
    else
    {
        c = (Column *)mo;
        t = (Table *)c->getParent();
    }

    FieldPropertiesFrame *f = new FieldPropertiesFrame(w, -1, wxString::Format(_("TABLE: %s"), std2wx(t->getName()).c_str()), t);
    if (c)
        f->setField(c);
    else
        f->setProperties();
    f->Show();
    return true;
}
//-----------------------------------------------------------------------------
