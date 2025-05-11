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
#include "metadata/procedure.h"

Procedure::Procedure(DatabasePtr database, const wxString& name)
    : MetadataItem(ntProcedure, database.get(), name)
{
}

Procedure::Procedure(MetadataItem* parent, const wxString& name)
    : MetadataItem(ntProcedure, parent, name)
{
}

void Procedure::loadChildren()
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
        "select rdb$parameter_name, rdb$field_source, "
        "rdb$parameter_type, "
    );
    sql += db->getInfo().getODSVersionIsHigherOrEqualTo(11, 1) ? "rdb$default_source, rdb$null_flag, rdb$parameter_mechanism,  " : "null, null, -1, ";
    sql += db->getInfo().getODSVersionIsHigherOrEqualTo(11, 2) ? " rdb$field_name, rdb$relation_name, " : " null, null, ";
	sql += "rdb$description from rdb$procedure_parameters "
		"where rdb$procedure_name = ? ";
    if (getParent()->getType() == ntDatabase) {
        sql += db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0) ? " and rdb$package_name is null " : "";
    }
    else
        if (getParent()->getType() == ntPackage) {
            sql += db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0) ? " and rdb$package_name = ? " : "";
        }
    sql += "order by rdb$parameter_type, rdb$parameter_number";

    IBPP::Statement st1 = loader->getStatement(sql);
    st1->Set(1, wx2std(getName_(), converter));
    if (getParent()->getType() == ntPackage) {
        st1->Set(2, wx2std(getParent()->getName_(), converter));
    }
    st1->Execute();

    ParameterPtrs parameters;
    while (st1->Fetch())
    {
        std::string s;
        st1->Get(1, s);
        wxString param_name(std2wxIdentifier(s, converter));
        st1->Get(2, s);
        wxString source(std2wxIdentifier(s, converter));

        short partype, mechanism = -1;
        st1->Get(3, &partype);
        bool hasDefault = !st1->IsNull(4);
        wxString defaultSrc;
        if (hasDefault)
        {
            st1->Get(4, s);
            defaultSrc = std2wxIdentifier(s, converter);
        }
        bool notNull = false;
        if (!st1->IsNull(5))
            st1->Get(5, &notNull);
        if (!st1->IsNull(6))
            st1->Get(6, mechanism);
        wxString field;
        if (!st1->IsNull(7)){
            st1->Get(7, s);
            field = std2wxIdentifier(s, converter);
        }
        wxString relation;
        if (!st1->IsNull(8)) {
            st1->Get(8, s);
            relation = std2wxIdentifier(s, converter);
        }

        bool hasDescription = !st1->IsNull(9);

        ParameterPtr par = findParameter(param_name);
        if (!par)
        {
            par.reset(new Parameter(this, param_name));
            initializeLockCount(par, getLockCount());
        }
        parameters.push_back(par);
        par->initialize(source, partype, mechanism, !notNull, defaultSrc,
            hasDefault, hasDescription, relation, field);
    }

    setChildrenLoaded(true);
    if (!childrenWereLoaded || parametersM != parameters)
    {
        parametersM.swap(parameters);
        notifyObservers();
    }
}

bool Procedure::getChildren(std::vector<MetadataItem *>& temp)
{
    if (parametersM.empty())
        return false;
    std::transform(parametersM.begin(), parametersM.end(),
        std::back_inserter(temp), std::mem_fn(&ParameterPtr::get));
    return !parametersM.empty();
}

void Procedure::lockChildren()
{
    std::for_each(parametersM.begin(), parametersM.end(),
        std::mem_fn(&Parameter::lockSubject));
}

void Procedure::unlockChildren()
{
    std::for_each(parametersM.begin(), parametersM.end(),
        std::mem_fn(&Parameter::unlockSubject));
}

ParameterPtrs::iterator Procedure::begin()
{
    // please - don't load here
    // this code is used to get columns we want to alert about changes
    // but if there aren't any columns, we don't want to waste time
    // loading them
    return parametersM.begin();
}

ParameterPtrs::iterator Procedure::end()
{
    // please see comment for begin()
    return parametersM.end();
}

ParameterPtrs::const_iterator Procedure::begin() const
{
    return parametersM.begin();
}

ParameterPtrs::const_iterator Procedure::end() const
{
    return parametersM.end();
}

ParameterPtr Procedure::findParameter(const wxString& name) const
{
    for (ParameterPtrs::const_iterator it = parametersM.begin();
        it != parametersM.end(); ++it)
    {
        if ((*it)->getName_() == name)
            return *it;
    }
    return ParameterPtr();
}

size_t Procedure::getParamCount() const
{
    return parametersM.size();
}

