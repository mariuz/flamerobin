///////////////////////////////////////////////////////////////////////////////
//
//	File    : $Id$
//	Subject : IBPP, Service class implementation
//
///////////////////////////////////////////////////////////////////////////////
//
//	The contents of this file are subject to the Mozilla Public License
//	Version 1.0 (the "License"); you may not use this file except in
//	compliance with the License. You may obtain a copy of the License at
//	http://www.mozilla.org/MPL/
//
//	Software distributed under the License is distributed on an "AS IS"
//	basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
//	License for the specific language governing rights and limitations
//	under the License.
//
//	The Original Code is "IBPP 0.9" and all its associated documentation.
//
//	The Initial Developer of the Original Code is T.I.P. Group S.A.
//	Portions created by T.I.P. Group S.A. are
//	Copyright (C) 2000 T.I.P Group S.A.
//	All Rights Reserved.
//
//	Contributor(s): ______________________________________.
//
///////////////////////////////////////////////////////////////////////////////
//
//	COMMENTS
//	* Tabulations should be set every four characters when editing this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "ibpp.h"
#include "_internals.h"

#ifdef HAS_HDRSTOP
#pragma hdrstop
#endif

using namespace ibpp_internals;

//	(((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

IBPP::IDatabase* StatementImpl::Database(void) const
{
	return mDatabase;
}

IBPP::ITransaction* StatementImpl::Transaction(void) const
{
	return mTransaction;
}

