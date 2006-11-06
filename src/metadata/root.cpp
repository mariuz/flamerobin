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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/xml/xml.h>

#include <fstream>
#include <sstream>

#include "config/Config.h"
#include "core/StringUtils.h"
#include "metadata/database.h"
#include "metadata/MetadataItemVisitor.h"
#include "metadata/root.h"
#include "metadata/server.h"
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
//! access to the singleton root of the DBH.
Root& getGlobalRoot()
{
    static Root globalRoot;
    return globalRoot;
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
Root::Root()
    : MetadataItem(), fileNameM(wxT("")), dirtyM(false), loadingM(false), nextIdM(1)
{
    setName_(wxT("Home"));
    typeM = ntRoot;
}
//-----------------------------------------------------------------------------
void Root::disconnectAllDatabases()
{
    std::list<Server>::iterator its;
    for (its = serversM.begin(); its != serversM.end(); ++its)
    {
        std::list<Database>::iterator itdb;
        for (itdb = its->getDatabases()->begin(); itdb != its->getDatabases()->end(); ++itdb)
            itdb->disconnect();
    }
}
//-----------------------------------------------------------------------------
Root::~Root()
{
    if (dirtyM)
        save();
}
//-----------------------------------------------------------------------------
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
    if (xmlr->GetName() != wxT("root"))
        return false;

    loadingM = true;
    for (wxXmlNode* xmln = doc.GetRoot()->GetChildren();
        (xmln); xmln = xmln->GetNext())
    {
        if (xmln->GetType() != wxXML_ELEMENT_NODE)
            continue;
        if (xmln->GetName() == wxT("server"))
            parseServer(xmln);
        if (xmln->GetName() == wxT("nextId"))
        {
            wxString value(getNodeContent(xmln, wxEmptyString));
            unsigned long l;
            // nextIdM may have been written already (database id)
            if (!value.IsEmpty() && value.ToULong(&l) && l > nextIdM)
                nextIdM = l;
        }
    }
    dirtyM = false;
    loadingM = false;
    return true;
}
//-----------------------------------------------------------------------------
bool Root::parseDatabase(Server* server, wxXmlNode* xmln)
{
    wxASSERT(server);
    wxASSERT(xmln);
    Database tempDb;
    Database* database = server->addDatabase(tempDb);
    SubjectLocker locker(database);

    for (xmln = xmln->GetChildren(); (xmln); xmln = xmln->GetNext())
    {
        if (xmln->GetType() != wxXML_ELEMENT_NODE)
            continue;

        wxString value(getNodeContent(xmln, wxEmptyString));
        if (xmln->GetName() == wxT("name"))
            database->setName_(value);
        else if (xmln->GetName() == wxT("path"))
            database->setPath(value);
        else if (xmln->GetName() == wxT("charset"))
            database->setConnectionCharset(value);
        else if (xmln->GetName() == wxT("username"))
            database->setUsername(value);
        else if (xmln->GetName() == wxT("password"))
            database->setRawPassword(value);
        else if (xmln->GetName() == wxT("encrypted"))
            database->setStoreEncryptedPassword(value == wxT("1"));
        else if (xmln->GetName() == wxT("role"))
            database->setRole(value);
        else if (xmln->GetName() == wxT("id"))
        {
            unsigned long id;
            if (value.ToULong(&id))
            {
                database->setId(id);
                // force nextIdM to be higher than ids of existing databases
                if (nextIdM <= id)
                    nextIdM = id + 1;
            }
        }
    }
    // make sure the database has an Id before Root::save() is called,
    // otherwise a new Id will be generated then, but the generator value
    // will not be stored because it's at the beginning of the file.
    database->getId();
    // backward compatibility with FR < 0.3.0
    if (database->getName_().IsEmpty())
        database->setName_(database->extractNameFromConnectionString());
    return true;
}
//-----------------------------------------------------------------------------
bool Root::parseServer(wxXmlNode* xmln)
{
    wxASSERT(xmln);
    Server tempSrv;
    Server* server = addServer(tempSrv);
    SubjectLocker locker(server);

    for (xmln = xmln->GetChildren(); (xmln); xmln = xmln->GetNext())
    {
        if (xmln->GetType() != wxXML_ELEMENT_NODE)
            continue;

        wxString value(getNodeContent(xmln, wxEmptyString));
        if (xmln->GetName() == wxT("name"))
            server->setName_(value);
        else if (xmln->GetName() == wxT("host"))
            server->setHostname(value);
        else if (xmln->GetName() == wxT("port"))
            server->setPort(value);
        else if (xmln->GetName() == wxT("database"))
        {
            if (!parseDatabase(server, xmln))
                return false;
        }
    }
    // backward compatibility with FR < 0.3.0
    if (server->getName_().IsEmpty())
        server->setName_(server->getConnectionString());
    return true;
}
//-----------------------------------------------------------------------------
Server* Root::addServer(Server& server)
{
    Server* temp = serversM.add(server);
    temp->setParent(this);                    // grab it from collection
    dirtyM = true;
    notifyObservers();
    save();
    return temp;
}
//-----------------------------------------------------------------------------
void Root::removeServer(Server* server)
{
    serversM.remove(server);
    dirtyM = true;
    notifyObservers();
    save();
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
// browses the server nodes, and their database nodes
// saves everything to fr_databases.conf file
// returns: false if file cannot be opened for writing, true otherwise
//
bool Root::save()
{
    if (loadingM)
        return true;

    // create directory if it doesn't exist yet.
    wxString dir = wxPathOnly(getFileName());
    if (!wxDirExists(dir))
        wxMkdir(dir);
#if 0
    wxXmlDocument doc;
    wxXmlNode* rn = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("root"));
    doc.SetRoot(rn);

    rsAddChildNode(rn, wxT("nextId"), wxString::Format(wxT("%d"), nextIdM));

    for (std::list<Server>::iterator its = serversM.begin();
        its != serversM.end(); ++its)
    {
        wxXmlNode* srvn = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("server"));
        rn->AddChild(srvn);
        
        rsAddChildNode(srvn, wxT("name"), its->getName_());
        rsAddChildNode(srvn, wxT("host"), its->getHostname());
        rsAddChildNode(srvn, wxT("port"), its->getPort());

        for (std::list<Database>::iterator itdb = its->getDatabases()->begin();
            itdb != its->getDatabases()->end(); ++itdb)
        {
            itdb->resetCredentials();    // clean up eventual extra credentials

            wxXmlNode* dbn = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("database"));
            srvn->AddChild(dbn);

            rsAddChildNode(dbn, wxT("id"), itdb->getId());
            rsAddChildNode(dbn, wxT("name"), itdb->getName_());
            rsAddChildNode(dbn, wxT("path"), itdb->getPath());
            rsAddChildNode(dbn, wxT("charset"), itdb->getConnectionCharset());
            rsAddChildNode(dbn, wxT("username"), itdb->getUsername());
            rsAddChildNode(dbn, wxT("password"), itdb->getRawPassword());
            rsAddChildNode(dbn, wxT("encrypted"),
                (itdb->getStoreEncryptedPassword() ? wxT("1") : wxT("0")));
            rsAddChildNode(dbn, wxT("role"), itdb->getRole());
        }
    }
    if (!doc.Save(getFileName()))
        return false;
