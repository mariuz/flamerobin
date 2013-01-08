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


  $Id$

*/

//-----------------------------------------------------------------------------
#ifndef FR_RELATION_H
#define FR_RELATION_H

#include <vector>

#include "metadata/constraints.h"
#include "metadata/MetadataClasses.h"
#include "metadata/metadataitem.h"
#include "metadata/privilege.h"
#include "metadata/trigger.h"
//-----------------------------------------------------------------------------
class Relation: public MetadataItem
{
private:
    int relationTypeM;
    wxString ownerM;
protected:
    void getDependentChecks(std::vector<CheckConstraint>& checks);
    void getDependentViews(std::vector<Relation*>& views,
        const wxString& forColumn = wxT(""));

    ColumnPtrs columnsM;
    std::vector<Privilege> privilegesM;

    virtual void loadProperties();
    virtual void loadChildren();
    virtual void lockChildren();
    virtual void unlockChildren();

    // property setters, used for either tables or views
    // (called from loadProperties() method)
    // this loads more data than necesary, but causes less database roundtrips
    virtual void setExternalFilePath(const wxString& value);
    virtual void setSource(const wxString& value);

public:
    Relation(NodeType type, DatabasePtr database, const wxString& name);

    wxString getOwner();
    int getRelationType();

    ColumnPtrs::iterator begin();
    ColumnPtrs::iterator end();
    ColumnPtrs::const_iterator begin() const;
    ColumnPtrs::const_iterator end() const;

    size_t getColumnCount() const;
    ColumnPtr findColumn(const wxString& name) const;

    wxString getRebuildSql(const wxString& forColumn = wxT(""));
    std::vector<Privilege>* getPrivileges();
    bool getChildren(std::vector<MetadataItem *>& temp);
    void getTriggers(std::vector<Trigger*>& list,
        Trigger::FiringTime time);
};
//-----------------------------------------------------------------------------
#endif // FR_RELATION_H
