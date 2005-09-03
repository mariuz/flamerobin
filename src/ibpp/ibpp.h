///////////////////////////////////////////////////////////////////////////////
//
//	File    : $Id$
//	Subject : IBPP public header file. This is _the_ file you include in your
//			  application files when developing with IBPP.
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
//	Contributor(s) to and since version 2.0 :
//
//		Olivier Mascia, main coding
//		Matt Hortman, initial linux port
//		Mark Jordan, design contributions
//		Maxim Abrashkin, enhancement patches
//		Torsten Martinsen, enhancement patches
//		Michael Hieke, darwin (OS X) port, enhancement patches
//		Val Samko, enhancement patches and debugging
//		Mike Nordell, invaluable C++ advices
//		Claudio Valderrama, help with not-so-well documented IB/FB features
//		Many others, excellent suggestions, bug finding, and support
//
///////////////////////////////////////////////////////////////////////////////
//
//	COMMENTS
//	Tabulations should be set every four characters when editing this file.
//
//	When compiling an IBPP project (or IBPP library itself), the following
//	defines should be made on the command-line (or in makefiles) according
//	to the OS platform and compiler used.
//
//	Select the platform:	IBPP_WINDOWS | IBPP_LINUX | IBPP_DARWIN
//	Select the compiler:	IBPP_BCC | IBPP_GCC | IBPP_MSVC | IBPP_DMC
//
//	See the documentation and makefiles for more information.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __IBPP_H__
#define __IBPP_H__

#if !defined(IBPP_WINDOWS) && !defined(IBPP_LINUX) && !defined(IBPP_DARWIN)
#error Please define IBPP_WINDOWS/IBPP_LINUX/IBPP_DARWIN before compiling !
#endif

#if !defined(IBPP_BCC) && !defined(IBPP_GCC) \
	&& !defined(IBPP_MSVC) && !defined(IBPP_DMC)
#error Please define IBPP_BCC/IBPP_GCC/IBPP_MSVC/IBPP_DMC before compiling !
#endif

#if defined(IBPP_LINUX) || defined(IBPP_DARWIN)
#define IBPP_UNIX	// IBPP_UNIX stands as a common denominator to *NIX flavours
#endif

#if defined(IBPP_MSVC)
typedef __int64 int64_t;
#else
#include <stdint.h>			// C99 (§7.18) integer types definitions
#endif

#include <exception>
#include <string>
#include <vector>

namespace IBPP
{
	//	Typically you use this constant in a call IBPP::CheckVersion as in:
	//	if (! IBPP::CheckVersion(IBPP::Version)) { throw .... ; }
	const unsigned int Version = 0x02040001; // Version == 2.4.0.1

	//	Dates range checking
	const int MinDate = -693594;	//  1 JAN 0001
	const int MaxDate = 2958464;	// 31 DEC 9999
	
	//	Transaction Access Modes
	enum TAM {amWrite, amRead};

	//	Transaction Isolation Levels
	enum TIL {ilConcurrency, ilReadDirty, ilReadCommitted, ilConsistency};

	//	Transaction Lock Resolution
	enum TLR {lrWait, lrNoWait};

	// Transaction Table Reservation
	enum TTR {trSharedWrite, trSharedRead, trProtectedWrite, trProtectedRead};

	//	Prepared Statement Types
	enum STT {stUnknown, stUnsupported,
		stSelect, stInsert, stUpdate, stDelete,	stDDL, stExecProcedure,
		stSelectUpdate, stOther};

	//	SQL Data Types
	enum SDT {sdArray, sdBlob, sdDate, sdTime, sdTimestamp, sdString,
		sdSmallint, sdInteger, sdLargeint, sdFloat, sdDouble};

	//	Array Data Types
	enum ADT {adDate, adTime, adTimestamp, adString,
		adBool, adShort, adInt, adInt64, adFloat, adDouble};

	// Database::Shutdown Modes
	enum DSM {dsForce, dsDenyTrans, dsDenyAttach};

	// Service::StartBackup && Service::StartRestore Flags
	enum BRF {
		brVerbose = 0x1,
		// Backup flags
		brIgnoreChecksums = 0x100, brIgnoreLimbo = 0x200,
		brMetadataOnly = 0x400, brNoGarbageCollect = 0x800,
		brNonTransportable = 0x1000, brConvertExtTables = 0x2000,
		// Restore flags
		brReplace = 0x10000, brDeactivateIdx = 0x20000,
		brNoShadow = 0x40000, brNoValidity = 0x80000,
		brPerTableCommit = 0x100000, brUseAllSpace = 0x200000
	};

