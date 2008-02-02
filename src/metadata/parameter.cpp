/*
  Copyright (c) 2004-2008 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <sstream>

#include "core/Visitor.h"
#include "metadata/column.h"
#include "metadata/domain.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/parameter.h"
//-----------------------------------------------------------------------------
Parameter::Parameter(wxString source, int parameterType)
    : Column()
{
    Column::Init(true, source, wxT(""), wxT(""), wxT(""), false);
    typeM = (parameterType == 0) ? ntParameterInput : ntParameterOutput;
}
//-----------------------------------------------------------------------------
Parameter::Parameter()
    : Column()
{
    typeM = ntParameterInput;
}
//-----------------------------------------------------------------------------
wxString Parameter::getPrintableName()
{
    wxString ret;
    ret = (typeM == ntParameterInput ? wxT("in ") : wxT("out ")) + getName_();
    Domain* d = getDomain();
    return ret + wxT(" ") + ((d) ? d->getDatatypeAsString() : getSource());
}
//-----------------------------------------------------------------------------
bool Parameter::isOutputParameter() const
{
    return typeM == ntParameterOutput;
}
//-----------------------------------------------------------------------------
void Parameter::loadDescription()
{
    MetadataItem::loadDescription(
        wxT("select RDB$DESCRIPTION from RDB$PROCEDURE_PARAMETERS ")
        wxT("where RDB$PARAMETER_NAME = ? and RDB$PROCEDURE_NAME = ?"));
}
//-----------------------------------------------------------------------------
void Parameter::saveDescription(wxString description)
{
    MetadataItem::saveDescription(
        wxT("update RDB$PROCEDURE_PARAMETERS set RDB$DESCRIPTION = ? ")
        wxT("where RDB$PARAMETER_NAME = ? and RDB$PROCEDURE_NAME = ?"),
        description);
}
//-----------------------------------------------------------------------------
void Parameter::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitParameter(*this);
}
//-----------------------------------------------------------------------------
