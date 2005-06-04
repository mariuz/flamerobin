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

  Contributor(s):
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

#include <algorithm>

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
        if ((*it).second == frame)
        {
            mipFramesM.erase(it);
            it = frames.begin();
        }
        else
            it++;
    }
}
//-----------------------------------------------------------------------------
void FrameManager::showMetadataPropertyFrame(wxWindow* parent, 
    YxMetadataItem* item, bool delayed)
{
    MetadataItemPropertiesFrame* mipf = 
        dynamic_cast<MetadataItemPropertiesFrame*>(mipFramesM[item]);
    if (!mipf)
    {
        mipf = new MetadataItemPropertiesFrame(parent, item);
        mipFramesM[item] = mipf;
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
