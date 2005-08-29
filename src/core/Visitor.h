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

  Contributor(s): Milan Babuskov.
*/
//-----------------------------------------------------------------------------

#ifndef FR_VISITOR_H
#define FR_VISITOR_H

// [GoF] Visitor pattern. <Visitor>

// It is possible to use forward declaration here
// but then you will have to include this files in each
// <concrete_visitor>.cpp (or .h) file

#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/exception.h"
#include "metadata/function.h"
#include "metadata/generator.h"
#include "metadata/procedure.h"
#include "metadata/parameter.h"
#include "metadata/role.h"
#include "metadata/server.h"
#include "metadata/table.h"
#include "metadata/trigger.h"
#include "metadata/view.h"
#include "metadata/metadataitem.h"
#include "metadata/item.h"
//-----------------------------------------------------------------------------
class Visitor
{
protected:
	//! aviod the need to implement visit actions for each type in descendent classes
	virtual void defaultAction() {};

public:
    virtual void visit(Column&) { defaultAction(); };
    virtual void visit(Database&) { defaultAction(); };
    virtual void visit(Domain&) { defaultAction(); };
    virtual void visit(Exception&) { defaultAction(); };
    virtual void visit(Function&) { defaultAction(); };
    virtual void visit(Generator&) { defaultAction(); };
    virtual void visit(Procedure&) { defaultAction(); };
    virtual void visit(Parameter&) { defaultAction(); };
    virtual void visit(Role&) { defaultAction(); };
    virtual void visit(Server&) { defaultAction(); };
    virtual void visit(Table&) { defaultAction(); };
    virtual void visit(Trigger&) { defaultAction(); };
    virtual void visit(View&) { defaultAction(); };
	virtual void visit(MetadataItem&) { defaultAction(); };

	virtual void visit(Item&) { defaultAction(); };
public:
	Visitor() {};
    virtual ~Visitor() {};
};
//-----------------------------------------------------------------------------
#endif //FR_VISITOR_H
