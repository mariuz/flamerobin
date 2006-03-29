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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <fstream>
#include <iomanip>
#include <sstream>

#include <wx/clipbrd.h>
#include <wx/file.h>
#include <wx/filedlg.h>

#include "config/Config.h"
#include "core/FRError.h"
#include "dberror.h"
#include "framemanager.h"
#include "frutils.h"
#include "gui/MetadataItemPropertiesFrame.h"
#include "gui/ProgressDialog.h"
#include "images.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/database.h"
#include "metadata/exception.h"
#include "metadata/metadataitem.h"
#include "metadata/table.h"
#include "metadata/view.h"
#include "ugly.h"
#include "urihandler.h"
//-----------------------------------------------------------------------------
//! converts chars that have special meaning in HTML, so they get displayed
wxString escapeHtmlChars(wxString s, bool processNewlines = true)
{
    typedef std::pair<char, wxString> par;
    std::vector<par> symbol_table;
    symbol_table.push_back(par('&', wxT("&amp;")));      // this has to go first, since others use &
    symbol_table.push_back(par('<', wxT("&lt;")));
    symbol_table.push_back(par('>', wxT("&gt;")));
    symbol_table.push_back(par('"', wxT("&quot;")));
    if (processNewlines)                            // BR has to be at end, since it adds < and >
        symbol_table.push_back(par('\n', wxT("<BR>")));

    for (std::vector<par>::iterator it = symbol_table.begin(); it != symbol_table.end(); ++it)
    {
        wxString::size_type pos = 0;
        while (pos < s.length())
        {
            pos = s.find((*it).first, pos);
            if (pos == wxString::npos)
                break;
            s.replace(pos, 1, (*it).second);
            pos++;
        }
    }
    return s;
}
//-----------------------------------------------------------------------------
//! MetadataItemPropertiesFrame class
MetadataItemPropertiesFrame::MetadataItemPropertiesFrame(wxWindow* parent, MetadataItem *object, int id):
    BaseFrame(parent, id, wxT(""))
{
    pageTypeM = ptSummary;
    storageNameM = wxT("unassigned");

    if (!object)
    {
        ::wxMessageBox(wxT("MIPF::ctor, Object == 0"), _("Error"), wxOK);
        return;
    }

    objectM = object;
    objectM->attachObserver(this);

    window_1 = new PrintableHtmlWindow(this);
    CreateStatusBar();
    wxString title = objectM->getName_().c_str();
    window_1->SetRelatedFrame(this, title + wxT(": %s"));
    window_1->SetRelatedStatusBar(0);
    SetTitle(wxString::Format(_("%s: properties"), objectM->getName_().c_str()));

    update();   // initial rendering

    wxBitmap bmp = getImage32(objectM->getType());
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);
}
//-----------------------------------------------------------------------------
const wxRect MetadataItemPropertiesFrame::getDefaultRect() const
{
    return wxRect(-1, -1, 600, 420);
}
//-----------------------------------------------------------------------------
const wxString MetadataItemPropertiesFrame::getName() const
{
    return wxT("MIPFrame");
}
//-----------------------------------------------------------------------------
MetadataItem *MetadataItemPropertiesFrame::getObservedObject() const
{
    return objectM;
}
//-----------------------------------------------------------------------------
const wxString MetadataItemPropertiesFrame::getStorageName() const
{
    if (storageNameM == wxT("unassigned"))
    {
        StorageGranularity g;
        if (!config().getValue(wxT("MetadataFrameStorageGranularity"), g))
            g = sgFrame;

        switch (g)
        {
            case sgFrame:
                storageNameM = getName();
                break;
            case sgObjectType:
                storageNameM = getName() + Config::pathSeparator + objectM->getTypeName();
                break;
            case sgObject:
                storageNameM = getName() + Config::pathSeparator + objectM->getItemPath();
                break;
            default:
                storageNameM = wxT("");
                break;
        }
    }
    return storageNameM;
}
//-----------------------------------------------------------------------------
//! determine the path, load and display html page
void MetadataItemPropertiesFrame::loadPage()
{
    wxString htmlpage = config().getHtmlTemplatesPath();
    switch (pageTypeM)
    {
        case ptSummary:
            htmlpage += objectM->getTypeName() + wxT(".html");
            break;
        case ptConstraints:
            htmlpage += objectM->getTypeName() + wxT("constraints.html");
            break;
        case ptTriggers:
            htmlpage += objectM->getTypeName() + wxT("triggers.html");
            break;
        case ptTableIndices:
            htmlpage += wxT("TABLEindices.html");
            break;
        case ptDependencies:
            htmlpage += wxT("dependencies.html");
            break;
        case ptDDL:
            htmlpage += wxT("DDL.html");
            break;
    }
    processHtmlFile(htmlpage);  // load HTML template, parse, and fill the HTML control
}
//-----------------------------------------------------------------------------
//! processes commands found in HTML template
//
//! command is in format:   {%action:data%}
//! data field can be empty
void MetadataItemPropertiesFrame::processCommand(wxString cmd, MetadataItem *object, wxString& htmlpage)
{
    wxString::size_type pos = cmd.find(':');
    wxString suffix;
    if (pos != wxString::npos)
    {
        suffix = cmd.substr(pos+1);
        cmd = cmd.substr(0, pos);
    }

    // TODO: change this to getURLencodedName()
    if (cmd == wxT("object_name"))
        htmlpage += object->getName_();

    else if (cmd == wxT("object_type"))
        htmlpage += object->getTypeName();

    else if (cmd == wxT("object_address"))
        htmlpage += wxString::Format(wxT("%ld"), (uintptr_t)object);

    else if (cmd == wxT("parent_window"))
        htmlpage += wxString::Format(wxT("%ld"), (uintptr_t)this);

    else if (cmd == wxT("fr_home"))
        htmlpage += config().getHomePath();

    else if (cmd == wxT("object_description"))
    {
        wxString s = object->getDescription();
        if (s == wxT(""))
            s = wxT("No description");
        htmlpage += escapeHtmlChars(s);
    }

    else if (cmd == wxT("show_if_config"))
    {
        wxString::size_type poscolon = suffix.find(':');
        if (poscolon != wxString::npos)
        {
            wxString configKey = suffix.substr(0, poscolon);
            suffix = suffix.substr(poscolon+1);
            wxString::size_type pos2 = suffix.find(':');
            if (pos2 != wxString::npos)
            {
                wxString flag = suffix.substr(0, pos2);
                if (config().get(configKey, (flag == wxT("true"))))
                    processHtmlCode(htmlpage, suffix.substr(pos2+1), object);
            }
        }
    }

    else if (cmd == wxT("columns"))  // table and view columns
    {
        Relation* r = dynamic_cast<Relation*>(object);
        if (!r)
            return;
        std::vector<MetadataItem*> tmp;
        if (r->checkAndLoadColumns() && r->getChildren(tmp))
        {
            for (std::vector<MetadataItem*>::iterator it = tmp.begin(); it != tmp.end(); ++it)
                processHtmlCode(htmlpage, suffix, *it);
        }
    }

    else if (cmd == wxT("triggers")) // table triggers,  triggers:after or triggers:befor  <- not a typo
    {
        Relation* r = dynamic_cast<Relation*>(object);
        if (!r)
            return;
        std::vector<Trigger*> tmp;
        bool result;
        if (suffix.substr(0, 5) == wxT("after"))
            result = r->getTriggers(tmp, Trigger::afterTrigger);
        else
            result = r->getTriggers(tmp, Trigger::beforeTrigger);
        suffix.erase(0, 5);
        if (result)
        {
            for (std::vector<Trigger*>::iterator it = tmp.begin(); it != tmp.end(); ++it)
                processHtmlCode(htmlpage, suffix, *it);
        }
        else
            ::wxMessageBox(lastError().getMessage(), _("Error"), wxOK);
    }

    else if (cmd == wxT("depends_on") || cmd == wxT("depend_of"))
    {
        MetadataItem* m = dynamic_cast<MetadataItem*>(object);
        if (!m)
            return;
        std::vector<Dependency> tmp;
        if (m->getDependencies(tmp, cmd == wxT("depends_on")))
        {
            for (std::vector<Dependency>::iterator it = tmp.begin(); it != tmp.end(); ++it)
                processHtmlCode(htmlpage, suffix, &(*it));
        }
        else
            ::wxMessageBox(lastError().getMessage(), _("Error"), wxOK);
    }

    else if (cmd == wxT("dependency_columns"))
    {
        Dependency* d = dynamic_cast<Dependency*>(object);
        if (!d)
            return;
        htmlpage += d->getFields();
    }

    else if (cmd == wxT("primary_key"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        ColumnConstraint* pk = t->getPrimaryKey();
        if (!pk)
            return;
        processHtmlCode(htmlpage, suffix, pk);
    }

    else if (cmd == wxT("foreign_keys"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        std::vector<ForeignKey>* fk = t->getForeignKeys();
        if (!fk)
            return;
        for (std::vector<ForeignKey>::iterator it = fk->begin(); it != fk->end(); ++it)
            processHtmlCode(htmlpage, suffix, &(*it));
    }

    else if (cmd == wxT("check_constraints"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        std::vector<CheckConstraint>* c = t->getCheckConstraints();
        if (!c)
            return;
        for (std::vector<CheckConstraint>::iterator it = c->begin(); it != c->end(); ++it)
            processHtmlCode(htmlpage, suffix, &(*it));
    }

    else if (cmd == wxT("unique_constraints"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        std::vector<ColumnConstraint>* c = t->getUniqueConstraints();
        if (!c)
            return;
        for (std::vector<ColumnConstraint>::iterator it = c->begin(); it != c->end(); ++it)
            processHtmlCode(htmlpage, suffix, &(*it));
    }

    else if (cmd == wxT("check_source"))
    {
        CheckConstraint* c = dynamic_cast<CheckConstraint*>(object);
        if (!c)
            return;
        htmlpage += escapeHtmlChars(c->sourceM);
    }

    else if (cmd == wxT("constraint_columns"))
    {
        ColumnConstraint* c = dynamic_cast<ColumnConstraint*>(object);
        if (!c)
            return;
        htmlpage += c->getColumnList();
    }

    else if (cmd == wxT("fk_referenced_columns") || cmd == wxT("fk_table"))
    {
        ForeignKey* fk = dynamic_cast<ForeignKey*>(object);
        if (!fk)
            return;
        if (cmd == wxT("fk_table"))
            htmlpage += fk->referencedTableM;
        else
            htmlpage += fk->getReferencedColumnList();
    }

    else if (cmd == wxT("fk_update") || cmd == wxT("fk_delete"))  // table and view columns
    {
        ForeignKey* fk = dynamic_cast<ForeignKey*>(object);
        if (!fk)
            return;
        if (cmd == wxT("fk_update"))
            htmlpage += fk->updateActionM;
        else
            htmlpage += fk->deleteActionM;
    }

    else if (cmd == wxT("column_datatype"))
    {
        Column* c = dynamic_cast<Column*>(object);
        if (c)
        {
            htmlpage += c->getDatatype();
            // TODO: make the domain name (if any) a link to the domain's property page?
        }
    }

    else if (cmd == wxT("column_nulloption"))
    {
        Column* c = dynamic_cast<Column*>(object);
        if (c)
            htmlpage += (c->isNullable() ? wxT("") : wxT("<b>not null</b>"));
    }

    else if (cmd == wxT("column_default"))
    {
        Column* c = dynamic_cast<Column*>(object);
        if (c)
        {
            wxString def(c->getDefault());
            def.Trim(false);    // left trim
            if (def.Upper().StartsWith(wxT("DEFAULT")))
            {
                def.Remove(0, 7);
                def.Trim(false);
            }
            htmlpage += def;
        }
    }

    else if (cmd == wxT("input_parameters") || cmd == wxT("output_parameters"))     // SP params
    {
        Procedure* p = dynamic_cast<Procedure*>(object);
        if (!p)
            return;

        SubjectLocker locker(p);
        std::vector<MetadataItem*> tmp;
        if (p->checkAndLoadParameters() && p->getChildren(tmp))
        {
            ParameterType pt = (cmd == wxT("input_parameters")) ? ptInput : ptOutput;
            std::vector<MetadataItem*>::iterator it;
            for (it = tmp.begin(); it != tmp.end(); ++it)
            {
                if ((dynamic_cast<Parameter*>(*it))->getParameterType() == pt)
                    processHtmlCode(htmlpage, suffix, *it);
            }
        }
    }

    else if (cmd == wxT("view_source"))
    {
        View* v = dynamic_cast<View*>(object);
        wxString src;
        if (!v || !v->getSource(src))
            return;
        htmlpage += escapeHtmlChars(src, false);
    }

    else if (cmd == wxT("procedure_source"))
    {
        Procedure* p = dynamic_cast<Procedure*>(object);
        wxString src;
        if (!p || !p->getSource(src))
            return;
        htmlpage += escapeHtmlChars(src, false);
    }

    else if (cmd == wxT("trigger_source"))
    {
        Trigger* t = dynamic_cast<Trigger*>(object);
        wxString src;
        if (!t || !t->getSource(src))
            return;
        htmlpage += escapeHtmlChars(src, false);
    }

    else if (cmd == wxT("trigger_info"))
    {
        Trigger* t = dynamic_cast<Trigger*>(object);
        wxString object, type;
        bool active;
        int position;
        if (!t || !t->getTriggerInfo(object, active, position, type))
            return;
        wxString s(active ? wxT("Active ") : wxT("Inactive "));
        s << type << wxT(" trigger for ") << object << wxT(" at position ") << position;
        htmlpage += escapeHtmlChars(s, false);
    }

    else if (cmd == wxT("generator_value"))
    {
        Generator* g = dynamic_cast<Generator*>(object);
        if (!g)
            return;
        std::ostringstream ss;
        ss << g->getValue();
        htmlpage += escapeHtmlChars(std2wx(ss.str()), false);
    }

    else if (cmd == wxT("exception_number"))
    {
        Exception* e = dynamic_cast<Exception*>(object);
        if (!e)
            return;
        wxString s;
        s << e->getNumber();
        htmlpage += escapeHtmlChars(s, false);
    }

    else if (cmd == wxT("exception_message"))
    {
        Exception* e = dynamic_cast<Exception*>(object);
        if (!e)
            return;
        htmlpage += escapeHtmlChars(e->getMessage(), false);
    }

    else if (cmd == wxT("udf_info"))
    {
        Function* f = dynamic_cast<Function*>(object);
        if (!f)
            return;
        wxString src = f->getDefinition();
        htmlpage += f->getHtmlHeader() + escapeHtmlChars(src, false);
    }

    else if (cmd == wxT("varcolor"))
    {
        static bool first = false;
        first = !first;
        pos = suffix.find('/');
        if (first)
            htmlpage += suffix.substr(0, pos);
        else
            htmlpage += suffix.substr(pos + 1);
    }

    else if (cmd == wxT("indices"))
    {
        Table* t = dynamic_cast<Table*>(object);
        if (!t)
            return;
        std::vector<Index>* ix = t->getIndices();
        if (!ix)
            return;
        for (std::vector<Index>::iterator it = ix->begin(); it != ix->end(); ++it)
            processHtmlCode(htmlpage, suffix, &(*it));
    }

    else if (cmd == wxT("object_ddl"))
    {
        ProgressDialog pd(this, _("Extracting DDL Definitions"), 2);

        CreateDDLVisitor cdv(&pd);
        object->acceptVisitor(&cdv);
        htmlpage += escapeHtmlChars(cdv.getSql(), false);
    }

    else if (cmd.substr(0, 5) == wxT("index"))
    {
        wxString okimage = wxT("<img src=\"") + config().getHtmlTemplatesPath() + wxT("ok.png\">");
        wxString ximage = wxT("<img src=\"") + config().getHtmlTemplatesPath() + wxT("redx.png\">");
        Index* i = dynamic_cast<Index*>(object);
        if (!i)
            return;
        if (cmd == wxT("index_type"))
            htmlpage += (i->getIndexType() == Index::itAscending ? wxT("ASC") : wxT("DESC"));
        if (cmd == wxT("index_active"))
            htmlpage += (i->isActive() ? okimage : ximage);
        if (cmd == wxT("index_unique") && i->isUnique())
            htmlpage += okimage;
        else if (cmd == wxT("index_stats"))
        {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << i->getStatistics();
            htmlpage += std2wx(ss.str());
        }
        else if (cmd == wxT("index_fields"))
            htmlpage += i->getFieldsAsString();
    }
    else if (cmd == wxT("creation_date"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += d->getInfo()->getCreated();
    }
    else if (cmd == wxT("ods_version"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo()->getODS();
    }
    else if (cmd == wxT("dialect"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo()->getDialect();
    }
    else if (cmd == wxT("sweep_interval"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo()->getSweep();
    }
    else if (cmd == wxT("page_size"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo()->getPageSize();
    }
    else if (cmd == wxT("page_buffers"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo()->getBuffers();
    }
    else if (cmd == wxT("oldest_transaction"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo()->getOldestTransaction();
    }
    else if (cmd == wxT("next_transaction"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo()->getNextTransaction();
    }
    else if (cmd == wxT("read_only"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        if (d->getInfo()->getReadOnly())
            htmlpage += wxT("true");
    else
            htmlpage += wxT("false");
    }
    else if (cmd == wxT("forced_writes"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        wxString okimage = wxT("<img src=\"") + config().getHtmlTemplatesPath() + wxT("ok.png\">");
        wxString ximage = wxT("<img src=\"") + config().getHtmlTemplatesPath() + wxT("redx.png\">");

    htmlpage += (d->getInfo()->getForcedWrites() ? okimage : ximage);
    }
}
//-----------------------------------------------------------------------------
//! processes html template code given in the htmlsource wxString
void MetadataItemPropertiesFrame::processHtmlCode(wxString& htmlpage, wxString htmlsource, MetadataItem *object)
{
    if (object == 0)
        object = objectM;

    using namespace std;
    wxString::size_type pos = 0, oldpos = 0, endpos = 0;
    while (true)
    {
        pos = htmlsource.find(wxT("{%"), pos);
        if (pos == wxString::npos)
        {
            htmlpage += htmlsource.substr(oldpos);
            break;
        }

        wxString::size_type check, startpos = pos;
        int cnt = 1;
        while (cnt > 0)
        {
            endpos = htmlsource.find(wxT("%}"), startpos+1);
            if (endpos == wxString::npos)
                break;

            check = htmlsource.find(wxT("{%"), startpos+1);
            if (check == wxString::npos)
                startpos = endpos;
            else
            {
                startpos = (check < endpos ? check : endpos);
                if (startpos == check)
                    cnt++;
            }
            if (startpos == endpos)
                cnt--;
            startpos++;
        }

        if (cnt > 0)    // no matching closing %}
            break;

        htmlpage += htmlsource.substr(oldpos, pos - oldpos);
        wxString cmd = htmlsource.substr(pos + 2, endpos - pos - 2); // 2 = start_marker_len = end_marker_len
        processCommand(cmd, object, htmlpage);
        oldpos = pos = endpos + 2;
    }
}
//-----------------------------------------------------------------------------
//! processes the given html template file
void MetadataItemPropertiesFrame::processHtmlFile(wxString fileName)
{
    using namespace std;
    wxString htmlpage;        // create html page into variable

    wxFileName localFileName = fileName;
    if (!localFileName.FileExists())
    {
        wxString msg;
        msg.Printf(_("The file \"%s\" does not exist."),
            localFileName.GetFullPath().c_str());
        throw FRError(msg);
    }

    ifstream file(wx2std(fileName).c_str()); // read entire file into wxString buffer
    if (!file)
    {
        wxString msg;
        msg.Printf(_("The file \"%s\" cannot be opened."),
            fileName.c_str());
        throw FRError(msg);
    }

    stringstream ss;
    ss << file.rdbuf();
    wxString s(std2wx(ss.str()));
    file.close();

    processHtmlCode(htmlpage, s);

    int x = 0, y = 0;
    window_1->GetViewStart(&x, &y);         // save scroll position
    window_1->setPageSource(htmlpage);
    window_1->Scroll(x, y);                 // restore scroll position
}
//-----------------------------------------------------------------------------
//! closes window if observed object gets removed (disconnecting, dropping, etc)
void MetadataItemPropertiesFrame::removeSubject(Subject* subject)
{
    Observer::removeSubject(subject);
    // main observed object is getting destroyed
    if (subject == objectM)
    {
        objectM = 0;
        Close();
    }
}
//-----------------------------------------------------------------------------
void MetadataItemPropertiesFrame::setPage(const wxString& type)
{
    if (type == wxT("constraints"))
        pageTypeM = ptConstraints;
    else if (type == wxT("dependencies"))
        pageTypeM = ptDependencies;
    else if (type == wxT("triggers"))
        pageTypeM = ptTriggers;
    else if (type == wxT("indices"))
        pageTypeM = ptTableIndices;
    else if (type == wxT("ddl"))
        pageTypeM = ptDDL;
    // add more page types here when needed
    else
        pageTypeM = ptSummary;
    loadPage();
}
//-----------------------------------------------------------------------------
//! recreate html page if something changes
void MetadataItemPropertiesFrame::update()
{
    Database *db = dynamic_cast<Database *>(objectM);
    if (db && !db->isConnected())
    {
        objectM = 0;
        Close();
        return;
    }

    // if table or view columns change, we need to reattach
    if (objectM->getType() == ntTable || objectM->getType() == ntView)  // also observe columns
    {
        Relation* t = dynamic_cast<Relation*>(objectM);
        if (!t)
            return;

        SubjectLocker locker(t);
        t->checkAndLoadColumns();       // load column data if needed
        std::vector<MetadataItem*> temp;
        objectM->getChildren(temp);
        for (std::vector<MetadataItem*>::iterator it = temp.begin(); it != temp.end(); ++it)
            (*it)->attachObserver(this);
    }

    // if description of procedure params change, we need to reattach
    if (objectM->getType() == ntProcedure)
    {
        Procedure* p = dynamic_cast<Procedure*>(objectM);
        if (!p)
            return;

        SubjectLocker locker(p);
        p->checkAndLoadParameters();        // load column data if needed
        std::vector<MetadataItem*> temp;
        objectM->getChildren(temp);
        for (std::vector<MetadataItem *>::iterator it = temp.begin(); it != temp.end(); ++it)
            (*it)->attachObserver(this);
    }

    loadPage();
}
//-----------------------------------------------------------------------------
//! PageHandler class
class PageHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    static const PageHandler handlerInstance;   // singleton; registers itself on creation.
};
const PageHandler PageHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool PageHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("page"))
        return false;

    wxString ms = uri.getParam(wxT("parent_window"));     // window
    unsigned long mo;
    if (!ms.ToULong(&mo))
        return true;
    MetadataItemPropertiesFrame* m = (MetadataItemPropertiesFrame*)mo;
    if (uri.getParam(wxT("target")) == wxT("new"))
    {
        wxWindow* mainFrame = m->GetParent();
        if (mainFrame)                                              // !delayed, force_new
            m = frameManager().showMetadataPropertyFrame(mainFrame, m->getObservedObject(), false, true);
    }

    if (m)
    {
        m->setPage(uri.getParam(wxT("type")));
        frameManager().rebuildMenu();
    }
    return true;
}
//-----------------------------------------------------------------------------
//! PropertiesHandler class
class PropertiesHandler: public URIHandler
{
public:
    bool handleURI(URI& uri);
private:
    static const PropertiesHandler handlerInstance; // singleton; registers itself on creation.
};
const PropertiesHandler PropertiesHandler::handlerInstance;
//-----------------------------------------------------------------------------
bool PropertiesHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("properties"))
        return false;

    MetadataItemPropertiesFrame* parent = dynamic_cast<MetadataItemPropertiesFrame*>(getWindow(uri));
    if (!parent)
        return true;
    Database* d = parent->getObservedObject()->getDatabase();
    if (!d)
        return true;
    NodeType n = getTypeByName(uri.getParam(wxT("object_type")));
    MetadataItem* object = d->findByNameAndType(n, uri.getParam(wxT("object_name")));
    if (!object)
    {
        ::wxMessageBox(_("Cannot find destination object\nThis should never happen."), _("Error"), wxICON_ERROR);
        return true;
    }

    // check if window with properties of that object is already open and show it
    wxWindow* mainFrame = parent->GetParent();
    if (mainFrame)
        frameManager().showMetadataPropertyFrame(mainFrame, object, false,
            uri.getParam(wxT("target")) == wxT("new"));
    return true;
}
//-----------------------------------------------------------------------------
