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
#include "metadata/MetadataItemVisitor.h"
#include "metadata/server.h"
#include "metadata/User.h"

void User::loadProperties()
{
    serverM = getDatabase()->getServer();
}

User::User(ServerPtr server)
    : MetadataItem(ntUnknown, server.get()), serverM(server)
{
}

User::User(ServerPtr server, const IBPP::User& src)
    : MetadataItem(ntUnknown, server.get()), serverM(server),
        useridM(src.userid), groupidM(src.groupid)
{
    usernameM = src.username;
    passwordM = src.password;
    firstnameM = src.firstname;
    middlenameM = src.middlename;
    lastnameM = src.lastname;
}

User::User(DatabasePtr database, const wxString& name)
    : MetadataItem(ntUser, database.get(), name)
{
}

ServerPtr User::getServer() const
{
    return ServerPtr(serverM);
}

wxString User::getUsername() const
{
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

IBPP::User User::getUserIBPP() 
{
    ensurePropertiesLoaded();

    IBPP::User usr;
    
    usr.username = wx2std(usernameM);
    usr.password = wx2std(passwordM);
    usr.firstname = wx2std(firstnameM);
    usr.lastname = wx2std(lastnameM);
    usr.middlename = wx2std(middlenameM);
    usr.userid = useridM;
    usr.groupid = groupidM;

    return usr;
}

void User::setServer(ServerPtr srv)
{
    serverM = srv;
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

void User::setUserIBPP(const IBPP::User& usr) 
{
    setUsername(usr.username);
    setUsername(usr.username);
    setPassword(usr.password);
    setFirstName(usr.firstname);
    setMiddleName(usr.middlename);
    setLastName(usr.lastname);
    setUserId(usr.userid);
    setGroupId(usr.groupid);

    notifyObservers();
}

void User::assignTo(IBPP::User& dest) const
{
    //dest = getUserIBPP();
    dest.username = wx2std(usernameM);
    dest.password = wx2std(passwordM);
    dest.firstname = wx2std(firstnameM);
    dest.lastname = wx2std(lastnameM);
    dest.middlename = wx2std(middlenameM);
    dest.userid = useridM;
    dest.groupid = groupidM;

}

void User::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitUser(*this);
}

const wxString User::getTypeName() const
{
    return "USER";
}

wxString User::getSource()
{
    ensurePropertiesLoaded();
    wxString sql = "FIRSTNAME '" + getFirstName() + "' \n" +
        "MIDDLENAME '" + getMiddleName() + "' \n"
        "LASTNAME '" + getLastName() + "' \n"
        "USING PLUGIN "+" \n"
        ;
    return sql;
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

const wxString Users::getTypeName() const
{
    return "USERS_COLLECTION";
}



