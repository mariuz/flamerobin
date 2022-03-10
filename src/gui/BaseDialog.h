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

#ifndef BASEDIALOG_H
#define BASEDIALOG_H

#include <list>

// Base class for dialogs in FlameRobin. Implements helper methods to ease
// dynamic layout.
class BaseDialog: public wxDialog
{
public:
    BaseDialog(wxWindow* parent, int id, const wxString& title,
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    virtual bool Show(bool show = TRUE);
    virtual ~BaseDialog();
protected:
    void layoutSizers(wxSizer* controls, wxSizer* buttons, bool expandControls = false);
    wxPanel* getControlsPanel();

    // Update the colors of textboxes
    void updateColors(wxWindow *parent = 0);

    // Reads any settings from config. The predefined implementation reads
    // the size of the dialog based on getStorageName(). No need to call
    // it directly except when wanting to "reload" the saved settings.
    void readConfigSettings();
    // Use this to customize which settings are read from config().
    virtual void doReadConfigSettings(const wxString& prefix);
    // Writes any settings to config. The predefined implementation saves
    // the size of the dialog based on getStorageName(). No need to call
    // it directly except when wanting to save settings without destroying the
    // dialog.
    void writeConfigSettings() const;
    // Use this to customize which settings are written to config().
    virtual void doWriteConfigSettings(const wxString& prefix) const;
    // Returns the name of the dialog for storage purpose.
    // A dialog that wants its settings stored and retrieved must override this
    // function and return a nonempty wxString. The predefined implementation
    // returns getName().
    virtual const wxString getStorageName() const;
    // Returns the name of the dialog, which can be the same for all instances
    // of the class or different for each instance. Currently it isn't really
    // used except as a base for getStorageName().
    // The predefined implementation returns "".
    virtual const wxString getName() const;
    // Returns the default position and size for the dialog; it's called by
    // readConfigSettings() to get first-time default position and size.
    // The predefined implementation returns -1 for all 4 items.
    virtual const wxRect getDefaultRect() const;
    // Returns true in default implementation, but allows for descendent
    // classes to always come up in minimal width or height - just return
    // false in overridden methods.
    virtual bool getConfigStoresWidth() const;
    virtual bool getConfigStoresHeight() const;
private:
    wxPanel* panel_controls;
};

#endif // BASEDIALOG_H
