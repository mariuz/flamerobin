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

#include "engine/db/DatabaseFactory.h"
#include "engine/db/IService.h"
#include "engine/db/IDatabase.h"
#include "engine/db/TestUtils.h"
#include "engine/db/fbcpp/FbCppService.h"

namespace
{

void appendStrAttr(std::vector<unsigned char>& buf, unsigned char tag, const std::string& val)
{
    buf.push_back(tag);
    unsigned short len = static_cast<unsigned short>(val.size());
    buf.push_back(static_cast<unsigned char>(len & 0xFF));
    buf.push_back(static_cast<unsigned char>((len >> 8) & 0xFF));
    buf.insert(buf.end(), val.begin(), val.end());
}

void appendIntAttr(std::vector<unsigned char>& buf, unsigned char tag, uint32_t val)
{
    buf.push_back(tag);
    buf.push_back(static_cast<unsigned char>(val & 0xFF));
    buf.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
    buf.push_back(static_cast<unsigned char>((val >> 16) & 0xFF));
    buf.push_back(static_cast<unsigned char>((val >> 24) & 0xFF));
}

std::vector<unsigned char> makeUsersChunk(
    const std::vector<unsigned char>& payload,
    unsigned char trailingTag)
{
    std::vector<unsigned char> reply;
    reply.push_back(isc_info_svc_get_users);
    unsigned short len = static_cast<unsigned short>(payload.size());
    reply.push_back(static_cast<unsigned char>(len & 0xFF));
    reply.push_back(static_cast<unsigned char>((len >> 8) & 0xFF));
    reply.insert(reply.end(), payload.begin(), payload.end());
    reply.push_back(trailingTag);
    if (trailingTag != isc_info_end)
        reply.push_back(isc_info_end);
    return reply;
}

bool testParseUsersQueryReply()
{
    bool ok = true;
    std::cout << "Running parseUsersQueryReply unit tests (#758)...\n";

    // Test 1: Single untruncated chunk with multiple users & admin/groupname tags
    {
        std::vector<unsigned char> payload;
        appendStrAttr(payload, isc_spb_sec_username, "SYSDBA");
        appendStrAttr(payload, isc_spb_sec_firstname, "Sql");
        appendStrAttr(payload, isc_spb_sec_middlename, "Server");
        appendStrAttr(payload, isc_spb_sec_lastname, "Administrator");
        appendIntAttr(payload, isc_spb_sec_userid, 0);
        appendIntAttr(payload, isc_spb_sec_groupid, 0);
        appendIntAttr(payload, isc_spb_sec_admin, 1);

        appendStrAttr(payload, isc_spb_sec_username, "ALICE");
        appendStrAttr(payload, isc_spb_sec_groupname, "STAFF");
        appendStrAttr(payload, isc_spb_sec_firstname, "Alice");
        appendStrAttr(payload, isc_spb_sec_lastname, "Smith");
        appendIntAttr(payload, isc_spb_sec_userid, 101);
        appendIntAttr(payload, isc_spb_sec_groupid, 200);

        auto reply = makeUsersChunk(payload, isc_info_end);
        fr::UserData currentUser;
        std::vector<fr::UserData> users;
        std::vector<unsigned char> carryOver;

        bool needMore = fr::parseUsersQueryReply(
            reply.data(), reply.size(), currentUser, users, carryOver);
        if (!currentUser.username.empty())
            users.push_back(currentUser);

        ok = fr_test::check(!needMore, "single chunk: needMore is false") && ok;
        ok = fr_test::check(carryOver.empty(), "single chunk: carryOver is empty") && ok;
        ok = fr_test::check(users.size() == 2, "single chunk: parsed 2 users") && ok;
        if (users.size() == 2)
        {
            ok = fr_test::check(users[0].username == "SYSDBA", "user 0 username") && ok;
            ok = fr_test::check(users[0].firstName == "Sql", "user 0 firstName") && ok;
            ok = fr_test::check(users[0].middleName == "Server", "user 0 middleName") && ok;
            ok = fr_test::check(users[0].lastName == "Administrator", "user 0 lastName") && ok;
            ok = fr_test::check(users[1].username == "ALICE", "user 1 username") && ok;
            ok = fr_test::check(users[1].firstName == "Alice", "user 1 firstName") && ok;
            ok = fr_test::check(users[1].lastName == "Smith", "user 1 lastName") && ok;
            ok = fr_test::check(users[1].userId == 101, "user 1 userId") && ok;
            ok = fr_test::check(users[1].groupId == 200, "user 1 groupId") && ok;
        }
    }

    // Test 2: Multi-chunk reply with isc_info_truncated where one user's fields
    // are split across chunk 1 and chunk 2
    {
        std::vector<unsigned char> chunk1Payload;
        appendStrAttr(chunk1Payload, isc_spb_sec_username, "USER_ONE");
        appendStrAttr(chunk1Payload, isc_spb_sec_firstname, "First1");
        appendStrAttr(chunk1Payload, isc_spb_sec_lastname, "Last1");
        appendIntAttr(chunk1Payload, isc_spb_sec_userid, 1);
        appendIntAttr(chunk1Payload, isc_spb_sec_groupid, 1);

        // USER_TWO starts in chunk 1 (only username and firstname fit)
        appendStrAttr(chunk1Payload, isc_spb_sec_username, "USER_TWO");
        appendStrAttr(chunk1Payload, isc_spb_sec_firstname, "First2");

        // USER_TWO finishes in chunk 2, followed by USER_THREE
        std::vector<unsigned char> chunk2Payload;
        appendStrAttr(chunk2Payload, isc_spb_sec_middlename, "Middle2");
        appendStrAttr(chunk2Payload, isc_spb_sec_lastname, "Last2");
        appendIntAttr(chunk2Payload, isc_spb_sec_userid, 2);
        appendIntAttr(chunk2Payload, isc_spb_sec_groupid, 20);

        appendStrAttr(chunk2Payload, isc_spb_sec_username, "USER_THREE");
        appendStrAttr(chunk2Payload, isc_spb_sec_firstname, "First3");
        appendStrAttr(chunk2Payload, isc_spb_sec_lastname, "Last3");
        appendIntAttr(chunk2Payload, isc_spb_sec_userid, 3);
        appendIntAttr(chunk2Payload, isc_spb_sec_groupid, 30);

        auto reply1 = makeUsersChunk(chunk1Payload, isc_info_truncated);
        auto reply2 = makeUsersChunk(chunk2Payload, isc_info_end);

        fr::UserData currentUser;
        std::vector<fr::UserData> users;
        std::vector<unsigned char> carryOver;

        bool needMore1 = fr::parseUsersQueryReply(
            reply1.data(), reply1.size(), currentUser, users, carryOver);
        ok = fr_test::check(needMore1, "split user chunk 1: needMore is true") && ok;
        ok = fr_test::check(users.size() == 1, "split user chunk 1: only USER_ONE committed so far") && ok;
        ok = fr_test::check(currentUser.username == "USER_TWO", "split user chunk 1: USER_TWO in progress") && ok;

        bool needMore2 = fr::parseUsersQueryReply(
            reply2.data(), reply2.size(), currentUser, users, carryOver);
        if (!currentUser.username.empty())
            users.push_back(currentUser);

        ok = fr_test::check(!needMore2, "split user chunk 2: needMore is false") && ok;
        ok = fr_test::check(users.size() == 3, "split user chunk 2: all 3 users present") && ok;
        if (users.size() == 3)
        {
            ok = fr_test::check(users[1].username == "USER_TWO", "USER_TWO username") && ok;
            ok = fr_test::check(users[1].firstName == "First2", "USER_TWO firstName from chunk 1") && ok;
            ok = fr_test::check(users[1].middleName == "Middle2", "USER_TWO middleName from chunk 2") && ok;
            ok = fr_test::check(users[1].lastName == "Last2", "USER_TWO lastName from chunk 2") && ok;
            ok = fr_test::check(users[1].userId == 2, "USER_TWO userId from chunk 2") && ok;
            ok = fr_test::check(users[1].groupId == 20, "USER_TWO groupId from chunk 2") && ok;
            ok = fr_test::check(users[2].username == "USER_THREE", "USER_THREE username") && ok;
            ok = fr_test::check(users[2].lastName == "Last3", "USER_THREE lastName") && ok;
        }
    }

    // Test 3: Large user list (> 64 KB, 2000 users) across multiple 64 KB chunks
    // (reproduces the ~1,800-user scenario from issue #758, including mid-item split)
    {
        const size_t totalUsers = 2000;
        std::vector<unsigned char> fullStream;
        for (size_t i = 0; i < totalUsers; ++i)
        {
            std::string idx = std::to_string(i);
            appendStrAttr(fullStream, isc_spb_sec_username, "USER_" + idx);
            appendStrAttr(fullStream, isc_spb_sec_firstname, "FirstName_" + idx);
            appendStrAttr(fullStream, isc_spb_sec_middlename, "Middle_" + idx);
            appendStrAttr(fullStream, isc_spb_sec_lastname, "LastName_" + idx);
            appendIntAttr(fullStream, isc_spb_sec_userid, static_cast<uint32_t>(1000 + i));
            appendIntAttr(fullStream, isc_spb_sec_groupid, static_cast<uint32_t>(i % 10));
        }
        ok = fr_test::check(fullStream.size() > 65535, "2000-user stream exceeds 64 KB") && ok;

        const size_t maxPayloadPerChunk = 65520;
        fr::UserData currentUser;
        std::vector<fr::UserData> users;
        std::vector<unsigned char> carryOver;

        size_t offset = 0;
        size_t chunks = 0;
        while (offset < fullStream.size())
        {
            size_t count = std::min(maxPayloadPerChunk, fullStream.size() - offset);
            bool hasMore = (offset + count) < fullStream.size();
            std::vector<unsigned char> slice(
                fullStream.begin() + offset, fullStream.begin() + offset + count);
            auto reply = makeUsersChunk(
                slice, hasMore ? isc_info_truncated : isc_info_end);

            bool needMore = fr::parseUsersQueryReply(
                reply.data(), reply.size(), currentUser, users, carryOver);
            ok = fr_test::check(needMore == hasMore, "chunk needMore matches hasMore") && ok;
            offset += count;
            ++chunks;
        }
        if (!currentUser.username.empty())
            users.push_back(currentUser);

        ok = fr_test::check(chunks >= 2, "stream required multiple chunks") && ok;
        ok = fr_test::check(carryOver.empty(), "no leftover carryOver after final chunk") && ok;
        ok = fr_test::check(users.size() == totalUsers, "all 2000 users parsed without omission") && ok;
        if (users.size() == totalUsers)
        {
            ok = fr_test::check(users.front().username == "USER_0", "first user is USER_0") && ok;
            ok = fr_test::check(users.back().username == "USER_1999", "last user is USER_1999") && ok;
            ok = fr_test::check(users.back().firstName == "FirstName_1999", "last user firstName") && ok;
            ok = fr_test::check(users.back().lastName == "LastName_1999", "last user lastName") && ok;
            ok = fr_test::check(users.back().userId == 2999, "last user userId") && ok;
        }
    }

    return ok;
}

} // namespace

