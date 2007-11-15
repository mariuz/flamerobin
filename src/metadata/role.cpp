/*
  Copyright (c) 2004-2007 The FlameRobin Development Team

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

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <ibpp.h>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "core/Visitor.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/role.h"
//-----------------------------------------------------------------------------
Role::Role()
    : MetadataItem()
{
    typeM = ntRole;
}
//-----------------------------------------------------------------------------
std::vector<Privilege>* Role::getPrivileges()
{
    // load privileges from database and return the pointer to collection
    Database *d = getDatabase();
    if (!d)
        throw FRError(_("database not set"));
    privilegesM.clear();
    IBPP::Database& db = d->getIBPPDatabase();
    IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
    tr1->Start();
    IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
    st1->Prepare(
        "select RDB$USER, RDB$USER_TYPE, RDB$GRANTOR, RDB$PRIVILEGE, "
        "RDB$GRANT_OPTION "
        "from RDB$USER_PRIVILEGES "
        "where RDB$RELATION_NAME = ? and rdb$object_type = 13 "
        "order by rdb$user, rdb$user_type, rdb$privilege"
    );
    st1->Set(1, wx2std(getName_()));
    st1->Execute();
    std::string lastuser;
    int lasttype = -1;
    Privilege *pr = 0;
    while (st1->Fetch())
    {
        std::string user, grantor, privilege;
        int usertype, grantoption = 0;
        st1->Get(1, user);
        st1->Get(2, usertype);
        st1->Get(3, grantor);
        st1->Get(4, privilege);
        if (!st1->IsNull(5))
            st1->Get(5, grantoption);
        if (!pr || user != lastuser || usertype != lasttype)
        {
            Privilege p(this, std2wx(user).Strip(), usertype);
            privilegesM.push_back(p);
            pr = &privilegesM.back();
            lastuser = user;
            lasttype = usertype;
        }
        pr->addPrivilege(privilege[0], std2wx(grantor).Strip(),
            grantoption != 0);  // ADMIN OPTION = 2
    }
    tr1->Commit();
    return &privilegesM;
}
//-----------------------------------------------------------------------------
wxString Role::getOwner()
{
    Database* d = getDatabase();
    if (!d)
        throw FRError(_("database not set"));
    IBPP::Database& db = d->getIBPPDatabase();
    IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
    tr1->Start();
    IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
    st1->Prepare(
        "select rdb$owner_name from rdb$roles where rdb$role_name = ?");
    st1->Set(1, wx2std(getName_()));
    st1->Execute();
    st1->Fetch();
    std::string name;
    st1->Get(1, name);
    tr1->Commit();
    return std2wx(name).Trim();
}
//-----------------------------------------------------------------------------
void Role::loadDescription()
{
    MetadataItem::loadDescription(
        wxT("select RDB$DESCRIPTION from RDB$ROLES ")
        wxT("where RDB$ROLE_NAME = ?"));
}
//-----------------------------------------------------------------------------
void Role::saveDescription(wxString description)
{
    MetadataItem::saveDescription(
        wxT("update RDB$ROLES set RDB$DESCRIPTION = ? ")
        wxT("where RDB$ROLE_NAME = ?"),
        description);
}
//-----------------------------------------------------------------------------
wxString Role::getCreateSqlTemplate() const
{
    return  wxT("CREATE ROLE role_name;\n");
}
//-----------------------------------------------------------------------------
const wxString Role::getTypeName() const
{
    return wxT("ROLE");
}
//-----------------------------------------------------------------------------
void Role::acceptVisitor(MetadataItemVisitor *visitor)
{
    visitor->visitRole(*this);
}
//-----------------------------------------------------------------------------
