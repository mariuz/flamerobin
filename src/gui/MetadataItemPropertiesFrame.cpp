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

#include <wx/clipbrd.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/platform.h>
#include <wx/tipwin.h>
#include <wx/wupdlock.h>

#include <list>

#include "config/Config.h"
#include "core/ArtProvider.h"
#include "core/FRError.h"
#include "core/URIProcessor.h"
#include "engine/MetadataLoader.h"
#include "gui/GUIURIHandlerHelper.h"
#include "gui/HtmlTemplateProcessor.h"
#include "gui/MetadataItemPropertiesFrame.h"
#include "gui/ProgressDialog.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/parameter.h"
#include "metadata/procedure.h"
#include "metadata/table.h"
#include "metadata/view.h"

//! MetadataItemPropertiesPanel class
class MetadataItemPropertiesPanel: public wxPanel, public Observer
{
private:
    enum { ptSummary, ptConstraints, ptDependencies, ptTriggers,
        ptTableIndices, ptDDL, ptPrivileges } pageTypeM;

    MetadataItem* objectM;
    bool htmlReloadRequestedM;
    PrintableHtmlWindow* html_window;

    // load page in idle handler, only request a reload in update()
    void requestLoadPage(bool showLoadingPage);
    void loadPage();

    // observer stuff
    virtual void subjectRemoved(Subject* subject);
    virtual void update();
public:
    MetadataItemPropertiesPanel(MetadataItemPropertiesFrame* parent,
        MetadataItem* object);
    virtual ~MetadataItemPropertiesPanel();

    MetadataItem* getObservedObject() const;
    MetadataItemPropertiesFrame* getParentFrame();

    void setPage(const wxString& type);
private:
    // event handling
    void OnCloseFrame(wxCommandEvent& event);
    void OnHtmlCellHover(wxHtmlCellEvent &event);
    void OnIdle(wxIdleEvent& event);
    void OnRefresh(wxCommandEvent& event);
};

typedef std::list<MetadataItemPropertiesPanel*> MIPPanels;

static MIPPanels mipPanels;

MetadataItemPropertiesPanel::MetadataItemPropertiesPanel(
        MetadataItemPropertiesFrame* parent, MetadataItem* object)
    : wxPanel(parent, wxID_ANY), pageTypeM(ptSummary), objectM(object),
        htmlReloadRequestedM(false)
{
    wxASSERT(object);
    mipPanels.push_back(this);

    html_window = new PrintableHtmlWindow(this, wxID_ANY);
    parent->SetTitle(object->getName_());

    wxBoxSizer* bSizer2 = new wxBoxSizer( wxVERTICAL );
    bSizer2->Add(html_window, 1, wxEXPAND, 0 );
    SetSizer( bSizer2 );
    Layout();

    wxAcceleratorEntry entries[4];
    entries[0].Set(wxACCEL_CMD, (int) 'W', wxID_CLOSE_FRAME);
    entries[1].Set(wxACCEL_CMD, (int) 'R', wxID_REFRESH);
    // MSW only
    entries[2].Set(wxACCEL_CTRL, WXK_F4, wxID_CLOSE_FRAME);
    entries[3].Set(wxACCEL_NORMAL, WXK_F5, wxID_REFRESH);

    bool isMSW =
        (wxPlatformInfo::Get().GetOperatingSystemId() & wxOS_WINDOWS) != 0;
    wxAcceleratorTable acct(isMSW ? 4 : 2, entries);
    SetAcceleratorTable(acct);

    Connect(wxID_CLOSE_FRAME, wxEVT_COMMAND_MENU_SELECTED,
        wxCommandEventHandler(MetadataItemPropertiesPanel::OnCloseFrame));
    Connect(wxID_ANY, wxEVT_COMMAND_HTML_CELL_HOVER,
        wxHtmlCellEventHandler(MetadataItemPropertiesPanel::OnHtmlCellHover));
    Connect(wxID_REFRESH, wxEVT_COMMAND_MENU_SELECTED,
        wxCommandEventHandler(MetadataItemPropertiesPanel::OnRefresh));

    // request initial rendering
    requestLoadPage(true);
    objectM->attachObserver(this, true);
}

MetadataItemPropertiesPanel::~MetadataItemPropertiesPanel()
{
    mipPanels.remove(this);
}

MetadataItem* MetadataItemPropertiesPanel::getObservedObject() const
{
    return objectM;
}

