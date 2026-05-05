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

#include <ibpp.h>

#include "engine/db/DatabaseFactory.h"
#include "engine/db/IDatabase.h"
#include "engine/db/ITransaction.h"
#include "engine/db/IStatement.h"
#include "engine/db/TestUtils.h"

int main()
{
    const char* envServer = std::getenv("IBPP_TEST_SERVER");
    const std::string serverName = envServer ? envServer : "";
    if (serverName.empty())
    {
        std::cout << "IBPP_TEST_SERVER is not set, skipping ProfilerTest.\n";
        return 0;
    }

    const std::string dbName = fr_test::getTestDbPath("profiler_test");

    // Create DB using IBPP
    try 
    {
        IBPP::Database db = IBPP::DatabaseFactory(serverName, dbName, "SYSDBA", "masterkey");
        db->Create(3);
    }
    catch (const IBPP::Exception& e)
    {
        fr_test::printException(e, "create test database");
        return 1;
    }

    bool ok = true;
    std::cout << "Starting RDB$PROFILER extensive tests...\n";

    try 
    {
        fr::IDatabasePtr db = fr::DatabaseFactory::createDatabase(fr::DatabaseBackend::FbCpp);
        db->setConnectionString(dbName);
        db->setCredentials("SYSDBA", "masterkey");
        db->connect();

        fr::ITransactionPtr tr = db->createTransaction();
        tr->start();

        fr::IStatementPtr st = db->createStatement(tr);
        
        // Check if RDB$PROFILER exists
        try {
            st->prepare("SELECT 1 FROM RDB$PACKAGES WHERE RDB$PACKAGE_NAME = 'RDB$PROFILER'");
            st->execute();
            if (!st->fetch())
            {
                std::cout << "RDB$PROFILER package not found, skipping tests.\n";
                tr->rollback();
                db->disconnect();
                return 0;
            }
        } catch(const std::exception& e) {
            fr_test::printException(e, "checking for RDB$PROFILER");
            return 0;
        }

        // Test 1: Simple Profiling Session
        std::cout << "  Test 1: Simple Profiling Session...\n";
        st->prepare("SELECT RDB$PROFILER.START_SESSION('Test Session') FROM RDB$DATABASE");
        st->execute();
        int64_t sessionId = 0;
        if (st->fetch())
            sessionId = st->getInt64(0);
        
        ok = fr_test::check(sessionId != 0, "Session started") && ok;
        std::cout << "    Debug: Session ID = " << sessionId << "\n";

        st->prepare("CREATE TABLE t1 (id INT PRIMARY KEY, val VARCHAR(100))");
        st->execute();
        tr->commitRetain();

        st->prepare("INSERT INTO t1 VALUES (1, 'test')");
        st->execute();

        // Flush and finish
        try {
            st->prepare("EXECUTE PROCEDURE RDB$PROFILER.FLUSH_STATS");
            st->execute();
        } catch(...) {}

        st->prepare("EXECUTE PROCEDURE RDB$PROFILER.FINISH_SESSION(TRUE)");
        st->execute();

        // Give Firebird a moment to flush profiler data if needed
        st->prepare("SELECT COUNT(*) FROM PLG$PROF_RECORD_SOURCE_STATS WHERE PROFILE_ID = ?");
        st->setInt64(0, sessionId);
        st->execute();
        int count = 0;
        if (st->fetch())
            count = st->getInt32(0);
        
        if (count == 0)
        {
             std::cout << "    FAILURE: No record source stats found for session " << sessionId << "\n";
             // Debug: check session
             st->prepare("SELECT PROFILE_ID, SESSION_NAME, DESCRIPTION FROM PLG$PROF_SESSIONS WHERE PROFILE_ID = ?");
             st->setInt64(0, sessionId);
             st->execute();
             if (st->fetch())
                 std::cout << "    Debug: Session exists: " << st->getInt64(0) << " (" << st->getString(1) << ")\n";
             else
                 std::cout << "    Debug: Session NOT FOUND in PLG$PROF_SESSIONS\n";

             // Check all sessions to see what is there
             std::cout << "    Debug: Listing all sessions:\n";
             st->prepare("SELECT PROFILE_ID, SESSION_NAME FROM PLG$PROF_SESSIONS");
             st->execute();
             while (st->fetch())
                 std::cout << "      ID: " << st->getInt64(0) << ", Name: " << st->getString(1) << "\n";
        }

        ok = fr_test::check(count > 0, "Record source stats collected for simple INSERT") && ok;

        // Test 2: Nested PSQL calls
        std::cout << "  Test 2: Nested PSQL calls...\n";
        st->prepare("CREATE PROCEDURE p_child AS BEGIN END");
        st->execute();
        st->prepare("CREATE PROCEDURE p_parent AS BEGIN EXECUTE PROCEDURE p_child; END");
        st->execute();
        tr->commitRetain();

        st->prepare("SELECT RDB$PROFILER.START_SESSION('Nested Session') FROM RDB$DATABASE");
        st->execute();
        sessionId = 0;
        if (st->fetch())
            sessionId = st->getInt64(0);

        st->prepare("EXECUTE PROCEDURE p_parent");
        st->execute();

        try {
            st->prepare("EXECUTE PROCEDURE RDB$PROFILER.FLUSH_STATS");
            st->execute();
        } catch(...) {}

        st->prepare("EXECUTE PROCEDURE RDB$PROFILER.FINISH_SESSION(TRUE)");
        st->execute();

        st->prepare("SELECT COUNT(DISTINCT REQUEST_NAME) FROM PLG$PROF_REQUESTS WHERE PROFILE_ID = ?");
        st->setInt64(0, sessionId);
        st->execute();
        count = 0;
        if (st->fetch())
            count = st->getInt32(0);
        ok = fr_test::check(count >= 2, "Stats collected for both parent and child procedures") && ok;

        // Test 3: Triggers
        std::cout << "  Test 3: Triggers...\n";
        st->prepare("CREATE TABLE t2 (id INT)");
        st->execute();
        st->prepare("CREATE TRIGGER t2_ai FOR t2 AFTER INSERT AS BEGIN END");
        st->execute();
        tr->commitRetain();

        st->prepare("SELECT RDB$PROFILER.START_SESSION('Trigger Session') FROM RDB$DATABASE");
        st->execute();
        sessionId = 0;
        if (st->fetch())
            sessionId = st->getInt64(0);

        st->prepare("INSERT INTO t2 VALUES (1)");
        st->execute();

        try {
            st->prepare("EXECUTE PROCEDURE RDB$PROFILER.FLUSH_STATS");
            st->execute();
        } catch(...) {}

        st->prepare("EXECUTE PROCEDURE RDB$PROFILER.FINISH_SESSION(TRUE)");
        st->execute();

        st->prepare("SELECT COUNT(*) FROM PLG$PROF_REQUESTS WHERE PROFILE_ID = ? AND REQUEST_NAME = 'T2_AI'");
        st->setInt64(0, sessionId);
        st->execute();
        count = 0;
        if (st->fetch())
            count = st->getInt32(0);
        ok = fr_test::check(count > 0, "Stats collected for trigger") && ok;

        // Test 4: Record Source Stats with JOIN
        std::cout << "  Test 4: Record Source Stats with JOIN...\n";
        st->prepare("SELECT RDB$PROFILER.START_SESSION('JOIN Session') FROM RDB$DATABASE");
        st->execute();
        sessionId = 0;
        if (st->fetch())
            sessionId = st->getInt64(0);

        st->prepare("SELECT COUNT(*) FROM t1 a JOIN t2 b ON a.id = b.id");
        st->execute();
        if (st->fetch()) {}

        try {
            st->prepare("EXECUTE PROCEDURE RDB$PROFILER.FLUSH_STATS");
            st->execute();
        } catch(...) {}

        st->prepare("EXECUTE PROCEDURE RDB$PROFILER.FINISH_SESSION(TRUE)");
        st->execute();

        st->prepare("SELECT COUNT(*) FROM PLG$PROF_RECORD_SOURCE_STATS WHERE PROFILE_ID = ?");
        st->setInt64(0, sessionId);
        st->execute();
        count = 0;
        if (st->fetch())
            count = st->getInt32(0);
        ok = fr_test::check(count >= 2, "Record source stats collected for JOIN") && ok;

        tr->commit();
        db->disconnect();
    }
    catch (const std::exception& e)
    {
        fr_test::printException(e, "ProfilerTest");
        ok = false;
    }

    // Cleanup
    try 
    {
        IBPP::Database db = IBPP::DatabaseFactory(serverName, dbName, "SYSDBA", "masterkey");
        db->Connect();
        db->Drop();
    }
    catch (...) {}

    if (ok)
    {
        std::cout << "ALL Profiler TESTS PASSED\n";
        return 0;
    }
    else
    {
        std::cout << "SOME Profiler TESTS FAILED\n";
        return 1;
    }
}

