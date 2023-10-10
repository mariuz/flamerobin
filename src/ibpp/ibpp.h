//  Subject : IBPP public header file. This is _the_ only file you include in
//            your application files when developing with IBPP.
//  COMMENTS
//  When compiling a project using IBPP, the following defines should be made
//  on the command-line (or in makefiles) according to the OS platform and
//  compiler used.
//
//  Select the platform:    IBPP_WINDOWS | IBPP_LINUX | IBPP_DARWIN
//

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

  Contributor(s):

      Olivier Mascia, main coding
      Matt Hortman, initial linux port
      Mark Jordan, design contributions
      Maxim Abrashkin, enhancement patches
      Torsten Martinsen, enhancement patches
      Michael Hieke, darwin (OS X) port, enhancement patches
      Val Samko, enhancement patches and debugging
      Mike Nordell, invaluable C++ advices
      Claudio Valderrama, help with not-so-well documented IB/FB features
      Many others, excellent suggestions, bug finding, and support
*/

#ifndef __IBPP_H__
#define __IBPP_H__

#if !defined(IBPP_WINDOWS) && !defined(IBPP_LINUX) && !defined(IBPP_DARWIN)
#error Please define IBPP_WINDOWS/IBPP_LINUX/IBPP_DARWIN before compiling !
#endif

#if !defined(__GNUC__) && !defined(_MSC_VER) 
#error Your compiler is not recognized.
#endif

#if defined(IBPP_LINUX) || defined(IBPP_DARWIN)
#define IBPP_UNIX   // IBPP_UNIX stands as a common denominator to *NIX flavours
#endif

// The standard type 'int' is assumed to be at least 32 bits.
// And the standard type 'short' is assumed to be exactly 16 bits.
// Everywhere possible, where the exact size of an integer does not matter,
// the standard type 'int' is used. And where an exact integer size is required
// the standard exact precision types definitions of C 99 standard are used.

#include <stdint.h>

#if !defined(_)
#define _(s)    s
#endif

#include <exception>
#include <map>
#include <string>
#include <vector>

#ifdef HAVE_DECIMAL128
#include <decimal/decimal>
#endif

namespace IBPP
{
    //  Typically you use this constant in a call IBPP::CheckVersion as in:
    //  if (! IBPP::CheckVersion(IBPP::Version)) { throw .... ; }
    const uint32_t Version = (2<<24) + (5<<16) + (2<<8) + 1; // Version == 2.5.2.1

    //  Dates range checking
    const int MinDate = -693594;    //  1 JAN 0001
    const int MaxDate = 2958464;    // 31 DEC 9999

    //  Transaction Access Modes
    enum TAM {amWrite, amRead};

    //  Transaction Isolation Levels
    enum TIL {ilConcurrency, ilReadDirty, ilReadCommitted, ilConsistency};

    //  Transaction Lock Resolution
    enum TLR {lrWait, lrNoWait};

    // Transaction Table Reservation
    enum TTR {trSharedWrite, trSharedRead, trProtectedWrite, trProtectedRead};

    //  Prepared Statement Types
    enum STT {stUnknown, stUnsupported,
        stSelect, stInsert, stUpdate, stDelete, stDDL, stExecProcedure,
        stStartTransaction, stCommitTransaction, stRollbackTransaction,
        stSelectUpdate, stSetGenerator, stSavePoint};

    //  SQL Data Types
    enum SDT {sdArray, sdBlob, sdDate, sdTime, sdTimestamp, sdString,
        sdSmallint, sdInteger, sdLargeint, sdFloat, sdDouble, sdBoolean,
        sdTimeTz, sdTimestampTz, sdInt128, sdDec16, sdDec34};

    bool isIntegerNumber(SDT type);
    bool isRationalNumber(SDT type);

    //  Array Data Types
    enum ADT {adDate, adTime, adTimestamp, adString,
        adBool, adInt16, adInt32, adInt64, adFloat, adDouble};

    // Database::Shutdown Modes
    enum DSM {
        dsVerbose = 0x1,
        // Shutdhown  modes
        dsCache = 0x100, 
        dsForce = 0x200, 
        dsDenyTrans = 0x400, 
        dsDenyAttach = 0x800,
        // database Modes
        dsNormal =  0x1000,
        dsSingle = 0x2000,
        dsMulti = 0x4000,
        dsFull = 0x8000
    };


