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

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "gui/HtmlHeaderMetadataItemVisitor.h"
#include "metadata/database.h"


HtmlHeaderMetadataItemVisitor::HtmlHeaderMetadataItemVisitor(
    std::vector<wxString>& titles)
    : MetadataItemVisitor(), titlesM(titles)
{
}

HtmlHeaderMetadataItemVisitor::~HtmlHeaderMetadataItemVisitor()
{
}

void HtmlHeaderMetadataItemVisitor::visitDatabase(Database& database)
{
    emptyTitles();
    addSummary();
    // database triggers have been introduced in Firebird 2.1 (ODS 11.1)
    if (database.getInfo().getODSVersionIsHigherOrEqualTo(11, 1))
        addTriggers();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitDomain(Domain& /*domain*/)
{
    emptyTitles();
    addSummary();
    addPrivileges();
    // TODO: Support dependencies retrieval in MetadataItem::getDependencies().
    //addDependencies();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitException(Exception& /*exception*/)
{
    emptyTitles();
    addSummary();
    addPrivileges();
    addDependencies();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitFunctionSQL(FunctionSQL& /*function*/)
{
    emptyTitles();
    addSummary();
    addPrivileges();
    addDependencies();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitUDF(UDF& /*function*/)
{
    emptyTitles();
    addSummary();
    addPrivileges();
    addDependencies();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitGenerator(Generator& /*generator*/)
{
    emptyTitles();
    addSummary();
    addPrivileges();
    addDependencies();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitIndex(Index& /*index*/)
{
    emptyTitles();
    addSummary();
    addPrivileges();
    addDependencies();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitPackage(Package& /*procedure*/)
{
    emptyTitles();
    addSummary();
    addPrivileges();
    addDependencies();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitProcedure(Procedure& /*procedure*/)
{
    emptyTitles();
    addSummary();
    addPrivileges();
    addDependencies();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitRole(Role& /*role*/)
{
    emptyTitles();
    addSummary();
    addPrivileges();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitTable(Table& /*table*/)
{
    emptyTitles();
    addSummary();
    addConstraints();
    addIndices();
    addTriggers();
    addPrivileges();
    addDependencies();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitGTTable(GTTable& /*table*/)
{
	emptyTitles();
	addSummary();
	addConstraints();
	addIndices();
	addTriggers();
	addPrivileges();
	addDependencies();
	addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitDBTrigger(DBTrigger& /*trigger*/)
{
    emptyTitles();
    addSummary();
    addDependencies();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitDDLTrigger(DDLTrigger& /*trigger*/)
{
    emptyTitles();
    addSummary();
    addDependencies();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitDMLTrigger(DMLTrigger& /*trigger*/)
{
    emptyTitles();
    addSummary();
    addDependencies();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::visitView(View& /*view*/)
{
    emptyTitles();
    addSummary();
    addTriggers();
    addPrivileges();
    addDependencies();
    addDDL();
}

void HtmlHeaderMetadataItemVisitor::defaultAction()
{
    emptyTitles();
} 

