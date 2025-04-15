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

#include <ibpp.h>

#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "frutils.h"
#include "gui/AdvancedMessageDialog.h"
#include "metadata/database.h"
#include "metadata/domain.h"
#include "metadata/function.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/parameter.h"
#include "sql/StatementBuilder.h"


Function::Function(DatabasePtr database, const wxString& name)
    : MetadataItem(ntFunction, database.get(), name)
{
    ensurePropertiesLoaded();
}

Function::Function(MetadataItem* parent, const wxString& name)
    : MetadataItem(ntFunction, parent, name)
{
    ensurePropertiesLoaded();
}

void Function::loadChildren()
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


    std::string sql("select");
    if (db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0))
        sql += " a.rdb$argument_name, a.rdb$field_source, "; //1..2
    else
        sql += " null rdb$argument_name, null rdb$field_source, "; //1..2
    sql += "a.rdb$mechanism, a.rdb$field_type, a.rdb$field_scale, a.rdb$field_length, " //3..6
        "a.rdb$field_sub_type, a.rdb$field_precision, "//7..8
        "f.rdb$return_argument, a.rdb$argument_position, "; //9..10
    
    if (db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0))
        sql += "rdb$default_source, rdb$null_flag, rdb$argument_mechanism, rdb$field_name, rdb$relation_name, a.rdb$description "; //11..16
    else
        sql += "null, null, -1, null, null, null";
    sql += " from rdb$function_arguments a "
        " join rdb$functions f on f.rdb$function_name = a.rdb$function_name ";
    sql += db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0)  ? " and ((f.rdb$package_name = a.rdb$package_name) or (a.rdb$package_name is null)) " : "";
    sql += "where a.rdb$function_name = ? ";
    if (getParent()->getType() == ntDatabase) {
        sql += db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0) ? " and a.rdb$package_name is null " : " ";
    }
    else
        if (getParent()->getType() == ntPackage) {
            sql += db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0) ? " and a.rdb$package_name = ? " : "";
        }
    sql += "order by iif(a.rdb$argument_name is null,255, a.rdb$argument_position) ";
 //    sql += "order by iif(a.rdb$argument_name is null, 2014, a.rdb$argument_position) ";

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
        
        short returnarg, retpos;
        st1->Get(9, returnarg);
        st1->Get(10, retpos);

        if (!st1->IsNull(1)) {
            st1->Get(1, s);
        }
        else {
            s = (returnarg == retpos) ? "RETURN " : "";
        }
        wxString param_name(std2wxIdentifier(s, converter));
        if (!st1->IsNull(2)) {
            st1->Get(2, s);
        }
        else {
            if (!st1->IsNull(4)) {
                short  type, scale, length, subtype, precision;
                st1->Get(4, type);
                st1->Get(5, scale);
                st1->Get(6, length);
                st1->Get(7, subtype);
                st1->Get(8, precision);
                s = Domain::dataTypeToString(type, scale, precision, subtype, length);
            }
        }
        wxString source(std2wxIdentifier(s, converter));

        short partype, mechanism = -1;
        partype = (returnarg == retpos) ? 1 : 0;
        /*if (st1->IsNull(3)) {
            partype = st1->IsNull(1) ? 1 : 0;
        }
        else {
            st1->Get(3, partype);
        }*/
        bool hasDefault = !st1->IsNull(11);
        wxString defaultSrc;
        if (hasDefault)
        {
            st1->Get(11, s);
            defaultSrc = std2wxIdentifier(s, converter);
        }
        bool notNull = false;
        if (!st1->IsNull(12))
            st1->Get(12, &notNull);
        if (!st1->IsNull(13)) {
            st1->Get(13, mechanism);
        }
        else {
            if (!st1->IsNull(3))
                st1->Get(3, mechanism);
        }
        wxString field;
        if (!st1->IsNull(14)) {
            st1->Get(14, s);
            field = std2wxIdentifier(s, converter);
        }
        wxString relation;
        if (!st1->IsNull(15)) {
            st1->Get(15, s);
            relation = std2wxIdentifier(s, converter);
        }

        bool hasDescription = !st1->IsNull(16);

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