#else
    ofstream file(wx2std(getFileName()).c_str());
    if (!file)
        return false;
    file << "<?xml version='1.0' encoding='ISO-8859-1'?>\n";
    file << "<root>\n";
    file << "\t<nextId>" << nextIdM << "</nextId>\n";
    for (std::list<Server>::iterator it = serversM.begin(); it != serversM.end(); ++it)
    {
        file << "\t<server>\n";
        file << "\t\t<name>" << wx2std(it->getName_()) << "</name>\n";
        file << "\t\t<host>" << wx2std(it->getHostname()) << "</host>\n";
        file << "\t\t<port>" << wx2std(it->getPort()) << "</port>\n";

        for (std::list<Database>::iterator it2 = it->getDatabases()->begin(); it2 != it->getDatabases()->end(); ++it2)
        {
            it2->resetCredentials();    // clean up eventual extra credentials
            file << "\t\t<database>\n";
            file << "\t\t\t<id>" << wx2std(it2->getId()) << "</id>\n";
            file << "\t\t\t<name>" << wx2std(it2->getName_()) << "</name>\n";
            file << "\t\t\t<path>" << wx2std(it2->getPath()) << "</path>\n";
            file << "\t\t\t<charset>" << wx2std(it2->getConnectionCharset()) << "</charset>\n";
            file << "\t\t\t<username>" << wx2std(it2->getUsername()) << "</username>\n";
            file << "\t\t\t<password>" << wx2std(it2->getRawPassword()) << "</password>\n";
            file << "\t\t\t<encrypted>" << (it2->getStoreEncryptedPassword() ? "1" : "0") << "</encrypted>\n";
            file << "\t\t\t<role>" << wx2std(it2->getRole()) << "</role>\n";
            file << "\t\t</database>\n";
        }
        file << "\t</server>\n";
    }
    file << "</root>\n";

    file.close();
#endif
    dirtyM = false;
    return true;
}
//-----------------------------------------------------------------------------
void Root::notifyAllServers()
{
    MetadataCollection<Server>::iterator it;
    for (it = serversM.begin(); it != serversM.end(); ++it)
        (*it).notifyObservers();
}
//-----------------------------------------------------------------------------
bool Root::getChildren(vector<MetadataItem *>& temp)
{
    return serversM.getChildren(temp);
}
//-----------------------------------------------------------------------------
bool Root::orderedChildren() const
{
    bool ordered = false;
    config().getValue(wxT("OrderServersInTree"), ordered);
    return ordered;
}
//-----------------------------------------------------------------------------
void Root::lockChildren()
{
    serversM.lockSubject();
}
//-----------------------------------------------------------------------------
void Root::unlockChildren()
{
    serversM.unlockSubject();

}
//-----------------------------------------------------------------------------
const wxString Root::getItemPath() const
{
    // Root is root, don't make the path strings any longer than needed.
    return wxT("");
}
//-----------------------------------------------------------------------------
wxString Root::getFileName()
{
    if (fileNameM.empty())
        fileNameM = config().getDBHFileName();
    return fileNameM;
}
//-----------------------------------------------------------------------------
const unsigned int Root::getNextId()
{
    return nextIdM++;
}
//-----------------------------------------------------------------------------
void Root::acceptVisitor(MetadataItemVisitor* visitor)
{
    visitor->visitRoot(*this);
}
//-----------------------------------------------------------------------------
