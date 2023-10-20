/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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
*/


// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <vector>

#include "core/ProgressIndicator.h"
#include "metadata/column.h"
#include "metadata/Collation.h"
#include "metadata/constraints.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/exception.h"
#include "metadata/function.h"
#include "metadata/generator.h"
#include "metadata/Index.h"
#include "metadata/package.h"
#include "metadata/parameter.h"
#include "metadata/procedure.h"
#include "metadata/role.h"
#include "metadata/table.h"
#include "metadata/view.h"

// forward declaration to keep compilers happy
void addIndex(std::vector<Index> *ix, wxString& sql, ColumnConstraint *cc);

wxString CreateDDLVisitor::getCommentOn(MetadataItem& object)
{
    wxString comment = "";
    wxString description = object.getDescription();
    if (!description.empty())
    {
        comment << "COMMENT ON ";
        description.Replace("'", "''");
        wxString name(object.getQuotedName());

        switch (object.getType())
        {
        case ntColumn:
        {
            Column c = dynamic_cast<Column&>(object);
            wxString tabname(c.getTable()->getQuotedName());
            name = tabname << "." << name;
            comment << "COLUMN ";
            break;
        };
        case ntUDF:
            comment << "EXTERNAL FUNCTION ";
            break;
        case ntGenerator:
            comment << "SECUENCE ";
            break;
        default:
            comment << object.getTypeName() << " ";
            break;
        }
        comment << name << " IS "<< "'" << description << "';\n";
    }


    return comment;
}

CreateDDLVisitor::CreateDDLVisitor(ProgressIndicator* progressIndicator)
    : MetadataItemVisitor()
{
    progressIndicatorM = progressIndicator;
}

CreateDDLVisitor::~CreateDDLVisitor()
{
}

wxString CreateDDLVisitor::getSql() const
{
    return sqlM;
}

wxString CreateDDLVisitor::getPrefixSql() const
{
    return preSqlM;
}

wxString CreateDDLVisitor::getSuffixSql() const
{
    return postSqlM + grantSqlM;
}

void CreateDDLVisitor::visitCollation(Collation& collation)
{
    preSqlM += "CREATE COLLATION " + collation.getName_() + " \n" +
        collation.getSource() + "\n; \n";
    postSqlM << getCommentOn(collation);
    sqlM = preSqlM + postSqlM;

}

// this one is not called from "outside", but from visit(Table) function
void CreateDDLVisitor::visitColumn(Column& c)
{
    wxString computed = c.getComputedSource();
    if (!computed.IsEmpty())
    {
        wxString add = "ALTER TABLE " + c.getTable()->getQuotedName()
            + " ADD " + c.getQuotedName() + " COMPUTED BY "
            + computed + ";\n";
        postSqlM << add;
        sqlM << add;
        return;
    }

    preSqlM << c.getQuotedName() << " ";
    wxString collate = c.getCollation();
    if (DomainPtr d = c.getDomain())
    {
        if (d->isSystem())
        {
            preSqlM << d->getDatatypeAsString();
            if (c.isIdentity())
                preSqlM << c.getSource(true);
            wxString charset = d->getCharset();
            DatabasePtr db = d->getDatabase();
            if (!charset.IsEmpty())
            {
                if (!db || db->getDatabaseCharset() != charset)
                    preSqlM << " CHARACTER SET " << charset;
                if (db && db->isDefaultCollation(charset, collate))
                    collate.clear();    // don't show default collations
            }
        }
        else
            preSqlM << d->getQuotedName();
    }
    else
        preSqlM <<  c.getSource();  // shouldn't happen

    wxString defaultValue;
    if (c.getDefault(IgnoreDomainDefault, defaultValue))
        preSqlM << " DEFAULT " << defaultValue;
    if (!c.isNullable(IgnoreDomainNullability))
        preSqlM << " NOT NULL";
    if (!collate.IsEmpty())
    {
        preSqlM << " COLLATE " << collate;
    }
    postSqlM << getCommentOn(c);
}

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

