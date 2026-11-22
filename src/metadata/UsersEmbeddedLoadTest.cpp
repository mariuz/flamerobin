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

// Regression test for GitHub issue #766:
// "Embedded issues" - Minimal Firebird embedded deployments do not include
// UserManager plugins (e.g. srp.dll) or a security database (SECURITY5.FDB).
// Users::load() queries SEC$USERS during Database::connect() and must degrade
// gracefully to an empty collection instead of aborting the connection, while
// still propagating CancelProgressException and loading rows when available.

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "config/Config.h"
#include "core/Observer.h"
#include "core/ProgressIndicator.h"
#include "engine/MetadataLoader.h"
#include "engine/db/IStatement.h"
#include "metadata/collection.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/User.h"

namespace
{

struct SecUserRow
{
    std::string userName;
    bool firstNull = true;
    std::string firstName;
    bool middleNull = true;
    std::string middleName;
    bool lastNull = true;
    std::string lastName;
    bool pluginNull = true;
    std::string plugin;
};

struct TestLoadState
{
    int odsMajor = 13;
    int odsMinor = 1;
    bool throwOnCreateStatement = false;
    bool throwOnExecute = false;
    bool throwOnLoadIdentifiers = false;
    std::string errorMessage = "I/O error during CreateFile (open) operation for file \"SECURITY5.FDB\"";
    std::vector<SecUserRow> rows;
    int createStatementCalls = 0;
    int getStatementCalls = 0;
};

TestLoadState gState;

class FakeStatement : public fr::IStatement
{
private:
    std::string sqlM;
    int rowIndexM = -1;

public:
    explicit FakeStatement(const std::string& sql) : sqlM(sql) {}

    void prepare(const std::string& sql) override { sqlM = sql; }
    std::string getSql() const override { return sqlM; }

    void execute() override
    {
        if (gState.throwOnExecute)
            throw std::runtime_error(gState.errorMessage);
        rowIndexM = -1;
    }

    bool fetch() override
    {
        ++rowIndexM;
        return rowIndexM >= 0 && static_cast<size_t>(rowIndexM) < gState.rows.size();
    }

    void close() override {}

    void setNull(int) override {}
    void setString(int, const std::string&) override {}
    void setInt32(int, int32_t) override {}
    void setInt64(int, int64_t) override {}
    void setDouble(int, double) override {}
    void setBool(int, bool) override {}
    void setDate(int, int, int, int) override {}
    void setTime(int, int, int, int, int) override {}
    void setTimestamp(int, int, int, int, int, int, int, int) override {}
    void setBytes(int, const void*, int) override {}

    bool isNull(int index) override
    {
        const auto& r = gState.rows.at(static_cast<size_t>(rowIndexM));
        switch (index)
        {
            case 0: return false;
            case 1: return r.firstNull;
            case 2: return r.middleNull;
            case 3: return r.lastNull;
            case 4: return r.pluginNull;
            default: return true;
        }
    }

    std::string getString(int index) override
    {
        const auto& r = gState.rows.at(static_cast<size_t>(rowIndexM));
        switch (index)
        {
            case 0: return r.userName;
            case 1: return r.firstName;
            case 2: return r.middleName;
            case 3: return r.lastName;
            case 4: return r.plugin;
            default: return "";
        }
    }

    int32_t getInt32(int) override { return 0; }
    int64_t getInt64(int) override { return 0; }
    double getDouble(int) override { return 0.0; }
    bool getBool(int) override { return false; }

    void getBytes(int, void*, int) override {}
    fr::IBlobPtr getBlob(int) override { return nullptr; }
    void setBlob(int, fr::IBlobPtr) override {}

    std::string getDate(int) override { return ""; }
    std::string getTime(int) override { return ""; }
    std::string getTimestamp(int) override { return ""; }
    std::string getTimeTz(int) override { return ""; }
    std::string getTimestampTz(int) override { return ""; }

    void getDate(int, int&, int&, int&) override {}
    void getTime(int, int&, int&, int&, int&) override {}
    void getTimestamp(int, int&, int&, int&, int&, int&, int&, int&) override {}

