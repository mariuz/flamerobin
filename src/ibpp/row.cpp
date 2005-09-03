///////////////////////////////////////////////////////////////////////////////
//
//	File    : $Id$
//	Subject : IBPP, Row class implementation
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

#include <limits>

#include <math.h>
#include <time.h>

using namespace ibpp_internals;

namespace ibpp_internals
{
	const double dscales[19] =
	{
		1, 1E1, 1E2, 1E3, 1E4,	1E5, 1E6, 1E7, 1E8,
		1E9, 1E10, 1E11, 1E12, 1E13, 1E14, 1E15,
		1E16, 1E17, 1E18
	};

// Many compilers confuses those following min/max with macros min and max !
#undef min
#undef max

#ifdef IBPP_DMC // Needs to break-down the declaration else compiler crash (!)
	const std::numeric_limits<short> short_limits;
	const std::numeric_limits<long> long_limits;
	const short minshort = short_limits.min();
	const short maxshort = short_limits.max();
	const long minlong = long_limits.min();
	const long maxlong = long_limits.max();
#else
	const short minshort = std::numeric_limits<short>::min();
	const short maxshort = std::numeric_limits<short>::max();
	const long minlong = std::numeric_limits<long>::min();
	const long maxlong = std::numeric_limits<long>::max();
#endif

};

//	(((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

IBPP::IDatabase* RowImpl::Database(void) const
{
	return mDatabase;
}

IBPP::ITransaction* RowImpl::Transaction(void) const
{
	return mTransaction;
}

void RowImpl::SetNull(int param)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::SetNull", "The row is not initialized.");
	if (param < 1 || param > mDescrArea->sqld)
		throw LogicExceptionImpl("Row::SetNull", "Variable index out of range.");

	XSQLVAR* var = &(mDescrArea->sqlvar[param-1]);
	if (! (var->sqltype & 1))
		throw LogicExceptionImpl("Row::SetNull", "This column can't be null.");

	*var->sqlind = -1;	// Set the column to SQL NULL
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, bool value)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[bool]", "The row is not initialized.");

	SetValue(param, ivBool, &value);
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, const char* cstring)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[char*]", "The row is not initialized.");
	if (cstring == 0)
		throw LogicExceptionImpl("Row::Set[char*]", "null char* pointer detected.");

	SetValue(param, ivByte, cstring, (int)strlen(cstring));
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, const void* bindata, int len)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[void*]", "The row is not initialized.");
	if (bindata == 0)
		throw LogicExceptionImpl("Row::Set[void*]", "null char* pointer detected.");
	if (len < 0)
		throw LogicExceptionImpl("Row::Set[void*]", "Length must be >= 0");
		
	SetValue(param, ivByte, bindata, len);
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, const std::string& s)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[string]", "The row is not initialized.");

	SetValue(param, ivString, (void*)&s);
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, short value)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[short]", "The row is not initialized.");
											
	SetValue(param, ivShort, &value);
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, int value)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[int]", "The row is not initialized.");

	SetValue(param, ivLong, &value);
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, long value)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[long]", "The row is not initialized.");

	SetValue(param, ivLong, &value);
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, int64_t value)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[int64]", "The row is not initialized.");

	SetValue(param, ivLarge, &value);
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, float value)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[float]", "The row is not initialized.");

	SetValue(param, ivFloat, &value);
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, double value)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[double]", "The row is not initialized.");

	SetValue(param, ivDouble, &value);
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, const IBPP::Timestamp& value)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[Timestamp]", "The row is not initialized.");

	SetValue(param, ivTimestamp, &value);
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, const IBPP::Date& value)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[Date]", "The row is not initialized.");

	if (mDialect == 1)
	{
		// In dialect 1, IBPP::Date is supposed to work with old 'DATE'
		// fields which are actually ISC_TIMESTAMP.
		IBPP::Timestamp timestamp;
		timestamp = value;
		SetValue(param, ivTimestamp, &timestamp);
	}
	else
	{
		// Dialect 3
		SetValue(param, ivDate, (void*)&value);
	}

	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, const IBPP::Time& value)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[Time]", "The row is not initialized.");
	if (mDialect == 1)
		throw LogicExceptionImpl("Row::Set[Time]", "Requires use of a dialect 3 database.");

	SetValue(param, ivTime, &value);
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, const IBPP::Blob& blob)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[Blob]", "The row is not initialized.");
	if (mDatabase != 0 && dynamic_cast<DatabaseImpl*>(blob->Database()) != mDatabase)
		throw LogicExceptionImpl("Row::Set[Blob]",
			"IBlob and Row attached to different databases");
	if (mTransaction != 0 && dynamic_cast<TransactionImpl*>(blob->Transaction()) != mTransaction)
		throw LogicExceptionImpl("Row::Set[Blob]",
			"IBlob and Row attached to different transactions");

	SetValue(param, ivBlob, blob.intf());
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, const IBPP::Array& array)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[Array]", "The row is not initialized.");
	if (mDatabase != 0 && dynamic_cast<DatabaseImpl*>(array->Database()) != mDatabase)
		throw LogicExceptionImpl("Row::Set[Array]",
			"IArray and Row attached to different databases");
	if (mTransaction != 0 && dynamic_cast<TransactionImpl*>(array->Transaction()) != mTransaction)
		throw LogicExceptionImpl("Row::Set[Array]",
			"IArray and Row attached to different transactions");

	SetValue(param, ivArray, (void*)array.intf());
	mUpdated[param-1] = true;
}

void RowImpl::Set(int param, const IBPP::DBKey& key)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[DBKey]", "The row is not initialized.");

	SetValue(param, ivDBKey, (void*)&key);
	mUpdated[param-1] = true;
}

/*
void RowImpl::Set(int param, const IBPP::Value& value)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Set[Value]", "The row is not initialized.");

	//SetValue(param, ivDBKey, (void*)&key);
	//mUpdated[param-1] = true;
}
*/

bool RowImpl::IsNull(int column)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::IsNull", "The row is not initialized.");
	if (column < 1 || column > mDescrArea->sqld)
		throw LogicExceptionImpl("Row::IsNull", "Variable index out of range.");

	XSQLVAR* var = &(mDescrArea->sqlvar[column-1]);
	return ((var->sqltype & 1) && *(var->sqlind) != 0) ? true : false;
}