// build the sql script for entire database
void CreateDDLVisitor::visitDatabase(Database& d)
{
    if (progressIndicatorM)
        progressIndicatorM->initProgress(wxEmptyString, 10, 0, 1);

    try
    {

        preSqlM << "/********************* COLLATES **********************/\n\n";
        iterateit<CollationsPtr, Collation>(this, d.getCollations(), progressIndicatorM);

        preSqlM << "/********************* ROLES **********************/\n\n";
        iterateit<RolesPtr, Role>(this, d.getRoles(), progressIndicatorM);

        preSqlM << "/********************* UDFS ***********************/\n\n";
        iterateit<UDFsPtr, UDF>(this, d.getUDFs(), progressIndicatorM);
        
        if (d.getInfo().getODSVersionIsHigherOrEqualTo(12.0)) {
            preSqlM << "/********************* FUNCTIONS ***********************/\n\n";
            iterateit<FunctionSQLsPtr, FunctionSQL>(this, d.getFunctionSQLs(),
                progressIndicatorM);
        }

        preSqlM << "/****************** SEQUENCES ********************/\n\n";
        iterateit<GeneratorsPtr, Generator>(this, d.getGenerators(),
            progressIndicatorM);

        preSqlM << "/******************** DOMAINS *********************/\n\n";
        iterateit<DomainsPtr, Domain>(this, d.getDomains(),
            progressIndicatorM);

        preSqlM << "/******************* PROCEDURES ******************/\n\n";
        iterateit<ProceduresPtr, Procedure>(this, d.getProcedures(),
            progressIndicatorM);

        if (d.getInfo().getODSVersionIsHigherOrEqualTo(12.0)) {
            preSqlM << "/******************* PACKAGES ******************/\n\n";
            iterateit<PackagesPtr, Package>(this, d.getPackages(),
                progressIndicatorM);
        }
      
        preSqlM << "/******************** TABLES **********************/\n\n";
        iterateit<TablesPtr, Table>(this, d.getTables(), progressIndicatorM);
        if (d.getInfo().getODSVersionIsHigherOrEqualTo(11.1)) {
            iterateit<GTTablesPtr, GTTable>(this, d.getGTTables(), progressIndicatorM);
        }

        preSqlM << "/********************* VIEWS **********************/\n\n";
        // TODO: build dependecy tree first, and order views by it
        //       also include computed columns of tables?
        iterateit<ViewsPtr, View>(this, d.getViews(), progressIndicatorM);

        preSqlM << "/******************* EXCEPTIONS *******************/\n\n";
        iterateit<ExceptionsPtr, Exception>(this, d.getExceptions(),
            progressIndicatorM);

        preSqlM << "/******************** TRIGGERS ********************/\n\n";
        iterateit<DMLTriggersPtr, DMLTrigger>(this, d.getDMLTriggers(),
            progressIndicatorM);

        if (d.getInfo().getODSVersionIsHigherOrEqualTo(11.1)) {
            preSqlM << "/******************** DB TRIGGERS ********************/\n\n";
            iterateit<DBTriggersPtr, DBTrigger>(this, d.getDBTriggers(),
                progressIndicatorM);
        }
        if (d.getInfo().getODSVersionIsHigherOrEqualTo(12.0)) {
            preSqlM << "/******************** DDL TRIGGERS ********************/\n\n";
            iterateit<DDLTriggersPtr, DDLTrigger>(this, d.getDDLTriggers(),
                progressIndicatorM);
        }
    }
    catch (CancelProgressException&)
    {
        // this is expected if user cancels the extraction
        sqlM = _("Extraction canceled");
        return;
    }

    sqlM = preSqlM + "\n" + postSqlM + grantSqlM;
    if (progressIndicatorM)
    {
        progressIndicatorM->initProgress(_("Extraction complete."), 1, 1);
        progressIndicatorM->initProgress(_("Done."), 1, 1, 2);
    }
}

