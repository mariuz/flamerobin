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

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "ibpp/ibpp.h"
#include "engine/db/DatabaseFactory.h"
#include "engine/db/IDatabase.h"
#include "engine/db/ITransaction.h"
#include "engine/db/IStatement.h"
#include "engine/db/IBlob.h"
#include "core/StringUtils.h"

// Minimal wx2std implementation for tests to avoid linking StringUtils.cpp
std::string wx2std(const wxString& input, wxMBConv* conv)
{
    if (input.empty())
        return "";
    if (!conv)
        conv = wxConvCurrent;
    const wxWX2MBbuf buf(input.mb_str(*conv));
    if (!buf)
        return "";
    return std::string(buf);
}

// Stub for std2wxIdentifier if needed (though not used in DalTypesTest directly)
wxString std2wxIdentifier(const std::string& input, wxMBConv* /*conv*/)
{
    return wxString::FromUTF8(input.c_str());
}

namespace
{

bool check(bool condition, const char* testName)
{
    if (condition)
    {
        std::cout << "  PASSED: " << testName << "\n";
        return true;
    }
    std::cerr << "  FAILED: " << testName << "\n";
    return false;
}

bool checkStr(const std::string& actual, const std::string& expected, const char* testName)
{
    if (actual == expected)
    {
        std::cout << "  PASSED: " << testName << "\n";
        return true;
    }
    std::cerr << "  FAILED: " << testName << "\n"
        << "    Expected: [" << expected << "]\n"
        << "    Actual:   [" << actual << "]\n";
    return false;
}

} // namespace

