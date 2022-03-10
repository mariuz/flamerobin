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

// needed for platform independent EOL
#include <wx/textbuf.h>

#include <string>

#include <ibpp.h>

#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "gui/AdvancedMessageDialog.h"
#include "metadata/domain.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/parameter.h"
#include "metadata/package.h"
#include "metadata/function.h"
#include "metadata/procedure.h"

Package::Package(DatabasePtr database, const wxString& name)
    : MetadataItem(ntPackage, database.get(), name)
{
}

void Package::loadChildren()
{
    bool childrenWereLoaded = childrenLoaded();
    // in case an exception is thrown this should be repeated
    setChildrenLoaded(false);
  
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the procedure
    // when objects go out of scope and are destroyed, procedure will be
    // unlocked before the transaction is committed - any update() calls on
    // observers can possibly use the same transaction
    // when objects go out of scope and are destroyed, object will be unlocked
    // before the transaction is committed - any update() calls on observers
    // can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(db.get());
    wxMBConv* converter = db->getCharsetConverter();

    std::string sql(
        "select 15 rdb$object_type, rdb$function_name "
        "from rdb$functions "
        "where rdb$private_flag = 0 and rdb$package_name = ? "
        "union all "
        "select 05 rdb$object_type,rdb$procedure_name "
        "from rdb$procedures "
        "where rdb$private_flag = 0 and rdb$package_name = ? "
        "order by 1,2 "
    );

    IBPP::Statement st1 = loader->getStatement(sql);
    st1->Set(1, wx2std(getName_(), converter));
    st1->Set(2, wx2std(getName_(), converter));
    st1->Execute();

    FunctionSQLPtrs functions;
    ProcedurePtrs procedures;
    while (st1->Fetch())
    {
        short objtype = -1;
        st1->Get(1, &objtype);
        std::string s;
        st1->Get(2, s);
        wxString method_name(std2wxIdentifier(s, converter));

        if (objtype == 15) {
            FunctionSQLPtr fun = findFunctionSQL(method_name);
            if (!fun) {
                fun.reset(new FunctionSQL(this, method_name));
                initializeLockCount(fun, getLockCount());
            }
            functions.push_back(fun);
        }else{
            ProcedurePtr prc = findProcedure(method_name);
            if (!prc) {
                prc.reset(new Procedure(this, method_name));
                initializeLockCount(prc, getLockCount());
            }
            procedures.push_back(prc);
        }
    }

    setChildrenLoaded(true);

    if (!childrenWereLoaded /*|| methodsM != methods*/)
    {
        //methodsM.swap(methods);
        functionsM.swap(functions);
        proceduresM.swap(procedures);
        notifyObservers();
    }

}

bool Package::getChildren(std::vector<MetadataItem *>& temp)
{
    if (functionsM.empty() && proceduresM.empty())
        return false;
    std::transform(functionsM.begin(), functionsM.end(),
        std::back_inserter(temp), std::mem_fn(&FunctionSQLPtr::get));
    std::transform(proceduresM.begin(), proceduresM.end(),
        std::back_inserter(temp), std::mem_fn(&ProcedurePtr::get));
    return !(functionsM.empty() && proceduresM.empty());
}

void Package::lockChildren()
{
    std::for_each(methodsM.begin(), methodsM.end(),
        std::mem_fn(&Method::lockSubject));
    std::for_each(functionsM.begin(), functionsM.end(),
        std::mem_fn(&FunctionSQL::lockSubject));
    std::for_each(proceduresM.begin(), proceduresM.end(),
        std::mem_fn(&Procedure::lockSubject));

}

void Package::unlockChildren()
{
    std::for_each(methodsM.begin(), methodsM.end(),
        std::mem_fn(&Method::unlockSubject));
    std::for_each(functionsM.begin(), functionsM.end(),
        std::mem_fn(&FunctionSQL::unlockSubject));
    std::for_each(proceduresM.begin(), proceduresM.end(),
        std::mem_fn(&Procedure::unlockSubject));
}

FunctionSQLPtr Package::findFunctionSQL(const wxString& name) const
{
    for (FunctionSQLPtrs::const_iterator it = functionsM.begin();
        it != functionsM.end(); ++it)
    {
        if ((*it)->getName_() == name)
            return *it;
    }
    return FunctionSQLPtr();
}

ProcedurePtr Package::findProcedure(const wxString& name) const
{
    for (ProcedurePtrs::const_iterator it = proceduresM.begin();
        it != proceduresM.end(); ++it)
    {
        if ((*it)->getName_() == name)
            return *it;
    }
    return ProcedurePtr();
}