void StatementImpl::Prepare(const std::string& sql)
{
	if (mDatabase == 0)
		throw LogicExceptionImpl("Statement::Prepare", "An IDatabase must be attached.");
	if (mDatabase->GetHandle() == 0)
		throw LogicExceptionImpl("Statement::Prepare", "IDatabase must be connected.");
	if (mTransaction == 0)
		throw LogicExceptionImpl("Statement::Prepare", "An ITransaction must be attached.");
	if (mTransaction->GetHandle() == 0)
		throw LogicExceptionImpl("Statement::Prepare", "ITransaction must be started.");
	if (sql.empty())
		throw LogicExceptionImpl("Statement::Prepare", "SQL statement can't be 0.");

	// Saves the SQL sentence, only for reporting reasons in case of errors
	mSql = sql;
	
	IBS status;

	// Free all resources currently attached to this Statement, then allocate
	// a new statement descriptor.
	try {Close();} catch(IBPP::Exception&) {}
	(*gds.Call()->m_dsql_allocate_statement)(status.Self(), mDatabase->GetHandlePtr(), &mHandle);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Statement::Prepare",
			"isc_dsql_allocate_statement failed");

	// Empirical estimate of parameters count and output columns count.
	// This is by far not an exact estimation, which would require parsing the
	// SQL statement. If the SQL statement contains '?' and ',' in string
	// constants, this count will obviously be wrong, but it will exagerated.
	// It won't hurt. We just try to not have to re-allocate those descriptors later.
	// So we prefer to get them a little bit larger than needed than the other way.
	short inEstimate = 0;
	short outEstimate = 1;
	for (int i = 0; i < (int)strlen(sql.c_str()); i++)
	{
		if (sql[i] == '?') ++inEstimate;
		if (sql[i] == ',') ++outEstimate;
	}

	DebugStream()<< "Prepare(\""<< sql<< "\")"<< fds;
	DebugStream()<< "Estimation: "<< inEstimate<< " IN parameters and "
			<< outEstimate<< " OUT columns"<< fds;

	// Allocates output descriptor and prepares the statement
	mOutRow = new RowImpl(mDatabase->Dialect(), outEstimate, mDatabase, mTransaction);
	mOutRow->AddRef();
	
	status.Reset();
	(*gds.Call()->m_dsql_prepare)(status.Self(), mTransaction->GetHandlePtr(),
		&mHandle, (short)sql.length(), const_cast<char*>(sql.c_str()),
			short(mDatabase->Dialect()), mOutRow->Self());
	if (status.Errors())
	{
		try {Close();} catch(IBPP::Exception&) {}
		std::string context = "Statement::Prepare( ";
		context.append(mSql).append(" )");
		throw SQLExceptionImpl(status, context.c_str(),
			"isc_dsql_prepare failed");
	}

	// Read what kind of statement was prepared
	status.Reset();
	char itemsReq[] = {isc_info_sql_stmt_type};
	char itemsRes[8];
	(*gds.Call()->m_dsql_sql_info)(status.Self(), &mHandle, 1, itemsReq,
		sizeof(itemsRes), itemsRes);
	if (status.Errors())
	{
		try {Close();} catch(IBPP::Exception&) {}
		throw SQLExceptionImpl(status, "Statement::Prepare",
			"isc_dsql_sql_info failed");
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
			case isc_info_sql_stmt_exec_procedure : mType = IBPP::stExecProcedure; break;
			case isc_info_sql_stmt_select_for_upd : mType = IBPP::stSelectUpdate; break;
			case isc_info_sql_stmt_set_generator :	mType = IBPP::stOther; break;
			default : mType = IBPP::stUnsupported;
		}
	}
	if (mType == IBPP::stUnknown || mType == IBPP::stUnsupported)
	{
		try {Close();} catch(IBPP::Exception&) {}
		throw LogicExceptionImpl("Statement::Prepare",
			"Unknown or unsupported statement type");
	}

	if (mOutRow->Columns() == 0)
	{
		// Get rid of the output descriptor, if it wasn't required (no output)
		mOutRow->Release(mOutRow);
		DebugStream()<< "Dropped output descriptor which was not required"<< fds;
	}
	else if (mOutRow->Columns() > mOutRow->AllocatedSize())
	{
		// Resize the output descriptor (which is too small).
		// The statement does not need to be prepared again, though the
		// output columns must be described again.

		DebugStream()<< "Resize output descriptor from "
			<< mOutRow->AllocatedSize()<< " to "<< mOutRow->Columns()<< fds;

		mOutRow->Resize(mOutRow->Columns());
		status.Reset();
		(*gds.Call()->m_dsql_describe)(status.Self(), &mHandle, 1, mOutRow->Self());
		if (status.Errors())
		{
			try {Close();} catch(IBPP::Exception&) {}
			throw SQLExceptionImpl(status, "Statement::Prepare",
				"isc_dsql_describe failed");
		}
	}

	if (inEstimate > 0)
	{
		// Ready an input descriptor
		mInRow = new RowImpl(mDatabase->Dialect(), inEstimate, mDatabase, mTransaction);
		mInRow->AddRef();
		
		status.Reset();
		(*gds.Call()->m_dsql_describe_bind)(status.Self(), &mHandle, 1, mInRow->Self());
		if (status.Errors())
		{
			try {Close();} catch(IBPP::Exception&) {}
			throw SQLExceptionImpl(status, "Statement::Prepare",
				"isc_dsql_describe_bind failed");
		}

		if (mInRow->Columns() == 0)
		{
			// Get rid of the input descriptor, if it wasn't required (no parameters)
			mInRow->Release(mInRow);
			DebugStream()<< "Dropped input descriptor which was not required"<< fds;
		}
		else if (mInRow->Columns() > mInRow->AllocatedSize())
		{
			// Resize the input descriptor (which is too small).
			// The statement does not need to be prepared again, though the
			// parameters must be described again.

			DebugStream()<< "Resize input descriptor from "
					<< mInRow->AllocatedSize()<< " to "
					<< mInRow->Columns()<< fds;
			
			mInRow->Resize(mInRow->Columns());
			status.Reset();
			(*gds.Call()->m_dsql_describe_bind)(status.Self(), &mHandle, 1, mInRow->Self());
			if (status.Errors())
			{
				try {Close();} catch(IBPP::Exception&) {}
				throw SQLExceptionImpl(status, "Statement::Prepare",
					"isc_dsql_describe_bind failed");
			}
		}
	}

	// Allocates variables of the input descriptor
	if (mInRow != 0) mInRow->AllocVariables();

	// Allocates variables of the output descriptor
	if (mOutRow != 0) mOutRow->AllocVariables();
}

