/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#ifndef FR_DATABASE_BACKEND_H
#define FR_DATABASE_BACKEND_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <array>

namespace fr
{

typedef std::array<uint8_t, 8> DBKey;

enum class DatabaseBackend
{
    FbCpp,
    IBPP,
    Default
};

enum class ColumnType
{
    Unknown,
    Char,
    Varchar,
    Integer,
    BigInt,
    Float,
    Double,
    Boolean,
    Date,
    Time,
    Timestamp,
    Blob,
    Numeric,
    Decimal,
    Decfloat16,
    Decfloat34,
    Int128,
    TimeTz,
    TimestampTz
};

enum class StatementType
{
    Unknown,
    Select,
    Insert,
    Update,
    Delete,
    DDL,
    ExecProcedure,
    StartTransaction,
    Commit,
    Rollback,
    SetGenerator,
    Savepoint
};

struct DatabaseInfoData
{
    int ods;
    int odsMinor;
    int pageSize;
    int pages;
    int buffers;
    int sweep;
    bool forcedWrites;
    bool reserve;
    bool readOnly;

    int oldestTransaction;
    int oldestActiveTransaction;
    int oldestSnapshot;
    int nextTransaction;
};

struct CountInfo
{
    int inserts;
    int updates;
    int deletes;
    int readIndex;
    int readSequence;

    CountInfo() : inserts(0), updates(0), deletes(0), readIndex(0), readSequence(0) {}
};

struct UserData
{
    std::string username;
    std::string password;
    std::string firstName;
    std::string middleName;
    std::string lastName;
    uint32_t userId;
    uint32_t groupId;
};

enum class BackupFlags
{
    None = 0,
    IgnoreChecksums = 1,
    IgnoreLimbo = 2,
    MetadataOnly = 4,
    NoGarbageCollect = 8,
    NonTransportable = 16,
    ConvertExtTables = 32,
    Expand = 64, // No data compression
    OldDescriptions = 128,
    NoDBTriggers = 256,
    Zip = 512,
    Verbose = 1024,
    StatTime = 2048,
    StatDelta = 4096,
    StatPageReads = 8192,
    StatPageWrites = 16384
};

enum class RestoreFlags
{
    None = 0,
    DeactivateIndices = 1,
    NoShadow = 2,
    NoValidityCheck = 4,
    OneAtATime = 8,
    Replace = 16,
    Create = 32,
    UseAllSpace = 64,
    MetadataOnly = 128,
    Verbose = 256,
    PerTableCommit = 512,
    FixFssData = 1024,
    FixFssMetadata = 2048,
    ReadOnly = 4096,
    StatTime = 8192,
    StatDelta = 16384,
    StatPageReads = 32768,
    StatPageWrites = 65536
};

struct BackupConfig
{
    std::string dbPath;
    std::string backupPath;
    std::string outputFile;
    int factor = 0;
    BackupFlags flags = BackupFlags::None;
    std::string cryptPlugin;
    std::string keyHolder;
    std::string keyName;
    std::string skipData;
    std::string includeData;
    int interval = 0;
    int parallel = 0;
};

struct RestoreConfig
{
    std::string backupPath;
    std::string dbPath;
    std::string outputFile;
    RestoreFlags flags = RestoreFlags::None;
    int pageSize = 0;
    int cacheBuffers = 0;
    std::string cryptPlugin;
    std::string keyHolder;
    std::string keyName;
    std::string skipData;
    std::string includeData;
    int interval = 0;
    int parallel = 0;
};

enum class ShutdownMode
{
    Forced = 1,
    DenyTransactions = 2,
    DenyAttachments = 4
};

struct ShutdownConfig
{
    std::string dbPath;
    ShutdownMode mode = ShutdownMode::Forced;
    int timeout = 0;
};

// Common types and forward declarations
class IDatabase;
class ITransaction;
class IStatement;
class IService;
class IBlob;

typedef std::shared_ptr<IDatabase> IDatabasePtr;
typedef std::shared_ptr<ITransaction> ITransactionPtr;
typedef std::shared_ptr<IStatement> IStatementPtr;
typedef std::shared_ptr<IService> IServicePtr;
typedef std::shared_ptr<IBlob> IBlobPtr;

} // namespace fr

#endif // FR_DATABASE_BACKEND_H
