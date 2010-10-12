/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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

#include "core/StringUtils.h"
#include "metadata/server.h"
#include "metadata/User.h"
//-----------------------------------------------------------------------------
User::User(ServerPtr server)
    :MetadataItem()
{
    serverM = server;
    setParent(server.get());
}
//-----------------------------------------------------------------------------
User::User(ServerPtr server, const IBPP::User& src)
    : MetadataItem(), useridM(src.userid), groupidM(src.groupid)
{
    serverM = server;
    setParent(server.get());
    usernameM = std2wx(src.username);
    passwordM = std2wx(src.password);
    firstnameM = std2wx(src.firstname);
    middlenameM = std2wx(src.middlename);
    lastnameM = std2wx(src.lastname);
}
//-----------------------------------------------------------------------------
ServerPtr User::getServer() const
{
    return serverM.lock();
}
//-----------------------------------------------------------------------------
wxString User::getUsername() const
{
    return usernameM;
}
//-----------------------------------------------------------------------------
wxString User::getPassword() const
{
    return passwordM;
}
//-----------------------------------------------------------------------------
wxString User::getFirstName() const
{
    return firstnameM;
}
//-----------------------------------------------------------------------------
wxString User::getMiddleName() const
{
    return middlenameM;
}
//-----------------------------------------------------------------------------
wxString User::getLastName() const
{
    return lastnameM;
}
//-----------------------------------------------------------------------------
uint32_t User::getUserId() const
{
    return useridM;
}
//-----------------------------------------------------------------------------
uint32_t User::getGroupId() const
{
    return groupidM;
}
//-----------------------------------------------------------------------------
void User::setUsername(const wxString& value)
{
    if (usernameM != value)
    {
        usernameM = value;
        notifyObservers();
    }
}
//-----------------------------------------------------------------------------
void User::setPassword(const wxString& value)
{
    if (passwordM != value)
    {
        passwordM = value;
        notifyObservers();
    }
}
//-----------------------------------------------------------------------------
void User::setFirstName(const wxString& value)
{
    if (firstnameM != value)
    {
        firstnameM = value;
        notifyObservers();
    }
}
//-----------------------------------------------------------------------------
void User::setMiddleName(const wxString& value)
{
    if (middlenameM != value)
    {
        middlenameM = value;
        notifyObservers();
    }
}
//-----------------------------------------------------------------------------
void User::setLastName(const wxString& value)
{
    if (lastnameM != value)
    {
        lastnameM = value;
        notifyObservers();
    }
}
//-----------------------------------------------------------------------------
void User::setUserId(uint32_t value)
{
    if (useridM != value)
    {
        useridM = value;
        notifyObservers();
    }
}
//-----------------------------------------------------------------------------
void User::setGroupId(uint32_t value)
{
    if (groupidM != value)
    {
        groupidM = value;
        notifyObservers();
    }
}
//-----------------------------------------------------------------------------
void User::assignTo(IBPP::User& dest) const
{
    dest.username = wx2std(usernameM);
    dest.password = wx2std(passwordM);
    dest.firstname = wx2std(firstnameM);
    dest.lastname = wx2std(lastnameM);
    dest.middlename = wx2std(middlenameM);
    dest.userid = useridM;
    dest.groupid = groupidM;
}
//-----------------------------------------------------------------------------
bool User::isSystem() const
{
    return usernameM == wxT("SYSDBA");
}
//-----------------------------------------------------------------------------
