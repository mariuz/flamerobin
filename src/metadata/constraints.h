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

#ifndef FR_CONSTRAINTS_H
#define FR_CONSTRAINTS_H

#include <vector>

#include "metadata/metadataitem.h"

class Relation;
class Table;

class Constraint: public MetadataItem
{
public:
    virtual Table* getTable() const;
    virtual bool isSystem() const;
    virtual const wxString getTypeName() const;
};

class ColumnConstraint: public Constraint
{
    friend class Relation;
    friend class Table;
public:
    typedef std::vector<wxString>::const_iterator const_iterator;
    std::vector<wxString>::const_iterator begin() { return columnsM.begin(); }
    std::vector<wxString>::const_iterator end() { return columnsM.end(); }
    wxString getColumnList(const wxString& separator = ", ",
        const wxString& suffix = "") const;
    bool hasColumn(const wxString& column) const;
    const wxString& getIndexName() const { return indexNameM; }
    std::vector<wxString>& getColumns() { return columnsM; }
protected:
    std::vector<wxString> columnsM;
private:
    wxString indexNameM;
};

//! uniques
class UniqueConstraint: public ColumnConstraint
{
public:
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};

//! primary keys
class PrimaryKeyConstraint: public UniqueConstraint
{
public:
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};

//! checks
class CheckConstraint: public ColumnConstraint
{
    friend class Relation;
    friend class Table;
public:
    const wxString& getSource() const { return sourceM; }
private:
    wxString sourceM;
};

//! foreign keys
class ForeignKey: public ColumnConstraint
{
    friend class Relation;
    friend class Table;
public:
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    wxString getReferencedColumnList() const;
    wxString getJoin(bool quoted) const;
    const wxString& getReferencedTable() const { return referencedTableM; };
    const wxString& getUpdateAction() const { return updateActionM; };
    const wxString& getDeleteAction() const { return deleteActionM; };
    const std::vector<wxString>& getReferencedColumns() const { return referencedColumnsM; };
private:
    wxString referencedTableM;
    std::vector<wxString> referencedColumnsM;
    wxString updateActionM;
    wxString deleteActionM;
};

#endif