int main()
{
    if (!testParseUsersQueryReply())
    {
        std::cout << "SOME FbCppService TESTS FAILED\n";
        return 1;
    }

    const char* envServer = std::getenv("IBPP_TEST_SERVER");
    const std::string serverName = envServer ? envServer : "";
    if (serverName.empty())
    {
        std::cout << "IBPP_TEST_SERVER is not set, skipping FbCppServiceTest.\n";
        return 0;
    }

    const std::string dbName = fr_test::getTestDbPath("fbcpp_svc_test");

    // Create DB using FbCpp
    try 
    {
        fr::IDatabasePtr db = fr::DatabaseFactory::createDatabase(fr::DatabaseBackend::FbCpp);
        db->setConnectionString(serverName + ":" + dbName);
        db->setCredentials("SYSDBA", "masterkey");
        db->create(8192, 3);
    }
    catch (const std::exception& e)
    {
        fr_test::printException(e, "create test database");
        return 1;
    }

    bool ok = true;
    std::cout << "Starting FbCppService tests...\n";

    try 
    {
        fr::IServicePtr svc = fr::DatabaseFactory::createService(fr::DatabaseBackend::FbCpp);
        std::cout << "  Setting connection string: " << serverName << "\n";
        svc->setConnectionString(serverName);
        svc->setCredentials("SYSDBA", "masterkey");
        
        std::cout << "  Testing getVersion...\n";
        std::string version = svc->getVersion();
        std::cout << "    Debug: Raw version string length: " << version.length() << "\n";
        std::cout << "    Version: " << version << "\n";
        ok = fr_test::check(!version.empty(), "getVersion result is not empty") && ok;

        std::cout << "  Testing getConnectedUsers (via IDatabase)...\n";
        fr::IDatabasePtr db = fr::DatabaseFactory::createDatabase(fr::DatabaseBackend::FbCpp);
        std::string fullConnStr = serverName + ":" + dbName;
        std::cout << "  Connecting to database: " << fullConnStr << "\n";
        db->setConnectionString(fullConnStr);
        db->setCredentials("SYSDBA", "masterkey");
        try {
            db->connect();
            std::cout << "    Connected successfully.\n";
        } catch (const std::exception& e) {
            std::cerr << "    FAILED to connect to " << fullConnStr << "\n";
            std::cerr << "    Exception: " << e.what() << "\n";
            throw;
        }

        std::cout << "    Engine Version: " << db->getEngineVersion() << "\n";
        fr::DatabaseInfoData info;
        std::cout << "    Fetching database info...\n";
        db->getInfo(&info);
        std::cout << "    ODS Version: " << info.ods << "." << info.odsMinor << "\n";
        std::cout << "    Page Size: " << info.pageSize << ", Pages: " << info.pages << "\n";
        std::cout << "    Forced Writes: " << (info.forcedWrites ? "ON" : "OFF") << "\n";
        std::cout << "    Reserve Space: " << (info.reserve ? "ON" : "OFF") << "\n";
        std::cout << "    Sweep Interval: " << info.sweep << "\n";
        std::cout << "    Page Buffers: " << info.buffers << "\n";
        std::cout << "    Next Transaction: " << info.nextTransaction << "\n";

        ok = fr_test::check(info.pageSize > 0, "pageSize > 0") && ok;
        ok = fr_test::check(info.sweep >= 0, "sweep >= 0") && ok;
        
        std::cout << "    Fetching connected users...\n";
        std::vector<std::string> users;
        db->getConnectedUsers(users);
        std::cout << "    Debug: Found " << users.size() << " connected users.\n";
        ok = fr_test::check(!users.empty(), "getConnectedUsers list is not empty") && ok;
        bool foundSysdba = false;
        for (const auto& u : users)
        {
            std::cout << "    Connected user: [" << u << "]\n";
            if (u == "SYSDBA") foundSysdba = true;
        }
        ok = fr_test::check(foundSysdba, "found SYSDBA in connected users list") && ok;

        std::cout << "  Testing getDialect...\n";
        int dialect = db->getDialect();
        std::cout << "    Dialect: " << dialect << "\n";
        ok = fr_test::check(dialect == 3, "getDialect returns 3") && ok;

        std::cout << "  Testing database property modifications via Service...\n";
        svc->setSweepInterval(dbName, 20000);
        std::vector<std::string> logLines;
        std::string logLine;
        while (!(logLine = svc->getNextLine()).empty())
        {
            logLines.push_back(logLine);
        }
        std::cout << "    Fetched " << logLines.size() << " log lines for sweep interval.\n";
        ok = fr_test::check(!logLines.empty(), "getNextLine streamed log lines successfully") && ok;

        svc->setPageBuffers(dbName, 2500);
        svc->setSyncWrite(dbName, true);
        svc->setReserveSpace(dbName, true);

        // Setting database to READ ONLY requires exclusive access (no active attachments)
        std::cout << "  Disconnecting db to test setReadOnly(true)...\n";
        db->disconnect();
        std::cout << "  Testing setReadOnly(true)...\n";
        svc->setReadOnly(dbName, true);

        // Reconnect and check updated properties
        db->connect();
        fr::DatabaseInfoData updatedInfo;
        db->getInfo(&updatedInfo);
        std::cout << "    Updated Sweep: " << updatedInfo.sweep << ", ForcedWrites: " << (updatedInfo.forcedWrites ? "ON" : "OFF") << ", ReadOnly: " << (updatedInfo.readOnly ? "ON" : "OFF") << "\n";
        ok = fr_test::check(updatedInfo.sweep == 20000, "updated sweep interval is 20000") && ok;
        ok = fr_test::check(updatedInfo.forcedWrites == true, "updated forced writes is true") && ok;
        ok = fr_test::check(updatedInfo.reserve == true, "updated reserve space is true") && ok;
        ok = fr_test::check(updatedInfo.readOnly == true, "updated read-only is true") && ok;

        // Disconnect to restore READ WRITE mode
        std::cout << "  Disconnecting db to test setReadOnly(false)...\n";
        db->disconnect();
        std::cout << "  Testing setReadOnly(false)...\n";
        svc->setReadOnly(dbName, false);
        db->connect();
        db->getInfo(&updatedInfo);
        std::cout << "    Restored ReadOnly: " << (updatedInfo.readOnly ? "ON" : "OFF") << "\n";
        ok = fr_test::check(updatedInfo.readOnly == false, "restored read-only is false") && ok;

        std::cout << "  Disconnecting...\n";
        db->disconnect();
        std::cout << "    Disconnected successfully.\n";
    }
    catch (const std::exception& e)
    {
        fr_test::printException(e, "FbCppServiceTest");
        ok = false;
    }

    // Cleanup
    try 
    {
        fr::IDatabasePtr db = fr::DatabaseFactory::createDatabase(fr::DatabaseBackend::FbCpp);
        db->setConnectionString(serverName + ":" + dbName);
        db->setCredentials("SYSDBA", "masterkey");
        db->connect();
        db->drop();
    }
    catch (...) {}

    if (ok)
    {
        std::cout << "ALL FbCppService TESTS PASSED\n";
        return 0;
    }
    else
    {
        std::cout << "SOME FbCppService TESTS FAILED\n";
        return 1;
    }
}