void StatementImpl::Plan(std::string& plan)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Plan", "No statement has been prepared.");
	if (mDatabase == 0)
		throw LogicExceptionImpl("Statement::Plan", "A Database must be attached.");
	if (mDatabase->GetHandle() == 0)
		throw LogicExceptionImpl("Statement::Plan", "Database must be connected.");

	IBS status;
	RB result(2048);
	char itemsReq[] = {isc_info_sql_get_plan};

	(*gds.Call()->m_dsql_sql_info)(status.Self(), &mHandle, 1, itemsReq,
								   result.Size(), result.Self());
	if (status.Errors()) throw SQLExceptionImpl(status,
								"Statement::Plan", "isc_dsql_sql_info failed.");

	result.GetString(isc_info_sql_get_plan, plan);
	if (plan[0] == '\n') plan.erase(0, 1);
}

void StatementImpl::Execute(const std::string& sql)
{
	if (! sql.empty()) Prepare(sql);

	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Execute",
			"No statement has been prepared.");

	// Check that a value has been set for each input parameter
	if (mInRow != 0 && mInRow->MissingValues())
		throw LogicExceptionImpl("Statement::Execute",
			"All parameters must be specified.");

	CursorFree();	// Free a previous 'cursor' if any

	IBS status;
	if (mType == IBPP::stSelect)
	{
		// Could return a result set (none, single or multi rows)
		(*gds.Call()->m_dsql_execute)(status.Self(), mTransaction->GetHandlePtr(),
			&mHandle, 1, mInRow == 0 ? 0 : mInRow->Self());
		if (status.Errors())
		{
			//Close();	Commented because Execute error should not free the statement
			std::string context = "Statement::Execute( ";
			context.append(mSql).append(" )");
			throw SQLExceptionImpl(status, context.c_str(),
				"isc_dsql_execute failed");
		}
		if (mOutRow != 0) mResultSetAvailable = true;
	}
	else
	{
		// Should return at most a single row
		(*gds.Call()->m_dsql_execute2)(status.Self(), mTransaction->GetHandlePtr(),
			&mHandle, 1, mInRow == 0 ? 0 : mInRow->Self(),
			mOutRow == 0 ? 0 : mOutRow->Self());
		if (status.Errors())
		{
			//Close();	Commented because Execute error should not free the statement
			std::string context = "Statement::Execute( ";
			context.append(mSql).append(" )");
			throw SQLExceptionImpl(status, context.c_str(),
				"isc_dsql_execute2 failed");
		}
	}
}

void StatementImpl::CursorExecute(const std::string& cursor, const std::string& sql)
{
	if (cursor.empty())
		throw LogicExceptionImpl("Statement::CursorExecute", "Cursor name can't be 0.");

	if (! sql.empty()) Prepare(sql);

	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::CursorExecute", "No statement has been prepared.");
	if (mType != IBPP::stSelectUpdate)
		throw LogicExceptionImpl("Statement::CursorExecute", "Statement must be a SELECT FOR UPDATE.");
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::CursorExecute", "Statement would return no rows.");

	// Check that a value has been set for each input parameter
	if (mInRow != 0 && mInRow->MissingValues())
		throw LogicExceptionImpl("Statement::CursorExecute",
			"All parameters must be specified.");

	CursorFree();	// Free a previous 'cursor' if any

	IBS status;
	(*gds.Call()->m_dsql_execute)(status.Self(), mTransaction->GetHandlePtr(),
		&mHandle, 1, mInRow == 0 ? 0 : mInRow->Self());
	if (status.Errors())
	{
		//Close();	Commented because Execute error should not free the statement
		std::string context = "Statement::CursorExecute( ";
		context.append(mSql).append(" )");
		throw SQLExceptionImpl(status, context.c_str(),
			"isc_dsql_execute failed");
	}

	status.Reset();
	(*gds.Call()->m_dsql_set_cursor_name)(status.Self(), &mHandle, const_cast<char*>(cursor.c_str()), 0);
	if (status.Errors())
	{
		//Close();	Commented because Execute error should not free the statement
		throw SQLExceptionImpl(status, "Statement::CursorExecute",
			"isc_dsql_set_cursor_name failed");
	}

	mResultSetAvailable = true;
}

