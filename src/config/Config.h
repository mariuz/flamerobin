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

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2004 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): Nando Dessena, Michael Hieke
*/

//! Class that handles reading/writing of configuration options
//! and provides interface to all interested parties
//
#ifndef FR_CONFIG_H
#define FR_CONFIG_H

#include <wx/stdpaths.h>

#include <map>
#include <vector>
//-----------------------------------------------------------------------------
enum StorageGranularity
{
    sgFrame,        // Store settings per frame type.
    sgObjectType,   // Store settings per object type.
    sgObject,       // Store settings per object.
};
//-----------------------------------------------------------------------------
// forward declarations
class wxFileConfig;
//-----------------------------------------------------------------------------
//! Do not instantiate objects of this class. Use config() function (see below).

#if defined(__UNIX__) && !defined(__WXMAC_OSX__)
    #define FR_CONFIG_USE_PRIVATE_STDPATHS
#else
    #undef FR_CONFIG_USE_PRIVATE_STDPATHS
#endif

class Config
{
public:
    // return true if value exists, false if not
    virtual bool keyExists(const wxString& key) const;
    virtual bool getValue(wxString key, wxString& value);
    bool getValue(wxString key, int& value);
    bool getValue(wxString key, double& value);
    bool getValue(wxString key, bool& value);
    bool getValue(wxString key, StorageGranularity& value);
    bool getValue(wxString key, std::vector<wxString>& value);

    // returns the value for key if it exists, or default value if it doesn't.
    template <typename T>
    T get(wxString key, const T& defaultValue)
    {
        T temp;
        if (getValue(key, temp))
            return temp;
        else
            return defaultValue;
    }
    
    // We must use an instance of wxStandardPaths for UNIX, but must not
    // use such an instance for things to work on Mac OS X.  Great...
    // These methods are to work around that, use them instead of 
    // wxStandardPaths methods.
    wxString getDataDir() const;
    wxString getLocalDataDir() const;
    wxString getUserLocalDataDir() const;

    // these should be called before calling the get* functions below,
    // otherwise defaults apply.
    void setHomePath(const wxString& homePath);
    void setUserHomePath(const wxString& userHomePath);

    // returns the home path to use as the basis for the following calls.
    wxString getHomePath() const;
    // returns the path from which to load the HTML templates.
    wxString getHtmlTemplatesPath() const;
    // returns the path containing the docs.
    wxString getDocsPath() const;
    // returns the path containing the confdefs.
    wxString getConfDefsPath() const;

    // returns the home path to use as the basis for the following call.
    wxString getUserHomePath() const;
    // returns the file name (with full path) of the file containing
    // registered databases.
    wxString getDBHFileName() const;

    // return true if value existed, false if not.
    virtual bool setValue(wxString key, wxString value);
    bool setValue(wxString key, int value);
    bool setValue(wxString key, double value);
    bool setValue(wxString key, bool value);
    bool setValue(wxString key, StorageGranularity value);
    bool setValue(wxString key, std::vector<wxString> value);

    static const wxString pathSeparator;

    Config();
    virtual ~Config();
private:
    mutable wxFileConfig* configM;
    // performs lazy initialization of configM.
    wxFileConfig* getConfig() const;
#ifdef FR_CONFIG_USE_PRIVATE_STDPATHS
    wxStandardPaths standardPathsM;
#endif
    wxString getConfigFileName() const;
    wxString homePathM;
    wxString userHomePathM;
};
//-----------------------------------------------------------------------------
Config& config();
//-----------------------------------------------------------------------------
#endif
