//-----------------------------------------------------------------------------
/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2006 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/
//-----------------------------------------------------------------------------

#ifndef FR_CREATEDDLVISITOR_H
#define FR_CREATEDDLVISITOR_H

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

public:
    CreateDDLVisitor();
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
