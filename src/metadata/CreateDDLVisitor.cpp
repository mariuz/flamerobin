//-----------------------------------------------------------------------------
/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License") you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babauskov.

  Portions created by the original developer
  are Copyright (C) 2006 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/
//-----------------------------------------------------------------------------

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

#include "CreateDDLVisitor.h"
//-----------------------------------------------------------------------------
CreateDDLVisitor::CreateDDLVisitor()
{
}
//-----------------------------------------------------------------------------
CreateDDLVisitor::~CreateDDLVisitor()
{
}
//-----------------------------------------------------------------------------
wxString CreateDDLVisitor::getSQL() const
{
    return sqlM;
}
//-----------------------------------------------------------------------------
wxString CreateDDLVisitor::getPrefixSql() const
{
    return preSqlM;
}
//-----------------------------------------------------------------------------
wxString CreateDDLVisitor::getSuffixSql() const
{
    return postSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Column&)
{
    // empty
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Database&)
{
    // TODO: build the sql script for entire database?
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Domain& d)
{
    sqlM = wxT("CREATE DOMAIN ") + d.getQuotedName() + wxT("\n AS ") +
            d.getDatatypeAsString() + wxT("\n");
    wxString dflt = d.getDefault();
    if (!dflt.IsEmpty())
        sqlM += wxT(" DEFAULT ") + dflt + wxT("\n");
    if (d.isNotNull())
        sqlM += wxT(" NOT NULL\n")
    wxString check = d.getCheckConstraint();
    if (!check.IsEmpty())
        sqlM += wxT(" CHECK (") + check + wxT(")\n");
    wxString collate = d.getCollation();
    if (!collate.IsEmpty())
        sqlM += wxT(" COLLATE ") + collate;

    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Exception& e)
{
    sqlM = wxT("CREATE EXCEPTION )" + e.getQuotedName() + wxT("'") +
        e.getMessage() + wxT("'\n");

    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Function& f)
{
    sqlM = f.getCreateSQL();
    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Generator& g)
{
    sqlM = wxT("CREATE GENERATOR ") + g.getQuotedName() + wxT("\n");
    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Procedure& p)
{
    wxString temp(p.getAlterSql());
    postSqlM = temp;
    temp.Replace(wxT("ALTER"), wxT("CREATE"), false);   // just first
    sqlM = temp;

    // TODO: grant execute on [name] to [user/role]

    preSqlM = wxT("SET TERM ^ ;\nCREATE PROCEDURE ") + p.getQuotedName()
        + wxT(" ") + p.params
        + wxT("\nAS\nBEGIN\n/* nothing */\nEND^\nSET TERM ; ^");
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Parameter&)
{
    // empty
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Role& r)
{
    sqlM = wxT("CREATE ROLE ") + r.getQuotedName();

    // TODO: grant role [name] to [user]

    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Root&)
{
    // empty
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Server&)
{
    // empty
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Table& t)
{
    preSqlM = wxT("CREATE TABLE ") + t.getQuotedName() + wxT("\n(\n");
    for (MetadataCollection<Column>::iterator it=t.begin(); it!=t.end; ++it)
        preSqlM += (*it).getDDL();

    presqlM +=
    // primary keys (detect the name and use CONSTRAINT name PRIMARY KEY... or PRIMARY KEY(col)
    // unique constraints

    postSqlM =
    // foreign keys
    // check constraints
    // indices
    // TODO: grant sel/ins/upd/del/ref/all ON [name] to [SP,user,role]

    sqlM = preSqlM + postSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Trigger& t)
{
    wxString object, source, type, relation;
    bool active;
    int position;
    t.getTriggerInfo(object, active, position, type);
    t.getSource(source);
    t.getRelation(relation);

    sqlM << wxT("SET TERM ^ ;\nCREATE TRIGGER ") << getQuotedName()
         << wxT(" FOR ") << relation;
    if (active)
        sqlM << wxT(" ACTIVE\n");
    else
        sqlM << wxT(" INACTIVE\n");
    sqlM << type;
    sqlM << wxT(" POSITION ");
    sqlM << position << wxT("\n");
    sqlM << source;
    sqlM << wxT("^\nSET TERM ; ^");

    postSqlM = sqlM;    // create triggers at the end
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(View& v)
{
    wxString src;
    v.checkAndLoadColumns();
    v.getSource(src);

    sqlM = wxT("CREATE VIEW ") + getQuotedName() + wxT(" (");
    bool first = true;
    for (MetadataCollection<Column>::const_iterator it = v.begin();
        it != v.end(); ++it)
    {
        if (first)
            first = false;
        else
            sqlM += wxT(", ");
        sqlM += (*it).getQuotedName();
    }
    sqlM += wxT(")\nAS ");
    sqlM += src;

    // TODO: grant sel/ins/upd/del/ref/all ON [name] to [SP,user,role]

    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(MetadataItem&)
{
    // empty
}
//-----------------------------------------------------------------------------
