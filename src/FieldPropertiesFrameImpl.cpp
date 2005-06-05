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

#include <string>
#include "metadata/collection.h"
#include "metadata/generator.h"
#include "metadata/database.h"
#include "ugly.h"
#include "dberror.h"
#include "gui/ExecuteSqlFrame.h"
#include "FieldPropertiesFrame.h"
#include "urihandler.h"
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
	YDatabase *d = tableM->getDatabase();	// get list of collations from Ydatabase
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

	ch_charset->Enable(fieldM == 0 && domain == wxT("[new]"));	// only for new fields with new domain
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
	updateSqlWindow();	// just in case, so we can copy from it

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
	if (fieldM)			// detect changes and do appropriate SQL actions
	{
		if (std2wx(fieldM->getName()) != fieldName)		// field name changed
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
		else if (selectedDomain == wxT("[new]")		// user domain -> autogenerated domain
			|| std2wx(type) != selectedDatatype		// OR changed datatype, size or scale
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
		if (cb_notnull->IsChecked() == fieldM->isNullable())	// watch for double negation!
		{
			if (cb_notnull->IsChecked())	// change from NULL to NOT NULL
			{
				wxString s = ::wxGetTextFromUser(_("Enter value for existing columns with NULLs"),
					_("Changing field from NULL to NOT NULL"), wxT(""), this);
				sql += "UPDATE " + tableM->getName() + " \nSET "
					+ wx2std(fieldName) + " = '" + wx2std(s) + "' \nWHERE "
					+ wx2std(fieldName) + " IS NULL;\n";
			}

			sql += "UPDATE RDB$RELATION_FIELDS SET RDB$NULL_FLAG = ";
			if (cb_notnull->IsChecked())
				sql += "1";
			else
				sql += "NULL";
			sql += " \nWHERE RDB$FIELD_NAME = '" + wx2std(fieldName)
				+ "' AND RDB$RELATION_NAME = '" + tableM->getName() + "';\n\n";
		}
	}
	else	// new field
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
			sql += " not null";
		sql += ";\n\n";
	}

	wxString wxsql = std2wx(sql);
	wxsql += textctrl_sql->GetValue();	// execute autoinc sql (gen+trigger),

	ExecuteSqlFrame *eff = 0;
	if (!wxsql.Trim().IsEmpty())
	{
		// create ExecuteSqlFrame with option to close at once
		eff = new ExecuteSqlFrame(GetParent(), -1, _("Executing change script"));
		eff->setDatabase(tableM->getDatabase());
	}

	Close();

	if (eff)			// this piece of code has to be after Close()
	{					// otherwise ExecuteSqlFrame is shown underneath the html-properties window
		eff->Show();
		eff->Raise();	// make sure it's on top (otherwise it gets hidden behind)
		eff->setSql(wxsql);
		eff->executeAllStatements(true);		// true = user must commit/rollback + frame is closed at once
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

	YDatabase *d = dynamic_cast<YDatabase *>(tableM->getDatabase());
	if (!d)
		return;
	ch_generators->Clear();
	for (YMetadataCollection<YGenerator>::const_iterator it = d->generatorsBegin(); it != d->generatorsEnd(); ++it)
		ch_generators->Append(std2wx((*it).getName()));
	ch_domains->Clear();
	if (fieldM == 0 || fieldM->getSource().substr(0,4) != "RDB$")
	{
		ch_domains->Append(wxT("[new]"));
		if (fieldM == 0)
			ch_domains->SetSelection(0);
	}
	for (YMetadataCollection<YDomain>::const_iterator it = d->domainsBegin(); it != d->domainsEnd(); ++it)
	{
		std::string name = (*it).getName();
		if (!fieldM && name.substr(0, 4) == "RDB$")		// when new column is added to the table
			continue;									// only offer user-created domains
		ch_domains->Append(std2wx(name));
	}

	if (fieldM)
	{
		// editing existing field...
		textctrl_fieldname->SetValue(std2wx(fieldM->getName()));
		cb_notnull->SetValue(!fieldM->isNullable());
		ch_domains->SetSelection(ch_domains->FindString(std2wx(fieldM->getSource())));

		wxCommandEvent dummy;
		OnChDomainsClick(dummy);	// loads list of domains
		loadCollations(fieldM->getCollation());
	}
	updateEditBoxes();
}
//-----------------------------------------------------------------------------
bool FieldPropertiesFrame::getDomainInfo(std::string domain, std::string& type, std::string& size,
	std::string& scale, std::string& charset)
{
	YDatabase *db = dynamic_cast<YDatabase *>(tableM->getDatabase());
	if (!db)
		return false;
	for (YMetadataCollection<YDomain>::const_iterator it = db->domainsBegin(); it != db->domainsEnd(); ++it)
	{
		if (domain == (*it).getName())
		{
			YDomain *d = (YDomain *)&(*it);
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
void FieldPropertiesFrame::removeObservedObject(YxSubject *object)
{
	YxObserver::removeObservedObject(object);
	if (object == fieldM)
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
void FieldPropertiesFrame::setField(YColumn *field)
{
	fieldM = field;
	if (field)
	{
		field->attach(this);
		setProperties();
	}
}
//-----------------------------------------------------------------------------
const std::string FieldPropertiesFrame::getName() const
{
    return "FieldPropertiesFrame";
}
//-----------------------------------------------------------------------------
class ColumnPropertiesHandler: public YxURIHandler
{
public:
	bool handleURI(std::string& uriStr);
private:
    // singleton; registers itself on creation.
    static const ColumnPropertiesHandler handlerInstance;
};
//-----------------------------------------------------------------------------
const ColumnPropertiesHandler ColumnPropertiesHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool ColumnPropertiesHandler::handleURI(std::string& uriStr)
{
    YURI uriObj(uriStr);
	if (uriObj.action != "edit_field" && uriObj.action != "add_field")
		return false;

	std::string ms = uriObj.getParam("object_address");		// object
	unsigned long mo;
	if (!std2wx(ms).ToULong(&mo))
		return true;

	YColumn *c = 0;
	YTable *t;

	if (uriObj.action == "add_field")
		t = (YTable *)mo;
	else
	{
		c = (YColumn *)mo;
		t = (YTable *)c->getParent();
	}

	ms = uriObj.getParam("parent_window");		// window
	if (!std2wx(ms).ToULong(&mo))
		return true;
	wxWindow *w = (wxWindow *)mo;

	FieldPropertiesFrame *f = new FieldPropertiesFrame(w, -1, wxString::Format(_("TABLE: %s"), std2wx(t->getName()).c_str()), t);
	if (c)
		f->setField(c);
	else
		f->setProperties();
	f->Show();
	return true;
}
//-----------------------------------------------------------------------------
