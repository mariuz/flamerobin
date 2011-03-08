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

#ifndef FR_CONTEXTMENUMETADATAITEMVISITOR_H
#define FR_CONTEXTMENUMETADATAITEMVISITOR_H

#include "metadata/MetadataClasses.h"
#include "metadata/MetadataItemVisitor.h"
//-----------------------------------------------------------------------------
class wxMenu;
//-----------------------------------------------------------------------------
class ContextMenuMetadataItemVisitor : public MetadataItemVisitor
{
public:
    explicit ContextMenuMetadataItemVisitor(wxMenu* menu);
    virtual ~ContextMenuMetadataItemVisitor();

    virtual void visitColumn(Column& column);
    virtual void visitDatabase(Database& database);
    virtual void visitDomain(Domain& domain);
    virtual void visitDomains(Domains& domains);
    virtual void visitException(Exception& exception);
    virtual void visitExceptions(Exceptions& exceptions);
    virtual void visitFunction(Function& function);
    virtual void visitFunctions(Functions& functions);
    virtual void visitGenerator(Generator& generator);
    virtual void visitGenerators(Generators& generators);
    virtual void visitProcedure(Procedure& procedure);
    virtual void visitProcedures(Procedures& procedures);
    virtual void visitRole(Role& role);
    virtual void visitRoles(Roles& roles);
    virtual void visitRoot(Root& root);
    virtual void visitSysTables(SysTables& tables);
    virtual void visitServer(Server& server);
    virtual void visitTable(Table& table);
    virtual void visitTables(Tables& tables);
    virtual void visitTrigger(Trigger& trigger);
    virtual void visitTriggers(Triggers& triggers);
    virtual void visitView(View& view);
    virtual void visitViews(Views& views);

private:
    wxMenu* menuM;
    // helper member functions to add menu items and separators
    void addAlterItem(MetadataItem& metadataItem);
    void addCreateItem();
    void addDropItem(MetadataItem& metadataItem);
    void addGenerateScriptMenu(MetadataItem& metadataItem);
    void addPropertiesItem();
    void addRefreshItem();
    void addSelectItem();
    void addShowColumnsItem();
    void addSeparator();
};
//-----------------------------------------------------------------------------
#endif //FR_CONTEXTMENUMETADATAITEMVISITOR_H
