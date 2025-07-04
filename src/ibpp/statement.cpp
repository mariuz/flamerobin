//	Subject : IBPP, Service class implementation
/*
    (C) Copyright 2000-2006 T.I.P. Group S.A. and the IBPP Team (www.ibpp.org)

    The contents of this file are subject to the IBPP License (the "License");
    you may not use this file except in compliance with the License.  You may
    obtain a copy of the License at http://www.ibpp.org or in the 'license.txt'
    file which must have been distributed along with this file.

    This software, distributed under the License, is distributed on an "AS IS"
    basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the
    License for the specific language governing rights and limitations
    under the License.
*/

#ifdef _MSC_VER
#pragma warning(disable: 4786 4996)
#ifndef _DEBUG
#pragma warning(disable: 4702)
#endif
#endif

#include "_ibpp.h"

#include <cctype>
#include <functional>
#include <array>

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

#include <iostream>
#include <algorithm>

using namespace ibpp_internals;

//	(((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

void StatementImpl::Prepare(const std::string& sql)
{
	if (mDatabase == 0)
		throw LogicExceptionImpl("Statement::Prepare", _("An IDatabase must be attached."));
	if (mDatabase->GetHandle() == 0)
		throw LogicExceptionImpl("Statement::Prepare", _("IDatabase must be connected."));
	if (mTransaction == 0)
		throw LogicExceptionImpl("Statement::Prepare", _("An ITransaction must be attached."));
	if (mTransaction->GetHandle() == 0)
		throw LogicExceptionImpl("Statement::Prepare", _("ITransaction must be started."));
	if (sql.empty())
		throw LogicExceptionImpl("Statement::Prepare", _("SQL statement can't be 0."));

	// Saves the SQL sentence, only for reporting reasons in case of errors
	mSql = ParametersParser(sql);

	IBS status;

	// Free all resources currently attached to this Statement, then allocate
	// a new statement descriptor.
	Close();
	(*getGDS().Call()->m_dsql_allocate_statement)(status.Self(), mDatabase->GetHandlePtr(), &mHandle);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Statement::Prepare",
			_("isc_dsql_allocate_statement failed"));

	// Empirical estimate of parameters count and output columns count.
	// This is by far not an exact estimation, which would require parsing the
	// SQL statement. If the SQL statement contains '?' and ',' in string
	// constants, this count will obviously be wrong, but it will be exagerated.
	// It won't hurt. We just try to not have to re-allocate those descriptors later.
	// So we prefer to get them a little bit larger than needed than the other way.
	int16_t inEstimate = 0;
	int16_t outEstimate = 1;
	size_t len = strlen(mSql.c_str());
	for (size_t i = 0; i < len; i++)
	{
		if (mSql[i] == '?') ++inEstimate;
		if (mSql[i] == ',') ++outEstimate;
	}

	/*
	DebugStream()<< "Prepare(\""<< mSql<< "\")"<< fds;
	DebugStream()<< _("Estimation: ")<< inEstimate<< _(" IN parameters and ")
			<< outEstimate<< _(" OUT columns")<< fds;
	*/

	// Allocates output descriptor and prepares the statement
	mOutRow = new RowImpl(mDatabase->Dialect(), outEstimate, mDatabase, mTransaction);
	mOutRow->AddRef();

	status.Reset();
	(*getGDS().Call()->m_dsql_prepare)(status.Self(), mTransaction->GetHandlePtr(),
		&mHandle, 0, const_cast<char*>(mSql.c_str()),
			short(mDatabase->Dialect()), mOutRow->Self());
	if (status.Errors())
	{
		Close();
		std::string context = "Statement::Prepare( ";
		context.append(mSql).append(" )");
		throw SQLExceptionImpl(status, context.c_str(),
			_("isc_dsql_prepare failed"));
	}

	// Read what kind of statement was prepared
	status.Reset();
	char itemsReq[] = {isc_info_sql_stmt_type};
	char itemsRes[8];
	(*getGDS().Call()->m_dsql_sql_info)(status.Self(), &mHandle, 1, itemsReq,
		sizeof(itemsRes), itemsRes);
	if (status.Errors())
	{
		Close();
		throw SQLExceptionImpl(status, "Statement::Prepare",
			_("isc_dsql_sql_info failed"));
	}
	if (itemsRes[0] == isc_info_sql_stmt_type)
	{
		switch (itemsRes[3])
		{
			case isc_info_sql_stmt_select :		mType = IBPP::stSelect; break;
			case isc_info_sql_stmt_insert :		mType = IBPP::stInsert; break;
			case isc_info_sql_stmt_update :		mType = IBPP::stUpdate; break;
			case isc_info_sql_stmt_delete :		mType = IBPP::stDelete; break;
			case isc_info_sql_stmt_ddl :		mType = IBPP::stDDL; break;
			case isc_info_sql_stmt_exec_procedure :	mType = IBPP::stExecProcedure; break;
			case isc_info_sql_stmt_start_trans:	mType = IBPP::stStartTransaction; break;
			case isc_info_sql_stmt_commit:		mType = IBPP::stCommitTransaction; break;
			case isc_info_sql_stmt_rollback:	mType = IBPP::stRollbackTransaction; break;
			case isc_info_sql_stmt_select_for_upd : mType = IBPP::stSelectUpdate; break;
			case isc_info_sql_stmt_set_generator :	mType = IBPP::stSetGenerator; break;
			case isc_info_sql_stmt_savepoint :	mType = IBPP::stSavePoint; break;
			default : mType = IBPP::stUnsupported;
		}
	}
	if (mType == IBPP::stUnknown || mType == IBPP::stUnsupported)
	{
		Close();
		throw LogicExceptionImpl("Statement::Prepare",
			_("Unknown or unsupported statement type"));
	}

	if (mOutRow->Columns() == 0)
	{
		// Get rid of the output descriptor, if it wasn't required (no output)
		mOutRow->Release();
		mOutRow = 0;
		/*
		DebugStream()<< _("Dropped output descriptor which was not required")<< fds;
		*/
	}
	else if (mOutRow->Columns() > mOutRow->AllocatedSize())
	{
		// Resize the output descriptor (which is too small).
		// The statement does not need to be prepared again, though the
		// output columns must be described again.

		/*
		DebugStream()<< _("Resize output descriptor from ")
			<< mOutRow->AllocatedSize()<< _(" to ")<< mOutRow->Columns()<< fds;
		*/

		mOutRow->Resize(mOutRow->Columns());
		status.Reset();
		(*getGDS().Call()->m_dsql_describe)(status.Self(), &mHandle, 1, mOutRow->Self());
		if (status.Errors())
		{
			Close();
			throw SQLExceptionImpl(status, "Statement::Prepare",
				_("isc_dsql_describe failed"));
		}
	}

	if (inEstimate > 0)
	{
		// Ready an input descriptor
		mInRow = new RowImpl(mDatabase->Dialect(), inEstimate, mDatabase, mTransaction);
		mInRow->AddRef();

		status.Reset();
		(*getGDS().Call()->m_dsql_describe_bind)(status.Self(), &mHandle, 1, mInRow->Self());
		if (status.Errors())
		{
			Close();
			throw SQLExceptionImpl(status, "Statement::Prepare",
				_("isc_dsql_describe_bind failed"));
		}

		if (mInRow->Columns() == 0)
		{
			// Get rid of the input descriptor, if it wasn't required (no parameters)
			mInRow->Release();
			mInRow = 0;
			/*
			DebugStream()<< _("Dropped input descriptor which was not required")<< fds;
			*/
		}
		else if (mInRow->Columns() > mInRow->AllocatedSize())
		{
			// Resize the input descriptor (which is too small).
			// The statement does not need to be prepared again, though the
			// parameters must be described again.

			/*
			DebugStream()<< _("Resize input descriptor from ")
					<< mInRow->AllocatedSize()<< _(" to ")
					<< mInRow->Columns()<< fds;
			*/

			mInRow->Resize(mInRow->Columns());
			status.Reset();
			(*getGDS().Call()->m_dsql_describe_bind)(status.Self(), &mHandle, 1, mInRow->Self());
			if (status.Errors())
			{
				Close();
				throw SQLExceptionImpl(status, "Statement::Prepare",
					_("isc_dsql_describe_bind failed"));
			}
		}
	}

	// Allocates variables of the input descriptor
	if (mInRow != 0)
	{
		// Turn on 'can be NULL' on each input parameter
		for (int i = 0; i < mInRow->Columns(); i++)
		{
			XSQLVAR* var = &(mInRow->Self()->sqlvar[i]);
			if (! (var->sqltype & 1)) var->sqltype += short(1);
		}
		mInRow->AllocVariables();
	}

	// Allocates variables of the output descriptor
	if (mOutRow != 0) mOutRow->AllocVariables();
}

