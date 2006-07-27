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

#include "collection.h"
#include "column.h"
#include "config/Config.h"
#include "constraints.h"
#include "database.h"
#include "domain.h"
#include "MetadataItemVisitor.h"
//-----------------------------------------------------------------------------
//! new undefined column
Column::Column()
    : MetadataItem()
{
    typeM = ntColumn;
}
//-----------------------------------------------------------------------------
//! initialize properties
void Column::Init(bool notnull, wxString source, wxString computedSource,
    wxString collation, wxString defaultValue, bool hasDefault)
{
    source.Trim();        // right trim everything
    collation.Trim();
    notnullM = notnull;
    sourceM = source;
    computedSourceM = computedSource;
    collationM = collation;
    defaultM = defaultValue;
    hasDefaultM = hasDefault;
}
//-----------------------------------------------------------------------------
bool Column::isNullable(bool checkDomain) const
{
    if (notnullM)
        return false;
    if (!checkDomain)
        return true;
    Domain *d = getDomain();
    if (d)
        return d->isNullable();
    return true;
}
//-----------------------------------------------------------------------------
bool Column::hasDefault(bool checkDomain) const
{
    if (hasDefaultM)
        return true;
    if (!checkDomain)
        return true;
    Domain *d = getDomain();
    if (d)
        return d->hasDefault();
    return false;
}
//-----------------------------------------------------------------------------
bool Column::isPrimaryKey() const
{
    Table* t = getTable();
    if (!t) // view/SP
        return false;
    ColumnConstraint *key = t->getPrimaryKey();
    if (!key)
        return false;
    for (ColumnConstraint::const_iterator it = key->begin(); it != key->end(); ++it)
        if ((*it) == getName_())
            return true;
    return false;
}
//-----------------------------------------------------------------------------
//! retrieve datatype from domain if possible
wxString Column::getDatatype()
{
    enum
    {
        showType = 0,
        showFormula,
        showAll
    };
    int flag = showFormula;
    config().getValue(wxT("ShowComputed"), flag);
    // view columns are all computed and have their source empty
    if (flag == showFormula && !computedSourceM.empty())
        return computedSourceM;

    wxString ret;
    Domain *d = getDomain();
    wxString datatype;
    if (d)
        datatype = d->getDatatypeAsString();
    else
        datatype = sourceM;

    enum
    {
        showDatatype = 0,
        showDomain,
        showBoth
    };
    int show = showBoth;
    config().getValue(wxT("ShowDomains"), show);

    if (!d || d->isSystem() || show == showBoth || show == showDatatype)
        ret += datatype;

    if (d && !d->isSystem() && (show == showBoth || show == showDomain))
    {
        if (!ret.empty())
            ret += wxT(" ");
        ret += wxT("(") + d->getName_() + wxT(")");
    }

    if (flag == showAll && !computedSourceM.empty())
        ret += wxT(" (") + computedSourceM + wxT(")");
    return ret;
}
//-----------------------------------------------------------------------------
//! printable name = column_name + column_datatype [+ not null]
wxString Column::getPrintableName()
{
    wxString ret = getName_() + wxT(" ") + getDatatype();
    if (notnullM)
        ret += wxT(" not null");
    return ret;
}
//-----------------------------------------------------------------------------
Domain *Column::getDomain() const
{
    Database *d = getDatabase();
    if (!d)
        return 0;
    for (MetadataCollection<Domain>::const_iterator it = d->domainsBegin(); it != d->domainsEnd(); ++it)
        if ((*it).getName_() == sourceM)
            return (Domain *)&(*it);

    // since we haven't find the domain, check the database
    return d->loadMissingDomain(sourceM);
}
//-----------------------------------------------------------------------------
Table* Column::getTable() const
{
    return dynamic_cast<Table*>(getParentObjectOfType(ntTable));
}
//-----------------------------------------------------------------------------
wxString Column::getComputedSource() const
{
    return computedSourceM;
}
//-----------------------------------------------------------------------------
wxString Column::getSource() const
{
    return sourceM;
}
//-----------------------------------------------------------------------------
wxString Column::getCollation() const
{
    return collationM;
}
//-----------------------------------------------------------------------------
wxString Column::getDefault() const
{
    if (defaultM.IsEmpty())
    {
        Domain *d = getDomain();
        if (d)
            return d->getDefault();
    }
    return defaultM;
}
//-----------------------------------------------------------------------------
const wxString Column::getTypeName() const
{
    return wxT("COLUMN");
}
//-----------------------------------------------------------------------------
wxString Column::getDropSqlStatement() const
{
    return wxT("ALTER TABLE ") + getTable()->getQuotedName() + wxT(" DROP ") + getQuotedName();
}
//-----------------------------------------------------------------------------
void Column::loadDescription()
{
    MetadataItem::loadDescription(
        wxT("select RDB$DESCRIPTION from RDB$RELATION_FIELDS ")
        wxT("where RDB$FIELD_NAME = ? and RDB$RELATION_NAME = ?"));
}
//-----------------------------------------------------------------------------
void Column::saveDescription(wxString description)
{
    MetadataItem::saveDescription(
        wxT("update RDB$RELATION_FIELDS set RDB$DESCRIPTION = ? ")
        wxT("where RDB$FIELD_NAME = ? and RDB$RELATION_NAME = ?"),
        description);
}
//-----------------------------------------------------------------------------
void Column::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitColumn(*this);
}
//-----------------------------------------------------------------------------