//! defer (possibly expensive) creation and display of html page to idle time
void MetadataItemPropertiesPanel::requestLoadPage(bool showLoadingPage)
{
    if (!htmlReloadRequestedM)
    {
        if (showLoadingPage)
        {
            html_window->LoadFile(config().getHtmlTemplatesPath()
                + "ALLloading.html");
        }

        Connect(wxID_ANY, wxEVT_IDLE,
            wxIdleEventHandler(MetadataItemPropertiesPanel::OnIdle));
        htmlReloadRequestedM = true;
    }
}

//! determine the path, load and display html page
void MetadataItemPropertiesPanel::loadPage()
{
    wxString fileName = config().getHtmlTemplatesPath();
    switch (pageTypeM)
    {
        case ptSummary:
            fileName += objectM->getTypeName() + ".html";
            break;
        case ptConstraints:
            fileName += objectM->getTypeName() + "constraints.html";
            break;
        case ptTriggers:
            fileName += objectM->getTypeName() + "triggers.html";
            break;
        case ptPrivileges:
            fileName += objectM->getTypeName() + "privileges.html";
            break;
        case ptTableIndices:
            fileName += "TABLEindices.html";
            break;
        case ptDependencies:
            fileName += "dependencies.html";
            break;
        case ptDDL:
            fileName += "DDL.html";
            break;
    }

    wxBusyCursor bc;

    // start a transaction for metadata loading and lock the object
    DatabasePtr db = objectM->getDatabase();
    MetadataLoaderTransaction tr((db) ? db->getMetadataLoader() : 0);
    SubjectLocker lock(objectM);

    ProgressDialog pd(this, _("Processing template..."));
    pd.doShow();

    wxString htmlpage;
    HtmlTemplateProcessor tp(objectM, this);
    tp.processTemplateFile(htmlpage, fileName, 0, &pd);

    pd.SetTitle(_("Rendering page..."));

    wxWindowUpdateLocker freeze(html_window);
    int x = 0, y = 0;
    html_window->GetViewStart(&x, &y);         // save scroll position
    html_window->setPageSource(htmlpage);
    html_window->Scroll(x, y);                 // restore scroll position

    // set title
    if (MetadataItemPropertiesFrame* pf = getParentFrame())
    {
        pf->setTabTitle(this, objectM->getName_() + ": "
            + html_window->GetOpenedPageTitle());
    }
}

//! closes window if observed object gets removed (disconnecting, dropping, etc)
void MetadataItemPropertiesPanel::subjectRemoved(Subject* subject)
{
    // main observed object is getting destroyed
    if (subject == objectM)
    {
        objectM = 0;
        if (MetadataItemPropertiesFrame* f = getParentFrame())
            f->removePanel(this);
    }
}

MetadataItemPropertiesFrame* MetadataItemPropertiesPanel::getParentFrame()
{
    for (wxWindow* w = GetParent(); w; w = w->GetParent())
    {
        if (MetadataItemPropertiesFrame* f =
            dynamic_cast<MetadataItemPropertiesFrame*>(w))
        {
            return f;
        }
    }
    return 0;
}

void MetadataItemPropertiesPanel::setPage(const wxString& type)
{
    if (type == "constraints")
        pageTypeM = ptConstraints;
    else if (type == "dependencies")
        pageTypeM = ptDependencies;
    else if (type == "triggers")
        pageTypeM = ptTriggers;
    else if (type == "indices")
        pageTypeM = ptTableIndices;
    else if (type == "ddl")
        pageTypeM = ptDDL;
    else if (type == "privileges")
        pageTypeM = ptPrivileges;
    // add more page types here when needed
    else
        pageTypeM = ptSummary;
    requestLoadPage(true);
}

//! recreate html page if something changes
void MetadataItemPropertiesPanel::update()
{
    Database* db = dynamic_cast<Database*>(objectM);
    if (db && !db->isConnected())
    {
        objectM = 0;
        if (MetadataItemPropertiesFrame* f = getParentFrame())
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
        Relation* r = dynamic_cast<Relation*>(objectM);
        if (!r)
            return;

        SubjectLocker locker(r);
        r->ensureChildrenLoaded();
        for (ColumnPtrs::iterator it = r->begin(); it != r->end(); ++it)
            (*it)->attachObserver(this, false);
    }

    // if description of procedure params change, we need to reattach
    if (objectM->getType() == ntProcedure)
    {
        Procedure* p = dynamic_cast<Procedure*>(objectM);
        if (!p)
            return;

        SubjectLocker locker(p);
        p->ensureChildrenLoaded();
        for (ParameterPtrs::iterator it = p->begin(); it != p->end(); ++it)
            (*it)->attachObserver(this, false);
    }

    if (objectM->getType() == ntIndex)
    {
        Index* i = dynamic_cast<Index*>(objectM);
        if (!i)
            return;
        SubjectLocker locker(i);
        i->ensurePropertiesLoaded();
    }
    // with this set to false updates to the same page do not show the
    // "Please wait while the data is being loaded..." temporary page
    // this results in less flicker, but may also seem less responsive
    if (!htmlReloadRequestedM)
        requestLoadPage(false);
}