    int getColumnCount() override { return 5; }
    std::string getColumnName(int) override { return ""; }
    fr::ColumnType getColumnType(int) override { return fr::ColumnType::Varchar; }
    int getColumnSubtype(int) override { return 0; }
    int getColumnScale(int) override { return 0; }
    int getColumnSize(int) override { return 0; }
    std::string getColumnAlias(int) override { return ""; }
    std::string getColumnTable(int) override { return ""; }

    std::string getPlan() override { return ""; }
    fr::StatementType getType() override { return fr::StatementType::Select; }
    int getParameterCount() override { return 0; }
    std::string getParameterName(int) override { return ""; }
    std::vector<int> findParameterIndicesByName(const std::string&) override { return {}; }
    fr::ColumnType getParameterType(int) override { return fr::ColumnType::Varchar; }
    int getParameterSubtype(int) override { return 0; }
    int getParameterScale(int) override { return 0; }
    int getParameterSize(int) override { return 0; }

    int getAffectedRows() override { return 0; }

    fr::IDatabasePtr getDatabase() override { return nullptr; }
    fr::ITransactionPtr getTransaction() override { return nullptr; }
};

class FakeProgressIndicator : public ProgressIndicator
{
private:
    bool canceledM;

public:
    explicit FakeProgressIndicator(bool canceled) : canceledM(canceled) {}

    bool isCanceled() override { return canceledM; }
    void initProgress(wxString, size_t, size_t, size_t) override {}
    void initProgressIndeterminate(wxString, size_t) override {}
    void setProgressMessage(wxString, size_t) override {}
    void setProgressPosition(size_t, size_t) override {}
    void stepProgress(int, size_t) override {}
    void doShow() override {}
    void doHide() override {}
    void setProgressLevelCount(size_t) override {}
};

bool check(bool condition, const char* testName)
{
    if (condition)
    {
        std::cout << "[PASS] " << testName << "\n";
        return true;
    }
    std::cerr << "[FAIL] " << testName << "\n";
    return false;
}

} // namespace

// --- Minimal stubs required to link User.cpp in isolation ---

MetadataItem::MetadataItem()
    : parentM(0), typeM(ntUnknown), propertiesLoadedM(lsNotLoaded),
      childrenLoadedM(lsNotLoaded), metadataIdM(-1)
{
}

MetadataItem::MetadataItem(NodeType type, MetadataItem* parent, const wxString& name, int id)
    : parentM(parent), typeM(type), identifierM(name),
      propertiesLoadedM(lsNotLoaded), childrenLoadedM(lsNotLoaded), metadataIdM(id)
{
}

MetadataItem::~MetadataItem() {}

wxString MetadataItem::getName_() const { return identifierM.get(); }
wxString MetadataItem::getQuotedName() const { return identifierM.getQuoted(); }
Identifier MetadataItem::getIdentifier() const { return identifierM; }
void MetadataItem::setName_(const wxString& name) { identifierM.setText(name); }
NodeType MetadataItem::getType() const { return typeM; }
void MetadataItem::setType(NodeType type) { typeM = type; }
int MetadataItem::getMetadataId() { return metadataIdM; }
void MetadataItem::setMetadataId(int id) { metadataIdM = id; }

const wxString MetadataItem::getTypeName() const { return ""; }
const wxString MetadataItem::getItemPath() const { return ""; }
const wxString MetadataItem::getPathId() const { return ""; }
const wxString MetadataItem::getId() const { return getName_(); }
wxString MetadataItem::getSqlSecurity() { return ""; }
wxString MetadataItem::getDropSqlStatement() const { return ""; }
MetadataItem* MetadataItem::getParent() const { return parentM; }
bool MetadataItem::isSystem() const { return false; }
void MetadataItem::acceptVisitor(MetadataItemVisitor*) {}

void MetadataItem::loadDescription() {}
void MetadataItem::saveDescription(const wxString&) {}
void MetadataItem::saveDescription(const wxString&, const wxString&) {}
wxString MetadataItem::getDescription() { return ""; }
bool MetadataItem::getDescription(wxString&) { return false; }

