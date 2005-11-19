/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Michael Hieke.

  Portions created by the original developer
  are Copyright (C) 2005 Michael Hieke.

  All Rights Reserved.

  $Id$

  Contributor(s): Milan Babuskov.
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

#include "ugly.h"
#include "metadata/database.h"
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
}
//-----------------------------------------------------------------------------
// TODO: currently we just clear and rebuild from scratch
//       we could implement more efficient algorithm later (find the one that is gone and remove, etc)
void FrameManager::rebuildMenu()
{
    if (windowMenuM == 0)
        return;

    const int regularItems = 5;

    // remove all items
    while (windowMenuM->GetMenuItemCount() > regularItems)
        windowMenuM->Destroy(windowMenuM->FindItemByPosition(regularItems));
    if (windowMenuM->GetMenuItemCount() == regularItems-1)
        windowMenuM->AppendSeparator();

    // each database has it's submenu
    std::map<Database *, wxMenu *> dmm;

    // build submenus
    int id = 5000;    // these menu IDs start from "safe" 5000 and upwards to 6000 (let's hope users won't open 1001 windows)
    for (ItemFrameMap::iterator it = mipFramesM.begin(); it != mipFramesM.end(); ++it)
    {
        MetadataItemPropertiesFrame *mf = dynamic_cast<MetadataItemPropertiesFrame *>((*it).second.frame);
        if (!mf)
            continue;
        MetadataItem *m = mf->getObservedObject();
        if (!m)
            continue;
        Database *db = m->getDatabase();
        if (!db)
            continue;

        if (dmm.find(db) == dmm.end())        // add database if not already there
            dmm.insert(dmm.begin(), std::pair<Database*, wxMenu*>(db, new wxMenu));
        (dmm[db])->Append(id, mf->GetTitle());
        (*it).second.id = id;
        ++id;
    }

    // wxWidgets manual says that we should insert the submenus at end
    for (std::map<Database *, wxMenu *>::iterator it = dmm.begin(); it != dmm.end(); ++it)
        windowMenuM->Append(-1, (*it).first->getName(), (*it).second);

    if (windowMenuM->GetMenuItemCount() == regularItems)    // remove the separator if nothing is beneath
        windowMenuM->Destroy(windowMenuM->FindItemByPosition(regularItems-1));
}
//-----------------------------------------------------------------------------
void FrameManager::bringOnTop(int id)
{
    for (ItemFrameMap::iterator it = mipFramesM.begin(); it != mipFramesM.end(); ++it)
    {
        if ((*it).second.id == id)
        {
            MetadataItemPropertiesFrame* mipf = dynamic_cast<MetadataItemPropertiesFrame *>((*it).second.frame);
            if (mipf)
            {
                mipf->Show();
                mipf->Raise();
            }
            break;
        }
    }
}
//-----------------------------------------------------------------------------
void FrameManager::removeFrame(BaseFrame* frame)
{
    if (frame)
    {
        removeFrame(frame, mipFramesM);
    }
}
//-----------------------------------------------------------------------------
void FrameManager::removeFrame(BaseFrame* frame, ItemFrameMap& frames)
{
    ItemFrameMap::iterator it;
    for (it = frames.begin(); it != frames.end();)
    {
        if ((*it).second.frame == frame)
        {
            mipFramesM.erase(it);
            it = frames.begin();
        }
        else
            it++;
    }
    rebuildMenu();
}
//-----------------------------------------------------------------------------
MetadataItemPropertiesFrame* FrameManager::showMetadataPropertyFrame(wxWindow* parent,
    MetadataItem* item, bool delayed, bool force_new)
{
    MetadataItemPropertiesFrame* mipf = 0;
    ItemFrameMap::iterator it = mipFramesM.find(item);
    if (it != mipFramesM.end())
        mipf = dynamic_cast<MetadataItemPropertiesFrame *>((*it).second.frame);
    if (!mipf || force_new)
    {
        mipf = new MetadataItemPropertiesFrame(parent, item);
        FrameAndId fai(mipf, 0);
        mipFramesM.insert(mipFramesM.begin(), std::pair<MetadataItem*, FrameAndId>(item, fai));
    }
    if (delayed)
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_ACTIVATE_FRAME);
        event.SetEventObject(mipf);
        AddPendingEvent(event);
    }
    else
    {
        mipf->Show();
        mipf->Raise();
    }
    if (!force_new)     // forced ones would rebuild themselves anyway
        rebuildMenu();
    return mipf;
}
//-----------------------------------------------------------------------------
//! event handlers
BEGIN_EVENT_TABLE(FrameManager, wxEvtHandler)
    EVT_MENU(FrameManager::ID_ACTIVATE_FRAME, FrameManager::OnCommandEvent)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void FrameManager::OnCommandEvent(wxCommandEvent& event)
{
    wxFrame* frame = dynamic_cast<wxFrame*>(event.GetEventObject());
    if (frame)
    {
        frame->Show();
        frame->Raise();
    }
}
//-----------------------------------------------------------------------------