	// Service::Repair Flags
	enum RPF
	{
		// Mandatory and mutually exclusives
		rpMendRecords = 0x1, rpValidatePages = 0x2, rpValidateFull = 0x4,
		// Options
		rpReadOnly = 0x100, rpIgnoreChecksums = 0x200, rpKillShadows = 0x400
	};

	// TransactionFactory Flags
	enum TFF {tfIgnoreLimbo = 0x1, tfAutoCommit = 0x2, tfNoAutoUndo = 0x4};

	//	Interface Wrapper
	template <class T>
	class Ptr
	{
	private:
		T* mObject;

	public:
		void clear()			{ if (mObject != 0) mObject->Release(mObject); }
		T* intf() const			{ return mObject; }
		T* operator->() const	{ return mObject; }
		Ptr& operator=(T* p)
		{
			// AddRef _before_ Release gives correct behaviour on self-assigns
			T* tmp = (p == 0 ? 0 : p->AddRef());	// Take care of 0
			if (mObject != 0) mObject->Release(mObject);
			mObject = tmp; return *this;
		}
		Ptr& operator=(const Ptr& r)
		{
			// AddRef _before_ Release gives correct behaviour on self-assigns
			T* tmp = (r.intf() == 0 ? 0 : r->AddRef());// Take care of 0
			if (mObject != 0) mObject->Release(mObject);
			mObject = tmp; return *this;
		}
		Ptr(T* p)			{ mObject = (p == 0 ? 0 : p->AddRef()); }
		Ptr(const Ptr& r)	{ mObject = (r.intf() == 0 ? 0 : r->AddRef()); }

		Ptr() : mObject(0) { }
		~Ptr() { clear(); }
	};

	//	Some forward declarations to keep the compiler happy
	class IDatabase;
	class ITransaction;
	class IStatement;
	class EventInterface;
	class Timestamp;

	/* Classes Date, Time, Timestamp and DBKey are 'helper' classes.  They help
	 * in retrieving or setting some special SQL types. Dates, times and dbkeys
	 * are often read and written as strings in SQL scripts. When programming
	 * with IBPP, we handle those data with these specific classes, which
	 * enhance their usefullness and free us of format problems (M/D/Y, D/M/Y,
	 * Y-M-D ?, and so on...). */

	/* Class Date represent purely a Date (no time part specified). It is
	 * usefull in interactions with the SQL DATE type of Interbase.  You can add
	 * or substract a number from a Date, that will modify it to represent the
	 * correct date, X days later or sooner. All the Y2K details taken into
	 * account. The assignment operator and conversion operator that deal with
	 * 'int' type are used to assign and extract the date as a stand alone
	 * integer number. This number represents the date as the number of days
	 * elapsed since 31 Dec 1899. So 1 is 1 Jan 1900, 2 is 2 Jan 1900 and so
	 * on...  See the IBPP::dtoi and IBPP::itod methods. The full range goes
	 * from integer values IBPP::MinDate to IBPP::MaxDate which means from 01
	 * Jan 0001 to 31 Dec 9999. Which is inherently incorrect as this assumes
	 * Gregorian calendar. */

	class Date
	{
	protected:
		int mDate;	// The date as an integer : 1 == 1 Jan 1900

	public:
		void Clear(void) { mDate = MinDate - 1; };
		void Today(void);
		void SetDate(int year, int month, int day);
		void GetDate(int& year, int& month, int& day) const;
		void Add(int days);
		void StartOfMonth(void);
		void EndOfMonth(void);
	
		Date() { Clear(); };
		Date(int year, int month, int day);
		Date(const Date&);					// Copy Constructor
		Date(const int&);
		Date& operator=(const Timestamp&);	// Timestamp Assignment operator
		Date& operator=(const Date&);		// Date Assignment operator
		Date& operator=(const int&);		// int Assignment operator
		operator int() const { return mDate; }	// int Conversion operator
		~Date() { };
	};

