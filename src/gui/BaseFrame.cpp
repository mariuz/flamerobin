/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wx/display.h"

#if defined(__WXMSW__)
    #include "wx/msw/wrapwin.h" // for "windows.h"
#endif

#include "config/Config.h"
#include "gui/BaseFrame.h"

BaseFrame::FrameIdMap BaseFrame::frameIdsM;

BaseFrame::BaseFrame(wxWindow* parent, int id, const wxString& title,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
    : wxFrame(parent, id, title, pos, size, style, name)
{
    frameIdsM.insert(FrameIdPair(this, wxEmptyString));

    Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(BaseFrame::OnClose));
}

BaseFrame::~BaseFrame()
{
    frameIdsM.erase(this);
}

bool BaseFrame::Show(bool show)
{
    if (show && !IsShown())
         readConfigSettings();
    return wxFrame::Show(show);
}

bool BaseFrame::Destroy()
{
    writeConfigSettings();
    return wxFrame::Destroy();
}

bool BaseFrame::canClose()
{
    return doCanClose();
}

bool BaseFrame::doCanClose()
{
    return true;
}

void BaseFrame::doBeforeDestroy()
{
}

void BaseFrame::readConfigSettings()
{
    // load position and size from config; it values are not set, they will be untouched
    wxRect rcDefault = getDefaultRect();
    wxRect rc = rcDefault;
    bool enabled = false;
    bool maximized = false;
    if (config().getValue("FrameStorage", enabled) && enabled)
    {
        wxString itemPrefix = getStorageName();
        if (!itemPrefix.empty())
        {
            config().getValue(itemPrefix + Config::pathSeparator + "maximized", maximized);
            config().getValue(itemPrefix + Config::pathSeparator + "x", rc.x);
            config().getValue(itemPrefix + Config::pathSeparator + "y", rc.y);
            config().getValue(itemPrefix + Config::pathSeparator + "width", rc.width);
            config().getValue(itemPrefix + Config::pathSeparator + "height", rc.height);
            doReadConfigSettings(itemPrefix);
        }
    }

    // check whether rect intersects at least one monitor rect
    // otherwise (for example because monitor is not attached any more
    // or a remote desktop connection is active) use the default size and position
    for (unsigned i = 0; i < wxDisplay::GetCount(); ++i)
    {
        wxDisplay dsp(i);
        if (dsp.IsOk() && rc.Intersects(dsp.GetClientArea()))
        {
            SetSize(rc);
            if (maximized)
                Maximize();
            return;
        }
    }
    SetSize(rcDefault);
}
void BaseFrame::doReadConfigSettings(const wxString& WXUNUSED(prefix))
{
}

void BaseFrame::writeConfigSettings() const
{
    // wxFileConfig::Flush() should only be called once
    SubjectLocker locker(&config());

    // propagate call to children frames.
    const wxWindowList& children = GetChildren();
    for (wxWindowList::const_iterator it = children.begin();
        it != children.end(); ++it)
    {
        BaseFrame *f = dynamic_cast<BaseFrame *>(*it);
        if (f)
            f->writeConfigSettings();
    }

    if (config().get("FrameStorage", false) && !IsIconized())    // don't save for minimized windows
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
            config().setValue(itemPrefix + Config::pathSeparator + "maximized", IsMaximized());
#endif
            config().setValue(itemPrefix + Config::pathSeparator + "x", r.x);
            config().setValue(itemPrefix + Config::pathSeparator + "y", r.y);
            config().setValue(itemPrefix + Config::pathSeparator + "width", r.width);
            config().setValue(itemPrefix + Config::pathSeparator + "height", r.height);
            doWriteConfigSettings(itemPrefix);
        }
    }
}

void BaseFrame::doWriteConfigSettings(const wxString& WXUNUSED(prefix)) const
{
}

const wxString BaseFrame::getName() const
{
    // Couldn't find a reliable (meaning supportable and cross-platform) way
    // to use the class name here, so every derived frame needs to override getName()
    // if it needs to use features that depend on it.
    return "";
}

const wxString BaseFrame::getStorageName() const
{
    return getName();
}

const wxRect BaseFrame::getDefaultRect() const
{
    return wxRect(wxDefaultPosition, wxDefaultSize);
}

/* static */
void BaseFrame::setIdString(BaseFrame* frame, const wxString& id)
{
    FrameIdMap::iterator it = frameIdsM.find(frame);
    if (it != frameIdsM.end())
        (*it).second = id;
}

/* static */
BaseFrame* BaseFrame::frameFromIdString(const wxString& id)
{
    if (!id.empty())
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

/* static */
std::vector<BaseFrame*> BaseFrame::getFrames()
{
    std::vector<BaseFrame*> frames;
    FrameIdMap::iterator it;
    for (it = frameIdsM.begin(); it != frameIdsM.end(); it++)
        frames.push_back((*it).first);
    return frames;
}

// event handling
void BaseFrame::OnClose(wxCloseEvent& event)
{
    if (event.CanVeto() && !doCanClose())
    {
        event.Veto();
        return;
    }
    doBeforeDestroy();
    Destroy();
}