void StatementImpl::ExecuteImmediate(const std::string& sql)
{
	if (mDatabase == 0)
		throw LogicExceptionImpl("Statement::ExecuteImmediate", "An IDatabase must be attached.");
	if (mDatabase->GetHandle() == 0)
		throw LogicExceptionImpl("Statement::ExecuteImmediate", "IDatabase must be connected.");
	if (mTransaction == 0)
		throw LogicExceptionImpl("Statement::ExecuteImmediate", "An ITransaction must be attached.");
	if (mTransaction->GetHandle() == 0)
		throw LogicExceptionImpl("Statement::ExecuteImmediate", "ITransaction must be started.");
	if (sql.empty())
		throw LogicExceptionImpl("Statement::ExecuteImmediate", "SQL statement can't be 0.");

	IBS status;
	try {Close();} catch(IBPP::Exception&) {}
    (*gds.Call()->m_dsql_execute_immediate)(status.Self(), mDatabase->GetHandlePtr(),
    	mTransaction->GetHandlePtr(), 0, const_cast<char*>(sql.c_str()),
    		short(mDatabase->Dialect()), 0);
    if (status.Errors())
	{
		try {Close();} catch(IBPP::Exception&) {}
		std::string context = "Statement::ExecuteImmediate( ";
		context.append(sql).append(" )");
		throw SQLExceptionImpl(status, context.c_str(),
			"isc_dsql_execute_immediate failed");
	}
}

int StatementImpl::AffectedRows(void)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::AffectedRows", "No statement has been prepared.");
	if (mDatabase == 0)
		throw LogicExceptionImpl("Statement::AffectedRows", "A Database must be attached.");
	if (mDatabase->GetHandle() == 0)
		throw LogicExceptionImpl("Statement::AffectedRows", "Database must be connected.");

	/*
	if (mType != IBPP::stInsert && mType != IBPP::stUpdate && mType != IBPP::stDelete)
	{
		// We return a count of 0 for any non interesting request. For selects, it is
		// assumed to be better to count the rows as they are fetched. So this IBPP API
		// can only be called after a successfull INSERT, UPDATE or DELETE.
		return 0;
	}
	*/

	int count;
	IBS status;
	RB result;
	char itemsReq[] = {isc_info_sql_records};

	(*gds.Call()->m_dsql_sql_info)(status.Self(), &mHandle, 1, itemsReq,
		result.Size(), result.Self());
	if (status.Errors()) throw SQLExceptionImpl(status,
			"Statement::AffectedRows", "isc_dsql_sql_info failed.");

	if (mType == IBPP::stInsert)
			count = result.GetValue(isc_info_sql_records, isc_info_req_insert_count);
	else if (mType == IBPP::stUpdate)
			count = result.GetValue(isc_info_sql_records, isc_info_req_update_count);
	else if (mType == IBPP::stDelete)
			count = result.GetValue(isc_info_sql_records, isc_info_req_delete_count);
	else if (mType == IBPP::stSelect)
			count = result.GetValue(isc_info_sql_records, isc_info_req_select_count);
	else	count = 0;	// Returns zero count for unknown cases

	return count;
}

bool StatementImpl::Fetch(void)
{
	if (! mResultSetAvailable)
		throw LogicExceptionImpl("Statement::Fetch",
			"No statement has been executed or no result set available.");

	IBS status;
	int code = (*gds.Call()->m_dsql_fetch)(status.Self(), &mHandle, 1, mOutRow->Self());
	if (code == 100)	// This special code means "no more rows"
	{
		CursorFree();	// Free the explicit or implicit cursor/result-set
		return false;
	}
	if (status.Errors())
	{
		try {Close();} catch(IBPP::Exception&) {}
		throw SQLExceptionImpl(status, "Statement::Fetch",
			"isc_dsql_fetch failed.");
	}

	return true;
}