void MetadataItemPropertiesPanel::OnCloseFrame(wxCommandEvent& WXUNUSED(event))
{
    if (MetadataItemPropertiesFrame* f = getParentFrame())
        f->removePanel(this);
}

void MetadataItemPropertiesPanel::OnHtmlCellHover(wxHtmlCellEvent& event)
{
    wxHtmlCell* c = event.GetCell();
    if (!c)
        return;
    wxHtmlLinkInfo* lnk = c->GetLink();
    if (!lnk)
        return;

    wxString addr = lnk->GetHref();
    URI uri(addr);
    if (uri.protocol == "info")    // special
    {
        //      GetStatusBar()->SetStatusText(uri.action);
        static wxTipWindow* tw;
        if (tw) {
            tw->Close();
        }
        // I'm having a hard time trying to convert this to screen coordinates
        // since parent's coords cannot be retrieved(?)
        //wxRect r(c->GetPosX(), c->GetPosY(), c->GetWidth(), c->GetHeight());

        // M.B. So I decided to use a 21x9 box around the mouse
        wxRect r(::wxGetMousePosition().x - 10, ::wxGetMousePosition().y - 4,
            21, 9);

        tw = new wxTipWindow(this, uri.action, 100, &tw);
        tw->SetBoundingRect(r);
    }
}

void MetadataItemPropertiesPanel::OnIdle(wxIdleEvent& WXUNUSED(event))
{
    Disconnect(wxID_ANY, wxEVT_IDLE);
    wxBusyCursor bc;
    loadPage();
    htmlReloadRequestedM = false;
}

void MetadataItemPropertiesPanel::OnRefresh(wxCommandEvent& WXUNUSED(event))
{
    if (objectM)
        objectM->invalidate();
    // with this set to false updates to the same page do not show the
    // "Please wait while the data is being loaded..." temporary page
    // this results in less flicker, but may also seem less responsive
    requestLoadPage(false);
    SetFocus();
}

// TODO: replace this with a nice generic property page icon for all types
wxIcon getMetadataItemIcon(NodeType type)
{
    wxSize sz(32, 32);
    switch (type)
    {
        case ntColumn:
            return wxArtProvider::GetIcon(ART_Column, wxART_OTHER, sz);
        case ntDatabase:
            return wxArtProvider::GetIcon(ART_DatabaseConnected, wxART_OTHER, sz);
        case ntDomain:
            return wxArtProvider::GetIcon(ART_Domain, wxART_OTHER, sz);
        case ntFunctionSQL:
            return wxArtProvider::GetIcon(ART_Function, wxART_OTHER, sz);
        case ntUDF:
            return wxArtProvider::GetIcon(ART_Function, wxART_OTHER, sz);
        case ntGenerator:
            return wxArtProvider::GetIcon(ART_Generator, wxART_OTHER, sz);
// TODO: replace package art
        case ntPackage: 
            return wxArtProvider::GetIcon(ART_Procedure, wxART_OTHER, sz);
        case ntProcedure:
            return wxArtProvider::GetIcon(ART_Procedure, wxART_OTHER, sz);
        case ntServer:
            return wxArtProvider::GetIcon(ART_Server, wxART_OTHER, sz);
        case ntSysTable:
            return wxArtProvider::GetIcon(ART_SystemTable, wxART_OTHER, sz);
        case ntTable:
            return wxArtProvider::GetIcon(ART_Table, wxART_OTHER, sz);
        case ntDMLTrigger:
            return wxArtProvider::GetIcon(ART_Trigger, wxART_OTHER, sz);
        case ntView:
            return wxArtProvider::GetIcon(ART_View, wxART_OTHER, sz);
        default:
            break;
    }
    return wxArtProvider::GetIcon(ART_FlameRobin, wxART_OTHER, sz);
}

