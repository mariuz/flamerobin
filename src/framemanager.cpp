/*
  Copyright (c) 2004-2009 The FlameRobin Development Team

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

#include "config/Config.h"
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
}
//-----------------------------------------------------------------------------
MetadataItemPropertiesPanel* FrameManager::showMetadataPropertyFrame(
    MetadataItem* item, bool delayed, bool new_frame, bool new_tab,
    MetadataItemPropertiesFrame *windowHint)
{
    MetadataItemPropertiesPanel* mpp = 0;
    ItemPanelMap::iterator it = mipPanelsM.find(item);
    if (it != mipPanelsM.end())
        mpp = (*it).second.panel;

    // when user selected preference to open in new window and
    // property page does not exist, and new tab is not explicitly requested
    if (!mpp && !new_tab && !new_frame)
    {
        if (!config().get(wxT("linksOpenInTabs"), true))
            new_frame = true;
    }

    // doesn't exists || forced creation of duplicate
    if (!mpp || new_frame || new_tab)
    {
        // find frame showing the same database
        MetadataItemPropertiesFrame *mf = 0;
        if (!new_frame) // either a new tab, or does not exists
        {
            for (it = mipPanelsM.begin(); it != mipPanelsM.end(); ++it)
            {
                if ((*it).first->findDatabase() == item->findDatabase())
                {
                    mf = (*it).second.panel->getParentFrame();
                    if (!windowHint || mf == windowHint)
                        break;
                }
            }
        }

        if (new_frame || !mf)   // not exists, or new frame forced
        {   // MainFrame is the parent of all MIPFrames
            mf = new MetadataItemPropertiesFrame(wxTheApp->GetTopWindow(),
                item);
        }
        mpp = new MetadataItemPropertiesPanel(mf, item);
        PanelAndId fai(mpp, 0);
        mipPanelsM.insert(mipPanelsM.begin(),
            std::pair<MetadataItem*, PanelAndId>(item, fai));
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
