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

#include <vector>

#include "core/FRError.h"
#include "core/ProgressIndicator.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "metadata/Index.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/database.h"
#include "sql/SqlTokenizer.h"

void Index::loadProperties()
{
    setPropertiesLoaded(false);
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = db->getCharsetConverter();


    IBPP::Statement& st1 = loader->getStatement(
        "SELECT i.rdb$index_name, i.rdb$unique_flag, i.rdb$index_inactive, "
        " i.rdb$index_type, i.rdb$statistics, "
        " s.rdb$field_name, rc.rdb$constraint_name, i.rdb$expression_source "
        " from rdb$indices i "
        " left join rdb$index_segments s on i.rdb$index_name = s.rdb$index_name "
        " left join rdb$relation_constraints rc "
        "   on rc.rdb$index_name = i.rdb$index_name "
        " where i.rdb$index_name = ? "
        " order by i.rdb$index_name, s.rdb$field_position "
    );

    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    Index* i = 0;
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        wxString ixname(std2wxIdentifier(s, converter));

        short unq, inactive, type;
        if (st1->IsNull(2))     // null = non-unique
            unq = 0;
        else
            st1->Get(2, unq);
        uniqueFlagM = unq == 1;
        if (st1->IsNull(3))     // null = active
            inactive = 0;
        else
            st1->Get(3, inactive);
        activeM = inactive == 0;
        if (st1->IsNull(4))     // null = ascending
            type = 0;
        else
            st1->Get(4, type);
        indexTypeM = type == 0 ? itAscending : itDescending;
        if (st1->IsNull(5))     // this can happen, see bug #1825725
            statisticsM = -1;
        else
            st1->Get(5, statisticsM);

        st1->Get(6, s);
        wxString fname(std2wxIdentifier(s, converter));
        wxString expression;
        readBlob(st1, 8, expressionM, converter);

        if (i && i->getName_() == ixname)
            i->getSegments()->push_back(fname);
        else
        {
            /*Index x(
                unq == 1,
                inactive == 0,
                type == 0,
                statistics,
                !st1->IsNull(7),
                expression
            );
            indicesM.push_back(x);*/
            //i = &indicesM.back();
            //i->setName_(ixname);
            /*i->*/getSegments()->push_back(fname);
            //i->setParent(this);
        }

    }

    setPropertiesLoaded(true);
}

Index::Index(DatabasePtr database, const wxString& name)
    : MetadataItem(ntIndex, database.get(), name), activeM(true)
{
}

Index::Index(bool unique, bool active, bool ascending, double statistics,
        bool system, wxString expression)
    : MetadataItem(ntIndex), isSystemM(system), uniqueFlagM(unique),
        activeM(active), indexTypeM(ascending ? itAscending : itDescending),
        statisticsM(statistics), segmentsM(), expressionM(expression)
{
}

bool Index::isSystem() const
{
    return isSystemM;
}

void Index::setActive(bool active)
{
    activeM = active;
}

bool Index::getActive()
{
    return activeM;
}

bool Index::isActive()
{
    return getActive();
}

bool Index::isUnique() const
{
    return uniqueFlagM;
}

double Index::getStatistics()
{
    return statisticsM;
}

std::vector<wxString> *Index::getSegments()
{
    return &segmentsM;
}

wxString Index::getFieldsAsString()
{
    if (!expressionM.IsEmpty())
        return expressionM;
    else
    {
        wxString retval;
        for (std::vector<wxString>::iterator it = segmentsM.begin(); 
            it != segmentsM.end(); ++it)
        {
            if (!retval.empty())
                retval += ", ";
            retval += (*it);
        }
        return retval;
    }
}

Index::IndexType Index::getIndexType()
{
    return indexTypeM;
}

const wxString Index::getTypeName() const
{
    return "INDEX";
}