void MetadataItem::loadProperties() { propertiesLoadedM = lsLoaded; }
void MetadataItem::loadChildren() {}
void MetadataItem::lockChildren() {}
void MetadataItem::unlockChildren() {}
void MetadataItem::doSetChildrenLoaded(bool loaded) { childrenLoadedM = (loaded ? lsLoaded : lsNotLoaded); }
bool MetadataItem::childrenLoaded() const { return childrenLoadedM == lsLoaded; }
void MetadataItem::setChildrenLoaded(bool loaded) { childrenLoadedM = (loaded ? lsLoaded : lsNotLoaded); }
bool MetadataItem::propertiesLoaded() const { return propertiesLoadedM == lsLoaded; }
void MetadataItem::setPropertiesLoaded(bool loaded) { propertiesLoadedM = (loaded ? lsLoaded : lsNotLoaded); }
bool MetadataItem::getChildren(std::vector<MetadataItem*>&) { return false; }
void MetadataItem::ensureChildrenLoaded()
{
    if (!childrenLoaded())
        loadChildren();
}

void initializeLockCount(MetadataItem*, unsigned) {}
void initializeLockCount(MetadataItemPtr, unsigned) {}

void MetadataItem::lockSubject() {}
void MetadataItem::unlockSubject() {}
void MetadataItem::invalidate()
{
    setChildrenLoaded(false);
    setPropertiesLoaded(false);
    notifyObservers();
}

DatabasePtr MetadataItem::getDatabase() const
{
    if (parentM)
        return parentM->getDatabase();
    return DatabasePtr();
}

fr::IDatabasePtr MetadataItem::getDALDatabase() const { return nullptr; }

void DatabaseODSVersion::load(const fr::DatabaseInfoData& infoData)
{
    odsMajorM = infoData.ods;
    odsMinorM = infoData.odsMinor;
    odsFB60OrHigher = checkVersionIsAtLeast(fr::ODSVersion::ODS_FB60_MAJOR, fr::ODSVersion::ODS_FB60_MINOR);
    odsFB50OrHigher = odsFB60OrHigher
        || checkVersionIsAtLeast(fr::ODSVersion::ODS_FB50_MAJOR, fr::ODSVersion::ODS_FB50_MINOR);
    odsFB40OrHigher = odsFB50OrHigher
        || checkVersionIsAtLeast(fr::ODSVersion::ODS_FB40_MAJOR, fr::ODSVersion::ODS_FB40_MINOR);
    odsFB30OrHigher = odsFB40OrHigher
        || checkVersionIsAtLeast(fr::ODSVersion::ODS_FB30_MAJOR, fr::ODSVersion::ODS_FB30_MINOR);
    odsFB25OrHigher = odsFB30OrHigher
        || checkVersionIsAtLeast(fr::ODSVersion::ODS_FB25_MAJOR, fr::ODSVersion::ODS_FB25_MINOR);
    odsFB21OrHigher = odsFB25OrHigher
        || checkVersionIsAtLeast(fr::ODSVersion::ODS_FB21_MAJOR, fr::ODSVersion::ODS_FB21_MINOR);
    odsFB20OrHigher = odsFB21OrHigher
        || checkVersionIsAtLeast(fr::ODSVersion::ODS_FB20_MAJOR, fr::ODSVersion::ODS_FB20_MINOR);
    odsFB15OrHigher = odsFB20OrHigher
        || checkVersionIsAtLeast(fr::ODSVersion::ODS_FB15_MAJOR, fr::ODSVersion::ODS_FB15_MINOR);
}

bool DatabaseODSVersion::checkVersionIsAtLeast(int major, int minor) const
{
    return odsMajorM > major || (odsMajorM == major && odsMinorM >= minor);
}

DatabaseAuthenticationMode::DatabaseAuthenticationMode() {}

Database::Database() : MetadataItem(ntDatabase, nullptr, "embedded_test", 0)
{
    fr::DatabaseInfoData data;
    data.ods = gState.odsMajor;
    data.odsMinor = gState.odsMinor;
    databaseODSVersionM.load(data);
}

Database::~Database() {}

