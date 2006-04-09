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

#ifndef METADATAITEMPROPERTIESFRAME_H
#define METADATAITEMPROPERTIESFRAME_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/wxhtml.h>

#include "core/Observer.h"
#include "gui/BaseFrame.h"
#include "metadata/metadataitem.h"
#include "PrintableHtmlWindow.h"
//-----------------------------------------------------------------------------
class MetadataItemPropertiesFrame: public BaseFrame, public Observer
{
private:
    enum { ptSummary, ptConstraints, ptDependencies, ptTriggers,
        ptTableIndices, ptDDL, ptPrivileges } pageTypeM;

    MetadataItem *objectM;
    void removeSubject(Subject* subject);

    void do_layout();
    void loadPage();    // force reload from outside
    void processCommand(wxString cmd, MetadataItem *object, wxString& htmlpage);
    void processHtmlCode(wxString& htmlpage, wxString htmlsource, MetadataItem *object = 0);
    void update();
    // used to remember the value among calls to getStorageName(), needed because
    // it's not possible to access objectM (see getStorageName()) after detaching from it.
    mutable wxString storageNameM;
protected:
    PrintableHtmlWindow* window_1;
    virtual const wxString getName() const;
    virtual const wxString getStorageName() const;
    virtual const wxRect getDefaultRect() const;
public:
    MetadataItem *getObservedObject() const;
    void processHtmlFile(wxString fileName);
    void setPage(const wxString& type);

    MetadataItemPropertiesFrame(wxWindow* parent, MetadataItem *object, int id = -1);
};
//-----------------------------------------------------------------------------
#endif // METADATAITEMPROPERTIESFRAME_H
