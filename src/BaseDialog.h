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
are Copyright (C) 2004 Michael Hieke.

All Rights Reserved.

$Id$

Contributor(s): Nando Dessena
*/

#ifndef BASEDIALOG_H
#define BASEDIALOG_H

#include <list>
//-----------------------------------------------------------------------------
// Base class for dialogs in FlameRobin. Implements helper methods to ease
// dynamic layout.
class BaseDialog: public wxDialog 
{
public:
    BaseDialog(wxWindow* parent, int id, const wxString& title, 
        const wxPoint& pos = wxDefaultPosition,	const wxSize& size = wxDefaultSize, 
        long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    virtual bool Show(bool show = TRUE);
    virtual ~BaseDialog();
protected:
    void layoutSizers(wxSizer* controls, wxSizer* buttons, bool expandControls = false);

    // Reads any settings from config. The predefined implementation reads
    // the size of the dialog based on getStorageName(). No need to call
    // it directly except when wanting to "reload" the saved settings.
    void readConfigSettings();
    // Use this to customize which settings are read from config().
    virtual void doReadConfigSettings(const std::string& prefix);
    // Writes any settings to config. The predefined implementation saves
    // the size of the dialog based on getStorageName(). No need to call
    // it directly except when wanting to save settings without destroying the
    // dialog.
    void writeConfigSettings() const;
    // Use this to customize which settings are written to config().
    virtual void doWriteConfigSettings(const std::string& prefix) const;
    // Returns the name of the dialog for storage purpose.
    // A dialog that wants its settings stored and retrieved must override this
    // function and return a nonempty string. The predefined implementation
    // returns getName().
    virtual const std::string getStorageName() const;
    // Returns the name of the dialog, which can be the same for all instances
    // of the class or different for each instance. Currently it isn't really
    // used except as a base for getStorageName().
    // The predefined implementation returns "".
    virtual const std::string getName() const;
    // Returns the default position and size for the dialog; it's called by
    // readConfigSettings() to get first-time default position and size.
    // The predefined implementation returns -1 for all 4 items.
    virtual const wxRect getDefaultRect() const;
};
//-----------------------------------------------------------------------------
#endif // BASEDIALOG_H
