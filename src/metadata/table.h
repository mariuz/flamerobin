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


#ifndef FR_TABLE_H
#define FR_TABLE_H

#include "metadata/collection.h"
#include "metadata/constraints.h"
#include "metadata/Index.h"
#include "metadata/relation.h"

class Table: public Relation
{
private:
    PrimaryKeyConstraint primaryKeyM;           // table can have only one pk
    bool primaryKeyLoadedM;
    void loadPrimaryKey();

    std::vector<ForeignKey> foreignKeysM;
    bool foreignKeysLoadedM;
    void loadForeignKeys();

    std::vector<CheckConstraint> checkConstraintsM;
    bool checkConstraintsLoadedM;
    void loadCheckConstraints();

    std::vector<UniqueConstraint> uniqueConstraintsM;
    bool uniqueConstraintsLoadedM;
    void loadUniqueConstraints();

    std::vector<Index> indicesM;
    bool indicesLoadedM;
    void loadIndices();

    wxString externalPathM;

protected:
    virtual void setExternalFilePath(const wxString& value);

    virtual void loadChildren();

public:
    Table(DatabasePtr database, const wxString& name);

    static bool tablesRelate(const std::vector<wxString>& tables,
        Table *table, std::vector<ForeignKey>& list);

    void invalidateIndices(const wxString& forIndex = wxEmptyString);

    wxString getExternalPath();

    PrimaryKeyConstraint *getPrimaryKey();
    std::vector<ForeignKey> *getForeignKeys();
    std::vector<CheckConstraint> *getCheckConstraints();
    std::vector<UniqueConstraint> *getUniqueConstraints();
    std::vector<Index> *getIndices();

    virtual const wxString getTypeName() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);

};

class SysTable : public Table
{
public:
    SysTable(DatabasePtr database, const wxString& name);
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};

class GTTable : public Table
{
public:
    GTTable(DatabasePtr database, const wxString& name);
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};

class SysTables: public MetadataCollection<SysTable>
{
protected:
    virtual void loadChildren();
public:
    SysTables(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    virtual bool isSystem() const;
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;
};

class Tables: public MetadataCollection<Table>
{
protected:
    virtual void loadChildren();
public:
    Tables(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;
};

class GTTables: public MetadataCollection<GTTable>
{
protected:
    virtual void loadChildren();
public:
    GTTables(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;
};

#endif