wxString Procedure::getOwner()
{
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);

    IBPP::Statement st1 = loader->getStatement(
        "select rdb$owner_name from rdb$procedures where rdb$procedure_name = ?");
    st1->Set(1, wx2std(getName_(), db->getCharsetConverter()));
    st1->Execute();
    st1->Fetch();
    std::string name;
    st1->Get(1, name);
    return std2wxIdentifier(name, db->getCharsetConverter());
}

wxString Procedure::getSource()
{
    DatabasePtr db = getDatabase();
    MetadataLoader* loader = db->getMetadataLoader();
    MetadataLoaderTransaction tr(loader);
	wxMBConv* converter = db->getCharsetConverter();

	std::string sql(
		"select rdb$procedure_source, "
	);
    sql += db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0) ? "rdb$entrypoint, rdb$engine_name  " : "null, null ";
	sql += "from rdb$procedures where rdb$procedure_name = ?";
	if (db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0))
		sql += " and rdb$package_name is null ";
	IBPP::Statement st1 = loader->getStatement( sql );
	st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    st1->Fetch();
    wxString source;
	if (!st1->IsNull(2))
	{
		std::string s;
		st1->Get(2, s);
		source += "EXTERNAL NAME '"+std2wxIdentifier(s, converter)+ "' \n";
        if (!st1->IsNull(3))
        {
            s.clear();
            st1->Get(3, s);
            source += "ENGINE " + std2wxIdentifier(s, converter) + "\n";
            if (!st1->IsNull(1))
            {
                wxString source1;
                readBlob(st1, 1, source1, converter);
                source1.Trim(false);     // remove leading whitespace
                source += "\nAS\n" + source1 + "\n";
            }
        }
    }
	else
	{
		wxString source1;
		readBlob(st1, 1, source1, converter);
		source1.Trim(false);     // remove leading whitespace
		source += "\nAS\n" + source1 + "\n";
	}
    return source;
}

wxString Procedure::getDefinition()
{
    ensureChildrenLoaded();
    wxString collist, parlist;
    ParameterPtrs::const_iterator lastInput, lastOutput;
    for (ParameterPtrs::const_iterator it = parametersM.begin();
        it != parametersM.end(); ++it)
    {
        if ((*it)->isOutputParameter())
            lastOutput = it;
        else
            lastInput = it;
    }
    for (ParameterPtrs::const_iterator it =
        parametersM.begin(); it != parametersM.end(); ++it)
    {
        // No need to quote domains, as currently only regular datatypes can be
        // used for SP parameters
        if ((*it)->isOutputParameter())
        {
            collist += "    " + (*it)->getQuotedName() + " "
                + (*it)->getDomain()->getDatatypeAsString();
            if (it != lastOutput)
                collist += ",";
            collist += "\n";
        }
        else
        {
            parlist += "    " + (*it)->getQuotedName() + " "
                + (*it)->getDomain()->getDatatypeAsString();
            if (it != lastInput)
                parlist += ",";
            parlist += "\n";
        }
    }
    wxString retval = getQuotedName();
    if (!parlist.empty())
        retval += "(\n" + parlist + ")";
    retval += "\n";
    if (!collist.empty())
        retval += "returns:\n" + collist;
    return retval;
}

wxString Procedure::getSqlSecurity()
{
    DatabasePtr db = getDatabase();
    if (db->getInfo().getODSVersionIsHigherOrEqualTo(13, 0))
    {
        MetadataLoader* loader = db->getMetadataLoader();
        MetadataLoaderTransaction tr(loader);
        wxMBConv* converter = db->getCharsetConverter();
        std::string sql(
            "select rdb$sql_security from rdb$procedures where rdb$procedure_name = ?"
            " and rdb$package_name is null ");
        IBPP::Statement st1 = loader->getStatement(sql);
        st1->Set(1, wx2std(getName_(), converter));
        st1->Execute();
        st1->Fetch();
        if (st1->IsNull(1))
            return wxString();
        bool b;
        st1->Get(1, b);
        return wxString(b ? "SQL SECURITY DEFINER" : "SQL SECURITY INVOKER");
    }
    else
    {
        return wxString();
    }
}

