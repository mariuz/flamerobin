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
void MetadataItemVisitor::visit(ForeignKey&)
{
    defaultAction();
}
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
void MetadataItemVisitor::visit(PrimaryKeyConstraint&)
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
void MetadataItemVisitor::visit(UniqueConstraint&)
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
