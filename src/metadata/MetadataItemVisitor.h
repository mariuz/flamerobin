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

  The Initial Developer of the Original Code is Gregory Sapunkov.

  Portions created by the original developer
  are Copyright (C) 2004 Gregory Sapunkov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena
*/
//-----------------------------------------------------------------------------

#ifndef FR_METADATAITEMVISITOR_H
#define FR_METADATAITEMVISITOR_H

// [GoF] Visitor pattern. Abstract Visitor for metadata items.

#include "core/Visitor.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/exception.h"
#include "metadata/function.h"
#include "metadata/generator.h"
#include "metadata/metadataitem.h"
#include "metadata/parameter.h"
#include "metadata/procedure.h"
#include "metadata/role.h"
#include "metadata/root.h"
#include "metadata/server.h"
#include "metadata/table.h"
#include "metadata/trigger.h"
#include "metadata/view.h"
//-----------------------------------------------------------------------------
class MetadataItemVisitor: public Visitor
{
public:
    MetadataItemVisitor();
    virtual ~MetadataItemVisitor();

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
#endif //FR_METADATAITEMVISITOR_H
