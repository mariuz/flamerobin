///////////////////////////////////////////////////////////////////////////////
//
//	File    : $Id$
//	Subject : IBPP, Database class implementation
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

#include <algorithm>

using namespace ibpp_internals;

//	(((((((( OBJECT INTERFACE IMPLEMENTATION ))))))))

void DatabaseImpl::Create(int32_t dialect)
{
	if (mHandle != 0)
		throw LogicExceptionImpl("Database::Create", _("Database is already connected."));
	if (mDatabaseName.empty())
		throw LogicExceptionImpl("Database::Create", _("Unspecified database name."));
	if (mUserName.empty())
		throw LogicExceptionImpl("Database::Create", _("Unspecified user name."));
	if (dialect != 1 && dialect != 3)
		throw LogicExceptionImpl("Database::Create", _("Only dialects 1 and 3 are supported."));

	// Build the SQL Create Statement
	std::string create;
	create.assign("CREATE DATABASE '");
	if (! mServerName.empty()) create.append(mServerName).append(":");
	create.append(mDatabaseName).append("' ");

	create.append("USER '").append(mUserName).append("' ");
	if (! mUserPassword.empty())
		create.append("PASSWORD '").append(mUserPassword).append("' ");

	if (! mCreateParams.empty()) create.append(mCreateParams);

	// Call ExecuteImmediate to create the database
	isc_tr_handle tr_handle = 0;
	IBS status;
    (*gds.Call()->m_dsql_execute_immediate)(status.Self(), &mHandle, &tr_handle,
    	0, const_cast<char*>(create.c_str()), short(dialect), NULL);
    if (status.Errors())
		throw SQLExceptionImpl(status, "Database::Create", _("isc_dsql_execute_immediate failed"));

	Disconnect();
}

void DatabaseImpl::Connect(void)
{
	if (mHandle != 0) return;	// Already connected

	if (mDatabaseName.empty())
		throw LogicExceptionImpl("Database::Connect", _("Unspecified database name."));
	if (mUserName.empty())
		throw LogicExceptionImpl("Database::Connect", _("Unspecified user name."));

    // Build a DPB based on the properties
	DPB dpb;
    dpb.Insert(isc_dpb_user_name, mUserName.c_str());
    dpb.Insert(isc_dpb_password, mUserPassword.c_str());
    if (! mRoleName.empty()) dpb.Insert(isc_dpb_sql_role_name, mRoleName.c_str());
    if (! mCharSet.empty()) dpb.Insert(isc_dpb_lc_ctype, mCharSet.c_str());

	std::string connect;
	if (! mServerName.empty())
		connect.assign(mServerName).append(":");
	connect.append(mDatabaseName);

	IBS status;
	(*gds.Call()->m_attach_database)(status.Self(), (short)connect.size(),
		const_cast<char*>(connect.c_str()), &mHandle, dpb.Size(), dpb.Self());
    if (status.Errors())
    {
        mHandle = 0;     // Should be, but better be sure...
		throw SQLExceptionImpl(status, "Database::Connect", _("isc_attach_database failed"));
    }

	// Now, get ODS version information and dialect.
	// If ODS major is lower of equal to 9, we reject the connection.
	// If ODS major is 10 or higher, this is at least an InterBase 6.x Server
	// OR FireBird 1.x Server.

	char items[] = {isc_info_ods_version,
					isc_info_db_SQL_dialect,
					isc_info_end};
	RB result(100);

	status.Reset();
	(*gds.Call()->m_database_info)(status.Self(), &mHandle, sizeof(items), items,
		result.Size(), result.Self());
	if (status.Errors())
	{
		status.Reset();
	    (*gds.Call()->m_detach_database)(status.Self(), &mHandle);
        mHandle = 0;     // Should be, but better be sure...
		throw SQLExceptionImpl(status, "Database::Connect", _("isc_database_info failed"));
	}

	int ODS = result.GetValue(isc_info_ods_version);
	if (ODS <= 9)
	{
		status.Reset();
	    (*gds.Call()->m_detach_database)(status.Self(), &mHandle);
        mHandle = 0;     // Should be, but better be sure...
		throw LogicExceptionImpl("Database::Connect",
			_("Unsupported Server : wrong ODS version (%d), at least '10' required."), ODS);
	}

	mDialect = result.GetValue(isc_info_db_SQL_dialect);
	if (mDialect != 1 && mDialect != 3)
	{
		status.Reset();
	    (*gds.Call()->m_detach_database)(status.Self(), &mHandle);
        mHandle = 0;     // Should be, but better be sure...
		throw LogicExceptionImpl("Database::Connect", _("Dialect 1 or 3 required"));
	}

	// Now, verify the GDS32.DLL we are using is compatible with the server
	if (ODS >= 10 && gds.Call()->mGDSVersion < 60)
	{
		status.Reset();
	    (*gds.Call()->m_detach_database)(status.Self(), &mHandle);
        mHandle = 0;     // Should be, but better be sure...
		throw LogicExceptionImpl("Database::Connect", _("GDS32.DLL version 5 against IBSERVER 6"));
	}
}