	/* Class Time represent purely a Time (H:M:S). It is usefull in interactions
	 * with the SQL TIME type of Interbase. The assignment operator and
	 * conversion operator that deal with 'int' type are used to assign and
	 * extract the time as a stand alone integer number. This number represents
	 * the time as the number of seconds elapsed since midnight. See the
	 * IBPP::ttoi and IBPP::itot methods. */

	class Time
	{
	protected:
		int mTime;		// The time as an integer, seconds since midnight

	public:
		void Clear(void) { mTime = 0; }
		void Now(void);
		void SetTime(int hour, int minute, int second);
		void GetTime(int& hour, int& minute, int& second) const;

		Time() { Clear(); }
		Time(int hour, int minute, int second);
		Time(const Time&);					// Copy Constructor
		Time(const int&);
		Time& operator=(const Timestamp&);	// Timestamp Assignment operator
		Time& operator=(const Time&);		// Time Assignment operator
		Time& operator=(const int&);		// int Assignment operator
		operator int() const { return mTime; }	// int Conversion operator
		~Time() { };
	};

	/* Class Timestamp represent a date AND a time. It is usefull in
	 * interactions with the SQL TIMESTAMP type of Interbase. This class
	 * inherits from Date and Time and completely inline implements its small
	 * specific details. */

	class Timestamp : public Date, public Time
	{
	public:
		void Clear(void)	{ Date::Clear(); Time::Clear(); }
		void Today(void)	{ Date::Today(); Time::Clear(); }
		void Now(void)		{ Date::Today(); Time::Now(); }

		using Date::GetDate;
		int GetDate(void) const	{ return mDate; };

		using Time::GetTime;
		int GetTime(void) const	{ return mTime; }

		Timestamp()	{ Clear(); }

	  	Timestamp(int y, int m, int d)
	  		{ Date::SetDate(y, m, d); Time::Clear(); }

		Timestamp(int y, int mo, int d, int h, int mi, int s)
	  		{ Date::SetDate(y, mo, d); Time::SetTime(h, mi, s); }

		Timestamp(const Timestamp& rv)			// Copy Constructor
			{ mDate = rv.mDate; mTime = rv.mTime; }

		Timestamp& operator=(const Timestamp& rv)	// Timestamp Assignment operator
			{ mDate = rv.mDate; mTime = rv.mTime; return *this; }

		Timestamp& operator=(const Date& rv)	// Date Assignment operator
			{ mDate = rv; return *this; }

		Timestamp& operator=(const Time& rv)	// Time Assignment operator
			{ mTime = rv; return *this; }

		~Timestamp() { }
	};

	/* Class DBKey can store a DBKEY, that special value which the hidden
	 * RDB$DBKEY can give you from a select statement. A DBKey is nothing
	 * specific to IBPP. It's a feature of the Firebird database engine. See its
	 * documentation for more information. */

	class DBKey
	{
	private:
		void* mDBKey;				// Stores the binary DBKey
		mutable char* mString;		// String (temporary) representation of it
		int mSize;					// Length of DBKey (multiple of 8 bytes) 

		void BlindCopy(const DBKey&);

	public:
		void Clear(void);
		void SetKey(const void*, int size);
		void GetKey(void*, int size) const;
		const char* AsString(void) const;

		DBKey& operator=(const DBKey&);	// Assignment operator
		DBKey(const DBKey&);			// Copy Constructor
		DBKey();
		~DBKey();
	};

	/* IBPP never return any error codes. It throws exceptions.
	 * On database engine reported errors, an IBPP::SQLException is thrown.
	 * In all other cases, IBPP throws IBPP::LogicException.
	 * Also note that the runtime and the language might also throw exceptions
	 * while executing some IBPP methods. A failing new operator will throw
	 * std::bad_alloc, IBPP does nothing to alter the standard behaviour.
	 *
	 *                    std::exception
	 *                           |
	 *                   IBPP::Exception
	 *                 /                 \
	 *    IBPP::LogicException    IBPP::SQLException
	 */

	class Exception : public std::exception
	{
	public:
		virtual const char* Origin() const throw() = 0;
		virtual const char* ErrorMessage() const throw() = 0;	// Deprecated, use what()
		virtual const char* what() const throw() = 0;
		virtual ~Exception() throw();
	};

	class LogicException : public Exception
	{
	public:
		virtual ~LogicException() throw();
	};