void CreateDDLVisitor::visitDomain(Domain& d)
{
    preSqlM += "CREATE DOMAIN " + d.getQuotedName() + "\n AS " +
            d.getDatatypeAsString();
    wxString charset = d.getCharset();
    DatabasePtr db = d.getDatabase();
    if (!charset.IsEmpty() && (!db || db->getDatabaseCharset() != charset))
        preSqlM += " CHARACTER SET " + charset;
    preSqlM += "\n";
    wxString defaultValue;
    if (d.getDefault(defaultValue))
        preSqlM += " DEFAULT " + defaultValue + "\n";
    if (!d.isNullable())
        preSqlM += " NOT NULL\n";
    wxString check = d.getCheckConstraint();
    if (!check.IsEmpty())
        preSqlM += " " + check + "\n";  // already contains CHECK keyword
    wxString collate = d.getCollation();
    if (!collate.IsEmpty())
        preSqlM += " COLLATE " + collate;
    preSqlM += ";\n";

    postSqlM << getCommentOn(d);
    sqlM = preSqlM + postSqlM;
}

void CreateDDLVisitor::visitException(Exception& e)
{
    wxString ms(e.getMessage());
    ms.Replace("'", "''");    // escape quotes
    preSqlM += "CREATE EXCEPTION " + e.getQuotedName() + "\n'" +
        ms + "';\n";
    
    postSqlM << getCommentOn(e);
    sqlM = preSqlM + postSqlM;
}

void CreateDDLVisitor::visitForeignKey(ForeignKey& fk)
{
    Identifier reftab(fk.getReferencedTable());
    wxString src_col, dest_col;
    for (std::vector<wxString>::const_iterator it = fk.begin(); it != fk.end(); ++it)
    {
        if (it != fk.begin())
            src_col += ",";
        Identifier id(*it);
        src_col += id.getQuoted();
    }
    for (std::vector<wxString>::const_iterator it = fk.getReferencedColumns().begin();
        it != fk.getReferencedColumns().end(); ++it)
    {
        if (it != fk.getReferencedColumns().begin())
            dest_col += ",";
        Identifier id(*it);
        dest_col += id.getQuoted();
    }
    postSqlM += "ALTER TABLE " + fk.getTable()->getQuotedName() + " ADD";
    if (!fk.isSystem())
        postSqlM += " CONSTRAINT " + fk.getQuotedName();
    postSqlM += "\n  FOREIGN KEY (" + src_col + ") REFERENCES "
        + reftab.getQuoted() + " (" + dest_col + ")";
    wxString upd = fk.getUpdateAction();
    if (!upd.IsEmpty() && upd != "RESTRICT")
        postSqlM += " ON UPDATE " + upd;
    wxString del = fk.getDeleteAction();
    if (!del.IsEmpty() && del != "RESTRICT")
        postSqlM += " ON DELETE " + del;
    addIndex(fk.getTable()->getIndices(), postSqlM, &fk);
    postSqlM += ";\n";
    sqlM = postSqlM;
}

void CreateDDLVisitor::visitFunctionSQL(FunctionSQL& f)
{
    wxString temp(f.getAlterSql());
    temp += "\n";

    // grant execute on [name] to [user/role]
    const std::vector<Privilege>* priv = f.getPrivileges();
    if (priv)
    {
        for (std::vector<Privilege>::const_iterator ci = priv->begin();
            ci != priv->end(); ++ci)
        {
            grantSqlM += (*ci).getSql() + "\n";
        }
    }

    /* description of function and parameters */
    postSqlM << getCommentOn(f);
    
    for (ParameterPtrs::iterator it = f.begin(); it != f.end(); ++it)
    {
        temp << getCommentOn(*(*it));
    }

    postSqlM << temp << "\n";
    temp.Replace("ALTER", "CREATE", false);   // just first
    sqlM << temp << grantSqlM;

    // create empty function body (for database DDL dump)
    temp = f.getAlterSql(false);    // false = only headers
    temp.Replace("ALTER", "CREATE", false);   // just first
    preSqlM << temp << "\n";
}


