/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

#include "metadata/MetadataItemVisitor.h"
#include "metadata/parameter.h"
#include "metadata/procedure.h"
//-----------------------------------------------------------------------------
// TODO: pass ProcedurePtr instead of Procedure*
Parameter::Parameter(Procedure* procedure, const wxString& name)
    : Column(procedure, name), outputParameterM(false), parameterMechanismM(-1)
{
    setType(ntParameter);
}
//-----------------------------------------------------------------------------
void Parameter::initialize(wxString source, int parameterType, int mechanism)
{
    SubjectLocker lock(this);

    Column::initialize(false, source, wxEmptyString, wxEmptyString,
        wxEmptyString, false);
    bool changed = false;
    if (parameterMechanismM != mechanism)
    {
        parameterMechanismM = mechanism;
        changed= true;
    }
    bool outputParam = parameterType != 0;
    if (outputParameterM != outputParam)
    {
        outputParameterM = outputParam;
        changed= true;
    }
    if (changed)
        notifyObservers();
}
//-----------------------------------------------------------------------------
bool Parameter::isOutputParameter() const
{
    return outputParameterM;
}
//-----------------------------------------------------------------------------
int Parameter::getMechanism() const
{
    return parameterMechanismM;
}
//-----------------------------------------------------------------------------
void Parameter::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitParameter(*this);
}
//-----------------------------------------------------------------------------
