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

#ifndef FR_DATABASE_H
#define FR_DATABASE_H

#include <wx/strconv.h>

#include <map>

#include <ibpp.h>

#include "metadata/MetadataClasses.h"
#include "metadata/metadataitem.h"

class MetadataLoader;
class ProgressIndicator;
class SqlStatement;


/*
class CharacterSet
{
private:
    wxString nameM;
    int idM;
    int bytesPerCharM;
public:
    CharacterSet(const wxString& name, int id = -1, int bytesPerChar = -1);

    bool operator< (const CharacterSet& other) const;
    int getBytesPerChar() const;
    int getId() const;
    wxString getName() const;
};
*/
class Credentials
{
private:
    wxString charsetM;
    wxString roleM;
    wxString usernameM;
    wxString passwordM;

public:
    wxString getCharset() const;
    wxString getUsername() const;
    wxString getPassword() const;
    wxString getRole() const;
    void setCharset(const wxString& value);
    void setUsername(const wxString& value);
    void setPassword(const wxString& value);
    void setRole(const wxString& value);
};

class DatabaseInfo
{
    friend class Database;
private:
    int odsM;
    int odsMinorM;

    int pageSizeM;
    int buffersM;
    int pagesM;

    int oldestTransactionM;
    int oldestActiveTransactionM;
    int oldestSnapshotM;
    int nextTransactionM;

    int sweepM;

    bool readOnlyM;
    bool forcedWritesM;
    bool reserveM;

    mutable wxLongLong loadTimeMillisM;
    void load(const IBPP::Database database);
    void reloadIfNecessary(const IBPP::Database database);
public:
    int getODS() const;
    int getODSMinor() const;
    bool getODSVersionIsHigherOrEqualTo(int versionMajor) const;
    bool getODSVersionIsHigherOrEqualTo(int versionMajor, int versionMinor) const;

    int getPageSize() const;
    int getBuffers() const;
    int getPages() const;
    int64_t getSizeInBytes() const;

    int getOldestTransaction() const;
    int getOldestActiveTransaction() const;
    int getOldestSnapshot() const;
    int getNextTransaction() const;

    int getSweep() const;

    bool getReadOnly() const;
    bool getForcedWrites() const;
    bool getReserve() const;
    
};

class DatabaseAuthenticationMode
{
public:
    DatabaseAuthenticationMode();

    enum Mode { UseSavedPassword, UseSavedEncryptedPwd, AlwaysEnterPassword,
        TrustedUser };
    int getMode() const;
    void setMode(int mode);

    wxString getConfigValue() const;
    void setConfigValue(const wxString& value);
    // support for old "encrypted password" setting
    void setStoreEncryptedPassword();

    bool getAlwaysAskForPassword() const;
    bool getIgnoreUsernamePassword() const;
    bool getUseEncryptedPassword() const;
private:
    Mode modeM;
};

class TimezoneInfo
{
public:
    TimezoneInfo() { name = ""; id = 0; };

    bool empty() { return (name == "") && (id == 0); };

    wxString name;
    uint16_t id;
};

