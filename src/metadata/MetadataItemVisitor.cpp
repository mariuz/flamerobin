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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "MetadataItemVisitor.h"
//-----------------------------------------------------------------------------
MetadataItemVisitor::MetadataItemVisitor()
{
};
//-----------------------------------------------------------------------------
MetadataItemVisitor::~MetadataItemVisitor()
{
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(Column&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(Database&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(Domain&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(Exception&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(Function&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(Generator&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(Procedure&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(Parameter&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(Role&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(Root&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(Server&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(Table&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(Trigger&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(View&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
void MetadataItemVisitor::visit(MetadataItem&)
{
    defaultAction();
};
//-----------------------------------------------------------------------------
