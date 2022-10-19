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

#ifndef FR_HTMLHEADERMETADATAITEMVISITOR_H
#define FR_HTMLHEADERMETADATAITEMVISITOR_H

#include "metadata/metadataitem.h"
#include "metadata/MetadataItemVisitor.h"

class HtmlHeaderMetadataItemVisitor: public MetadataItemVisitor
{
public:
    HtmlHeaderMetadataItemVisitor(std::vector<wxString>& titles);
    virtual ~HtmlHeaderMetadataItemVisitor();

    virtual void visitDatabase(Database& database);
    virtual void visitDBTrigger(DBTrigger& trigger);
    virtual void visitDDLTrigger(DDLTrigger& trigger);
    virtual void visitDMLTrigger(DMLTrigger& trigger);
    virtual void visitDomain(Domain& domain);
    virtual void visitException(Exception& exception);
    virtual void visitFunctionSQL(FunctionSQL& function);
    virtual void visitGenerator(Generator& generator);
    virtual void visitIndex(Index& index);
    virtual void visitPackage(Package& package);
    virtual void visitProcedure(Procedure& procedure);
    virtual void visitRole(Role& role);
    virtual void visitTable(Table& table);
    virtual void visitGTTable(GTTable& table);
    virtual void visitUDF(UDF & function);
    virtual void visitView(View& view);
protected:
    virtual void defaultAction();
private:
    std::vector<wxString>& titlesM;
    void emptyTitles() { titlesM.clear(); };
    // TODO: These should be localizable - see also HtmlTemplateProcessor.
    void addSummary()      { titlesM.push_back("Summary"); }
    void addPrivileges()   { titlesM.push_back("Privileges"); }
    void addTriggers()     { titlesM.push_back("Triggers"); }
    void addConstraints()  { titlesM.push_back("Constraints"); }
    void addIndices()      { titlesM.push_back("Indices"); }
    void addDependencies() { titlesM.push_back("Dependencies"); }
    void addDDL()          { titlesM.push_back("DDL"); }
};

#endif // FR_HTMLHEADERMETADATAITEMVISITOR_H
