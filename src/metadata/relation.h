/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

//-----------------------------------------------------------------------------
#ifndef FR_RELATION_H
#define FR_RELATION_H

#include <vector>

#include "metadata/collection.h"
#include "metadata/column.h"
#include "metadata/constraints.h"
#include "metadata/metadataitem.h"
#include "metadata/privilege.h"
#include "metadata/trigger.h"
//-----------------------------------------------------------------------------
class Relation: public MetadataItem
{
protected:
    int relationTypeM;
    wxString ownerM;
    void loadInfo();

    void getDependentChecks(std::vector<CheckConstraint>& checks);
    void getDependentViews(std::vector<Relation*>& views,
        const wxString& forColumn = wxT(""));

    MetadataCollection<Column> columnsM;
    std::vector<Privilege> privilegesM;

    virtual void loadChildren();
    virtual void lockChildren();
    virtual void unlockChildren();

    virtual bool addRdbKeyToSelect();
public:
    Relation();
    Relation(const Relation& rhs);

    wxString getOwner();
    int getRelationType();

    bool columnsLoaded();

    wxString getSelectStatement();

    MetadataCollection<Column>::iterator begin();
    MetadataCollection<Column>::iterator end();
    MetadataCollection<Column>::const_iterator begin() const;
    MetadataCollection<Column>::const_iterator end() const;

    size_t getColumnCount() const;

    wxString getRebuildSql(const wxString& forColumn = wxT(""));
    std::vector<Privilege>* getPrivileges();
    bool getChildren(std::vector<MetadataItem *>& temp);
    void getTriggers(std::vector<Trigger *>& list,
        Trigger::fireTimeType beforeOrAfter);
};
//-----------------------------------------------------------------------------
#endif // FR_RELATION_H