    // Service::StartBackup && Service::StartRestore Flags
    enum BRF {
        // Backup flags
        brConvertExtTables      = 0x0000001,
        brExpand                = 0x0000002,
        brNoGarbageCollect      = 0x0000004,
        brIgnoreChecksums       = 0x0000008, 
        brIgnoreLimbo           = 0x0000010,
        brNoDBTriggers          = 0x0000020, //FB2.5+
        brOldDescriptions       = 0x0000040,
        brNonTransportable      = 0x0000080,
        brZip                   = 0x0000100, //FB4.0+
        // Restore flags
        brFix_Fss_Data          = 0x0000200, //FB2.0+
        brFix_Fss_Metadata      = 0x0000400, //FB2.0+
        brDeactivateIdx         = 0x0000800,
        brNoShadow              = 0x0001000,
        brNoValidity            = 0x0002000,
        brPerTableCommit        = 0x0040000,
        brUseAllSpace           = 0x0080000,
        brDatabase_readonly     = 0x0100000,
        brReplicaMode_none      = 0x0200000, //FB4.0+
        brReplicaMode_readonly  = 0x0400000, //FB4.0+
        brReplicaMode_readwrite = 0x0800000, //FB4.0+
        brReplace               = 0x1000000,
        // General flags
        brMetadataOnly          = 0x2000000, //FB2.5+
        brVerbose               = 0x4000000,
        brstatistics_time       = 0x8000000, //FB2.5+
        brstatistics_delta      = 0x10000000,//FB2.5+
        brstatistics_pagereads  = 0x20000000,//FB2.5+
        brstatistics_pagewrites = 0x40000000 //FB2.5+
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

    // int128 - FB4
    #pragma pack(push, 1)

    #ifndef HAVE_INT128
    // NOTICE: could/should be replaced with int128_t if msvc supports this
    typedef struct IBPP_INT128_T
    {
    private:
        // _InOut_ dst
        // _In_ toadd
        // _Out_ overflow
        void AddPart128(uint32_t* dst, const uint32_t& toadd, bool* overflow);
        // _InOut_ T1
        // _In_ T2
        // _Out_ overflow
        void Add128(IBPP_INT128_T* T1, const IBPP_INT128_T& T2, bool* overflow);
public:
        union DATA
        {
            struct
            {
                uint64_t lowPart;
                int64_t highPart;
            } s2;
            struct
            {
                uint64_t lowPart;
                uint64_t highPart;
            } us2;
            struct
            {
                uint32_t llPart;
                uint32_t hlPart;
                uint32_t lhPart;
                uint32_t hhPart;
            } s4;
        } data;

        // constructor
        IBPP_INT128_T() {};
        IBPP_INT128_T(const int64_t value);

        IBPP_INT128_T operator-();
        IBPP_INT128_T operator-(const IBPP_INT128_T& T2);
        bool operator<(const IBPP_INT128_T& T2) const;
    } ibpp_int128_t;
    typedef struct IBPP_UINT128_T
    {
        uint64_t lowPart;
        uint64_t highPart;
    } ibpp_uint128_t;
    #else
    typedef __int128 ibpp_int128_t;
    typedef __uint128_t ibpp_uint128_t;
    #endif

    #ifndef HAVE_DECIMAL128
    typedef struct IBPP_DEC16_T
    {
        int64_t lowPart;
    } ibpp_dec16_t;
    typedef struct IBPP_DEC34_T
    {
        uint64_t lowPart;
        uint64_t highPart;
    } ibpp_dec34_t;
    #else
    typedef std::decimal::decimal64 ibpp_dec16_t;
    typedef std::decimal::decimal128 ibpp_dec34_t;
    #endif

    #pragma pack(pop)

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
     *             |
     *      IBPP::WrongType
     */

    class Exception : public std::exception
    {
    public:
        virtual const char* Origin() const throw() = 0;        
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
        virtual int SqlCode() const throw() = 0;
        virtual int EngineCode() const throw() = 0;

        virtual ~SQLException() throw();
    };

    class WrongType : public LogicException
    {
    public:
        virtual ~WrongType() throw();
    };

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
     * account.
     * The full range goes from integer values IBPP::MinDate to IBPP::MaxDate
     * which means from 01 Jan 0001 to 31 Dec 9999. ( Which is inherently
     * incorrect as this assumes Gregorian calendar. ) */

    class Timestamp;    // Cross-reference between Timestamp, Date and Time

    class Date
    {
    protected:
        int mDate;  // The date : 1 == 1 Jan 1900

    public:
        void Clear()    { mDate = MinDate - 1; }
        void Today();
        void SetDate(int year, int month, int day);
        void SetDate(int dt);
        void GetDate(int& year, int& month, int& day) const;
        int GetDate() const { return mDate; }
        int Year() const;
        int Month() const;
        int Day() const;
        void Add(int days);
        void StartOfMonth();
        void EndOfMonth();

        Date()          { Clear(); }
        Date(int dt)    { SetDate(dt); }
        Date(int year, int month, int day);
        Date(const Date&);                          // Copy Constructor
        Date& operator=(const Timestamp&);          // Timestamp Assignment operator
        Date& operator=(const Date&);               // Date Assignment operator

        bool operator==(const Date& rv) { return mDate == rv.GetDate(); }
        bool operator<(const Date& rv) { return mDate < rv.GetDate(); }