void CreateDDLVisitor::visitUDF(UDF& f)
{
    preSqlM << f.getCreateSql() << "\n";
    postSqlM << getCommentOn(f);
    sqlM = preSqlM + postSqlM;
}

void CreateDDLVisitor::visitGenerator(Generator& g)
{
    preSqlM += "CREATE " + g.getSource() + ";\n";
    postSqlM << getCommentOn(g);
    sqlM = preSqlM + postSqlM;
}

void CreateDDLVisitor::visitIndex(Index& i)
{
//    preSqlM += "CREATE INDEX " + i.getQuotedName() + "\n AS ";

    preSqlM += "CREATE ";
    if (i.isUnique())
        preSqlM += "UNIQUE ";
    if (i.getIndexType() == Index::itDescending)
        preSqlM += "DESCENDING ";
    preSqlM += "INDEX " + i.getQuotedName() + " ON " /*+ t.getQuotedName()*/;
    wxString expre = i.getExpression();
    if (!expre.IsEmpty())
        preSqlM += " COMPUTED BY " + expre;
    else
    {
        preSqlM += " (";
        std::vector<wxString>* cols = i.getSegments();
        for (std::vector<wxString>::const_iterator it = cols->begin(); it != cols->end(); ++it)
        {
            if (it != cols->begin())
                preSqlM += ",";
            Identifier id(*it);
            preSqlM += id.getQuoted();
        }
        preSqlM += ")";
    }
    preSqlM += ";\n";



    postSqlM << getCommentOn(i);
    sqlM = preSqlM + postSqlM;

}

void CreateDDLVisitor::visitPrimaryKeyConstraint(PrimaryKeyConstraint& pk)
{
    wxString sql;
    if (!pk.isSystem())     // system one, noname
        sql += " CONSTRAINT " + pk.getQuotedName();
    sql += " PRIMARY KEY (";

    for (std::vector<wxString>::const_iterator it = pk.begin(); it != pk.end(); ++it)
    {
        if (it != pk.begin())
            sql += ",";
        Identifier id(*it);
        sql += id.getQuoted();
    }
    sql += ")";
    addIndex(pk.getTable()->getIndices(), sql, &pk);
    preSqlM += ",\n " + sql;
    sqlM = "ALTER TABLE " + pk.getTable()->getQuotedName() + " ADD" +
        sql + ";\n";
}

void CreateDDLVisitor::visitPackage(Package& package)
{
    wxString temp(package.getAlterSql());
    temp += "\n";

    // grant execute on [name] to [user/role]
    const std::vector<Privilege>* priv = package.getPrivileges();
    if (priv)
    {
        for (std::vector<Privilege>::const_iterator ci = priv->begin();
            ci != priv->end(); ++ci)
        {
            grantSqlM += (*ci).getSql() + "\n";
        }
    }

    /* description of package*/
    postSqlM << getCommentOn(package);

    postSqlM << temp << "\n";
    temp.Replace("ALTER", "CREATE", false);   // just first
    sqlM << temp << grantSqlM;

    temp = package.getAlterSql(false);    // false = only headers
    temp.Replace("ALTER", "CREATE", false);   // just first
    preSqlM << temp << "\n";

}

