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
