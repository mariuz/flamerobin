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


#ifndef BASEFRAME_H
#define BASEFRAME_H

#include <wx/wx.h>

#include <map>

// Base class for all the frames in FlameRobin. Implements storing and restoring
// of settings in config and other commonalities.
class BaseFrame: public wxFrame {
private:
    typedef std::map<BaseFrame*, wxString> FrameIdMap;
    typedef FrameIdMap::value_type FrameIdPair;
    static FrameIdMap frameIdsM;

    // Override to implement checks and show confirmation dialogs to prevent
    // closing of the frame if necessary.
    virtual bool doCanClose();
    // Override to execute code immediately before frame is destroyed.
    virtual void doBeforeDestroy();
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
    BaseFrame(wxWindow* parent, int id, const wxString& title,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_FRAME_STYLE,
        const wxString& name = "FlameRobin");
    virtual ~BaseFrame();
    virtual bool Show(bool show = TRUE);
    virtual bool Destroy();

    // Returns whether the frame can be closed, potentially showing
    // confirmation dialogs to the user
    // Override doCanClose() in descendent classes to implement this
    bool canClose();

    static std::vector<BaseFrame*> getFrames();

private:
    // event handling
    void OnClose(wxCloseEvent& event);
};

#endif // BASEFRAME_H