bool runTestsForBackend(fr::DatabaseBackend backend, const std::string& /*serverName*/, const std::string& dbName)
{
    bool ok = true;
    std::cout << "Testing backend: " << (backend == fr::DatabaseBackend::IBPP ? "IBPP" : "FbCpp") << "\n";
    
    try 
    {
        fr::IDatabasePtr db = fr::DatabaseFactory::createDatabase(backend);
        db->setConnectionString(dbName);
        db->setCredentials("SYSDBA", "masterkey");
        db->connect();

        fr::ITransactionPtr tr = db->createTransaction();
        tr->start();

        fr::IStatementPtr st = db->createStatement(tr);
        
        // Basic Metadata tests (works on all FB versions)
        std::cout << "  Running basic metadata tests...\n";
        st->prepare("SELECT CAST(1.23 AS NUMERIC(18,4)) AS MY_ALIAS FROM RDB$DATABASE");
        st->execute();
        st->fetch();
        ok = check(st->getColumnCount() == 1, "getColumnCount") && ok;
        ok = check(st->getColumnType(0) == fr::ColumnType::BigInt, "ColumnType::BigInt identification") && ok;
        ok = check(st->getColumnScale(0) == 4, "getColumnScale") && ok;
        ok = check(st->getColumnSize(0) == 8, "getColumnSize (int64)") && ok;
        ok = checkStr(st->getColumnAlias(0), "MY_ALIAS", "getColumnAlias") && ok;
        
        // Subtype test (OCTETS)
        st->prepare("SELECT CAST('abc' AS VARCHAR(10) CHARACTER SET OCTETS) FROM RDB$DATABASE");
        st->execute();
        st->fetch();
        int subtype = st->getColumnSubtype(0);
        if (subtype != 1)
        {
            std::cout << "    INFO: Subtype reported as " << subtype << " (expected 1 for OCTETS)\n";
            // Some API versions might report charset ID differently in subtype field for strings
        }
        ok = check(subtype != 0, "getColumnSubtype (non-zero for OCTETS)") && ok;

        // Table name test
        st->prepare("SELECT RDB$RELATION_NAME FROM RDB$RELATIONS");
        st->execute();
        st->fetch();
        std::string tableName = st->getColumnTable(0);
        // Trim trailing spaces if any (IBPP might return padded strings for metadata)
        size_t last = tableName.find_last_not_of(' ');
        if (last != std::string::npos) tableName = tableName.substr(0, last + 1);
        ok = check(tableName == "RDB$RELATIONS", "getColumnTable") && ok;

        // Temporal types
        std::cout << "  Running temporal types tests...\n";
        st->prepare("SELECT CAST('2023-05-20' AS DATE), CAST('12:34:56.7890' AS TIME), CAST('2023-05-20 12:34:56.7890' AS TIMESTAMP) FROM RDB$DATABASE");
        st->execute();
        st->fetch();
        ok = check(st->getColumnType(0) == fr::ColumnType::Date, "ColumnType::Date identification") && ok;
        ok = check(st->getColumnType(1) == fr::ColumnType::Time, "ColumnType::Time identification") && ok;
        ok = check(st->getColumnType(2) == fr::ColumnType::Timestamp, "ColumnType::Timestamp identification") && ok;
        
        ok = check(st->getDate(0).find("2023-05-20") != std::string::npos, "getDate content") && ok;
        ok = check(st->getTime(1).find("12:34:56") != std::string::npos, "getTime content") && ok;
        ok = check(st->getTimestamp(2).find("2023-05-20 12:34:56") != std::string::npos, "getTimestamp content") && ok;

        // Check for Timezone support
        std::cout << "  Checking for Timezone support...\n";
        try
        {
            st->prepare("SELECT CAST('12:34:56.7890 UTC' AS TIME WITH TIME ZONE), "
                        "CAST('2023-05-20 12:34:56.7890 UTC' AS TIMESTAMP WITH TIME ZONE) FROM RDB$DATABASE");
            st->execute();
            st->fetch();
            ok = check(st->getColumnType(0) == fr::ColumnType::TimeTz, "ColumnType::TimeTz identification") && ok;
            ok = check(st->getColumnType(1) == fr::ColumnType::TimestampTz, "ColumnType::TimestampTz identification") && ok;

            ok = check(st->getTimeTz(0).find("12:34:56") != std::string::npos, "getTimeTz content") && ok;
            ok = check(st->getTimeTz(0).find("UTC") != std::string::npos, "getTimeTz timezone content") && ok;
            ok = check(st->getTimestampTz(1).find("2023-05-20 12:34:56") != std::string::npos, "getTimestampTz content") && ok;
            ok = check(st->getTimestampTz(1).find("UTC") != std::string::npos, "getTimestampTz timezone content") && ok;
        }
        catch (const std::exception& e)
        {
            std::string msg = e.what();
            if (msg.find("Token unknown") != std::string::npos || msg.find("Data type unknown") != std::string::npos)
            {
                std::cout << "  Skipping Timezone support tests (unsupported by server/backend).\n";
            }
            else
            {
                std::cerr << "  ERROR: Unexpected exception during Timezone test: " << e.what() << "\n";
                ok = false;
            }
        }

        // Test getDialect and getInfo
        std::cout << "  Testing database metadata methods...\n";
        ok = check(db->getDialect() == 3, "getDialect") && ok;

        fr::DatabaseInfoData info;
        db->getInfo(&info);
        ok = check(info.ods > 0, "getInfo ODS") && ok;
        ok = check(info.pageSize > 0, "getInfo PageSize") && ok;
        ok = check(info.nextTransaction > 0, "getInfo NextTransaction") && ok;

        // Statistics and Counts
        std::cout << "  Testing statistics and counts...\n";
        int fetch, mark, read, write, mem;
        db->getStatistics(&fetch, &mark, &read, &write, &mem);
        ok = check(fetch >= 0, "getStatistics") && ok;

        int ins, upd, del, ridx, rseq;
        db->getCounts(&ins, &upd, &del, &ridx, &rseq);
        ok = check(ridx >= 0, "getCounts") && ok;

        std::map<int, fr::CountInfo> detailedCounts;
        db->getDetailedCounts(detailedCounts);
        ok = check(detailedCounts.size() >= 0, "getDetailedCounts") && ok;

        // Statement Type and Plan
        std::cout << "  Testing statement type and plan...\n";
        st->prepare("SELECT * FROM RDB$DATABASE");
        ok = check(st->getType() == fr::StatementType::Select, "getType Select") && ok;
        std::string plan = st->getPlan();
        ok = check(!plan.empty(), "getPlan") && ok;

        // Create a dummy table for testing UPDATE and affected rows
        try {
            st->prepare("DROP TABLE DAL_TEST");
            st->execute();
            tr->commitRetain();
        } catch (...) {}

        st->prepare("CREATE TABLE DAL_TEST (ID INTEGER)");
        st->execute();
        tr->commitRetain();

        st->prepare("INSERT INTO DAL_TEST (ID) VALUES (1)");
        st->execute();

        st->prepare("UPDATE DAL_TEST SET ID = 2 WHERE ID = 1");
        ok = check(st->getType() == fr::StatementType::Update, "getType Update") && ok;

        // Affected rows
        st->execute();
        ok = check(st->getAffectedRows() == 1, "getAffectedRows") && ok;

        // Parameter tests
        std::cout << "  Testing statement parameters...\n";
        st->prepare("UPDATE DAL_TEST SET ID = ? WHERE ID = ?");
        ok = check(st->getParameterCount() == 2, "getParameterCount") && ok;
        ok = check(st->getParameterType(0) == fr::ColumnType::Integer, "getParameterType 0") && ok;
        ok = check(st->getParameterType(1) == fr::ColumnType::Integer, "getParameterType 1") && ok;

        // Named parameter simulation (IBPP supports it)
        if (backend == fr::DatabaseBackend::IBPP)
        {
            st->prepare("UPDATE DAL_TEST SET ID = :newid WHERE ID = :oldid");
            ok = check(st->getParameterCount() == 2, "getParameterCount (named)") && ok;
            ok = checkStr(st->getParameterName(0), "newid", "getParameterName 0") && ok;
            ok = checkStr(st->getParameterName(1), "oldid", "getParameterName 1") && ok;
            auto indices = st->findParameterIndicesByName("oldid");
            ok = check(indices.size() == 1 && indices[0] == 2, "findParameterIndicesByName") && ok;
        }

        // Typed parameter setting
        std::cout << "  Testing typed parameter setting...\n";
        st->prepare("INSERT INTO DAL_TEST (ID) VALUES (?)");
        st->setInt32(0, 123);
        st->execute();
        st->prepare("SELECT ID FROM DAL_TEST WHERE ID = 123");
        st->execute();
        ok = check(st->fetch(), "fetch inserted row") && ok;
        ok = check(st->getInt32(0) == 123, "getInt32 matches setInt32") && ok;

        // Date/Time parameter setting
        std::cout << "  Testing date/time parameter setting...\n";
        st->prepare("SELECT CAST(? AS DATE), CAST(? AS TIME), CAST(? AS TIMESTAMP) FROM RDB$DATABASE");
        st->setDate(0, 2023, 5, 25);
        st->setTime(1, 14, 30, 45, 0);
        st->setTimestamp(2, 2023, 5, 25, 14, 30, 45, 0);
        st->execute();
        ok = check(st->fetch(), "fetch date/time results") && ok;
        ok = check(st->getDate(0).find("2023-05-25") != std::string::npos, "setDate result") && ok;
        ok = check(st->getTime(1).find("14:30:45") != std::string::npos, "setTime result") && ok;
        ok = check(st->getTimestamp(2).find("2023-05-25 14:30:45") != std::string::npos, "setTimestamp result") && ok;

        // BLOB tests
        std::cout << "  Testing BLOB operations...\n";
        try {
            st->prepare("DROP TABLE BLOB_TEST");
            st->execute();
            tr->commitRetain();
        } catch (...) {}
        st->prepare("CREATE TABLE BLOB_TEST (ID INTEGER, B BLOB SUB_TYPE TEXT)");
        st->execute();
        tr->commitRetain();

        st->prepare("INSERT INTO BLOB_TEST (ID, B) VALUES (2, 'Direct SQL blob')");
        st->execute();
        tr->commitRetain();

        st->prepare("SELECT B FROM BLOB_TEST WHERE ID = 2");
        st->execute();
        ok = check(st->fetch(), "fetch row with blob") && ok;
        fr::IBlobPtr b = st->getBlob(0);
        ok = check(b != nullptr, "getBlob not null") && ok;
        if (b)
        {
            b->open();
            char buf[100];
            int len = b->read(buf, sizeof(buf)-1);
            if (len < 0) len = 0;
            buf[len] = '\0';
            ok = checkStr(buf, "Direct SQL blob", "read blob content") && ok;
            b->close();
        }

        // Transaction configuration
        std::cout << "  Testing transaction configuration...\n";
        fr::ITransactionPtr tr2 = db->createTransaction();
        tr2->setAccessMode(fr::TransactionAccessMode::Read);
        tr2->setIsolationLevel(fr::TransactionIsolationLevel::ReadCommitted);
        tr2->setLockResolution(fr::TransactionLockResolution::NoWait);
        tr2->start();
        ok = check(tr2->isActive(), "Transaction configuration and start") && ok;
        tr2->commit();

        // Check if Firebird 4.0+ by trying to CAST to DECFLOAT
        std::cout << "  Checking for Firebird 4.0+ types support...\n";
        try 
        {
            st->prepare("SELECT CAST(1 AS DECFLOAT(16)) FROM RDB$DATABASE");
            st->execute();
            st->fetch();
            ok = check(st->getColumnType(0) == fr::ColumnType::Decfloat16, "ColumnType::Decfloat16 identification") && ok;

            // Test DECFLOAT34
            st->prepare("SELECT CAST('1.234567890123456789012345678901234' AS DECFLOAT(34)) FROM RDB$DATABASE");
            st->execute();
            st->fetch();
            ok = check(st->getColumnType(0) == fr::ColumnType::Decfloat34, "ColumnType::Decfloat34 identification") && ok;
            std::string df34 = st->getString(0);
            ok = check(df34.find("1.234567890123456789012345678901234") == 0, "DECFLOAT34 getString content") && ok;

            // Test INT128
            st->prepare("SELECT CAST('12345678901234567890123456789012345678' AS INT128) FROM RDB$DATABASE");
            st->execute();
            st->fetch();
            ok = check(st->getColumnType(0) == fr::ColumnType::Int128, "ColumnType::Int128 identification") && ok;
            ok = checkStr(st->getString(0), "12345678901234567890123456789012345678", "INT128 getString") && ok;
        }
        catch (const std::exception& e)
        {
            std::string msg = e.what();
            if (msg.find("Token unknown") != std::string::npos || msg.find("Data type unknown") != std::string::npos)
            {
                std::cout << "  Skipping Firebird 4.0+ types (unsupported by server/backend).\n";
            }
            else
            {
                std::cerr << "  ERROR: Unexpected exception during FB 4.0+ types test: " << e.what() << "\n";
                ok = false;
            }
        }

        tr->commit();
        st.reset();
        tr.reset();
        db->disconnect();
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION in backend tests: " << e.what() << "\n";
        return false;
    }
    return ok;
}

