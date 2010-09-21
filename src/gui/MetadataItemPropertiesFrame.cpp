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
#include "engine/MetadataLoader.h"
#include "framemanager.h"
#include "frutils.h"
#include "gui/HtmlTemplateProcessor.h"
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
        sb->SetStatusText(object->getName_());

    wxIcon icon;
    if (d && config().get(wxT("linksOpenInTabs"), true))
    {
        wxBitmap bmp = getImage32(d->getType());
        icon.CopyFromBitmap(bmp);
        databaseNameM = d->getName_();
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
    if (panel)
        panel->SetFocus();
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
//! processes the given html template file
void MetadataItemPropertiesPanel::processHtmlFile(wxString fileName)
{
    wxString htmlpage;
    HtmlTemplateProcessor tp(objectM);
    tp.processTemplateFile(htmlpage, fileName, 0, this);

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
        Relation* r = dynamic_cast<Relation*>(objectM);
        if (!r)
            return;

        SubjectLocker locker(r);
        r->ensureChildrenLoaded();
        RelationColumns::iterator it;
        for (it = r->begin(); it != r->end(); ++it)
            (*it)->attachObserver(this);
    }

    // if description of procedure params change, we need to reattach
    if (objectM->getType() == ntProcedure)
    {
        Procedure* p = dynamic_cast<Procedure*>(objectM);
        if (!p)
            return;

        SubjectLocker locker(p);
        p->ensureChildrenLoaded();
        ProcedureParameters::iterator it;
        for (it = p->begin(); it != p->end(); ++it)
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
