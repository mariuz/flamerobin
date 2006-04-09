/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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

#ifndef FR_CREATEDDLVISITOR_H
#define FR_CREATEDDLVISITOR_H

#include "core/ProgressIndicator.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/exception.h"
#include "metadata/function.h"
#include "metadata/generator.h"
#include "metadata/metadataitem.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/parameter.h"
#include "metadata/procedure.h"
#include "metadata/role.h"
#include "metadata/root.h"
#include "metadata/server.h"
#include "metadata/table.h"
#include "metadata/trigger.h"
#include "metadata/view.h"
//-----------------------------------------------------------------------------
class CreateDDLVisitor: public MetadataItemVisitor
{
private:
    wxString sqlM;      // main

    wxString preSqlM;   // used for scripts to create entire database
    wxString postSqlM;  // sometimes it's the same as sqlM, sometimes not

    ProgressIndicator* progressIndicatorM;

public:
    CreateDDLVisitor(ProgressIndicator* progressIndicator = 0);
    virtual ~CreateDDLVisitor();
    wxString getSql() const;
    wxString getPrefixSql() const;
    wxString getSuffixSql() const;

    virtual void visit(Column& dolumn);
    virtual void visit(Database& database);
    virtual void visit(Domain& domain);
    virtual void visit(Exception& exception);
    virtual void visit(Function& function);
    virtual void visit(Generator& generator);
    virtual void visit(Procedure& procedure);
    virtual void visit(Parameter& parameter);
    virtual void visit(Role& role);
    virtual void visit(Root& root);
    virtual void visit(Server& server);
    virtual void visit(Table& table);
    virtual void visit(Trigger& trigger);
    virtual void visit(View& view);
    virtual void visit(MetadataItem& metadataItem);
};
//-----------------------------------------------------------------------------
#endif
