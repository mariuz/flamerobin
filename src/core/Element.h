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

  The Initial Developer of the Original Code is Nando Dessena.

  Portions created by the original developer
  are Copyright (C) 2005 Nando Dessena.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/
//-----------------------------------------------------------------------------
#ifndef FR_ELEMENT_H
#define FR_ELEMENT_H

// [GoF] Visitor pattern. Abstract generic Element.

class Visitor;
//-----------------------------------------------------------------------------
class Element
{
public:
    virtual void acceptVisitor(Visitor* visitor);
};
//-----------------------------------------------------------------------------
#endif //FR_ELEMENT_H
