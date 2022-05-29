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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/xml/xml.h>

#include <algorithm>

#include "config/Config.h"
#include "core/StringUtils.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/root.h"
#include "metadata/server.h"

static const wxString getNodeContent(wxXmlNode* node, const wxString& defvalue)
{
    for (wxXmlNode* n = node->GetChildren(); (n); n = n->GetNext())
    {
        if (n->GetType() == wxXML_TEXT_NODE
            || n->GetType() == wxXML_CDATA_SECTION_NODE)
        {
            return n->GetContent();
        }
    }
    return defvalue;
}

Root::Root()
    : MetadataItem(ntRoot, 0, _("Home"))
{
    setChildrenLoaded(true);
}

void Root::disconnectAllDatabases()
{
    for (ServerPtrs::iterator its = serversM.begin();
        its != serversM.end(); ++its)
    {
        DatabasePtrs databases((*its)->getDatabases());
        std::for_each(databases.begin(), databases.end(),
            std::mem_fn(&Database::disconnect));
    }
}

Root::~Root()
{
}

//! loads fr_databases.conf file and:
//! creates server nodes, fills their properties
//! creates database nodes for server nodes, fills their properties
//! returns: false if file cannot be loaded, true otherwise
//
bool Root::load()
{
    wxXmlDocument doc;
    wxFileName fileName = getFileName();
    if (fileName.FileExists())
    {
        wxFileInputStream stream(fileName.GetFullPath());
        if (stream.Ok())
            doc.Load(stream);
    }
    if (!doc.IsOk())
        return false;

    wxXmlNode* xmlr = doc.GetRoot();
    if (xmlr->GetName() != "root")
        return false;

    SubjectLocker locker(this);
    for (wxXmlNode* xmln = doc.GetRoot()->GetChildren();
        (xmln); xmln = xmln->GetNext())
    {
        if (xmln->GetType() != wxXML_ELEMENT_NODE)
            continue;
        if (xmln->GetName() == "server")
            parseServer(xmln);
        if (xmln->GetName() == "nextId")
        {
            wxString value(getNodeContent(xmln, wxEmptyString));
            unsigned long l;
            // nextIdM may have been written already (database id)
            if (!value.IsEmpty() && value.ToULong(&l))
            {
                if (Database::getUIDGeneratorValue() < l)
                    Database::setUIDGeneratorValue(l);
            }
        }
    }
    return true;
}

bool Root::parseDatabase(ServerPtr server, wxXmlNode* xmln)
{
    wxASSERT(server);
    wxASSERT(xmln);
    DatabasePtr database = server->addDatabase();
    SubjectLocker locker(database.get());

    for (xmln = xmln->GetChildren(); (xmln); xmln = xmln->GetNext())
    {
        if (xmln->GetType() != wxXML_ELEMENT_NODE)
            continue;

        wxString value(getNodeContent(xmln, wxEmptyString));
        if (xmln->GetName() == "name")
            database->setName_(value);
        else if (xmln->GetName() == "path")
            database->setPath(value);
        else if (xmln->GetName() == "charset")
            database->setConnectionCharset(value);
        else if (xmln->GetName() == "username")
            database->setUsername(value);
        else if (xmln->GetName() == "password")
            database->setRawPassword(value);
        else if (xmln->GetName() == "encrypted" && value == "1")
            database->getAuthenticationMode().setStoreEncryptedPassword();
        else if (xmln->GetName() == "authentication")
            database->getAuthenticationMode().setConfigValue(value);
        else if (xmln->GetName() == "role")
            database->setRole(value);
        else if (xmln->GetName() == "id")
        {
            unsigned long id;
            if (value.ToULong(&id))
                database->setId(id);
        }
        else if (xmln->GetName() == "fbclient")
            database->setClientLibrary(value);
    }

    // make sure the database has an Id before Root::save() is called,
    // otherwise a new Id will be generated then, but the generator value
    // will not be stored because it's at the beginning of the file.
    database->getId();
    return true;
}

bool Root::parseServer(wxXmlNode* xmln)
{
    wxASSERT(xmln);
    ServerPtr server = addServer();
    SubjectLocker locker(server.get());

    for (xmln = xmln->GetChildren(); (xmln); xmln = xmln->GetNext())
    {
        if (xmln->GetType() != wxXML_ELEMENT_NODE)
            continue;

        wxString value(getNodeContent(xmln, wxEmptyString));
        if (xmln->GetName() == "name")
            server->setName_(value);
        else if (xmln->GetName() == "host")
            server->setHostname(value);
        else if (xmln->GetName() == "port")
            server->setPort(value);
        else if (xmln->GetName() == "database")
        {
            if (!parseDatabase(server, xmln))
                return false;
        }
    }
    // backward compatibility with FR < 0.3.0
    if (server->getName_().IsEmpty())
        server->setName_(server->getConnectionString());
    server->setChildrenLoaded(true);
    return true;
}