void CreateDDLVisitor::visitProcedure(Procedure& p)
{
    wxString temp(p.getAlterSql());
    temp += "\n";

    // grant execute on [name] to [user/role]
    const std::vector<Privilege>* priv = p.getPrivileges();
    if (priv)
    {
        for (std::vector<Privilege>::const_iterator ci = priv->begin();
            ci != priv->end(); ++ci)
        {
            grantSqlM += (*ci).getSql() + "\n";
        }
    }

    /* description of procedure and parameters */
    postSqlM << getCommentOn(p);
    for (ParameterPtrs::iterator it = p.begin(); it != p.end(); ++it)
    {
        temp << getCommentOn(*(*it));
    }

    postSqlM << temp << "\n";
    temp.Replace("ALTER", "CREATE", false);   // just first
    sqlM << temp << grantSqlM;

    // create empty procedure body (for database DDL dump)
    temp = p.getAlterSql(false);    // false = only headers
    temp.Replace("ALTER", "CREATE", false);   // just first
    preSqlM << temp << "\n";
}

void CreateDDLVisitor::visitRole(Role& r)
{
    preSqlM += "CREATE ROLE " + r.getQuotedName() + ";\n";

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
    postSqlM << getCommentOn(r);
    sqlM = preSqlM + "\n" + postSqlM + grantSqlM;
}

// used by visit(Table)
void addIndex(std::vector<Index> *ix, wxString& sql, ColumnConstraint *cc)
{
    // only for FB 1.5+
    if (!ix || cc->getIndexName().StartsWith("RDB$") || cc->getName_() == cc->getIndexName())
        return;
    for (std::vector<Index>::iterator it = ix->begin(); it != ix->end(); ++it)
    {
        if ((*it).getName_() == cc->getIndexName())
        {
            sql += "\n  USING ";
            if ((*it).getIndexType() == Index::itDescending)
                sql += "DESC ";
            sql += "INDEX " + (*it).getQuotedName();
        }
    }
}

void CreateDDLVisitor::visitTable(Table& t)
{
    int type = t.getRelationType();
    preSqlM += "CREATE ";
    if (type == 4 || type == 5)
        preSqlM += "GLOBAL TEMPORARY ";
    preSqlM += "TABLE " + t.getQuotedName();
    wxString external = t.getExternalPath();
    if (!external.IsEmpty())
    {
        external.Replace("'", "''");
        preSqlM += " EXTERNAL '" + external + "'";
    }
    preSqlM += "\n(\n  ";
    t.ensureChildrenLoaded();
    for (ColumnPtrs::iterator it=t.begin(); it!=t.end(); ++it)
    {
        if (it != t.begin() && (*it)->getComputedSource().empty())
            preSqlM += ",\n  ";
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
            postSqlM += "ALTER TABLE " + t.getQuotedName() + " ADD ";
            if (!(*ci).isSystem())
                postSqlM += "CONSTRAINT " + (*ci).getQuotedName();
            postSqlM += "\n  " + (*ci).getSource() + ";\n";
        }
    }

    // indices
    if (ix)
    {
        for (std::vector<Index>::iterator ci = ix->begin(); ci != ix->end(); ++ci)
        {
            if ((*ci).isSystem())
                continue;
            postSqlM += "CREATE ";
            if ((*ci).isUnique())
                postSqlM += "UNIQUE ";
            if ((*ci).getIndexType() == Index::itDescending)
                postSqlM += "DESCENDING ";
            postSqlM += "INDEX " + (*ci).getQuotedName() + " ON " + t.getQuotedName();
            wxString expre = (*ci).getExpression();
            if (!expre.IsEmpty())
                postSqlM += " COMPUTED BY " + expre;
            else
            {
                postSqlM += " (";
                std::vector<wxString> *cols = (*ci).getSegments();
                for (std::vector<wxString>::const_iterator it = cols->begin(); it != cols->end(); ++it)
                {
                    if (it != cols->begin())
                        postSqlM += ",";
                    Identifier id(*it);
                    postSqlM += id.getQuoted();
                }
                postSqlM += ")";
            }
            postSqlM += ";\n";
        }
    }

    // grant sel/ins/upd/del/ref/all ON [name] to [SP,user,role]
    const std::vector<Privilege>* priv = t.getPrivileges();
    if (priv)
        for (std::vector<Privilege>::const_iterator ci = priv->begin(); ci != priv->end(); ++ci)
            grantSqlM += (*ci).getSql() + "\n";

    preSqlM += "\n)";
    if (type == 4)
        preSqlM += "\nON COMMIT PRESERVE ROWS";
    preSqlM += ";\n";

    postSqlM << getCommentOn(t);

    sqlM = preSqlM + "\n" + postSqlM + grantSqlM;
}

