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

#pragma once

#ifndef FR_CHARACTERSET_H
#define FR_CHARACTERSET_H

#include <vector>
#include "metadata/collection.h"
#include "metadata/Collation.h"
#include "metadata/privilege.h"

class CharacterSets;

class CharacterSet: public MetadataItem
{
private:
    int bytesPerCharM;
    wxString collationDefaultM;
    CollationPtrs collationsM;

    friend class CharacterSets;
protected:

    CollationPtr findCollation(const wxString& name) const;

    static std::string getLoadStatement(bool list);
    void loadProperties(IBPP::Statement& statement, wxMBConv* converter);
    virtual void loadProperties();

    virtual void loadChildren();
    virtual void lockChildren();
    virtual void unlockChildren();


public:
    CharacterSet();
    CharacterSet(DatabasePtr database, const wxString& name, int id = -1, int bytesPerChar = -1);
    ~CharacterSet();

    CollationPtrs::iterator begin();
    CollationPtrs::iterator end();
    CollationPtrs::const_iterator begin() const;
    CollationPtrs::const_iterator end() const;

    int getBytesPerChar() const;
    void setBytesPerChar(int bytes);
    wxString getCollationDefault() const;
    void setCollationDefault(wxString collation);
    bool getChildren(std::vector<MetadataItem*>& temp);
    wxArrayString getCollations();

    virtual const wxString getTypeName() const;
    virtual void acceptVisitor(MetadataItemVisitor* visitor);

};


class CharacterSets : public MetadataCollection<CharacterSet>
{
protected:
    virtual void loadChildren();
public:
    CharacterSets(DatabasePtr database);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);
    void load(ProgressIndicator* progressIndicator);
    virtual const wxString getTypeName() const;


};

#endif // FR_CHARACTERSET_H
