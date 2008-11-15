/*
  Copyright (c) 2004-2008 The FlameRobin Development Team

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

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/clipbrd.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/tipwin.h>

#include <fstream>
#include <iomanip>
#include <sstream>

#include "config/Config.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "engine/MetadataLoader.h"
#include "framemanager.h"
#include "frutils.h"
#include "gui/MetadataItemPropertiesFrame.h"
#include "gui/ProgressDialog.h"
#include "images.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/database.h"
#include "metadata/exception.h"
#include "metadata/server.h"
#include "metadata/table.h"
#include "metadata/view.h"
#include "urihandler.h"
//-----------------------------------------------------------------------------
//! MetadataItemPropertiesFrame class
MetadataItemPropertiesFrame::MetadataItemPropertiesFrame(wxWindow* parent,
        MetadataItem *object)
    : BaseFrame(parent, wxID_ANY, wxEmptyString)
{
    // we need to store this right now, since we might lose the object later
    setStorageName(object);

    wxStatusBar *sb = CreateStatusBar();

    Database* d = object->findDatabase();
    if (d)  // server property page doesn't have a database, so don't crash
        sb->SetStatusText(d->getConnectionInfoString());
    else
        sb->SetStatusText(object->getPrintableName());

    wxIcon icon;
    if (d && config().get(wxT("linksOpenInTabs"), true))
    {
        wxBitmap bmp = getImage32(d->getType());
        icon.CopyFromBitmap(bmp);
        databaseNameM = d->getPrintableName();
    }
    else  // when linksOpenInTabs, only the server node
    {
        wxBitmap bmp = getImage32(object->getType());
        icon.CopyFromBitmap(bmp);
    }
    SetIcon(icon);

    notebookM = new wxAuiNotebook(this, ID_notebook, wxDefaultPosition,
        wxDefaultSize, wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_WINDOWLIST_BUTTON
        | wxAUI_NB_TAB_EXTERNAL_MOVE | wxNO_BORDER );

    auiManagerM.SetManagedWindow(this);
    auiManagerM.AddPane(notebookM,
        wxAuiPaneInfo().CenterPane().PaneBorder(false));
    auiManagerM.Update();

    Connect(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, wxAuiNotebookEventHandler(
        MetadataItemPropertiesFrame::OnNotebookPageClose), NULL, this);
    Connect(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, wxAuiNotebookEventHandler(
        MetadataItemPropertiesFrame::OnNotebookPageChanged), NULL, this);
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
const wxString MetadataItemPropertiesFrame::getStorageName() const
{
    return storageNameM;
}
//-----------------------------------------------------------------------------
void MetadataItemPropertiesFrame::removePanel(wxWindow* panel)
{
    int pg = notebookM->GetPageIndex(panel);
    if (pg == wxNOT_FOUND)
        return;

    notebookM->DeletePage(pg);
    if (notebookM->GetPageCount() < 1)
        Close();
}
//-----------------------------------------------------------------------------
void MetadataItemPropertiesFrame::setStorageName(MetadataItem *object)
{
    StorageGranularity g;
    if (!config().getValue(wxT("MetadataFrameStorageGranularity"), g))
        g = sgFrame;
    if (config().get(wxT("linksOpenInTabs"), true))
        g = sgFrame;

    switch (g)
    {
        case sgFrame:
            storageNameM = getName();
            break;
        case sgObjectType:
            storageNameM = getName() + Config::pathSeparator
                + object->getTypeName();
            break;
        case sgObject:
            storageNameM = getName() + Config::pathSeparator
                + object->getItemPath();
            break;
        default:
            storageNameM = wxT("");
            break;
    }
}
//-----------------------------------------------------------------------------
void MetadataItemPropertiesFrame::setTabTitle(wxWindow *panel,
    const wxString& title)
{
    int pg = notebookM->GetPageIndex(panel);
    if (pg == wxNOT_FOUND)
        return;
    notebookM->SetPageText(pg, title);
}
//-----------------------------------------------------------------------------
void MetadataItemPropertiesFrame::showPanel(wxWindow* panel,
    const wxString& title)
{
    int pg = notebookM->GetPageIndex(panel);
    if (pg == wxNOT_FOUND)
        notebookM->AddPage(panel, title, true);
    else
        notebookM->SetSelection(pg);

    Show();
    Raise();
}
//-----------------------------------------------------------------------------
MetadataItemPropertiesPanel::MetadataItemPropertiesPanel(
    MetadataItemPropertiesFrame* parent, MetadataItem *object)
    :wxPanel(parent, wxID_ANY)
{
    pageTypeM = ptSummary;
    objectM = object;
    htmlReloadRequestedM = false;

    if (!object)
    {
        ::wxMessageBox(wxT("MIPF::ctor, Object == 0"), _("Error"), wxOK);
        return;
    }

    html_window = new PrintableHtmlWindow(this, HtmlWindowID);
    parent->SetTitle(object->getName_());

    wxBoxSizer* bSizer2 = new wxBoxSizer( wxVERTICAL );
    bSizer2->Add( html_window, 1, wxEXPAND, 0 );
    SetSizer( bSizer2 );
    Layout();

    Database* d = objectM->findDatabase();
    // start a transaction for metadata loading and lock the object
    MetadataLoaderTransaction tr((d) ? d->getMetadataLoader() : 0);
    SubjectLocker lock(objectM);

    // request initial rendering
    requestLoadPage(true);
    objectM->attachObserver(this);
}
//-----------------------------------------------------------------------------
MetadataItemPropertiesPanel::~MetadataItemPropertiesPanel()
{
    frameManager().removeFrame(this);
}
//-----------------------------------------------------------------------------
void MetadataItemPropertiesPanel::showIt()
{
    MetadataItemPropertiesFrame *f = getParentFrame();
    if (f)
        f->showPanel(this, objectM->getName_());
}
//-----------------------------------------------------------------------------
MetadataItem *MetadataItemPropertiesPanel::getObservedObject() const
{
    return objectM;
}
//-----------------------------------------------------------------------------
//! defer (possibly expensive) creation and display of html page to idle time
void MetadataItemPropertiesPanel::requestLoadPage(bool showLoadingPage)
{
    if (!htmlReloadRequestedM)
    {
        if (showLoadingPage)
        {
            wxBusyCursor bc;
            wxString path(config().getHtmlTemplatesPath());
            processHtmlFile(path + wxT("ALLloading.html"));
        }

        Connect(wxID_ANY, wxEVT_IDLE,
            wxIdleEventHandler(MetadataItemPropertiesPanel::OnIdle));
        htmlReloadRequestedM = true;
    }
}
//-----------------------------------------------------------------------------
//! determine the path, load and display html page
void MetadataItemPropertiesPanel::loadPage()
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
        case ptPrivileges:
            htmlpage += objectM->getTypeName() + wxT("privileges.html");
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

    wxBusyCursor bc;

    // start a transaction for metadata loading and lock the object
    Database* d = objectM->findDatabase();
    MetadataLoaderTransaction tr((d) ? d->getMetadataLoader() : 0);
    SubjectLocker lock(objectM);

    processHtmlFile(htmlpage);  // load HTML template, parse, and fill the HTML control
}
//-----------------------------------------------------------------------------
//! processes commands found in HTML template
//
//! command is in format:   {%action:data%}
//! data field can be empty
void MetadataItemPropertiesPanel::processCommand(wxString cmd, MetadataItem *object, wxString& htmlpage)
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

    else if (cmd == wxT("NotSystem"))
    {
        if (!object->isSystem())
            processHtmlCode(htmlpage, suffix, object);
    }

    else if (cmd == wxT("ParentNotSystem"))
    {
        if (object->getParent() && !object->getParent()->isSystem())
            processHtmlCode(htmlpage, suffix, object);
    }

    else if (cmd == wxT("header"))  // include another file
    {
        std::vector<wxString> pages;            // pages this object has
        pages.push_back(wxT("Summary"));
        if (object->getType() == ntRole)        // special case, roles
            pages.push_back(wxT("Privileges")); // don't have dependencies
        if (object->getType() == ntDatabase)
            pages.push_back(wxT("Triggers"));   // FB 2.1
        switch (object->getType())
        {
            case ntSysTable:
            case ntTable:
                pages.push_back(wxT("Constraints"));
                pages.push_back(wxT("Indices"));
            case ntView:
                pages.push_back(wxT("Triggers"));
            case ntProcedure:
                pages.push_back(wxT("Privileges"));
            case ntTrigger:
            case ntException:
            case ntFunction:
            case ntGenerator:
                pages.push_back(wxT("Dependencies"));
            case ntDatabase:
            case ntRole:
                pages.push_back(wxT("DDL"));
        };
        wxString page = loadEntireFile(config().getHtmlTemplatesPath()
            + wxT("header.html"));
        bool first = true;
        while (!page.Strip().IsEmpty())
        {
            wxString::size_type p2 = page.find('|');
            wxString part(page);
            if (p2 != wxString::npos)
            {
                part = page.substr(0, p2);
                page.Remove(0, p2+1);
            }
            else
                page.Clear();
            for (std::vector<wxString>::iterator it = pages.begin(); it !=
                pages.end(); ++it)
            {
                if (part.Find(wxT(">")+(*it)+wxT("<")) == -1)
                    continue;
                if (first)
                    first = false;
                else
                    htmlpage += wxT(" | ");
                if (part.Find(wxT(">")+suffix+wxT("<")) != -1)
                    htmlpage += suffix;
                else
                    processHtmlCode(htmlpage, part, object);
            }
        }
    }

    else if (cmd == wxT("users"))
    {
        Server* s = dynamic_cast<Server*>(object);
        if (!s)
            return;

        ProgressDialog pd(GetParent(), _("Connecting to Server..."), 1);
        UserList* usr = s->getUsers(&pd);
        if (!usr || !usr->size())
        {
            Close();
            return;
        }
        for (UserList::iterator it = usr->begin(); it != usr->end(); ++it)
            processHtmlCode(htmlpage, suffix, &(*it));
    }

    else if (cmd == wxT("userinfo"))
    {
        User* u = dynamic_cast<User*>(object);
        if (!u)
            return;
        if (suffix == wxT("username"))
            htmlpage += escapeHtmlChars(u->usernameM);
        if (suffix == wxT("first_name"))
            htmlpage += escapeHtmlChars(u->firstnameM);
        if (suffix == wxT("middle_name"))
            htmlpage += escapeHtmlChars(u->middlenameM);
        if (suffix == wxT("last_name"))
            htmlpage += escapeHtmlChars(u->lastnameM);
        if (suffix == wxT("unix_user"))
            htmlpage << u->useridM;
        if (suffix == wxT("unix_group"))
            htmlpage << u->groupidM;
    }

    else if (cmd == wxT("owner_name"))
    {
        wxString name;
        if (Relation* r = dynamic_cast<Relation*>(object))
            name = r->getOwner();
        else if (Procedure* p = dynamic_cast<Procedure*>(object))
            name = p->getOwner();
        else if (Role* r = dynamic_cast<Role*>(object))
            name = r->getOwner();
        if (!name.empty())
            htmlpage += escapeHtmlChars(name, false);
    }

    else if (cmd == wxT("object_description"))
    {
        if (object->isDescriptionAvailable())
        {
            wxString s = object->getDescription();
            if (s == wxT(""))
                s = wxT("No description");
            htmlpage += escapeHtmlChars(s);
            if (!suffix.IsEmpty())
                processHtmlCode(htmlpage, suffix, object);
        }
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
        r->checkAndLoadColumns();
        if (r->getChildren(tmp))
        {
            for (std::vector<MetadataItem*>::iterator it = tmp.begin(); it != tmp.end(); ++it)
                processHtmlCode(htmlpage, suffix, *it);
        }
    }

    // table triggers,  triggers:after or triggers:befor  <- not a typo
    else if (cmd == wxT("triggers"))
    {
        Relation* r = dynamic_cast<Relation*>(object);
        Database* d = dynamic_cast<Database*>(object);
        if (!r && !d)
            return;
        std::vector<Trigger*> tmp;
        if (r)
        {
            if (suffix.substr(0, 5) == wxT("after"))
                r->getTriggers(tmp, Trigger::afterTrigger);
            else
                r->getTriggers(tmp, Trigger::beforeTrigger);
            suffix.erase(0, 5);
        }
        else // d
            d->getDatabaseTriggers(tmp);
        for (std::vector<Trigger*>::iterator it = tmp.begin(); it != tmp.end(); ++it)
            processHtmlCode(htmlpage, suffix, *it);
    }

    else if (cmd == wxT("depends_on") || cmd == wxT("depend_of"))
    {
        MetadataItem* m = dynamic_cast<MetadataItem*>(object);
        if (!m)
            return;
        std::vector<Dependency> tmp;
        m->getDependencies(tmp, cmd == wxT("depends_on"));
        for (std::vector<Dependency>::iterator it = tmp.begin();
            it != tmp.end(); ++it)
        {
            processHtmlCode(htmlpage, suffix, &(*it));
        }
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
        PrimaryKeyConstraint* pk = t->getPrimaryKey();
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
        std::vector<UniqueConstraint>* c = t->getUniqueConstraints();
        if (!c)
            return;
        for (std::vector<UniqueConstraint>::iterator it = c->begin(); it != c->end(); ++it)
            processHtmlCode(htmlpage, suffix, &(*it));
    }

    else if (cmd == wxT("privileges"))
    {
        Relation* rel = dynamic_cast<Relation*>(object);
        Procedure* proc = dynamic_cast<Procedure*>(object);
        Role* role = dynamic_cast<Role*>(object);
        std::vector<Privilege>* p = 0;
        if (rel)
            p = rel->getPrivileges();
        if (proc)
            p = proc->getPrivileges();
        if (role)
            p = role->getPrivileges();
        if (!p)
            return;
        for (std::vector<Privilege>::iterator it = p->begin();
            it != p->end(); ++it)
        {
            processHtmlCode(htmlpage, suffix, &(*it));
        }
    }

    else if (cmd == wxT("grantee_name"))
    {
        Privilege* p = dynamic_cast<Privilege*>(object);
        if (!p)
            return;
        htmlpage += escapeHtmlChars(p->getGrantee());
    }

    else if (cmd == wxT("privilege"))
    {
        Privilege* p = dynamic_cast<Privilege*>(object);
        if (!p)
            return;
        wxString okimage = wxT("<img src=\"") +
            config().getHtmlTemplatesPath() + wxT("ok.png\">");
        wxString ok2image = wxT("<img src=\"") +
            config().getHtmlTemplatesPath() + wxT("ok2.png\">");
        // see which type
        std::vector<PrivilegeItem> list;
        p->getPrivileges(suffix, list);
        if (list.size())
        {
            bool brnext = false;
            for (std::vector<PrivilegeItem>::iterator it = list.begin(); it !=
                list.end(); ++it)
            {
                if (brnext)
                {
                    htmlpage += wxT("<br>");
                    brnext = false;
                }

                // wxHTML doesn't support TITLE or ALT property of IMG tag so
                // we use our custom 'link hower' handler to show it
                htmlpage += wxT("<a href=\"info://Granted by ") + (*it).grantor
                    + wxT("\">");

                htmlpage += wxT("<img src=\"") +
                    config().getHtmlTemplatesPath();
                if ((*it).grantOption)
                    htmlpage += wxT("ok2.png\"");
                else
                    htmlpage += wxT("ok.png\"");
                htmlpage += wxT("\"></a>");

                if ((*it).columns.size())
                {
                    htmlpage += wxT(" <font size=-1>");
                    for (std::vector<wxString>::iterator i =
                        (*it).columns.begin(); i != (*it).columns.end(); ++i)
                    {
                        if (i != (*it).columns.begin())
                            htmlpage += wxT(",");
                        htmlpage += (*i);
                    }
                    htmlpage += wxT("</font>");
                    brnext = true;
                }
            }
        }
        else
        {
            htmlpage += wxT("<img src=\"") + config().getHtmlTemplatesPath()
                + wxT("redx.png\">");
        }
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
        {   // needs newlines escaped for computed column source
            htmlpage += escapeHtmlChars(c->getDatatype());
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
        p->checkAndLoadParameters();
        if (p->getChildren(tmp))
        {
            bool parOut = (cmd == wxT("output_parameters"));
            std::vector<MetadataItem*>::iterator it;
            for (it = tmp.begin(); it != tmp.end(); ++it)
            {
                if ((dynamic_cast<Parameter*>(*it))->isOutputParameter() == parOut)
                    processHtmlCode(htmlpage, suffix, *it);
            }
        }
    }

    else if (cmd == wxT("view_source"))
    {
        View* v = dynamic_cast<View*>(object);
        if (!v)
            return;
        htmlpage += escapeHtmlChars(v->getSource(), false);
    }

    else if (cmd == wxT("procedure_source"))
    {
        Procedure* p = dynamic_cast<Procedure*>(object);
        if (!p)
            return;
        htmlpage += escapeHtmlChars(p->getSource(), false);
    }

    else if (cmd == wxT("trigger_source"))
    {
        Trigger* t = dynamic_cast<Trigger*>(object);
        if (!t)
            return;
        htmlpage += escapeHtmlChars(t->getSource(), false);
    }

    else if (cmd == wxT("trigger_info"))
    {
        Trigger* t = dynamic_cast<Trigger*>(object);
        wxString object, type;
        bool active, isDBtrigger;
        int position;
        if (!t)
            return;
        t->getTriggerInfo(object, active, position, type, isDBtrigger);
        wxString s(active ? wxT("Active ") : wxT("Inactive "));
        s << type << wxT(" trigger ");
        if (!isDBtrigger)
            s << wxT("for ") << object;
        s << wxT(" at position ") << position;
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
        ProgressDialog pd(GetParent(), _("Extracting DDL Definitions"), 2);

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

        htmlpage += d->getInfo().getCreated();
    }
    else if (cmd == wxT("default_charset"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += d->getDatabaseCharset();
    }
    else if (cmd == wxT("dialect"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getDialect();
    }
    else if (cmd == wxT("filesize"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        int64_t size = d->getInfo().getSizeInBytes();
        const float kilo = 1024.0;
        const float mega = kilo * kilo;
        const float giga = kilo * mega;
        if (size >= giga)
            htmlpage += wxString::Format(wxT("%0.2fGB"), size / giga);
        else if (size >= mega)
            htmlpage += wxString::Format(wxT("%0.2fMB"), size / mega);
        else
            htmlpage += wxString::Format(wxT("%0.2fkB"), size / kilo);
    }
    else if (cmd == wxT("forced_writes"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        wxString okimage = wxT("<img src=\"") + config().getHtmlTemplatesPath() + wxT("ok.png\">");
        wxString ximage = wxT("<img src=\"") + config().getHtmlTemplatesPath() + wxT("redx.png\">");

        htmlpage += (d->getInfo().getForcedWrites() ? okimage : ximage);
    }
    else if (cmd == wxT("fullpath"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += d->getConnectionString();
    }
    else if (cmd == wxT("next_transaction"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getNextTransaction();
    }
    else if (cmd == wxT("ods_version"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getODS();
        if (d->getInfo().getODSMinor())
        {
            htmlpage += wxT(".");
            htmlpage += wxString() << d->getInfo().getODSMinor();
        }
    }
    else if (cmd == wxT("oldest_transaction"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getOldestTransaction();
    }
    else if (cmd == wxT("page_buffers"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getBuffers();
    }
    else if (cmd == wxT("page_size"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getPageSize();
    }
    else if (cmd == wxT("pages"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getPages();
    }
    else if (cmd == wxT("read_only"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;
        if (d->getInfo().getReadOnly())
            htmlpage += wxT("true");
    else
            htmlpage += wxT("false");
    }
    else if (cmd == wxT("sweep_interval"))
    {
        Database* d = dynamic_cast<Database*>(object);
        if (!d)
            return;

        htmlpage += wxString() << d->getInfo().getSweep();
    }
}
//-----------------------------------------------------------------------------
//! processes html template code given in the htmlsource wxString
void MetadataItemPropertiesPanel::processHtmlCode(wxString& htmlpage, wxString htmlsource, MetadataItem *object)
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
void MetadataItemPropertiesPanel::processHtmlFile(wxString fileName)
{
    wxString htmlpage;
    processHtmlCode(htmlpage, loadEntireFile(fileName));

    int x = 0, y = 0;
    html_window->GetViewStart(&x, &y);         // save scroll position
    html_window->setPageSource(htmlpage);
    html_window->Scroll(x, y);                 // restore scroll position

    // set title
    if (getParentFrame())
    {
        getParentFrame()->setTabTitle(this, objectM->getName_() + wxT(": ")
            + html_window->GetOpenedPageTitle());
    }
}
//-----------------------------------------------------------------------------
//! closes window if observed object gets removed (disconnecting, dropping, etc)
void MetadataItemPropertiesPanel::removeSubject(Subject* subject)
{
    Observer::removeSubject(subject);
    // main observed object is getting destroyed
    if (subject == objectM)
    {
        objectM = 0;
        MetadataItemPropertiesFrame *f = getParentFrame();
        if (f)
            f->removePanel(this);
    }
}
//-----------------------------------------------------------------------------
MetadataItemPropertiesFrame* MetadataItemPropertiesPanel::getParentFrame()
{
    for (wxWindow *w = GetParent(); w; w = w->GetParent())
    {
        MetadataItemPropertiesFrame *f = dynamic_cast<
            MetadataItemPropertiesFrame *>(w);
        if (f)
            return f;
    }
    return 0;
}
//-----------------------------------------------------------------------------
/*
MetadataItemPropertiesPanel *MetadataItemPropertiesFrame::getItemPanel(
    MetadataItem *item)
{
    for (size_t c = 0; c < notebookM->GetPageCount(); c++)
    {
        MetadataItemPropertiesPanel *p = dynamic_cast<
            MetadataItemPropertiesPanel *>(notebookM->GetPage(c));
        if (p && p->getObservedObject() == item)
            return p;
    }
    return 0;
}*/
//-----------------------------------------------------------------------------
void MetadataItemPropertiesPanel::setPage(const wxString& type)
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
    else if (type == wxT("privileges"))
        pageTypeM = ptPrivileges;
    // add more page types here when needed
    else
        pageTypeM = ptSummary;
    requestLoadPage(true);
}
//-----------------------------------------------------------------------------
//! recreate html page if something changes
void MetadataItemPropertiesPanel::update()
{
    Database *db = dynamic_cast<Database *>(objectM);
    if (db && !db->isConnected())
    {
        objectM = 0;
        MetadataItemPropertiesFrame* f = getParentFrame();
        if (f)
            f->Close();

            // MB: This code used to use:
            //f->removePanel(this);
            // which would allow us to mix property pages from different
            // databases in the same Frame, but there are some mysterious
            // reasons why it causes heap corruption with MSVC

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
        std::vector<MetadataItem *>::iterator it;
        for (it = temp.begin(); it != temp.end(); ++it)
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
        std::vector<MetadataItem *>::iterator it;
        for (it = temp.begin(); it != temp.end(); ++it)
            (*it)->attachObserver(this);
    }

    // with this set to false updates to the same page do not show the
    // "Please wait while the data is being loaded..." temporary page
    // this results in less flicker, but may also seem less responsive
    requestLoadPage(false);
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(MetadataItemPropertiesPanel, wxPanel)
    EVT_HTML_CELL_HOVER(MetadataItemPropertiesPanel::HtmlWindowID,
        MetadataItemPropertiesPanel::OnHtmlCellHover)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(MetadataItemPropertiesFrame, BaseFrame)
    EVT_CLOSE(MetadataItemPropertiesFrame::OnClose)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
// when last tab is closed, close the frame
void MetadataItemPropertiesFrame::OnNotebookPageClose(wxAuiNotebookEvent&
    WXUNUSED(event))
{
    // seems that page count returns pages before event not after
    // probably because event can be Vetoed
    if (notebookM->GetPageCount() < 2)
        Close();
}
//-----------------------------------------------------------------------------
// when last tab is closed, close the frame
void MetadataItemPropertiesFrame::OnNotebookPageChanged(wxAuiNotebookEvent&
    event)
{
    int sel = event.GetSelection();
    if (sel == wxNOT_FOUND)
        return;
    if (databaseNameM.IsEmpty())
        SetTitle(notebookM->GetPageText(sel));
    else
        SetTitle(databaseNameM + wxT(" - ") + notebookM->GetPageText(sel));
}
//-----------------------------------------------------------------------------
void MetadataItemPropertiesFrame::OnClose(wxCloseEvent& event)
{
    Disconnect(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, wxAuiNotebookEventHandler(
        MetadataItemPropertiesFrame::OnNotebookPageClose), NULL, this);
    Disconnect(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED,
        wxAuiNotebookEventHandler(
        MetadataItemPropertiesFrame::OnNotebookPageChanged), NULL, this);
    BaseFrame::OnClose(event);
}
//-----------------------------------------------------------------------------
void MetadataItemPropertiesPanel::OnIdle(wxIdleEvent& WXUNUSED(event))
{
    Disconnect(wxID_ANY, wxEVT_IDLE);
    wxBusyCursor bc;
    htmlReloadRequestedM = false;
    loadPage();
}
//-----------------------------------------------------------------------------
void MetadataItemPropertiesPanel::OnHtmlCellHover(wxHtmlCellEvent& event)
{
    wxHtmlCell *c = event.GetCell();
    if (!c)
        return;
    wxHtmlLinkInfo *lnk = c->GetLink();
    if (!lnk)
        return;

    wxString addr = lnk->GetHref();
    URI uri(addr);
    if (uri.protocol == wxT("info"))    // special
    {
        //      GetStatusBar()->SetStatusText(uri.action);

        // I'm having a hard time trying to convert this to screen coordinates
        // since parent's coords cannot be retrieved(?)
        //wxRect r(c->GetPosX(), c->GetPosY(), c->GetWidth(), c->GetHeight());

        // M.B. So I decided to use a 21x9 box around the mouse
        wxRect r(::wxGetMousePosition().x - 10, ::wxGetMousePosition().y - 4,
            21, 9);

        wxTipWindow *tw = new wxTipWindow(this, uri.action);
        tw->SetBoundingRect(r);
    }
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

    MetadataItemPropertiesPanel* mpp = dynamic_cast<
        MetadataItemPropertiesPanel*>(getWindow(uri));
    if (!mpp)
        return true;

    if (uri.getParam(wxT("target")) == wxT("new"))
    {
        mpp = frameManager().showMetadataPropertyFrame(
            mpp->getObservedObject(), false, true); // !delayed, force_new
    }
    else if (uri.getParam(wxT("target")) == wxT("new_tab"))
    {
        mpp = frameManager().showMetadataPropertyFrame(
            mpp->getObservedObject(), false, false, true,
            mpp->getParentFrame()); // true = new_tab
    }

    mpp->setPage(uri.getParam(wxT("type")));
    //frameManager().rebuildMenu();
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

    MetadataItemPropertiesPanel* parent = dynamic_cast<
        MetadataItemPropertiesPanel*>(getWindow(uri));
    if (!parent)
        return true;
    Database* d = parent->getObservedObject()->findDatabase();
    if (!d)
        return true;
    NodeType n = getTypeByName(uri.getParam(wxT("object_type")));
    MetadataItem* object = d->findByNameAndType(n,
        uri.getParam(wxT("object_name")));
    if (!object)
    {
        ::wxMessageBox(
            _("Cannot find destination object\nThis should never happen."),
            _("Error"), wxICON_ERROR);
        return true;
    }

    frameManager().showMetadataPropertyFrame(object, false,
        uri.getParam(wxT("target")) == wxT("new"),
        uri.getParam(wxT("target")) == wxT("new_tab"),
        parent->getParentFrame());
    return true;
}
//-----------------------------------------------------------------------------
