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


#ifndef FR_METADATAITEMVISITOR_H
#define FR_METADATAITEMVISITOR_H

// [GoF] Visitor pattern. Abstract Visitor for metadata items.

#include "core/Visitor.h"
#include "metadata/MetadataClasses.h"

class MetadataItemVisitor: public Visitor
{
public:
    MetadataItemVisitor();
    virtual ~MetadataItemVisitor();

    virtual void visitCharacterSet(CharacterSet& characterSet);
    virtual void visitCharacterSets(CharacterSets& characterSets);
    virtual void visitCollation(Collation& collation);
    virtual void visitSysCollations(SysCollations& collations);
    virtual void visitCollations(Collations& collations);
    virtual void visitColumn(Column& column);
    virtual void visitDatabase(Database& database);
    virtual void visitDBTrigger(DBTrigger& trigger);
    virtual void visitDBTriggers(DBTriggers& triggers);
    virtual void visitDDLTrigger(DDLTrigger& trigger);
    virtual void visitDDLTriggers(DDLTriggers& triggers);
    virtual void visitDMLTrigger(DMLTrigger& trigger);
    virtual void visitDMLTriggers(DMLTriggers& triggers);
    virtual void visitDomain(Domain& domain);
    virtual void visitDomains(Domains& domains);
    virtual void visitException(Exception& exception);
    virtual void visitExceptions(Exceptions& exceptions);
    virtual void visitForeignKey(ForeignKey& fk);
    virtual void visitFunctionSQL(FunctionSQL& function);
    virtual void visitFunctionSQLs(FunctionSQLs& functions);
    virtual void visitGenerator(Generator& generator);
    virtual void visitGenerators(Generators& generators);
    virtual void visitGTTable(GTTable& table);
    virtual void visitGTTables(GTTables& tables);
    virtual void visitIndex(Index& index);
    virtual void visitIndices(Indices& indices);
    virtual void visitMetadataItem(MetadataItem& metadataItem);
    virtual void visitMethod(Method& method);
    virtual void visitParameter(Parameter& parameter);
    virtual void visitPrimaryKeyConstraint(PrimaryKeyConstraint& pk);
    virtual void visitPackage(Package& package);
    virtual void visitPackages(Packages& packages);
    virtual void visitProcedure(Procedure& procedure);
    virtual void visitProcedures(Procedures& procedures);
    virtual void visitRelation(Relation& relation);
    virtual void visitRole(Role& role);
    virtual void visitRoles(Roles& roles);
    virtual void visitRoot(Root& root);
    virtual void visitServer(Server& server);
    virtual void visitSysIndices(SysIndices& indices);
    virtual void visitUsrIndices(UsrIndices& indices);
    virtual void visitSysDomains(SysDomains& domains);
    virtual void visitSysPackages(SysPackages& packages);
    virtual void visitSysRoles(SysRoles& sysRoles);
    virtual void visitSysTable(SysTable& table);
    virtual void visitSysTables(SysTables& sysTables);
    virtual void visitTable(Table& table);
    virtual void visitTables(Tables& tables);
    virtual void visitTrigger(Trigger& trigger);
    virtual void visitTriggers(Triggers& triggers);
    virtual void visitUDF(UDF& function);
    virtual void visitUDFs(UDFs& functions);
    virtual void visitUser(User& user);
    virtual void visitUsers(Users& users);
    virtual void visitUniqueConstraint(UniqueConstraint& unq);
    virtual void visitView(View& view);
    virtual void visitViews(Views& views);
};

#endif //FR_METADATAITEMVISITOR_H
