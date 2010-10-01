/*
  Copyright (c) 2004-2009 The FlameRobin Development Team

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

#ifndef FR_PRIVILEGE_H
#define FR_PRIVILEGE_H

#include <map>
#include <vector>

class MetadataItem;
class PrivilegeItem;
//-----------------------------------------------------------------------------
typedef std::vector<PrivilegeItem> PrivilegeItems;
//-----------------------------------------------------------------------------
// PrivilegeItem class only descends from MetadataItem to be able to be used in
// the HTML template processor.
// Perhaps it could be changed to have a common class for that
class PrivilegeItem: public MetadataItem
{
public:
    wxString grantor;
    bool grantOption;
    std::vector<wxString> columns;
    PrivilegeItem(const wxString& grantorName, bool withGrantOption,
        const wxString& fieldName);
};
//-----------------------------------------------------------------------------
// Privilege class only descends from MetadataItem to be able to be used in
// the HTML template processor.
// Perhaps it could be changed to have a common class for that
class Privilege: public MetadataItem
{
private:
    MetadataItem* parentObjectM;
    int granteeTypeM;
    wxString granteeM;

    // type (SEL, INS, ...), privilege
    typedef std::multimap<wxString, PrivilegeItem> PMap;
    PMap privilegeItemsM;

    wxString getSql(bool withGrantOption) const;

public:
    Privilege(MetadataItem *parent, const wxString& grantee, int granteeType);
    void addPrivilege(char privilege, const wxString& grantor,
        bool withGrantOption, const wxString& field = wxEmptyString);

    wxString getSql() const;
    wxString getGrantee() const;
    void getPrivilegeItems(const wxString& type, PrivilegeItems& list) const;
};
//-----------------------------------------------------------------------------
#endif
