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

#ifndef FR_CONSTRAINTS_H
#define FR_CONSTRAINTS_H
//-----------------------------------------------------------------------------
#include <vector>

#include "metadata/metadataitem.h"
class Table;
//-----------------------------------------------------------------------------
// These could all be simple "struct"s but we want to add some functionality later
//
class Constraint: public MetadataItem
{
public:
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    virtual Table* getTable() const;
    virtual bool isSystem() const;
};
//-----------------------------------------------------------------------------
//! primary keys and uniques
class ColumnConstraint: public Constraint
{
public:
    typedef std::vector<wxString>::const_iterator const_iterator;
    wxString indexName; // needed for DDL extraction
    std::vector<wxString> columnsM;

    wxString getColumnList() const;
    const_iterator begin() { return columnsM.begin(); };
    const_iterator end() { return columnsM.end(); };
};
//-----------------------------------------------------------------------------
//! checks
class CheckConstraint: public Constraint
{
public:
    wxString sourceM;
};
//-----------------------------------------------------------------------------
//! foreign keys
class ForeignKey: public ColumnConstraint
{
public:
    wxString referencedTableM;                   // referenced table
    std::vector<wxString> referencedColumnsM;    // referenced columns
    wxString updateActionM;
    wxString deleteActionM;
    wxString getReferencedColumnList() const;
    wxString getJoin(bool quoted) const;
};
//-----------------------------------------------------------------------------
#endif