void StatementImpl::Plan(std::string& plan)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Plan", _("No statement has been prepared."));
	if (mDatabase == 0)
		throw LogicExceptionImpl("Statement::Plan", _("A Database must be attached."));
	if (mDatabase->GetHandle() == 0)
		throw LogicExceptionImpl("Statement::Plan", _("Database must be connected."));

	IBS status;
	RB result(65535);
	char itemsReq[] = {isc_info_sql_get_plan};

	(*getGDS().Call()->m_dsql_sql_info)(status.Self(), &mHandle, 1, itemsReq,
								   result.Size(), result.Self());
	if (status.Errors()) throw SQLExceptionImpl(status,
								"Statement::Plan", _("isc_dsql_sql_info failed."));

	result.GetString(isc_info_sql_get_plan, plan);
	if (plan[0] == '\n') plan.erase(0, 1);
}

void StatementImpl::Execute(const std::string& sql)
{
	if (! sql.empty()) Prepare(sql);

	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Execute",
			_("No statement has been prepared."));

	// Check that a value has been set for each input parameter
	if (mInRow != 0 && mInRow->MissingValues())
		throw LogicExceptionImpl("Statement::Execute",
			_("All parameters must be specified."));

	CursorFree();	// Free a previous 'cursor' if any

	IBS status;
	if (mType == IBPP::stSelect)
	{
		// Could return a result set (none, single or multi rows)
		(*getGDS().Call()->m_dsql_execute)(status.Self(), mTransaction->GetHandlePtr(),
			&mHandle, 1, mInRow == 0 ? 0 : mInRow->Self());
		if (status.Errors())
		{
			//Close();	Commented because Execute error should not free the statement
			std::string context = "Statement::Execute( ";
			context.append(mSql).append(" )");
			throw SQLExceptionImpl(status, context.c_str(),
				_("isc_dsql_execute failed"));
		}
		if (mOutRow != 0) mResultSetAvailable = true;
	}
	else
	{
		// Should return at most a single row
		(*getGDS().Call()->m_dsql_execute2)(status.Self(), mTransaction->GetHandlePtr(),
			&mHandle, 1, mInRow == 0 ? 0 : mInRow->Self(),
			mOutRow == 0 ? 0 : mOutRow->Self());
		if (status.Errors())
		{
			//Close();	Commented because Execute error should not free the statement
			std::string context = "Statement::Execute( ";
			context.append(mSql).append(" )");
			throw SQLExceptionImpl(status, context.c_str(),
				_("isc_dsql_execute2 failed"));
		}
	}
}