        virtual ~Date() { }
    };

    /* Class Time represent purely a Time. It is usefull in interactions
     * with the SQL TIME type of Interbase. */

    class Time
    {
	public:
        enum TimezoneMode
        {
            // ISC_TIME / ISC_TIMESTAMP
            tmNone,
            // ISC_TIME_TZ / ISC_TIMESTAMP_TZ
            tmTimezone,
        };
		/* no time zone -> utc = local */
		const static int TZ_NONE     =  0;
    protected:
        // The time, in ten-thousandths of seconds since midnight - UTC and TZ
        mutable int mTime;
        mutable TimezoneMode mTimezoneMode;
        // The timezone
        int mTimezone;

        void SetTimezone(int tz);
    public:
        void Clear()    { mTime = 0; mTimezoneMode = tmNone; mTimezone = TZ_NONE; }
        void Now();
        void SetTime(TimezoneMode tzMode, int hour, int minute, int second, int tenthousandths, int timezone);
        void SetTime(TimezoneMode tzMode, int tm, int timezone);
        void GetTime(int& hour, int& minute, int& second) const;
        void GetTime(int& hour, int& minute, int& second, int& tenthousandths) const;
        int GetTime() const;
        int GetTimezone() const { return mTimezone; }
        int Hours() const;
        int Minutes() const;
        int Seconds() const;
        int SubSeconds() const;     // Actually tenthousandths of seconds
        Time()          { Clear(); }
        Time(TimezoneMode tzMode, int tm, int timezone)    { SetTime(tzMode, tm, timezone); }
        Time(TimezoneMode tzMode, int hour, int minute, int second, int tenthousandths, int timezone);
        Time(const Time&);                          // Copy Constructor
        Time& operator=(const Timestamp&);          // Timestamp Assignment operator
        Time& operator=(const Time&);               // Time Assignment operator

        bool operator==(const Time& rv) { return mTime == rv.GetTime(); }
        bool operator<(const Time& rv) { return mTime < rv.GetTime(); }
    };

    /* Class Timestamp represent a date AND a time. It is usefull in
     * interactions with the SQL TIMESTAMP type of Interbase. This class
     * inherits from Date and Time and completely inline implements its small
     * specific details. */

    class Timestamp : public Date, public Time
    {
    public:
        void Clear()    { Date::Clear(); Time::Clear(); }
        void Today()    { Date::Today(); Time::Clear(); }
        void Now(/*int timezone*/) { Date::Today(); Time::Now(/*timezone*/); }

        Timestamp()     { Clear(); }

        Timestamp(int y, int m, int d)
            { Date::SetDate(y, m, d); Time::Clear(); }

        Timestamp(int y, int mo, int d, TimezoneMode tzMode, int h, int mi, int s, int t, int tz)
            { Date::SetDate(y, mo, d); Time::SetTime(tzMode, h, mi, s, t, tz); }

        Timestamp(const Timestamp& rv)
            : Date(rv.mDate), Time::Time(rv) {} // Copy Constructor

        Timestamp(const Date& rv)
            { mDate = rv.GetDate(); Time::Clear(); }

        Timestamp(const Time& rv)
            { mDate = 0; (Time)*this = rv; }

        Timestamp& operator=(const Timestamp& rv)   // Timestamp Assignment operator
            { mDate = rv.mDate; (Time)*this = (Time)rv; return *this; }

        Timestamp& operator=(const Date& rv)        // Date Assignment operator
            { mDate = rv.GetDate(); return *this; }

        Timestamp& operator=(const Time& rv)        // Time Assignment operator
            { (Time)*this = (Time)rv; return *this; }

        bool operator==(const Timestamp& rv)
            { return (mDate == rv.GetDate()) && (mTime == rv.GetTime()); }

        bool operator<(const Timestamp& rv)
            { return (mDate < rv.GetDate()) ||
                (mDate == rv.GetDate() && mTime < rv.GetTime()); }

        ~Timestamp() { }
    };

    /* Class DBKey can store a DBKEY, that special value which the hidden
     * RDB$DBKEY can give you from a select statement. A DBKey is nothing
     * specific to IBPP. It's a feature of the Firebird database engine. See its
     * documentation for more information. */

    class DBKey
    {
    private:
        std::string mDBKey;         // Stores the binary DBKey
        mutable std::string mString;// String (temporary) representation of it

    public:
        void Clear();
        int Size() const    { return (int)mDBKey.size(); }
        void SetKey(const void*, int size);
        void GetKey(void*, int size) const;
        const char* AsString() const;

        DBKey& operator=(const DBKey&); // Assignment operator
        DBKey(const DBKey&);            // Copy Constructor
        DBKey() { }
        ~DBKey() { }
    };

    /* Class User wraps all the information about a user that the engine can manage. */