bool RowImpl::Get(int column, bool& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	void* pvalue = GetValue(column, ivBool);
	if (pvalue != 0)
		retvalue = (*(short*)pvalue == 0 ? false : true);
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, char* retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");
	if (retvalue == 0)
		throw LogicExceptionImpl("Row::Get", "Null pointer detected");

	int sqllen;
	void* pvalue = GetValue(column, ivByte, &sqllen);
	if (pvalue != 0)
	{
		memcpy(retvalue, pvalue, sqllen);
		retvalue[sqllen] = '\0';
	}
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, void* bindata, int& userlen)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");
	if (bindata == 0)
		throw LogicExceptionImpl("Row::Get", "Null pointer detected");
	if (userlen < 0)
		throw LogicExceptionImpl("Row::Get", "Length must be >= 0");

	int sqllen;
	void* pvalue = GetValue(column, ivByte, &sqllen);
	if (pvalue != 0)
	{
		// userlen says how much bytes the user can accept
		// let's shorten it, if there is less bytes available
		if (sqllen < userlen) userlen = sqllen;
		memcpy(bindata, pvalue, userlen);
	}
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, std::string& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	void* pvalue = GetValue(column, ivString, &retvalue);
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, short& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	void* pvalue = GetValue(column, ivShort);
	if (pvalue != 0)
		retvalue = *(short*)pvalue;
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, int& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	void* pvalue = GetValue(column, ivLong);
	if (pvalue != 0)
		retvalue = *(int*)pvalue;
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, long& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	void* pvalue = GetValue(column, ivLong);
	if (pvalue != 0)
		retvalue = *(long*)pvalue;
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, int64_t& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	void* pvalue = GetValue(column, ivLarge);
	if (pvalue != 0)
		retvalue = *(int64_t*)pvalue;
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, float& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	void* pvalue = GetValue(column, ivFloat);
	if (pvalue != 0)
		retvalue = *(float*)pvalue;
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, double& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	void* pvalue = GetValue(column, ivDouble);
	if (pvalue != 0)
		retvalue = *(double*)pvalue;
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, IBPP::Timestamp& timestamp)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	void* pvalue = GetValue(column, ivTimestamp, (void*)&timestamp);
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, IBPP::Date& date)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	if (mDialect == 1)
	{
		// Dialect 1. IBPP::Date is supposed to work with old 'DATE'
		// fields which are actually ISC_TIMESTAMP.
		IBPP::Timestamp timestamp;
		void* pvalue = GetValue(column, ivTimestamp, (void*)&timestamp);
		if (pvalue != 0) date = timestamp;
		return pvalue == 0 ? true : false;
	}
	else
	{
		void* pvalue = GetValue(column, ivDate, (void*)&date);
		return pvalue == 0 ? true : false;
	}
}

bool RowImpl::Get(int column, IBPP::Time& time)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	void* pvalue = GetValue(column, ivTime, (void*)&time);
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, IBPP::Blob& retblob)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	void* pvalue = GetValue(column, ivBlob, (void*)retblob.intf());
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, IBPP::DBKey& retkey)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	void* pvalue = GetValue(column, ivDBKey, (void*)&retkey);
	return pvalue == 0 ? true : false;
}

bool RowImpl::Get(int column, IBPP::Array& retarray)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	void* pvalue = GetValue(column, ivArray, (void*)retarray.intf());
	return pvalue == 0 ? true : false;
}

/*
const IBPP::Value RowImpl::Get(int column)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	//void* value = GetValue(column, ivArray, (void*)retarray.intf());
	//return value == 0 ? true : false;
	return IBPP::Value();
}
*/

bool RowImpl::IsNull(const std::string& name)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::IsNull", "The row is not initialized.");

	return IsNull(ColumnNum(name));
}

bool RowImpl::Get(const std::string& name, bool& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue);
}

bool RowImpl::Get(const std::string& name, char* retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get[char*]", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue);
}

bool RowImpl::Get(const std::string& name, void* retvalue, int& count)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get[void*,int]", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue, count);
}

bool RowImpl::Get(const std::string& name, std::string& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::GetString", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue);
}

bool RowImpl::Get(const std::string& name, short& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue);
}

bool RowImpl::Get(const std::string& name, int& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue);
}

bool RowImpl::Get(const std::string& name, long& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue);
}

bool RowImpl::Get(const std::string& name, int64_t& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue);
}

bool RowImpl::Get(const std::string& name, float& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue);
}

bool RowImpl::Get(const std::string& name, double& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue);
}

bool RowImpl::Get(const std::string& name, IBPP::Timestamp& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue);
}

bool RowImpl::Get(const std::string& name, IBPP::Date& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue);
}

bool RowImpl::Get(const std::string& name, IBPP::Time& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue);
}

bool RowImpl::Get(const std::string&name, IBPP::Blob& retblob)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name), retblob);
}

bool RowImpl::Get(const std::string& name, IBPP::DBKey& retvalue)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name), retvalue);
}

bool RowImpl::Get(const std::string& name, IBPP::Array& retarray)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name), retarray);
}

/*
const IBPP::Value RowImpl::Get(const std::string& name)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Get", "The row is not initialized.");

	return Get(ColumnNum(name));
}
*/

int RowImpl::Columns(void)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::Columns", "The row is not initialized.");

	return mDescrArea->sqld;
}

