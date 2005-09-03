///////////////////////////////////////////////////////////////////////////////
//
//	File    : $Id$
//	Subject : IBPP, Blob class implementation
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

void BlobImpl::AttachDatabase(IBPP::IDatabase* database)
{
	if (database == 0) throw LogicExceptionImpl("Blob::AttachDatabase",
			"Can't attach a NULL Database object.");

	if (mDatabase != 0) mDatabase->DetachBlob(this);
	mDatabase = dynamic_cast<DatabaseImpl*>(database);
	mDatabase->AttachBlob(this);
}

void BlobImpl::AttachTransaction(IBPP::ITransaction* transaction)
{
	if (transaction == 0) throw LogicExceptionImpl("Blob::AttachTransaction",
			"Can't attach a NULL Transaction object.");

	if (mTransaction != 0) mTransaction->DetachBlob(this);
	mTransaction = dynamic_cast<TransactionImpl*>(transaction);
	mTransaction->AttachBlob(this);
}

void BlobImpl::DetachDatabase(void)
{
	if (mDatabase == 0) throw LogicExceptionImpl("Blob::DetachDatabase",
			"No Database was attached.");

	mDatabase->DetachBlob(this);
	mDatabase = 0;
}

void BlobImpl::DetachTransaction(void)
{
	if (mTransaction == 0) throw LogicExceptionImpl("Blob::DetachTransaction",
			"No Transaction was attached.");

	mTransaction->DetachBlob(this);
	mTransaction = 0;
}

IBPP::IDatabase* BlobImpl::Database(void) const
{
	if (mDatabase == 0) throw LogicExceptionImpl("Blob::GetDatabase",
			"No Database is attached.");
	return mDatabase;
}

IBPP::ITransaction* BlobImpl::Transaction(void) const
{
	if (mTransaction == 0) throw LogicExceptionImpl("Blob::GetTransaction",
			"No Transaction is attached.");
	return mTransaction;
}

void BlobImpl::Open(void)
{
	if (mHandle != 0)
		throw LogicExceptionImpl("Blob::Open", "Blob already opened.");
	if (mDatabase == 0)
		throw LogicExceptionImpl("Blob::Open", "No Database is attached.");
	if (mTransaction == 0)
		throw LogicExceptionImpl("Blob::Open", "No Transaction is attached.");
	if (! mIdAssigned)
		throw LogicExceptionImpl("Blob::Open", "Blob Id is not assigned.");

	IBS status;
	(*gds.Call()->m_open_blob2)(status.Self(), mDatabase->GetHandlePtr(),
		mTransaction->GetHandlePtr(), &mHandle, &mId, 0, 0);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Open", "isc_open_blob2 failed.");
	mWriteMode = false;
}

void BlobImpl::Create(void)
{
	if (mHandle != 0)
		throw LogicExceptionImpl("Blob::Create", "Blob already opened.");
	if (mDatabase == 0)
		throw LogicExceptionImpl("Blob::Create", "No Database is attached.");
	if (mTransaction == 0)
		throw LogicExceptionImpl("Blob::Create", "No Transaction is attached.");

	IBS status;
	(*gds.Call()->m_create_blob2)(status.Self(), mDatabase->GetHandlePtr(),
		mTransaction->GetHandlePtr(), &mHandle, &mId, 0, 0);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Create",
			"isc_create_blob failed.");
	mIdAssigned = true;
	mWriteMode = true;
}

void BlobImpl::Close(void)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Blob::Close", "The Blob is not opened");

	IBS status;
	(*gds.Call()->m_close_blob)(status.Self(), &mHandle);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Close", "isc_close_blob failed.");
	mHandle = 0;
}

void BlobImpl::Cancel(void)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Blob::Cancel", "The Blob is not opened");
	if (! mWriteMode)
		throw LogicExceptionImpl("Blob::Cancel", "Can't cancel a Blob opened for read");

	IBS status;
	(*gds.Call()->m_cancel_blob)(status.Self(), &mHandle);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Cancel", "isc_cancel_blob failed.");
	mHandle = 0;
	mIdAssigned = false;
}