    class User
    {
    public:
        std::string username;
        std::string password;
        std::string firstname;
        std::string middlename;
        std::string lastname;
        uint32_t userid;        // Only relevant on unixes
        uint32_t groupid;       // Only relevant on unixes

    private:
        void copyfrom(const User& r);

    public:
        void clear();
        User& operator=(const User& r)  { copyfrom(r); return *this; }
        User(const User& r)             { copyfrom(r); }
        User() : userid(0), groupid(0)  { }
        ~User() { }
    };

    //  Interface Wrapper
    template <class T>
    class Ptr
    {
    private:
        T* mObject;

    public:
        void clear()
        {
            if (mObject != 0) { mObject->Release(); mObject = 0; }
        }

        T* intf() const                     { return mObject; }
        T* operator->() const               { return mObject; }

        bool operator==(const T* p) const   { return mObject == p; }
        bool operator==(const Ptr& r) const { return mObject == r.mObject; }
        bool operator!=(const T* p) const   { return mObject != p; }
        bool operator!=(const Ptr& r) const { return mObject != r.mObject; }

        Ptr& operator=(T* p)
        {
            // AddRef _before_ Release gives correct behaviour on self-assigns
            T* tmp = (p == 0 ? 0 : p->AddRef());    // Take care of 0
            if (mObject != 0) mObject->Release();
            mObject = tmp; return *this;
        }

        Ptr& operator=(const Ptr& r)
        {
            // AddRef _before_ Release gives correct behaviour on self-assigns
            T* tmp = (r.intf() == 0 ? 0 : r->AddRef());// Take care of 0
            if (mObject != 0) mObject->Release();
            mObject = tmp; return *this;
        }

        Ptr(T* p) : mObject(p == 0 ? 0 : p->AddRef()) { }
        Ptr(const Ptr& r) : mObject(r.intf() == 0 ? 0 : r->AddRef()) {  }

        Ptr() : mObject(0) { }
        ~Ptr() { clear(); }
    };

    //  --- Interface Classes --- //

    /* Interfaces IBlob, IArray, IService, IDatabase, ITransaction and
     * IStatement are at the core of IBPP. Though it is possible to program your
     * applications by using theses interfaces directly (as was the case with
     * IBPP 1.x), you should refrain from using them and prefer the new IBPP
     * Objects Blob, Array, ... (without the I in front). Those new objects are
     * typedef'd right after each interface class definition as you can read
     * below. If you program using the Blob (instead of the IBlob interface
     * itself), you'll never have to care about AddRef/Release and you'll never
     * have to care about deleting your objects. */

    class IBlob;            typedef Ptr<IBlob> Blob;
    class IArray;           typedef Ptr<IArray> Array;
    class IService;         typedef Ptr<IService> Service;
    class IDatabase;        typedef Ptr<IDatabase> Database;
    class ITransaction;     typedef Ptr<ITransaction> Transaction;
    class IStatement;       typedef Ptr<IStatement> Statement;
    class IEvents;          typedef Ptr<IEvents> Events;
    class IRow;             typedef Ptr<IRow> Row;

    /* IBlob is the interface to the blob capabilities of IBPP. Blob is the
     * object class you actually use in your programming. In Firebird, at the
     * row level, a blob is merely a handle to a blob, stored elsewhere in the
     * database. Blob allows you to retrieve such a handle and then read from or
     * write to the blob, much in the same manner than you would do with a file. */

    class IBlob
    {
    public:
        virtual void Create() = 0;
        virtual void Open() = 0;
        virtual void Close() = 0;
        virtual void Cancel() = 0;
        virtual int Read(void*, int size) = 0;
        virtual void Write(const void*, int size) = 0;
        virtual void Info(int* Size, int* Largest, int* Segments) = 0;

        virtual void Save(const std::string& data) = 0;
        virtual void Load(std::string& data) = 0;

        virtual Database DatabasePtr() const = 0;
        virtual Transaction TransactionPtr() const = 0;

        virtual IBlob* AddRef() = 0;
        virtual void Release() = 0;

        virtual ~IBlob() { }
    };

    /*  IArray is the interface to the array capabilities of IBPP. Array is the
    * object class you actually use in your programming. With an Array object, you
    * can create, read and write Interbase Arrays, as a whole or in slices. */

    class IArray
    {
    public:
        virtual void Describe(const std::string& table, const std::string& column) = 0;
        virtual void ReadTo(ADT, void* buffer, int elemcount) = 0;
        virtual void WriteFrom(ADT, const void* buffer, int elemcount) = 0;
        virtual SDT ElementType() = 0;
        virtual int ElementSize() = 0;
        virtual int ElementScale() = 0;
        virtual int Dimensions() = 0;
        virtual void Bounds(int dim, int* low, int* high) = 0;
        virtual void SetBounds(int dim, int low, int high) = 0;

