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

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "ibpp/ibpp.h"

namespace
{

std::string makeIdentifier(const std::string& prefix, char fill, size_t len)
{
    std::string id(prefix);
    if (id.length() < len)
        id.append(len - id.length(), fill);
    return id;
}

std::string quoteIdentifier(const std::string& id)
{
    return "\"" + id + "\"";
}

bool check(bool condition, const char* testName)
{
    if (condition)
        return true;
    std::cerr << testName << " failed.\n";
    return false;
}

bool checkStr(const char* actual, const std::string& expected, const char* testName)
{
    const std::string actualStr(actual ? actual : "");
    if (actualStr == expected)
        return true;
    std::cerr << testName << " failed.\n"
        << "  Expected: [" << expected << "]\n"
        << "  Actual:   [" << actualStr << "]\n";
    return false;
}

// Firebird system-table columns are CHAR and are padded with trailing spaces.
// Trim them before comparing.
std::string rtrim(std::string s)
{
    const std::string::size_type end = s.find_last_not_of(' ');
    if (end == std::string::npos)
        s.clear();
    else
        s.erase(end + 1);
    return s;
}

} // namespace

int main()
{
    bool ok = true;

    const char* envServer = std::getenv("IBPP_TEST_SERVER");
    const std::string serverName = envServer ? envServer : "";
    if (serverName.empty())
    {
        std::cout << "IBPP_TEST_SERVER is not set, skipping ibpp_row_metadata_test.\n";
        return 0;
    }

    // Build a temporary database path that works on both POSIX and Windows.
#ifdef _WIN32
    char tmpDir[MAX_PATH];
    DWORD tmpLen = GetTempPathA(MAX_PATH, tmpDir);
    std::string dbName = (tmpLen > 0 ? std::string(tmpDir, tmpLen) : "C:\\Temp\\") +
        "flamerobin_row_metadata_test_" +
        std::to_string(static_cast<long long>(std::time(0))) + ".fdb";
#else
    const std::string dbName = "/tmp/flamerobin_row_metadata_test_" +
        std::to_string(static_cast<long long>(std::time(0))) + ".fdb";
#endif

    IBPP::Database db;

    try
    {
        db = IBPP::DatabaseFactory(serverName, dbName, "SYSDBA", "masterkey");
        db->Create(3);
        db->Connect();

        int odsMajor = 0;
        db->Info(&odsMajor, 0, 0, 0, 0, 0, 0, 0, 0);
        int idLen = (odsMajor >= 13 ? 63 : 31);
        std::cout << "Detected ODS " << odsMajor << ". Testing identifiers with length: " << idLen << "\n";

        const std::string tableName = makeIdentifier("TBL_", 'T', idLen);
        const std::string columnName = makeIdentifier("COL_", 'C', idLen);
        const std::string aliasName = makeIdentifier("ALIAS_", 'A', idLen);

        // DDL transaction: Firebird 3.0 requires DDL to be committed before
        // the new table is visible to subsequent DML in a fresh transaction.
        IBPP::Transaction tr = IBPP::TransactionFactory(db);
        tr->Start();

        IBPP::Statement st = IBPP::StatementFactory(db, tr);
        st->Execute("CREATE TABLE " + quoteIdentifier(tableName) + " (" +
            quoteIdentifier(columnName) + " INTEGER)");
        tr->Commit();

        // DML transaction
        tr->Start();
        st->Execute("INSERT INTO " + quoteIdentifier(tableName) + " (" +
            quoteIdentifier(columnName) + ") VALUES (1)");

        IBPP::Statement query = IBPP::StatementFactory(db, tr);
        query->Prepare("SELECT t." + quoteIdentifier(columnName) + " AS " +
            quoteIdentifier(aliasName) + " FROM " + quoteIdentifier(tableName) + " t");
        query->Execute();

        ok = check(query->Fetch(), "fetch result row") && ok;
        ok = check(query->Columns() == 1, "single result column") && ok;

        ok = checkStr(query->ColumnName(1), columnName, "column name is not truncated") && ok;
        ok = checkStr(query->ColumnAlias(1), aliasName, "column alias is not truncated") && ok;
        ok = checkStr(query->ColumnTable(1), tableName, "column table is not truncated") && ok;

        ok = check(query->ColumnNum(columnName) == 1, "ColumnNum by name") && ok;
        ok = check(query->ColumnNum(aliasName) == 1, "ColumnNum by alias") && ok;
        ok = check(query->ColumnNum(makeIdentifier("alias_", 'a', idLen)) == 1,
            "ColumnNum is case-insensitive for aliases") && ok;

        tr->Commit();

        // Regression coverage for PR #510:
        // Relation metadata query must use f.rdb$character_set_id in the
        // rdb$collations join and execute successfully on Firebird 3/4/5/6.
        const std::string domainName = makeIdentifier("DM_", 'D', idLen);
        const std::string relationName = makeIdentifier("REL_", 'R', idLen);
        const std::string relationColumnName = makeIdentifier("TXT_", 'X', idLen);

        tr->Start();
        st->Execute("CREATE DOMAIN " + quoteIdentifier(domainName) +
            " AS VARCHAR(20) CHARACTER SET UTF8");
        st->Execute("CREATE TABLE " + quoteIdentifier(relationName) + " (" +
            quoteIdentifier(relationColumnName) + " " + quoteIdentifier(domainName) +
            " COLLATE UNICODE)");
        tr->Commit();

        tr->Start();
        IBPP::Statement metadataQuery = IBPP::StatementFactory(db, tr);
        metadataQuery->Prepare(
            "select r.rdb$field_name, r.rdb$null_flag, r.rdb$field_source,"
            " l.rdb$collation_name, f.rdb$computed_source, r.rdb$default_source,"
            " r.rdb$description"
            " from rdb$fields f"
            " join rdb$relation_fields r"
            "     on f.rdb$field_name=r.rdb$field_source"
            " left outer join rdb$collations l"
            "     on l.rdb$collation_id = coalesce(r.rdb$collation_id, f.rdb$collation_id)"
            " and l.rdb$character_set_id = f.rdb$character_set_id"
            " where r.rdb$relation_name = ?"
            " order by r.rdb$field_position");
        metadataQuery->Set(1, relationName);
        metadataQuery->Execute();

        const bool hasMetadataRow = metadataQuery->Fetch();
        ok = check(hasMetadataRow, "relation metadata query returned rows") && ok;
        if (hasMetadataRow)
        {
            std::string metadataFieldName;
            metadataQuery->Get(1, metadataFieldName);
            ok = checkStr(rtrim(metadataFieldName).c_str(), relationColumnName,
                "relation metadata field name") && ok;

            if (metadataQuery->IsNull(4))
            {
                ok = check(false, "relation metadata collation is null") && ok;
            }
            else
            {
                std::string collationName;
                metadataQuery->Get(4, collationName);
                ok = checkStr(rtrim(collationName).c_str(), "UNICODE",
                    "relation metadata collation join") && ok;
            }
        }

        tr->Rollback();

        // Regression coverage for issue #368:
        // Ensure TIMESTAMP WITH TIME ZONE values are fetched successfully from
        // a quoted table name containing a dot (Firebird 4.0+ only).
        if (odsMajor >= 13)
        {
            const std::string tzTableName = "stg.Actions";
            const std::string tzColumnName = "Time_Stamp";

            try
            {
                tr->Start();
                st->Execute("CREATE TABLE " + quoteIdentifier(tzTableName) + " (" +
                    quoteIdentifier(tzColumnName) + " TIMESTAMP WITH TIME ZONE NOT NULL)");
                tr->Commit();

                tr->Start();
                st->Execute("INSERT INTO " + quoteIdentifier(tzTableName) + " (" +
                    quoteIdentifier(tzColumnName) + ") VALUES ('2022-05-19 16:27:11.0000 +00:00')");

                IBPP::Statement tzQuery = IBPP::StatementFactory(db, tr);
                tzQuery->Prepare("SELECT a." + quoteIdentifier(tzColumnName) +
                    " FROM " + quoteIdentifier(tzTableName) + " a");
                tzQuery->Execute();

                ok = check(tzQuery->Fetch(), "timestamp with time zone row fetch") && ok;
                ok = check(tzQuery->Columns() == 1, "timestamp with time zone column count") && ok;
                ok = check(tzQuery->ColumnType(1) == IBPP::sdTimestampTz,
                    "timestamp with time zone column type") && ok;
                ok = checkStr(tzQuery->ColumnName(1), tzColumnName,
                    "timestamp with time zone column name") && ok;
                ok = checkStr(tzQuery->ColumnTable(1), tzTableName,
                    "timestamp with time zone table name") && ok;

                IBPP::Timestamp timestampTz;
                ok = check(!tzQuery->Get(1, timestampTz), "timestamp with time zone value read") && ok;

                int year = 0, month = 0, day = 0;
                int hour = 0, minute = 0, second = 0;
                timestampTz.GetDate(year, month, day);
                timestampTz.GetTime(hour, minute, second);

                ok = check(year == 2022 && month == 5 && day == 19,
                    "timestamp with time zone date value") && ok;
                ok = check(hour == 16 && minute == 27 && second == 11,
                    "timestamp with time zone time value") && ok;
                ok = check(timestampTz.GetTimezone() != IBPP::Time::TZ_NONE,
                    "timestamp with time zone timezone is present") && ok;

                tr->Rollback();
            }
            catch (const IBPP::Exception& e)
            {
                // The system Firebird client library may be an older version
                // (e.g. Firebird 3.x from the OS package manager) that does
                // not understand the TIMESTAMP WITH TIME ZONE wire type
                // introduced in Firebird 4.  In that case isc_dsql_prepare
                // returns engine code 335544573 ("Data type unknown").  Treat
                // this as a skip rather than a test failure; any other
                // IBPP exception is re-thrown so real regressions still fail.
                std::string msg(e.what());
                if (msg.find("Data type unknown") != std::string::npos)
                {
                    std::cout << "Skipping issue #368 TZ scenario: client "
                                 "library does not support TIMESTAMP WITH TIME "
                                 "ZONE (upgrade to Firebird 4+ client).\n";
                    try { tr->Rollback(); } catch (...) {}
                }
                else
                {
                    throw;
                }
            }
        }
        else
        {
            std::cout << "Skipping issue #368 regression scenario: requires Firebird 4.0+.\n";
        }

        // Regression coverage for issue #338:
        // After applying SET BIND OF TIME WITH TIME ZONE TO LEGACY and
        // SET BIND OF TIMESTAMP WITH TIME ZONE TO LEGACY, the values of
        // LOCALTIME and CURRENT_TIME (and CURRENT_TIMESTAMP) must represent
        // the same local moment - i.e. the time zone conversion must not be
        // applied a second time by FlameRobin/IBPP.
        // Requires Firebird 4.0+ (ODS 13+) which introduced TIME WITH TIME ZONE.
        if (odsMajor >= 13)
        {
            try
            {
                tr->Start();

                // Pin the session time zone to a fixed UTC offset so the test
                // is deterministic regardless of the server's system clock.
                st->Execute("SET TIME ZONE '+00:00'");
                st->Execute("SET BIND OF TIME WITH TIME ZONE TO LEGACY");
                st->Execute("SET BIND OF TIMESTAMP WITH TIME ZONE TO LEGACY");

                IBPP::Statement tzQuery = IBPP::StatementFactory(db, tr);
                tzQuery->Prepare(
                    "SELECT localtime, current_time, CURRENT_TIMESTAMP"
                    " FROM rdb$database");
                tzQuery->Execute();

                ok = check(tzQuery->Fetch(),
                    "issue#338: fetched a row") && ok;
                ok = check(tzQuery->Columns() == 3,
                    "issue#338: three result columns") && ok;

                // With the legacy bind active both LOCALTIME and CURRENT_TIME
                // are returned as plain TIME (sdTime), not as TIME WITH TIME
                // ZONE (sdTimeTz).
                ok = check(tzQuery->ColumnType(1) == IBPP::sdTime,
                    "issue#338: LOCALTIME column type is sdTime") && ok;
                ok = check(tzQuery->ColumnType(2) == IBPP::sdTime,
                    "issue#338: CURRENT_TIME (legacy) column type is sdTime") && ok;
                ok = check(tzQuery->ColumnType(3) == IBPP::sdTimestamp,
                    "issue#338: CURRENT_TIMESTAMP (legacy) column type is sdTimestamp") && ok;

                IBPP::Time localTime;
                IBPP::Time currentTime;
                IBPP::Timestamp currentTimestamp;
                tzQuery->Get(1, localTime);
                tzQuery->Get(2, currentTime);
                tzQuery->Get(3, currentTimestamp);

                // LOCALTIME and CURRENT_TIME must carry the same local-clock
                // value.  Both are computed at statement execution time so
                // they are identical down to the sub-second.
                ok = check(localTime.GetTime() == currentTime.GetTime(),
                    "issue#338: LOCALTIME == CURRENT_TIME (legacy bind)") && ok;

                // The time portion of CURRENT_TIMESTAMP must match at
                // whole-second granularity.  CURRENT_TIMESTAMP defaults to 0
                // sub-second precision (tenthousandths == 0) while LOCALTIME
                // carries full sub-second precision, so exact GetTime()
                // equality is not guaranteed; compare seconds only.
                ok = check(localTime.GetTime() / 10000 ==
                               currentTimestamp.GetTime() / 10000,
                    "issue#338: LOCALTIME == CURRENT_TIMESTAMP time part (legacy bind)") && ok;

                tr->Rollback();
            }
            catch (const IBPP::Exception& e)
            {
                std::string msg(e.what());
                // An older fbclient (< 4.0) may not understand the SET BIND
                // or SET TIME ZONE statements.  Treat that as a skip.
                if (msg.find("feature is not supported") != std::string::npos ||
                    msg.find("Data type unknown") != std::string::npos ||
                    msg.find("token unknown") != std::string::npos)
                {
                    std::cout << "Skipping issue #338 TZ scenario: client "
                                 "library does not support SET TIME ZONE / "
                                 "SET BIND (upgrade to Firebird 4+ client).\n";
                    try { tr->Rollback(); } catch (...) {}
                }
                else
                {
                    throw;
                }
            }
        }
        else
        {
            std::cout << "Skipping issue #338 regression scenario: requires Firebird 4.0+.\n";
        }

        // Regression coverage for issue #436:
        // Creating and then dropping a trigger must not crash the application.
        // The FlameRobin tree observer (DBHTreeItemData) held a raw pointer
        // to the MetadataItem representing the trigger.  When the trigger was
        // dropped the MetadataItem was destroyed but the pointer was never
        // cleared, causing a read-access violation on the next UI action.
        // This section verifies that the SQL operations (create + drop trigger)
        // complete successfully on the Firebird side.
        {
            const std::string triggerTable = makeIdentifier("TRG_TBL_", 'T', idLen);
            const std::string triggerName  = makeIdentifier("TRG_", 'G', idLen);

            tr->Start();
            st->Execute("CREATE TABLE " + quoteIdentifier(triggerTable) +
                " (ID INTEGER)");
            tr->Commit();

            // Create the trigger.
            tr->Start();
            st->Execute(
                "CREATE TRIGGER " + quoteIdentifier(triggerName) +
                " FOR " + quoteIdentifier(triggerTable) +
                " ACTIVE BEFORE INSERT POSITION 0"
                " AS BEGIN END");
            tr->Commit();

            // Verify the trigger exists in the system catalogue.
            tr->Start();
            IBPP::Statement checkSt = IBPP::StatementFactory(db, tr);
            checkSt->Prepare(
                "SELECT COUNT(*) FROM rdb$triggers"
                " WHERE rdb$trigger_name = ?");
            checkSt->Set(1, triggerName);
            checkSt->Execute();
            checkSt->Fetch();
            int countAfterCreate = 0;
            checkSt->Get(1, &countAfterCreate);
            tr->Rollback();

            ok = check(countAfterCreate == 1,
                "issue #436: trigger exists after CREATE") && ok;

            // Drop the trigger – this is the operation that used to crash.
            tr->Start();
            st->Execute("DROP TRIGGER " + quoteIdentifier(triggerName));
            tr->Commit();

            // Verify the trigger was removed from the system catalogue.
            tr->Start();
            checkSt->Execute();
            checkSt->Fetch();
            int countAfterDrop = 0;
            checkSt->Get(1, &countAfterDrop);
            tr->Rollback();

            ok = check(countAfterDrop == 0,
                "issue #436: trigger absent after DROP") && ok;
        }

        db->Drop();
    }
    catch (const IBPP::Exception& e)
    {
        std::cerr << "IBPP exception: " << e.what() << "\n";
        ok = false;
        try
        {
            if (db != 0 && db->Connected())
                db->Drop();
        }
        catch (...)
        {
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
        ok = false;
        try
        {
            if (db != 0 && db->Connected())
                db->Drop();
        }
        catch (...)
        {
        }
    }
    catch (...)
    {
        std::cerr << "Unknown exception.\n";
        ok = false;
        try
        {
            if (db != 0 && db->Connected())
                db->Drop();
        }
        catch (...)
        {
        }
    }

    return ok ? 0 : 1;
}
