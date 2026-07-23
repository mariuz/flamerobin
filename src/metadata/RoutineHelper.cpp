/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "metadata/RoutineHelper.h"
#include "metadata/procedure.h"
#include "metadata/function.h"
#include "metadata/parameter.h"
#include "metadata/package.h"

wxString RoutineHelper::getRoutineCalltip(MetadataItem* item)
{
    if (!item)
        return wxEmptyString;

    Procedure* p = dynamic_cast<Procedure*>(item);
    if (p)
    {
        p->ensureChildrenLoaded();
        wxString inParams;
        wxString outParams;

        for (auto it = p->begin(); it != p->end(); ++it)
        {
            Parameter* param = (*it).get();
            if (!param) continue;

            if (param->isOutputParameter())
            {
                if (!outParams.IsEmpty()) outParams << ", ";
                outParams << param->getName_() << " " << param->getDatatype();
            }
            else
            {
                if (!inParams.IsEmpty()) inParams << ", ";
                inParams << param->getName_() << " " << param->getDatatype();
                wxString defVal;
                if (param->getDefault(IgnoreDomainDefault, defVal) && !defVal.IsEmpty())
                    inParams << " DEFAULT " << defVal;
            }
        }

        wxString tip = p->getName_() + "(" + inParams + ")";
        if (!outParams.IsEmpty())
            tip << " RETURNS (" << outParams << ")";
        return tip;
    }

    Function* f = dynamic_cast<Function*>(item);
    if (f)
    {
        f->ensureChildrenLoaded();
        wxString inParams;
        for (auto it = f->begin(); it != f->end(); ++it)
        {
            Parameter* param = (*it).get();
            if (!param || param->isOutputParameter()) continue;

            if (!inParams.IsEmpty()) inParams << ", ";
            inParams << param->getName_() << " " << param->getDatatype();
            wxString defVal;
            if (param->getDefault(IgnoreDomainDefault, defVal) && !defVal.IsEmpty())
                inParams << " DEFAULT " << defVal;
        }

        return f->getName_() + "(" + inParams + ") RETURNS FUNCTION";
    }

    Package* pkg = dynamic_cast<Package*>(item);
    if (pkg)
    {
        pkg->ensureChildrenLoaded();
        wxString tip = "Package " + pkg->getName_() + ":\n";
        std::vector<MetadataItem*> children;
        pkg->getChildren(children);
        for (auto child : children)
        {
            wxString childTip = getRoutineCalltip(child);
            if (!childTip.IsEmpty())
                tip << "  - " << childTip << "\n";
        }
        return tip;
    }

    return item->getName_();
}

wxString RoutineHelper::getRoutineExecutionTemplate(MetadataItem* item)
{
    if (!item)
        return wxEmptyString;

    Procedure* p = dynamic_cast<Procedure*>(item);
    if (p)
    {
        p->ensureChildrenLoaded();
        wxString inPlaceholders;
        wxString outCols;

        for (auto it = p->begin(); it != p->end(); ++it)
        {
            Parameter* param = (*it).get();
            if (!param) continue;

            if (param->isOutputParameter())
            {
                if (!outCols.IsEmpty()) outCols << ", ";
                outCols << param->getQuotedName();
            }
            else
            {
                if (!inPlaceholders.IsEmpty()) inPlaceholders << ", ";
                inPlaceholders << ":" << param->getName_() << " /* " << param->getDatatype() << " */";
            }
        }

        wxString sql;
        if (!outCols.IsEmpty())
        {
            sql << "SELECT " << outCols << "\nFROM " << p->getQuotedName();
            if (!inPlaceholders.IsEmpty())
                sql << "(" << inPlaceholders << ")";
            sql << ";\n";
        }
        else
        {
            sql << "EXECUTE PROCEDURE " << p->getQuotedName();
            if (!inPlaceholders.IsEmpty())
                sql << "(" << inPlaceholders << ")";
            sql << ";\n";
        }
        return sql;
    }

    Function* f = dynamic_cast<Function*>(item);
    if (f)
    {
        f->ensureChildrenLoaded();
        wxString inPlaceholders;
        for (auto it = f->begin(); it != f->end(); ++it)
        {
            Parameter* param = (*it).get();
            if (!param || param->isOutputParameter()) continue;

            if (!inPlaceholders.IsEmpty()) inPlaceholders << ", ";
            inPlaceholders << ":" << param->getName_() << " /* " << param->getDatatype() << " */";
        }

        wxString sql;
        sql << "SELECT " << f->getQuotedName() << "(" << inPlaceholders << ")\nFROM RDB$DATABASE;\n";
        return sql;
    }

    return wxEmptyString;
}
