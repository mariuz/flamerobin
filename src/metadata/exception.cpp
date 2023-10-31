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
#include "core/ProgressIndicator.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "metadata/database.h"
#include "metadata/exception.h"
#include "metadata/MetadataItemVisitor.h"
#include "sql/StatementBuilder.h"

/*static*/
std::string Exception::getLoadStatement(bool list)
{
    std::string stmt("select"
            " rdb$exception_name,"          // 1
            " rdb$message,"                 // 2
            " rdb$exception_number,"        // 3
            " rdb$description"              // 4
        " from rdb$exceptions");
    if (list)
    {
        stmt += " where rdb$system_flag is null or rdb$system_flag = 0"
            " order by 1";
    }
    else
        stmt += " where rdb$exception_name = ?";
    return stmt;
}

Exception::Exception(DatabasePtr database, const wxString& name)
    : MetadataItem(ntException, database.get(), name), numberM(0)
{
}

wxString Exception::getMessage()
{
    ensurePropertiesLoaded();
    return messageM;
}

int Exception::getNumber()
{
    ensurePropertiesLoaded();
    return numberM;
}

void Exception::loadProperties()
{
    setPropertiesLoaded(false);

    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = db->getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(getLoadStatement(false));
    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    if (!st1->Fetch())
        throw FRError(_("Exception not found: ") + getName_());

    loadProperties(st1, converter);
}

void Exception::loadProperties(IBPP::Statement& statement, wxMBConv* converter)
{
    setPropertiesLoaded(false);

    std::string message;
    statement->Get(2, message);
    messageM = wxString(message.c_str(), *converter);
    statement->Get(3, numberM);
    if (statement->IsNull(4))
        setDescriptionIsEmpty();

    setPropertiesLoaded(true);
}

wxString Exception::getAlterSql()
{
    wxString message = getMessage();
    message.Replace("'", "''");

    StatementBuilder sb;
    sb << kwALTER << ' ' << kwEXCEPTION << ' ' << getQuotedName() << " '"
        << message << "';";
    return sb;
}

const wxString Exception::getTypeName() const
{
    return "EXCEPTION";
}

void Exception::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitException(*this);
}

std::vector<Privilege>* Exception::getPrivileges(bool splitPerGrantor)
{
    // load privileges from database and return the pointer to collection
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();

    privilegesM.clear();

    // first start a transaction for metadata loading, then lock the relation
    // when objects go out of scope and are destroyed, object will be unlocked
    // before the transaction is committed - any update() calls on observers
    // can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);
    wxMBConv* converter = db->getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(
        "select RDB$USER, RDB$USER_TYPE, RDB$GRANTOR, RDB$PRIVILEGE, "
        "RDB$GRANT_OPTION, RDB$FIELD_NAME "
        "from RDB$USER_PRIVILEGES "
        "where RDB$RELATION_NAME = ? and rdb$object_type = 7 "
        "order by rdb$user, rdb$user_type, rdb$grantor, rdb$grant_option, rdb$privilege"
    );
    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    std::string lastuser;
    std::string lastGrantor;
    int lasttype = -1;
    Privilege* pr = 0;
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
        if (!pr || user != lastuser || usertype != lasttype || (splitPerGrantor && grantor != lastGrantor))
        {
            Privilege p(this, wxString(user.c_str(), *converter).Strip(), usertype);
            privilegesM.push_back(p);
            pr = &privilegesM.back();
            lastuser = user;
            lastGrantor = grantor;
            lasttype = usertype;
        }
        pr->addPrivilege(privilege[0], std2wxIdentifier(grantor, converter),
            grantoption == 1, std2wxIdentifier(field, converter));
    }
    return &privilegesM;
}

// Exceptions collection
Exceptions::Exceptions(DatabasePtr database)
    : MetadataCollection<Exception>(ntExceptions, database, _("Exceptions"))
{
}

void Exceptions::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitExceptions(*this);
}

void Exceptions::load(ProgressIndicator* progressIndicator)
{
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = db->getCharsetConverter();

    IBPP::Statement& st1 = loader->getStatement(
        Exception::getLoadStatement(true));

    CollectionType exceptions;
    st1->Execute();
    checkProgressIndicatorCanceled(progressIndicator);
    while (st1->Fetch())
    {
        if (!st1->IsNull(1))
        {
            std::string s;
            st1->Get(1, s);
            wxString name(std2wxIdentifier(s, converter));

            ExceptionPtr exception = findByName(name);
            if (!exception)
            {
                exception.reset(new Exception(db, name));
                initializeLockCount(exception, getLockCount());
            }
            exceptions.push_back(exception);
            exception->loadProperties(st1, converter);
        }
        checkProgressIndicatorCanceled(progressIndicator);
    }

    setItems(exceptions);
}

void Exceptions::loadChildren()
{
    load(0);
}

const wxString Exceptions::getTypeName() const
{
    return "EXCEPTION_COLLECTION";
}