MethodPtrs::iterator Package::begin()
{
    // please - don't load here
    // this code is used to get columns we want to alert about changes
    // but if there aren't any columns, we don't want to waste time
    // loading them
    return methodsM.begin();
}

MethodPtrs::iterator Package::end()
{
    // please see comment for begin()
    return methodsM.end();    
}

MethodPtrs::const_iterator Package::begin() const
{
    return methodsM.begin();
}

MethodPtrs::const_iterator Package::end() const
{
    return methodsM.end();
}

MethodPtr Package::findMethod(const wxString& name) const
{
    for (MethodPtrs::const_iterator it = methodsM.begin();
        it != methodsM.end(); ++it)
    {
        if ((*it)->getName_() == name)
            return *it;
    }
    return MethodPtr();
}

size_t Package::getMethodCount() const
{
    //return methodsM.size();
    return functionsM.size() + proceduresM.size();
}

wxString Package::getOwner()
{
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
	wxMBConv* converter = db->getCharsetConverter();
	std::string sql(
		"select rdb$owner_name from rdb$packages where rdb$package_name = ?"
	);
	IBPP::Statement st1 = loader->getStatement(sql);
	st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    st1->Fetch();
    std::string name;
    st1->Get(1, name);
    return std2wxIdentifier(name, converter);
}

wxString Package::getSource()
{
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
	wxMBConv* converter = db->getCharsetConverter();

	std::string sql(
		"select rdb$package_body_source "
        "from rdb$packages "
        "where rdb$package_name = ? "
	);
	IBPP::Statement st1 = loader->getStatement( sql );
	st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    st1->Fetch();
    wxString source;
	if (!st1->IsNull(1))
	{
        wxString source1;
        readBlob(st1, 1, source1, converter);
        source1.Trim(false);     // remove leading whitespace
        source += source1;
    }
    return source;
}

wxString Package::getDefinition()
{
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
    wxMBConv* converter = db->getCharsetConverter();

    std::string sql(
        "select rdb$package_header_source "
        "from rdb$packages "
        "where rdb$package_name = ? "
    );
    IBPP::Statement st1 = loader->getStatement(sql);
    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    st1->Fetch();
    wxString source;
    if (!st1->IsNull(1))
    {
        wxString source1;
        readBlob(st1, 1, source1, converter);
        source1.Trim(false);     // remove leading whitespace
        source += source1;
    }
    return source;
}

wxString Package::getSqlSecurity()
{
    DatabasePtr db = getDatabase();
    if (db->getInfo().getODSVersionIsHigherOrEqualTo(13, 0))
    {
        MetadataLoader* loader = db->getMetadataLoader();
        MetadataLoaderTransaction tr(loader);
        wxMBConv* converter = db->getCharsetConverter();
        std::string sql(
            "select rdb$sql_security "
            "from rdb$packages where rdb$package_name = ? "
            );
        IBPP::Statement st1 = loader->getStatement(sql);
        st1->Set(1, wx2std(getName_(), converter));
        st1->Execute();
        st1->Fetch();
        bool b;
        st1->Get(1, b);
        return wxString(b ? "SQL SECURITY DEFINER" : "SQL SECURITY INVOKER");
    }
    else
    {
        return wxString();
    }
}

wxString Package::getAlterHeader()
{
    wxString sql = "SET TERM ^ ;\n ";
    sql += "ALTER PACKAGE " + getQuotedName() + "\n";
    sql += "AS \n";
    sql += getDefinition();
    sql += "^\nSET TERM ; ^\n";
    return sql;
}

wxString Package::getAlterBody()
{
    wxString sql = "SET TERM ^ ;\n";
    sql+= "RECREATE PACKAGE BODY " + getQuotedName()+"\n";
    sql += "AS\n";
    sql += getSource();
    sql += "^\nSET TERM ; ^\n";
    return sql;
}

wxString Package::getAlterSql(bool WXUNUSED(full))
{
    ensureChildrenLoaded();


    wxString sql = getAlterHeader();
    sql += "\n \n";
    sql += getAlterBody();
    return sql;
}