bool Function::getChildren(std::vector<MetadataItem*>& temp)
{
    if (parametersM.empty())
        return false;
    std::transform(parametersM.begin(), parametersM.end(),
        std::back_inserter(temp), std::mem_fn(&ParameterPtr::get));
    return !parametersM.empty();
}

void Function::lockChildren()
{
    std::for_each(parametersM.begin(), parametersM.end(),
        std::mem_fn(&Parameter::lockSubject));
}

void Function::unlockChildren()
{
	std::for_each(parametersM.begin(), parametersM.end(),
		std::mem_fn(&Parameter::unlockSubject));
}

ParameterPtrs::iterator Function::begin()
{
	// please - don't load here
	// this code is used to get columns we want to alert about changes
	// but if there aren't any columns, we don't want to waste time
	// loading them
	return parametersM.begin();
}

ParameterPtrs::iterator Function::end()
{
	// please see comment for begin()
	return parametersM.end();
}

ParameterPtrs::const_iterator Function::begin() const
{
	return parametersM.begin();
}

ParameterPtrs::const_iterator Function::end() const
{
	return parametersM.end();
}

ParameterPtr Function::findParameter(const wxString& name) const
{
	for (ParameterPtrs::const_iterator it = parametersM.begin();
		it != parametersM.end(); ++it)
	{
		if ((*it)->getName_() == name)
			return *it;
	}
	return ParameterPtr();
}

size_t Function::getParamCount() const
{
	return parametersM.size();
}

wxString Function::getOwner()
{
	DatabasePtr db = getDatabase();
	if (db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0))
	{
		MetadataLoader* loader = db->getMetadataLoader();
		MetadataLoaderTransaction tr(loader);
		wxMBConv* converter = db->getCharsetConverter();
		std::string sql(
			"select rdb$owner_name from rdb$functions where rdb$function_name = ?"
		     " and rdb$package_name is null ");
		IBPP::Statement st1 = loader->getStatement(sql);
		st1->Set(1, wx2std(getName_(), converter));
		st1->Execute();
		st1->Fetch();
		std::string name;
		st1->Get(1, name);
		return std2wxIdentifier(name, converter);
	}
	else
	{
		return wxString();
	}
}

wxString Function::getSqlSecurity()
{
	DatabasePtr db = getDatabase();
	if (db->getInfo().getODSVersionIsHigherOrEqualTo(13, 0))
	{
		MetadataLoader* loader = db->getMetadataLoader();
		MetadataLoaderTransaction tr(loader);
		wxMBConv* converter = db->getCharsetConverter();
		std::string sql(
			"select rdb$sql_security from rdb$functions where rdb$function_name = ?"
			" and rdb$package_name is null ");
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

std::vector<Privilege>* Function::getPrivileges(bool splitPerGrantor)
{
	// load privileges from database and return the pointer to collection
	DatabasePtr db = getDatabase();
	MetadataLoader* loader = db->getMetadataLoader();
	// first start a transaction for metadata loading, then lock the function
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
		"where RDB$RELATION_NAME = ? and rdb$object_type = 15 "
		"order by rdb$user, rdb$user_type, rdb$grantor, rdb$privilege"
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


wxString Function::getDefinition()
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
		// used for Function parameters
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

void Function::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitMetadataItem(*this); 
}

void Function::checkDependentFunction()
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
					_("Function %s depends on parameter %s.%s"),
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
				_("Some other functions depend on %s:%s%s%s"),
				getName_().c_str(),
				wxTextBuffer::GetEOL(),
				wxTextBuffer::GetEOL(),
				missing.c_str()),
			AdvancedMessageDialogButtonsOk()
		);
	}

}


FunctionSQL::FunctionSQL(DatabasePtr database, const wxString & name)
	: Function(database, name), deterministicM(FALSE)
{
    setType(ntFunctionSQL);
}

FunctionSQL::FunctionSQL(MetadataItem* parent, const wxString& name)
    : Function(parent, name), deterministicM(FALSE)
{
    setType(ntFunctionSQL);
}

