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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <ibpp.h>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/role.h"

Role::Role(DatabasePtr database, const wxString& name)
    : MetadataItem(hasSystemPrefix(name) ? ntSysRole : ntRole, database.get(),
        name)
{
}

std::vector<Privilege>* Role::getPrivileges(bool splitPerGrantor)
{
    // load privileges from database and return the pointer to collection
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the role
    // when objects go out of scope and are destroyed, role will be
    // unlocked before the transaction is committed - any update() calls on
    // observers can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);

    privilegesM.clear();

    IBPP::Statement st1 = loader->getStatement(
        "select RDB$USER, RDB$USER_TYPE, RDB$GRANTOR, RDB$PRIVILEGE, "
        "RDB$GRANT_OPTION "
        "from RDB$USER_PRIVILEGES "
        "where RDB$RELATION_NAME = ? and rdb$object_type = 13 "
        "order by rdb$user, rdb$user_type, rdb$grantor, rdb$grant_option, rdb$privilege"
    );
    st1->Set(1, wx2std(getName_(), db->getCharsetConverter()));
    st1->Execute();
    std::string lastuser;
    std::string lastGrantor;
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
        if (!pr || user != lastuser || usertype != lasttype || (splitPerGrantor && grantor != lastGrantor))
        {
            Privilege p(this, wxString(user).Strip(), usertype);
            privilegesM.push_back(p);
            pr = &privilegesM.back();
            lastuser = user;
            lastGrantor = grantor;
            lasttype = usertype;
        }
        pr->addPrivilege(privilege[0], wxString(grantor).Strip(),
            grantoption != 0);  // ADMIN OPTION = 2
    }
    return &privilegesM;
}

wxString Role::getOwner()
{
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement st1 = loader->getStatement(
        "select rdb$owner_name from rdb$roles where rdb$role_name = ?");
    st1->Set(1, wx2std(getName_(), db->getCharsetConverter()));
    st1->Execute();
    st1->Fetch();
    std::string name;
    st1->Get(1, name);
    return wxString(name).Trim();
}

const wxString Role::getTypeName() const
{
    return "ROLE";
}

void Role::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitRole(*this);
}

// System roles collection
SysRoles::SysRoles(DatabasePtr database)
    : MetadataCollection<Role>(ntSysRoles, database, _("System Roles"))
{
}

void SysRoles::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitSysRoles(*this);
}

bool SysRoles::isSystem() const
{
    return true;
}

void SysRoles::load(ProgressIndicator* progressIndicator)
{
    DatabasePtr db = getDatabase();
    if (db && db->getInfo().getODSVersionIsHigherOrEqualTo(11, 1))
    {
        wxString stmt = "select rdb$role_name from rdb$roles"
            " where (rdb$system_flag > 0) order by 1";
        setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
    }
}

void SysRoles::loadChildren()
{
    load(0);
}

const wxString SysRoles::getTypeName() const
{
    return "SYSROLE_COLLECTION";
}

// Roles collection
Roles::Roles(DatabasePtr database)
    : MetadataCollection<Role>(ntRoles, database, _("Roles"))
{
}

void Roles::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitRoles(*this);
}

void Roles::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = "select rdb$role_name from rdb$roles";
    DatabasePtr db = getDatabase();
    if (db && db->getInfo().getODSVersionIsHigherOrEqualTo(11, 1))
        stmt += " where (rdb$system_flag = 0 or rdb$system_flag is null)";
    stmt += " order by 1";
    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
}

void Roles::loadChildren()
{
    load(0);
}

const wxString Roles::getTypeName() const
{
    return "ROLE_COLLECTION";
}

