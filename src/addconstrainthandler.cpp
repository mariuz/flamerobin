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

  Contributor(s):
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

#include <sstream>

#include "dberror.h"
#include "gui/ExecuteSqlFrame.h"
#include "gui/MultilineEnterDialog.h"
#include "metadata/database.h"
#include "metadata/metadataitem.h"
#include "ugly.h"
#include "urihandler.h"
//-----------------------------------------------------------------------------
class AddConstraintHandler: public YxURIHandler
{
public:
	bool handleURI(std::string& uriStr);
private:
    static const AddConstraintHandler handlerInstance;	// singleton; registers itself on creation.

	std::string selectTableColumns(YTable *t, wxWindow *parent) const;
	YTable *selectTable(YDatabase *d, wxWindow *parent) const;
	std::string selectAction(const wxString& label, wxWindow *parent) const;
};
//-----------------------------------------------------------------------------
const AddConstraintHandler AddConstraintHandler::handlerInstance;
//-----------------------------------------------------------------------------
std::string AddConstraintHandler::selectTableColumns(YTable *t, wxWindow *parent) const
{
	t->checkAndLoadColumns();
	std::vector<YxMetadataItem *> temp;
	t->getChildren(temp);
	wxArrayString columns;
	for (std::vector<YxMetadataItem *>::const_iterator it = temp.begin(); it != temp.end(); ++it)
		columns.Add(std2wx((*it)->getName()));

	wxArrayInt selected_columns;
	if (!::wxGetMultipleChoices(selected_columns, _("Select one or more fields... (use ctrl key)"),  _("Table fields"), columns, parent))
		return "";

	std::string retval;
	for (size_t i=0; i<selected_columns.GetCount(); ++i)
	{
		if (i)
			retval += ", ";
		retval += wx2std(columns[selected_columns[i]]);
	}
	return retval;
}
//-----------------------------------------------------------------------------
YTable *AddConstraintHandler::selectTable(YDatabase *d, wxWindow *parent) const
{
	wxArrayString tables;
	for (YMetadataCollection<YTable>::const_iterator it = d->tablesBegin(); it != d->tablesEnd(); ++it)
		tables.Add(std2wx((*it).getName()));
	int index = ::wxGetSingleChoiceIndex(_("Select table to reference"), _("Creating foreign key"), tables, parent);
	if (index == -1)
		return 0;
	for (YMetadataCollection<YTable>::const_iterator it = d->tablesBegin(); it != d->tablesEnd(); ++it)
		if ((*it).getName() == wx2std(tables[index]))
			return const_cast<YTable *>(&(*it));
	return 0;
}
//-----------------------------------------------------------------------------
std::string AddConstraintHandler::selectAction(const wxString& label, wxWindow *parent) const
{
	wxArrayString actions;
	actions.Add(wxT("RESTRICT"));
	actions.Add(wxT("NO ACTION"));
	actions.Add(wxT("CASCADE"));
	actions.Add(wxT("SET DEFAULT"));
	actions.Add(wxT("SET NULL"));
	int index = ::wxGetSingleChoiceIndex(wxString::Format(_("Select action for %s"), label.c_str()),
		_("Creating foreign key"), actions, parent);
	if (index == -1)
		return "CANCEL";
	return wx2std(actions[index]);
}
//-----------------------------------------------------------------------------
bool AddConstraintHandler::handleURI(std::string& uriStr)
{
    YURI uriObj(uriStr);
	if (uriObj.action != "add_constraint")
		return false;

	std::string type = uriObj.getParam("type");	// pk, fk, check, unique

	std::string ms = uriObj.getParam("object_address");		// object
	unsigned long mo;
	if (!std2wx(ms).ToULong(&mo))
		return true;
	YTable *t = (YTable *)mo;

	ms = uriObj.getParam("parent_window");		// window
	if (!std2wx(ms).ToULong(&mo))
		return true;
	wxWindow *w = (wxWindow *)mo;

	if (!t || !w)
		return true;

	// Find first available constraint name:
	YDatabase *db = t->getDatabase();
	std::string default_value;
	std::string prefix = type + "_" + t->getName();
	std::vector<std::string> cnames;
	if (db->fillVector(cnames,
		"select rdb$constraint_name from rdb$relation_constraints "
		"where rdb$relation_name = '" + t->getName()
		+ "' and rdb$constraint_name starting with '" + prefix + "' order by 1"))
	{
		int i=0;
		do
		{
			i++;
			std::ostringstream ss;
			ss << i;
			default_value = prefix + "_" + ss.str();
		}
		while (std::find(cnames.begin(), cnames.end(), default_value) != cnames.end());
	}
	else
		default_value = prefix;

	wxString cname = ::wxGetTextFromUser(_("Enter constraint name"),
		_("Adding new table constraint"), std2wx(default_value), w);
	if (cname == wxT(""))	// cancel
		return true;

	std::string sql = "alter table " + t->getName() + "\nadd constraint " + wx2std(cname);

	if (type == "PK")
	{
		std::string columnlist = selectTableColumns(t, w);
		sql += "\nprimary key (" + columnlist + ")";
	}
	else if (type == "FK")
	{
		std::string columnlist = selectTableColumns(t, w);
		if (columnlist == "")
			return true;
		YTable *ref = selectTable(t->getDatabase(), w);
		if (!ref)
			return true;
		std::string refcolumnlist = selectTableColumns(ref, w);
		if (refcolumnlist == "")
			return true;
		sql += "\nforeign key (" + columnlist + ") \nreferences " + ref->getName() + " (" + refcolumnlist + ")";
		std::string action = selectAction(_("update"), w);
		if (action == "CANCEL")
			return true;
		else if (action != "RESTRICT")
			sql += "\non update " + action + " ";

		action = selectAction(_("delete"), w);
		if (action == "CANCEL")
			return true;
		else if (action != "RESTRICT")
			sql += "\non delete " + action + " ";
	}
	else if (type == "CHK")
	{
		wxString source;
		if (!GetMultilineTextFromUser(_("Enter check condition"), source, w))
			return true;
		sql += "\ncheck (" + wx2std(source) + ")";
	}
	else if (type == "UNQ")
	{
		std::string columnlist = selectTableColumns(t, w);
		sql += "\nunique (" + columnlist + ")";
	}
	else
	{
		::wxMessageBox(_("Unknown constraint type"), _("Error."), wxOK|wxICON_ERROR);
		return true;
	}

	ExecuteSqlFrame *eff = new ExecuteSqlFrame(w, -1, wxT(""));
	eff->setDatabase(db);
	eff->Show();
	eff->setSql(std2wx(sql));
	eff->executeAllStatements(true);		// true = user must commit/rollback + frame is closed at once
	return true;
}
//-----------------------------------------------------------------------------
