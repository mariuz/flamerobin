/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "config/Config.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/parameter.h"
#include "metadata/procedure.h"

// TODO: pass ProcedurePtr instead of Procedure*
Parameter::Parameter(MetadataItem* metadataitem, const wxString& name)
    : ColumnBase(ntParameter, metadataitem, name), outputParameterM(false),
        parameterMechanismM(-1)
{
}

void Parameter::initialize(const wxString& source, int parameterType,
    int mechanism, bool nullable, const wxString& defaultValue,
    bool hasDefault, bool hasDescription, wxString& relation, wxString& field)
{
    SubjectLocker lock(this);

    ColumnBase::initialize(source, nullable, defaultValue, hasDefault,
        hasDescription);

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
    relationM = relation;
    fieldM = field;
    if (changed)
        notifyObservers();
}

bool Parameter::isOutputParameter() const
{
    return outputParameterM;
}

int Parameter::getMechanism() const
{
    return parameterMechanismM;
}

const wxString Parameter::getTypeName() const
{
    return "PARAMETER";
}

void Parameter::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitParameter(*this);
}

wxString Parameter::getTypeOf(bool large)
{
    if (relationM.empty()) {
        if (getDomain()->isSystem())
            return "";
        else
            if (getSource() == "") {
                return (large ? "TYPE OF " : "") + getDomain()->getDatatypeAsString();
            }
            else
                return (large ? "TYPE OF " : "") + getSource();
    }
    else
        return (large ? "TYPE OF COLUMN " : "") + relationM + "." + fieldM;
}

bool Parameter::isTypeOf()
{
    return !relationM.IsEmpty();
}

