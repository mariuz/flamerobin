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
  are Copyright (C) 2006 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/

#ifndef FR_PRIVILEGE_H
#define FR_PRIVILEGE_H

#include <map>
#include <vector>

class MetadataItem;
//-----------------------------------------------------------------------------
class PrivilegeItem
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
// MetadataItemPropertiesFrame::processHtmlCode
// Perhaps it could be changed to have a common class for that
class Privilege: public MetadataItem
{
private:
    MetadataItem* parentObjectM;
    int granteeTypeM;
    wxString granteeM;

    // type (SEL, INS, ...), privilege
    typedef std::multimap<wxString, PrivilegeItem> PMap;
    PMap privilegesM;

    wxString getSql(bool withGrantOption) const;

public:
    Privilege(MetadataItem *parent, const wxString& grantee, int granteeType);
    void addPrivilege(char privilege, const wxString& grantor,
        bool withGrantOption, const wxString& field = wxEmptyString);

    wxString getSql() const;
    wxString getGrantee() const;
    void getPrivileges(const wxString& type,
        std::vector<PrivilegeItem>& list) const;
};
//-----------------------------------------------------------------------------
#endif