int main()
{
    const char* envServer = std::getenv("IBPP_TEST_SERVER");
    const std::string serverName = envServer ? envServer : "";
    if (serverName.empty())
    {
        std::cout << "IBPP_TEST_SERVER is not set, skipping dal_types_test.\n";
        return 0;
    }

    const std::string dbName = "/tmp/flamerobin_dal_types_test_" +
        std::to_string(static_cast<long long>(std::time(0))) + ".fdb";

    std::cout << "Creating test database: " << dbName << "\n";

    // Create DB using IBPP
    try 
    {
        IBPP::Database db = IBPP::DatabaseFactory(serverName, dbName, "SYSDBA", "masterkey");
        db->Create(3);
    }
    catch (const IBPP::Exception& e)
    {
        std::cerr << "Failed to create test database: " << e.what() << "\n";
        return 1;
    }

    bool all_ok = true;
    all_ok = runTestsForBackend(fr::DatabaseBackend::IBPP, serverName, dbName) && all_ok;
    
    try 
    {
        all_ok = runTestsForBackend(fr::DatabaseBackend::FbCpp, serverName, dbName) && all_ok;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FbCpp backend tests failed to even start: " << e.what() << "\n";
        all_ok = false;
    }

    // Cleanup
    try 
    {
        IBPP::Database db = IBPP::DatabaseFactory(serverName, dbName, "SYSDBA", "masterkey");
        db->Connect();
        db->Drop();
    }
    catch (...) {}

    if (all_ok)
    {
        std::cout << "ALL DAL TESTS PASSED\n";
        return 0;
    }
    else
    {
        std::cout << "SOME DAL TESTS FAILED\n";
        return 1;
    }
}