void FunctionSQL::loadProperties()
{
    getSource();
}

wxString FunctionSQL::getSource()
{
	DatabasePtr db = getDatabase();
	MetadataLoader* loader = db->getMetadataLoader();
	MetadataLoaderTransaction tr(loader);
	wxMBConv* converter = db->getCharsetConverter();
	std::string sql = "select rdb$function_source, ";
	sql += db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0) ? "rdb$entrypoint, rdb$engine_name, rdb$deterministic_flag  " : "null, null, null ";
	sql += "from rdb$functions where rdb$function_name = ?";
	sql += " and rdb$package_name is null ";
	IBPP::Statement st1 = loader->getStatement(sql);
	st1->Set(1, wx2std(getName_(), converter));
	st1->Execute();
	st1->Fetch();
	wxString source;
	if (!st1->IsNull(2))
	{
		std::string s;
		st1->Get(2, s);
		source += "EXTERNAL NAME '" + std2wxIdentifier(s, converter) + "'\n";
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

    if (!st1->IsNull(4)) {
        int i = 0;
        st1->Get(4, i);
        deterministicM = (i == 1);
    }

	return source;
}

wxString FunctionSQL::getAlterSql(bool full)
{
    ensurePropertiesLoaded();

    ensureChildrenLoaded();

	DatabasePtr db = getDatabase();

	wxString sql = "SET TERM ^ ;\nALTER FUNCTION " + getQuotedName();
	if (getParamCount() > 0)
	{
		wxString input, output;
		for (ParameterPtrs::const_iterator it = begin();
			it != end(); ++it)
		{
			wxString charset;
			wxString param = (*it)->isOutputParameter() ? "" : (*it)->getQuotedName() + " ";
			DomainPtr dm = (*it)->getDomain();
            if ((*it)->getMechanism() == 1 && full) { //when header only, it's better to get the type from the domain to avoid dependency lock
                param += (*it)->getTypeOf();
            }
            else
                if (dm)
			{
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
                    if ((*it)->getMechanism() == 1 && full)
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
					output += "\nRETURNS ";
				else
					output += ",\n    ";
				output += param + charset;
				if (!(*it)->isNullable(IgnoreDomainNullability))
					output += " NOT NULL";
                if (deterministicM)
                    output += " DETERMINISTIC ";
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
			sql += output ;
	}
    sql += +"\n" + getSqlSecurity() + "\n";
	if (full)
		sql += getSource();
	else
		sql += "as BEGIN END";
	sql += "^\nSET TERM ; ^\n";
	return sql;
}

wxString FunctionSQL::getDefinition()
{
	return wxString("Function SQL");
}

const wxString FunctionSQL::getTypeName() const
{
	return wxString("FUNCTION");
}

void FunctionSQL::acceptVisitor(MetadataItemVisitor * visitor)
{
	visitor->visitFunctionSQL(*this);
}

wxString FunctionSQL::getQuotedName() const
{
    if (getParent()->getType() == ntDatabase)
        return MetadataItem::getQuotedName();
    else
        return getParent()->getQuotedName() + '.' + MetadataItem::getQuotedName();
}


// Functions SQL collection
FunctionSQLs::FunctionSQLs(DatabasePtr database)
	: MetadataCollection<FunctionSQL>(ntFunctionSQLs, database, _("Functions"))
{
}

void FunctionSQLs::acceptVisitor(MetadataItemVisitor* visitor)
{
	visitor->visitFunctionSQLs(*this);
}

void FunctionSQLs::load(ProgressIndicator* progressIndicator)
{
	DatabasePtr db = getDatabase();
	if (db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0))
	{
		wxString stmt = "select rdb$function_name from rdb$functions"
		" where (rdb$system_flag = 0 or rdb$system_flag is null)";
		stmt += " and RDB$LEGACY_FLAG = 0  and rdb$package_name is null ";
		stmt += " order by 1";
		setItems(db->loadIdentifiers(stmt, progressIndicator));
	}
}

void FunctionSQLs::loadChildren()
{
	load(0);
}

const wxString FunctionSQLs::getTypeName() const
{
	return "FUNCTIONSQL_COLLECTION";
}


void UDF::loadProperties()
{
	setPropertiesLoaded(false);
		wxString mechanismNames[] = { "value", "reference",
			"descriptor", "blob descriptor", "scalar array",
			"null", wxEmptyString };
		wxString mechanismDDL[] = { " BY VALUE ", wxEmptyString,
			" BY DESCRIPTOR ", wxEmptyString, " BY SCALAR ARRAY ",
			" NULL ", wxEmptyString };

		bool first = true;
		wxString retstr;
		definitionM = getName_() + "(" + wxTextBuffer::GetEOL();
		paramListM = wxEmptyString;

		DatabasePtr db = getDatabase();
		MetadataLoader* loader = db->getMetadataLoader();
		wxMBConv* converter = db->getCharsetConverter();
		MetadataLoaderTransaction tr(loader);

		std::string stmt = "SELECT f.RDB$RETURN_ARGUMENT, a.RDB$MECHANISM,"
			" a.RDB$ARGUMENT_POSITION, a.RDB$FIELD_TYPE, a.RDB$FIELD_SCALE,"
			" a.RDB$FIELD_LENGTH, a.RDB$FIELD_SUB_TYPE, a.RDB$FIELD_PRECISION,"
			" f.RDB$MODULE_NAME, f.RDB$ENTRYPOINT, c.RDB$CHARACTER_SET_NAME, ";
		if (db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0))
			stmt += " f.RDB$LEGACY_FLAG ";
		else
			stmt += "null ";

		stmt += " FROM RDB$FUNCTIONS f"
			" LEFT OUTER JOIN RDB$FUNCTION_ARGUMENTS a"
			" ON f.RDB$FUNCTION_NAME = a.RDB$FUNCTION_NAME"
			" LEFT OUTER JOIN RDB$CHARACTER_SETS c"
			" ON a.RDB$CHARACTER_SET_ID = c.RDB$CHARACTER_SET_ID"
			" WHERE f.RDB$FUNCTION_NAME = ? ";
		if (db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0))
			stmt += " and f.rdb$package_name is null";
		stmt +=	" ORDER BY a.RDB$ARGUMENT_POSITION";


		IBPP::Statement& st1 = loader->getStatement(stmt);
		st1->Set(1, wx2std(getName_(), converter));
		st1->Execute();
		while (st1->Fetch())
		{
			short returnarg, mechanism, type, scale, length, subtype, precision,
				retpos;
			std::string libraryName, entryPoint, charset;
			st1->Get(1, returnarg);
			st1->Get(2, mechanism);
			st1->Get(3, retpos);
			st1->Get(4, type);
			st1->Get(5, scale);
			st1->Get(6, length);
			st1->Get(7, subtype);
			st1->Get(8, precision);
			st1->Get(9, libraryName);
			libraryNameM = wxString(libraryName.c_str(), *converter).Strip();
			st1->Get(10, entryPoint);
			entryPointM = wxString(entryPoint.c_str(), *converter).Strip();
			wxString datatype = Domain::dataTypeToString(type, scale,
				precision, subtype, length);
			if (!st1->IsNull(11))
			{
				st1->Get(11, charset);
				wxString chset = wxString(charset.c_str(), *converter).Strip();
				if (db->getDatabaseCharset() != chset)
				{
					datatype += " " + SqlTokenizer::getKeyword(kwCHARACTER)
						+ " " + SqlTokenizer::getKeyword(kwSET)
						+ " " + chset;
				}
			}
			if (type == 261)    // avoid subtype information for BLOB
				datatype = SqlTokenizer::getKeyword(kwBLOB);


			int mechIndex = (mechanism < 0 ? -mechanism : mechanism);
			if (mechIndex >= (sizeof(mechanismNames)/sizeof(wxString)))
				mechIndex = (sizeof(mechanismNames)/sizeof(wxString)) - 1;
			wxString param = "    " + datatype + " "
				+ SqlTokenizer::getKeyword(kwBY) + " "
				+ mechanismNames[mechIndex];
			if (mechanism < 0)
				param += wxString(" ") + SqlTokenizer::getKeyword(kwFREE_IT);
			if (returnarg == retpos)    // output
			{
				retstr = param;
				retstrM = datatype + mechanismDDL[mechIndex];
				if (retpos != 0)
				{
					retstrM = SqlTokenizer::getKeyword(kwPARAMETER) + " ";
					retstrM << retpos;
					if (!paramListM.IsEmpty())
						paramListM += ", ";
					paramListM += datatype + mechanismDDL[mechIndex];
				}
				if (mechanism < 0){
				  retstrM += wxString(" ") + SqlTokenizer::getKeyword(kwFREE_IT);
				}
			}
			else
			{
				if (first)
					first = false;
				else
					definitionM += wxString(",") + wxTextBuffer::GetEOL();
				definitionM += param;
				if (!paramListM.empty())
					paramListM += ", ";
				paramListM += datatype + mechanismDDL[mechIndex];
			}
		}
		definitionM += wxString(wxTextBuffer::GetEOL()) + ")"
			+ wxTextBuffer::GetEOL() + SqlTokenizer::getKeyword(kwRETURNS)
			+ ":" + wxTextBuffer::GetEOL() + retstr;
	
	setPropertiesLoaded(true);

}