void DatabaseImpl::Inactivate(void)
{
	if (mHandle == 0) return;	// Not connected anyway

    IBS status;

	// Cancel all pending event traps
	ClearEvents();

    // Rollback any started transaction...
	for (unsigned i = 0; i < mTransactions.size(); i++)
	{
		if (mTransactions[i]->Started())
				mTransactions[i]->Rollback();
	}

	// Let's detach from all Blobs
	while (mBlobs.size() > 0)
		mBlobs.back()->DetachDatabase();

	// Let's detach from all Arrays
	while (mArrays.size() > 0)
		mArrays.back()->DetachDatabase();

	// Let's detach from all Statements
	while (mStatements.size() > 0)
		mStatements.back()->DetachDatabase();

	// Let's detach from all Transactions
	while (mTransactions.size() > 0)
		mTransactions.back()->DetachDatabase(this);
}

void DatabaseImpl::Disconnect(void)
{
	if (mHandle == 0) return;	// Not connected anyway

	// Put the connection to rest
	Inactivate();

	// Detach from the server
	IBS status;
    (*gds.Call()->m_detach_database)(status.Self(), &mHandle);

    // Should we throw, set mHandle to 0 first, because Disconnect() may
	// be called from Database destructor (keeps the object coherent).
	mHandle = 0;
    if (status.Errors())
		throw SQLExceptionImpl(status, "Database::Disconnect", _("isc_detach_database failed"));
}

void DatabaseImpl::Drop(void)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Database::Drop", _("Database must be connected."));

	// Put the connection to a rest
	Inactivate();

	IBS vector;
	(*gds.Call()->m_drop_database)(vector.Self(), &mHandle);
    if (vector.Errors())
    	throw SQLExceptionImpl(vector, "Database::Drop", _("isc_drop_database failed"));

    mHandle = 0;
}

void DatabaseImpl::DefineEvent(const std::string& eventname, IBPP::EventInterface* objref)
{
	if (eventname.empty())
		throw LogicExceptionImpl("Database::DefineEvent", _("Null pointer reference detected."));

	if (mEventsThrew)
		throw LogicExceptionImpl("Database::DefineEvent", _("An error condition was "
							"detected by the asynchronous EventHandler() method."));

	if (mEvents == 0) mEvents = new EPB;
	else CancelEvents();

	mEvents->Define(eventname, objref);
	QueueEvents();
}

void DatabaseImpl::ClearEvents(void)
{
	CancelEvents();

	if (mEvents != 0)
	{
		delete mEvents;
		mEvents = 0;
		mEventsId = 0;
	}
}

void DatabaseImpl::DispatchEvents(void)
{
	// If no events registered, nothing to do of course.
	// If we are still waiting for some events to fire, nothing to do, too.
	if (mEvents == 0 || mEventsQueued) return;

	if (mHandle == 0)
		throw LogicExceptionImpl("Database::DispatchEvents", _("Database is not connected."));

	if (mEventsThrew)
		throw LogicExceptionImpl("Database::DispatchEvents", _("An error condition was "
							"detected by the asynchronous EventHandler() method."));
	
	// Let's fire the events actions for all the events which triggered, if any.
	if (mEventsTrapped) mEvents->FireActions(this);

	// Requeue the events
	QueueEvents();
}

void DatabaseImpl::Info(int32_t* ODSMajor, int32_t* ODSMinor,
	int32_t* PageSize, int32_t* Pages, int32_t* Buffers, int32_t* Sweep,
	bool* Sync, bool* Reserve)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Database::Info", _("Database is not connected."));

	char items[] = {isc_info_ods_version,
					isc_info_ods_minor_version,
					isc_info_page_size,
					isc_info_allocation,
					isc_info_num_buffers,
					isc_info_sweep_interval,
					isc_info_forced_writes,
					isc_info_no_reserve,
					isc_info_end};
    IBS status;
	RB result(256);

	status.Reset();
	(*gds.Call()->m_database_info)(status.Self(), &mHandle, sizeof(items), items,
		result.Size(), result.Self());
	if (status.Errors())
		throw SQLExceptionImpl(status, "Database::Info", _("isc_database_info failed"));

	if (ODSMajor != 0) *ODSMajor = result.GetValue(isc_info_ods_version);
	if (ODSMinor != 0) *ODSMinor = result.GetValue(isc_info_ods_minor_version);
	if (PageSize != 0) *PageSize = result.GetValue(isc_info_page_size);
	if (Pages != 0) *Pages = result.GetValue(isc_info_allocation);
	if (Buffers != 0) *Buffers = result.GetValue(isc_info_num_buffers);
	if (Sweep != 0) *Sweep = result.GetValue(isc_info_sweep_interval);
	if (Sync != 0)
		*Sync = result.GetValue(isc_info_forced_writes) == 1 ? true : false;
	if (Reserve != 0)
		*Reserve = result.GetValue(isc_info_no_reserve) == 1 ? false : true;
}

