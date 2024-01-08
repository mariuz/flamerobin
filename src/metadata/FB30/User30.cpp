/*
  Copyright (c) 2004-2024 The FlameRobin Development Team

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
#include "metadata/FB30/User30.h"

Users30::Users30(DatabasePtr database)
    :Users(database)
{
}

void Users30::load(ProgressIndicator* progressIndicator)
{
    DatabasePtr db = getDatabase();
    wxString stmt = "select sec$user_name from sec$users order by 1 ";
    setItems(db->loadIdentifiers(stmt, progressIndicator));
}


void User30::loadProperties()
{

    DatabasePtr db = getDatabase();

    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = db->getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(
        "select sec$user_name, "
        "sec$first_name, "
        "sec$middle_name, "
        "sec$last_name, "
        "sec$active, "
        "sec$admin, "
        "sec$description, "
        "sec$plugin "
        "from sec$users "
        "where sec$user_name = ? "
    );
    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    if (!st1->Fetch())
        throw FRError(_("User not found: ") + getName_());

    setPropertiesLoaded(false);
    std::string lstr;


    if (st1->IsNull(2)) 
        lstr = ""; 
    else  
        st1->Get(2, lstr);
    setFirstName(lstr);
    
    if (st1->IsNull(3))
        lstr = "";
    else
        st1->Get(3, lstr);
    setMiddleName(lstr);

    if (st1->IsNull(4))
        lstr = "";
    else
        st1->Get(4, lstr);
    setLastName(lstr);

    if (st1->IsNull(4))
        lstr = "";
    else
        st1->Get(4, lstr);
    setLastName(lstr);

    setPropertiesLoaded(true);
    notifyObservers();
}


User30::User30(DatabasePtr database, const wxString& name)
    :User(database, name)
{
}

wxString User30::getAlterSqlStatement()
{
    ensurePropertiesLoaded();
    wxString sql = "ALTER USER " + getName_() + " \n" +
        "PASSWORD '' \n"
        "FIRSTNAME '" + getFirstName() + "' \n"
        "MIDDLENAME '" + getMiddleName() + "' \n"
        "LASTNAME '" + getLastName() + "' \n"
        "{GRANT | REVOKE} ADMIN ROLE \n"
        ;
    return sql;
}

