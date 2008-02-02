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

#ifndef FR_METADATAITEMPROPERTIESFRAME_H
#define FR_METADATAITEMPROPERTIESFRAME_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/wxhtml.h>

// wx 2.6 doesn't support wxHtmlCellEvent
#if wxCHECK_VERSION(2, 8, 0)
#include <wx/html/htmlwin.h>
#endif

#include "core/Observer.h"
#include "gui/BaseFrame.h"
#include "gui/controls/PrintableHtmlWindow.h"
#include "metadata/metadataitem.h"
//-----------------------------------------------------------------------------
class MetadataItemPropertiesFrame: public BaseFrame, public Observer
{
private:
    enum { ptSummary, ptConstraints, ptDependencies, ptTriggers,
        ptTableIndices, ptDDL, ptPrivileges } pageTypeM;

	enum { HtmlWindowID = 42 };

    MetadataItem *objectM;
    bool htmlReloadRequestedM;
    PrintableHtmlWindow* html_window;

    // load page in idle handler, only request a reload in update()
    void requestLoadPage(bool showLoadingPage);
    void loadPage();
    void processCommand(wxString cmd, MetadataItem* object,
        wxString& htmlpage);
    void processHtmlCode(wxString& htmlpage, wxString htmlsource,
        MetadataItem* object = 0);

    // used to remember the value among calls to getStorageName(),
    // needed because it's not possible to access objectM
    // (see getStorageName()) after detaching from it.
    mutable wxString storageNameM;
protected:
    virtual const wxString getName() const;
    virtual const wxString getStorageName() const;
    virtual const wxRect getDefaultRect() const;
    virtual void removeSubject(Subject* subject);
    virtual void update();
public:
    MetadataItemPropertiesFrame(wxWindow* parent, MetadataItem *object);

    MetadataItem* getObservedObject() const;
    void processHtmlFile(wxString fileName);
    void setPage(const wxString& type);
private:
    // event handling
    void OnIdle(wxIdleEvent& event);

#if wxCHECK_VERSION(2, 8, 0)
    void OnHtmlCellHover(wxHtmlCellEvent &event);
    DECLARE_EVENT_TABLE()
#endif
};
//-----------------------------------------------------------------------------
#endif // FR_METADATAITEMPROPERTIESFRAME_H