ServerPtr Root::addServer()
{
    ServerPtr server(new Server());
    addServer(server);
    return server;
}

void Root::addServer(ServerPtr server)
{
    if (server)
    {
        serversM.push_back(server);
        server->setParent(this);
        notifyObservers();
    }
}

void Root::removeServer(ServerPtr server)
{
    if (unregLocalDatabasesM == server)
        unregLocalDatabasesM.reset();

    ServerPtrs::iterator it = std::remove(serversM.begin(), serversM.end(),
        server);
    if (it != serversM.end())
    {
        serversM.erase(it, serversM.end());
        notifyObservers();
    }
}

void Root::addUnregisteredDatabase(DatabasePtr database)
{
    // on-demand creation of parent node for unregistered databases
    if (!unregLocalDatabasesM)
    {
        ServerPtr server(new Server());
        serversM.push_back(server);
        server->setName_(_("Unregistered local databases"));
        server->setHostname("localhost");
        server->setParent(this);

        unregLocalDatabasesM = server;
        notifyObservers();
    }

    unregLocalDatabasesM->addDatabase(database);
}

// helper for Root::save()
void rsAddChildNode(wxXmlNode* parentNode, const wxString nodeName,
    const wxString nodeContent)
{
    if (!nodeContent.IsEmpty())
    {
        wxXmlNode* propn = new wxXmlNode(wxXML_ELEMENT_NODE, nodeName);
        parentNode->AddChild(propn);
        propn->AddChild(new wxXmlNode(wxXML_TEXT_NODE, wxEmptyString,
            nodeContent));
    } 
}

// browses the server nodes, and their database nodes
// saves everything to fr_databases.conf file
// returns: false if file cannot be opened for writing, true otherwise
//
bool Root::save()
{
    // create directory if it doesn't exist yet.
    wxString dir = wxPathOnly(getFileName());
    if (!wxDirExists(dir))
        wxMkdir(dir);

    wxXmlDocument doc;
    wxXmlNode* rn = new wxXmlNode(wxXML_ELEMENT_NODE, "root");
    doc.SetRoot(rn);

    rsAddChildNode(rn, "nextId",
        wxString::Format("%d", Database::getUIDGeneratorValue()));

    for (ServerPtrs::iterator its = serversM.begin();
        its != serversM.end(); ++its)
    {
        // do not save the dummy server node for databases that were opened
        // either via command line switch or via drag and drop
        if ((*its) == unregLocalDatabasesM)
            continue;

        wxXmlNode* srvn = new wxXmlNode(wxXML_ELEMENT_NODE, "server");
        rn->AddChild(srvn);
        
        rsAddChildNode(srvn, "name", (*its)->getName_());
        rsAddChildNode(srvn, "host", (*its)->getHostname());
        rsAddChildNode(srvn, "port", (*its)->getPort());

        DatabasePtrs databases((*its)->getDatabases());
        for (DatabasePtrs::iterator itdb = databases.begin();
            itdb != databases.end(); ++itdb)
        {
            (*itdb)->resetCredentials();    // clean up eventual extra credentials

            wxXmlNode* dbn = new wxXmlNode(wxXML_ELEMENT_NODE, "database");
            srvn->AddChild(dbn);

            rsAddChildNode(dbn, "id", (*itdb)->getId());
            rsAddChildNode(dbn, "name", (*itdb)->getName_());
            rsAddChildNode(dbn, "path", (*itdb)->getPath());
            rsAddChildNode(dbn, "charset", (*itdb)->getConnectionCharset());
            rsAddChildNode(dbn, "username", (*itdb)->getUsername());
            rsAddChildNode(dbn, "password", (*itdb)->getRawPassword());
            rsAddChildNode(dbn, "role", (*itdb)->getRole());
            rsAddChildNode(dbn, "fbclient", (*itdb)->getClientLibrary());
            rsAddChildNode(dbn, "authentication",
                (*itdb)->getAuthenticationMode().getConfigValue());
        }
    }
    return doc.Save(getFileName());
}

ServerPtrs Root::getServers() const
{
    return serversM;
}

bool Root::getChildren(std::vector<MetadataItem *>& temp)
{
    if (serversM.empty())
        return false;
    std::transform(serversM.begin(), serversM.end(), std::back_inserter(temp),
        std::mem_fn(&ServerPtr::get));
    return !serversM.empty();
}

void Root::lockChildren()
{
    std::for_each(serversM.begin(), serversM.end(),
        std::mem_fn(&Server::lockSubject));
}

void Root::unlockChildren()
{
    std::for_each(serversM.begin(), serversM.end(),
        std::mem_fn(&Server::unlockSubject));
}

const wxString Root::getItemPath() const
{
    // Root is root, don't make the path strings any longer than needed.
    return "";
}

wxString Root::getFileName()
{
    if (fileNameM.empty())
        fileNameM = config().getDBHFileName();
    return fileNameM;
}

void Root::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitRoot(*this);
}

const wxString Root::getTypeName() const
{
    return "ROOT";
}

