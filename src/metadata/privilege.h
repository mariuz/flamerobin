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

#include <vector>

class MetadataItem;
//-----------------------------------------------------------------------------
class Privilege
{
private:
    MetadataItem* parentM;
    int granteeTypeM;
    wxString granteeM;
    wxString grantorM;
    bool withGrantOptionM;
    std::vector<wxString> privilegesM;      // INS, UPD, DEL, REF, EXECUTE, ALL
    std::vector<wxString> updateColumnsM;
    std::vector<wxString> refColumnsM;

public:
    Privilege(MetadataItem *parent, const wxString& grantee, int granteeType,
        const wxString& grantor, bool withGrantOption);
    void addPrivilege(char privilege);
    void addUpdateColumn(const wxString& column);
    void addReferencesColumn(const wxString& column);
    wxString getSql() const;
};
//-----------------------------------------------------------------------------
#endif