        virtual Database DatabasePtr() const = 0;
        virtual Transaction TransactionPtr() const = 0;

        virtual IArray* AddRef() = 0;
        virtual void Release() = 0;

        virtual ~IArray() { }
    };

    /* IService is the interface to the service capabilities of IBPP. Service is
     * the object class you actually use in your programming. With a Service
     * object, you can do some maintenance work of databases and servers
     * (backup, restore, create/update users, ...) */

    class IService
    {
    public:
        virtual void Connect() = 0;
        virtual bool Connected() = 0;
        virtual void Disconnect() = 0;

        virtual void GetVersion(std::string& version) = 0;
        virtual bool versionIsHigherOrEqualTo(int versionMajor, int versionMinor) = 0;

        virtual void AddUser(const User&) = 0;
        virtual void GetUser(User&) = 0;
        virtual void GetUsers(std::vector<User>&) = 0;
        virtual void ModifyUser(const User&) = 0;
        virtual void RemoveUser(const std::string& username) = 0;

        virtual void SetPageBuffers(const std::string& dbfile, int buffers) = 0;
        virtual void SetSweepInterval(const std::string& dbfile, int sweep) = 0;
        virtual void SetSyncWrite(const std::string& dbfile, bool) = 0;
        virtual void SetReadOnly(const std::string& dbfile, bool) = 0;
        virtual void SetReserveSpace(const std::string& dbfile, bool) = 0;

        virtual void Shutdown(const std::string& dbfile, DSM flags, int sectimeout) = 0;
        virtual void Restart(const std::string& dbfile, DSM flags) = 0;
        virtual void Sweep(const std::string& dbfile) = 0;
        virtual void Repair(const std::string& dbfile, RPF flags) = 0;

        virtual void StartBackup(
            const std::string& dbfile,const std::string& bkfile, const std::string& outfile = "",
            const int factor = 0, BRF flags = BRF(0),
            const std::string& cryptName = "", const std::string& keyHolder="", const std::string& keyName="",
            const std::string& skipData = "", const std::string& includeData = "", const int verboseInteval = 0,
            const int parallelWorkers = 0
        ) = 0;

        virtual void StartRestore(
            const std::string& bkfile, const std::string& dbfile, const std::string& outfile = "",
            int pagesize = 0, int buffers = 0, BRF flags = BRF(0),
            const std::string & cryptName = "", const std::string & keyHolder = "", const std::string & keyName = "",
            const std::string & skipData = "", const std::string & includeData = "", const int verboseInteval = 0,
            const int parallelWorkers = 0
        ) = 0;

        virtual const char* WaitMsg() = 0;  // With reporting (does not block)
        virtual void Wait() = 0;            // Without reporting (does block)

        virtual IService* AddRef() = 0;
        virtual void Release() = 0;

        virtual ~IService() { }
    };

    /*  IDatabase is the interface to the database connections in IBPP. Database
     * is the object class you actually use in your programming. With a Database
     * object, you can create/drop/connect databases. */

    class EventInterface;   // Cross-reference between EventInterface and IDatabase

    class CountInfo
    {
    public:
        CountInfo(): inserts(0), updates(0), deletes(0), readIndex(0), readSequence(0) {}
        int inserts;
        int updates;
        int deletes;
        int readIndex;
        int readSequence;
    };
    typedef std::map<int, CountInfo> DatabaseCounts; // int = relation ID

    class IDatabase
    {
    public:
        virtual const char* ServerName() const = 0;
        virtual const char* DatabaseName() const = 0;
        virtual const char* Username() const = 0;
        virtual const char* UserPassword() const = 0;
        virtual const char* RoleName() const = 0;
        virtual const char* CharSet() const = 0;
        virtual const char* CreateParams() const = 0;

        virtual void Info(int* ODS, int* ODSMinor, int* PageSize,
            int* Pages, int* Buffers, int* Sweep, bool* Sync,
            bool* Reserve, bool* ReadOnly) = 0;
        virtual void TransactionInfo(int* Oldest, int* OldestActive,
            int* OldestSnapshot, int* Next) = 0;
        virtual void Statistics(int* Fetches, int* Marks,
            int* Reads, int* Writes, int* CurrentMemory ) = 0;
        virtual void Counts(int* Insert, int* Update, int* Delete,
            int* ReadIdx, int* ReadSeq) = 0;
        virtual void DetailedCounts(DatabaseCounts& counts) = 0;
        virtual void Users(std::vector<std::string>& users) = 0;
        virtual int Dialect() = 0;

        virtual void Create(int dialect) = 0;
        virtual void Connect() = 0;
        virtual bool Connected() = 0;
        virtual void Inactivate() = 0;
        virtual void Disconnect() = 0;
        virtual void Drop() = 0;

        virtual IDatabase* Clone() = 0;

        virtual IDatabase* AddRef() = 0;
        virtual void Release() = 0;

