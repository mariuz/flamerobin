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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "metadata/database.h"
#include "metadata/privilege.h"
#include "metadata/procedure.h"
#include "metadata/relation.h"
#include "metadata/role.h"

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
    : parentObjectM(parent), granteeM(grantee), granteeTypeM(granteeType),
     ProcessableObject()
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
    for (it = privilegeItemsM.lower_bound(p);
        it != privilegeItemsM.upper_bound(p); ++it)
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
    privilegeItemsM.insert(std::pair<wxString,PrivilegeItem>(p,pi));
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
    for (PMap::const_iterator c = privilegeItemsM.begin();
        c != privilegeItemsM.end(); ++c)
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
    if (dynamic_cast<Procedure *>(parentObjectM))
        ret += wxT("PROCEDURE ");
    Identifier id(granteeM);
    ret += parentObjectM->getQuotedName() + wxT(" TO ")
        + granteeTypeToString(granteeTypeM) + wxT(" ") + id.getQuoted();

    if (withGrantOption)
        ret += wxT(" WITH GRANT OPTION");
    ret += wxT(";\n");
    return ret;
}
//-----------------------------------------------------------------------------
wxString Privilege::getSql() const
{
    Role *r = dynamic_cast<Role *>(parentObjectM);
    if (!r)
        return getSql(true) + getSql(false);

    wxString ret = wxT("GRANT ") + r->getQuotedName() + wxT(" TO ") + granteeM;
    for (PMap::const_iterator c = privilegeItemsM.begin();
        c != privilegeItemsM.end(); ++c)
    {
        if ((*c).second.grantOption)
        {
            ret += wxT(" WITH ADMIN OPTION");
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
void Privilege::getPrivilegeItems(const wxString& type,
    std::vector<PrivilegeItem>& list) const
{
    PMap::const_iterator it;
    for (it = privilegeItemsM.lower_bound(type);
        it != privilegeItemsM.upper_bound(type); ++it)
    {
        list.push_back((*it).second);
    }
}
//-----------------------------------------------------------------------------
