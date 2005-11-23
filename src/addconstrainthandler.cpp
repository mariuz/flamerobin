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

#include <algorithm>
#include <sstream>

#include "dberror.h"
#include "frutils.h"
#include "gui/ExecuteSqlFrame.h"
#include "gui/MultilineEnterDialog.h"
#include "metadata/database.h"
#include "metadata/metadataitem.h"
#include "ugly.h"
#include "urihandler.h"
//-----------------------------------------------------------------------------
class AddConstraintHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    static const AddConstraintHandler handlerInstance;    // singleton; registers itself on creation.

    Table *selectTable(Database *d, wxWindow *parent) const;
    wxString selectAction(const wxString& label, wxWindow *parent) const;
};
//-----------------------------------------------------------------------------
const AddConstraintHandler AddConstraintHandler::handlerInstance;
//-----------------------------------------------------------------------------
Table *AddConstraintHandler::selectTable(Database *d, wxWindow *parent) const
{
    wxArrayString tables;
    for (MetadataCollection<Table>::const_iterator it = d->tablesBegin(); it != d->tablesEnd(); ++it)
        tables.Add((*it).getName_());
    int index = ::wxGetSingleChoiceIndex(_("Select table to reference"), _("Creating foreign key"), tables, parent);
    if (index == -1)
        return 0;
    for (MetadataCollection<Table>::const_iterator it = d->tablesBegin(); it != d->tablesEnd(); ++it)
        if ((*it).getName_() == tables[index])
            return const_cast<Table *>(&(*it));
    return 0;
}
//-----------------------------------------------------------------------------
wxString AddConstraintHandler::selectAction(const wxString& label, wxWindow *parent) const
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
        return wxT("CANCEL");
    return actions[index];
}
//-----------------------------------------------------------------------------
bool AddConstraintHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("add_constraint"))
        return false;

    wxString type = uri.getParam(wxT("type"));    // pk, fk, check, unique
    Table *t = (Table *)getObject(uri);
    wxWindow *w = getWindow(uri);
    if (!t || !w)
        return true;

    // Find first available constraint name:
    Database *db = t->getDatabase();
    wxString default_value;
    wxString prefix = type + wxT("_") + t->getName_();
    std::vector<wxString> cnames;
    if (db->fillVector(cnames,
        wxT("select rdb$constraint_name from rdb$relation_constraints ")
        wxT("where rdb$relation_name = '") + t->getName_()
        + wxT("' and rdb$constraint_name starting with '") + prefix + wxT("' order by 1")))
    {
        int i = 0;
        do
        {
            i++;
            default_value = prefix + wxString::Format(wxT("_%d"), i);
        }
        while (std::find(cnames.begin(), cnames.end(), default_value) != cnames.end());
    }
    else
        default_value = prefix;

    wxString cname = ::wxGetTextFromUser(_("Enter constraint name"),
        _("Adding new table constraint"), default_value, w);
    if (cname.IsEmpty())    // cancel
        return true;

    Identifier cqname(cname);
    wxString sql = wxT("alter table ") + t->getQuotedName() + wxT("\nadd constraint ") + cqname.getQuoted();

    if (type == wxT("PK"))
    {
        wxString columnlist = selectTableColumns(t, w);
        sql += wxT("\nprimary key (") + columnlist + wxT(")");
    }
    else if (type == wxT("FK"))
    {
        wxString columnlist = selectTableColumns(t, w);
        if (columnlist == wxT(""))
            return true;
        Table* ref = selectTable(t->getDatabase(), w);
        if (!ref)
            return true;
        wxString refcolumnlist = selectTableColumns(ref, w);
        if (refcolumnlist == wxT(""))
            return true;
        sql += wxT("\nforeign key (") + columnlist + wxT(") \nreferences ") + ref->getQuotedName()
            + wxT(" (") + refcolumnlist + wxT(")");
        wxString action = selectAction(_("update"), w);
        if (action == wxT("CANCEL"))
            return true;
        else if (action != wxT("RESTRICT"))
            sql += wxT("\non update ") + action + wxT(" ");

        action = selectAction(_("delete"), w);
        if (action == wxT("CANCEL"))
            return true;
        else if (action != wxT("RESTRICT"))
            sql += wxT("\non delete ") + action + wxT(" ");
    }
    else if (type == wxT("CHK"))
    {
        wxString source;
        if (!GetMultilineTextFromUser(w, _("Enter check condition"), source))
            return true;
        sql += wxT("\ncheck (") + source + wxT(")");
    }
    else if (type == wxT("UNQ"))
    {
        wxString columnlist = selectTableColumns(t, w);
        sql += wxT("\nunique (") + columnlist + wxT(")");
    }
    else
    {
        ::wxMessageBox(_("Unknown constraint type"), _("Error."), wxOK | wxICON_ERROR);
        return true;
    }

    ExecuteSqlFrame *eff = new ExecuteSqlFrame(w, -1, wxT(""));
    eff->setDatabase(db);
    eff->Show();
    eff->setSql(sql);
    eff->executeAllStatements(true);        // true = user must commit/rollback + frame is closed at once
    return true;
}
//-----------------------------------------------------------------------------