UDF::UDF(DatabasePtr database, const wxString& name)
	: Function(database, name)
{
    setType(ntUDF);
	ensurePropertiesLoaded();
}

const wxString UDF::getTypeName() const
{
	return "UDF";
}

wxString UDF::getDropSqlStatement() const
{
	StatementBuilder sb;
	sb << kwDROP << ' ' << kwEXTERNAL << ' ' << kwFUNCTION << ' '
		<< getQuotedName() << ';';
	return sb;
}


wxString UDF::getCreateSql()
{
	ensurePropertiesLoaded();
	StatementBuilder sb;
	    sb << kwDECLARE << ' ' << kwEXTERNAL << ' ' << kwFUNCTION << ' '
			<< getQuotedName() << StatementBuilder::NewLine
			<< paramListM  << StatementBuilder::NewLine
			<< kwRETURNS << ' ' << retstrM << StatementBuilder::NewLine
			<< kwENTRY_POINT << " '" << entryPointM << '\''
			<< StatementBuilder::NewLine
			<< kwMODULE_NAME << " '" << libraryNameM << "\';"
			<< StatementBuilder::NewLine;
	return sb;
}

wxString UDF::getDefinition()
{
	ensurePropertiesLoaded();
	return definitionM;
}

wxString UDF::getLibraryName()
{
	ensurePropertiesLoaded();
	return libraryNameM;
}