        virtual ~IDatabase() { }
    };

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
        virtual void AttachDatabase(Database db, TAM am = amWrite,
            TIL il = ilConcurrency, TLR lr = lrWait, TFF flags = TFF(0)) = 0;
        virtual void DetachDatabase(Database db) = 0;
        virtual void AddReservation(Database db,
                const std::string& table, TTR tr) = 0;

        virtual void Start() = 0;
        virtual bool Started() = 0;
        virtual void Commit() = 0;
        virtual void Rollback() = 0;
        virtual void CommitRetain() = 0;
        virtual void RollbackRetain() = 0;

        virtual ITransaction* AddRef() = 0;
        virtual void Release() = 0;

        virtual ~ITransaction() { }
    };

    /*
     *  Class Row can hold all the values of a row (from a SELECT for instance).
     */

    class IRow
    {
    public:
        virtual void SetNull(int) = 0;
        virtual void Set(int, bool) = 0;
        virtual void Set(int, const void*, int) = 0;        // byte buffers
        virtual void Set(int, const char*) = 0;             // c-string
        virtual void Set(int, const std::string&) = 0;
        virtual void Set(int, int16_t) = 0;
        virtual void Set(int, int32_t) = 0;
        virtual void Set(int, int64_t) = 0;
        virtual void Set(int, IBPP::ibpp_int128_t) = 0;
        virtual void Set(int, float) = 0;
        virtual void Set(int, double) = 0;
        virtual void Set(int, IBPP::ibpp_dec16_t) = 0;
        virtual void Set(int, IBPP::ibpp_dec34_t) = 0;
        virtual void Set(int, const Timestamp&) = 0;
        virtual void Set(int, const Date&) = 0;
        virtual void Set(int, const Time&) = 0;
        virtual void Set(int, const DBKey&) = 0;
        virtual void Set(int, const Blob&) = 0;
        virtual void Set(int, const Array&) = 0;

        virtual bool IsNull(int) = 0;
        virtual bool Get(int, bool&) = 0;
        virtual bool Get(int, void*, int&) = 0; // byte buffers
        virtual bool Get(int, std::string&) = 0;
        virtual bool Get(int, int16_t&) = 0;
        virtual bool Get(int, int32_t&) = 0;
        virtual bool Get(int, int64_t&) = 0;
        virtual bool Get(int, IBPP::ibpp_int128_t&) = 0;
        virtual bool Get(int, float&) = 0;
        virtual bool Get(int, double&) = 0;
        virtual bool Get(int, IBPP::ibpp_dec16_t&) = 0;
        virtual bool Get(int, IBPP::ibpp_dec34_t&) = 0;
        virtual bool Get(int, Timestamp&) = 0;
        virtual bool Get(int, Date&) = 0;
        virtual bool Get(int, Time&) = 0;
        virtual bool Get(int, DBKey&) = 0;
        virtual bool Get(int, Blob&) = 0;
        virtual bool Get(int, Array&) = 0;

        virtual bool IsNull(const std::string&) = 0;
        virtual bool Get(const std::string&, bool&) = 0;
        virtual bool Get(const std::string&, void*, int&) = 0;  // byte buffers
        virtual bool Get(const std::string&, std::string&) = 0;
        virtual bool Get(const std::string&, int16_t&) = 0;
        virtual bool Get(const std::string&, int32_t&) = 0;
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
        virtual int ColumnSubtype(int) = 0;
        virtual int ColumnSize(int) = 0;
        virtual int ColumnScale(int) = 0;
        virtual int Columns() = 0;

        virtual bool ColumnUpdated(int) = 0;
        virtual bool Updated() = 0;

        virtual Database DatabasePtr() const = 0;
        virtual Transaction TransactionPtr() const = 0;

        virtual IRow* Clone() = 0;
        virtual IRow* AddRef() = 0;
        virtual void Release() = 0;

        virtual ~IRow() {}
    };

    /* IStatement is the interface to the statements execution in IBPP.
     * Statement is the object class you actually use in your programming. A
     * Statement object is the work horse of IBPP. All your data manipulation
     * statements will be done through it. It is also used to access the result
     * set of a query (when the statement is such), one row at a time and in
     * strict forward direction. */

    class IStatement
    {
    public:
        virtual void Prepare(const std::string&) = 0;
        virtual void Execute() = 0;
        virtual void Execute(const std::string&) = 0;
        virtual void ExecuteImmediate(const std::string&) = 0;
        virtual void CursorExecute(const std::string& cursor) = 0;
        virtual void CursorExecute(const std::string& cursor, const std::string&) = 0;
        virtual bool Fetch() = 0;
        virtual bool Fetch(Row&) = 0;
        virtual int AffectedRows() = 0;
        virtual void Close() = 0;
        virtual std::string& Sql() = 0;
        virtual STT Type() = 0;