void CreateDDLVisitor::visitGTTable(GTTable& t)
{
    int type = t.getRelationType();
    preSqlM += "CREATE ";
    if (type == 4 || type == 5)
        preSqlM += "GLOBAL TEMPORARY ";
    preSqlM += "TABLE " + t.getQuotedName();
    wxString external = t.getExternalPath();
    if (!external.IsEmpty())
    {
        external.Replace("'", "''");
        preSqlM += " EXTERNAL '" + external + "'";
    }
    preSqlM += "\n(\n  ";
    t.ensureChildrenLoaded();
    for (ColumnPtrs::iterator it = t.begin(); it != t.end(); ++it)
    {
        if (it != t.begin() && (*it)->getComputedSource().empty())
            preSqlM += ",\n  ";
        visitColumn(*(*it).get());
    }

    std::vector<Index>* ix = t.getIndices();

    // primary keys (detect the name and use CONSTRAINT name PRIMARY KEY... or PRIMARY KEY(col)
    PrimaryKeyConstraint* pk = t.getPrimaryKey();
    if (pk)
        visitPrimaryKeyConstraint(*pk);

    // unique constraints
    std::vector<UniqueConstraint>* uc = t.getUniqueConstraints();
    if (uc)
        for (std::vector<UniqueConstraint>::iterator it = uc->begin(); it != uc->end(); ++it)
            visitUniqueConstraint(*it);

    // foreign keys
    std::vector<ForeignKey>* fk = t.getForeignKeys();
    if (fk)
        for (std::vector<ForeignKey>::iterator it = fk->begin(); it != fk->end(); ++it)
            visitForeignKey(*it);

    // check constraints
    std::vector<CheckConstraint>* chk = t.getCheckConstraints();
    if (chk)
    {
        for (std::vector<CheckConstraint>::iterator ci = chk->begin(); ci != chk->end(); ++ci)
        {
            postSqlM += "ALTER TABLE " + t.getQuotedName() + " ADD ";
            if (!(*ci).isSystem())
                postSqlM += "CONSTRAINT " + (*ci).getQuotedName();
            postSqlM += "\n  " + (*ci).getSource() + ";\n";
        }
    }

    // indices
    if (ix)
    {
        for (std::vector<Index>::iterator ci = ix->begin(); ci != ix->end(); ++ci)
        {
            if ((*ci).isSystem())
                continue;
            postSqlM += "CREATE ";
            if ((*ci).isUnique())
                postSqlM += "UNIQUE ";
            if ((*ci).getIndexType() == Index::itDescending)
                postSqlM += "DESCENDING ";
            postSqlM += "INDEX " + (*ci).getQuotedName() + " ON " + t.getQuotedName();
            wxString expre = (*ci).getExpression();
            if (!expre.IsEmpty())
                postSqlM += " COMPUTED BY " + expre;
            else
            {
                postSqlM += " (";
                std::vector<wxString>* cols = (*ci).getSegments();
                for (std::vector<wxString>::const_iterator it = cols->begin(); it != cols->end(); ++it)
                {
                    if (it != cols->begin())
                        postSqlM += ",";
                    Identifier id(*it);
                    postSqlM += id.getQuoted();
                }
                postSqlM += ")";
            }
            postSqlM += ";\n";
        }
    }

    // grant sel/ins/upd/del/ref/all ON [name] to [SP,user,role]
    const std::vector<Privilege>* priv = t.getPrivileges();
    if (priv)
        for (std::vector<Privilege>::const_iterator ci = priv->begin(); ci != priv->end(); ++ci)
            grantSqlM += (*ci).getSql() + "\n";

    preSqlM += "\n)";
    if (type == 4)
        preSqlM += "\nON COMMIT PRESERVE ROWS";
    preSqlM += ";\n";

    postSqlM << getCommentOn(t);

    sqlM = preSqlM + "\n" + postSqlM + grantSqlM;
}