void DatabaseImpl::Statistics(int32_t* Fetches, int32_t* Marks, int32_t* Reads, int32_t* Writes)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Database::Statistics", _("Database is not connected."));

	char items[] = {isc_info_fetches,
					isc_info_marks,
					isc_info_reads,
					isc_info_writes,
					isc_info_end};
    IBS status;
	RB result(128);

	status.Reset();
	(*gds.Call()->m_database_info)(status.Self(), &mHandle, sizeof(items), items,
		result.Size(), result.Self());
	if (status.Errors())
		throw SQLExceptionImpl(status, "Database::Statistics", _("isc_database_info failed"));

	if (Fetches != 0) *Fetches = result.GetValue(isc_info_fetches);
	if (Marks != 0) *Marks = result.GetValue(isc_info_marks);
	if (Reads != 0) *Reads = result.GetValue(isc_info_reads);
	if (Writes != 0) *Writes = result.GetValue(isc_info_writes);
}

void DatabaseImpl::Counts(int32_t* Insert, int32_t* Update, int32_t* Delete, 
	int32_t* ReadIdx, int32_t* ReadSeq)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Database::Counts", _("Database is not connected."));

	char items[] = {isc_info_insert_count,
					isc_info_update_count,
					isc_info_delete_count,
					isc_info_read_idx_count,
					isc_info_read_seq_count,
					isc_info_end};
    IBS status;
	RB result(1024);

	status.Reset();
	(*gds.Call()->m_database_info)(status.Self(), &mHandle, sizeof(items), items,
		result.Size(), result.Self());
	if (status.Errors())
		throw SQLExceptionImpl(status, "Database::Counts", _("isc_database_info failed"));

	if (Insert != 0) *Insert = result.GetCountValue(isc_info_insert_count);
	if (Update != 0) *Update = result.GetCountValue(isc_info_update_count);
	if (Delete != 0) *Delete = result.GetCountValue(isc_info_delete_count);
	if (ReadIdx != 0) *ReadIdx = result.GetCountValue(isc_info_read_idx_count);
	if (ReadSeq != 0) *ReadSeq = result.GetCountValue(isc_info_read_seq_count);
}

void DatabaseImpl::Users(std::vector<std::string>& users)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Database::Users", _("Database is not connected."));

	char items[] = {isc_info_user_names,
					isc_info_end};
    IBS status;
	RB result(8000);

	status.Reset();
	(*gds.Call()->m_database_info)(status.Self(), &mHandle, sizeof(items), items,
		result.Size(), result.Self());
	if (status.Errors())
	{
		status.Reset();
		throw SQLExceptionImpl(status, "Database::Users", _("isc_database_info failed"));
	}

	users.clear();
	char* p = result.Self();
	while (*p == isc_info_user_names)
	{
		p += 3;		// Get to the length byte (there are two undocumented bytes which we skip)
		int len = (int)(*p);
		++p;		// Get to the first char of username
    	if (len != 0) users.push_back(std::string().append(p, len));
   		p += len;	// Skip username
    }
	return;
}

IBPP::IDatabase* DatabaseImpl::AddRef(void)
{
	ASSERTION(mRefCount >= 0);
	++mRefCount;
	return this;
}

void DatabaseImpl::Release(IBPP::IDatabase*& Self)
{
	if (this != dynamic_cast<DatabaseImpl*>(Self))
		throw LogicExceptionImpl("Database::Release", _("Invalid Release()"));

	ASSERTION(mRefCount >= 0);

	--mRefCount;
	if (mRefCount <= 0) delete this;
	Self = 0;
}

//	(((((((( OBJECT INTERNAL METHODS ))))))))

void DatabaseImpl::AttachTransaction(TransactionImpl* tr)
{
	if (tr == 0)
		throw LogicExceptionImpl("Database::AttachTransaction",
					_("Transaction object is null."));

	mTransactions.push_back(tr);
}

void DatabaseImpl::DetachTransaction(TransactionImpl* tr)
{
	if (tr == 0)
		throw LogicExceptionImpl("Database::DetachTransaction",
				_("ITransaction object is null."));

	mTransactions.erase(std::find(mTransactions.begin(), mTransactions.end(), tr));
}

void DatabaseImpl::AttachStatement(StatementImpl* st)
{
	if (st == 0)
		throw LogicExceptionImpl("Database::AttachStatement",
					_("Can't attach a null Statement object."));

	mStatements.push_back(st);
}