class Database: public MetadataItem,
    public std::enable_shared_from_this<Database>
{
private:
    ServerWeakPtr serverM;
    IBPP::Database databaseM;
    MetadataLoader* metadataLoaderM;

    bool connectedM;
    bool volatileM;
    wxString databaseCharsetM;
    wxString connectionUserM;
    wxString connectionRoleM;

    int lingerM; // ODS 12
    wxString sqlSecurityM; // ODS 13

    wxString pathM;
    wxString clientLibraryM;
    int dialectM;
    Credentials credentialsM;
    Credentials* connectionCredentialsM;
    DatabaseAuthenticationMode authenticationModeM;
    std::vector<TimezoneInfo*> timezonesM;
    TimezoneInfo defaultTimezoneM;

    std::unique_ptr<wxMBConv> charsetConverterM;
    void createCharsetConverter();

    DatabaseInfo databaseInfoM;

    CharacterSetsPtr characterSetsM;
    CollationsPtr collationsM;
    DBTriggersPtr DBTriggersM;
    DDLTriggersPtr DDLTriggersM;
    DMLTriggersPtr DMLtriggersM;
    DomainsPtr userDomainsM;
    ExceptionsPtr exceptionsM;
    FunctionSQLsPtr functionSQLsM;
    GeneratorsPtr generatorsM;
    GTTablesPtr GTTablesM;
    IndicesPtr indicesM;
    PackagesPtr packagesM;
    ProceduresPtr proceduresM;
    RolesPtr rolesM;
    SysIndicesPtr sysIndicesM;
    SysDomainsPtr sysDomainsM;
    SysPackagesPtr sysPackagesM;
    SysRolesPtr sysRolesM;
    SysTablesPtr sysTablesM;
    TablesPtr tablesM;
    UDFsPtr UDFsM;
    UsersPtr usersM;
    UsrIndicesPtr usrIndicesM;
    ViewsPtr viewsM;

    // copy constructor implementation removed since it's no longer needed
    // (Server uses a vector of std::shared_ptr<Database> now)
    Database(const Database& rhs);

    void setDisconnected();

    //std::multimap<CharacterSet, wxString> collationsM;
    void loadCollations();

    void loadCollections(ProgressIndicator* progressIndicator);

    void loadDatabaseInfo();

    void loadDefaultTimezone();
    void loadTimezones();

    // small help for parser
    wxString getTableForIndex(const wxString& indexName);

    mutable unsigned idM;

    bool showSystemCharacterSet();
    bool showSystemIndices();
    bool showSystemDomains();
    bool showSystemPackages();
    bool showSystemRoles();
    bool showSystemTables();
    bool showOneNodeIndices();

    inline void checkConnected(const wxString& operation) const;
protected:
    virtual void loadChildren();
    virtual void lockChildren();
    virtual void unlockChildren();

public:
    Database();
    ~Database();

    virtual bool getChildren(std::vector<MetadataItem *>& temp);
    void getCollections(std::vector<MetadataItem *>& temp, bool system);

    CharacterSetsPtr getCharacterSets();
    CollationsPtr getCollations();
    DBTriggersPtr getDBTriggers();
    DDLTriggersPtr getDDLTriggers();
    DMLTriggersPtr getDMLTriggers();
    DomainsPtr getDomains();
    ExceptionsPtr getExceptions();
    FunctionSQLsPtr getFunctionSQLs();
    GTTablesPtr getGTTables();
    GeneratorsPtr getGenerators();
    IndicesPtr getIndices();
    PackagesPtr getPackages();
    ProceduresPtr getProcedures();
    RolesPtr getRoles();
    SysIndicesPtr getSysIndices();
    SysDomainsPtr getSysDomains();
    SysPackagesPtr getSysPackages();
    SysRolesPtr getSysRoles();
    SysTablesPtr getSysTables();
    TablesPtr getTables();
    UDFsPtr getUDFs();
    UsersPtr getUsers();
    UsrIndicesPtr getUsrIndices();
    ViewsPtr getViews();

    bool isConnected() const;
    void create(int pagesize, int dialect);
    void connect(const wxString& password, ProgressIndicator* indicator = 0);
    void disconnect();
    void reconnect();
    void prepareTemporaryCredentials();
    void resetCredentials();
    void drop();

    MetadataLoader* getMetadataLoader();

    wxArrayString loadIdentifiers(const wxString& loadStatement,
        ProgressIndicator* progressIndicator = 0);

    wxString loadDomainNameForColumn(const wxString& table,
        const wxString& field);
    DomainPtr getDomain(const wxString& name);

    void loadGeneratorValues();
    Relation* getRelationForTrigger(DMLTrigger* trigger);

    virtual DatabasePtr getDatabase() const;
    MetadataItem* findByNameAndType(NodeType nt, const wxString& name);
    MetadataItem* findByName(const wxString& name);
    MetadataItem* findByIdAndType(NodeType nt, const int id);

    Relation* findRelation(const Identifier& name);
    void dropObject(MetadataItem *object);
    void addObject(NodeType type, const wxString& name);
    void parseCommitedSql(const SqlStatement& stm);     // reads a DDL statement and does accordingly

    CharacterSetPtr getCharsetById(int id);
    wxArrayString getCharacterSet();
    wxArrayString getCollations(const wxString& charset);
    bool isDefaultCollation(const wxString& charset, const wxString& collate);

    TimezoneInfo getDefaultTimezone();
    wxString getTimezoneName(int timezone);

    //! fill vector with names of all tables, views, etc.
    void getIdentifiers(std::vector<Identifier>& temp);

    //! gets the database triggers (FB2.1+)
    void getDatabaseTriggers(std::vector<Trigger *>& list);

    bool getIsVolative();
    wxString getPath() const;
    wxString getClientLibrary() const;
    int getSqlDialect() const;
    wxString getDatabaseCharset() const;
    wxString getConnectionCharset() const;
    wxString getConnectionInfoString() const;
    bool usesDifferentConnectionCharset() const;
    wxString getUsername() const;
    wxString getRawPassword() const;
    wxString getDecryptedPassword() const;
    DatabaseAuthenticationMode& getAuthenticationMode();
    wxString getRole() const;
    IBPP::Database& getIBPPDatabase();
    void setIsVolatile(const bool isVolatile);
    void setPath(const wxString& value);
    void setClientLibrary(const wxString& value);
    void setConnectionCharset(const wxString& value);
    void setUsername(const wxString& value);
    void setRawPassword(const wxString& value);
    void setEncryptedPassword(const wxString& value);
    void setRole(const wxString& value);
    virtual const wxString getTypeName() const;
    ServerPtr getServer() const;
    void setServer(ServerPtr server);
    // returns the complete connection wxString.
    wxString getConnectionString() const;
    // returns a candidate name based on the connection wxString. Example:
    // path is "C:\data\database.fdb" -> returns "database".
    static wxString extractNameFromConnectionString(const wxString& path);
    virtual const wxString getId() const;
    void setId(unsigned id);

    // returns the value of the Id generator and increments it afterwards.
    static unsigned getUniqueId();
    // returns the current value of the Id generator.
    static unsigned getUIDGeneratorValue();
    // sets the current value of the Id generator.
    static void setUIDGeneratorValue(unsigned value);

    virtual void acceptVisitor(MetadataItemVisitor* visitor);

    const DatabaseInfo& getInfo();
    void loadInfo();

    void getConnectedUsers(wxArrayString& users) const;
    int getLinger() const; // ODS:12
    wxString getSqlSecurity() const; // ODS:13

    wxMBConv* getCharsetConverter() const;
};

#endif