void CreateDDLVisitor::visitDBTrigger(DBTrigger& t)
{
    preSqlM << "SET TERM ^ ;\nCREATE TRIGGER " << t.getQuotedName();
    if (t.getActive())
        preSqlM << " ACTIVE\n";
    else
        preSqlM << " INACTIVE\n";
    preSqlM << t.getFiringEvent();
    preSqlM << " POSITION ";
    preSqlM << t.getPosition() << "\n";
    preSqlM << t.getSource();
    preSqlM << "^\nSET TERM ; ^\n";

    postSqlM << getCommentOn(t);
    sqlM = preSqlM + postSqlM;
}

void CreateDDLVisitor::visitDDLTrigger(DDLTrigger& t)
{
    preSqlM << "SET TERM ^ ;\nCREATE TRIGGER " << t.getQuotedName();
    if (t.isDMLTrigger())
    {
        Identifier id(t.getRelationName());
        preSqlM << " FOR " << id.getQuoted();
    }
    if (t.getActive())
        preSqlM << " ACTIVE\n";
    else
        preSqlM << " INACTIVE\n";
    preSqlM << t.getFiringEvent();
    preSqlM << " POSITION ";
    preSqlM << t.getPosition() << "\n";
    preSqlM << t.getSource();
    preSqlM << "^\nSET TERM ; ^\n";

    postSqlM << getCommentOn(t);
    sqlM = preSqlM + postSqlM;
}

void CreateDDLVisitor::visitDMLTrigger(DMLTrigger& t)
{
    preSqlM << "SET TERM ^ ;\nCREATE TRIGGER " << t.getQuotedName();
    if (t.isDMLTrigger())
    {
        Identifier id(t.getRelationName());
        preSqlM << " FOR " << id.getQuoted();
    }
    if (t.getActive())
        preSqlM << " ACTIVE\n";
    else
        preSqlM << " INACTIVE\n";
    preSqlM << t.getFiringEvent();
    preSqlM << " POSITION ";
    preSqlM << t.getPosition() << "\n";
    preSqlM << t.getSource();
    preSqlM << "^\nSET TERM ; ^\n";

    postSqlM << getCommentOn(t);
    sqlM = preSqlM + postSqlM;
}

void CreateDDLVisitor::visitUniqueConstraint(UniqueConstraint& unq)
{
    wxString sql;
    if (!unq.isSystem())
        sql += " CONSTRAINT " + unq.getQuotedName();
    sql += " UNIQUE (";
    for (std::vector<wxString>::const_iterator it = unq.begin(); it != unq.end(); ++it)
    {
        if (it != unq.begin())
            sql += ",";
        Identifier id(*it);
        sql += id.getQuoted();
    }
    sql += ")";
    addIndex(unq.getTable()->getIndices(), sql, &unq);
    preSqlM += ",\n " + sql;
    sqlM = "ALTER TABLE " + unq.getTable()->getQuotedName()
        + " ADD" + sql + ";\n";
}

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
            grantSqlM += (*ci).getSql() + "\n";
        }
    }

    postSqlM << getCommentOn(v);

    // description for columns
    for (ColumnPtrs::iterator it = v.begin(); it != v.end();
        ++it)
    {
        postSqlM << getCommentOn(*(*it));
    }
    

    sqlM += preSqlM + "\n" + postSqlM + grantSqlM;
}

void CreateDDLVisitor::visitCharacterSet(CharacterSet&  /*characterset*/)
{
}
