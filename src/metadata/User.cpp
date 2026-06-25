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

#include "core/ProgressIndicator.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "engine/db/IStatement.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/server.h"
#include "metadata/User.h"

User::User(ServerPtr server)
    : MetadataItem(ntUnknown, server.get()), serverM(server)
{
}

User::User(ServerPtr server, const fr::UserData& src)
    : MetadataItem(ntUser, server.get()), serverM(server),
        useridM(src.userId), groupidM(src.groupId)
{
    usernameM = src.username;
    passwordM = src.password;
    firstnameM = src.firstName;
    middlenameM = src.middleName;
    lastnameM = src.lastName;
    pluginM = src.plugin;
}

User::User(DatabasePtr database, const wxString& name)
    : MetadataItem(ntUser, database.get(), name)
{
}

ServerPtr User::getServer() const
{
    ServerPtr s = serverM.lock();
    if (s)
        return s;
    DatabasePtr db = getDatabase();
    if (db)
        return db->getServer();
    return ServerPtr();
}

wxString User::getUsername() const
{
    if (usernameM.IsEmpty())
        return getName_();
    return usernameM;
}

wxString User::getPassword() const
{
    return passwordM;
}

wxString User::getFirstName() const
{
    return firstnameM;
}

wxString User::getMiddleName() const
{
    return middlenameM;
}

wxString User::getLastName() const
{
    return lastnameM;
}

uint32_t User::getUserId() const
{
    return useridM;
}

uint32_t User::getGroupId() const
{
    return groupidM;
}

void User::setUsername(const wxString& value)
{
    if (usernameM != value)
    {
        usernameM = value;
        notifyObservers();
    }
}

void User::setPassword(const wxString& value)
{
    if (passwordM != value)
    {
        passwordM = value;
        notifyObservers();
    }
}

void User::setFirstName(const wxString& value)
{
    if (firstnameM != value)
    {
        firstnameM = value;
        notifyObservers();
    }
}

void User::setMiddleName(const wxString& value)
{
    if (middlenameM != value)
    {
        middlenameM = value;
        notifyObservers();
    }
}

void User::setLastName(const wxString& value)
{
    if (lastnameM != value)
    {
        lastnameM = value;
        notifyObservers();
    }
}

void User::setUserId(uint32_t value)
{
    if (useridM != value)
    {
        useridM = value;
        notifyObservers();
    }
}

void User::setGroupId(uint32_t value)
{
    if (groupidM != value)
    {
        groupidM = value;
        notifyObservers();
    }
}

wxString User::getPlugin() const
{
    return pluginM;
}

void User::setPlugin(const wxString& value)
{
    if (pluginM != value)
    {
        pluginM = value;
        notifyObservers();
    }
}

void User::assignTo(fr::UserData& dest) const
{
    dest.username = wx2std(usernameM);
    dest.password = wx2std(passwordM);
    dest.firstName = wx2std(firstnameM);
    dest.lastName = wx2std(lastnameM);
    dest.middleName = wx2std(middlenameM);
    dest.userId = useridM;
    dest.groupId = groupidM;
    dest.plugin = wx2std(pluginM);
}

void User::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitUser(*this);
}

const wxString User::getTypeName() const
{
    return "USER";
}

bool User::isSystem() const
{
    return usernameM == "SYSDBA";
}

void Users::loadChildren()
{
    load(0);
}

Users::Users(DatabasePtr database)
    : MetadataCollection<User>(ntUsers, database, _("Users"))
{
}

void Users::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitUsers(*this);
}

void Users::load(ProgressIndicator* progressIndicator)
{
    DatabasePtr db = getDatabase();
    if (db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0))
    {
        MetadataLoader* loader = db->getMetadataLoader();
        MetadataLoaderTransaction tr(loader);
        wxMBConv* converter = db->getCharsetConverter();

        std::string stmt = "select sec$user_name, sec$first_name, sec$middle_name, "
            "sec$last_name, sec$plugin from sec$users order by 1";
        fr::IStatementPtr& st1 = loader->getStatement(stmt);
        st1->execute();

        std::vector<UserPtr> users;
        while (st1->fetch())
        {
            checkProgressIndicatorCanceled(progressIndicator);
            wxString name = std2wxIdentifier(st1->getString(0), converter);
            UserPtr u(new User(db, name));
            if (!st1->isNull(1))
                u->setFirstName(std2wxIdentifier(st1->getString(1), converter));
            if (!st1->isNull(2))
                u->setMiddleName(std2wxIdentifier(st1->getString(2), converter));
            if (!st1->isNull(3))
                u->setLastName(std2wxIdentifier(st1->getString(3), converter));
            if (!st1->isNull(4))
                u->setPlugin(std2wxIdentifier(st1->getString(4), converter));
            users.push_back(u);
        }
        setItems(users);
    }
    else
    {
        wxString stmt = "select sec$user_name from sec$users a order by 1 ";
        setItems(db->loadIdentifiers(stmt, progressIndicator));
    }
}

const wxString Users::getTypeName() const
{
    return "USERS_COLLECTION";
}