bool StatementImpl::Fetch(IBPP::Row& row)
{
	if (! mResultSetAvailable)
		throw LogicExceptionImpl("Statement::Fetch(row)",
			"No statement has been executed or no result set available.");

	RowImpl* rowimpl = new RowImpl(*mOutRow);
	row = rowimpl;

	IBS status;
	int code = (*gds.Call()->m_dsql_fetch)(status.Self(), &mHandle, 1,
					rowimpl->Self());
	if (code == 100)	// This special code means "no more rows"
	{
		CursorFree();	// Free the explicit or implicit cursor/result-set
		row.clear();
		return false;
	}
	if (status.Errors())
	{
		try {Close();} catch(IBPP::Exception&) {}
		row.clear();
		throw SQLExceptionImpl(status, "Statement::Fetch(row)",
			"isc_dsql_fetch failed.");
	}

	return true;
}

void StatementImpl::Close(void)
{
	// Free all statement resources. Used before preparing a new statement or
	// from destructor.
	if (mHandle != 0)
	{
		IBS status;
		(*gds.Call()->m_dsql_free_statement)(status.Self(), &mHandle, DSQL_drop);
		mHandle = 0;
		if (status.Errors())
			throw SQLExceptionImpl(status, "Statement::Close",
				"isc_dsql_free_statement failed.");
	}
	if (mInRow != 0) mInRow->Release(mInRow);
	if (mOutRow != 0) mOutRow->Release(mOutRow);

	mResultSetAvailable = false;
	mType = IBPP::stUnknown;
}

void StatementImpl::SetNull(int param)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::SetNull", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::SetNull", "The statement does not take parameters.");

	mInRow->SetNull(param);
}

void StatementImpl::Set(int param, bool value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[bool]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[bool]", "The statement does not take parameters.");

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, const char* cstring)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[char*]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[char*]", "The statement does not take parameters.");

	mInRow->Set(param, cstring);
}

void StatementImpl::Set(int param, const void* bindata, int len)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[void*]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[void*]", "The statement does not take parameters.");
		
	mInRow->Set(param, bindata, len);
}

void StatementImpl::Set(int param, const std::string& s)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[string]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[string]", "The statement does not take parameters.");

	mInRow->Set(param, s);
}

void StatementImpl::Set(int param, short value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[short]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[short]", "The statement does not take parameters.");
											
	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, int value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[int]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[int]", "The statement does not take parameters.");

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, long value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[long]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[long]", "The statement does not take parameters.");

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, int64_t value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[int64]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[int64]", "The statement does not take parameters.");

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, float value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[float]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[float]", "The statement does not take parameters.");

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, double value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[double]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[double]", "The statement does not take parameters.");

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, const IBPP::Timestamp& value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[Timestamp]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[Timestamp]", "The statement does not take parameters.");

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, const IBPP::Date& value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[Date]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[Date]", "The statement does not take parameters.");

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, const IBPP::Time& value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[Time]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[Time]", "The statement does not take parameters.");

	mInRow->Set(param, value);
}

void StatementImpl::Set(int param, const IBPP::Blob& blob)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[Blob]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[Blob]", "The statement does not take parameters.");

	mInRow->Set(param, blob);
}

void StatementImpl::Set(int param, const IBPP::Array& array)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[Array]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[Array]", "The statement does not take parameters.");

	mInRow->Set(param, array);
}

void StatementImpl::Set(int param, const IBPP::DBKey& key)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[DBKey]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[DBKey]", "The statement does not take parameters.");

	mInRow->Set(param, key);
}

/*
void StatementImpl::Set(int param, const IBPP::Value& value)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Set[Value]", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Set[Value]", "The statement does not take parameters.");

	mInRow->Set(param, value);
}
*/

bool StatementImpl::IsNull(int column)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::IsNull", "The row is not initialized.");

	return mOutRow->IsNull(column);
}

bool StatementImpl::Get(int column, bool* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(column, *retvalue);
}

bool StatementImpl::Get(int column, bool& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, char* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, void* bindata, int& userlen)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, bindata, userlen);
}

bool StatementImpl::Get(int column, std::string& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, short* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(column, *retvalue);
}

bool StatementImpl::Get(int column, short& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, int* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(column, *retvalue);
}

