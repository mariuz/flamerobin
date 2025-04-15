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


#ifndef FR_INDEX_H
#define FR_INDEX_H

#include <vector>

#include "metadata/collection.h"
#include "metadata/metadataitem.h"

class Index: public MetadataItem
{
public:
    // needs to be declared here as type is used in private section
    enum IndexType { itAscending, itDescending };

private:
    bool isSystemM;
    bool uniqueFlagM;
    bool activeM;
    IndexType indexTypeM;
    double statisticsM;
    std::vector<wxString> segmentsM;
    wxString expressionM;
protected:
    virtual void loadProperties();
public:
    Index(DatabasePtr database, const wxString& name);
    Index(bool unique, bool active, bool ascending, double statistics,
        bool system, wxString expression);

    virtual bool isSystem() const;
    void setActive(bool active);
    bool getActive();
    bool isActive();
    bool isUnique() const;
    double getStatistics();
    wxString getExpression() const;
    IndexType getIndexType();
    virtual const wxString getTypeName() const;
    bool hasColumn(wxString segment) const;
    // Returns a list of index fields, or the expression source if
    // the index is an expression-based index.
    wxString getFieldsAsString();
    std::vector<wxString> *getSegments();

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};



class Indices : public MetadataCollection<Index>
{
protected:
    virtual void loadChildren();
public:
    Indices(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;

};


class SysIndices : public MetadataCollection<Index>
{
protected:
    virtual void loadChildren();
public:
    SysIndices(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;
};

class UsrIndices : public MetadataCollection<Index>
{
protected:
    virtual void loadChildren();
public:
    UsrIndices(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;
};


#endif