bool Index::hasColumn(wxString segment) const
{
    if (!expressionM.IsEmpty())
        return expressionM.compare(segment) == 0;
    else
    {
        wxString retval;
        for (std::vector<wxString>::const_iterator it = segmentsM.begin();
            it != segmentsM.end(); ++it)
        {
            if ((*it).compare(segment) == 0)
                return true;
        }
    }
    return false;
}

wxString Index::getExpression() const
{
    return expressionM;
}

void Index::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitIndex(*this);
}

void Indices::loadChildren()
{
    load(0);
}

Indices::Indices(DatabasePtr database)
    : MetadataCollection<Index>(ntIndices, database, _("Indices")) 
{
}

void Indices::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitIndices(*this);
}

void Indices::load(ProgressIndicator* progressIndicator)
{
    DatabasePtr db = getDatabase();
    wxString stmt = "select a.rdb$index_name from rdb$indices a "
            " where (rdb$system_flag = 0 or rdb$system_flag is null) "
            " order by 1 ";
    setItems(db->loadIdentifiers(stmt, progressIndicator));
    
    stmt = "select a.rdb$index_name from rdb$indices a "
        " where (rdb$system_flag = 0 or rdb$system_flag is null) and a.rdb$index_inactive = 1 "
        " order by 1 ";
    setInactiveItems(db->loadIdentifiers(stmt, progressIndicator));
}

const wxString Indices::getTypeName() const
{
    return "INDICES_COLLECTION";
}

void SysIndices::loadChildren()
{
    load(0);
}

SysIndices::SysIndices(DatabasePtr database)
    : MetadataCollection<Index>(ntSysIndices, database, _("System Indices"))
{
}

void SysIndices::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitSysIndices(*this);
}

void SysIndices::load(ProgressIndicator* progressIndicator)
{
    DatabasePtr db = getDatabase();
    wxString stmt = "select a.rdb$index_name from rdb$indices a "
        "   left join rdb$relation_constraints b on b.rdb$index_name = a.rdb$index_name "
        " where (rdb$system_flag = 0 or rdb$system_flag is null) "
        "   and b.rdb$index_name is not null "
        " order by 1 ";
    setItems(db->loadIdentifiers(stmt, progressIndicator));

    stmt = "select a.rdb$index_name from rdb$indices a "
        "   left join rdb$relation_constraints b on b.rdb$index_name = a.rdb$index_name "
        " where (rdb$system_flag = 0 or rdb$system_flag is null) and a.rdb$index_inactive = 1 "
        "   and b.rdb$index_name is not null "
        " order by 1 ";
    setInactiveItems(db->loadIdentifiers(stmt, progressIndicator));
}

const wxString SysIndices::getTypeName() const
{
    return "SYSINDICES_COLLECTION";
}

void UsrIndices::loadChildren()
{
    load(0);
}

UsrIndices::UsrIndices(DatabasePtr database)
    : MetadataCollection<Index>(ntUsrIndices, database, _("Indices"))
{
}

void UsrIndices::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitUsrIndices(*this);
}

void UsrIndices::load(ProgressIndicator* progressIndicator)
{
    DatabasePtr db = getDatabase();
    wxString stmt = "select a.rdb$index_name from rdb$indices a "
        "   left join rdb$relation_constraints b on b.rdb$index_name = a.rdb$index_name "
        " where (rdb$system_flag = 0 or rdb$system_flag is null) "
        "   and b.rdb$index_name is null "
        " order by 1 ";
    setItems(db->loadIdentifiers(stmt, progressIndicator));

    stmt = "select a.rdb$index_name from rdb$indices a "
        "   left join rdb$relation_constraints b on b.rdb$index_name = a.rdb$index_name "
        " where (rdb$system_flag = 0 or rdb$system_flag is null) and a.rdb$index_inactive = 1 "
        "   and b.rdb$index_name is null "
        " order by 1 ";
    setInactiveItems(db->loadIdentifiers(stmt, progressIndicator));
}


const wxString UsrIndices::getTypeName() const
{
    return "USRINDICES_COLLECTION";
}