        virtual void SetNull(int) = 0;
        virtual void Set(int, bool) = 0;
        virtual void Set(int, const void*, int) = 0;        // byte buffers
        virtual void Set(int, const char*) = 0;             // c-string
        virtual void Set(int, const std::string&) = 0;
        virtual void Set(int, int16_t value) = 0;
        virtual void Set(int, int32_t value) = 0;
        virtual void Set(int, int64_t value) = 0;
        virtual void Set(int, float value) = 0;
        virtual void Set(int, double value) = 0;
        virtual void Set(int, const Timestamp& value) = 0;
        virtual void Set(int, const Date& value) = 0;
        virtual void Set(int, const Time& value) = 0;
        virtual void Set(int, const DBKey& value) = 0;
        virtual void Set(int, const Blob& value) = 0;
        virtual void Set(int, const Array& value) = 0;

        virtual void SetNull(std::string) = 0;
        virtual void Set(std::string, bool) = 0;
        virtual void Set(std::string, const void*, int) = 0;        // byte buffers
        virtual void Set(std::string, const char*) = 0;             // c-string
        virtual void Set(std::string, const std::string&) = 0;
        virtual void Set(std::string, int16_t value) = 0;
        virtual void Set(std::string, int32_t value) = 0;
        virtual void Set(std::string, int64_t value) = 0;
        virtual void Set(std::string, float value) = 0;
        virtual void Set(std::string, double value) = 0;
        virtual void Set(std::string, const Timestamp& value) = 0;
        virtual void Set(std::string, const Date& value) = 0;
        virtual void Set(std::string, const Time& value) = 0;
        virtual void Set(std::string, const DBKey& value) = 0;
        virtual void Set(std::string, const Blob& value) = 0;
        virtual void Set(std::string, const Array& value) = 0;

        virtual std::vector<std::string> ParametersByName() = 0;
        virtual std::vector<int> FindParamsByName(std::string name) = 0;
        virtual int ParameterNum(const std::string& name) = 0;


        virtual bool IsNull(int) = 0;
        virtual bool Get(int, bool&) = 0;
        virtual bool Get(int, void*, int&) = 0; // byte buffers
        virtual bool Get(int, std::string&) = 0;
        virtual bool Get(int, int16_t&) = 0;
        virtual bool Get(int, int32_t&) = 0;
        virtual bool Get(int, int64_t&) = 0;
        virtual bool Get(int, IBPP::ibpp_int128_t&) = 0;
        virtual bool Get(int, float&) = 0;
        virtual bool Get(int, double&) = 0;
        virtual bool Get(int, IBPP::ibpp_dec16_t&) = 0;
        virtual bool Get(int, IBPP::ibpp_dec34_t&) = 0;
        virtual bool Get(int, Timestamp& value) = 0;
        virtual bool Get(int, Date& value) = 0;
        virtual bool Get(int, Time& value) = 0;
        virtual bool Get(int, DBKey& value) = 0;
        virtual bool Get(int, Blob& value) = 0;
        virtual bool Get(int, Array& value) = 0;

        virtual bool IsNull(const std::string&) = 0;
        virtual bool Get(const std::string&, bool&) = 0;
        virtual bool Get(const std::string&, void*, int&) = 0;  // byte buffers
        virtual bool Get(const std::string&, std::string&) = 0;
        virtual bool Get(const std::string&, int16_t&) = 0;
        virtual bool Get(const std::string&, int32_t&) = 0;
        virtual bool Get(const std::string&, int64_t&) = 0;
        virtual bool Get(const std::string&, float&) = 0;
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
        virtual int ColumnSubtype(int) = 0;
        virtual int ColumnSize(int) = 0;
        virtual int ColumnScale(int) = 0;
        virtual int Columns() = 0;

        virtual SDT ParameterType(int) = 0;
        virtual int ParameterSubtype(int) = 0;
        virtual int ParameterSize(int) = 0;
        virtual int ParameterScale(int) = 0;
        virtual int Parameters() = 0;

        virtual void Plan(std::string&) = 0;

        virtual Database DatabasePtr() const = 0;
        virtual Transaction TransactionPtr() const = 0;

        virtual IStatement* AddRef() = 0;
        virtual void Release() = 0;

        virtual ~IStatement() { }