int RowImpl::ColumnNum(const std::string& name)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::ColumnNum", "The row is not initialized.");
	if (name.empty())
		throw LogicExceptionImpl("Row::ColumnNum", "Column name <empty> not found.");

	XSQLVAR* var;
	char Uname[sizeof(var->sqlname)+1];		// Max size of sqlname + '\0'

	// Local upper case copy of the column name
	size_t len = name.length();
	if (len > sizeof(var->sqlname)) len = sizeof(var->sqlname);
	strncpy(Uname, name.c_str(), len);
	Uname[len] = '\0';
	char* p = Uname;
	while (*p != '\0') { *p = char(toupper(*p)); ++p; }

	// Loop through the columns of the descriptor
	for (int i = 0; i < mDescrArea->sqld; i++)
	{
		var = &(mDescrArea->sqlvar[i]);
		if (var->sqlname_length != (short)len) continue;
		if (strncmp(Uname, var->sqlname, len) == 0) return i+1;
	}

	// Failed finding the column name, let's retry using the aliases
	char Ualias[sizeof(var->aliasname)+1];		// Max size of aliasname + '\0'

	// Local upper case copy of the column name
	len = name.length();
	if (len > sizeof(var->aliasname)) len = sizeof(var->aliasname);
	strncpy(Ualias, name.c_str(), len);
	Ualias[len] = '\0';
	p = Ualias;
	while (*p != '\0') { *p = char(toupper(*p)); ++p; }

	// Loop through the columns of the descriptor
	for (int i = 0; i < mDescrArea->sqld; i++)
	{
		var = &(mDescrArea->sqlvar[i]);
		if (var->aliasname_length != (short)len) continue;
		if (strncmp(Ualias, var->aliasname, len) == 0) return i+1;
	}

	throw LogicExceptionImpl("Row::ColumnNum", "Could not find matching column.");
}

/*
ColumnName, ColumnAlias, ColumnTable : all these 3 have a mistake.
Ideally, the strings should be stored elsewhere (like _Numerics and so on) to
take into account the final '\0' which needs to be added. For now, we insert
the '\0' in the original data, which will cut the 32th character. Not terribly
bad, but should be cleanly rewritten.
*/

const char* RowImpl::ColumnName(int varnum)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::ColumnName", "The row is not initialized.");
	if (varnum < 1 || varnum > mDescrArea->sqld)
		throw LogicExceptionImpl("Row::ColumName", "Variable index out of range.");

	XSQLVAR* var = &(mDescrArea->sqlvar[varnum-1]);
	if (var->sqlname_length >= 31) var->sqlname_length = 31;
	var->sqlname[var->sqlname_length] = '\0';
	return var->sqlname;
}

const char* RowImpl::ColumnAlias(int varnum)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::ColumnAlias", "The row is not initialized.");
	if (varnum < 1 || varnum > mDescrArea->sqld)
		throw LogicExceptionImpl("Row::ColumnAlias", "Variable index out of range.");

	XSQLVAR* var = &(mDescrArea->sqlvar[varnum-1]);
	if (var->aliasname_length >= 31) var->aliasname_length = 31;
	var->aliasname[var->aliasname_length] = '\0';
	return var->aliasname;
}

const char* RowImpl::ColumnTable(int varnum)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::ColumnTable", "The row is not initialized.");
	if (varnum < 1 || varnum > mDescrArea->sqld)
		throw LogicExceptionImpl("Row::ColumnTable", "Variable index out of range.");

	XSQLVAR* var = &(mDescrArea->sqlvar[varnum-1]);
	if (var->relname_length >= 31) var->relname_length = 31;
	var->relname[var->relname_length] = '\0';
	return var->relname;
}

IBPP::SDT RowImpl::ColumnType(int varnum)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::ColumnType", "The row is not initialized.");
	if (varnum < 1 || varnum > mDescrArea->sqld)
		throw LogicExceptionImpl("Row::ColumnType", "Variable index out of range.");

	IBPP::SDT value;
	XSQLVAR* var = &(mDescrArea->sqlvar[varnum-1]);

	switch (var->sqltype & ~1)
	{
		case SQL_TEXT :      value = IBPP::sdString;    break;
		case SQL_VARYING :   value = IBPP::sdString;    break;
		case SQL_SHORT :     value = IBPP::sdSmallint;  break;
		case SQL_LONG :      value = IBPP::sdInteger;   break;
		case SQL_INT64 :     value = IBPP::sdLargeint;  break;
		case SQL_FLOAT :     value = IBPP::sdFloat;     break;
		case SQL_DOUBLE :    value = IBPP::sdDouble;    break;
		case SQL_TIMESTAMP : value = IBPP::sdTimestamp; break;
		case SQL_TYPE_DATE : value = IBPP::sdDate;      break;
		case SQL_TYPE_TIME : value = IBPP::sdTime;      break;
		case SQL_BLOB :      value = IBPP::sdBlob;      break;
		case SQL_ARRAY :     value = IBPP::sdArray;     break;
		default : throw LogicExceptionImpl("Row::ColumnType",
						"Found an unknown sqltype !");
	}

	return value;
}

int RowImpl::ColumnSize(int varnum)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::ColumnSize", "The row is not initialized.");
	if (varnum < 1 || varnum > mDescrArea->sqld)
		throw LogicExceptionImpl("Row::ColumnSize", "Variable index out of range.");

	XSQLVAR* var = &(mDescrArea->sqlvar[varnum-1]);
    return var->sqllen;
}

int RowImpl::ColumnScale(int varnum)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::ColumnScale", "The row is not initialized.");
	if (varnum < 1 || varnum > mDescrArea->sqld)
		throw LogicExceptionImpl("Row::ColumnScale", "Variable index out of range.");

	XSQLVAR* var = &(mDescrArea->sqlvar[varnum-1]);
    return -var->sqlscale;
}

bool RowImpl::ColumnUpdated(int varnum)
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::ColumnUpdated", "The row is not initialized.");
	if (varnum < 1 || varnum > mDescrArea->sqld)
		throw LogicExceptionImpl("Row::ColumnUpdated", "Variable index out of range.");

	return mUpdated[varnum-1];
}

bool RowImpl::Updated()
{
	if (mDescrArea == 0)
		throw LogicExceptionImpl("Row::ColumnUpdated", "The row is not initialized.");

	for (int i = 0; i < mDescrArea->sqld; i++)
		if (mUpdated[i]) return true;
	return false;
}

IBPP::IRow* RowImpl::AddRef(void)
{
	ASSERTION(mRefCount >= 0);
	++mRefCount;
	return this;
}

IBPP::IRow* RowImpl::Clone()
{
	// By definition the clone of an IBPP Row is a new row (so refcount=0).

	RowImpl* clone = new RowImpl(*this);
	return clone;
}

void RowImpl::Release(IBPP::IRow*& Self)
{
	if (this != dynamic_cast<RowImpl*>(Self))
		throw LogicExceptionImpl("Row::Release", "Invalid Release()");

	ASSERTION(mRefCount >= 0);
	--mRefCount;
	if (mRefCount <= 0) delete this;
	Self = 0;
}