//! MetadataItemPropertiesFrame class
MetadataItemPropertiesFrame::MetadataItemPropertiesFrame(wxWindow* parent,
        MetadataItem* object)
    : BaseFrame(parent, wxID_ANY, wxEmptyString)
{
    // we need to store this right now, since we might lose the object later
    setStorageName(object);

    wxStatusBar* sb = CreateStatusBar();

    DatabasePtr db = object->getDatabase();
    if (db)  // server property page doesn't have a database, so don't crash
        sb->SetStatusText(db->getConnectionInfoString());
    else
        sb->SetStatusText(object->getName_());

    if (db && config().get("linksOpenInTabs", true))
    {
        SetIcon(wxArtProvider::GetIcon(ART_DatabaseConnected,
            wxART_FRAME_ICON));
        databaseNameM = db->getName_();
    }
    else  // when linksOpenInTabs, only the server node
    {
        SetIcon(getMetadataItemIcon(object->getType()));
    }

    notebookM = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_WINDOWLIST_BUTTON
        | wxAUI_NB_TAB_EXTERNAL_MOVE | wxNO_BORDER);

    auiManagerM.SetManagedWindow(this);
    auiManagerM.AddPane(notebookM,
        wxAuiPaneInfo().CenterPane().PaneBorder(false));
    auiManagerM.Update();

    Connect(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, wxAuiNotebookEventHandler(
        MetadataItemPropertiesFrame::OnNotebookPageClose), NULL, this);
    Connect(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, wxAuiNotebookEventHandler(
        MetadataItemPropertiesFrame::OnNotebookPageChanged), NULL, this);
}

MetadataItemPropertiesFrame::~MetadataItemPropertiesFrame()
{
    auiManagerM.UnInit();
}

/*static*/ MetadataItemPropertiesPanel*
MetadataItemPropertiesFrame::openNewPropertyPageInFrame(MetadataItem* object)
{
    MetadataItemPropertiesFrame* mf = new MetadataItemPropertiesFrame(
        wxTheApp->GetTopWindow(), object);
    MetadataItemPropertiesPanel* mpp = new MetadataItemPropertiesPanel(mf,
        object);

    mf->showPanel(mpp, object->getName_());
    return mpp;
}

/*static*/ MetadataItemPropertiesPanel*
MetadataItemPropertiesFrame::openNewPropertyPageInTab(MetadataItem* object,
    MetadataItemPropertiesFrame* parentFrame)
{
    // find frame showing the same database
    MetadataItemPropertiesFrame* mf = 0;
    if (object)
    {
        DatabasePtr db = object->getDatabase();
        if (db)
        {
            for (MIPPanels::iterator it = mipPanels.begin();
                it != mipPanels.end(); ++it)
            {
                MetadataItem* mi = (*it)->getObservedObject();
                if (mi && mi->getDatabase() == db)
                {
                    mf = (*it)->getParentFrame();
                    if (parentFrame == 0 || parentFrame == mf)
                        break;
                }
            }
        }
    }
    if (!mf)
        return openNewPropertyPageInFrame(object);

    MetadataItemPropertiesPanel* mpp = new MetadataItemPropertiesPanel(mf,
        object);
    mf->showPanel(mpp, object->getName_());
    return mpp;
}

/*static*/ MetadataItemPropertiesPanel*
MetadataItemPropertiesFrame::showPropertyPage(MetadataItem* object)
{
    if (object)
    {
        for (MIPPanels::iterator it = mipPanels.begin();
            it != mipPanels.end(); ++it)
        {
            if ((*it)->getObservedObject() == object)
            {
                if (MetadataItemPropertiesFrame* mf = (*it)->getParentFrame())
                    mf->showPanel(*it, object->getName_());
                return *it;
            }
        }
    }

    if (config().get("linksOpenInTabs", true))
        return openNewPropertyPageInTab(object, 0);
    return openNewPropertyPageInFrame(object);
}

const wxRect MetadataItemPropertiesFrame::getDefaultRect() const
{
    return wxRect(-1, -1, 600, 420);
}

const wxString MetadataItemPropertiesFrame::getName() const
{
    return "MIPFrame";
}

const wxString MetadataItemPropertiesFrame::getStorageName() const
{
    return storageNameM;
}

void MetadataItemPropertiesFrame::removePanel(
    MetadataItemPropertiesPanel* panel)
{
    int pg = notebookM->GetPageIndex(panel);
    if (pg == wxNOT_FOUND)
        return;

    notebookM->DeletePage(pg);
    if (notebookM->GetPageCount() < 1)
        Close();
}