const DatabaseODSVersion& Database::getODSVersion() const { return databaseODSVersionM; }

MetadataLoader* Database::getMetadataLoader()
{
    static MetadataLoader* dummy = reinterpret_cast<MetadataLoader*>(0x1);
    return dummy;
}

wxMBConv* Database::getCharsetConverter() const { return &wxConvUTF8; }

wxArrayString Database::loadIdentifiers(const wxString&, ProgressIndicator* indicator)
{
    checkProgressIndicatorCanceled(indicator);
    if (gState.throwOnLoadIdentifiers)
        throw std::runtime_error(gState.errorMessage);
    wxArrayString result;
    for (const auto& r : gState.rows)
        result.Add(wxString::FromUTF8(r.userName.c_str()));
    return result;
}

ServerPtr Database::getServer() const { return ServerPtr(); }
DatabasePtr Database::getDatabase() const { return std::const_pointer_cast<Database>(shared_from_this()); }
const wxString Database::getTypeName() const { return "DATABASE"; }
const wxString Database::getId() const { return getName_(); }
wxString Database::getSqlSecurity() { return ""; }
void Database::acceptVisitor(MetadataItemVisitor*) {}
fr::IDatabasePtr Database::getDALDatabase() const { return nullptr; }
void Database::loadChildren() {}
void Database::lockChildren() {}
void Database::unlockChildren() {}
bool Database::getChildren(std::vector<MetadataItem*>&) { return false; }
void Database::loadProperties() { MetadataItem::loadProperties(); }
void Database::invalidate() { MetadataItem::invalidate(); }

MetadataLoaderTransaction::MetadataLoaderTransaction(MetadataLoader*) {}
MetadataLoaderTransaction::~MetadataLoaderTransaction() {}

fr::IStatementPtr MetadataLoader::createStatement(const std::string& sql)
{
    ++gState.createStatementCalls;
    if (gState.throwOnCreateStatement)
        throw std::runtime_error(gState.errorMessage);
    return std::make_shared<FakeStatement>(sql);
}

fr::IStatementPtr& MetadataLoader::getStatement(const std::string&)
{
    ++gState.getStatementCalls;
    static fr::IStatementPtr dummy;
    return dummy;
}

wxString std2wxIdentifier(const std::string& str, wxMBConv* conv)
{
    wxString s(str.c_str(), conv ? *conv : wxConvUTF8);
    return s.Strip(wxString::trailing);
}

std::string wx2std(const wxString& input, wxMBConv* conv)
{
    if (input.empty())
        return "";
    wxCharBuffer buf = input.mb_str(conv ? *conv : wxConvUTF8);
    return buf.data() ? std::string(buf.data()) : std::string();
}

bool Config::getValue(const wxString&, bool&) { return false; }
Config::Config() {}
Config::~Config() {}
wxFileName Config::getConfigFileName() const { return wxFileName(); }
bool Config::keyExists(const wxString&) const { return false; }
bool Config::getValue(const wxString&, wxString&) { return false; }
bool Config::setValue(const wxString&, const wxString&) { return false; }
void Config::lockedChanged(bool) {}
FRConfig& config()
{
    static FRConfig c;
    return c;
}
wxFileName FRConfig::getConfigFileName() const { return wxFileName(); }
ConfigCache::ConfigCache(Config&) {}
void ConfigCache::ensureCacheValid() {}
void ConfigCache::update() {}
void ConfigCache::loadFromConfig() {}