//	(((((((( OBJECT INTERNAL METHODS ))))))))

void RowImpl::Release(RowImpl*& Self)
{
	if (this != Self)
		throw LogicExceptionImpl("RowImpl::Release", "Invalid Release()");

	ASSERTION(mRefCount >= 0);
	--mRefCount;
	if (mRefCount <= 0) delete this;
	Self = 0;
}

void RowImpl::SetValue(int varnum, IITYPE ivType, const void* value, int userlen)
{
	if (varnum < 1 || varnum > mDescrArea->sqld)
		throw LogicExceptionImpl("RowImpl::SetValue", "Variable index out of range.");
	if (value == 0)
		throw LogicExceptionImpl("RowImpl::SetValue", "Unexpected null pointer detected.");

	short len;
	XSQLVAR* var = &(mDescrArea->sqlvar[varnum-1]);
	switch (var->sqltype & ~1)
	{
		case SQL_TEXT :
			if (ivType == ivString)
			{
				std::string* svalue = (std::string*)value;
				len = (short)svalue->length();
				if (len > var->sqllen) len = var->sqllen;
				strncpy(var->sqldata, svalue->c_str(), len);
				while (len < var->sqllen) var->sqldata[len++] = ' ';
			}
			else if (ivType == ivByte)
			{
				if (userlen > var->sqllen) userlen = var->sqllen;
				memcpy(var->sqldata, value, userlen);
				while (userlen < var->sqllen) var->sqldata[userlen++] = ' ';
			}
			else if (ivType == ivDBKey)
			{
				IBPP::DBKey* key = (IBPP::DBKey*)value;
				key->GetKey(var->sqldata, var->sqllen);
			}
			else if (ivType == ivBool)
			{
				var->sqldata[0] = *(bool*)value ? 'T' : 'F';
				len = 1;
				while (len < var->sqllen) var->sqldata[len++] = ' ';
			}
			else throw LogicExceptionImpl("RowImpl::SetValue", "Incompatible types.");
			break;

		case SQL_VARYING :
			if (ivType == ivString)
			{
				std::string* svalue = (std::string*)value;
				len = (short)svalue->length();
				if (len > var->sqllen) len = var->sqllen;
				*(short*)var->sqldata = (short)len;
				strncpy(var->sqldata+2, svalue->c_str(), len);
			}
			else if (ivType == ivByte)
			{
				if (userlen > var->sqllen) userlen = var->sqllen;
				*(short*)var->sqldata = (short)userlen;
				memcpy(var->sqldata+2, value, userlen);
			}
			else if (ivType == ivBool)
			{
				*(short*)var->sqldata = (short)1;
				var->sqldata[2] = *(bool*)value ? 'T' : 'F';
			}
			else throw LogicExceptionImpl("RowImpl::SetValue", "Incompatible types.");
			break;

		case SQL_SHORT :
			if (ivType == ivBool)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"NUM/DEC with scale : use SetDouble()");
				*(short*)var->sqldata = short(*(bool*)value ? 1 : 0);
			}
			else if (ivType == ivShort)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"NUM/DEC with scale : use SetDouble()");
				*(short*)var->sqldata = *(short*)value;
			}
			else if (ivType == ivLong)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"NUM/DEC with scale : use SetDouble()");
				if (*(long*)value < minshort || *(long*)value > maxshort)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"Out of range numeric conversion !");
				*(short*)var->sqldata = (short)*(long*)value;
			}
			else if (ivType == ivLarge)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"NUM/DEC with scale : use SetDouble()");
				if (*(int64_t*)value < minshort || *(int64_t*)value > maxshort)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"Out of range numeric conversion !");
				*(short*)var->sqldata = (short)*(int64_t*)value;
			}
			else if (ivType == ivFloat)
			{
				// This SQL_SHORT is a NUMERIC(x,y), scale it !
				double multiplier = dscales[-var->sqlscale];
				*(short*)var->sqldata =
					(short)floor(*(float*)value * multiplier + 0.5);
			}
			else if (ivType == ivDouble)
			{
				// This SQL_SHORT is a NUMERIC(x,y), scale it !
				double multiplier = dscales[-var->sqlscale];
				*(short*)var->sqldata =
					(short)floor(*(double*)value * multiplier + 0.5);
			}
			else throw LogicExceptionImpl("RowImpl::SetValue", "Incompatible types.");
			break;

		case SQL_LONG :
			if (ivType == ivBool)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"NUM/DEC with scale : use SetDouble()");
				*(long*)var->sqldata = *(bool*)value ? 1L : 0L;
			}
			else if (ivType == ivLong)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"NUM/DEC with scale : use SetDouble()");
				*(long*)var->sqldata = *(long*)value;
			}
			else if (ivType == ivShort)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"NUM/DEC with scale : use SetDouble()");
				*(long*)var->sqldata = *(short*)value;
			}
			else if (ivType == ivLarge)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"NUM/DEC with scale : use SetDouble()");
				if (*(int64_t*)value < minlong || *(int64_t*)value > maxlong)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"Out of range numeric conversion !");
				*(long*)var->sqldata = (long)*(int64_t*)value;
			}
			else if (ivType == ivFloat)
			{
				// This SQL_LONG is a NUMERIC(x,y), scale it !
				double multiplier = dscales[-var->sqlscale];
				*(long*)var->sqldata =
					(long)floor(*(float*)value * multiplier + 0.5);
			}
			else if (ivType == ivDouble)
			{
				// This SQL_LONG is a NUMERIC(x,y), scale it !
				double multiplier = dscales[-var->sqlscale];
				*(long*)var->sqldata =
					(long)floor(*(double*)value * multiplier + 0.5);
			}
			else throw LogicExceptionImpl("RowImpl::SetValue", "Incompatible types.");
			break;

		case SQL_INT64 :
			if (ivType == ivBool)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"Numeric(x,y) field : use IStatement::GetDouble()");
				*(int64_t*)var->sqldata = *(bool*)value ? 1 : 0;
			}
			else if (ivType == ivLarge)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"Numeric(x,y) field : use IStatement::GetDouble()");
				*(int64_t*)var->sqldata = *(int64_t*)value;
			}
			else if (ivType == ivShort)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"NUM/DEC with scale : use SetDouble()");
				*(int64_t*)var->sqldata = *(short*)value;
			}
			else if (ivType == ivLong)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::SetValue",
						"NUM/DEC with scale : use SetDouble()");
				*(int64_t*)var->sqldata = *(long*)value;
			}
			else if (ivType == ivFloat)
			{
				// This SQL_INT64 is a NUMERIC(x,y), scale it !
				double multiplier = dscales[-var->sqlscale];
				*(int64_t*)var->sqldata =
					(int64_t)floor(*(float*)value * multiplier + 0.5);
			}
			else if (ivType == ivDouble)
			{
				// This SQL_INT64 is a NUMERIC(x,y), scale it !
				double multiplier = dscales[-var->sqlscale];
				*(int64_t*)var->sqldata =
					(int64_t)floor(*(double*)value * multiplier + 0.5);
			}
			else throw LogicExceptionImpl("RowImpl::SetValue", "Incompatible types.");
			break;

		case SQL_FLOAT :
			if (ivType != ivFloat || var->sqlscale != 0)
				throw LogicExceptionImpl("RowImpl::SetValue", "Incompatible types.");
			*(float*)var->sqldata = *(float*)value;
			break;

		case SQL_DOUBLE :
			if (ivType != ivDouble) throw LogicExceptionImpl("RowImpl::SetValue",
										"Incompatible types.");
			if (var->sqlscale != 0)
			{
				// Round to scale of NUMERIC(x,y)
				double multiplier = dscales[-var->sqlscale];
				*(double*)var->sqldata =
					floor(*(double*)value * multiplier + 0.5) / multiplier;
			}
			else *(double*)var->sqldata = *(double*)value;
			break;

		case SQL_TIMESTAMP :
			if (ivType != ivTimestamp)
				throw LogicExceptionImpl("RowImpl::SetValue", "Incompatible types.");
			{
				IBPP::Timestamp* timestamp = (IBPP::Timestamp*)value;
				//timestamp->GetNative((ISC_TIMESTAMP*)var->sqldata);
				int year, month, day;
				int hour, minute, second;
				timestamp->GetDate(year, month, day);
				timestamp->GetTime(hour, minute, second);
				tm tms;
				memset(&tms, 0, sizeof(tms));
				tms.tm_year = year - 1900;
				tms.tm_mon = month - 1;
				tms.tm_mday = day;
				tms.tm_hour = hour;
				tms.tm_min = minute;
				tms.tm_sec = second;
				(*gds.Call()->m_encode_timestamp)(&tms, (ISC_TIMESTAMP*)var->sqldata);
			}
			break;

		case SQL_TYPE_DATE :
			if (ivType != ivDate) throw LogicExceptionImpl("RowImpl::SetValue",
										"Incompatible types.");
			{
				IBPP::Date* date = (IBPP::Date*)value;
				//date->GetNative((ISC_DATE*)var->sqldata);
				int year, month, day;
				date->GetDate(year, month, day);
				tm tms;
				memset(&tms, 0, sizeof(tms));
				tms.tm_year = year - 1900;
				tms.tm_mon = month - 1;
				tms.tm_mday = day;
				(*gds.Call()->m_encode_sql_date)(&tms, (ISC_DATE*)var->sqldata);
			}
			break;

		case SQL_TYPE_TIME :
			if (ivType != ivTime) throw LogicExceptionImpl("RowImpl::SetValue",
										"Incompatible types.");
			{
				IBPP::Time* time = (IBPP::Time*)value;
				//time->GetNative((ISC_TIME*)var->sqldata);
				int hour, minute, second;
				time->GetTime(hour, minute, second);
				tm tms;
				memset(&tms, 0, sizeof(tms));
				tms.tm_hour = hour;
				tms.tm_min = minute;
				tms.tm_sec = second;
				(*gds.Call()->m_encode_sql_time)(&tms, (ISC_TIME*)var->sqldata);
			}
			break;

		case SQL_BLOB :
			if (ivType == ivBlob)
			{
				BlobImpl* blob = (BlobImpl*)value;
				blob->GetId((ISC_QUAD*)var->sqldata);
			}
			else if (ivType == ivString)
			{
				BlobImpl blob(mDatabase, mTransaction);
				blob.Save(*(std::string*)value);
				blob.GetId((ISC_QUAD*)var->sqldata);
			}
			else throw LogicExceptionImpl("RowImpl::SetValue",
					"Incompatible types.");
			break;

		case SQL_ARRAY :
			if (ivType != ivArray) throw LogicExceptionImpl("RowImpl::SetValue",
										"Incompatible types.");
			{
				ArrayImpl* array = (ArrayImpl*)value;
				array->GetId((ISC_QUAD*)var->sqldata);
				// When an array has been affected to a column, we want to reset
				// its ID. This way, the next WriteFrom() on the same Array object
				// will allocate a new ID. This protects against storing the same
				// array ID in multiple columns or rows.
				array->ResetId();
			}
			break;

		default : throw LogicExceptionImpl("RowImpl::SetValue",
						"The field uses an unsupported SQL type !");
	}

	if (var->sqltype & 1) *var->sqlind = 0;		// Remove the 0 flag
}