void StatementImpl::CursorExecute(const std::string& cursor, const std::string& sql)
{
	if (cursor.empty())
		throw LogicExceptionImpl("Statement::CursorExecute", _("Cursor name can't be 0."));

	if (! sql.empty()) Prepare(sql);

	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::CursorExecute", _("No statement has been prepared."));
	if (mType != IBPP::stSelectUpdate)
		throw LogicExceptionImpl("Statement::CursorExecute", _("Statement must be a SELECT FOR UPDATE."));
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::CursorExecute", _("Statement would return no rows."));

	// Check that a value has been set for each input parameter
	if (mInRow != 0 && mInRow->MissingValues())
		throw LogicExceptionImpl("Statement::CursorExecute",
			_("All parameters must be specified."));

	CursorFree();	// Free a previous 'cursor' if any

	IBS status;
	(*getGDS().Call()->m_dsql_execute)(status.Self(), mTransaction->GetHandlePtr(),
		&mHandle, 1, mInRow == 0 ? 0 : mInRow->Self());
	if (status.Errors())
	{
		//Close();	Commented because Execute error should not free the statement
		std::string context = "Statement::CursorExecute( ";
		context.append(mSql).append(" )");
		throw SQLExceptionImpl(status, context.c_str(),
			_("isc_dsql_execute failed"));
	}

	status.Reset();
	(*getGDS().Call()->m_dsql_set_cursor_name)(status.Self(), &mHandle, const_cast<char*>(cursor.c_str()), 0);
	if (status.Errors())
	{
		//Close();	Commented because Execute error should not free the statement
		throw SQLExceptionImpl(status, "Statement::CursorExecute",
			_("isc_dsql_set_cursor_name failed"));
	}

	mResultSetAvailable = true;
	mCursorOpened = true;
}

void StatementImpl::ExecuteImmediate(const std::string& sql)
{
	if (mDatabase == 0)
		throw LogicExceptionImpl("Statement::ExecuteImmediate", _("An IDatabase must be attached."));
	if (mDatabase->GetHandle() == 0)
		throw LogicExceptionImpl("Statement::ExecuteImmediate", _("IDatabase must be connected."));
	if (mTransaction == 0)
		throw LogicExceptionImpl("Statement::ExecuteImmediate", _("An ITransaction must be attached."));
	if (mTransaction->GetHandle() == 0)
		throw LogicExceptionImpl("Statement::ExecuteImmediate", _("ITransaction must be started."));
	if (sql.empty())
		throw LogicExceptionImpl("Statement::ExecuteImmediate", _("SQL statement can't be 0."));

	IBS status;
	Close();
    (*getGDS().Call()->m_dsql_execute_immediate)(status.Self(), mDatabase->GetHandlePtr(),
    	mTransaction->GetHandlePtr(), 0, const_cast<char*>(sql.c_str()),
    		short(mDatabase->Dialect()), 0);
    if (status.Errors())
	{
		std::string context = "Statement::ExecuteImmediate( ";
		context.append(sql).append(" )");
		throw SQLExceptionImpl(status, context.c_str(),
			_("isc_dsql_execute_immediate failed"));
	}
}

