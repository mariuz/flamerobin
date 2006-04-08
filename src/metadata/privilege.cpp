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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "relation.h"
#include "procedure.h"
#include "role.h"

#include "privilege.h"
//-----------------------------------------------------------------------------
PrivilegeItem::PrivilegeItem(const wxString& grantorName,
    bool withGrantOption, const wxString& fieldName)
    :grantor(grantorName), grantOption(withGrantOption)
{
    if (!fieldName.IsEmpty())
        columns.push_back(fieldName);
}
//-----------------------------------------------------------------------------
Privilege::Privilege(MetadataItem *parent, const wxString& grantee,
    int granteeType)
    :parentM(parent), granteeM(grantee), granteeTypeM(granteeType)
{
}
//-----------------------------------------------------------------------------
void Privilege::addPrivilege(char privilege, const wxString& grantor,
    bool withGrantOption, const wxString& field)
{
    wxString p;
    switch (privilege)
    {
        case 'S':   p = wxT("SELECT");      break;
        case 'I':   p = wxT("INSERT");      break;
        case 'U':   p = wxT("UPDATE");      break;
        case 'D':   p = wxT("DELETE");      break;
        case 'R':   p = wxT("REFERENCES");  break;
        case 'X':   p = wxT("EXECUTE");     break;
        case 'M':   p = wxT("MEMBER OF");   break;
        default:
            return;
    };

    // iterate all of this type
    PMap::iterator it;
    for (it = privilegesM.lower_bound(p);
        it != privilegesM.upper_bound(p); ++it)
    {
        if ((*it).second.grantor == grantor &&
            (*it).second.grantOption == withGrantOption)    // got it
        {
            std::vector<wxString> *cols = 0;
            if (p == wxT("UPDATE") || p == wxT("REFERENCES"))
                cols = &((*it).second.columns);
            if (!field.IsEmpty() && cols && cols->end() ==
                std::find(cols->begin(), cols->end(), field))
            {
                cols->push_back(field);
            }
            return;
        }
    }

    // not found, so add it
    PrivilegeItem pi(grantor, withGrantOption, field);
    privilegesM.insert(std::pair<wxString,PrivilegeItem>(p,pi));
}
//-----------------------------------------------------------------------------
wxString granteeTypeToString(int type)
{
    if (type == 1)
        return wxT("VIEW");
    if (type == 2)
        return wxT("TRIGGER");
    if (type == 5)
        return wxT("PROCEDURE");
    if (type == 13)
        return wxT("ROLE");
    return wxEmptyString;
}
//-----------------------------------------------------------------------------
wxString Privilege::getSql(bool withGrantOption) const
{
    wxString ret;
    for (PMap::const_iterator c = privilegesM.begin();
        c != privilegesM.end(); ++c)
    {
        if ((*c).second.grantOption != withGrantOption)
            continue;
        if (!ret.IsEmpty())
            ret += wxT(", ");
        ret += (*c).first;
        const std::vector<wxString>& cols = (*c).second.columns;
        if (cols.size())
        {
            ret += wxT("(");
            for (std::vector<wxString>::const_iterator ci = cols.begin();
                ci != cols.end(); ++ci)
            {
                if (ci != cols.begin())
                    ret += wxT(",");
                Identifier id(*ci);
                ret += id.getQuoted();
            }
            ret += wxT(")");
        }
    }

    if (ret.IsEmpty())          // no privileges found
        return wxEmptyString;

    ret = wxT("GRANT ") + ret + wxT("\n ON ");
    if (dynamic_cast<Procedure *>(parentM))
        ret += wxT("PROCEDURE ");
    ret += parentM->getQuotedName()
        + wxT(" TO ") + granteeTypeToString(granteeTypeM) + wxT(" ")
        + granteeM;

    if (withGrantOption)
        ret += wxT(" WITH GRANT OPTION");
    ret += wxT(";\n");
    return ret;
}
//-----------------------------------------------------------------------------
wxString Privilege::getSql() const
{
    Role *r = dynamic_cast<Role *>(parentM);
    if (!r)
        return getSql(true) + getSql(false);

    wxString ret = wxT("GRANT ") + r->getQuotedName() + wxT(" TO ") + granteeM;
    for (PMap::const_iterator c = privilegesM.begin();
        c != privilegesM.end(); ++c)
    {
        if ((*c).second.grantOption)
        {
            ret += wxT(" WITH GRANT OPTION");
            break;
        }
    }
    ret += wxT(";\n");
    return ret;
}
//-----------------------------------------------------------------------------
wxString Privilege::getGrantee() const
{
    wxString gt = granteeTypeToString(granteeTypeM);
    if (!gt.IsEmpty())
        gt += wxT(" ");
    return gt + granteeM;
}
//-----------------------------------------------------------------------------
void Privilege::getPrivileges(const wxString& type,
    std::vector<PrivilegeItem>& list) const
{
    PMap::const_iterator it;
    for (it = privilegesM.lower_bound(type);
        it != privilegesM.upper_bound(type); ++it)
    {
        list.push_back((*it).second);
    }
}
//-----------------------------------------------------------------------------
