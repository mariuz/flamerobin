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

#include "metadata/CreateDDLVisitor.h"
//-----------------------------------------------------------------------------
CreateDDLVisitor::CreateDDLVisitor(ProgressIndicator* progressIndicator)
    : MetadataItemVisitor()
{
    progressIndicatorM = progressIndicator;
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
    wxString description = c.getDescription();
    if (!description.IsEmpty())
    {
        wxString colname(c.getName_());
        wxString tabname(c.getTable()->getName_());
        description.Replace(wxT("'"), wxT("''"));
        colname.Replace(wxT("'"), wxT("''"));
        tabname.Replace(wxT("'"), wxT("''"));
        postSqlM << wxT("UPDATE RDB$RELATION_FIELDS set RDB$DESCRIPTION = '")
                 << description << wxT("'  where RDB$FIELD_NAME = '")
                 << colname << wxT("' and RDB$RELATION_NAME = '") << tabname
                 << wxT("';\n");
    }
}
//-----------------------------------------------------------------------------
template <class T>
void iterateit(CreateDDLVisitor* v, Database& db, ProgressIndicator* pi)
{
    // this doesn't work on GCC 3.3:
    //MetadataCollection<T>* p = db.getCollection<T> ();
    MetadataCollection<T>* p = db.template getCollection<T> ();
    pi->setProgressMessage(wxT("Extracting ") + p->getPrintableName());
    pi->stepProgress();
    pi->initProgress(wxEmptyString, p->getChildrenCount(), 0, 2);

    for (typename MetadataCollection<T>::iterator it = p->begin();
        it != p->end(); ++it)
    {
        if (pi->isCanceled())
            break;
        pi->setProgressMessage(wxT("Extracting ") + (*it).getName_(), 2);
        pi->stepProgress(2);
        v->visit(*it);
    }
}
// build the sql script for entire database
void CreateDDLVisitor::visit(Database& d)
{
    // TODO: use progressIndicatorM to show what's going on, and check for 
    //       isCanceled()
    if (progressIndicatorM)
        progressIndicatorM->initProgress(wxEmptyString, 10, 0, 1);

    preSqlM << wxT("/********************* ROLES **********************/\n\n");
    iterateit<Role>(this, d, progressIndicatorM);

    preSqlM << wxT("/********************* UDFS ***********************/\n\n");
    iterateit<Function>(this, d, progressIndicatorM);

    preSqlM << wxT("/****************** GENERATORS ********************/\n\n");
    iterateit<Generator>(this, d, progressIndicatorM);

    preSqlM << wxT("/******************** DOMAINS *********************/\n\n");
    iterateit<Domain>(this, d, progressIndicatorM);

    preSqlM << wxT("/******************** TABLES **********************/\n\n");
    iterateit<Table>(this, d, progressIndicatorM);

    preSqlM << wxT("/********************* VIEWS **********************/\n\n");
    // TODO: build dependecy tree first, and order views by it
    iterateit<View>(this, d, progressIndicatorM);

    preSqlM << wxT("/******************* EXCEPTIONS *******************/\n\n");
    iterateit<Exception>(this, d, progressIndicatorM);

    preSqlM << wxT("/******************* PROCEDURES ******************/\n\n");
    iterateit<Procedure>(this, d, progressIndicatorM);

    preSqlM << wxT("/******************** TRIGGERS ********************/\n\n");
    iterateit<Trigger>(this, d, progressIndicatorM);

    sqlM = preSqlM + wxT("\n") + postSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Domain& d)
{
    sqlM += wxT("CREATE DOMAIN ") + d.getQuotedName() + wxT("\n AS ") +
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

    wxString description = d.getDescription();
    if (!description.IsEmpty())
    {
        wxString colname(d.getName_());
        description.Replace(wxT("'"), wxT("''"));
        colname.Replace(wxT("'"), wxT("''"));
        sqlM << wxT("UPDATE RDB$FIELDS set\n  RDB$DESCRIPTION = '")
             << description << wxT("'\n  where RDB$FIELD_NAME = '")
             << colname << wxT("';\n");
    }
    preSqlM << sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Exception& e)
{
    wxString ms(e.getMessage());
    ms.Replace(wxT("'"), wxT("''"));    // escape quotes
    sqlM += wxT("CREATE EXCEPTION ") + e.getQuotedName() + wxT("\n'") +
        ms + wxT("';\n");

    wxString description = e.getDescription();
    if (!description.IsEmpty())
    {
        wxString name(e.getName_());
        description.Replace(wxT("'"), wxT("''"));
        name.Replace(wxT("'"), wxT("''"));
        sqlM << wxT("UPDATE RDB$EXCEPTIONS set\n  RDB$DESCRIPTION = '")
             << description << wxT("'\n  where RDB$EXCEPTION_NAME = '")
             << name << wxT("';\n");
    }
    preSqlM << sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Function& f)
{
    sqlM << f.getCreateSql();
    wxString description = f.getDescription();
    if (!description.IsEmpty())
    {
        wxString name(f.getName_());
        description.Replace(wxT("'"), wxT("''"));
        name.Replace(wxT("'"), wxT("''"));
        sqlM << wxT("UPDATE RDB$FUNCITIONS set\n  RDB$DESCRIPTION = '")
             << description << wxT("'\n  where RDB$FUNCITION_NAME = '")
             << name << wxT("';\n");
    }
    preSqlM << sqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Generator& g)
{
    sqlM += wxT("CREATE GENERATOR ") + g.getQuotedName() + wxT(";\n");
    wxString description = g.getDescription();
    if (!description.IsEmpty())
    {
        wxString name(g.getName_());
        description.Replace(wxT("'"), wxT("''"));
        name.Replace(wxT("'"), wxT("''"));
        sqlM << wxT("UPDATE RDB$GENERATORS set\n  RDB$DESCRIPTION = '")
             << description << wxT("'\n  where RDB$GENERATOR_NAME = '")
             << name << wxT("';\n");
    }
    preSqlM << sqlM;
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

    /* description of procedure and parameters */
    wxString name(p.getName_());
    name.Replace(wxT("'"), wxT("''"));
    wxString description = p.getDescription();
    if (!description.IsEmpty())
    {
        description.Replace(wxT("'"), wxT("''"));
        temp << wxT("UPDATE RDB$PROCEDURES set\n  RDB$DESCRIPTION = '")
             << description << wxT("'\n  where RDB$PROCEDURE_NAME = '")
             << name << wxT("';\n");
    }
    std::vector<MetadataItem *> params;
    if (p.getChildren(params))
    {
        for (std::vector<MetadataItem *>::iterator it = params.begin();
            it != params.end(); ++it)
        {
            wxString description = (*it)->getDescription();
            if (!description.IsEmpty())
            {
                wxString pname((*it)->getName_());
                description.Replace(wxT("'"), wxT("''"));
                pname.Replace(wxT("'"), wxT("''"));
                temp <<
                wxT("UPDATE RDB$PROCEDURE_PARAMETERS set RDB$DESCRIPTION = '")
                << description << wxT("'\n  where RDB$PARAMETER_NAME = '")
                << pname << wxT("' AND RDB$PROCEDURE_NAME = '") << name
                << wxT("';\n");
            }
        }
    }

    postSqlM << temp;
    temp.Replace(wxT("ALTER"), wxT("CREATE"), false);   // just first
    sqlM << temp;

    // create empty procedure body (for database DDL dump)
    preSqlM << p.getAlterSql(false);
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Parameter&)
{
    // empty
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(Role& r)
{
    preSqlM += wxT("CREATE ROLE ") + r.getQuotedName() + wxT(";\n");

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
    wxString description = r.getDescription();
    if (!description.IsEmpty())
    {
        wxString name(r.getName_());
        description.Replace(wxT("'"), wxT("''"));
        name.Replace(wxT("'"), wxT("''"));
        postSqlM << wxT("UPDATE RDB$ROLES set\nRDB$DESCRIPTION = '")
                 << description << wxT("'\nwhere RDB$ROLE_NAME = '")
                 << name << wxT("';\n");
    }
    sqlM += preSqlM + wxT("\n") + postSqlM;
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
    preSqlM += wxT("CREATE TABLE ") + t.getQuotedName() + wxT("\n(\n  ");
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

    wxString description = t.getDescription();
    if (!description.IsEmpty())
    {
        wxString name(t.getName_());
        description.Replace(wxT("'"), wxT("''"));
        name.Replace(wxT("'"), wxT("''"));
        postSqlM << wxT("UPDATE RDB$RELATIONS set\nRDB$DESCRIPTION = '")
                 << description << wxT("'\nwhere RDB$RELATION_NAME = '")
                 << name << wxT("';\n");
    }

    sqlM += preSqlM + wxT("\n") + postSqlM;
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

    wxString description = t.getDescription();
    if (!description.IsEmpty())
    {
        wxString name(t.getName_());
        description.Replace(wxT("'"), wxT("''"));
        name.Replace(wxT("'"), wxT("''"));
        sqlM << wxT("UPDATE RDB$TRIGGERS set\n  RDB$DESCRIPTION = '")
             << description << wxT("'\n  where RDB$TRIGGER_NAME = '")
             << name << wxT("';\n");
    }
    postSqlM << sqlM;    // create triggers at the end
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(View& v)
{
    preSqlM << v.getCreateSql();

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
    wxString name(v.getName_());
    name.Replace(wxT("'"), wxT("''"));
    wxString description = v.getDescription();
    if (!description.IsEmpty())
    {
        description.Replace(wxT("'"), wxT("''"));
        postSqlM << wxT("UPDATE RDB$RELATIONS set\n  RDB$DESCRIPTION = '")
                 << description << wxT("'\n  where RDB$RELATION_NAME = '")
                 << name << wxT("';\n");
    }

    // description for columns
    std::vector<MetadataItem *> cols;
    if (v.getChildren(cols))
    {
        for (std::vector<MetadataItem *>::iterator it = cols.begin();
            it != cols.end(); ++it)
        {
            wxString description = (*it)->getDescription();
            if (!description.IsEmpty())
            {
                wxString cname((*it)->getName_());
                description.Replace(wxT("'"), wxT("''"));
                cname.Replace(wxT("'"), wxT("''"));
                postSqlM <<
                wxT("UPDATE RDB$RELATION_FIELDS set\n  RDB$DESCRIPTION = '")
                << description << wxT("'\n  where RDB$FIELD_NAME = '") <<
                cname << wxT(" AND RDB$RELATION_NAME = '") << name <<
                wxT("';\n");
            }
        }
    }

    sqlM += preSqlM + wxT("\n") + postSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visit(MetadataItem&)
{
    // empty
}
//-----------------------------------------------------------------------------