void* RowImpl::GetValue(int varnum, IITYPE ivType, void* retvalue)
{
	if (varnum < 1 || varnum > mDescrArea->sqld)
		throw LogicExceptionImpl("RowImpl::GetValue", "Variable index out of range.");

	void* value;
	int len;
	XSQLVAR* var = &(mDescrArea->sqlvar[varnum-1]);

	// When there is no value (SQL NULL)
	if ((var->sqltype & 1) && *(var->sqlind) != 0) return 0;

	switch (var->sqltype & ~1)
	{
		case SQL_TEXT :
			if (ivType == ivString)
			{
				// In case of ivString, 'void* retvalue' points to a std::string where we
				// will directly store the data.
				std::string* str = (std::string*)retvalue;
				str->erase();
				str->append(var->sqldata, var->sqllen);
				value = retvalue;	// value != 0 means 'not null'
			}
			else if (ivType == ivByte)
			{
				// In case of ivByte, void* retvalue points to an int where we
				// will store the len of the available data
				if (retvalue != 0) *(int*)retvalue = var->sqllen;
				value = var->sqldata;
			}
			else if (ivType == ivDBKey)
			{
				IBPP::DBKey* key = (IBPP::DBKey*)retvalue;
				key->SetKey(var->sqldata, var->sqllen);
				value = retvalue;
			}
			else if (ivType == ivBool)
			{
				mBools[varnum-1] = 0;
				if (var->sqllen >= 1)
				{
					char c = var->sqldata[0];
					if (c == 't' || c == 'T' || c == 'y' || c == 'Y' ||	c == '1')
						mBools[varnum-1] = 1;
				}
				value = &mBools[varnum-1];
			}
			else throw LogicExceptionImpl("RowImpl::GetValue", "Incompatible types.");
			break;

		case SQL_VARYING :
			if (ivType == ivString)
			{
				// In case of ivString, 'void* retvalue' points to a std::string where we
				// will directly store the data.
				std::string* str = (std::string*)retvalue;
				str->erase();
				str->append(var->sqldata+2, (int)*(short*)var->sqldata);
				value = retvalue;
			}
			else if (ivType == ivByte)
			{
				// In case of ivByte, void* retvalue points to an int where we
				// will store the len of the available data
				if (retvalue != 0) *(int*)retvalue = (int)*(short*)var->sqldata;
				value = var->sqldata+2;
			}
			else if (ivType == ivBool)
			{
				mBools[varnum-1] = 0;
				len = *(short*)var->sqldata;
				if (len >= 1)
				{
					char c = var->sqldata[2];
					if (c == 't' || c == 'T' || c == 'y' || c == 'Y' ||	c == '1')
						mBools[varnum-1] = 1;
				}
				value = &mBools[varnum-1];
			}
			else throw LogicExceptionImpl("RowImpl::GetValue",
										"Incompatible types.");
			break;

		case SQL_SHORT :
			if (ivType == ivShort)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"NUM/DEC with scale : use GetDouble()");
				value = var->sqldata;
			}
			else if (ivType == ivBool)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"NUM/DEC with scale : use GetDouble()");
				if (*(short*)var->sqldata == 0)
					mBools[varnum-1] = 0;
				else mBools[varnum-1] = 1;
				value = &mBools[varnum-1];
			}
			else if (ivType == ivLong)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"NUM/DEC with scale : use GetDouble()");
				mLongs[varnum-1] = *(short*)var->sqldata;
				value = &mLongs[varnum-1];
			}
			else if (ivType == ivLarge)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"NUM/DEC with scale : use GetDouble()");
				mLarges[varnum-1] = *(short*)var->sqldata;
				value = &mLarges[varnum-1];
			}
			else if (ivType == ivFloat)
			{
				// This SQL_SHORT is a NUMERIC(x,y), scale it !
				double divisor = dscales[-var->sqlscale];
				mFloats[varnum-1] = (float)(*(short*)var->sqldata / divisor);

				value = &mFloats[varnum-1];
			}
			else if (ivType == ivDouble)
			{
				// This SQL_SHORT is a NUMERIC(x,y), scale it !
				double divisor = dscales[-var->sqlscale];
				mNumerics[varnum-1] = *(short*)var->sqldata / divisor;
				value = &mNumerics[varnum-1];
			}
			else throw LogicExceptionImpl("RowImpl::GetValue", "Incompatible types.");
			break;

		case SQL_LONG :
			if (ivType == ivLong)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"NUM/DEC with scale : use GetDouble()");
				value = var->sqldata;
			}
			else if (ivType == ivBool)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"NUM/DEC with scale : use GetDouble()");
				if (*(long*)var->sqldata == 0L)
					mBools[varnum-1] = 0;
				else mBools[varnum-1] = 1;
				value = &mBools[varnum-1];
			}
			else if (ivType == ivShort)
			{
				long tmp;
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"NUM/DEC with scale : use GetDouble()");
				tmp = *(long*)var->sqldata;
				if (tmp < minshort || tmp > maxshort)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"Out of range numeric conversion !");
				mShorts[varnum-1] = (short)tmp;
				value = &mShorts[varnum-1];
			}
			else if (ivType == ivLarge)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"NUM/DEC with scale : use GetDouble()");
				mLarges[varnum-1] = *(long*)var->sqldata;
				value = &mLarges[varnum-1];
			}
			else if (ivType == ivFloat)
			{
				// This SQL_LONG is a NUMERIC(x,y), scale it !
				double divisor = dscales[-var->sqlscale];
				mFloats[varnum-1] = (float)(*(long*)var->sqldata / divisor);
				value = &mFloats[varnum-1];
			}
			else if (ivType == ivDouble)
			{
				// This SQL_LONG is a NUMERIC(x,y), scale it !
				double divisor = dscales[-var->sqlscale];
				mNumerics[varnum-1] = *(long*)var->sqldata / divisor;
				value = &mNumerics[varnum-1];
			}
			else throw LogicExceptionImpl("RowImpl::GetValue", "Incompatible types.");
			break;

		case SQL_INT64 :
			if (ivType == ivLarge)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"NUM/DEC with scale : use GetDouble()");
				value = var->sqldata;
			}
			else if (ivType == ivBool)
			{
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"NUM/DEC with scale : use GetDouble()");
				if (*(int64_t*)var->sqldata == 0)
					mBools[varnum-1] = 0;
				else mBools[varnum-1] = 1;
				value = &mBools[varnum-1];
			}
			else if (ivType == ivShort)
			{
				int64_t tmp;
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"NUM/DEC with scale : use GetDouble()");
				tmp = *(int64_t*)var->sqldata;
				if (tmp < minshort || tmp > maxshort)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"Out of range numeric conversion !");
				mShorts[varnum-1] = (short)tmp;
				value = &mShorts[varnum-1];
			}
			else if (ivType == ivLong)
			{
				int64_t tmp;
				if (var->sqlscale != 0)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"NUM/DEC with scale : use GetDouble()");
				tmp = *(int64_t*)var->sqldata;
				if (tmp < minlong || tmp > maxlong)
					throw LogicExceptionImpl("RowImpl::GetValue",
						"Out of range numeric conversion !");
				mLongs[varnum-1] = (long)tmp;
				value = &mLongs[varnum-1];
			}
			else if (ivType == ivFloat)
			{
				// This SQL_INT64 is a NUMERIC(x,y), scale it !
				double divisor = dscales[-var->sqlscale];
				mFloats[varnum-1] = (float)(*(int64_t*)var->sqldata / divisor);
				value = &mFloats[varnum-1];
			}
			else if (ivType == ivDouble)
			{
				// This SQL_INT64 is a NUMERIC(x,y), scale it !
				double divisor = dscales[-var->sqlscale];
				mNumerics[varnum-1] = *(int64_t*)var->sqldata / divisor;
				value = &mNumerics[varnum-1];
			}
			else throw LogicExceptionImpl("RowImpl::GetValue", "Incompatible types.");
			break;

		case SQL_FLOAT :
			if (ivType != ivFloat) throw LogicExceptionImpl("RowImpl::GetValue",
										"Incompatible types.");
			value = var->sqldata;
			break;

		case SQL_DOUBLE :
			if (ivType != ivDouble) throw LogicExceptionImpl("RowImpl::GetValue",
										"Incompatible types.");
			if (var->sqlscale != 0)
			{
				// Round to scale y of NUMERIC(x,y)
				double multiplier = dscales[-var->sqlscale];
				mNumerics[varnum-1] =
					floor(*(double*)var->sqldata * multiplier + 0.5) / multiplier;
				value = &mNumerics[varnum-1];
			}
			else value = var->sqldata;
			break;

		case SQL_TIMESTAMP :
			if (ivType != ivTimestamp)
				throw LogicExceptionImpl("RowImpl::SetValue", "Incompatible types.");
			{
				IBPP::Timestamp* timestamp = (IBPP::Timestamp*)retvalue;
				//timestamp->SetNative((ISC_TIMESTAMP*)var->sqldata);
				tm tms;
				(*gds.Call()->m_decode_timestamp)((ISC_TIMESTAMP*)var->sqldata, &tms);
				timestamp->SetDate(tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday);
				timestamp->SetTime(tms.tm_hour, tms.tm_min, tms.tm_sec);
				value = retvalue;
			}
			break;

		case SQL_TYPE_DATE :
			if (ivType != ivDate) throw LogicExceptionImpl("RowImpl::SetValue",
										"Incompatible types.");
			{
				IBPP::Date* date = (IBPP::Date*)retvalue;
				//date->SetNative((ISC_DATE*)var->sqldata);
				tm tms;
				(*gds.Call()->m_decode_sql_date)((ISC_DATE*)var->sqldata, &tms);
				date->SetDate(tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday);
				value = retvalue;
			}
			break;

		case SQL_TYPE_TIME :
			if (ivType != ivTime) throw LogicExceptionImpl("RowImpl::SetValue",
										"Incompatible types.");
			{
				IBPP::Time* time = (IBPP::Time*)retvalue;
				//time->SetNative((ISC_TIME*)var->sqldata);
				tm tms;
				(*gds.Call()->m_decode_sql_time)((ISC_TIME*)var->sqldata, &tms);
				time->SetTime(tms.tm_hour, tms.tm_min, tms.tm_sec);
				value = retvalue;
			}
			break;

		case SQL_BLOB :
			if (ivType == ivBlob)
			{
				BlobImpl* blob = (BlobImpl*)retvalue;
				blob->SetId((ISC_QUAD*)var->sqldata);
				value = retvalue;
			}
			else if (ivType == ivString)
			{
				BlobImpl blob(mDatabase, mTransaction);
				blob.SetId((ISC_QUAD*)var->sqldata);
				std::string* str = (std::string*)retvalue;
				blob.Load(*str);
				value = retvalue;
			}
			else throw LogicExceptionImpl("RowImpl::GetValue",
					"Incompatible types.");
			break;
			
		case SQL_ARRAY :
			if (ivType != ivArray) throw LogicExceptionImpl("RowImpl::GetValue",
										"Incompatible types.");
			{
				ArrayImpl* array = (ArrayImpl*)retvalue;
				array->SetId((ISC_QUAD*)var->sqldata);
				value = retvalue;
			}
			break;

		default : throw LogicExceptionImpl("RowImpl::GetValue",
						"Found an unknown sqltype !");
	}

	return value;
}

