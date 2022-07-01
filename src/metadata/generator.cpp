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
#include "metadata/generator.h"
#include "metadata/MetadataItemVisitor.h"
#include "sql/StatementBuilder.h"

Generator::Generator(DatabasePtr database, const wxString& name)
    : MetadataItem(ntGenerator, database.get(), name), 
    initialValueM(0) ,incrementalValueM(0)
{
}

int64_t Generator::getValue()
{
    ensurePropertiesLoaded();
    return valueM;
}

void Generator::loadProperties()
{
    setPropertiesLoaded(false);

    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = db->getCharsetConverter();

    // IMPORTANT: for all other loading where the name of the db object is
    // Set() into a parameter getName_() is used, but for dynamically
    // building the SQL statement getQuotedName() must be used!
    std::string sqlName(wx2std(getQuotedName(), converter));
    // do not use cached statements, because this can not be reused
    IBPP::Statement st1 = loader->createStatement(
        "select gen_id(" + sqlName + ", 0) from rdb$database");

    st1->Execute();
    st1->Fetch();
    st1->Get(1, &valueM);
    if (db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0)) {
        std::string sql("select RDB$INITIAL_VALUE, RDB$GENERATOR_INCREMENT from RDB$GENERATORS "
            "where RDB$GENERATOR_NAME = ? "
        );
        IBPP::Statement st2 = loader->createStatement(sql);
        st2->Set(1, wx2std(getName_(), converter));
        st2->Execute();
        if (st2->Fetch()) {
            st2->Get(1, initialValueM);
            st2->Get(2, incrementalValueM);

        }

    }

    setPropertiesLoaded(true);
    notifyObservers();
}

const wxString Generator::getTypeName() const
{
    return "GENERATOR";
}

void Generator::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitGenerator(*this);
}

wxString Generator::getSource()
{
    ensurePropertiesLoaded();
    StatementBuilder sql;
    sql << "SEQUENCE" << " " << getQuotedName() << " ";
    if (initialValueM != 0) {
        sql << kwSTART << " " << kwWITH << " " << std::to_string(initialValueM);
        if (incrementalValueM != 0) {
            sql << " INCREMENT BY " << std::to_string(incrementalValueM); 
        }
    }
    return sql ;
}

std::vector<Privilege>* Generator::getPrivileges()
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
        "where RDB$RELATION_NAME = ? and rdb$object_type = 14 "
        "order by rdb$user, rdb$user_type, rdb$grant_option, rdb$privilege"
    );
    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    std::string lastuser;
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
        if (!pr || user != lastuser || usertype != lasttype)
        {
            Privilege p(this, wxString(user.c_str(), *converter).Strip(), usertype);
            privilegesM.push_back(p);
            pr = &privilegesM.back();
            lastuser = user;
            lasttype = usertype;
        }
        pr->addPrivilege(privilege[0], std2wxIdentifier(grantor, converter),
            grantoption == 1, std2wxIdentifier(field, converter));
    }
    return &privilegesM;
}

// Generators collection
Generators::Generators(DatabasePtr database)
    : MetadataCollection<Generator>(ntGenerators, database, _("Generators"))
{
}

void Generators::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitGenerators(*this);
}

void Generators::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = "select rdb$generator_name from rdb$generators"
        " where (rdb$system_flag = 0 or rdb$system_flag is null)"
        " order by 1";
    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
}

void Generators::loadChildren()
{
    load(0);
}

const wxString Generators::getTypeName() const
{
    return "GENERATOR_COLLECTION";
}