	class SQLException : public Exception
	{
	public:
		virtual int SqlCode(void) const throw() = 0;
		virtual int EngineCode(void) const throw() = 0;
		
		virtual ~SQLException() throw();
	};

	//	--- Interface Classes --- //

	/* Interfaces IBlob, IArray, IService, IDatabase, ITransaction and
	 * IStatement are at the core of IBPP. Though it is possible to program your
	 * applications by using theses interfaces directly (as was the case with
	 * IBPP 1.x), you should refrain from using them and prefer the new IBPP
	 * Objects Blob, Array, ... (without the I in front). Those new objects are
	 * typedef'd right after each interface class definition as you can read
	 * below. If you program using the Blob (instead of the IBlob interface
	 * itself), you'll never have to care about AddRef/Release and you'll never
	 * have to care about deleting your objects. */

	/* IBlob is the interface to the blob capabilities of IBPP. Blob is the
	 * object class you actually use in your programming. In Firebird, at the
	 * row level, a blob is merely a handle to a blob, stored elsewhere in the
	 * database. Blob allows you to retrieve such a handle and then read from or
	 * write to the blob, much in the same manner than you would do with a file. */

	class IBlob
	{
	public:
		virtual IDatabase* Database(void) const = 0;
		virtual ITransaction* Transaction(void) const = 0;
		virtual void Create(void) = 0;
		virtual void Open(void) = 0;
		virtual void Close(void) = 0;
		virtual void Cancel(void) = 0;
		virtual int Read(void*, int size) = 0;
		virtual void Write(const void*, int size) = 0;
		virtual void Info(int* Size, int* Largest, int* Segments) = 0;
	
		virtual void Save(const std::string& data) = 0;
		virtual void Load(std::string& data) = 0;

		virtual IBlob* AddRef(void) = 0;
		virtual void Release(IBlob*&) = 0;

		virtual ~IBlob() { };
	};
	typedef Ptr<IBlob> Blob;

	/*	IArray is the interface to the array capabilities of IBPP. Array is the
	* object class you actually use in your programming. With an Array object, you
	* can create, read and write Interbase Arrays, as a whole or in slices. */

	class IArray
	{
	public:
		virtual IDatabase* Database(void) const = 0;
		virtual ITransaction* Transaction(void) const = 0;
		virtual void Describe(const std::string& table, const std::string& column) = 0;
		virtual void ReadTo(ADT, void* buffer, int elemcount) = 0;
		virtual void WriteFrom(ADT, const void* buffer, int elemcount) = 0;
		virtual SDT ElementType(void) = 0;
		virtual int ElementSize(void) = 0;
		virtual int ElementScale(void) = 0;
		virtual int Dimensions(void) = 0;
		virtual void Bounds(int dim, int* low, int* high) = 0;
		virtual void SetBounds(int dim, int low, int high) = 0;

		virtual IArray* AddRef(void) = 0;
		virtual void Release(IArray*&) = 0;

		virtual ~IArray() { };
	};
	typedef Ptr<IArray> Array;

	/* IService is the interface to the service capabilities of IBPP. Service is
	 * the object class you actually use in your programming. With a Service
	 * object, you can do some maintenance work of databases and servers
	 * (backup, restore, create/update users, ...) */

	class IService
	{
	public:
	    virtual void Connect(void) = 0;
		virtual bool Connected(void) = 0;
		virtual void Disconnect(void) = 0;

		virtual void GetVersion(std::string& version) = 0;

		virtual void AddUser(const std::string& username, const std::string& password,
			const std::string& first, const std::string& middle, const std::string& last) = 0;
		virtual void ModifyUser(const std::string& username, const std::string& password,
			const std::string& first, const std::string& middle, const std::string& last) = 0;
		virtual void RemoveUser(const std::string& username) = 0;
		virtual void ListUsers(std::vector<std::string>& users) = 0;

		virtual void SetPageBuffers(const std::string& dbfile, int buffers) = 0;
		virtual void SetSweepInterval(const std::string& dbfile, int sweep) = 0;
		virtual void SetSyncWrite(const std::string& dbfile, bool) = 0;
		virtual void SetReadOnly(const std::string& dbfile, bool) = 0;
		virtual void SetReserveSpace(const std::string& dbfile, bool) = 0;