int BlobImpl::Read(void* buffer, int size)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Blob::Read", "The Blob is not opened");
	if (mWriteMode)
		throw LogicExceptionImpl("Blob::Read", "Can't read from Blob opened for write");
	if (size < 1 || size > (64*1024-1))
		throw LogicExceptionImpl("Blob::Read", "Invalid segment size (max 64Kb-1)");

	IBS status;
	unsigned short bytesread;
	int result = (*gds.Call()->m_get_segment)(status.Self(), &mHandle, &bytesread,
					(unsigned short)size, (char*)buffer);
	if (result == isc_segstr_eof) return 0;	// Fin du blob
	if (result != isc_segment && status.Errors())
		throw SQLExceptionImpl(status, "Blob::Read", "isc_get_segment failed.");
	return (int)bytesread;
}

void BlobImpl::Write(const void* buffer, int size)
{
	if (mHandle == 0)
		throw LogicExceptionImpl("Blob::Write", "The Blob is not opened");
	if (! mWriteMode)
		throw LogicExceptionImpl("Blob::Write", "Can't write to Blob opened for read");
	if (size < 1 || size > (64*1024-1))
		throw LogicExceptionImpl("Blob::Write", "Invalid segment size (max 64Kb-1)");

	IBS status;
	(*gds.Call()->m_put_segment)(status.Self(), &mHandle,
		(unsigned short)size, (char*)buffer);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Write", "isc_put_segment failed.");
}

void BlobImpl::Info(int* Size, int* Largest, int* Segments)
{
	char items[] = {isc_info_blob_total_length,
					isc_info_blob_max_segment,
					isc_info_blob_num_segments};

	if (mHandle == 0)
		throw LogicExceptionImpl("Blob::GetInfo", "The Blob is not opened");

	IBS status;
	RB result(100);
	(*gds.Call()->m_blob_info)(status.Self(), &mHandle, sizeof(items), items,
		result.Size(), result.Self());
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::GetInfo", "isc_blob_info failed.");

	if (Size != 0) *Size = result.GetValue(isc_info_blob_total_length);
	if (Largest != 0) *Largest = result.GetValue(isc_info_blob_max_segment);
	if (Segments != 0) *Segments = result.GetValue(isc_info_blob_num_segments);
}

void BlobImpl::Save(const std::string& data)
{
	if (mHandle != 0)
		throw LogicExceptionImpl("Blob::Save", "Blob already opened.");
	if (mDatabase == 0)
		throw LogicExceptionImpl("Blob::Save", "No Database is attached.");
	if (mTransaction == 0)
		throw LogicExceptionImpl("Blob::Save", "No Transaction is attached.");

	IBS status;
	(*gds.Call()->m_create_blob2)(status.Self(), mDatabase->GetHandlePtr(),
		mTransaction->GetHandlePtr(), &mHandle, &mId, 0, 0);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Save",
			"isc_create_blob failed.");
	mIdAssigned = true;
	mWriteMode = true;

	size_t pos = 0;
	size_t len = data.size();
	while (len != 0)
	{
		size_t blklen = (len < 32*1024-1) ? len : 32*1024-1;
		status.Reset();
		(*gds.Call()->m_put_segment)(status.Self(), &mHandle,
			(unsigned short)blklen, const_cast<char*>(data.data()+pos));
		if (status.Errors())
			throw SQLExceptionImpl(status, "Blob::Save",
					"isc_put_segment failed.");
		pos += blklen;
		len -= blklen;
	}
	
	status.Reset();
	(*gds.Call()->m_close_blob)(status.Self(), &mHandle);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Save", "isc_close_blob failed.");
	mHandle = 0;
}