int StatementImpl::AffectedRows()
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::AffectedRows", _("No statement has been prepared."));
	if (mDatabase == 0)
		throw LogicExceptionImpl("Statement::AffectedRows", _("A Database must be attached."));
	if (mDatabase->GetHandle() == 0)
		throw LogicExceptionImpl("Statement::AffectedRows", _("Database must be connected."));

	int count(0);
	IBS status;
	RB result;
	char itemsReq[] = {isc_info_sql_records};

	(*getGDS().Call()->m_dsql_sql_info)(status.Self(), &mHandle, 1, itemsReq,
		result.Size(), result.Self());
	if (status.Errors()) throw SQLExceptionImpl(status,
			"Statement::AffectedRows", _("isc_dsql_sql_info failed."));

	// Cover the INSERT or UPDATE case
	if (mType == IBPP::stInsert || mType == IBPP::stUpdate)
	{
		count += result.GetValue(isc_info_sql_records, isc_info_req_insert_count);
		count += result.GetValue(isc_info_sql_records, isc_info_req_update_count);
	}
	else if (mType == IBPP::stDelete)
			count = result.GetValue(isc_info_sql_records, isc_info_req_delete_count);
	else if (mType == IBPP::stSelect)
			count = result.GetValue(isc_info_sql_records, isc_info_req_select_count);
	else	count = 0;	// Returns zero count for unknown cases

	return count;
}

bool StatementImpl::Fetch()
{
	if (! mResultSetAvailable)
		throw LogicExceptionImpl("Statement::Fetch",
			_("No statement has been executed or no result set available."));

	IBS status;
	ISC_STATUS code = (*getGDS().Call()->m_dsql_fetch)(status.Self(), &mHandle, 1, mOutRow->Self());
	if (code == 100)	// This special code means "no more rows"
	{
		mResultSetAvailable = false;
		// Oddly enough, fetching rows up to the last one seems to open
		// an 'implicit' cursor that needs to be closed.
		mCursorOpened = true;
		CursorFree();	// Free the explicit or implicit cursor/result-set
		return false;
	}
	if (status.Errors())
	{
		Close();
		throw SQLExceptionImpl(status, "Statement::Fetch",
			_("isc_dsql_fetch failed."));
	}

    // Close the 'implicit' cursor to allow for forther Execute() calls
    // on the prepared statement without fetching up to the last row
	mCursorOpened = true;
	return true;
}

bool StatementImpl::Fetch(IBPP::Row& row)
{
	if (! mResultSetAvailable)
		throw LogicExceptionImpl("Statement::Fetch(row)",
			_("No statement has been executed or no result set available."));

	RowImpl* rowimpl = new RowImpl(*mOutRow);
	row = rowimpl;

	IBS status;
	ISC_STATUS code = (*getGDS().Call()->m_dsql_fetch)(status.Self(), &mHandle, 1,
					rowimpl->Self());
	if (code == 100)	// This special code means "no more rows"
	{
		mResultSetAvailable = false;
		// Oddly enough, fetching rows up to the last one seems to open
		// an 'implicit' cursor that needs to be closed.
		mCursorOpened = true;
		CursorFree();	// Free the explicit or implicit cursor/result-set
		row.clear();
		return false;
	}
	if (status.Errors())
	{
		Close();
		row.clear();
		throw SQLExceptionImpl(status, "Statement::Fetch(row)",
			_("isc_dsql_fetch failed."));
	}

	return true;
}

void StatementImpl::Close()
{
	// Free all statement resources.
	// Used before preparing a new statement or from destructor.

	if (mInRow != 0) { mInRow->Release(); mInRow = 0; }
	if (mOutRow != 0) { mOutRow->Release(); mOutRow = 0; }

	mResultSetAvailable = false;
	mCursorOpened = false;
	mType = IBPP::stUnknown;

	if (mHandle != 0)
	{
		IBS status;
		(*getGDS().Call()->m_dsql_free_statement)(status.Self(), &mHandle, DSQL_drop);
		mHandle = 0;
		if (status.Errors())
			throw SQLExceptionImpl(status, "Statement::Close(DSQL_drop)",
				_("isc_dsql_free_statement failed."));
	}
}

void StatementImpl::SetNull(int param)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::SetNull", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::SetNull", _("The statement does not take parameters."));

	mInRow->SetNull(param);
}

void StatementImpl::Set(int param, bool value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[bool]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[bool]", _("The statement does not take parameters."));

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, const char* cstring)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[char*]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[char*]", _("The statement does not take parameters."));

	mInRow->Set(param, cstring);
}

void StatementImpl::Set(int param, const void* bindata, int len)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[void*]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[void*]", _("The statement does not take parameters."));

	mInRow->Set(param, bindata, len);
}

void StatementImpl::Set(int param, const std::string& s)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[string]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[string]", _("The statement does not take parameters."));

	mInRow->Set(param, s);
}

