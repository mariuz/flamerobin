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

//-----------------------------------------------------------------------------
#ifndef FR_METADATAITEMWITHCOLUMNS_H
#define FR_METADATAITEMWITHCOLUMNS_H

#include <vector>

#include "metadata/collection.h"
#include "metadata/column.h"
#include "metadata/metadataitem.h"
#include "metadata/privilege.h"
#include "metadata/trigger.h"

class View;
//-----------------------------------------------------------------------------
class Relation: public MetadataItem
{
protected:
    void getDependentChecks(std::vector<CheckConstraint>& checks);
    void getDependentViews(std::vector<Relation *>& views);

    MetadataCollection<Column> columnsM;
    std::vector<Privilege> privilegesM;

    virtual void loadDescription();
    virtual void saveDescription(wxString description);
public:
    Relation();
    Relation(const Relation& rhs);

    virtual void lockChildren();
    virtual void unlockChildren();

    bool checkAndLoadColumns();
    virtual bool loadColumns();
    Column *addColumn(Column &c);

    MetadataCollection<Column>::iterator begin();
    MetadataCollection<Column>::iterator end();
    MetadataCollection<Column>::const_iterator begin() const;
    MetadataCollection<Column>::const_iterator end() const;

    wxString getRebuildSql();
    std::vector<Privilege>* getPrivileges();
    bool getChildren(std::vector<MetadataItem *>& temp);
    bool getTriggers(std::vector<Trigger *>& list,
        Trigger::firingTimeType beforeOrAfter);
};
//-----------------------------------------------------------------------------
#endif