		virtual void Shutdown(const std::string& dbfile, DSM mode, int sectimeout) = 0;
		virtual void Restart(const std::string& dbfile) = 0;
		virtual void Sweep(const std::string& dbfile) = 0;
		virtual void Repair(const std::string& dbfile, RPF flags) = 0;

		virtual void StartBackup(const std::string& dbfile,
			const std::string& bkfile, BRF flags = BRF(0)) = 0;
		virtual void StartRestore(const std::string& bkfile, const std::string& dbfile,
			int pagesize = 0, BRF flags = BRF(0)) = 0;

		virtual const char* WaitMsg(void) = 0;	// With reporting (does not block)
		virtual void Wait(void) = 0;			// Without reporting (does block)

		virtual IService* AddRef() = 0;
		virtual void Release(IService*&) = 0;

		virtual ~IService() { };
	};
	typedef Ptr<IService> Service;

	/*	IDatabase is the interface to the database connections in IBPP. Database
	 * is the object class you actually use in your programming. With a Database
	 * object, you can create/drop/connect databases. */

	class IDatabase
	{
	public:
		virtual const char* ServerName(void) const = 0;
		virtual const char* DatabaseName(void) const = 0;
		virtual const char* Username(void) const = 0;
		virtual const char* UserPassword(void) const = 0;
		virtual const char* RoleName(void) const = 0;
		virtual const char* CharSet(void) const = 0;
		virtual const char* CreateParams(void) const = 0;

		virtual void Info(int* ODS, int* ODSMinor, int* PageSize, int* Pages,
			int* Buffers, int* Sweep, bool* Sync, bool* Reserve) = 0;
		virtual void Statistics(int* Fetches, int* Marks,
			int* Reads, int* Writes) = 0;
		virtual void Counts(int* Insert, int* Update, int* Delete, 
			int* ReadIdx, int* ReadSeq) = 0;
		virtual void Users(std::vector<std::string>& users) = 0;
		virtual int Dialect(void) = 0;

		virtual void Create(int dialect) = 0;
		virtual void Connect(void) = 0;
		virtual bool Connected(void) = 0;
		virtual void Inactivate(void) = 0;
		virtual void Disconnect(void) = 0;
		virtual void Drop(void) = 0;

		virtual void DefineEvent(const std::string&, EventInterface*) = 0;
		virtual void ClearEvents(void) = 0;
		virtual void DispatchEvents(void) = 0;

		virtual IDatabase* AddRef(void) = 0;
		virtual void Release(IDatabase*&) = 0;

	    virtual ~IDatabase() { };
	};
	typedef Ptr<IDatabase> Database;

	/* ITransaction is the interface to the transaction connections in IBPP.
	 * Transaction is the object class you actually use in your programming. A
	 * Transaction object can be associated with more than one Database,
	 * allowing for distributed transactions spanning multiple databases,
	 * possibly located on different servers. IBPP is one among the few
	 * programming interfaces to Firebird that allows you to support distributed
	 * transactions. */

	class ITransaction
	{
	public:
	    virtual void AttachDatabase(IDatabase* db, TAM am = amWrite,
			TIL il = ilConcurrency, TLR lr = lrWait, TFF flags = TFF(0)) = 0;
	    virtual void DetachDatabase(IDatabase* db) = 0;
	 	virtual void AddReservation(IDatabase* db,
	 			const std::string& table, TTR tr) = 0;

		virtual void Start(void) = 0;
		virtual bool Started(void) = 0;
	    virtual void Commit(void) = 0;
	    virtual void Rollback(void) = 0;
	    virtual void CommitRetain(void) = 0;
		virtual void RollbackRetain(void) = 0;

		virtual ITransaction* AddRef(void) = 0;
		virtual void Release(ITransaction*&) = 0;

	    virtual ~ITransaction() { };
	};
	typedef Ptr<ITransaction> Transaction;

	/*
	 *	Class Row can hold all the values of a row (from a SELECT for instance).
	 */

	class IRow
	{
	public:
		virtual	IDatabase* Database(void) const = 0;
		virtual ITransaction* Transaction(void) const = 0;