int main()
{
    bool ok = true;
    std::cout << "Running Users::load() embedded fallback tests (issue #766)...\n";

    // Test 1: FB 5.0 (ODS 13.1) embedded deploy without SECURITY5.FDB / srp.dll
    // (execute() throws when SEC$USERS initializes UserManager)
    {
        gState = TestLoadState{};
        gState.odsMajor = 13;
        gState.odsMinor = 1;
        gState.throwOnExecute = true;

        DatabasePtr db(new Database());
        UsersPtr users(new Users(db));

        bool threw = false;
        try
        {
            users->load(nullptr);
        }
        catch (...)
        {
            threw = true;
        }

        ok = check(!threw, "FB 5.0: missing SECURITY5.FDB / UserManager plugin does not throw from Users::load()") && ok;
        ok = check(users->childrenLoaded(), "FB 5.0: Users collection is marked loaded after SEC$USERS failure") && ok;
        ok = check(users->empty(), "FB 5.0: Users collection is empty after SEC$USERS failure") && ok;
        ok = check(gState.createStatementCalls == 1 && gState.getStatementCalls == 0,
            "FB 5.0: Users::load() uses uncached createStatement() rather than caching failed statement") && ok;
    }

    // Test 2: FB 5.0 (ODS 13.1) prepare/createStatement throws (e.g. SEC$USERS unavailable)
    {
        gState = TestLoadState{};
        gState.odsMajor = 13;
        gState.odsMinor = 1;
        gState.throwOnCreateStatement = true;

        DatabasePtr db(new Database());
        UsersPtr users(new Users(db));

        bool threw = false;
        try
        {
            users->load(nullptr);
        }
        catch (...)
        {
            threw = true;
        }

        ok = check(!threw, "FB 5.0: createStatement failure on SEC$USERS does not throw") && ok;
        ok = check(users->childrenLoaded() && users->empty(),
            "FB 5.0: createStatement failure leaves Users loaded and empty") && ok;
    }

    // Test 3: FB 2.5 (ODS 11.2) embedded / restricted SEC$USERS failure
    {
        gState = TestLoadState{};
        gState.odsMajor = 11;
        gState.odsMinor = 2;
        gState.throwOnLoadIdentifiers = true;

        DatabasePtr db(new Database());
        UsersPtr users(new Users(db));

        bool threw = false;
        try
        {
            users->load(nullptr);
        }
        catch (...)
        {
            threw = true;
        }

        ok = check(!threw, "FB 2.5: loadIdentifiers failure on SEC$USERS does not throw") && ok;
        ok = check(users->childrenLoaded() && users->empty(),
            "FB 2.5: loadIdentifiers failure leaves Users loaded and empty") && ok;
    }

    // Test 4: FB 5.0 (ODS 13.1) normal server / full install loads SEC$USERS rows
    {
        gState = TestLoadState{};
        gState.odsMajor = 13;
        gState.odsMinor = 1;
        gState.rows = {
            {"ALICE", false, "Alice", true, "", false, "Smith", false, "Srp"},
            {"SYSDBA", true, "", true, "", true, "", false, "Srp"}
        };

        DatabasePtr db(new Database());
        UsersPtr users(new Users(db));
        users->load(nullptr);

        ok = check(users->childrenLoaded() && !users->empty(),
            "FB 5.0: Users::load() populates collection when SEC$USERS is available") && ok;

        UserPtr alice = users->findByName("ALICE");
        UserPtr sysdba = users->findByName("SYSDBA");
        ok = check(alice != nullptr && alice->getFirstName() == "Alice"
            && alice->getLastName() == "Smith" && alice->getPlugin() == "Srp",
            "FB 5.0: Loaded user attributes and plugin are preserved") && ok;
        ok = check(sysdba != nullptr && sysdba->getUsername() == "SYSDBA",
            "FB 5.0: SYSDBA row is loaded") && ok;
    }

    // Test 5: CancelProgressException is still propagated (not swallowed)
    {
        gState = TestLoadState{};
        gState.odsMajor = 13;
        gState.odsMinor = 1;
        gState.rows = {
            {"SYSDBA", true, "", true, "", true, "", false, "Srp"}
        };

        DatabasePtr db(new Database());
        UsersPtr users(new Users(db));
        FakeProgressIndicator canceledIndicator(true);

        bool caughtCancel = false;
        try
        {
            users->load(&canceledIndicator);
        }
        catch (CancelProgressException&)
        {
            caughtCancel = true;
        }

        ok = check(caughtCancel, "CancelProgressException is rethrown and not swallowed") && ok;
    }

    std::cout << (ok ? "\nAll Users embedded fallback tests PASSED.\n"
                     : "\nSome Users embedded fallback tests FAILED.\n");
    return ok ? 0 : 1;
}
