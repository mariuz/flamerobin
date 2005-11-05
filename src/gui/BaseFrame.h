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

  Contributor(s):
*/

//-----------------------------------------------------------------------------
#ifndef BASEFRAME_H
#define BASEFRAME_H

#include <wx/wx.h>

#include <map>
//-----------------------------------------------------------------------------
// Base class for all the frames in FlameRobin. Implements storing and restoring
// of settings in config and other commonalities.
class BaseFrame: public wxFrame {
private:
    typedef std::map<BaseFrame*, wxString> FrameIdMap;
    typedef FrameIdMap::value_type FrameIdPair;
    static FrameIdMap frameIdsM;
protected:
    // Reads any settings from config. The predefined implementation reads
    // size and position of the frame based on getStorageName(). No need to call
    // it directly except when wanting to "reload" the saved settings.
    void readConfigSettings();
    // Use this to customize which settings are read from config().
    virtual void doReadConfigSettings(const wxString& prefix);
    // Writes any settings to config. The predefined implementation saves
    // size and position of the frame based on getStorageName(). No need to call
    // it directly except when wanting to save settings without destroying the
    // frame.
    void writeConfigSettings() const;
    // Use this to customize which settings are written to config().
    virtual void doWriteConfigSettings(const wxString& prefix) const;
    // Returns the name of the frame for storage purpose.
    // A frame that wants its settings stored and retrieved must override this
    // function and return a nonempty wxString. The predefined implementation
    // returns getName().
    virtual const wxString getStorageName() const;
    // Returns the name of the frame, which can be the same for all instances
    // of the class or different for each instance. Currently it isn't really
    // used except as a base for getStorageName().
    // The predefined implementation returns "".
    virtual const wxString getName() const;
    // Returns the default position and size for the frame; it's called by
    // readConfigSettings() to get first-time default position and size.
    // The predefined implementation returns -1 for all 4 items.
    virtual const wxRect getDefaultRect() const;
    // Maintains the connection between the frame object and its id string.
    // The id string may be constant over the lifetime of the frame 
    // (e.g. "BackupFrame/database_42"), or it may change with the content
    // of the frame (e.g. different property pages in the same MIPFrame may 
    // have different id strings).
    static void setIdString(BaseFrame* frame, const wxString& id);
    // Returns the first frame with a given id string. Note that more than 
    // one frame can exist for a given id string.
    static BaseFrame* frameFromIdString(const wxString& id);
public:
    BaseFrame(wxWindow* parent, int id, const wxString& title, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE, const wxString& name = wxT("FlameRobin"));
    virtual ~BaseFrame();
    virtual bool Show(bool show = TRUE);
    virtual bool Destroy();
protected:
    // event handling
    void OnClose(wxCloseEvent& event);

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // BASEFRAME_H
