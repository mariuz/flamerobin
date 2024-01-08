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
#include "metadata/FB20/User20.h"


Users20::Users20(DatabasePtr database)
    :Users(database)
{
}

void Users20::load(ProgressIndicator* )
{
    DatabasePtr db = getDatabase();
    IBPP::Service svc;
    if (db->getServer()->getService(svc, NULL, true)) {   // true = SYSDBA

        std::vector<IBPP::User> usr;
        svc->GetUsers(usr);
        for (std::vector<IBPP::User>::iterator it = usr.begin();
            it != usr.end(); ++it)
        {
            insert(it->username);
        }
        notifyObservers();
        setChildrenLoaded(true);

    }

}



void User20::loadProperties()
{
    setPropertiesLoaded(false);

    DatabasePtr db = getDatabase();
    IBPP::Service svc;
    if (db->getServer()->getService(svc, NULL, true)) {
        IBPP::User usr;
        usr.username = getName_();
        svc->GetUser(usr);
        setUserIBPP(usr);
    }

    setPropertiesLoaded(true);
    notifyObservers();
}

User20::User20(ServerPtr server) 
    : User(server)
{
}

User20::User20(ServerPtr server, const IBPP::User& src)
    :User(server, src)
{
}

User20::User20(DatabasePtr database, const wxString& name)
    :User(database, name)
{
}
