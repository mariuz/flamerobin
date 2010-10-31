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
#include <wx/platform.h>
#include <wx/tipwin.h>

#include "config/Config.h"
#include "core/FRError.h"
#include "core/URIProcessor.h"
#include "engine/MetadataLoader.h"
#include "framemanager.h"
#include "frutils.h"
#include "gui/GUIURIHandlerHelper.h"
#include "gui/HtmlTemplateProcessor.h"
#include "gui/MetadataItemPropertiesFrame.h"
#include "gui/ProgressDialog.h"
#include "images.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/exception.h"
#include "metadata/parameter.h"
#include "metadata/procedure.h"
#include "metadata/server.h"
#include "metadata/table.h"
#include "metadata/view.h"
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

    notebookM = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_WINDOWLIST_BUTTON
        | wxAUI_NB_TAB_EXTERNAL_MOVE | wxNO_BORDER);

    auiManagerM.SetManagedWindow(this);
    auiManagerM.AddPane(notebookM,
        wxAuiPaneInfo().CenterPane().PaneBorder(false));
    auiManagerM.Update();

    Connect(wxEVT_CLOSE_WINDOW,
        wxCloseEventHandler(MetadataItemPropertiesFrame::OnClose));
    Connect(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, wxAuiNotebookEventHandler(
        MetadataItemPropertiesFrame::OnNotebookPageClose), NULL, this);
    Connect(wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, wxAuiNotebookEventHandler(
        MetadataItemPropertiesFrame::OnNotebookPageChanged), NULL, this);
}
//-----------------------------------------------------------------------------
MetadataItemPropertiesFrame::~MetadataItemPropertiesFrame()
{
    auiManagerM.UnInit();
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

    html_window = new PrintableHtmlWindow(this, wxID_ANY);
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
}
//-----------------------------------------------------------------------------
MetadataItemPropertiesPanel::~MetadataItemPropertiesPanel()
{
    frameManager().removeFrame(this);
}
//-----------------------------------------------------------------------------
void MetadataItemPropertiesPanel::showIt()
{
    if (MetadataItemPropertiesFrame* f = getParentFrame())
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
            html_window->LoadFile(config().getHtmlTemplatesPath()
                + wxT("ALLloading.html"));
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
void MetadataItemPropertiesPanel::processHtmlFile(const wxString& fileName)
{
    ProgressDialog pd(this, wxT("Processing template..."));
    pd.doShow();

    wxString htmlpage;
    HtmlTemplateProcessor tp(objectM, this);
    tp.processTemplateFile(htmlpage, fileName, 0, &pd);

    int x = 0, y = 0;
    html_window->GetViewStart(&x, &y);         // save scroll position
    html_window->setPageSource(htmlpage);
    html_window->Scroll(x, y);                 // restore scroll position

    // set title
    if (MetadataItemPropertiesFrame* pf = getParentFrame())
    {
        pf->setTabTitle(this, objectM->getName_() + wxT(": ")
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
        if (MetadataItemPropertiesFrame* f = getParentFrame())
            f->removePanel(this);
    }
}
//-----------------------------------------------------------------------------
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
        for (ParameterPtrs::iterator it = p->begin(); it != p->end(); ++it)
            (*it)->attachObserver(this);
    }

    // with this set to false updates to the same page do not show the
    // "Please wait while the data is being loaded..." temporary page
    // this results in less flicker, but may also seem less responsive
    requestLoadPage(false);
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
void MetadataItemPropertiesPanel::OnCloseFrame(wxCommandEvent& WXUNUSED(event))
{
    if (MetadataItemPropertiesFrame* f = getParentFrame())
        f->removePanel(this);
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
void MetadataItemPropertiesPanel::OnIdle(wxIdleEvent& WXUNUSED(event))
{
    Disconnect(wxID_ANY, wxEVT_IDLE);
    wxBusyCursor bc;
    htmlReloadRequestedM = false;
    loadPage();
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
bool PageHandler::handleURI(URI& uri)
{
    if (uri.action != wxT("page"))
        return false;

    MetadataItemPropertiesPanel* mpp = dynamic_cast<
        MetadataItemPropertiesPanel*>(getParentWindow(uri));
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
class PropertiesHandler: public URIHandler, private GUIURIHandlerHelper
{
public:
    PropertiesHandler() {};
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
        MetadataItemPropertiesPanel*>(getParentWindow(uri));
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