wxString Procedure::getAlterSql(bool full)
{
    ensureChildrenLoaded();

    DatabasePtr db = getDatabase();
    wxString input, output;

    wxString sql = "SET TERM ^ ;\nALTER PROCEDURE " + getQuotedName();
    if (!parametersM.empty())
    {
        for (ParameterPtrs::const_iterator it = parametersM.begin();
            it != parametersM.end(); ++it)
        {
            wxString charset;
            wxString param = (*it)->getQuotedName() + " ";
            DomainPtr dm = (*it)->getDomain();
            if ((*it)->getMechanism() == 1 && full) { //when header only, it's better to get the type from the domain to avoud dependency lock
                param += (*it)->getTypeOf();
            }else
            if (dm){
                if (dm->isSystem()) // autogenerated domain -> use datatype
                {
                    param += dm->getDatatypeAsString();
                    charset = dm->getCharset();
                    if (!charset.empty())
                    {
                        if (charset != db->getDatabaseCharset())
                            charset = " CHARACTER SET " + charset;
                        else
                            charset.clear();
                    }
                }
                else
                {
                    if ((*it)->getMechanism() == 1 && full) //when header only, it's better to get the type from the domain to avoud dependency lock
                        param += (*it)->getTypeOf();
                    else
                        param += dm->getQuotedName();
                }
            }
            else
                param += (*it)->getSource();

            if ((*it)->isOutputParameter())
            {
                if (output.empty())
                    output += "\nRETURNS (\n    ";
                else
                    output += ",\n    ";
                output += param + charset;
                if (!(*it)->isNullable(IgnoreDomainNullability))
                    output += " NOT NULL";
            }
            else
            {
                if (input.empty())
                    input += " (\n    ";
                else
                    input += ",\n    ";
                input += param;
                input += charset;
                if (!(*it)->isNullable(IgnoreDomainNullability))
                    input += " NOT NULL";
                wxString defaultValue;
                // default either from parameter itself
                if ((*it)->getDefault(IgnoreDomainDefault, defaultValue)
                    // or from underlying autogenerated domain
                    || (dm && dm->isSystem() && dm->getDefault(defaultValue)))
                {
                    input += " DEFAULT " + defaultValue;
                }
            }
        }

        if (!input.empty())
            sql += input + " )";
        if (!output.empty())
            sql += output + " )";
    }
    sql += +"\n" + getSqlSecurity() + "\n";
    if (full)
        sql += getSource();
    else {
        if (!output.empty())
            sql += "AS \nBEGIN SUSPEND; \nEND";
        else
        sql += "AS \nBEGIN \nEND";
    }
        
    sql += "^\nSET TERM ; ^\n";
    return sql;
}

void Procedure::checkDependentProcedures()
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
            for (ParameterPtrs::iterator i2 = begin();
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
                    _("Procedure %s depends on parameter %s.%s"),
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
                _("Some other procedures depend on %s:%s%s%s"),
                getName_().c_str(),
                wxTextBuffer::GetEOL(),
                wxTextBuffer::GetEOL(),
                missing.c_str()),
            AdvancedMessageDialogButtonsOk()
        );
    }
}

std::vector<Privilege>* Procedure::getPrivileges(bool splitPerGrantor)
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
        "where RDB$RELATION_NAME = ? and rdb$object_type = 5 "
        "order by rdb$user, rdb$user_type, rdb$grantor, rdb$privilege"
    );
    st1->Set(1, wx2std(getName_(), converter));
    st1->Execute();
    std::string lastuser;
    std::string lastGrantor;
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
        if (!pr || user != lastuser || usertype != lasttype || (splitPerGrantor && grantor != lastGrantor))
        {
            Privilege p(this, std2wxIdentifier(user, converter),
                usertype);
            privilegesM.push_back(p);
            pr = &privilegesM.back();
            lastuser = user;
            lastGrantor = grantor;
            lasttype = usertype;
        }
        pr->addPrivilege(privilege[0], wxString(grantor.c_str(), *converter),
            grantoption == 1);
    }
    return &privilegesM;
}

const wxString Procedure::getTypeName() const
{
    return "PROCEDURE";
}

void Procedure::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitProcedure(*this);
}

wxString Procedure::getQuotedName() const
{
    if (getParent()->getType() == ntDatabase)
        return MetadataItem::getQuotedName();
    else
        return getParent()->getQuotedName() + '.' + MetadataItem::getQuotedName();
}

// Procedures collection
Procedures::Procedures(DatabasePtr database)
    : MetadataCollection<Procedure>(ntProcedures, database, _("Procedures"))
{
}

void Procedures::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitProcedures(*this);
}

void Procedures::load(ProgressIndicator* progressIndicator)
{
    wxString stmt = "select rdb$procedure_name from rdb$procedures"
        " where (rdb$system_flag = 0 or rdb$system_flag is null)";
    stmt += getDatabase()->getInfo().getODSVersionIsHigherOrEqualTo(12, 0) ? " and rdb$package_name is null " : " ";
    stmt += " order by 1";
    setItems(getDatabase()->loadIdentifiers(stmt, progressIndicator));
}

void Procedures::loadChildren()
{
    load(0);
}

const wxString Procedures::getTypeName() const
{
    return "PROCEDURE_COLLECTION";
}

