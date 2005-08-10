//------------------------------------------------------------------------------
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

  Contributor(s): Milan Babuskov.
*/
//------------------------------------------------------------------------------
#ifndef FR_CONTEXTMENUVISITOR_H
#define FR_CONTEXTMENUVISITOR_H

#include "visitor.h"

class wxMenu;
//------------------------------------------------------------------------------
class ContextMenuVisitor : public Visitor
{
public:
    explicit ContextMenuVisitor(wxMenu* menu);
    virtual ~ContextMenuVisitor();

    virtual void visit(Column&);
    virtual void visit(Database&);
    virtual void visit(Domain&);
    virtual void visit(Exception&);
    virtual void visit(Function&);
    virtual void visit(Generator&);
    virtual void visit(Procedure&);
    virtual void visit(Role&);
    virtual void visit(Server&);
    virtual void visit(Table&);
    virtual void visit(Trigger&);
    virtual void visit(View&);
	virtual void visit(MetadataItem&);

private:
    wxMenu* menuM;

	void addRegularObjectMenu();
	void addSelectMenu(bool isTable = false);
};
//------------------------------------------------------------------------------
#endif //FR_CONTEXTMENUVISITOR_H
