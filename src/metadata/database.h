/*
  Copyright (c) 2004-2009 The FlameRobin Development Team

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
class Server;
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
    wxString getCreated() const;

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

    MetadataCollection<Domain> domainsM;
    MetadataCollection<Exception> exceptionsM;
    MetadataCollection<Function> functionsM;
    MetadataCollection<Generator> generatorsM;
    MetadataCollection<Procedure> proceduresM;
    MetadataCollection<Role> rolesM;
    MetadataCollection<Table> tablesM;
    MetadataCollection<Table> sysTablesM;
    MetadataCollection<Trigger> triggersM;
    MetadataCollection<View> viewsM;

    void setDisconnected();

    std::multimap<CharacterSet, wxString> collationsM;
    void loadCollations();

    template<class T>
    void loadCollection(ProgressIndicator* progressIndicator,
        MetadataCollection<T>& collection, NodeType type,
        MetadataLoader* loader,std::string loadStatement);
    void loadCollections(ProgressIndicator* progressIndicator);

    // small help for parser
    wxString getTableForIndex(wxString indexName);

    mutable unsigned int idM;

    bool showSysTables();

protected:
    virtual void loadChildren();
    virtual void lockChildren();
    virtual void unlockChildren();

public:
    Database();
    Database(const Database& rhs);
    ~Database();

    virtual bool getChildren(std::vector<MetadataItem *>& temp);
    void getCollections(std::vector<MetadataItem *>& temp, bool system);

    template<class T>
    MetadataCollection<T>* getCollection()
    {
        std::vector<MetadataItem *> temp;
        getCollections(temp, false);    // not system
        for (std::vector<MetadataItem *>::iterator it = temp.begin();
            it != temp.end(); ++it)
        {
            MetadataCollection<T>* p =
                dynamic_cast<MetadataCollection<T>*>(*it);
            if (p && !p->isSystem())
                return p;
        }
        return 0;
    }

    MetadataCollection<Generator>::const_iterator generatorsBegin();
    MetadataCollection<Generator>::const_iterator generatorsEnd();
    MetadataCollection<Domain>::const_iterator domainsBegin();
    MetadataCollection<Domain>::const_iterator domainsEnd();
    MetadataCollection<Table>::const_iterator tablesBegin();
    MetadataCollection<Table>::const_iterator tablesEnd();

    void clear();               // sets all values to empty wxString
    bool isConnected() const;
    void connect(wxString password, ProgressIndicator* indicator = 0);
    void disconnect();
    void reconnect();
    void prepareTemporaryCredentials();
    void resetCredentials();
    void drop();

    MetadataLoader* getMetadataLoader();

    wxString loadDomainNameForColumn(wxString table, wxString field);
    Domain* loadMissingDomain(wxString name);
    //wxString getLoadingSql(NodeType type);

    void loadGeneratorValues();
    Relation* getRelationForTrigger(Trigger* trigger);

    MetadataItem* findByNameAndType(NodeType nt, wxString name);
    MetadataItem* findByName(wxString name);
    Relation* findRelation(const Identifier& name);
    void refreshByType(NodeType type);
    void dropObject(MetadataItem *object);
    void addObject(NodeType type, wxString name);
    void parseCommitedSql(const SqlStatement& stm);     // reads a DDL statement and does accordingly

    CharacterSet getCharsetById(int id);
    std::vector<wxString> getCollations(const wxString& charset);
    bool isDefaultCollation(const wxString& charset, const wxString& collate);

    //! fill vector with names of all tables, views, etc.
    void getIdentifiers(std::vector<Identifier>& temp);

    //! fill vector with result of sql statement
    void fillVector(std::vector<wxString>& list, wxString sql);

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
    void setId(int id);
    virtual void acceptVisitor(MetadataItemVisitor* visitor);

    const DatabaseInfo& getInfo();
    void loadInfo();

    wxMBConv* getCharsetConverter() const;
};
//----------------------------------------------------------------------------
#endif
