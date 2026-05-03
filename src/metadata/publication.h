/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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


#ifndef FR_PUBLICATION_H
#define FR_PUBLICATION_H

#include "metadata/collection.h"
#include "metadata/metadataitem.h"

class Publication: public MetadataItem
{
private:
    wxArrayString tablesM;
    bool allTablesM;
    void ensureTablesLoaded();
protected:
    virtual void loadDescription();
    virtual void saveDescription(const wxString& description);
public:
    Publication(DatabasePtr database, const wxString& name);
    virtual const wxString getTypeName() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);

    void setTables(const wxArrayString& tables);
    void setAllTables(bool all);
    wxArrayString getTables();
    bool getAllTables() const;
};

class Publications: public MetadataCollection<Publication>
{
protected:
    virtual void loadChildren();
public:
    Publications(DatabasePtr database);
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};

class Replication: public MetadataItem
{
private:
    PublicationsPtr publicationsM;
protected:
    virtual void loadChildren();
public:
    Replication(DatabasePtr database);
    PublicationsPtr getPublications();
    virtual const wxString getTypeName() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    virtual bool getChildren(std::vector<MetadataItem *>& temp);
};

#endif