void StatementImpl::Set(int param, int16_t value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[int16_t]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[int16_t]", _("The statement does not take parameters."));

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, int32_t value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[int32_t]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[int32_t]", _("The statement does not take parameters."));

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, int64_t value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[int64_t]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[int64_t]", _("The statement does not take parameters."));

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, float value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[float]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[float]", _("The statement does not take parameters."));

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, double value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[double]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[double]", _("The statement does not take parameters."));

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, const IBPP::Timestamp& value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[Timestamp]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[Timestamp]", _("The statement does not take parameters."));

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, const IBPP::Date& value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[Date]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[Date]", _("The statement does not take parameters."));

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, const IBPP::Time& value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[Time]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[Time]", _("The statement does not take parameters."));

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, const IBPP::Blob& blob)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[Blob]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[Blob]", _("The statement does not take parameters."));

	mInRow->Set(param, blob);
}

void StatementImpl::Set(int param, const IBPP::Array& array)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[Array]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[Array]", _("The statement does not take parameters."));

	mInRow->Set(param, array);
}

void StatementImpl::Set(int param, const IBPP::DBKey& key)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[DBKey]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[DBKey]", _("The statement does not take parameters."));

	mInRow->Set(param, key);
}

/*
void StatementImpl::Set(int param, const IBPP::Value& value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[Value]", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[Value]", _("The statement does not take parameters."));

	mInRow->Set(param, value);
}
*/

int StatementImpl::ParameterNum(const std::string& name) { //Dubious use, because we can have 2 :parameters with the same name!
                                                        //Maybe, remove this method?
    if (name.empty())
        throw LogicExceptionImpl("Statement::ColumnNum", _("Parameter name <empty> not found."));
    std::vector<int> params = this->FindParamsByName(name);
    if (params.size()>0)
        return params.at(0);  //Already returns ID+1
    throw LogicExceptionImpl("Statement::ColumnNum", _("Could not find matching parameter."));
}

std::vector<int> StatementImpl::FindParamsByName(std::string name) {
    if (name.empty())
		throw LogicExceptionImpl("Statement::FindParamsByName", _("Parameter name <empty> not found."));

    std::vector<int> params;
    unsigned int i;
    for(i=0;i < parametersDetailedByName_.size() ; i++) {
        if (parametersDetailedByName_.at(i) == name)
            params.push_back(i+1);
    }
    return params;
}

std::vector<std::string> StatementImpl::ParametersByName() {
  std::vector<std::string> vt;
  vt.reserve(parametersByName_.size());
  for(unsigned int i=0;i < parametersByName_.size() ; i++) {
      vt.push_back(parametersByName_.at(i));
  }
  return vt;
}