bool StatementImpl::Get(int column, int& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, long* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(column, *retvalue);
}

bool StatementImpl::Get(int column, long& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, int64_t* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(column, *retvalue);
}

bool StatementImpl::Get(int column, int64_t& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, float* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(column, *retvalue);
}

bool StatementImpl::Get(int column, float& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, double* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(column, *retvalue);
}

bool StatementImpl::Get(int column, double& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, retvalue);
}

bool StatementImpl::Get(int column, IBPP::Timestamp& timestamp)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, timestamp);
}

bool StatementImpl::Get(int column, IBPP::Date& date)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, date);
}

bool StatementImpl::Get(int column, IBPP::Time& time)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, time);
}

bool StatementImpl::Get(int column, IBPP::Blob& blob)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, blob);
}

bool StatementImpl::Get(int column, IBPP::DBKey& key)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column, key);
}

bool StatementImpl::Get(int column, IBPP::Array& array)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	
	return mOutRow->Get(column, array);
}

/*
const IBPP::Value StatementImpl::Get(int column)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(column); 
}
*/

bool StatementImpl::IsNull(const std::string& name)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::IsNull", "The row is not initialized.");

	return mOutRow->IsNull(name);
}

bool StatementImpl::Get(const std::string& name, bool* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(name, *retvalue);
}

bool StatementImpl::Get(const std::string& name, bool& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, char* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get[char*]", "The row is not initialized.");

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, void* retvalue, int& count)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get[void*,int]", "The row is not initialized.");

	return mOutRow->Get(name, retvalue, count);
}

bool StatementImpl::Get(const std::string& name, std::string& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::GetString", "The row is not initialized.");

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, short* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(name, *retvalue);
}

bool StatementImpl::Get(const std::string& name, short& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, int* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(name, *retvalue);
}

bool StatementImpl::Get(const std::string& name, int& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, long* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(name, *retvalue);
}

bool StatementImpl::Get(const std::string& name, long& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, int64_t* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(name, *retvalue);
}

bool StatementImpl::Get(const std::string& name, int64_t& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, float* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(name, *retvalue);
}

bool StatementImpl::Get(const std::string& name, float& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, double* retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Statement::Get", "Null pointer detected");

	return mOutRow->Get(name, *retvalue);
}

bool StatementImpl::Get(const std::string& name, double& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, IBPP::Timestamp& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, IBPP::Date& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, IBPP::Time& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string&name, IBPP::Blob& retblob)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name, retblob);
}

bool StatementImpl::Get(const std::string& name, IBPP::DBKey& retvalue)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name, retvalue);
}

bool StatementImpl::Get(const std::string& name, IBPP::Array& retarray)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name, retarray);
}

/*
const IBPP::Value StatementImpl::Get(const std::string& name)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Get", "The row is not initialized.");

	return mOutRow->Get(name);
}
*/

int StatementImpl::Columns(void)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Columns", "The row is not initialized.");

	return mOutRow->Columns();
}

int StatementImpl::ColumnNum(const std::string& name)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::ColumnNum", "The row is not initialized.");

	return mOutRow->ColumnNum(name);
}

const char* StatementImpl::ColumnName(int varnum)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Columns", "The row is not initialized.");

	return mOutRow->ColumnName(varnum);
}

const char* StatementImpl::ColumnAlias(int varnum)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Columns", "The row is not initialized.");

	return mOutRow->ColumnAlias(varnum);
}

const char* StatementImpl::ColumnTable(int varnum)
{
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::Columns", "The row is not initialized.");

	return mOutRow->ColumnTable(varnum);
}

IBPP::SDT StatementImpl::ColumnType(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ColumnType", "No statement has been prepared.");
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::ColumnType", "The statement does not return results.");

    return mOutRow->ColumnType(varnum);
}

int StatementImpl::ColumnSize(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ColumnSize", "No statement has been prepared.");
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::ColumnSize", "The row is not initialized.");

	return mOutRow->ColumnSize(varnum);
}

