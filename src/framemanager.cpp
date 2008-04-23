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
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <algorithm>

#include "metadata/database.h"
#include "gui/MetadataItemPropertiesFrame.h"
#include "framemanager.h"
//-----------------------------------------------------------------------------
FrameManager& frameManager()
{
    static FrameManager fm;
    return fm;
}
//-----------------------------------------------------------------------------
FrameManager::FrameManager()
{
}
//-----------------------------------------------------------------------------
FrameManager::~FrameManager()
{
}
//-----------------------------------------------------------------------------
void FrameManager::setWindowMenu(wxMenu *windowMenu)
{
    windowMenuM = windowMenu;

    // remember number of items that shouldn't be touched
    if (windowMenu)
        regularItemsM = windowMenu->GetMenuItemCount() + 1;
}
//-----------------------------------------------------------------------------
// TODO: currently we just clear and rebuild from scratch
//       we could implement more efficient algorithm later 
//       (find the one that is gone and remove, etc)
void FrameManager::rebuildMenu()
{
    if (windowMenuM == 0)
        return;

    /* this is currently not used?
     * 
    // remove all items
    while (windowMenuM->GetMenuItemCount() > regularItemsM)
        windowMenuM->Destroy(windowMenuM->FindItemByPosition(regularItemsM));
    if (windowMenuM->GetMenuItemCount() == regularItemsM-1)
        windowMenuM->AppendSeparator();

    // each database has it's submenu
    std::map<Database *, wxMenu *> dmm;

    // build submenus
    int id = 5000;    // start from "safe" 5000 and upwards to 6000
    for (ItemPanelMap::iterator it = mipPanelsM.begin(); 
        it != mipPanelsM.end(); ++it)
    {
        MetadataItem *m = (*it).second.panel->getObservedObject();
        if (!m)
            continue;
        Database* db = m->findDatabase();
        if (!db)
            continue;

        if (dmm.find(db) == dmm.end())  // add database if not already there
        {
            dmm.insert(dmm.begin(), 
                std::pair<Database*, wxMenu*>(db, new wxMenu));
        }
        (dmm[db])->Append(id, mf->GetTitle());
        (*it).second.id = id;
        ++id;
    }

    // wxWidgets manual says that we should insert the submenus at end
    for (std::map<Database *, wxMenu *>::iterator it = dmm.begin(); 
        it != dmm.end(); ++it)
    {
        windowMenuM->Append(-1, (*it).first->getName_(), (*it).second);
    }

    // remove the separator if nothing is beneath
    if (windowMenuM->GetMenuItemCount() == regularItemsM)    
        windowMenuM->Destroy(windowMenuM->FindItemByPosition(regularItemsM-1));
        */
}
//-----------------------------------------------------------------------------
void FrameManager::bringOnTop(int id)
{
    for (ItemPanelMap::iterator it = mipPanelsM.begin(); 
        it != mipPanelsM.end(); ++it)
    {
        if ((*it).second.id == id)
        {
            (*it).second.panel->showIt();
            break;
        }
    }
}
//-----------------------------------------------------------------------------
void FrameManager::removeFrame(MetadataItemPropertiesPanel* panel)
{
    if (panel)
        removeFrame(panel, mipPanelsM);
}
//-----------------------------------------------------------------------------
void FrameManager::removeFrame(MetadataItemPropertiesPanel* panel, 
    ItemPanelMap& frames)
{
    for (ItemPanelMap::iterator it = frames.begin(); it != frames.end(); ++it)
    {
        if ((*it).second.panel == panel)
        {
            mipPanelsM.erase(it);
            break;
        }
    }
    rebuildMenu();
}
//-----------------------------------------------------------------------------
MetadataItemPropertiesPanel* FrameManager::showMetadataPropertyFrame(
    wxWindow* parent, MetadataItem* item, bool delayed, bool force_new)
{
    MetadataItemPropertiesPanel* mpp = 0;
    ItemPanelMap::iterator it = mipPanelsM.find(item);
    if (it != mipPanelsM.end())
        mpp = (*it).second.panel;
        
    if (!mpp || force_new)
    {
        MetadataItemPropertiesPanel* newp = 0;
        // if that frame already shows another object, force new frame
        if (force_new || dynamic_cast<MetadataItemPropertiesFrame *>(parent))
        {
            // MainFrame should be parent of all MIPFrames
            MetadataItemPropertiesFrame* m = new MetadataItemPropertiesFrame(
                 wxTheApp->GetTopWindow(), item);
            newp = m->getPanel();
        }
        else
        {
            newp = new MetadataItemPropertiesPanel(
                dynamic_cast<BaseFrame *>(parent), item);
        }
        if (!mpp)   // new panel created, register it
        {
            PanelAndId fai(newp, 0);
            mipPanelsM.insert(mipPanelsM.begin(), 
                std::pair<MetadataItem*, PanelAndId>(item, fai));
        }
        mpp = newp;
    }
        
    if (delayed)
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_ACTIVATE_FRAME);
        event.SetEventObject(mpp);
        AddPendingEvent(event);
    }
    else
    {
        mpp->showIt();
    }
    if (!force_new)     // forced ones would rebuild themselves anyway
        rebuildMenu();
    return mpp;
}
//-----------------------------------------------------------------------------
//! event handlers
BEGIN_EVENT_TABLE(FrameManager, wxEvtHandler)
    EVT_MENU(FrameManager::ID_ACTIVATE_FRAME, FrameManager::OnCommandEvent)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void FrameManager::OnCommandEvent(wxCommandEvent& event)
{
    MetadataItemPropertiesPanel* panel = dynamic_cast<
        MetadataItemPropertiesPanel*>(event.GetEventObject());
    if (panel)
        panel->showIt();
}
//-----------------------------------------------------------------------------
