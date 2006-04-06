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

  The Initial Developer of the Original Code is Nando Dessena.

  Portions created by the original developer
  are Copyright (C) 2004 Nando Dessena.

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

#include "dberror.h"
#include "core/Visitor.h"
#include "MetadataItemVisitor.h"
#include "role.h"
//-----------------------------------------------------------------------------
const std::vector<Privilege>* Role::getPrivileges()
{
    // load privileges from database and return the pointer to collection
    Database *d = getDatabase();
    if (!d)
    {
        lastError().setMessage(wxT("database not set"));
        return 0;
    }
    privilegesM.clear();
    IBPP::Database& db = d->getIBPPDatabase();
    try
    {
        IBPP::Transaction tr1 = IBPP::TransactionFactory(db, IBPP::amRead);
        tr1->Start();
        IBPP::Statement st1 = IBPP::StatementFactory(db, tr1);
        st1->Prepare(
            "select RDB$USER, RDB$USER_TYPE, RDB$GRANTOR, RDB$PRIVILEGE, "
            "RDB$GRANT_OPTION, RDB$FIELD_NAME "
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
            std::string user, grantor, privilege, field;
            int usertype, grantoption = 0;
            st1->Get(1, user);
            st1->Get(2, usertype);
            st1->Get(3, grantor);
            st1->Get(4, privilege);
            if (!st1->IsNull(5))
                st1->Get(5, grantoption);
            st1->Get(6, field);
            if (!pr || user != lastuser || usertype != lasttype)
            {
                Privilege p(this, std2wx(user).Strip(), usertype,
                    std2wx(grantor).Strip(), grantoption == 1);
                privilegesM.push_back(p);
                pr = &privilegesM.back();
                lastuser = user;
                lasttype = usertype;
            }
            pr->addPrivilege(privilege[0]);
        }
        tr1->Commit();
        return &privilegesM;
    }
    catch (IBPP::Exception &e)
    {
        lastError().setMessage(std2wx(e.ErrorMessage()));
    }
    catch (...)
    {
        lastError().setMessage(_("System error."));
    }
    return 0;
}
//-----------------------------------------------------------------------------
wxString Role::getRoleOwner()
{
    Database* d = getDatabase();
    if (!d)
        return _("ERROR: Database not set");
    IBPP::Database& db = d->getIBPPDatabase();

    try
    {
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
        return std2wx(name).Strip();
    }
    catch (IBPP::Exception &e)
    {
        return std2wx(e.ErrorMessage());
    }
    catch (...)
    {
        return _("System error.");
    }

    return wxEmptyString;
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
    visitor->visit(*this);
}
//-----------------------------------------------------------------------------