std::string StatementImpl::ParametersParser(std::string sql)
{


    unsigned int i;
    bool isDMLcheck = false;
    
    parametersByName_.clear();
    parametersDetailedByName_.clear();

    //Improve this part, verify the SQL, if is really necessary to process the parameters this time
    //Can be done in the begin

    //Here we verify, the job done recently was time lost?
    std::string isDML(sql);

    isDML.erase(isDML.begin(), std::find_if(isDML.begin(), isDML.end(), [](int c) { return !std::isspace(c); })); //lTrim

    std::transform(isDML.begin(), isDML.end(), isDML.begin(), [](char c) { return (char)std::toupper(c); }); //UpperCase (only bothered about ASCII text, cast is okay)

    std::string isDML4 = isDML.substr(0, 4);  std::string isDML6 = isDML.substr(0, 6);  std::string isDML7 = isDML.substr(0, 7);

    std::array<std::string, 6> dml =
    {
        "INSERT",
        "UPDATE",
        "DELETE",
        "SELECT",
        "EXECUTE",
        "WITH"
    };
    std::array<std::string, 6> check =
    {
        isDML6,
        isDML6,
        isDML6,
        isDML6,
        isDML7,
        isDML4
    };
    for (i = 0; i<dml.size(); i++) {
        if (check.at(i) == dml.at(i))
        {
            if (dml.at(i) == "EXECUTE")//is execute procedure or execute block? else, is DML for sure
            {
                auto x = dml.at(i).size();
                while (std::isspace(isDML.at(x)) && x<isDML.size())
                    x++;
                char p = isDML.at(x);
                //std::cout << "Char:"<<p << std::endl;
                if (p != 'P')//Procedure: execute procedure $sp(:params), OK to replace, else, use original, break loop
                    break;     //I don't want to replace :parameters inside an execute block..
                               //TODO:
                               //It is possible to insert parameters in the begin of the block, example:
                               //execute block (x double precision = ?, y double precision = ?)
                               //But it will need more work to do it
                               //Source: http://www.firebirdsql.org/refdocs/langrefupd20-execblock.html

            }            
            isDMLcheck = true;
            break;
        }

    }
    if (!isDMLcheck) {
        //Probably is DDL... don't replace parameters then

        //std::cout << sql << std::endl;
        return sql;
    }






    //ctor
    bool comment = false, blockComment = false, palavra = false, quote = false, doubleQuote = false;
    std::ostringstream temp, sProcessedSQL;
    std::string debugProcessedSQL="";

    for(i= 0; i < sql.length(); i++){
        char nextChar=0;
        char previousChar=0;
        if (i < sql.length()-1)
            nextChar=sql.at(i+1);
        if (i > 0)
            previousChar=sql.at(i-1);

        if ((sql.at(i)=='\n') || (sql.at(i)=='\r')) {
            comment = false;
            //sProcessedSQL << sql.at(i);
        }else if (sql.at(i)=='-' && nextChar=='-' && !quote && !doubleQuote  && !blockComment) {
            comment = true;
        }else if (sql.at(i)=='/' && nextChar=='*' && !quote && !doubleQuote  && !blockComment) {
            blockComment = true;
        }else if (previousChar=='*' && sql.at(i)=='/' && blockComment) {
            blockComment = false;
            continue;
        }else if (sql.at(i)=='\'' && !doubleQuote && !quote && !comment && !blockComment) {
            quote = true;
            sProcessedSQL << sql.at(i);
        }else if (sql.at(i)=='\'' && quote && !comment && !blockComment) {
            quote = false;
            //sProcessedSQL << sql.at(i);
        }else if (sql.at(i)=='\"' && !quote && !doubleQuote && !comment && !blockComment) {
            doubleQuote = true;
            sProcessedSQL << sql.at(i);
        }else if (sql.at(i)=='\"' && doubleQuote) {
            doubleQuote = false;

        } else if (quote || doubleQuote)
        {
            sProcessedSQL << sql.at(i);
        }
        if (!(comment || blockComment || quote || doubleQuote ) || palavra || sql.at(i)=='\n' || sql.at(i)=='\r')
        {
            comment = false;  //New line?
            if (sql.at(i)=='?'){

                if (FindParamsByName("?").size() == 0)//if first time:
                    parametersByName_.push_back("?");
                parametersDetailedByName_.push_back("?");
            }
            if (sql.at(i)==':'){
                palavra = true;
                //comeco = i;
            }else if (palavra) {
                if (std::isalnum(sql.at(i)) || sql.at(i)=='_' || sql.at(i)=='$')
                { //A-Z 0-9 _
                    temp << (char)std::toupper(sql.at(i));
                    if (i==sql.length()-1)  //se o I esta no fim do SQL...
                        goto fimPalavra;
                }else{
                    fimPalavra :
                    palavra = false;

                    //cout << "sqletro encontrado: \""<< temp.str()<<"\""<<endl;
                    sProcessedSQL << "?"<<"/*:"<<temp.str()<<"*/";
                    if (!(std::isalnum(sql.at(i)) || sql.at(i)=='_' || sql.at(i)=='$'))
                        sProcessedSQL << sql.at(i);
                    std::vector<int> tmp = FindParamsByName(temp.str());
                    if (tmp.size() == 0)//If first time
                        parametersByName_.push_back(temp.str());
                    parametersDetailedByName_.push_back(temp.str());
                    temp.str(std::string());temp.clear(); //Limpar StringStream
                }
            }
            else
            {
                sProcessedSQL << sql.at(i);
            }
        }
        debugProcessedSQL = sProcessedSQL.str();
    }
  //std::cout << "sProcessedSQL: " << sProcessedSQL.str() << std::endl;

  return sProcessedSQL.str();
}

void StatementImpl::SetNull(std::string param) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        SetNull(*it);
    }
}
void StatementImpl::Set(std::string param, int64_t value){
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}

void StatementImpl::Set(std::string param, int32_t value){
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}

void StatementImpl::Set(std::string param, int16_t value){
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}

void StatementImpl::Set(std::string param, float value){
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}

void StatementImpl::Set(std::string param, double value){
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}

void StatementImpl::Set(std::string param, const void* bindata, int len) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, bindata, len);
    }
}
void StatementImpl::Set(std::string param, const std::string& s){
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, s);
    }
}
void StatementImpl::Set(std::string param, const char* cstring){
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, cstring);
    }
}
void StatementImpl::Set(std::string param, const IBPP::Timestamp& value) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}
void StatementImpl::Set(std::string param, const IBPP::Time& value) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}
void StatementImpl::Set(std::string param, const IBPP::Date& value) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}
void StatementImpl::Set(std::string param, const IBPP::DBKey& key) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, key);
    }
}
void StatementImpl::Set(std::string param, const IBPP::Blob& blob) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, blob);
    }
}
void StatementImpl::Set(std::string param, const IBPP::Array& array) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, array);
    }
}
void StatementImpl::Set(std::string param, bool value) {
    std::vector<int> params = FindParamsByName(param);
    if (params.size()==0){
        throw LogicExceptionImpl("Statement::Set[Value]", _("Parameter does not exists."));
    }
    for (std::vector<int>::iterator it = params.begin() ; it != params.end(); ++it) {
        Set(*it, value);
    }
}