void DatabaseImpl::DetachStatement(StatementImpl* st)
{
	if (st == 0)
		throw LogicExceptionImpl("Database::DetachStatement",
				_("Can't detach a null Statement object."));

	mStatements.erase(std::find(mStatements.begin(), mStatements.end(), st));
}

void DatabaseImpl::AttachBlob(BlobImpl* bb)
{
	if (bb == 0)
		throw LogicExceptionImpl("Database::AttachBlob",
					_("Can't attach a null Blob object."));

	mBlobs.push_back(bb);
}

void DatabaseImpl::DetachBlob(BlobImpl* bb)
{
	if (bb == 0)
		throw LogicExceptionImpl("Database::DetachBlob",
				_("Can't detach a null Blob object."));

	mBlobs.erase(std::find(mBlobs.begin(), mBlobs.end(), bb));
}

void DatabaseImpl::AttachArray(ArrayImpl* ar)
{
	if (ar == 0)
		throw LogicExceptionImpl("Database::AttachArray",
					_("Can't attach a null Array object."));

	mArrays.push_back(ar);
}

void DatabaseImpl::DetachArray(ArrayImpl* ar)
{
	if (ar == 0)
		throw LogicExceptionImpl("Database::DetachArray",
				_("Can't detach a null Array object."));

	mArrays.erase(std::find(mArrays.begin(), mArrays.end(), ar));
}

void DatabaseImpl::QueueEvents(void)
{
	if (mEvents != 0 && (!mEventsQueued))
	{
		if (mHandle == 0)
			throw LogicExceptionImpl("Database::QueueEvents",
				  _("Database is not connected"));

		IBS vector;
		mEventsTrapped = false;
		mEventsQueued = true;
		(*gds.Call()->m_que_events)(vector.Self(), &mHandle, &mEventsId,
			short(mEvents->Size()), mEvents->EventsBuffer(),
				(isc_callback)EventHandler, (char*)this);

		if (vector.Errors())
		{
			mEventsId = 0;	// Should be, but better be safe
			mEventsQueued = false;
			throw SQLExceptionImpl(vector, "Database::QueueEvents",
				_("isc_que_events failed"));
		}
	}
}

void DatabaseImpl::CancelEvents(void)
{
	if (mEvents != 0 && mEventsQueued)
	{
		if (mHandle == 0) throw LogicExceptionImpl("Database::CancelEvents",
			_("Database is not connected"));

		IBS vector;

		// A call to cancel_events will call *once* the handler routine, even
		// though no events had fired.
		(*gds.Call()->m_cancel_events)(vector.Self(), &mHandle, &mEventsId);

	    if (vector.Errors())
	    	throw SQLExceptionImpl(vector, "Database::CancelEvents",
	    		_("isc_cancel_events failed"));

		mEventsId = 0;	// Should be, but better be safe
		mEventsQueued = false;
		mEventsThrew = false;	// Reset potential error condition
	}
}

void DatabaseImpl::EventUpdateCounts(int32_t size, const char* tmpbuffer)
{
	if (size > mEvents->Size())
	{
		mEventsThrew = true;	// Take note. Will throw from another context.
		return;
	}

	for (int i = 0; i < size; i++)
		mEvents->ResultsBuffer()[i] = tmpbuffer[i];

	mEventsTrapped = true;
}

// This function must keep this prototype to stay compatible with
// what isc_que_events() expect
void DatabaseImpl::EventHandler(const char* object,
	short size, const char* tmpbuffer)
{
	// >>>>> This method is a STATIC member !! <<<<<
	// Consider this method as a kind of "interrupt handler". It should do as
	// few work as possible as quickly as possible and then return.

	DatabaseImpl* db = (DatabaseImpl*)object;	// Ugly, but wanted, c-style cast

	if (size != 0 && tmpbuffer != 0)
		db->EventUpdateCounts(size, tmpbuffer);

	db->mEventsQueued = false;
}

DatabaseImpl::DatabaseImpl(const std::string& ServerName, const std::string& DatabaseName,
						   const std::string& UserName, const std::string& UserPassword,
						   const std::string& RoleName, const std::string& CharSet,
						   const std::string& CreateParams) :

	mRefCount(0), mHandle(0),
	mServerName(ServerName), mDatabaseName(DatabaseName),
	mUserName(UserName), mUserPassword(UserPassword), mRoleName(RoleName),
	mCharSet(CharSet), mCreateParams(CreateParams),
	mDialect(3), mEvents(0), mEventsId(0), mEventsQueued(false),
	mEventsTrapped(false), mEventsThrew(false)
{
}

DatabaseImpl::~DatabaseImpl()
{
	if (Connected())
		try { Disconnect(); } catch(IBPP::Exception&) { }
}

//
//	EOF
//
