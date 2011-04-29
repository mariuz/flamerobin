/*
  Copyright (c) 2004-2011 The FlameRobin Development Team

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

#include <vector>

#include "core/ProgressIndicator.h"
#include "metadata/column.h"
#include "metadata/constraints.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/exception.h"
#include "metadata/function.h"
#include "metadata/generator.h"
#include "metadata/Index.h"
#include "metadata/parameter.h"
#include "metadata/procedure.h"
#include "metadata/role.h"
#include "metadata/table.h"
#include "metadata/view.h"

// forward declaration to keep compilers happy
void addIndex(std::vector<Index> *ix, wxString& sql, ColumnConstraint *cc);
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
    return postSqlM + grantSqlM;
}
//-----------------------------------------------------------------------------
// this one is not called from "outside", but from visit(Table) function
void CreateDDLVisitor::visitColumn(Column& c)
{
    wxString computed = c.getComputedSource();
    if (!computed.IsEmpty())
    {
        wxString add = wxT("ALTER TABLE ") + c.getTable()->getQuotedName()
            + wxT(" ADD ") + c.getQuotedName() + wxT(" COMPUTED BY ")
            + computed + wxT(";\n");
        postSqlM << add;
        sqlM << add;
        return;
    }

    preSqlM << c.getQuotedName() << wxT(" ");
    wxString collate = c.getCollation();
    if (DomainPtr d = c.getDomain())
    {
        if (d->isSystem())
        {
            preSqlM << d->getDatatypeAsString();
            wxString charset = d->getCharset();
            DatabasePtr db = d->getDatabase();
            if (!charset.IsEmpty())
            {
                if (!db || db->getDatabaseCharset() != charset)
                    preSqlM << wxT(" CHARACTER SET ") << charset;
                if (db && db->isDefaultCollation(charset, collate))
                    collate.clear();    // don't show default collations
            }
        }
        else
            preSqlM << d->getQuotedName();
    }
    else
        preSqlM <<  c.getSource();  // shouldn't happen

    wxString defaultVal = c.getDefault();
    if (!defaultVal.IsEmpty())
        preSqlM << wxT(" DEFAULT ") << defaultVal;
    if (c.hasNotNullConstraint())
        preSqlM << wxT(" NOT NULL");
    if (!collate.IsEmpty())
    {
        preSqlM << wxT(" COLLATE ") << collate;
    }
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
template <class C, class M>
void iterateit(CreateDDLVisitor* v, C mc, ProgressIndicator* pi)
{
    wxASSERT(mc);

    if (pi)
    {
        pi->setProgressMessage(_("Extracting ") + mc->getName_());
        pi->stepProgress();
        pi->initProgress(wxEmptyString, mc->getChildrenCount(), 0, 2);
    }

    for (typename MetadataCollection<M>::iterator it = mc->begin();
        it != mc->end(); ++it)
    {
        if (pi)
        {
            checkProgressIndicatorCanceled(pi);
            pi->setProgressMessage(_("Extracting ") + (*it)->getName_(), 2);
            pi->stepProgress(1, 2);
        }
        (*it)->acceptVisitor(v);
    }
}
//-----------------------------------------------------------------------------
// build the sql script for entire database
void CreateDDLVisitor::visitDatabase(Database& d)
{
    if (progressIndicatorM)
        progressIndicatorM->initProgress(wxEmptyString, 10, 0, 1);

    try
    {
        preSqlM << wxT("/********************* ROLES **********************/\n\n");
        iterateit<RolesPtr, Role>(this, d.getRoles(), progressIndicatorM);

        preSqlM << wxT("/********************* UDFS ***********************/\n\n");
        iterateit<FunctionsPtr, Function>(this, d.getFunctions(),
            progressIndicatorM);

        preSqlM << wxT("/****************** GENERATORS ********************/\n\n");
        iterateit<GeneratorsPtr, Generator>(this, d.getGenerators(),
            progressIndicatorM);

        preSqlM << wxT("/******************** DOMAINS *********************/\n\n");
        iterateit<DomainsPtr, Domain>(this, d.getDomains(),
            progressIndicatorM);

        preSqlM << wxT("/******************* PROCEDURES ******************/\n\n");
        iterateit<ProceduresPtr, Procedure>(this, d.getProcedures(),
            progressIndicatorM);

        preSqlM << wxT("/******************** TABLES **********************/\n\n");
        iterateit<TablesPtr, Table>(this, d.getTables(), progressIndicatorM);

        preSqlM << wxT("/********************* VIEWS **********************/\n\n");
        // TODO: build dependecy tree first, and order views by it
        //       also include computed columns of tables?
        iterateit<ViewsPtr, View>(this, d.getViews(), progressIndicatorM);

        preSqlM << wxT("/******************* EXCEPTIONS *******************/\n\n");
        iterateit<ExceptionsPtr, Exception>(this, d.getExceptions(),
            progressIndicatorM);

        preSqlM << wxT("/******************** TRIGGERS ********************/\n\n");
        iterateit<TriggersPtr, Trigger>(this, d.getTriggers(),
            progressIndicatorM);
    }
    catch (CancelProgressException&)
    {
        // this is expected if user cancels the extraction
        sqlM = _("Extraction canceled");
        return;
    }

    sqlM = preSqlM + wxT("\n") + postSqlM + grantSqlM;
    if (progressIndicatorM)
    {
        progressIndicatorM->setProgressMessage(_("Extraction complete."));
        progressIndicatorM->setProgressPosition(10);
        progressIndicatorM->setProgressMessage(_("Done."), 2);
    }
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visitDomain(Domain& d)
{
    preSqlM += wxT("CREATE DOMAIN ") + d.getQuotedName() + wxT("\n AS ") +
            d.getDatatypeAsString();
    wxString charset = d.getCharset();
    DatabasePtr db = d.getDatabase();
    if (!charset.IsEmpty() && (!db || db->getDatabaseCharset() != charset))
        preSqlM += wxT(" CHARACTER SET ") + charset;
    preSqlM += wxT("\n");
    wxString dflt(d.getDefault());
    if (!dflt.IsEmpty())
        preSqlM += wxT(" DEFAULT ") + dflt + wxT("\n");
    if (!d.isNullable())
        preSqlM += wxT(" NOT NULL\n");
    wxString check = d.getCheckConstraint();
    if (!check.IsEmpty())
        preSqlM += wxT(" ") + check + wxT("\n");  // already contains CHECK keyword
    wxString collate = d.getCollation();
    if (!collate.IsEmpty())
        preSqlM += wxT(" COLLATE ") + collate;
    preSqlM += wxT(";\n");

    wxString description = d.getDescription();
    if (!description.IsEmpty())
    {
        wxString colname(d.getName_());
        description.Replace(wxT("'"), wxT("''"));
        colname.Replace(wxT("'"), wxT("''"));
        postSqlM << wxT("UPDATE RDB$FIELDS set\n  RDB$DESCRIPTION = '")
             << description << wxT("'\n  where RDB$FIELD_NAME = '")
             << colname << wxT("';\n");
    }
    sqlM = preSqlM + postSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visitException(Exception& e)
{
    wxString ms(e.getMessage());
    ms.Replace(wxT("'"), wxT("''"));    // escape quotes
    preSqlM += wxT("CREATE EXCEPTION ") + e.getQuotedName() + wxT("\n'") +
        ms + wxT("';\n");

    wxString description = e.getDescription();
    if (!description.IsEmpty())
    {
        wxString name(e.getName_());
        description.Replace(wxT("'"), wxT("''"));
        name.Replace(wxT("'"), wxT("''"));
        postSqlM << wxT("UPDATE RDB$EXCEPTIONS set\n  RDB$DESCRIPTION = '")
             << description << wxT("'\n  where RDB$EXCEPTION_NAME = '")
             << name << wxT("';\n");
    }
    sqlM = preSqlM + postSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visitForeignKey(ForeignKey& fk)
{
    Identifier reftab(fk.getReferencedTable());
    wxString src_col, dest_col;
    for (std::vector<wxString>::const_iterator it = fk.begin(); it != fk.end(); ++it)
    {
        if (it != fk.begin())
            src_col += wxT(",");
        Identifier id(*it);
        src_col += id.getQuoted();
    }
    for (std::vector<wxString>::const_iterator it = fk.getReferencedColumns().begin();
        it != fk.getReferencedColumns().end(); ++it)
    {
        if (it != fk.getReferencedColumns().begin())
            dest_col += wxT(",");
        Identifier id(*it);
        dest_col += id.getQuoted();
    }
    postSqlM += wxT("ALTER TABLE ") + fk.getTable()->getQuotedName() + wxT(" ADD");
    if (!fk.isSystem())
        postSqlM += wxT(" CONSTRAINT ") + fk.getQuotedName();
    postSqlM += wxT("\n  FOREIGN KEY (") + src_col + wxT(") REFERENCES ")
        + reftab.getQuoted() + wxT(" (") + dest_col + wxT(")");
    wxString upd = fk.getUpdateAction();
    if (!upd.IsEmpty() && upd != wxT("RESTRICT"))
        postSqlM += wxT(" ON UPDATE ") + upd;
    wxString del = fk.getDeleteAction();
    if (!del.IsEmpty() && del != wxT("RESTRICT"))
        postSqlM += wxT(" ON DELETE ") + del;
    addIndex(fk.getTable()->getIndices(), postSqlM, &fk);
    postSqlM += wxT(";\n");
    sqlM = postSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visitFunction(Function& f)
{
    preSqlM << f.getCreateSql() << wxT("\n");
    wxString description = f.getDescription();
    if (!description.IsEmpty())
    {
        wxString name(f.getName_());
        description.Replace(wxT("'"), wxT("''"));
        name.Replace(wxT("'"), wxT("''"));
        postSqlM << wxT("UPDATE RDB$FUNCTIONS set\n  RDB$DESCRIPTION = '")
             << description << wxT("'\n  where RDB$FUNCTION_NAME = '")
             << name << wxT("';\n");
    }
    sqlM = preSqlM + postSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visitGenerator(Generator& g)
{
    preSqlM += wxT("CREATE GENERATOR ") + g.getQuotedName() + wxT(";\n");
    wxString description = g.getDescription();
    if (!description.IsEmpty())
    {
        wxString name(g.getName_());
        description.Replace(wxT("'"), wxT("''"));
        name.Replace(wxT("'"), wxT("''"));
        postSqlM << wxT("UPDATE RDB$GENERATORS set\n  RDB$DESCRIPTION = '")
             << description << wxT("'\n  where RDB$GENERATOR_NAME = '")
             << name << wxT("';\n");
    }
    sqlM = preSqlM + postSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visitPrimaryKeyConstraint(PrimaryKeyConstraint& pk)
{
    wxString sql;
    if (!pk.isSystem())     // system one, noname
        sql += wxT(" CONSTRAINT ") + pk.getQuotedName();
    sql += wxT(" PRIMARY KEY (");

    for (std::vector<wxString>::const_iterator it = pk.begin(); it != pk.end(); ++it)
    {
        if (it != pk.begin())
            sql += wxT(",");
        Identifier id(*it);
        sql += id.getQuoted();
    }
    sql += wxT(")");
    addIndex(pk.getTable()->getIndices(), sql, &pk);
    preSqlM += wxT(",\n ") + sql;
    sqlM = wxT("ALTER TABLE ") + pk.getTable()->getQuotedName() + wxT(" ADD") +
        sql + wxT(";\n");
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visitProcedure(Procedure& p)
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
            grantSqlM += (*ci).getSql() + wxT("\n");
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
    for (ParameterPtrs::iterator it = p.begin(); it != p.end(); ++it)
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

    postSqlM << temp << wxT("\n");
    temp.Replace(wxT("ALTER"), wxT("CREATE"), false);   // just first
    sqlM << temp << grantSqlM;

    // create empty procedure body (for database DDL dump)
    temp = p.getAlterSql(false);    // false = only headers
    temp.Replace(wxT("ALTER"), wxT("CREATE"), false);   // just first
    preSqlM << temp << wxT("\n");
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visitRole(Role& r)
{
    preSqlM += wxT("CREATE ROLE ") + r.getQuotedName() + wxT(";\n");

    // grant execute on [name] to [user/role]
    const std::vector<Privilege>* priv = r.getPrivileges();
    if (priv)
    {
        for (std::vector<Privilege>::const_iterator ci = priv->begin();
            ci != priv->end(); ++ci)
        {
            grantSqlM += (*ci).getSql();
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
    sqlM = preSqlM + wxT("\n") + postSqlM + grantSqlM;
}
//-----------------------------------------------------------------------------
// used by visit(Table)
void addIndex(std::vector<Index> *ix, wxString& sql, ColumnConstraint *cc)
{
    // only for FB 1.5+
    if (!ix || cc->getIndexName().StartsWith(wxT("RDB$")) || cc->getName_() == cc->getIndexName())
        return;
    for (std::vector<Index>::iterator it = ix->begin(); it != ix->end(); ++it)
    {
        if ((*it).getName_() == cc->getIndexName())
        {
            sql += wxT("\n  USING ");
            if ((*it).getIndexType() == Index::itDescending)
                sql += wxT("DESC ");
            sql += wxT("INDEX ") + (*it).getQuotedName();
        }
    }
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visitTable(Table& t)
{
    int type = t.getRelationType();
    preSqlM += wxT("CREATE ");
    if (type == 4 || type == 5)
        preSqlM += wxT("GLOBAL TEMPORARY ");
    preSqlM += wxT("TABLE ") + t.getQuotedName();
    wxString external = t.getExternalPath();
    if (!external.IsEmpty())
    {
        external.Replace(wxT("'"), wxT("''"));
        preSqlM += wxT(" EXTERNAL '") + external + wxT("'");
    }
    preSqlM += wxT("\n(\n  ");
    t.ensureChildrenLoaded();
    for (ColumnPtrs::iterator it=t.begin(); it!=t.end(); ++it)
    {
        if (it != t.begin() && (*it)->getComputedSource().empty())
            preSqlM += wxT(",\n  ");
        visitColumn(*(*it).get());
    }

    std::vector<Index> *ix = t.getIndices();

    // primary keys (detect the name and use CONSTRAINT name PRIMARY KEY... or PRIMARY KEY(col)
    PrimaryKeyConstraint *pk = t.getPrimaryKey();
    if (pk)
        visitPrimaryKeyConstraint(*pk);

    // unique constraints
    std::vector<UniqueConstraint> *uc = t.getUniqueConstraints();
    if (uc)
        for (std::vector<UniqueConstraint>::iterator it = uc->begin(); it != uc->end(); ++it)
            visitUniqueConstraint(*it);

    // foreign keys
    std::vector<ForeignKey> *fk = t.getForeignKeys();
    if (fk)
        for (std::vector<ForeignKey>::iterator it = fk->begin(); it != fk->end(); ++it)
            visitForeignKey(*it);

    // check constraints
    std::vector<CheckConstraint> *chk = t.getCheckConstraints();
    if (chk)
    {
        for (std::vector<CheckConstraint>::iterator ci = chk->begin(); ci != chk->end(); ++ci)
        {
            postSqlM += wxT("ALTER TABLE ") + t.getQuotedName() + wxT(" ADD ");
            if (!(*ci).isSystem())
                postSqlM += wxT("CONSTRAINT ") + (*ci).getQuotedName();
            postSqlM += wxT("\n  ") + (*ci).getSource() + wxT(";\n");
        }
    }

    // indices
    if (ix)
    {
        for (std::vector<Index>::iterator ci = ix->begin(); ci != ix->end(); ++ci)
        {
            if ((*ci).isSystem())
                continue;
            postSqlM += wxT("CREATE ");
            if ((*ci).isUnique())
                postSqlM += wxT("UNIQUE ");
            if ((*ci).getIndexType() == Index::itDescending)
                postSqlM += wxT("DESCENDING ");
            postSqlM += wxT("INDEX ") + (*ci).getQuotedName() + wxT(" ON ") + t.getQuotedName();
            wxString expre = (*ci).getExpression();
            if (!expre.IsEmpty())
                postSqlM += wxT(" COMPUTED BY ") + expre;
            else
            {
                postSqlM += wxT(" (");
                std::vector<wxString> *cols = (*ci).getSegments();
                for (std::vector<wxString>::const_iterator it = cols->begin(); it != cols->end(); ++it)
                {
                    if (it != cols->begin())
                        postSqlM += wxT(",");
                    Identifier id(*it);
                    postSqlM += id.getQuoted();
                }
                postSqlM += wxT(")");
            }
            postSqlM += wxT(";\n");
        }
    }

    // grant sel/ins/upd/del/ref/all ON [name] to [SP,user,role]
    const std::vector<Privilege>* priv = t.getPrivileges();
    if (priv)
        for (std::vector<Privilege>::const_iterator ci = priv->begin(); ci != priv->end(); ++ci)
            grantSqlM += (*ci).getSql() + wxT("\n");

    preSqlM += wxT("\n)");
    if (type == 4)
        preSqlM += wxT("\nON COMMIT PRESERVE ROWS");
    preSqlM += wxT(";\n");

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

    sqlM = preSqlM + wxT("\n") + postSqlM + grantSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visitTrigger(Trigger& t)
{
    wxString object, type;
    bool active, db;
    int position;
    t.getTriggerInfo(object, active, position, type, db);
    wxString source = t.getSource();
    wxString relation = t.getTriggerRelation();

    preSqlM << wxT("SET TERM ^ ;\nCREATE TRIGGER ") << t.getQuotedName();
    if (!db)
    {
        Identifier id(relation);
        preSqlM << wxT(" FOR ") << id.getQuoted();
    }
    if (active)
        preSqlM << wxT(" ACTIVE\n");
    else
        preSqlM << wxT(" INACTIVE\n");
    preSqlM << type;
    preSqlM << wxT(" POSITION ");
    preSqlM << position << wxT("\n");
    preSqlM << source;
    preSqlM << wxT("^\nSET TERM ; ^\n");

    wxString description = t.getDescription();
    if (!description.IsEmpty())
    {
        wxString name(t.getName_());
        description.Replace(wxT("'"), wxT("''"));
        name.Replace(wxT("'"), wxT("''"));
        postSqlM << wxT("UPDATE RDB$TRIGGERS set\n  RDB$DESCRIPTION = '")
             << description << wxT("'\n  where RDB$TRIGGER_NAME = '")
             << name << wxT("';\n");
    }
    sqlM = preSqlM + postSqlM;
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visitUniqueConstraint(UniqueConstraint& unq)
{
    wxString sql;
    if (!unq.isSystem())
        sql += wxT(" CONSTRAINT ") + unq.getQuotedName();
    sql += wxT(" UNIQUE (");
    for (std::vector<wxString>::const_iterator it = unq.begin(); it != unq.end(); ++it)
    {
        if (it != unq.begin())
            sql += wxT(",");
        Identifier id(*it);
        sql += id.getQuoted();
    }
    sql += wxT(")");
    addIndex(unq.getTable()->getIndices(), sql, &unq);
    preSqlM += wxT(",\n ") + sql;
    sqlM = wxT("ALTER TABLE ") + unq.getTable()->getQuotedName()
        + wxT(" ADD") + sql + wxT(";\n");
}
//-----------------------------------------------------------------------------
void CreateDDLVisitor::visitView(View& v)
{
    preSqlM << v.getCreateSql();

    // grant sel/ins/upd/del/ref/all ON [name] to [SP,user,role]
    const std::vector<Privilege>* priv = v.getPrivileges();
    if (priv)
    {
        for (std::vector<Privilege>::const_iterator ci = priv->begin();
            ci != priv->end(); ++ci)
        {
            grantSqlM += (*ci).getSql() + wxT("\n");
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
    for (ColumnPtrs::iterator it = v.begin(); it != v.end();
        ++it)
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
            cname << wxT("' AND RDB$RELATION_NAME = '") << name <<
            wxT("';\n");
        }
    }

    sqlM += preSqlM + wxT("\n") + postSqlM + grantSqlM;
}
//-----------------------------------------------------------------------------
