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
Privilege::Privilege(MetadataItem *parent, const wxString& grantee,
    int granteeType, const wxString& grantor, bool withGrantOption)
    :parentM(parent), granteeM(grantee), grantorM(grantor),
     granteeTypeM(granteeType), withGrantOptionM(withGrantOption)
{
}
//-----------------------------------------------------------------------------
void Privilege::addPrivilege(char privilege)
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
        //case '':   p = wxT("");      break;
        default:
            return;
    };

    if (privilegesM.end() ==
        std::find(privilegesM.begin(), privilegesM.end(), p))
        privilegesM.push_back(p);
}
//-----------------------------------------------------------------------------
void Privilege::addUpdateColumn(const wxString& column)
{
    updateColumnsM.push_back(column);
}
//-----------------------------------------------------------------------------
void Privilege::addReferencesColumn(const wxString& column)
{
    refColumnsM.push_back(column);
}
//-----------------------------------------------------------------------------
wxString granteeTypeToString(int type)
{
    if (type == 1)  // view
        return wxT("VIEW ");
    if (type == 2)  // trigger
        return wxT("TRIGGER ");
    if (type == 5)  // procedure
        return wxT("PROCEDURE ");
    return wxEmptyString;
}
//-----------------------------------------------------------------------------
wxString Privilege::getSql() const
{
    wxString ret;
    Relation *r = dynamic_cast<Relation *>(parentM);
    if (r)
    {
        ret = wxT("GRANT ");
        for (std::vector<wxString>::const_iterator c = privilegesM.begin();
            c != privilegesM.end(); ++c)
        {
            if (c != privilegesM.begin())
                ret += wxT(",");
            ret += (*c);
            if ((*c) == wxT("UPDATE") && updateColumnsM.size())
            {
                ret += wxT("(");
                for (std::vector<wxString>::const_iterator i =
                    updateColumnsM.begin(); i != updateColumnsM.end(); ++i)
                {
                    if (i != updateColumnsM.begin())
                        ret += wxT(",");
                    Identifier id(*i);
                    ret += id.getQuoted();
                }
                ret += wxT(")");
            }
            if ((*c) == wxT("REFERENCES") && refColumnsM.size())
            {
                ret += wxT("(");
                for (std::vector<wxString>::const_iterator i =
                    refColumnsM.begin(); i != refColumnsM.end(); ++i)
                {
                    if (i != updateColumnsM.begin())
                        ret += wxT(",");
                    Identifier id(*i);
                    ret += id.getQuoted();
                }
                ret += wxT(")");
            }
        }
        ret += wxT(" ON ") + r->getQuotedName() + wxT("\n    TO ")
            + granteeTypeToString(granteeTypeM) + granteeM;
        if (withGrantOptionM)
            ret += wxT(" WITH GRANT OPTION");
    }
    else
    {
        Procedure *p = dynamic_cast<Procedure *>(parentM);
        if (p)
        {
            if (privilegesM.end() != std::find(privilegesM.begin(),
                privilegesM.end(), wxT("EXECUTE")))
            {
                ret += wxT("GRANT EXECUTE ON PROCEDURE ") +
                    p->getQuotedName() + wxT(" TO ") +
                    granteeTypeToString(granteeTypeM) + granteeM;
            }
        }
        else
        {
            Role *r = dynamic_cast<Role *>(parentM);
            if (r)
            {
            }
        }
    }
    return ret;
}
//-----------------------------------------------------------------------------