		virtual void SetNull(int) = 0;
		virtual void Set(int, bool) = 0;
		virtual void Set(int, const void*, int) = 0;		// byte buffers
		virtual void Set(int, const std::string&) = 0;
		virtual void Set(int, short) = 0;
		virtual void Set(int, int) = 0;
		virtual void Set(int, long) = 0;
		virtual void Set(int, int64_t) = 0;
		virtual void Set(int, float) = 0;
		virtual void Set(int, double) = 0;
		virtual void Set(int, const Timestamp&) = 0;
		virtual void Set(int, const Date&) = 0;
		virtual void Set(int, const Time&) = 0;
		virtual void Set(int, const DBKey&) = 0;
		virtual void Set(int, const Blob&) = 0;
		virtual void Set(int, const Array&) = 0;

		virtual bool IsNull(int) = 0;
		virtual bool Get(int, bool&) = 0;
		virtual bool Get(int, void*, int&) = 0;	// byte buffers
		virtual bool Get(int, std::string&) = 0;
		virtual bool Get(int, short&) = 0;
		virtual bool Get(int, int&) = 0;
		virtual bool Get(int, long&) = 0;
		virtual bool Get(int, int64_t&) = 0;
		virtual bool Get(int, float&) = 0;
		virtual bool Get(int, double&) = 0;
		virtual bool Get(int, Timestamp&) = 0;
		virtual bool Get(int, Date&) = 0;
		virtual bool Get(int, Time&) = 0;
		virtual bool Get(int, DBKey&) = 0;
		virtual bool Get(int, Blob&) = 0;
		virtual bool Get(int, Array&) = 0;

		virtual bool IsNull(const std::string&) = 0;
		virtual bool Get(const std::string&, bool&) = 0;
		virtual bool Get(const std::string&, void*, int&) = 0;	// byte buffers
		virtual bool Get(const std::string&, std::string&) = 0;
		virtual bool Get(const std::string&, short&) = 0;
		virtual bool Get(const std::string&, int&) = 0;
		virtual bool Get(const std::string&, long&) = 0;
		virtual bool Get(const std::string&, int64_t&) = 0;
		virtual bool Get(const std::string&, float&) = 0;
		virtual bool Get(const std::string&, double&) = 0;
		virtual bool Get(const std::string&, Timestamp&) = 0;
		virtual bool Get(const std::string&, Date&) = 0;
		virtual bool Get(const std::string&, Time&) = 0;
		virtual bool Get(const std::string&, DBKey&) = 0;
		virtual bool Get(const std::string&, Blob&) = 0;
		virtual bool Get(const std::string&, Array&) = 0;

		virtual int ColumnNum(const std::string&) = 0;
		virtual const char* ColumnName(int) = 0;
		virtual const char* ColumnAlias(int) = 0;
		virtual const char* ColumnTable(int) = 0;
		virtual SDT ColumnType(int) = 0;
		virtual int ColumnSize(int) = 0;
		virtual int ColumnScale(int) = 0;
		virtual int Columns(void) = 0;
		
		virtual bool ColumnUpdated(int) = 0;
		virtual bool Updated() = 0;

		virtual IRow* Clone() = 0;
		virtual IRow* AddRef() = 0;
		virtual void Release(IRow*&) = 0;

	    virtual ~IRow() {};
	};
	typedef Ptr<IRow> Row;

	/* IStatement is the interface to the statements execution in IBPP.
	 * Statement is the object class you actually use in your programming. A
	 * Statement object is the work horse of IBPP. All your data manipulation
	 * statements will be done through it. It is also used to access the result
	 * set of a query (when the statement is such), one row at a time and in
	 * strict forward direction. */

	class IStatement
	{
	public:
		virtual	IDatabase* Database(void) const = 0;
		virtual ITransaction* Transaction(void) const = 0;
		virtual void Prepare(const std::string&) = 0;
		virtual void Execute(void) = 0;
		virtual void Execute(const std::string&) = 0;
		virtual void ExecuteImmediate(const std::string&) = 0;
		virtual void CursorExecute(const std::string& cursor) = 0;
		virtual void CursorExecute(const std::string& cursor, const std::string&) = 0;
		virtual bool Fetch(void) = 0;
		virtual bool Fetch(Row&) = 0;
		virtual int AffectedRows(void) = 0;
		virtual void Close(void) = 0;
		virtual STT Type(void) = 0;

