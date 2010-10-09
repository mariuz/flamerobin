/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

#include <algorithm>
#include <sstream>

#include "frutils.h"
#include "gui/ExecuteSql.h"
#include "gui/MultilineEnterDialog.h"
#include "metadata/database.h"
#include "metadata/metadataitem.h"
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
    for (Tables::const_iterator it = d->getTables()->begin(); it != d->getTables()->end(); ++it)
        tables.Add((*it).getName_());
    int index = ::wxGetSingleChoiceIndex(_("Select table to reference"), _("Creating foreign key"), tables, parent);
    if (index == -1)
        return 0;
    for (Tables::const_iterator it = d->getTables()->begin(); it != d->getTables()->end(); ++it)
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
    Database* db = t->getDatabase(wxT("AddConstraintHandler::handleURI"));
    wxString prefix = type + wxT("_") + t->getName_();
    wxString stmt(
        wxT("select rdb$constraint_name from rdb$relation_constraints ")
        wxT("where rdb$relation_name = '") + t->getName_() +
        wxT("' and rdb$constraint_name starting with '") + prefix +
        wxT("' order by 1"));
    wxString default_value;
    wxArrayString constraintNames(db->loadIdentifiers(stmt));
    for (int i = 0; ; ++i)
    {
        default_value = prefix + wxString::Format(wxT("_%d"), i);
        if (constraintNames.Index(default_value, false) == wxNOT_FOUND)
            break;
    }

    wxString cname = ::wxGetTextFromUser(_("Enter constraint name"),
        _("Adding new table constraint"), default_value, w);
    if (cname.IsEmpty())    // cancel
        return true;

    wxString sql = wxT("alter table ") + t->getQuotedName() +
            wxT("\nadd constraint ") + Identifier::userString(cname);

    if (type == wxT("PK"))
    {
        wxString columnlist = selectRelationColumns(t, w);
        if (columnlist.IsEmpty())   // cancel
            return true;
        sql += wxT("\nprimary key (") + columnlist + wxT(")");
    }
    else if (type == wxT("FK"))
    {
        wxString columnlist = selectRelationColumns(t, w);
        if (columnlist == wxT(""))
            return true;
        Table* ref = selectTable(t->findDatabase(), w);
        if (!ref)
            return true;
        wxString refcolumnlist = selectRelationColumns(ref, w);
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
        wxString columnlist = selectRelationColumns(t, w);
        if (columnlist.IsEmpty())   // cancel
            return true;
        sql += wxT("\nunique (") + columnlist + wxT(")");
    }
    else
    {
        ::wxMessageBox(_("Unknown constraint type"), _("Error."), wxOK | wxICON_ERROR);
        return true;
    }

    execSql(w, wxT(""),db, sql, true);  // true = commit + close at once
    return true;
}
//-----------------------------------------------------------------------------
