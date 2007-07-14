/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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
#include <map>

#include <ibpp.h>

#include "core/ProgressIndicator.h"
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
#include "sql/SqlStatement.h"
//-----------------------------------------------------------------------------
class Server;
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

public:
    void loadInfo(const IBPP::Database* database);

    wxString getCreated();

    int getODS();
    int getODSMinor();

    int getDialect();

    int getPageSize();
    int getBuffers();
    int getPages();

    int getOldestTransaction();
    int getNextTransaction();

    int getSweep();

    bool getReadOnly();
    bool getForcedWrites();
};
//-----------------------------------------------------------------------------
class Database: public MetadataItem
{
private:
    IBPP::Database databaseM;
    bool connectedM;
    wxString databaseCharsetM;

    wxString pathM;
    Credentials credentialsM;
    Credentials* connectionCredentialsM;
    bool storeEncryptedPasswordM;

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

    std::multimap<wxString, wxString> collationsM;
    void loadCollations();
    void loadObjects(NodeType type, IBPP::Transaction& tr1,
        ProgressIndicator* indicator = 0);

    // small help for parser
    wxString getTableForIndex(wxString indexName);

    mutable unsigned int idM;

    bool showSysTables();

protected:
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
    void disconnect(bool onlyDBH = false);
    void reconnect() const;
    void prepareTemporaryCredentials();
    void resetCredentials();
    void drop();

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
    bool addObject(NodeType type, wxString name);
    void parseCommitedSql(const SqlStatement& stm);     // reads a DDL statement and does accordingly

    std::vector<wxString> getCollations(wxString charset);
    bool isDefaultCollation(const wxString& charset, const wxString& collate);

    //! fill vector with names of all tables, views, etc.
    void getIdentifiers(std::vector<Identifier>& temp);

    //! fill vector with result of sql statement
    void fillVector(std::vector<wxString>& list, wxString sql);

    wxString getPath() const;
    wxString getDatabaseCharset() const;
    wxString getConnectionCharset() const;
    bool usesDifferentConnectionCharset() const;
    wxString getUsername() const;
    wxString getRawPassword() const;
    wxString getDecryptedPassword() const;
    bool getStoreEncryptedPassword() const;
    wxString getRole() const;
    IBPP::Database& getIBPPDatabase();
    void setPath(wxString value);
    void setConnectionCharset(wxString value);
    void setUsername(wxString value);
    void setRawPassword(wxString value);
    void setEncryptedPassword(wxString value);
    void setStoreEncryptedPassword(bool value);
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

    DatabaseInfo* getInfo() const;
};
//----------------------------------------------------------------------------
#endif