void Package::checkDependentPackage()
{
    // check dependencies and parameters
    ensureChildrenLoaded();
    std::vector<Dependency> deps;
    getDependencies(deps, false);

    // if there is a dependency, but parameter doesn't exist, warn the user
    int count = 0;
    wxString missing;
    for (std::vector<Dependency>::iterator it = deps.begin();
        it != deps.end(); ++it)
    {
        std::vector<DependencyField> fields;
        (*it).getFields(fields);
        for (std::vector<DependencyField>::const_iterator ci = fields.begin();
            ci != fields.end(); ++ci)
        {
            bool found = false;
            for (MethodPtrs::iterator i2 = begin();
                i2 != end(); ++i2)
            {
                if ((*i2)->getName_() == (ci->getName_()))
                {
                    found = true;
                    break;
                }
            }
            if (!found && ++count < 20)
            {
                missing += wxString::Format(
                    _("Package %s depends on parameter %s.%s"),
                    (*it).getName_().c_str(),
                    (ci->getName_()).c_str(),
                    wxTextBuffer::GetEOL()
                );
            }
        }
    }
    if (count > 0)
    {
        if (count > 19)
        {
            missing += wxTextBuffer::GetEOL()
            + wxString::Format(_("%d total dependencies (20 shown)."), count);
        }
        showWarningDialog(0,
            _("Dependencies broken"),
            wxString::Format(
                _("Some other packages depend on %s:%s%s%s"),
                getName_().c_str(),
                wxTextBuffer::GetEOL(),
                wxTextBuffer::GetEOL(),
                missing.c_str()),
            AdvancedMessageDialogButtonsOk()
        );
    }
}

std::vector<Privilege>* Package::getPrivileges()
{
    // load privileges from database and return the pointer to collection
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    // first start a transaction for metadata loading, then lock the procedure
    // when objects go out of scope and are destroyed, procedure will be
    // unlocked before the transaction is committed - any update() calls on
    // observers can possibly use the same transaction
    MetadataLoaderTransaction tr(loader);
    SubjectLocker lock(this);
    wxMBConv* converter = db->getCharsetConverter();

    privilegesM.clear();

    IBPP::Statement st1 = loader->getStatement(
        "select RDB$USER, RDB$USER_TYPE, RDB$GRANTOR, RDB$PRIVILEGE, "
        "RDB$GRANT_OPTION, RDB$FIELD_NAME "
        "from RDB$USER_PRIVILEGES "
        "where RDB$RELATION_NAME = ? and rdb$object_type in( 18, 19 ) "
        "order by rdb$user, rdb$user_type, rdb$privilege"
    );
    st1->Set(1, wx2std(getName_(), converter));
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
            Privilege p(this, std2wxIdentifier(user, converter),
                usertype);
            privilegesM.push_back(p);
            pr = &privilegesM.back();
            lastuser = user;
            lasttype = usertype;
        }
        pr->addPrivilege(privilege[0], wxString(grantor.c_str(), *converter),
            grantoption == 1);
    }
    return &privilegesM;
}

const wxString Package::getTypeName() const
{
    return "PACKAGE";
}

void Package::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitPackage(*this);
}

// Packages collection
Packages::Packages(DatabasePtr database)
    : MetadataCollection<Package>(ntPackages, database, _("Packages"))
{
}

void Packages::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitPackages(*this);
}

void Packages::load(ProgressIndicator* progressIndicator)
{
	DatabasePtr db = getDatabase();
    wxString stmt = "select rdb$package_name from rdb$packages ";
    stmt += " where rdb$system_flag = 0 ";
	stmt += " order by rdb$package_name ";
    setItems(db->loadIdentifiers(stmt, progressIndicator));
}

void Packages::loadChildren()
{
    load(0);
}

const wxString Packages::getTypeName() const
{
    return "PACKAGE_COLLECTION";
}

Method::Method(MetadataItem* parent, 
    const wxString& name)
    : MetadataItem(ntMethod, parent, name), functionM(false)
{
}

bool Method::isFunction() const
{
    return functionM;
}

const wxString Method::getTypeName() const
{
    return "METHOD";
}

void Method::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitMethod(*this);
}

void Method::initialize(int MethodType)
{
    functionM = MethodType == 15;
}

wxString Method::getQuotedName() const
{
    return getParent()->getQuotedName() + '.' + MetadataItem::getQuotedName();
}

// System Packages collection

SysPackages::SysPackages(DatabasePtr database)
    : MetadataCollection<Package>(ntSysPackages, database, _("System Packages"))
{
}

void SysPackages::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitSysPackages(*this);
}

void SysPackages::load(ProgressIndicator* progressIndicator)
{
    DatabasePtr db = getDatabase();
    wxString stmt = "select rdb$package_name from rdb$packages ";
    stmt += " where rdb$system_flag = 1 ";
    stmt += " order by rdb$package_name ";
    setItems(db->loadIdentifiers(stmt, progressIndicator));
}

const wxString SysPackages::getTypeName() const
{
    return "SYSPACKAGE_COLLECTION";
}

void SysPackages::loadChildren()
{
    load(0);
}