bool StatementImpl::IsNull(int column)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::IsNull", _("The row is not initialized."));

	return mOutRow->IsNull(column);
}

bool StatementImpl::Get(int column, bool* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

	return mOutRow->Get(column, *retvalue);
}

bool StatementImpl::Get(int column, bool& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, char* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, void* bindata, int& userlen)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, bindata, userlen);
}

bool StatementImpl::Get(int column, std::string& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, int16_t* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

	return mOutRow->Get(column, *retvalue);
}

bool StatementImpl::Get(int column, int16_t& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, int32_t* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

	return mOutRow->Get(column, *retvalue);
}

bool StatementImpl::Get(int column, int32_t& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, int64_t* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

	return mOutRow->Get(column, *retvalue);
}

bool StatementImpl::Get(int column, int64_t& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, IBPP::ibpp_int128_t& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, float* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

	return mOutRow->Get(column, *retvalue);
}

bool StatementImpl::Get(int column, float& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, double* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

	return mOutRow->Get(column, *retvalue);
}

bool StatementImpl::Get(int column, double& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, IBPP::ibpp_dec16_t& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, IBPP::ibpp_dec34_t& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, retvalue);
}


bool StatementImpl::Get(int column, IBPP::Timestamp& timestamp)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, timestamp);
}

bool StatementImpl::Get(int column, IBPP::Date& date)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, date);
}

bool StatementImpl::Get(int column, IBPP::Time& time)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, time);
}

bool StatementImpl::Get(int column, IBPP::Blob& blob)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, blob);
}

bool StatementImpl::Get(int column, IBPP::DBKey& key)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, key);
}

bool StatementImpl::Get(int column, IBPP::Array& array)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column, array);
}

/*
const IBPP::Value StatementImpl::Get(int column)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(column);
}
*/

bool StatementImpl::IsNull(const std::string& name)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::IsNull", _("The row is not initialized."));

	return mOutRow->IsNull(name);
}

bool StatementImpl::Get(const std::string& name, bool* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

	return mOutRow->Get(name, *retvalue);
}

bool StatementImpl::Get(const std::string& name, bool& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, char* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get[char*]", _("The row is not initialized."));

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, void* retvalue, int& count)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get[void*,int]", _("The row is not initialized."));

	return mOutRow->Get(name, retvalue, count);
}

bool StatementImpl::Get(const std::string& name, std::string& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::GetString", _("The row is not initialized."));

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, int16_t* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

	return mOutRow->Get(name, *retvalue);
}

bool StatementImpl::Get(const std::string& name, int16_t& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, int32_t* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

	return mOutRow->Get(name, *retvalue);
}

bool StatementImpl::Get(const std::string& name, int32_t& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, int64_t* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

	return mOutRow->Get(name, *retvalue);
}

bool StatementImpl::Get(const std::string& name, int64_t& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, float* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

	return mOutRow->Get(name, *retvalue);
}

bool StatementImpl::Get(const std::string& name, float& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, double* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", _("Null pointer detected"));

	return mOutRow->Get(name, *retvalue);
}

bool StatementImpl::Get(const std::string& name, double& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, IBPP::Timestamp& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, IBPP::Date& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, IBPP::Time& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string&name, IBPP::Blob& retblob)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(name, retblob);
}

bool StatementImpl::Get(const std::string& name, IBPP::DBKey& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, IBPP::Array& retarray)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(name, retarray);
}

/*
const IBPP::Value StatementImpl::Get(const std::string& name)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", _("The row is not initialized."));

	return mOutRow->Get(name);
}
*/

int StatementImpl::Columns()
{
	if (mOutRow == 0)
		return 0;

	return mOutRow->Columns();
}

int StatementImpl::ColumnNum(const std::string& name)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::ColumnNum", _("The row is not initialized."));

	return mOutRow->ColumnNum(name);
}

const char* StatementImpl::ColumnName(int varnum)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Columns", _("The row is not initialized."));

	return mOutRow->ColumnName(varnum);
}

const char* StatementImpl::ColumnAlias(int varnum)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Columns", _("The row is not initialized."));

	return mOutRow->ColumnAlias(varnum);
}

const char* StatementImpl::ColumnTable(int varnum)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Columns", _("The row is not initialized."));

	return mOutRow->ColumnTable(varnum);
}

int StatementImpl::ColumnSQLType(int varnum)
{
    if (mHandle == 0)
        throw LogicExceptionImpl("Statement::ColumnType", _("No statement has been prepared."));
    if (mOutRow == 0)
        throw LogicExceptionImpl("Statement::ColumnType", _("The statement does not return results."));

    return mOutRow->ColumnSQLType(varnum);
}

