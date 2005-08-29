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
  are Copyright (C) 2005 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/
//-----------------------------------------------------------------------------
#ifndef FR_ITEM_H
#define FR_ITEM_H

#include "core/Subject.h"
class Visitor;
//-----------------------------------------------------------------------------
//! Base Item class, currently metadataitems are only descendants, but in the future
//! those could be other things that need Visitor pattern implemented on them.
//! (Database Folders and other visual stuff comes to mind as a possibility)
class Item: public Subject
{
public:
    virtual void accept(Visitor *v);
};
//-----------------------------------------------------------------------------
#endif
