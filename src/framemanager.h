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

#ifndef FR_FRAMEMANAGER_H
#define FR_FRAMEMANAGER_H
//-----------------------------------------------------------------------------
#include <wx/event.h>

#include <map>

#include "gui/BaseFrame.h"
#include "gui/MetadataItemPropertiesFrame.h"
#include "metadata/metadataitem.h"
//--------------------------------------------------------------------------------------
class FrameAndId
{
public:
    BaseFrame *frame;
    int id;
    FrameAndId(BaseFrame *f, int i)
        :frame(f), id(i) {};
};
//--------------------------------------------------------------------------------------
typedef std::multimap<MetadataItem*, FrameAndId> ItemFrameMap;
//--------------------------------------------------------------------------------------
class FrameManager: public wxEvtHandler
{
public:
    FrameManager();
    ~FrameManager();

    void setWindowMenu(wxMenu *windowMenu);
    void rebuildMenu();
    void bringOnTop(int id);

    void removeFrame(BaseFrame* frame);
    MetadataItemPropertiesFrame* showMetadataPropertyFrame(wxWindow* parent, MetadataItem* item,
        bool delayed = false, bool force_new = false);

    void OnCommandEvent(wxCommandEvent& event);
protected:
    enum { ID_ACTIVATE_FRAME = 42 };
    DECLARE_EVENT_TABLE()
private:
    size_t regularItemsM;
    ItemFrameMap mipFramesM;
    wxMenu* windowMenuM;
    wxMenuBar* menuBarM;

    void removeFrame(BaseFrame* frame, ItemFrameMap& frames);
};
//--------------------------------------------------------------------------------------
FrameManager& frameManager();
//--------------------------------------------------------------------------------------
#endif // FR_FRAMEMANAGER_H
