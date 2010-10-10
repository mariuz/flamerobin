/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

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


  $Id$

*/

#ifndef FR_DATABASE_H
#define FR_DATABASE_H
//-----------------------------------------------------------------------------
#include <wx/strconv.h>

#include <map>

#include <ibpp.h>

#include "frtypes.h"
#include "metadata/collection.h"
#include "metadata/domain.h"
#include "metadata/exception.h"
#include "metadata/function.h"
#include "metadata/generator.h"
#include "metadata/MetadataClasses.h"
#include "metadata/metadataitem.h"
#include "metadata/procedure.h"
#include "metadata/role.h"
#include "metadata/table.h"
#include "metadata/trigger.h"
#include "metadata/view.h"
//-----------------------------------------------------------------------------
class Database;
class MetadataLoader;
class ProgressIndicator;
class SqlStatement;
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
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
    void setCharset(wxString value);
    void setUsername(wxString value);
    void setPassword(wxString value);
    void setRole(wxString value);
};
//-----------------------------------------------------------------------------
class DatabaseInfo
{
    friend class Database;
private:
    int odsM;
    int odsMinorM;
    int dialectM;

    int pageSizeM;
    int buffersM;
    int pagesM;

    int oldestTransactionM;
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

    int getDialect() const;

    int getPageSize() const;
    int getBuffers() const;
    int getPages() const;
    int64_t getSizeInBytes() const;

    int getOldestTransaction() const;
    int getNextTransaction() const;

    int getSweep() const;

    bool getReadOnly() const;
    bool getForcedWrites() const;
    bool getReserve() const;
};
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
class Database: public MetadataItem
{
private:
    IBPP::Database databaseM;
    MetadataLoader* metadataLoaderM;

    bool connectedM;
    wxString databaseCharsetM;

    wxString pathM;
    Credentials credentialsM;
    Credentials* connectionCredentialsM;
    DatabaseAuthenticationMode authenticationModeM;

    wxMBConv* charsetConverterM;
    void createCharsetConverter();

    DatabaseInfo databaseInfoM;

    Domains domainsM;
    Exceptions exceptionsM;
    Functions functionsM;
    Generators generatorsM;
    Procedures proceduresM;
    Roles rolesM;
    SysTables sysTablesM;
    Tables tablesM;
    Triggers triggersM;
    Views viewsM;

    // copy constructor implementation removed since it's no longer needed
    // (Server uses a vector of boost::shared_ptr<Database> now)
    Database(const Database& rhs);

    void setDisconnected();

    std::multimap<CharacterSet, wxString> collationsM;
    void loadCollations();

    void loadCollections(ProgressIndicator* progressIndicator);

    // small help for parser
    wxString getTableForIndex(wxString indexName);

    mutable unsigned idM;

    bool showSysTables();

protected:
    virtual void loadChildren();
    virtual void lockChildren();
    virtual void unlockChildren();

public:
    Database();
    ~Database();

    virtual bool getChildren(std::vector<MetadataItem *>& temp);
    void getCollections(std::vector<MetadataItem *>& temp, bool system);

    Domains* getDomains();
    Exceptions* getExceptions();
    Functions* getFunctions();
    Generators* getGenerators();
    Procedures* getProcedures();
    Roles* getRoles();
    Tables* getTables();
    SysTables* getSysTables();
    Triggers* getTriggers();
    Views* getViews();

    void clear();               // sets all values to empty wxString
    bool isConnected() const;
    void connect(wxString password, ProgressIndicator* indicator = 0);
    void disconnect();
    void reconnect();
    void prepareTemporaryCredentials();
    void resetCredentials();
    void drop();

    MetadataLoader* getMetadataLoader();

    wxArrayString loadIdentifiers(const wxString& loadStatement,
        ProgressIndicator* progressIndicator = 0);

    wxString loadDomainNameForColumn(wxString table, wxString field);
    Domain* loadMissingDomain(wxString name);

    void loadGeneratorValues();
    Relation* getRelationForTrigger(Trigger* trigger);

    MetadataItem* findByNameAndType(NodeType nt, wxString name);
    MetadataItem* findByName(wxString name);
    Relation* findRelation(const Identifier& name);
    void dropObject(MetadataItem *object);
    void addObject(NodeType type, wxString name);
    void parseCommitedSql(const SqlStatement& stm);     // reads a DDL statement and does accordingly

    CharacterSet getCharsetById(int id);
    std::vector<wxString> getCollations(const wxString& charset);
    bool isDefaultCollation(const wxString& charset, const wxString& collate);

    //! fill vector with names of all tables, views, etc.
    void getIdentifiers(std::vector<Identifier>& temp);

    //! gets the database triggers (FB2.1+)
    void getDatabaseTriggers(std::vector<Trigger *>& list);

    wxString getPath() const;
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
    void setPath(wxString value);
    void setConnectionCharset(wxString value);
    void setUsername(wxString value);
    void setRawPassword(wxString value);
    void setEncryptedPassword(wxString value);
    void setRole(wxString value);
    virtual const wxString getTypeName() const;
    Server *getServer() const;
    // returns the complete connection wxString.
    wxString getConnectionString() const;
    // returns a candidate name based on the connection wxString. Example:
    // path is "C:\data\database.fdb" -> returns "database".
    wxString extractNameFromConnectionString() const;
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

    wxMBConv* getCharsetConverter() const;
};
//----------------------------------------------------------------------------
#endif