void MetadataItemPropertiesFrame::setStorageName(MetadataItem* object)
{
    StorageGranularity g;
    if (!config().getValue("MetadataFrameStorageGranularity", g))
        g = sgFrame;
    if (config().get("linksOpenInTabs", true))
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
            storageNameM = "";
            break;
    }
}

void MetadataItemPropertiesFrame::setTabTitle(
    MetadataItemPropertiesPanel* panel, const wxString& title)
{
    int pg = notebookM->GetPageIndex(panel);
    if (pg == wxNOT_FOUND)
        return;
    notebookM->SetPageText(pg, title);
}

void MetadataItemPropertiesFrame::showPanel(MetadataItemPropertiesPanel* panel,
    const wxString& title)
{
    int pg = notebookM->GetPageIndex(panel);
    if (pg == wxNOT_FOUND)
        notebookM->AddPage(panel, title, true);
    else
        notebookM->SetSelection(pg);

    Show();
    if (!IsMaximized())
        Maximize(true);

    if (panel)
        panel->SetFocus();
    Raise();
}

// when last tab is closed, close the frame
void MetadataItemPropertiesFrame::OnNotebookPageClose(
    wxAuiNotebookEvent& WXUNUSED(event))
{
    // seems that page count returns pages before event not after
    // probably because event can be Vetoed
    if (notebookM->GetPageCount() < 2)
        Close();
}

// when last tab is closed, close the frame
void MetadataItemPropertiesFrame::OnNotebookPageChanged(
    wxAuiNotebookEvent& event)
{
    int sel = event.GetSelection();
    if (sel == wxNOT_FOUND)
        return;
    if (databaseNameM.empty())
        SetTitle(notebookM->GetPageText(sel));
    else
        SetTitle(databaseNameM + " - " + notebookM->GetPageText(sel));
}

//! PageHandler class
class PageHandler: public URIHandler, private GUIURIHandlerHelper
{
public:
    PageHandler() {};
    bool handleURI(URI& uri);
private:
    static const PageHandler handlerInstance;   // singleton; registers itself on creation.
};
const PageHandler PageHandler::handlerInstance;

bool PageHandler::handleURI(URI& uri)
{
    if (uri.action != "page")
        return false;

    MetadataItemPropertiesPanel* mpp = dynamic_cast<
        MetadataItemPropertiesPanel*>(getParentWindow(uri));
    if (!mpp)
        return true;

    if (uri.getParam("target") == "new")
    {
        mpp = MetadataItemPropertiesFrame::openNewPropertyPageInFrame(
            mpp->getObservedObject());
    }
    else if (uri.getParam("target") == "new_tab")
    {
        mpp = MetadataItemPropertiesFrame::openNewPropertyPageInTab(
            mpp->getObservedObject(), mpp->getParentFrame());
    }

    mpp->setPage(uri.getParam("type"));
    //frameManager().rebuildMenu();
    return true;
}

//! PropertiesHandler class
class PropertiesHandler: public URIHandler, private GUIURIHandlerHelper
{
public:
    PropertiesHandler() {};
    bool handleURI(URI& uri);
private:
    static const PropertiesHandler handlerInstance; // singleton; registers itself on creation.
};
const PropertiesHandler PropertiesHandler::handlerInstance;

bool PropertiesHandler::handleURI(URI& uri)
{
    if (uri.action != "properties")
        return false;

    MetadataItemPropertiesPanel* parent = dynamic_cast<
        MetadataItemPropertiesPanel*>(getParentWindow(uri));
    if (!parent)
        return true;
    DatabasePtr db = parent->getObservedObject()->getDatabase();
    if (!db)
        return true;
    NodeType n = getTypeByName(uri.getParam("object_type"));
    MetadataItem* object = db->findByNameAndType(n,
        uri.getParam("object_name"));
    if (!object)
    {
        ::wxMessageBox(
            _("Cannot find destination object\nThis should never happen."),
            _("Error"), wxICON_ERROR);
        return true;
    }

    if (uri.getParam("target") == "new_tab")
    {
        MetadataItemPropertiesFrame::openNewPropertyPageInTab(object,
            parent->getParentFrame());
    }
    else if (uri.getParam("target") == "new")
        MetadataItemPropertiesFrame::openNewPropertyPageInFrame(object);
    else 
        MetadataItemPropertiesFrame::showPropertyPage(object);
    return true;
}

