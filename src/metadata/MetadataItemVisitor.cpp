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

#include "metadata/MetadataItemVisitor.h"

MetadataItemVisitor::MetadataItemVisitor()
{
}

MetadataItemVisitor::~MetadataItemVisitor()
{
}

void MetadataItemVisitor::visitCharacterSet(CharacterSet&)
{
    defaultAction();
}

void MetadataItemVisitor::visitCharacterSets(CharacterSets&)
{
    defaultAction();
}

void MetadataItemVisitor::visitCollation(Collation&)
{
    defaultAction();
}

void MetadataItemVisitor::visitSysCollations(SysCollations&)
{
    defaultAction();
}

void MetadataItemVisitor::visitCollations(Collations&)
{
    defaultAction();
}


void MetadataItemVisitor::visitColumn(Column&)
{
    defaultAction();
}

void MetadataItemVisitor::visitDatabase(Database&)
{
    defaultAction();
}

void MetadataItemVisitor::visitDomain(Domain&)
{
    defaultAction();
}

void MetadataItemVisitor::visitDomains(Domains&)
{
    defaultAction();
}

void MetadataItemVisitor::visitException(Exception&)
{
    defaultAction();
}

void MetadataItemVisitor::visitExceptions(Exceptions&)
{
    defaultAction();
}

void MetadataItemVisitor::visitForeignKey(ForeignKey&)
{
    defaultAction();
}

void MetadataItemVisitor::visitFunctionSQL(FunctionSQL&)
{
    defaultAction();
}

void MetadataItemVisitor::visitFunctionSQLs(FunctionSQLs&)
{
    defaultAction();
}

void MetadataItemVisitor::visitUDF(UDF&)
{
    defaultAction();
}

void MetadataItemVisitor::visitUDFs(UDFs&)
{
    defaultAction();
}

void MetadataItemVisitor::visitUser(User&)
{
	defaultAction();
}

void MetadataItemVisitor::visitUsers(Users&)
{
    defaultAction();
}

void MetadataItemVisitor::visitGenerator(Generator&)
{
    defaultAction();
}

void MetadataItemVisitor::visitGenerators(Generators&)
{
    defaultAction();
}

void MetadataItemVisitor::visitIndex(Index&)
{
    defaultAction();
}

void MetadataItemVisitor::visitIndices(Indices&)
{
	defaultAction();
}

void MetadataItemVisitor::visitParameter(Parameter&)
{
    defaultAction();
}

void MetadataItemVisitor::visitPrimaryKeyConstraint(PrimaryKeyConstraint&)
{
    defaultAction();
}

void MetadataItemVisitor::visitPackage(Package&)
{
    defaultAction();
}

void MetadataItemVisitor::visitPackages(Packages&)
{
    defaultAction();
}

void MetadataItemVisitor::visitSysPackages(SysPackages&)
{
    defaultAction();
}

void MetadataItemVisitor::visitProcedure(Procedure&)
{
    defaultAction();
}

void MetadataItemVisitor::visitProcedures(Procedures&)
{
    defaultAction();
}

void MetadataItemVisitor::visitRelation(Relation&)
{
    defaultAction();
}

void MetadataItemVisitor::visitRole(Role&)
{
    defaultAction();
}

void MetadataItemVisitor::visitRoles(Roles&)
{
    defaultAction();
}

void MetadataItemVisitor::visitRoot(Root&)
{
    defaultAction();
}

void MetadataItemVisitor::visitServer(Server&)
{
    defaultAction();
}

void MetadataItemVisitor::visitSysIndices(SysIndices&)
{
    defaultAction();
}

void MetadataItemVisitor::visitUsrIndices(UsrIndices&)
{
	defaultAction();
}

void MetadataItemVisitor::visitSysDomains(SysDomains&)
{
    defaultAction();
}

void MetadataItemVisitor::visitSysRoles(SysRoles&)
{
    defaultAction();
}

void MetadataItemVisitor::visitSysTables(SysTables&)
{
    defaultAction();
}

void MetadataItemVisitor::visitGTTables(GTTables&)
{
    defaultAction();
}


void MetadataItemVisitor::visitTable(Table& table)
{
    visitRelation(*(Relation*)&table);
}

void MetadataItemVisitor::visitSysTable(SysTable& table)
{
    visitTable(*(Table*)&table);
}

void MetadataItemVisitor::visitGTTable(GTTable& table)
{
    visitTable(*(Table*)&table);
}

void MetadataItemVisitor::visitTables(Tables&)
{
    defaultAction();
}

void MetadataItemVisitor::visitTrigger(Trigger&)
{
    defaultAction();
}

void MetadataItemVisitor::visitTriggers(Triggers&)
{
    defaultAction();
}

void MetadataItemVisitor::visitDBTrigger(DBTrigger& trigger)
{
    visitTrigger(*(Trigger*)&trigger);
}

void MetadataItemVisitor::visitDDLTrigger(DDLTrigger& trigger)
{
    visitTrigger(*(Trigger*)&trigger);
}

void MetadataItemVisitor::visitDMLTrigger(DMLTrigger& trigger)
{
    visitTrigger(*(Trigger*)&trigger);
}

void MetadataItemVisitor::visitDMLTriggers(DMLTriggers&)
{
    defaultAction();
}

void MetadataItemVisitor::visitDDLTriggers(DDLTriggers&)
{
    defaultAction();
}

void MetadataItemVisitor::visitDBTriggers(DBTriggers&)
{
    defaultAction();
}

void MetadataItemVisitor::visitUniqueConstraint(UniqueConstraint&)
{
    defaultAction();
}

void MetadataItemVisitor::visitView(View& view)
{
    visitRelation(*(Relation*)&view);
}

void MetadataItemVisitor::visitViews(Views&)
{
    defaultAction();
}

void MetadataItemVisitor::visitMetadataItem(MetadataItem&)
{
    defaultAction();
}

void MetadataItemVisitor::visitMethod(Method&)
{
    defaultAction();
}