wxString UDF::getEntryPoint()
{
	ensurePropertiesLoaded();
	return entryPointM;
}

void UDF::acceptVisitor(MetadataItemVisitor* visitor)
{
	visitor->visitUDF(*this);
}

wxString UDF::getSource()
{
	ensurePropertiesLoaded();
	return definitionM;
}


// Functions UDF collection
UDFs::UDFs(DatabasePtr database)
	: MetadataCollection<UDF>(ntUDFs, database, _("UDFs"))
{
}

void UDFs::acceptVisitor(MetadataItemVisitor* visitor)
{
	visitor->visitUDFs(*this);
}

void UDFs::load(ProgressIndicator* progressIndicator)
{
	DatabasePtr db = getDatabase();
	wxString stmt = "select rdb$function_name from rdb$functions "
		" where (rdb$system_flag = 0 or rdb$system_flag is null) ";
	if (db->getInfo().getODSVersionIsHigherOrEqualTo(12, 0))
		stmt += " and RDB$LEGACY_FLAG = 1 and rdb$package_name is null ";
	stmt += " order by 1";
	setItems(db->loadIdentifiers(stmt, progressIndicator));
}

void UDFs::loadChildren()
{
	load(0);
}

const wxString UDFs::getTypeName() const
{
	return "UDF_COLLECTION";
}
