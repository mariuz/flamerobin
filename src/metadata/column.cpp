/*
  Copyright (c) 2004-2013 The FlameRobin Development Team

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


  $Id: column.cpp 2240 2013-09-14 20:03:30Z mghie $

*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "config/Config.h"
#include "metadata/collection.h"
#include "metadata/column.h"
#include "metadata/constraints.h"
#include "metadata/domain.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/table.h"

//-----------------------------------------------------------------------------
ColumnBase::ColumnBase(NodeType type, MetadataItem* parent,
        const wxString& name)
    : MetadataItem(type, parent, name), hasDefaultM(false)
{
}
//-----------------------------------------------------------------------------
wxString ColumnBase::getComputedSource() const
{
    return wxEmptyString;
}
//-----------------------------------------------------------------------------
//! retrieve datatype from domain if possible
wxString ColumnBase::getDatatype(bool useConfig)
{
    enum
    {
        showType = 0,
        showFormula,
        showAll
    };
    int flag = (useConfig ? showFormula : showType);
    if (useConfig)
        config().getValue(wxT("ShowComputed"), flag);
    // view columns are all computed and have their source empty
    if (flag == showFormula && !getComputedSource().empty())
        return getComputedSource();

    wxString ret;
    DomainPtr d = getDomain();
    wxString datatype(d ? d->getDatatypeAsString() : sourceM);

    enum
    {
        showDatatype = 0,
        showDomain,
        showBoth
    };
    int show = (useConfig ? showBoth : showDatatype);
    if (useConfig)
        config().getValue(wxT("ShowDomains"), show);

    if (!d || d->isSystem() || show == showBoth || show == showDatatype)
        ret += datatype;

    if (d && !d->isSystem() && (show == showBoth || show == showDomain))
    {
        if (!ret.empty())
            ret += wxT(" ");
        ret += wxT("(") + d->getName_() + wxT(")");
    }

    if (flag == showAll && !getComputedSource().empty())
        ret += wxT(" (") + getComputedSource() + wxT(")");
    return ret;
}
//-----------------------------------------------------------------------------
DomainPtr ColumnBase::getDomain() const
{
    DatabasePtr db = getDatabase();
    return (db) ? db->getDomain(sourceM) : DomainPtr();
}
//-----------------------------------------------------------------------------
bool ColumnBase::getDefault(GetColumnDefaultType type, wxString& value) const
{
    if (hasDefaultM)
    {
        value = defaultM;
        return true;
    }
    if (type == ReturnDomainDefault)
    {
        if (DomainPtr d = getDomain())
        {
            if (d->getDefault(value))
                return true;
        }
    }
    value = wxEmptyString;
    return false;
}
//-----------------------------------------------------------------------------
wxString ColumnBase::getSource() const
{
    return sourceM;
}
//-----------------------------------------------------------------------------
template<typename T>
inline void setIfChanged(T& value, const T& newValue, bool& changed)
{
    if (value != newValue)
    {
        value = newValue;
        changed = true;
    }
}
//-----------------------------------------------------------------------------
void ColumnBase::initialize(const wxString& source, bool nullable,
    const wxString& defaultValue, bool hasDefault, bool hasDescription)
{
    bool changed = false;
    setIfChanged(sourceM, source.Strip(wxString::both), changed);
    setIfChanged(defaultM, Domain::trimDefaultValue(defaultValue), changed);
    setIfChanged(hasDefaultM, hasDefault, changed);
    setIfChanged(nullableM, nullable, changed);
    if (!hasDescription)
        setDescriptionIsEmpty();
    if (changed)
        notifyObservers();
}
//-----------------------------------------------------------------------------
bool ColumnBase::isNullable(GetColumnNullabilityType type) const
{
    if (!nullableM)
        return false;
    if (type == CheckDomainNullability)
    {
        if (DomainPtr d = getDomain())
            return d->isNullable();
    }
    return true;
}
//-----------------------------------------------------------------------------
Column::Column(Relation* relation, const wxString& name)
    : ColumnBase(ntColumn, relation, name)
{
}
//-----------------------------------------------------------------------------
void Column::initialize(const wxString& source, const wxString& computedSource,
    const wxString& collation, bool nullable,
    const wxString& defaultValue, bool hasDefault, bool hasDescription)
{
    SubjectLocker lock(this);

    ColumnBase::initialize(source, nullable, defaultValue, hasDefault,
        hasDescription);

    bool changed = false;
    setIfChanged(computedSourceM, computedSource, changed);
    setIfChanged(collationM, collation.Strip(wxString::both), changed);
    if (changed)
        notifyObservers();
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
    for (ColumnConstraint::const_iterator it = key->begin(); it != key->end();
        ++it)
    {
        if ((*it) == getName_())
            return true;
    }
    return false;
}
//-----------------------------------------------------------------------------
bool Column::isForeignKey() const
{
    Table* t = getTable();
    if (!t) // view/SP
        return false;
    std::vector<ForeignKey> *fks = t->getForeignKeys();
    if (!fks)
        return false;
    for (std::vector<ForeignKey>::iterator it = fks->begin();
        it != fks->end(); ++it)
    {
        for (std::vector<wxString>::const_iterator c2 = (*it).begin();
            c2 != (*it).end(); ++c2)
        {
            if ((*c2) == getName_())
                return true;
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
bool Column::isString() const
{
    DomainPtr d = getDomain();
    return (d ? d->isString() : false);
}
//-----------------------------------------------------------------------------
Table* Column::getTable() const
{
    return dynamic_cast<Table*>(getParent());
}
//-----------------------------------------------------------------------------
wxString Column::getComputedSource() const
{
    return computedSourceM;
}
//-----------------------------------------------------------------------------
wxString Column::getCollation() const
{
    return collationM;
}
//-----------------------------------------------------------------------------
const wxString Column::getTypeName() const
{
    return wxT("COLUMN");
}
//-----------------------------------------------------------------------------
wxString Column::getDropSqlStatement() const
{
    Table* t = getTable();
    if (t == 0)
        return wxEmptyString;
    return wxT("ALTER TABLE ") + t->getQuotedName() + wxT(" DROP ") + getQuotedName();
}
//-----------------------------------------------------------------------------
void Column::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitColumn(*this);
}
//-----------------------------------------------------------------------------
