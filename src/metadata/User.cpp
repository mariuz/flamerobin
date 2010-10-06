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
User::User(Server *parent)
    :MetadataItem()
{
    setParent((MetadataItem *)parent);
}
//-----------------------------------------------------------------------------
User::User(const IBPP::User& src, Server *parent)
    :MetadataItem(), useridM(src.userid), groupidM(src.groupid)
{
    setParent((MetadataItem *)parent);
    usernameM = std2wx(src.username);
    passwordM = std2wx(src.password);
    firstnameM = std2wx(src.firstname);
    middlenameM = std2wx(src.middlename);
    lastnameM = std2wx(src.lastname);
}
//-----------------------------------------------------------------------------
void User::setIBPP(IBPP::User& dest) const
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
bool User::operator<(const User& rhs) const
{
    return usernameM < rhs.usernameM;
}
//-----------------------------------------------------------------------------
bool User::isSystem() const
{
    return usernameM == wxT("SYSDBA");
}
//-----------------------------------------------------------------------------
