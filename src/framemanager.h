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
#include "metadata/metadataitem.h"
class MetadataItemPropertiesPanel;
//--------------------------------------------------------------------------------------
class PanelAndId
{
public:
    MetadataItemPropertiesPanel *panel;
    int id;
    PanelAndId(MetadataItemPropertiesPanel *p, int i)
        :panel(p), id(i) {};
};
//--------------------------------------------------------------------------------------
typedef std::multimap<MetadataItem*, PanelAndId> ItemPanelMap;
//--------------------------------------------------------------------------------------
class FrameManager: public wxEvtHandler
{
public:
    FrameManager();
    ~FrameManager();

    void bringOnTop(int id);

    void removeFrame(MetadataItemPropertiesPanel* panel);
    MetadataItemPropertiesPanel* showMetadataPropertyFrame(
        MetadataItem* item, bool delayed = false, bool new_frame = false,
        bool new_tab = false);

    void OnCommandEvent(wxCommandEvent& event);
protected:
    enum { ID_ACTIVATE_FRAME = 42 };
    DECLARE_EVENT_TABLE()
private:
    size_t regularItemsM;
    ItemPanelMap mipPanelsM;
    wxMenu* windowMenuM;
    wxMenuBar* menuBarM;

    void removeFrame(MetadataItemPropertiesPanel* frame, ItemPanelMap& panels);
};
//--------------------------------------------------------------------------------------
FrameManager& frameManager();
//--------------------------------------------------------------------------------------
#endif // FR_FRAMEMANAGER_H
