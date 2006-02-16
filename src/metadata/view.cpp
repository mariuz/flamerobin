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

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <ibpp.h>

#include "core/Visitor.h"
#include "dberror.h"
#include "frutils.h"
#include "metadata/collection.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/view.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
View::View()
{
    typeM = ntView;
}
//-----------------------------------------------------------------------------
//! returns false if an error occurs
bool View::getSource(wxString& source)
{
    source = wxT("");
    Database *d = getDatabase();
    if (!d)
    {
        lastError().setMessage(wxT("Database not set."));
        return false;
    }

    IBPP::Database& db = d->getIBPPDatabase();

    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
        st1->Prepare("select rdb$view_source from rdb$relations where rdb$relation_name = ?");
        st1->Set(1, wx2std(getName_()));
        st1->Execute();
        st1->Fetch();
        readBlob(st1, 1, source);
        tr1->Commit();
        return true;
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }

    return false;
}
//-----------------------------------------------------------------------------
wxString View::getCreateSql()
{
    wxString src;
    if (!getSource(src))
        return lastError().getMessage();

    wxString sql;
    sql += wxT("CREATE VIEW ") + getQuotedName() + wxT(" (");

    bool first = true;
    for (MetadataCollection <Column>::const_iterator it = columnsM.begin(); it != columnsM.end(); ++it)
    {
        if (first)
            first = false;
        else
            sql += wxT(", ");
        sql += (*it).getQuotedName();
    }
    sql += wxT(")\nAS ");
    sql += src;
    sql += wxT(";\n");
    return sql;
}
//-----------------------------------------------------------------------------
wxString View::getAlterSql()
{
    if (!checkAndLoadColumns())
        return lastError().getMessage();

    wxString sql = wxT("DROP VIEW ") + getQuotedName() + wxT(";\n");
    sql += getCreateSql();

    // Restore user privileges
    const std::vector<Privilege>* priv = getPrivileges();
    if (priv)
    {
        for (std::vector<Privilege>::const_iterator ci = priv->begin();
            ci != priv->end(); ++ci)
        {
            sql += (*ci).getSql() + wxT(";\n");
        }
    }

    return sql;
}
//-----------------------------------------------------------------------------
// STEPS:
// * drop dependent check constraints (checks reference this view)
// * alter dep. stored procedures
// * drop dep. views (BUILD DEPENDENCY TREE)
// * drop this view
// * create this view
// * create dep. views
// * alter back SPs
// * recreate triggers
// * create checks
// * grant privileges on all dropped views
void View::getRebuildSql(std::vector<wxString>& prefix,
    std::vector<wxString>& suffix)
{
    // This comes first since suffix has reversed order
    const std::vector<Privilege>* priv = getPrivileges();
    if (priv)
    {
        for (std::vector<Privilege>::const_iterator ci = priv->begin();
            ci != priv->end(); ++ci)
        {
            suffix.push_back((*ci).getSql() + wxT(";\n"));
        }
    }

    // Recreate triggers on this view
    std::vector<Trigger *> trigs;
    getTriggers(trigs, Trigger::afterTrigger);
    getTriggers(trigs, Trigger::beforeTrigger);
    for (std::vector<Trigger *>::iterator it = trigs.begin();
        it != trigs.end(); ++it)
    {
        CreateDDLVisitor cdv;
        (*it)->acceptVisitor(&cdv);
        suffix.push_back(cdv.getSql());
    }

    // ret << wxT("/* drop checks that reference this view */\n");
    // TODO: add each CHECK to drops/creates


    wxString retval;
    std::vector<Dependency> list;
    if (getDependencies(list, false))
    {
        for (std::vector<Dependency>::iterator it = list.begin();
            it != list.end(); ++it)
        {
            Procedure *p = dynamic_cast<Procedure *>((*it).getDependentObject());
            if (p)
            {
                prefix.push_back(p->getAlterSql(false));    // alter to empty
                suffix.push_back(p->getAlterSql(true));     // alter to full
            }
        }

        // FIXME: it's not that simple, we need to build a dependecy tree
        for (std::vector<Dependency>::iterator it = list.begin();
            it != list.end(); ++it)
        {
            View *v = dynamic_cast<View *>((*it).getDependentObject());
            if (v && v != this)
                v->getRebuildSql(prefix, suffix);
        }
    }
    prefix.push_back(wxT("DROP VIEW ") + getQuotedName() + wxT(";\n"));
    suffix.push_back(getCreateSql());
}
//-----------------------------------------------------------------------------
wxString View::getCreateSqlTemplate() const
{
    wxString sql(
        wxT("CREATE VIEW name ( view_column, ...)\n")
        wxT("AS\n")
        wxT("/* write select statement here */\n")
        wxT("WITH CHECK OPTION;\n"));
    return sql;
}
//-----------------------------------------------------------------------------
const wxString View::getTypeName() const
{
    return wxT("VIEW");
}
//-----------------------------------------------------------------------------
void View::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visit(*this);
}
//-----------------------------------------------------------------------------
