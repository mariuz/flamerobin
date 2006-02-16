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
    :MetadataItemVisitor()
{
}
//-----------------------------------------------------------------------------
CreateDDLVisitor::~CreateDDLVisitor()
{
}
//-----------------------------------------------------------------------------
wxString CreateDDLVisitor::getSql() const
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
// this one is not called from "outside", but from visit(Table) function
void CreateDDLVisitor::visit(Column& c)
{
    preSqlM << c.getQuotedName() << wxT(" ");

    wxString computed = c.getComputedSource();
    if (!computed.IsEmpty())
    {
        preSqlM << wxT("COMPUTED BY ") << computed;
        return;
    }

    Domain *d = c.getDomain();
    if (d)
    {
        if (d->isSystem())
        {
            preSqlM << d->getDatatypeAsString();
            wxString charset = d->getCharset();
            Database *db = d->getDatabase();
            if (!charset.IsEmpty() && (!db
                || db->getDatabaseCharset() != charset))
            {
                preSqlM << wxT(" CHARACTER SET ") << charset;
            }
        }
        else
            preSqlM << d->getQuotedName();
    }
    else
        preSqlM <<  c.getSource();  // shouldn't happen

    wxString defaultVal = c.getDefault();
    if (!defaultVal.IsEmpty())
        preSqlM << wxT(" ") << defaultVal;     // already contains word DEFAULT
    if (!c.isNullable())
        preSqlM << wxT(" NOT NULL");
    wxString collate = c.getCollation();
    if (!collate.IsEmpty())
        preSqlM << wxT(" COLLATE ") << collate;
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
            d.getDatatypeAsString();
    wxString charset = d.getCharset();
    Database *db = d.getDatabase();
    if (!charset.IsEmpty() && (!db || db->getDatabaseCharset() != charset))
        sqlM += wxT(" CHARACTER SET ") + charset;
    sqlM += wxT("\n");
    wxString dflt(d.getDefault());
    if (!dflt.IsEmpty())
        sqlM += wxT(" ") + dflt + wxT("\n");   // already contains DEFAULT keyword
    if (!d.isNullable())
        sqlM += wxT(" NOT NULL\n");
    wxString check = d.getCheckConstraint();
    if (!check.IsEmpty())
        sqlM += wxT(" ") + check + wxT("\n");  // already contains CHECK keyword
    wxString collate = d.getCollation();
    if (!collate.IsEmpty())
        sqlM += wxT(" COLLATE ") + collate;
    sqlM += wxT(";");
    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Exception& e)
{
    wxString ms(e.getMessage());
    ms.Replace(wxT("'"), wxT("''"));    // escape quotes
    sqlM = wxT("CREATE EXCEPTION ") + e.getQuotedName() + wxT("\n'") +
        ms + wxT("';\n");

    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Function& f)
{
    sqlM = f.getCreateSql();
    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Generator& g)
{
    sqlM = wxT("CREATE GENERATOR ") + g.getQuotedName() + wxT(";\n");
    preSqlM = sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Procedure& p)
{
    wxString temp(p.getAlterSql());
    temp += wxT("\n");

    // grant execute on [name] to [user/role]
    const std::vector<Privilege>* priv = p.getPrivileges();
    if (priv)
    {
        for (std::vector<Privilege>::const_iterator ci = priv->begin();
            ci != priv->end(); ++ci)
        {
            temp += (*ci).getSql() + wxT(";\n");
        }
    }

    postSqlM = temp;
    temp.Replace(wxT("ALTER"), wxT("CREATE"), false);   // just first
    sqlM = temp;

    // TODO: create empty procedure body (for database DDL dump)
    preSqlM = wxT("SET TERM ^ ;\nCREATE PROCEDURE ") + p.getQuotedName()
    //    + wxT(" ") + p.params
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
    preSqlM = wxT("CREATE ROLE ") + r.getQuotedName() + wxT(";\n");

    // grant execute on [name] to [user/role]
    const std::vector<Privilege>* priv = r.getPrivileges();
    if (priv)
    {
        for (std::vector<Privilege>::const_iterator ci = priv->begin();
            ci != priv->end(); ++ci)
        {
            postSqlM += (*ci).getSql() + wxT(";\n");
        }
    }
    sqlM = preSqlM + wxT("\n") + postSqlM;
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
    preSqlM = wxT("CREATE TABLE ") + t.getQuotedName() + wxT("\n(\n  ");
    for (MetadataCollection<Column>::iterator it=t.begin(); it!=t.end(); ++it)
    {
        if (it != t.begin())
            preSqlM += wxT(",\n  ");
        visit(*it);
    }

    // primary keys (detect the name and use CONSTRAINT name PRIMARY KEY... or PRIMARY KEY(col)
    ColumnConstraint *pk = t.getPrimaryKey();
    if (pk)
    {
        preSqlM += wxT(",\n ");
        if (!pk->isSystem())     // system one, noname
            preSqlM += wxT(" CONSTRAINT ") + pk->getQuotedName();
        preSqlM += wxT(" PRIMARY KEY (");

        for (std::vector<wxString>::const_iterator it = pk->begin(); it != pk->end(); ++it)
        {
            if (it != pk->begin())
                preSqlM += wxT(",");
            Identifier id(*it);
            preSqlM += id.getQuoted();
        }
        preSqlM += wxT(")");
    }

    // unique constraints
    std::vector<ColumnConstraint> *uc = t.getUniqueConstraints();
    if (uc)
    {
        for (std::vector<ColumnConstraint>::iterator ci = uc->begin(); ci != uc->end(); ++ci)
        {
            preSqlM += wxT(",\n ");
            if (!(*ci).isSystem())
                preSqlM += wxT(" CONSTRAINT ") + (*ci).getQuotedName();
            preSqlM += wxT(" UNIQUE (");
            for (std::vector<wxString>::const_iterator it = (*ci).begin(); it != (*ci).end(); ++it)
            {
                if (it != (*ci).begin())
                    preSqlM += wxT(",");
                Identifier id(*it);
                preSqlM += id.getQuoted();
            }
            preSqlM += wxT(")");
        }
    }

    // foreign keys
    std::vector<ForeignKey> *fk = t.getForeignKeys();
    if (fk)
    {
        for (std::vector<ForeignKey>::iterator ci = fk->begin(); ci != fk->end(); ++ci)
        {
            Identifier reftab((*ci).referencedTableM);
            wxString src_col, dest_col;
            for (std::vector<wxString>::const_iterator it = (*ci).begin(); it != (*ci).end(); ++it)
            {
                if (it != (*ci).begin())
                    src_col += wxT(",");
                Identifier id(*it);
                src_col += id.getQuoted();
            }
            for (std::vector<wxString>::const_iterator it = (*ci).referencedColumnsM.begin();
                it != (*ci).referencedColumnsM.end(); ++it)
            {
                if (it != (*ci).referencedColumnsM.begin())
                    dest_col += wxT(",");
                Identifier id(*it);
                dest_col += id.getQuoted();
            }
            postSqlM += wxT("ALTER TABLE ") + t.getQuotedName() + wxT(" ADD");
            if (!(*ci).isSystem())
                postSqlM += wxT(" CONSTRAINT ") + (*ci).getQuotedName();
            postSqlM += wxT("\n  FOREIGN KEY (") + src_col + wxT(") REFERENCES ")
                + reftab.getQuoted() + wxT(" (") + dest_col + wxT(")");
            wxString upd = (*ci).updateActionM;
            if (!upd.IsEmpty() && upd != wxT("RESTRICT"))
                postSqlM += wxT(" ON UPDATE ") + upd;
            wxString del = (*ci).deleteActionM;
            if (!del.IsEmpty() && del != wxT("RESTRICT"))
                postSqlM += wxT(" ON DELETE ") + del;
            postSqlM += wxT(";\n");
        }
    }

    // check constraints
    std::vector<CheckConstraint> *chk = t.getCheckConstraints();
    if (chk)
    {
        for (std::vector<CheckConstraint>::iterator ci = chk->begin(); ci != chk->end(); ++ci)
        {
            postSqlM += wxT("ALTER TABLE ") + t.getQuotedName() + wxT(" ADD ");
            if (!(*ci).isSystem())
                postSqlM += wxT("CONSTRAINT ") + (*ci).getQuotedName();
            postSqlM += wxT("\n  ") + (*ci).sourceM + wxT(";\n");
        }
    }

    // indices
    std::vector<Index> *ix = t.getIndices();
    if (ix)
    {
        for (std::vector<Index>::iterator ci = ix->begin(); ci != ix->end(); ++ci)
        {
            if ((*ci).isSystem())
                continue;
            postSqlM += wxT("CREATE ");
            if ((*ci).getIndexType() == Index::itDescending)
                postSqlM += wxT("DESCENDING ");
            postSqlM += wxT("INDEX ") + (*ci).getQuotedName() + wxT(" ON ")
                + t.getQuotedName() + wxT(" (");
            std::vector<wxString> *cols = (*ci).getSegments();
            for (std::vector<wxString>::const_iterator it = cols->begin(); it != cols->end(); ++it)
            {
                if (it != cols->begin())
                    postSqlM += wxT(",");
                Identifier id(*it);
                postSqlM += id.getQuoted();
            }
            postSqlM += wxT(");\n");
        }
    }

    // grant sel/ins/upd/del/ref/all ON [name] to [SP,user,role]
    const std::vector<Privilege>* priv = t.getPrivileges();
    if (priv)
    {
        for (std::vector<Privilege>::const_iterator ci = priv->begin();
            ci != priv->end(); ++ci)
        {
            postSqlM += (*ci).getSql() + wxT(";\n");
        }
    }

    preSqlM += wxT("\n);\n");
    sqlM = preSqlM + wxT("\n") + postSqlM;
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

    sqlM << wxT("SET TERM ^ ;\nCREATE TRIGGER ") << t.getQuotedName()
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

    preSqlM = wxT("CREATE VIEW ") + v.getQuotedName() + wxT(" (");
    bool first = true;
    for (MetadataCollection<Column>::const_iterator it = v.begin();
        it != v.end(); ++it)
    {
        if (first)
            first = false;
        else
            preSqlM += wxT(", ");
        preSqlM += (*it).getQuotedName();
    }
    preSqlM += wxT(")\nAS ") + src + wxT(";\n");

    // grant sel/ins/upd/del/ref/all ON [name] to [SP,user,role]
    const std::vector<Privilege>* priv = v.getPrivileges();
    if (priv)
    {
        for (std::vector<Privilege>::const_iterator ci = priv->begin();
            ci != priv->end(); ++ci)
        {
            postSqlM += (*ci).getSql() + wxT(";\n");
        }
    }
    sqlM = preSqlM + wxT("\n") + postSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(MetadataItem&)
{
    // empty
}
//-----------------------------------------------------------------------------