        // DEPRECATED METHODS (WON'T BE AVAILABLE IN VERSIONS 3.x)
        virtual bool Get(int, char*) = 0;                   // DEPRECATED
        virtual bool Get(const std::string&, char*) = 0;    // DEPRECATED
        virtual bool Get(int, bool*) = 0;                   // DEPRECATED
        virtual bool Get(const std::string&, bool*) = 0;    // DEPRECATED
        virtual bool Get(int, int16_t*) = 0;                // DEPRECATED
        virtual bool Get(const std::string&, int16_t*) = 0; // DEPRECATED
        virtual bool Get(int, int32_t*) = 0;                // DEPRECATED
        virtual bool Get(const std::string&, int32_t*) = 0; // DEPRECATED
        virtual bool Get(int, int64_t*) = 0;                // DEPRECATED
        virtual bool Get(const std::string&, int64_t*) = 0; // DEPRECATED
        virtual bool Get(int, float*) = 0;                  // DEPRECATED
        virtual bool Get(const std::string&, float*) = 0;   // DEPRECATED
        virtual bool Get(int, double*) = 0;                 // DEPRECATED
        virtual bool Get(const std::string&, double*) = 0;  // DEPRECATED
    };

    class IEvents
    {
    public:
        virtual void Add(const std::string&, EventInterface*) = 0;
        virtual void Drop(const std::string&) = 0;
        virtual void List(std::vector<std::string>&) = 0;
        virtual void Clear() = 0;               // Drop all events
        virtual void Dispatch() = 0;            // Dispatch events (calls handlers)

        virtual Database DatabasePtr() const = 0;

        virtual IEvents* AddRef() = 0;
        virtual void Release() = 0;

        virtual ~IEvents() { }
    };

    /* Class EventInterface is merely a pure interface.
     * It is _not_ implemented by IBPP. It is only a base class definition from
     * which your own event interface classes have to derive from.
     * Please read the reference guide at http://www.ibpp.org for more info. */

    class EventInterface
    {
    public:
        virtual void ibppEventHandler(Events, const std::string&, int) = 0;
        virtual ~EventInterface() { }
    };

    //  --- Factories ---
    //  These methods are the only way to get one of the above
    //  Interfaces.  They are at the heart of how you program using IBPP.  For
    //  instance, to get access to a database, you'll write code similar to this:
    //  {
    //      Database db = DatabaseFactory("server", "databasename",
    //                      "user", "password");
    //      db->Connect();
    //      ...
    //      db->Disconnect();
    //  }

    Service ServiceFactory(const std::string& ServerName,
        const std::string& UserName, const std::string& UserPassword, 
        const std::string& RoleName, const std::string& CharSet,
        const std::string& FBClient = "");

    Database DatabaseFactory(const std::string& ServerName,
        const std::string& DatabaseName, const std::string& UserName,
            const std::string& UserPassword, const std::string& RoleName,
                const std::string& CharSet, const std::string& CreateParams,
                    const std::string& FBClient = "");

    inline Database DatabaseFactory(const std::string& ServerName,
        const std::string& DatabaseName, const std::string& UserName,
            const std::string& UserPassword, const std::string& FBClient = "")
        { return DatabaseFactory(ServerName, DatabaseName, UserName, UserPassword, "", "", "", FBClient); }

    Transaction TransactionFactory(Database db, TAM am = amWrite,
        TIL il = ilConcurrency, TLR lr = lrWait, TFF flags = TFF(0));

    Statement StatementFactory(Database db, Transaction tr);

    inline Statement StatementFactory(Database db, Transaction tr,
        const std::string& sql)
    {
            Statement st = StatementFactory(db, tr);
            if (!sql.empty())
                st->Prepare(sql);
            return st;
    }

    Blob BlobFactory(Database db, Transaction tr);

    Array ArrayFactory(Database db, Transaction tr);

    Events EventsFactory(Database db);

    /* IBPP uses a self initialization system. Each time an object that may
     * require the usage of the Interbase client C-API library is used, the
     * library internal handling details are automatically initialized, if not
     * already done. You can kick this initialization at the start of an
     * application by calling IBPP::CheckVersion(). This is recommended, because
     * IBPP::CheckVersion will assure you that YOUR code has been compiled
     * against a compatible version of the library. */

    bool CheckVersion(uint32_t);

    /* On Win32 platform, ClientLibSearchPaths() allows to setup
     * one or multiple additional paths (separated with a ';') where IBPP
     * will look for the client library (before the default implicit search
     * locations). This is usefull for applications distributed with a 'private'
     * copy of Firebird, when the registry is useless to identify the location
     * from where to attempt loading the fbclient.dll / gds32.dll.
     * If called, this function must be called *early* by the application,
     * before *any* other function or object methods of IBPP.
     * Currently, this is a NO-OP on platforms other than Win32. */

    void ClientLibSearchPaths(const std::string&);

    /* Finally, here are some date and time conversion routines used by IBPP and
     * that may be helpful at the application level. They do not depend on
     * anything related to Firebird/Interbase. Just a bonus. dtoi and itod
     * return false on invalid parameters or out of range conversions. */

    bool dtoi(int date, int* py, int* pm, int* pd);
    bool itod(int* pdate, int year, int month, int day);
    void ttoi(int itime, int* phour, int* pminute, int* psecond, int* ptt);
    void itot(int* ptime, int hour, int minute, int second = 0, int tenthousandths = 0);

}

#endif