IBPP::SDT StatementImpl::ColumnType(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ColumnType", _("No statement has been prepared."));
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::ColumnType", _("The statement does not return results."));

    return mOutRow->ColumnType(varnum);
}

int StatementImpl::ColumnSubtype(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ColumnSubtype", _("No statement has been prepared."));
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::ColumnSubtype", _("The statement does not return results."));

    return mOutRow->ColumnSubtype(varnum);
}

int StatementImpl::ColumnSize(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ColumnSize", _("No statement has been prepared."));
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::ColumnSize", _("The row is not initialized."));

	return mOutRow->ColumnSize(varnum);
}

int StatementImpl::ColumnScale(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ColumnScale", _("No statement has been prepared."));
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::ColumnScale", _("The row is not initialized."));

	return mOutRow->ColumnScale(varnum);
}

int StatementImpl::Parameters()
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Parameters", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Parameters", _("The statement uses no parameters."));

	return mInRow->Columns();
}

IBPP::SDT StatementImpl::ParameterType(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ParameterType", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::ParameterType", _("The statement uses no parameters."));

    return mInRow->ColumnType(varnum);
}

int StatementImpl::ParameterSubtype(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ParameterSubtype", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::ParameterSubtype", _("The statement uses no parameters."));

    return mInRow->ColumnSubtype(varnum);
}

int StatementImpl::ParameterSize(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ParameterSize", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::ParameterSize", _("The statement uses no parameters."));

	return mInRow->ColumnSize(varnum);
}

int StatementImpl::ParameterScale(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ParameterScale", _("No statement has been prepared."));
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::ParameterScale", _("The statement uses no parameters."));

	return mInRow->ColumnScale(varnum);
}

IBPP::Database StatementImpl::DatabasePtr() const
{
	return mDatabase;
}

IBPP::Transaction StatementImpl::TransactionPtr() const
{
	return mTransaction;
}

IBPP::IStatement* StatementImpl::AddRef()
{
	ASSERTION(mRefCount >= 0);
	++mRefCount;

	return this;
}

void StatementImpl::Release()
{
	// Release cannot throw, except in DEBUG builds on assertion
	ASSERTION(mRefCount >= 0);
	--mRefCount;
	try { if (mRefCount <= 0) delete this; }
		catch (...) { }
}

//	(((((((( OBJECT INTERNAL METHODS ))))))))

void StatementImpl::AttachDatabaseImpl(DatabaseImpl* database)
{
	if (database == 0)
		throw LogicExceptionImpl("Statement::AttachDatabase",
			_("Can't attach a 0 IDatabase object."));

	if (mDatabase != 0) mDatabase->DetachStatementImpl(this);
	mDatabase = database;
	mDatabase->AttachStatementImpl(this);
}

void StatementImpl::DetachDatabaseImpl()
{
	if (mDatabase == 0) return;

	Close();
	mDatabase->DetachStatementImpl(this);
	mDatabase = 0;
}

void StatementImpl::AttachTransactionImpl(TransactionImpl* transaction)
{
	if (transaction == 0)
		throw LogicExceptionImpl("Statement::AttachTransaction",
			_("Can't attach a 0 ITransaction object."));

	if (mTransaction != 0) mTransaction->DetachStatementImpl(this);
	mTransaction = transaction;
	mTransaction->AttachStatementImpl(this);
}

void StatementImpl::DetachTransactionImpl()
{
	if (mTransaction == 0) return;

	Close();
	mTransaction->DetachStatementImpl(this);
	mTransaction = 0;
}

void StatementImpl::CursorFree()
{
	if (mCursorOpened)
	{
		mCursorOpened = false;
		if (mHandle != 0)
		{
			IBS status;
			(*getGDS().Call()->m_dsql_free_statement)(status.Self(), &mHandle, DSQL_close);
			if (status.Errors())
				throw SQLExceptionImpl(status, "StatementImpl::CursorFree(DSQL_close)",
					_("isc_dsql_free_statement failed."));
		}
	}
}

StatementImpl::StatementImpl(DatabaseImpl* database, TransactionImpl* transaction)
	: mRefCount(0), mHandle(0), mDatabase(0), mTransaction(0),
	mInRow(0), mOutRow(0),
	mResultSetAvailable(false), mCursorOpened(false), mType(IBPP::stUnknown)
{
	AttachDatabaseImpl(database);
	if (transaction != 0) AttachTransactionImpl(transaction);
}

StatementImpl::~StatementImpl()
{
	try { Close(); }
		catch (...) { }
	try { if (mTransaction != 0) mTransaction->DetachStatementImpl(this); }
		catch (...) { }
	try { if (mDatabase != 0) mDatabase->DetachStatementImpl(this); }
		catch (...) { }
}

