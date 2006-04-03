/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena, Michael Hieke
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
    int getSize();

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

    DatabaseInfo databaseInfoM;

    MetadataCollection<Domain> domainsM;
    MetadataCollection<Exception> exceptionsM;
    MetadataCollection<Function> functionsM;
    MetadataCollection<Generator> generatorsM;
    MetadataCollection<Procedure> proceduresM;
    MetadataCollection<Role> rolesM;
    MetadataCollection<Table> tablesM;
    MetadataCollection<Trigger> triggersM;
    MetadataCollection<View> viewsM;

    std::multimap<wxString, wxString> collationsM;
    void loadCollations();
    bool loadObjects(NodeType type, IBPP::Transaction& tr1,
        ProgressIndicator* indicator = 0);

    // small help for parser
    wxString getTableForIndex(wxString indexName);

    mutable unsigned int idM;
protected:
    virtual void lockChildren();
    virtual void unlockChildren();

public:
    Database();
    Database(const Database& rhs);

    virtual bool getChildren(std::vector<MetadataItem *>& temp);
    void getCollections(std::vector<MetadataItem *>& temp);

    template<class T>
    MetadataCollection<T>* getCollection()
    {
        std::vector<MetadataItem *> temp;
        getCollections(temp);
        for (std::vector<MetadataItem *>::iterator it = temp.begin();
            it != temp.end(); ++it)
        {
            MetadataCollection<T>* p =
                dynamic_cast<MetadataCollection<T>*>(*it);
            if (p)
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
    bool connect(wxString password, ProgressIndicator* indicator = 0);
    bool disconnect(bool onlyDBH = false);
    bool reconnect() const;
    void prepareTemporaryCredentials();
    void resetCredentials();
    bool drop();

    wxString loadDomainNameForColumn(wxString table, wxString field);
    Domain* loadMissingDomain(wxString name);
    //wxString getLoadingSql(NodeType type);

    bool loadGeneratorValues();
    Relation* getRelationForTrigger(Trigger* trigger);

    MetadataItem* findByNameAndType(NodeType nt, wxString name);
    MetadataItem* findByName(wxString name);
    Relation* findRelation(const Identifier& name);
    void refreshByType(NodeType type);
    void dropObject(MetadataItem *object);
    bool addObject(NodeType type, wxString name);
    bool parseCommitedSql(wxString sql);     // reads a DDL statement and does accordingly

    std::vector<wxString> getCollations(wxString charset);

    //! fill vector with names of all tables, views, etc.
    void getIdentifiers(std::vector<Identifier>& temp);

    //! fill vector with result of sql statement
    bool fillVector(std::vector<wxString>& list, wxString sql);

    wxString getPath() const;
    wxString getDatabaseCharset() const;
    wxString getConnectionCharset() const;
    bool usesDifferentConnectionCharset() const;
    wxString getUsername() const;
    wxString getPassword() const;
    wxString getRole() const;
    IBPP::Database& getIBPPDatabase();
    void setPath(wxString value);
    void setConnectionCharset(wxString value);
    void setUsername(wxString value);
    void setPassword(wxString value);
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