		virtual void SetNull(int) = 0;
		virtual void Set(int, bool) = 0;
		virtual void Set(int, const char*) = 0;				// c-strings
		virtual void Set(int, const void*, int) = 0;		// byte buffers
		virtual void Set(int, const std::string&) = 0;
		virtual void Set(int, short value) = 0;
		virtual void Set(int, int value) = 0;
		virtual void Set(int, long value) = 0;
		virtual void Set(int, int64_t value) = 0;
		virtual void Set(int, float value) = 0;
		virtual void Set(int, double value) = 0;
		virtual void Set(int, const Timestamp& value) = 0;
		virtual void Set(int, const Date& value) = 0;
		virtual void Set(int, const Time& value) = 0;
		virtual void Set(int, const DBKey& value) = 0;
		virtual void Set(int, const Blob& value) = 0;
		virtual void Set(int, const Array& value) = 0;

		virtual bool IsNull(int) = 0;
		virtual bool Get(int, bool*) = 0;
		virtual bool Get(int, bool&) = 0;
		virtual bool Get(int, char*) = 0;  		// c-strings, len unchecked
		virtual bool Get(int, void*, int&) = 0;	// byte buffers
		virtual bool Get(int, std::string&) = 0;
		virtual bool Get(int, short*) = 0;
		virtual bool Get(int, short&) = 0;
		virtual bool Get(int, int*) = 0;
		virtual bool Get(int, int&) = 0;
		virtual bool Get(int, long*) = 0;
		virtual bool Get(int, long&) = 0;
		virtual bool Get(int, int64_t*) = 0;
		virtual bool Get(int, int64_t&) = 0;
		virtual bool Get(int, float*) = 0;
		virtual bool Get(int, float&) = 0;
		virtual bool Get(int, double*) = 0;
		virtual bool Get(int, double&) = 0;
		virtual bool Get(int, Timestamp& value) = 0;
		virtual bool Get(int, Date& value) = 0;
		virtual bool Get(int, Time& value) = 0;
		virtual bool Get(int, DBKey& value) = 0;
		virtual bool Get(int, Blob& value) = 0;
		virtual bool Get(int, Array& value) = 0;

		virtual bool IsNull(const std::string&) = 0;
		virtual bool Get(const std::string&, bool*) = 0;
		virtual bool Get(const std::string&, bool&) = 0;
		virtual bool Get(const std::string&, char*) = 0;	// c-strings, len unchecked
		virtual bool Get(const std::string&, void*, int&) = 0;	// byte buffers
		virtual bool Get(const std::string&, std::string&) = 0;
		virtual bool Get(const std::string&, short*) = 0;
		virtual bool Get(const std::string&, short&) = 0;
		virtual bool Get(const std::string&, int*) = 0;
		virtual bool Get(const std::string&, int&) = 0;
		virtual bool Get(const std::string&, long*) = 0;
		virtual bool Get(const std::string&, long&) = 0;
		virtual bool Get(const std::string&, int64_t*) = 0;
		virtual bool Get(const std::string&, int64_t&) = 0;
		virtual bool Get(const std::string&, float*) = 0;
		virtual bool Get(const std::string&, float&) = 0;
		virtual bool Get(const std::string&, double*) = 0;
		virtual bool Get(const std::string&, double&) = 0;
		virtual bool Get(const std::string&, Timestamp& value) = 0;
		virtual bool Get(const std::string&, Date& value) = 0;
		virtual bool Get(const std::string&, Time& value) = 0;
		virtual bool Get(const std::string&, DBKey& value) = 0;
		virtual bool Get(const std::string&, Blob& value) = 0;
		virtual bool Get(const std::string&, Array& value) = 0;

		virtual int ColumnNum(const std::string&) = 0;
		virtual const char* ColumnName(int) = 0;
		virtual const char* ColumnAlias(int) = 0;
		virtual const char* ColumnTable(int) = 0;
		virtual SDT ColumnType(int) = 0;
		virtual int ColumnSize(int) = 0;
		virtual int ColumnScale(int) = 0;
		virtual int Columns(void) = 0;

		virtual SDT ParameterType(int) = 0;
		virtual int ParameterSize(int) = 0;
		virtual int ParameterScale(int) = 0;
		virtual int Parameters(void) = 0;

		virtual void Plan(std::string&) = 0;

		virtual IStatement* AddRef(void) = 0;
		virtual void Release(IStatement*&) = 0;

