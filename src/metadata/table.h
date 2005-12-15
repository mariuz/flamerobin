/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena
*/

//-----------------------------------------------------------------------------
#ifndef FR_TABLE_H
#define FR_TABLE_H

#include "metadata/collection.h"
#include "metadata/column.h"
#include "metadata/constraints.h"
#include "metadata/Index.h"
#include "metadata/relation.h"
#include "metadata/trigger.h"
//-----------------------------------------------------------------------------
class Table: public Relation
{
private:
    ColumnConstraint primaryKeyM;           // table can have only one pk
    bool primaryKeyLoadedM;
    bool loadPrimaryKey();

    std::vector<ForeignKey> foreignKeysM;
    bool foreignKeysLoadedM;
    bool loadForeignKeys();

    std::vector<CheckConstraint> checkConstraintsM;
    bool checkConstraintsLoadedM;
    bool loadCheckConstraints();

    std::vector<ColumnConstraint> uniqueConstraintsM;
    bool uniqueConstraintsLoadedM;
    bool loadUniqueConstraints();

    std::vector<Index> indicesM;
    bool indicesLoadedM;
    bool loadIndices();

public:
    Table();

    static bool tablesRelate(const std::vector<wxString>& tables, Table *table,
                             std::vector<ForeignKey>& list);

    virtual wxString getCreateSqlTemplate() const;

    virtual bool loadColumns();         // update the keys info too
    void invalidateIndices(const wxString& forIndex = wxEmptyString);

    ColumnConstraint *getPrimaryKey();
    std::vector<ForeignKey> *getForeignKeys();
    std::vector<CheckConstraint> *getCheckConstraints();
    std::vector<ColumnConstraint> *getUniqueConstraints();
    std::vector<Index> *getIndices();

    wxString getInsertStatement();
    //wxString getUpdateStatement();        // use primary key info
    //wxString getDeleteStatement();
    virtual const wxString getTypeName() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};
//-----------------------------------------------------------------------------
#endif
