/*
  Copyright (c) 2004-2015 The FlameRobin Development Team

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


#ifndef FR_COLUMN_H
#define FR_COLUMN_H

#include "metadata/metadataitem.h"

enum GetColumnDefaultType { ReturnDomainDefault, IgnoreDomainDefault };
enum GetColumnNullabilityType { CheckDomainNullability, IgnoreDomainNullability };

class ColumnBase: public MetadataItem
{
private:
    wxString defaultM;
    bool hasDefaultM;
    bool nullableM;
    wxString sourceM;
protected:
    virtual wxString getComputedSource() const;
    void initialize(const wxString& source, bool nullable,
        const wxString& defaultValue, bool hasDefault, bool hasDescription);
public:
    ColumnBase(NodeType type, MetadataItem* parent, const wxString& name);

    wxString getDatatype(bool useConfig = true);
    DomainPtr getDomain() const;
    bool getDefault(GetColumnDefaultType type, wxString& value) const;
    wxString getSource() const;
    bool isNullable(GetColumnNullabilityType type) const;
};

class Column: public ColumnBase
{
private:
    bool computedM;
    wxString sourceM, computedSourceM, collationM;
public:
    Column(Relation* relation, const wxString& name);

    void initialize(const wxString& source, const wxString& computedSource,
        const wxString& collation, bool nullable,
        const wxString& defaultValue, bool hasDefault, bool hasDescription);
    virtual const wxString getTypeName() const;
    virtual wxString getDropSqlStatement() const;

    bool isForeignKey() const;
    bool isPrimaryKey() const;
    bool isString() const;
    virtual wxString getComputedSource() const;
    wxString getCollation() const;
    Table* getTable() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};

#endif