	    virtual ~IStatement() { };
	};
	typedef Ptr<IStatement> Statement;
	
	//	--- Factories ---
	//	These methods are the only way to get one of the above
	//	Interfaces.  They are at the heart of how you program using IBPP.  For
	//	instance, to get access to a database, you'll write code similar to this:
	//	{
	//		Database db = DatabaseFactory("server", "databasename",
	//						"user", "password");
	//		db->Connect();
	//		...
	//		db->Disconnect();
	//	}

	IService* ServiceFactory(const std::string& ServerName,
		const std::string& UserName, const std::string& UserPassword);

	IDatabase* DatabaseFactory(const std::string& ServerName,
		const std::string& DatabaseName, const std::string& UserName,
			const std::string& UserPassword, const std::string& RoleName,
				const std::string& CharSet, const std::string& CreateParams);

	inline IDatabase* DatabaseFactory(const std::string& ServerName,
		const std::string& DatabaseName, const std::string& UserName,
			const std::string& UserPassword)
		{ return DatabaseFactory(ServerName, DatabaseName, UserName, UserPassword, "", "", "");	}

	ITransaction* TransactionFactory(IDatabase* db, TAM am = amWrite,
		TIL il = ilConcurrency, TLR lr = lrWait, TFF flags = TFF(0));

	inline ITransaction* TransactionFactory(Database& db, TAM am = amWrite,
		TIL il = ilConcurrency, TLR lr = lrWait, TFF flags = TFF(0))
			{ return TransactionFactory(db.intf(), am, il, lr, flags); }

	//IRow* RowFactory(int dialect);
	
	IStatement* StatementFactory(IDatabase* db, ITransaction* tr,
		const std::string& sql);

	inline IStatement* StatementFactory(IDatabase* db, ITransaction* tr)
		{ return StatementFactory(db, tr, ""); }

	inline IStatement* StatementFactory(Database& db, Transaction& tr,
		const std::string& sql)
			{ return StatementFactory(db.intf(), tr.intf(), sql); }

	inline IStatement* StatementFactory(Database& db, Transaction& tr)
			{ return StatementFactory(db.intf(), tr.intf(), ""); }

	IBlob* BlobFactory(IDatabase* db, ITransaction* tr);
	inline IBlob* BlobFactory(Database& db, Transaction& tr)
		{ return BlobFactory(db.intf(), tr.intf()); }

	IArray* ArrayFactory(IDatabase* db, ITransaction* tr);
	inline IArray* ArrayFactory(Database& db, Transaction& tr)
		{ return ArrayFactory(db.intf(), tr.intf()); }

	/* Class EventInterface is merely a pure interface. It is _not_ implemented
	 * by IBPP. It is just base class definition from which your own event
	 * interface classes have to derive from.

	 * The Event handling system from the class Database requires the programmer
	 * to give an object pointer when defining a new event. When that event will
	 * fire, IBPP will call the method ibppEventHandler on the object registered
	 * with the event. So in order to catch events, the programmer must derive a
	 * class from EventInterface, implement 'ibppEventHandler' to do anything
	 * wanted when the event triggers and register an instance of that derived
	 * class when defining the event trap. */

	class EventInterface
	{
	public:
		virtual void ibppEventHandler(IDatabase*, const std::string&, int) = 0;

		virtual ~EventInterface() { };
	};

	/* IBPP uses a self initialization system. Each time an object that may
	 * require the usage of the Interbase client C-API library is used, the
	 * library internal handling details are automatically initialized, if not
	 * already done. You can kick this initialization at the start of an
	 * application by calling IBPP::CheckVersion(). This is recommended, because
	 * IBPP::CheckVersion will assure you that YOUR code has been compiled
	 * against a compatible version of the library. */

	bool CheckVersion(unsigned long);
	int GDSVersion(void);

	/* Finally, here are some date and time conversion routines used by IBPP and
	 * that may be helpful at the application level. They do not depend on
	 * anything related to Firebird/Interbase. Just a bonus. dtoi and itod
	 * return false on invalid parameters or out of range conversions. */

	bool dtoi (int date, int *y, int *m, int *d);
	bool itod (int* pdate, int year, int month, int day);
	void ttoi (int itime, int *hour, int *minute, int *second);
	void itot (int* ptime, int hour, int minute, int second);

};

#endif

//
//	EOF
//
