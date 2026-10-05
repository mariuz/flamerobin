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

#include <iterator>
#include <string>

#include "engine/Benchmark.h"

// All statements run inside the temporary benchmark database. The only
// exceptions are databaseInfo and databaseLinger, which read MON$DATABASE
// and RDB$DATABASE of the selected database when the user asks for it;
// nothing writes outside the FR_BENCHMARK_* objects. The SQL works on
// Firebird 3 and later. Server-side durations are measured with
// CAST('NOW' AS TIMESTAMP), which, unlike CURRENT_TIMESTAMP, is evaluated
// every time it is used.

namespace fr
{

namespace BenchmarkSql
{

std::vector<const char*> getStorageCreateStatements()
{
    return {
        "CREATE TABLE FR_BENCHMARK_TEST ("
        " ID INTEGER NOT NULL PRIMARY KEY,"
        " GRP INTEGER,"
        " CODE VARCHAR(40),"
        " AMOUNT DOUBLE PRECISION,"
        " CREATED TIMESTAMP,"
        " PAYLOAD VARCHAR(1000))",

        "CREATE INDEX FR_BENCHMARK_IDX_GRP ON FR_BENCHMARK_TEST (GRP)",
        "CREATE INDEX FR_BENCHMARK_IDX_CODE ON FR_BENCHMARK_TEST (CODE)",
        "CREATE INDEX FR_BENCHMARK_IDX_AMOUNT ON FR_BENCHMARK_TEST (AMOUNT)",
        "CREATE DESCENDING INDEX FR_BENCHMARK_IDX_CREATED"
        " ON FR_BENCHMARK_TEST (CREATED)",
        "CREATE INDEX FR_BENCHMARK_IDX_GRP_CODE ON FR_BENCHMARK_TEST (GRP, CODE)",

        // Server-side row generator: keeps the network out of the disk test.
        // The payload repeats a random UUID so that the record compression
        // cannot shrink it to almost nothing. GRP spreads the rows over 97
        // groups for the indexes and the prepare test.
        "CREATE PROCEDURE FR_BENCHMARK_FILL (FIRST_ID INTEGER, ROWS_TO_INSERT INTEGER,\n"
        "  PAYLOAD_LEN INTEGER)\n"
        "AS\n"
        "DECLARE I INTEGER = 0;\n"
        "BEGIN\n"
        "  WHILE (I < ROWS_TO_INSERT) DO\n"
        "  BEGIN\n"
        "    INSERT INTO FR_BENCHMARK_TEST (ID, GRP, CODE, AMOUNT, CREATED, PAYLOAD)\n"
        "    VALUES (:FIRST_ID + :I, MOD(:I, 97), UUID_TO_CHAR(GEN_UUID()), :I * 1.5,\n"
        "      CURRENT_TIMESTAMP, RPAD('', :PAYLOAD_LEN, UUID_TO_CHAR(GEN_UUID())));\n"
        "    I = I + 1;\n"
        "  END\n"
        "END"
    };
}

std::vector<const char*> getCreateStatements()
{
    std::vector<const char*> all = getStorageCreateStatements();
    const char* const others[] = {
        "CREATE GLOBAL TEMPORARY TABLE FR_BENCHMARK_GTT ("
        " ID INTEGER,"
        " VAL DOUBLE PRECISION,"
        " TXT VARCHAR(100))"
        " ON COMMIT DELETE ROWS",

        // Compute-bound loop; every 100th intermediate value goes to the GTT,
        // which lives in temporary space and therefore causes no database I/O.
        // Executable procedure: EXECUTE PROCEDURE opens no cursor.
        "CREATE PROCEDURE FR_BENCHMARK_CPU (ITERATIONS INTEGER)\n"
        "RETURNS (RESULT DOUBLE PRECISION, ELAPSED_MS INTEGER)\n"
        "AS\n"
        "DECLARE I INTEGER = 0;\n"
        "DECLARE X DOUBLE PRECISION = 0;\n"
        "DECLARE S VARCHAR(80) = 'FlameRobin Firebird benchmark 0123456789 "
        "abcdefghijklmnopqrstuvwxyz';\n"
        "DECLARE T VARCHAR(20);\n"
        "DECLARE T0 TIMESTAMP;\n"
        "BEGIN\n"
        "  T0 = CAST('NOW' AS TIMESTAMP);\n"
        "  WHILE (I < ITERATIONS) DO\n"
        "  BEGIN\n"
        "    I = I + 1;\n"
        "    X = X + SQRT(MOD(I, 1000) + 1);\n"
        "    T = SUBSTRING(S FROM MOD(I, 50) + 1 FOR 10);\n"
        "    X = X + CASE MOD(I, 4)\n"
        "      WHEN 0 THEN CHAR_LENGTH(T)\n"
        "      WHEN 1 THEN -1\n"
        "      WHEN 2 THEN ASCII_VAL(T)\n"
        "      ELSE 0 END;\n"
        "    IF (MOD(I, 100) = 0) THEN\n"
        "      INSERT INTO FR_BENCHMARK_GTT (ID, VAL, TXT) VALUES (:I, :X, :T);\n"
        "  END\n"
        "  SELECT COALESCE(SUM(VAL), 0) FROM FR_BENCHMARK_GTT INTO :RESULT;\n"
        "  RESULT = RESULT + X;\n"
        "  ELAPSED_MS = DATEDIFF(MILLISECOND FROM T0 TO CAST('NOW' AS TIMESTAMP));\n"
        "END",

        // Reads the same rows as the throughput test, but inside the server.
        // Comparing both durations separates server time from network and
        // client time. With SORTED = 1 the rows go through the sort module.
        "CREATE PROCEDURE FR_BENCHMARK_READ (SORTED INTEGER)\n"
        "RETURNS (ROWS_READ INTEGER, BYTES_READ DOUBLE PRECISION, ELAPSED_MS INTEGER)\n"
        "AS\n"
        "DECLARE V_ID INTEGER;\n"
        "DECLARE V_CODE VARCHAR(40);\n"
        "DECLARE V_PAYLOAD VARCHAR(1000);\n"
        "DECLARE T0 TIMESTAMP;\n"
        "BEGIN\n"
        "  ROWS_READ = 0;\n"
        "  BYTES_READ = 0;\n"
        "  T0 = CAST('NOW' AS TIMESTAMP);\n"
        "  IF (SORTED = 1) THEN\n"
        "    FOR SELECT ID, CODE, PAYLOAD FROM FR_BENCHMARK_TEST ORDER BY PAYLOAD\n"
        "      INTO :V_ID, :V_CODE, :V_PAYLOAD DO\n"
        "    BEGIN\n"
        "      ROWS_READ = ROWS_READ + 1;\n"
        "      BYTES_READ = BYTES_READ + OCTET_LENGTH(V_PAYLOAD) + OCTET_LENGTH(V_CODE) + 4;\n"
        "    END\n"
        "  ELSE\n"
        "    FOR SELECT ID, CODE, PAYLOAD FROM FR_BENCHMARK_TEST\n"
        "      INTO :V_ID, :V_CODE, :V_PAYLOAD DO\n"
        "    BEGIN\n"
        "      ROWS_READ = ROWS_READ + 1;\n"
        "      BYTES_READ = BYTES_READ + OCTET_LENGTH(V_PAYLOAD) + OCTET_LENGTH(V_CODE) + 4;\n"
        "    END\n"
        "  ELAPSED_MS = DATEDIFF(MILLISECOND FROM T0 TO CAST('NOW' AS TIMESTAMP));\n"
        "  SUSPEND;\n"
        "END",

        // target of the row-by-row vs. server-side batch comparison and of
        // the inserts of the OLTP workload
        "CREATE TABLE FR_BENCHMARK_ROWS ("
        " ID INTEGER NOT NULL PRIMARY KEY,"
        " PAYLOAD VARCHAR(200))",

        "CREATE PROCEDURE FR_BENCHMARK_FILL_ROWS (FIRST_ID INTEGER,\n"
        "  ROWS_TO_INSERT INTEGER)\n"
        "AS\n"
        "DECLARE I INTEGER = 0;\n"
        "BEGIN\n"
        "  WHILE (I < ROWS_TO_INSERT) DO\n"
        "  BEGIN\n"
        "    INSERT INTO FR_BENCHMARK_ROWS (ID, PAYLOAD)\n"
        "    VALUES (:FIRST_ID + :I, 'row by row vs. batch');\n"
        "    I = I + 1;\n"
        "  END\n"
        "END",

        // a few rows that are updated over and over again: record versions
        "CREATE TABLE FR_BENCHMARK_HOT ("
        " ID INTEGER NOT NULL PRIMARY KEY,"
        " VAL INTEGER,"
        " TXT VARCHAR(100))",

        "CREATE TABLE FR_BENCHMARK_BLOB ("
        " ID INTEGER NOT NULL PRIMARY KEY,"
        " DATA BLOB SUB_TYPE BINARY)",

        // lookups of the OLTP workload as executable procedures: the parallel
        // clients then need no cursors (see BenchmarkRunner::measureOltp)
        "CREATE PROCEDURE FR_BENCHMARK_POINT (ID_VALUE INTEGER)\n"
        "RETURNS (CODE VARCHAR(40), AMOUNT DOUBLE PRECISION)\n"
        "AS\n"
        "BEGIN\n"
        "  SELECT CODE, AMOUNT FROM FR_BENCHMARK_TEST WHERE ID = :ID_VALUE\n"
        "    INTO :CODE, :AMOUNT;\n"
        "END",

        // a range of 100 neighbouring rows by primary key, so that the
        // cost does not depend on the size of the table
        "CREATE PROCEDURE FR_BENCHMARK_RANGE (FIRST_ID INTEGER)\n"
        "RETURNS (ROW_TOTAL INTEGER)\n"
        "AS\n"
        "BEGIN\n"
        "  SELECT COUNT(*) FROM FR_BENCHMARK_TEST\n"
        "    WHERE ID BETWEEN :FIRST_ID AND :FIRST_ID + 99\n"
        "    INTO :ROW_TOTAL;\n"
        "END",

        // Primary key lookups spread over ID_COUNT rows (7919 is prime, so
        // the IDs jump around); with a minimal page cache nearly every
        // lookup reads pages from the drive. Inside the server, so the
        // network does not count.
        "CREATE PROCEDURE FR_BENCHMARK_LOOKUPS (FIRST_ID INTEGER, ID_COUNT INTEGER,\n"
        "  START_AT INTEGER, LOOKUPS INTEGER)\n"
        "RETURNS (FOUND INTEGER)\n"
        "AS\n"
        "DECLARE I INTEGER;\n"
        "DECLARE V_CODE VARCHAR(40);\n"
        "BEGIN\n"
        "  FOUND = 0;\n"
        "  I = START_AT;\n"
        "  WHILE (I < START_AT + LOOKUPS) DO\n"
        "  BEGIN\n"
        "    V_CODE = NULL;\n"
        "    SELECT CODE FROM FR_BENCHMARK_TEST\n"
        "      WHERE ID = :FIRST_ID + MOD(CAST(:I AS BIGINT) * 7919, :ID_COUNT)\n"
        "      INTO :V_CODE;\n"
        "    IF (V_CODE IS NOT NULL) THEN FOUND = FOUND + 1;\n"
        "    I = I + 1;\n"
        "  END\n"
        "END",

        // Sorts COPIES copies of the test rows. With more data than the
        // sort memory (TempCacheLimit) Firebird writes temporary files into
        // TempDirectories.
        "CREATE PROCEDURE FR_BENCHMARK_TEMP_SORT (COPIES INTEGER)\n"
        "RETURNS (ROWS_SORTED INTEGER, BYTES_SORTED DOUBLE PRECISION, ELAPSED_MS INTEGER)\n"
        "AS\n"
        "DECLARE V_COPY INTEGER;\n"
        "DECLARE V_PAYLOAD VARCHAR(1000);\n"
        "DECLARE T0 TIMESTAMP;\n"
        "BEGIN\n"
        "  ROWS_SORTED = 0;\n"
        "  BYTES_SORTED = 0;\n"
        "  T0 = CAST('NOW' AS TIMESTAMP);\n"
        "  FOR WITH RECURSIVE C (N) AS (\n"
        "      SELECT 1 FROM RDB$DATABASE\n"
        "      UNION ALL SELECT N + 1 FROM C WHERE N < :COPIES)\n"
        "    SELECT C.N, T.PAYLOAD FROM FR_BENCHMARK_TEST T CROSS JOIN C\n"
        "    ORDER BY T.PAYLOAD DESC, C.N\n"
        "    INTO :V_COPY, :V_PAYLOAD DO\n"
        "  BEGIN\n"
        "    ROWS_SORTED = ROWS_SORTED + 1;\n"
        "    BYTES_SORTED = BYTES_SORTED + OCTET_LENGTH(V_PAYLOAD) + 4;\n"
        "  END\n"
        "  ELAPSED_MS = DATEDIFF(MILLISECOND FROM T0 TO CAST('NOW' AS TIMESTAMP));\n"
        "  SUSPEND;\n"
        "END"
    };
    all.insert(all.end(), std::begin(others), std::end(others));
    all.push_back(getWideTableStatement());
    return all;
}

// 100 columns: ID and 33 columns each of INTEGER, VARCHAR(20) and DOUBLE
// PRECISION, like a wide table of a data entry application
const char* getWideTableStatement()
{
    static const std::string sql = []()
    {
        std::string s = "CREATE TABLE FR_BENCHMARK_WIDE (ID INTEGER NOT NULL PRIMARY KEY";
        for (int i = 1; i < wideColumns; ++i)
        {
            const char* type = i <= 33 ? "INTEGER" : i <= 66 ? "VARCHAR(20)"
                : "DOUBLE PRECISION";
            s += ", C" + std::to_string(i) + " " + type;
        }
        return s + ")";
    }();
    return sql.c_str();
}

const char* getWideInsertStatement()
{
    static const std::string sql = []()
    {
        std::string columns = "ID";
        std::string values = "?";
        for (int i = 1; i < wideColumns; ++i)
        {
            columns += ", C" + std::to_string(i);
            values += ", ?";
        }
        return "INSERT INTO FR_BENCHMARK_WIDE (" + columns + ") VALUES (" + values + ")";
    }();
    return sql.c_str();
}

const char* const engineVersion =
    "SELECT RDB$GET_CONTEXT('SYSTEM', 'ENGINE_VERSION') FROM RDB$DATABASE";

const char* const attachmentInfo =
    "SELECT MON$REMOTE_PROTOCOL, MON$REMOTE_ADDRESS, MON$CLIENT_VERSION"
    " FROM MON$ATTACHMENTS WHERE MON$ATTACHMENT_ID = CURRENT_CONNECTION";

// Firebird 3+
const char* const attachmentWireInfo =
    "SELECT CASE WHEN MON$WIRE_COMPRESSED THEN 1 ELSE 0 END,"
    " CASE WHEN MON$WIRE_ENCRYPTED THEN 1 ELSE 0 END"
    " FROM MON$ATTACHMENTS WHERE MON$ATTACHMENT_ID = CURRENT_CONNECTION";

const char* const databaseInfo =
    "SELECT MON$DATABASE_NAME, CAST(MON$BACKUP_STATE AS INTEGER),"
    " MON$PAGE_BUFFERS FROM MON$DATABASE";

const char* const databaseLinger =
    "SELECT RDB$LINGER FROM RDB$DATABASE";

const char* const sessionUser =
    "SELECT CURRENT_USER, CURRENT_ROLE FROM RDB$DATABASE";

// Firebird 3+: how this connection was made
const char* const attachmentDetails =
    "SELECT MON$AUTH_METHOD, MON$REMOTE_VERSION, MON$REMOTE_HOST"
    " FROM MON$ATTACHMENTS WHERE MON$ATTACHMENT_ID = CURRENT_CONNECTION";

// Firebird 4+: wire encryption plugin and the timeouts of this connection
// (0: none), which may close idle connections
const char* const attachmentTimeouts =
    "SELECT MON$WIRE_CRYPT_PLUGIN, MON$IDLE_TIMEOUT, MON$STATEMENT_TIMEOUT"
    " FROM MON$ATTACHMENTS WHERE MON$ATTACHMENT_ID = CURRENT_CONNECTION";

// Firebird 3+: the security database used for the logins
const char* const securityDatabase =
    "SELECT MON$SEC_DATABASE FROM MON$DATABASE";

// Firebird 4+; returns rows only for users with administrator rights:
// the settings the benchmark evaluates, and every setting that
// firebird.conf sets explicitly (RDB$CONFIG_IS_SET)
const char* const serverConfig =
    "SELECT TRIM(RDB$CONFIG_NAME), RDB$CONFIG_VALUE,"
    " CASE WHEN RDB$CONFIG_IS_SET THEN 1 ELSE 0 END FROM RDB$CONFIG"
    " WHERE RDB$CONFIG_IS_SET OR RDB$CONFIG_NAME IN ('ServerMode',"
    " 'DefaultDbCachePages', 'TempCacheLimit', 'TempDirectories',"
    " 'UseFileSystemCache', 'FileSystemCacheThreshold', 'WireCompression',"
    " 'WireCrypt', 'TcpRemoteBufferSize', 'TcpNoNagle', 'CpuAffinityMask',"
    " 'GCPolicy', 'ParallelWorkers', 'MaxParallelWorkers',"
    " 'MaxStatementCacheSize', 'LockHashSlots', 'RemoteServicePort',"
    " 'RemoteAuxPort', 'RemoteBindAddress', 'ConnectionIdleTimeout',"
    " 'StatementTimeout', 'DummyPacketInterval', 'AuthServer')";

// an event for the event delivery test (RemoteAuxPort)
const char* const postEvent =
    "EXECUTE BLOCK AS BEGIN POST_EVENT 'FR_BENCHMARK_EVENT'; END";

const char* const eventName = "FR_BENCHMARK_EVENT";

const char* const driveLookups =
    "EXECUTE PROCEDURE FR_BENCHMARK_LOOKUPS(?, ?, ?, ?)";

const char* const tempSort =
    "SELECT ROWS_SORTED, BYTES_SORTED, ELAPSED_MS FROM FR_BENCHMARK_TEMP_SORT(?)";

const char* const cpuTest =
    "EXECUTE PROCEDURE FR_BENCHMARK_CPU(?)";

const char* const commitInsert =
    "INSERT INTO FR_BENCHMARK_TEST (ID, GRP, CODE, AMOUNT, CREATED, PAYLOAD)"
    " VALUES (?, 0, 'COMMIT', 0, CURRENT_TIMESTAMP, 'commit latency test')";

const char* const fillTable =
    "EXECUTE PROCEDURE FR_BENCHMARK_FILL(?, ?, ?)";

// Changes every indexed column, so that every index gets new entries. Update
// and delete work on ID ranges so that the runner can check for
// cancellation between the chunks; all chunks run in one transaction.
const char* const bulkUpdate =
    "UPDATE FR_BENCHMARK_TEST SET"
    " GRP = MOD(GRP + 13, 97),"
    " CODE = REVERSE(CODE),"
    " AMOUNT = AMOUNT + 1.5,"
    " CREATED = DATEADD(1 SECOND TO CREATED),"
    " PAYLOAD = REVERSE(PAYLOAD)"
    " WHERE ID BETWEEN ? AND ?";

const char* const bulkDelete =
    "DELETE FROM FR_BENCHMARK_TEST WHERE ID BETWEEN ? AND ?";

// A full scan after a committed mass delete makes the engine visit all
// deleted records; with cooperative garbage collection (Classic,
// SuperClassic, GCPolicy = cooperative or combined) the scan removes them
const char* const garbageCollect =
    "SELECT COUNT(*) FROM FR_BENCHMARK_TEST";

const char* const scan =
    "SELECT COUNT(*), CAST(SUM(CHAR_LENGTH(PAYLOAD)) AS DOUBLE PRECISION)"
    " FROM FR_BENCHMARK_TEST";

const char* const serverRead =
    "SELECT ROWS_READ, BYTES_READ, ELAPSED_MS FROM FR_BENCHMARK_READ(?)";

const char* const latency =
    "SELECT 1 FROM RDB$DATABASE";

const char* const largeRoundTrip =
    "SELECT CAST(RPAD('', 8000, 'x') AS VARCHAR(8000) CHARACTER SET OCTETS)"
    " FROM RDB$DATABASE";

const char* const throughput =
    "SELECT ID, CODE, PAYLOAD FROM FR_BENCHMARK_TEST";

const char* const ioStats =
    "SELECT io.MON$PAGE_READS, io.MON$PAGE_WRITES,"
    " io.MON$PAGE_FETCHES, io.MON$PAGE_MARKS"
    " FROM MON$ATTACHMENTS a"
    " JOIN MON$IO_STATS io ON io.MON$STAT_ID = a.MON$STAT_ID"
    " WHERE a.MON$ATTACHMENT_ID = CURRENT_CONNECTION";

const char* const recordStats =
    "SELECT rs.MON$RECORD_BACKOUTS, rs.MON$RECORD_PURGES,"
    " rs.MON$RECORD_EXPUNGES"
    " FROM MON$ATTACHMENTS a"
    " JOIN MON$RECORD_STATS rs ON rs.MON$STAT_ID = a.MON$STAT_ID"
    " WHERE a.MON$ATTACHMENT_ID = CURRENT_CONNECTION";

const char* const rowInsert =
    "INSERT INTO FR_BENCHMARK_ROWS (ID, PAYLOAD) VALUES (?, 'row by row vs. batch')";

const char* const fillRows =
    "EXECUTE PROCEDURE FR_BENCHMARK_FILL_ROWS(?, ?)";

// a typical application query; prepared again and again
const char* const prepareProbe =
    "SELECT t.ID, t.CODE, t.AMOUNT,"
    " (SELECT COUNT(*) FROM FR_BENCHMARK_ROWS r WHERE r.ID = t.ID)"
    " FROM FR_BENCHMARK_TEST t"
    " WHERE t.GRP = ? AND t.AMOUNT > ?"
    " ORDER BY t.CREATED DESC"
    " ROWS 10";

// mixed OLTP transaction of a typical read/write workload: point
// lookups, a range count, an update and an insert. The lookups are
// executable procedures, so the parallel clients never open a cursor.
const char* const oltpPointSelect =
    "EXECUTE PROCEDURE FR_BENCHMARK_POINT(?)";

const char* const oltpRangeSelect =
    "EXECUTE PROCEDURE FR_BENCHMARK_RANGE(?)";

const char* const oltpUpdate =
    "UPDATE FR_BENCHMARK_TEST SET AMOUNT = AMOUNT + 1 WHERE ID = ?";

const char* const oltpInsert =
    "INSERT INTO FR_BENCHMARK_ROWS (ID, PAYLOAD) VALUES (?, 'oltp')";

const char* const blobInsert =
    "INSERT INTO FR_BENCHMARK_BLOB (ID, DATA) VALUES (?, ?)";

const char* const blobRead =
    "SELECT DATA FROM FR_BENCHMARK_BLOB";

const char* const hotFill =
    "INSERT INTO FR_BENCHMARK_HOT (ID, VAL, TXT)"
    " SELECT ID, 0, CODE FROM FR_BENCHMARK_TEST WHERE ID <= 100";

const char* const hotUpdate =
    "UPDATE FR_BENCHMARK_HOT SET VAL = VAL + 1";

const char* const hotRead =
    "SELECT SUM(VAL) FROM FR_BENCHMARK_HOT";

// only the benchmark's own connections are attached to its database
const char* const serverProcesses =
    "SELECT COUNT(DISTINCT MON$SERVER_PID) FROM MON$ATTACHMENTS"
    " WHERE MON$SYSTEM_FLAG = 0";

} // namespace BenchmarkSql

} // namespace fr