void RowImpl::Free(void)
{
	if (mDescrArea != 0)
	{
		for (int i = 0; i < mDescrArea->sqln; i++)
		{
			XSQLVAR* var = &(mDescrArea->sqlvar[i]);
			if (var->sqldata != 0)
			{
				switch (var->sqltype & ~1)
				{
					case SQL_ARRAY :
					case SQL_BLOB :		delete (ISC_QUAD*) var->sqldata; break;
					case SQL_TIMESTAMP :delete (ISC_TIMESTAMP*) var->sqldata; break;
					case SQL_TYPE_TIME :delete (ISC_TIME*) var->sqldata; break;
					case SQL_TYPE_DATE :delete (ISC_DATE*) var->sqldata; break;
					case SQL_TEXT :
					case SQL_VARYING :	delete [] var->sqldata; break;
					case SQL_SHORT :	delete (short*) var->sqldata; break;
					case SQL_LONG :		delete (long*) var->sqldata; break;
					case SQL_INT64 :	delete (int64_t*) var->sqldata; break;
					case SQL_FLOAT : 	delete (float*) var->sqldata; break;
					case SQL_DOUBLE :	delete (double*) var->sqldata; break;
					default : throw LogicExceptionImpl("RowImpl::Free",
								"Found an unknown sqltype !");
				}
			}
			if (var->sqlind != 0) delete var->sqlind;
		}
		delete [] (char*)mDescrArea;
		mDescrArea = 0;
	}

	mNumerics.clear();
	mFloats.clear();
	mLarges.clear();
	mLongs.clear();
	mShorts.clear();
	mBools.clear();
	mStrings.clear();
	mUpdated.clear();

	mDialect = 0;
	mDatabase = 0;
	mTransaction = 0;
}