void BlobImpl::Load(std::string& data)
{
	if (mHandle != 0)
		throw LogicExceptionImpl("Blob::Load", "Blob already opened.");
	if (mDatabase == 0)
		throw LogicExceptionImpl("Blob::Load", "No Database is attached.");
	if (mTransaction == 0)
		throw LogicExceptionImpl("Blob::Load", "No Transaction is attached.");
	if (! mIdAssigned)
		throw LogicExceptionImpl("Blob::Load", "Blob Id is not assigned.");

	IBS status;
	(*gds.Call()->m_open_blob2)(status.Self(), mDatabase->GetHandlePtr(),
		mTransaction->GetHandlePtr(), &mHandle, &mId, 0, 0);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Load", "isc_open_blob2 failed.");
	mWriteMode = false;

	size_t blklen = 32*1024-1;
	data.resize(blklen);

	size_t size = 0;
	size_t pos = 0;
	for (;;)
	{
		status.Reset();
		unsigned short bytesread;
		int result = (*gds.Call()->m_get_segment)(status.Self(), &mHandle,
						&bytesread, (unsigned short)blklen,
							const_cast<char*>(data.data()+pos));
		if (result == isc_segstr_eof) break;	// End of blob
		if (result != isc_segment && status.Errors())
			throw SQLExceptionImpl(status, "Blob::Load", "isc_get_segment failed.");

		pos += bytesread;
		size += bytesread;
		data.resize(size + blklen);
	}
	data.resize(size);
	
	status.Reset();
	(*gds.Call()->m_close_blob)(status.Self(), &mHandle);
	if (status.Errors())
		throw SQLExceptionImpl(status, "Blob::Load", "isc_close_blob failed.");
	mHandle = 0;
}

IBPP::IBlob* BlobImpl::AddRef(void)
{
	ASSERTION(mRefCount >= 0);
	++mRefCount;
	return this;
}

void BlobImpl::Release(IBPP::IBlob*& Self)
{
	if (this != dynamic_cast<BlobImpl*>(Self))
		throw LogicExceptionImpl("Blob::Release", "Invalid Release()");

	ASSERTION(mRefCount >= 0);

	--mRefCount;
	if (mRefCount <= 0) delete this;
	Self = 0;
}

//	(((((((( OBJECT INTERNAL METHODS ))))))))

void BlobImpl::Init(void)
{
	mIdAssigned = false;
	mWriteMode = false;
	mHandle = 0;
	mDatabase = 0;
	mTransaction = 0;
}

void BlobImpl::SetId(ISC_QUAD* quad)
{
	if (mHandle != 0)
		throw LogicExceptionImpl("BlobImpl::SetId", "Can't set Id on an opened BlobImpl.");
	if (quad == 0)
		throw LogicExceptionImpl("BlobImpl::SetId", "0 Id reference detected.");

	memcpy(&mId, quad, sizeof(mId));
	mIdAssigned = true;
}

void BlobImpl::GetId(ISC_QUAD* quad)
{
	if (mHandle != 0)
		throw LogicExceptionImpl("BlobImpl::GetId", "Can't get Id on an opened BlobImpl.");
	if (! mWriteMode)
		throw LogicExceptionImpl("BlobImpl::GetId", "Can only get Id of a newly created Blob.");
	if (quad == 0)
		throw LogicExceptionImpl("BlobImpl::GetId", "0 Id reference detected.");

	memcpy(quad, &mId, sizeof(mId));
}

BlobImpl::BlobImpl(DatabaseImpl* database, TransactionImpl* transaction)
	: mRefCount(0)
{
	Init();
	AttachDatabase(database);
	if (transaction != 0) AttachTransaction(transaction);
}

BlobImpl::~BlobImpl()
{
	if (mHandle != 0)
	{
		if (mWriteMode) Cancel();
		else try {Close();} catch (IBPP::Exception&) {}
	}
	if (mTransaction != 0) mTransaction->DetachBlob(this);
	if (mDatabase != 0) mDatabase->DetachBlob(this);
}

//
//	EOF
//
