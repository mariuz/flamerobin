/*
  Copyright (c) 2004-2008 The FlameRobin Development Team

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

//! Class that handles reading/writing of configuration options
//! and provides interface to all interested parties
//
#ifndef FR_CONFIG_H
#define FR_CONFIG_H

#include <wx/stdpaths.h>

#include <map>
#include <vector>

#include "core/Observer.h"
#include "core/Subject.h"
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

class Config: public Subject
{
private:
    mutable wxFileConfig* configM;
    bool needsFlushM;
    // performs lazy initialization of configM.
    wxFileConfig* getConfig() const;
#ifdef FR_CONFIG_USE_PRIVATE_STDPATHS
    wxStandardPaths standardPathsM;
#endif
    wxString getConfigFileName() const;
    wxString homePathM;
    wxString userHomePathM;
protected:
    virtual void lockedChanged(bool locked);
public:
    Config();
    virtual ~Config();

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
    // returns the path containing the images.
    wxString getImagesPath() const;

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
};
//-----------------------------------------------------------------------------
Config& config();
//-----------------------------------------------------------------------------
// class ConfigCache
// used to cache settings in a Config instance, observes the instance to
// reload the information when necessary (reloads on-demand)
class ConfigCache: public Observer
{
private:
    bool cacheValidM;
protected:
    void ensureCacheValid();
    virtual void loadFromConfig();
    virtual void update();
public:
    ConfigCache(Config& config);
};
//-----------------------------------------------------------------------------
#endif