int StatementImpl::ColumnScale(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ColumnScale", "No statement has been prepared.");
	if (mOutRow == 0)
		throw LogicExceptionImpl("Statement::ColumnScale", "The row is not initialized.");

	return mOutRow->ColumnScale(varnum);
}

int StatementImpl::Parameters(void)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::Parameters", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::Parameters", "The statement uses no parameters.");

	return mInRow->Columns();
}

IBPP::SDT StatementImpl::ParameterType(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ParameterType", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::ParameterType", "The statement uses no parameters.");

    return mInRow->ColumnType(varnum);
}

int StatementImpl::ParameterSize(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ParameterSize", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::ParameterSize", "The statement uses no parameters.");

	return mInRow->ColumnSize(varnum);
}

int StatementImpl::ParameterScale(int varnum)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Statement::ParameterScale", "No statement has been prepared.");
	if (mInRow == 0)
		throw LogicExceptionImpl("Statement::ParameterScale", "The statement uses no parameters.");

	return mInRow->ColumnScale(varnum);
}

IBPP::IStatement* StatementImpl::AddRef(void)
{
	ASSERTION(mRefCount >= 0);
	++mRefCount;

	return this;
}

void StatementImpl::Release(IBPP::IStatement*& Self)
{
	if (this != dynamic_cast<StatementImpl*>(Self))
		throw LogicExceptionImpl("Statement::Release", "Invalid Release()");

	ASSERTION(mRefCount >= 0);

	--mRefCount;

	if (mRefCount <= 0) delete this;
	Self = 0;
}

//	(((((((( OBJECT INTERNAL METHODS ))))))))

void StatementImpl::AttachDatabase(DatabaseImpl* database)
{
	if (database == 0)
		throw LogicExceptionImpl("Statement::AttachDatabase",
			"Can't attach a 0 IDatabase object.");

	if (mDatabase != 0) mDatabase->DetachStatement(this);
	mDatabase = database;
	mDatabase->AttachStatement(this);
}

void StatementImpl::DetachDatabase(void)
{
	if (mDatabase == 0)
		throw LogicExceptionImpl("Statement::DetachDatabase",
			"No IDatabase was attached.");

	try {Close();} catch(IBPP::Exception&) {}
	mDatabase->DetachStatement(this);
	mDatabase = 0;
}

void StatementImpl::AttachTransaction(TransactionImpl* transaction)
{
	if (transaction == 0)
		throw LogicExceptionImpl("Statement::AttachTransaction",
			"Can't attach a 0 ITransaction object.");

	if (mTransaction != 0) mTransaction->DetachStatement(this);
	mTransaction = transaction;
	mTransaction->AttachStatement(this);
}

void StatementImpl::DetachTransaction(void)
{
	if (mTransaction == 0)
		throw LogicExceptionImpl("Statement::DetachTransaction",
			"No ITransaction was attached.");

	try {Close();} catch(IBPP::Exception&) {}
	mTransaction->DetachStatement(this);
	mTransaction = 0;
}

void StatementImpl::CursorFree(void)
{
	if (mResultSetAvailable)
	{
		if (mHandle != 0)
		{
			IBS status;
			(*gds.Call()->m_dsql_free_statement)(status.Self(), &mHandle, DSQL_close);
			if (status.Errors())
				throw SQLExceptionImpl(status, "StatementImpl::CursorFree",
					"isc_dsql_free_statement failed.");
		}
		mResultSetAvailable = false;
	}
}

StatementImpl::StatementImpl(DatabaseImpl* database, TransactionImpl* transaction,
	const std::string& sql)
	: mRefCount(0), mHandle(0), mDatabase(0), mTransaction(0),
	mInRow(0), mOutRow(0),
	mResultSetAvailable(false), mType(IBPP::stUnknown)
{
	AttachDatabase(database);
	if (transaction != 0) AttachTransaction(transaction);
	if (! sql.empty()) Prepare(sql);
}

StatementImpl::~StatementImpl()
{
	try {Close();} catch (IBPP::Exception&) {}
	if (mTransaction != 0) mTransaction->DetachStatement(this);
	if (mDatabase != 0) mDatabase->DetachStatement(this);
}

//
//	EOF
//