void RowImpl::Resize(int n)
{
	const int size = XSQLDA_LENGTH(n);

	Free();
    mDescrArea = (XSQLDA*) new char[size];

	memset(mDescrArea, 0, size);
	mNumerics.resize(n);
	mFloats.resize(n);
	mLarges.resize(n);
	mLongs.resize(n);
	mShorts.resize(n);
	mBools.resize(n);
	mStrings.resize(n);
	mUpdated.resize(n);
	for (int i = 0; i < n; i++)
	{
		mNumerics[i] = 0.0;
		mFloats[i] = 0.0;
		mLarges[i] = 0;
		mLongs[i] = 0;
		mShorts[i] = 0;
		mBools[i] = 0;
		mStrings[i].erase();
		mUpdated[i] = false;
	}

	mDescrArea->version = SQLDA_VERSION1;
	mDescrArea->sqln = (short)n;
}

void RowImpl::AllocVariables(void)
{
	int i;
	for (i = 0; i < mDescrArea->sqld; i++)
	{
		XSQLVAR* var = &(mDescrArea->sqlvar[i]);
		switch (var->sqltype & ~1)
		{
			case SQL_ARRAY :
			case SQL_BLOB :		var->sqldata = (char*) new ISC_QUAD;
								memset(var->sqldata, 0, sizeof(ISC_QUAD));
								break;
			case SQL_TIMESTAMP :var->sqldata = (char*) new ISC_TIMESTAMP;
								memset(var->sqldata, 0, sizeof(ISC_TIMESTAMP));
								break;
			case SQL_TYPE_TIME :var->sqldata = (char*) new ISC_TIME;
								memset(var->sqldata, 0, sizeof(ISC_TIME));
								break;
			case SQL_TYPE_DATE :var->sqldata = (char*) new ISC_DATE;
								memset(var->sqldata, 0, sizeof(ISC_DATE));
								break;
			case SQL_TEXT :		var->sqldata = new char[var->sqllen+1];
								memset(var->sqldata, ' ', var->sqllen);
								var->sqldata[var->sqllen] = '\0';
								break;
			case SQL_VARYING :	var->sqldata = new char[var->sqllen+3];
								memset(var->sqldata, 0, 2);
								memset(var->sqldata+2, ' ', var->sqllen);
								var->sqldata[var->sqllen+2] = '\0';
								break;
			case SQL_SHORT :	var->sqldata = (char*) new short(0); break;
			case SQL_LONG :		var->sqldata = (char*) new long(0); break;
			case SQL_INT64 :	var->sqldata = (char*) new int64_t(0); break;
			case SQL_FLOAT : 	var->sqldata = (char*) new float(0.0); break;
			case SQL_DOUBLE :	var->sqldata = (char*) new double(0.0); break;
			default : throw LogicExceptionImpl("RowImpl::AllocVariables",
						"Found an unknown sqltype !");
		}
		if (var->sqltype & 1) var->sqlind = new short(-1);	// 0 indicator
	}
}

