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

  Contributor(s):
*/

#ifndef FR_CONSTRAINTS_H
#define FR_CONSTRAINTS_H
//-----------------------------------------------------------------------------
#include <string>
#include <vector>

#include "metadataitem.h"
//-----------------------------------------------------------------------------
// These could all be simple "struct"s but we want to add some functionality later
//
class Constraint: public MetadataItem
{
    // nothing needed yet, but it may be once
    virtual void accept(Visitor *v);
};
//-----------------------------------------------------------------------------
//! primary keys and uniques
class ColumnConstraint: public Constraint
{
public:
    typedef std::vector<std::string>::const_iterator const_iterator;
    //std::string indexName; needed?
    std::vector<std::string> columnsM;

    std::string getColumnList() const;
    const_iterator begin() { return columnsM.begin(); };
    const_iterator end() { return columnsM.end(); };
};
//-----------------------------------------------------------------------------
//! checks
class CheckConstraint: public Constraint
{
public:
    std::string sourceM;
};
//-----------------------------------------------------------------------------
//! foreign keys
class ForeignKey: public ColumnConstraint
{
public:
    std::string referencedTableM;                   // referenced table
    std::vector<std::string> referencedColumnsM;    // referenced columns
    std::string updateActionM;
    std::string deleteActionM;
    std::string getReferencedColumnList() const;
};
//-----------------------------------------------------------------------------
#endif
