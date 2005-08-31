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

  Contributor(s): Nando Dessena
*/

#ifndef FR_DATABASE_H
#define FR_DATABASE_H
//-----------------------------------------------------------------------------
#include <map>

#include <ibpp.h>

#include "collection.h"
#include "domain.h"
#include "exception.h"
#include "function.h"
#include "generator.h"
#include "metadataitem.h"
#include "procedure.h"
#include "role.h"
#include "table.h"
#include "trigger.h"
#include "view.h"
//-----------------------------------------------------------------------------
class Server;
//-----------------------------------------------------------------------------
class Credentials
{
private:
    std::string charsetM;
    std::string roleM;
    std::string usernameM;
    std::string passwordM;

public:
    std::string getCharset() const;
    std::string getUsername() const;
    std::string getPassword() const;
    std::string getRole() const;
    void setCharset(std::string value);
    void setUsername(std::string value);
    void setPassword(std::string value);
    void setRole(std::string value);
};
//-----------------------------------------------------------------------------
class Database: public MetadataItem
{
private:
    IBPP::Database databaseM;
    bool connectedM;

    std::string pathM;
    Credentials credentials;
    Credentials *connectionCredentials;

    MetadataCollection<Domain> domainsM;
    MetadataCollection<Exception> exceptionsM;
    MetadataCollection<Function> functionsM;
    MetadataCollection<Generator> generatorsM;
    MetadataCollection<Procedure> proceduresM;
    MetadataCollection<Role> rolesM;
    MetadataCollection<Table> tablesM;
    MetadataCollection<Trigger> triggersM;
    MetadataCollection<View> viewsM;

    std::multimap<std::string, std::string> collationsM;
    void loadCollations();

    // small help for parser
    std::string getTableForIndex(std::string indexName);

    mutable unsigned int idM;
public:
    Database();
    void initChildren();
    virtual bool getChildren(std::vector<MetadataItem *>& temp);
    void getCollections(std::vector<MetadataItem *>& temp);

    MetadataCollection<Generator>::const_iterator generatorsBegin();
    MetadataCollection<Generator>::const_iterator generatorsEnd();
    MetadataCollection<Domain>::const_iterator domainsBegin();
    MetadataCollection<Domain>::const_iterator domainsEnd();
    MetadataCollection<Table>::const_iterator tablesBegin();
    MetadataCollection<Table>::const_iterator tablesEnd();

    void clear();               // sets all values to empty string
    bool isConnected() const;
    bool connect(std::string password);
    bool disconnect();
    bool reconnect() const;
    void prepareTemporaryCredentials();
    void resetCredentials();

    std::string loadDomainNameForColumn(std::string table, std::string field);
    Domain *loadMissingDomain(std::string name);
    bool loadObjects(NodeType type);
    //std::string getLoadingSql(NodeType type);

    bool loadGeneratorValues();

    MetadataItem *findByNameAndType(NodeType nt, std::string name);
    MetadataItem *findByName(std::string name);
    void refreshByType(NodeType type);
    void dropObject(MetadataItem *object);
    bool addObject(NodeType type, std::string name);
    bool parseCommitedSql(std::string sql);     // reads a DDL statement and does accordingly

    std::vector<std::string> getCollations(std::string charset);

    //! fill vector with names of all tables, views, etc.
    void getIdentifiers(std::vector<std::string>& temp);

    //! fill vector with result of sql statement
    bool fillVector(std::vector<std::string>& list, std::string sql);

    std::string getPath() const;
    std::string getCharset() const;
    std::string getUsername() const;
    std::string getPassword() const;
    std::string getRole() const;
    IBPP::Database& getIBPPDatabase();
    void setPath(std::string value);
    void setCharset(std::string value);
    void setUsername(std::string value);
    void setPassword(std::string value);
    void setRole(std::string value);
    virtual const std::string getTypeName() const;
    Server *getServer() const;
    // returns the complete connection string.
    std::string getConnectionString() const;
    // returns a candidate name based on the connection string. Example:
    // path is "C:\data\database.fdb" -> returns "database".
    std::string extractNameFromConnectionString() const;
    virtual const string getId() const;
    void setId(int id);
    virtual void acceptVisitor(MetadataItemVisitor* visitor);
};
//----------------------------------------------------------------------------
#endif