bool RowImpl::MissingValues(void)
{
	for (int i = 0; i < mDescrArea->sqld; i++)
		if (! mUpdated[i]) return true;
	return false;
}

RowImpl& RowImpl::operator=(const RowImpl& copied)
{
	Free();

	const int n = copied.mDescrArea->sqln;
	const int size = XSQLDA_LENGTH(n);

	// Initial brute copy
    mDescrArea = (XSQLDA*) new char[size];
	memcpy(mDescrArea, copied.mDescrArea, size);

	// Copy of the columns data
	for (int i = 0; i < mDescrArea->sqld; i++)
	{
		XSQLVAR* var = &(mDescrArea->sqlvar[i]);
		XSQLVAR* org = &(copied.mDescrArea->sqlvar[i]);
		switch (var->sqltype & ~1)
		{
			case SQL_ARRAY :
			case SQL_BLOB :		var->sqldata = (char*) new ISC_QUAD;
								memcpy(var->sqldata, org->sqldata, sizeof(ISC_QUAD));
								break;
			case SQL_TIMESTAMP :var->sqldata = (char*) new ISC_TIMESTAMP;
								memcpy(var->sqldata, org->sqldata, sizeof(ISC_TIMESTAMP));
								break;
			case SQL_TYPE_TIME :var->sqldata = (char*) new ISC_TIME;
								memcpy(var->sqldata, org->sqldata, sizeof(ISC_TIME));
								break;
			case SQL_TYPE_DATE :var->sqldata = (char*) new ISC_DATE;
								memcpy(var->sqldata, org->sqldata, sizeof(ISC_DATE));
								break;
			case SQL_TEXT :		var->sqldata = new char[var->sqllen+1];
								memcpy(var->sqldata, org->sqldata, var->sqllen+1);
								break;
			case SQL_VARYING :	var->sqldata = new char[var->sqllen+3];
								memcpy(var->sqldata, org->sqldata, var->sqllen+3);
								break;
			case SQL_SHORT :	var->sqldata = (char*) new short(*(short*)org->sqldata); break;
			case SQL_LONG :		var->sqldata = (char*) new long(*(long*)org->sqldata); break;
			case SQL_INT64 :	var->sqldata = (char*) new int64_t(*(int64_t*)org->sqldata); break;
			case SQL_FLOAT : 	var->sqldata = (char*) new float(*(float*)org->sqldata); break;
			case SQL_DOUBLE :	var->sqldata = (char*) new double(*(double*)org->sqldata); break;
			default : throw LogicExceptionImpl("RowImpl::Ctor",
						"Found an unknown sqltype !");
		}
		if (var->sqltype & 1) var->sqlind = new short(*org->sqlind);	// 0 indicator
	}

	// Pointers init, real data copy
	mNumerics = copied.mNumerics;
	mFloats = copied.mFloats;
	mLarges = copied.mLarges;
	mLongs = copied.mLongs;
	mShorts = copied.mShorts;
	mBools = copied.mBools;
	mStrings = copied.mStrings;

	mDialect = copied.mDialect;
	mDatabase = copied.mDatabase;
	mTransaction = copied.mTransaction;
	
	return *this;
}

RowImpl::RowImpl(const RowImpl& copied)
	: mRefCount(0), mDescrArea(0)
{
	// mRefCount and mDescrArea are set to 0 before using the assignment operator
	*this = copied;		// The assignment operator does the real copy
}

RowImpl::RowImpl(int dialect, int n, DatabaseImpl* db, TransactionImpl* tr)
	: mRefCount(0), mDescrArea(0)
{
	Resize(n);
	mDialect = dialect;
	mDatabase = db;
	mTransaction = tr;
}

RowImpl::~RowImpl()
{
	Free();
}

//
//	EOF
//
