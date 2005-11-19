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

  The Initial Developer of the Original Code is Nando Dessena.

  Portions created by the original developer
  are Copyright (C) 2004 Nando Dessena.

  All Rights Reserved.

  $Id$

  Contributor(s): Milan Babuskov, Michael Hieke, Nando Dessena
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

#ifdef __WIN32__
    #include "windows.h"
#endif

#include "config/Config.h"
#include "framemanager.h"
#include "gui/BaseFrame.h"
//-----------------------------------------------------------------------------
BaseFrame::FrameIdMap BaseFrame::frameIdsM;
//-----------------------------------------------------------------------------
BaseFrame::BaseFrame(wxWindow* parent, int id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style, const wxString& name)
    : wxFrame(parent, id, title, pos, size, style | wxNO_FULL_REPAINT_ON_RESIZE, name)
{
    frameIdsM.insert(FrameIdPair(this, wxEmptyString));
}
//-----------------------------------------------------------------------------
BaseFrame::~BaseFrame()
{
    frameIdsM.erase(this);
}
//-----------------------------------------------------------------------------
bool BaseFrame::Show(bool show)
{
    if (show && !IsShown())
         readConfigSettings();
    return wxFrame::Show(show);
}
//-----------------------------------------------------------------------------
bool BaseFrame::Destroy()
{
    frameManager().removeFrame(this);
    writeConfigSettings();
    return wxFrame::Destroy();
}
//-----------------------------------------------------------------------------
void BaseFrame::readConfigSettings()
{
    // load position and size from config; it values are not set, they will be untouched
      wxRect r = getDefaultRect();
    bool enabled = false;
    bool maximized = false;
     if (config().getValue(wxT("FrameStorage"), enabled) && enabled)
    {
        wxString itemPrefix = getStorageName();
        if (!itemPrefix.empty())
        {
            config().getValue(itemPrefix + Config::pathSeparator + wxT("maximized"), maximized);
            config().getValue(itemPrefix + Config::pathSeparator + wxT("x"), r.x);
            config().getValue(itemPrefix + Config::pathSeparator + wxT("y"), r.y);
            config().getValue(itemPrefix + Config::pathSeparator + wxT("width"), r.width);
            config().getValue(itemPrefix + Config::pathSeparator + wxT("height"), r.height);
            doReadConfigSettings(itemPrefix);
        }
    }
    SetSize(r);
    if (maximized)
        Maximize();
}
void BaseFrame::doReadConfigSettings(const wxString& WXUNUSED(prefix))
{
}
//-----------------------------------------------------------------------------
void BaseFrame::writeConfigSettings() const
{
    // propagate call to children frames.
    const wxWindowList& children = GetChildren();
    wxWindowListNode *node = children.GetFirst();
    while (node)
    {
        BaseFrame *f = dynamic_cast<BaseFrame *>(node->GetData());
        if (f)
          {
               f->writeConfigSettings();
        }
        node = node->GetNext();
    }

    if (config().get(wxT("FrameStorage"), false) && !IsIconized())    // don't save for minimized windows
    {
        // save window position and size to config.
        wxString itemPrefix = getStorageName();
        if (!itemPrefix.empty())
        {
            wxRect r = GetRect();
            // TODO: move this to a better place when source is refactored
#ifdef __WIN32__
            WINDOWPLACEMENT wp;
            wp.length = sizeof(WINDOWPLACEMENT);
            if (::GetWindowPlacement((HWND)GetHandle(), &wp) && IsMaximized())
            {
                r.SetLeft(wp.rcNormalPosition.left);
                r.SetTop(wp.rcNormalPosition.top);
                r.SetRight(wp.rcNormalPosition.right);
                r.SetBottom(wp.rcNormalPosition.bottom);
            }
            config().setValue(itemPrefix + Config::pathSeparator + wxT("maximized"), IsMaximized());
#endif
            config().setValue(itemPrefix + Config::pathSeparator + wxT("x"), r.x);
            config().setValue(itemPrefix + Config::pathSeparator + wxT("y"), r.y);
            config().setValue(itemPrefix + Config::pathSeparator + wxT("width"), r.width);
            config().setValue(itemPrefix + Config::pathSeparator + wxT("height"), r.height);
            doWriteConfigSettings(itemPrefix);
        }
    }
}
//-----------------------------------------------------------------------------
void BaseFrame::doWriteConfigSettings(const wxString& WXUNUSED(prefix)) const
{
}
//-----------------------------------------------------------------------------
const wxString BaseFrame::getName() const
{
    // Couldn't find a reliable (meaning supportable and cross-platform) way
    // to use the class name here, so every derived frame needs to override getName()
    // if it needs to use features that depend on it.
    return wxT("");
}
//-----------------------------------------------------------------------------
const wxString BaseFrame::getStorageName() const
{
    return getName();
}
//-----------------------------------------------------------------------------
const wxRect BaseFrame::getDefaultRect() const
{
    return wxRect(-1, -1, -1, -1);
}
//-----------------------------------------------------------------------------
void BaseFrame::setIdString(BaseFrame* frame, const wxString& id)
{
    FrameIdMap::iterator it = frameIdsM.find(frame);
    if (it != frameIdsM.end())
        (*it).second = id;
}
//-----------------------------------------------------------------------------
BaseFrame* BaseFrame::frameFromIdString(const wxString& id)
{
    if (!id.IsEmpty())
    {
        FrameIdMap::iterator it;
        for (it = frameIdsM.begin(); it != frameIdsM.end(); it++)
        {
            if ((*it).second == id)
                return (*it).first;
        }
    }
    return 0;
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(BaseFrame, wxFrame)
    EVT_CLOSE(BaseFrame::OnClose)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void BaseFrame::OnClose(wxCloseEvent& WXUNUSED(event))
{
    Destroy();
}
//-----------------------------------------------------------------------------
