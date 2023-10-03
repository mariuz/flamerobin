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

#ifndef FR_CONFIG_H
#define FR_CONFIG_H

#include "wx/arrstr.h"
#include <wx/stdpaths.h>
#include <wx/filename.h>

#include "core/Observer.h"
#include "core/Subject.h"
#include "gui/FRStyle.h"

enum StorageGranularity
{
    sgFrame,        // Store settings per frame type.
    sgObjectType,   // Store settings per object type.
    sgObject,       // Store settings per object.
};

// forward declarations
class wxFileConfig;

//! Do not instantiate objects of this class. Use config() function (see below).

#if defined(__UNIX__) && !defined(__WXOSX_COCOA__)
    #define FR_CONFIG_USE_PRIVATE_STDPATHS
#else
    #undef FR_CONFIG_USE_PRIVATE_STDPATHS
#endif

//! Base class to be used as generic container of configuration info.
class Config: public Subject
{
private:
    mutable wxFileConfig* configM;
    bool needsFlushM;
    // performs lazy initialization of configM.
    wxFileConfig* getConfig() const;
    wxString homePathM;
    wxString userHomePathM;
    wxFileName configFileNameM;

protected:
    virtual void lockedChanged(bool locked);
public:
    Config();
    virtual ~Config();

    virtual wxFileName getConfigFileName() const;
    void setConfigFileName(const wxFileName& fileName);

    static const wxString pathSeparator;
    
    // We must use an instance of wxStandardPaths for UNIX, but must not
    // use such an instance for things to work on Mac OS X.  Great...
    // These methods are to work around that, use them instead of
    // wxStandardPaths methods.
    wxString getDataDir() const;
    wxString getLocalDataDir() const;
    wxString getUserLocalDataDir() const;

    // returns the home path to use as the basis for the following calls.
    wxString getHomePath() const;
    // returns the home path to use as the basis for the following call.
    wxString getUserHomePath() const;

    // these should be called before calling the get* functions below,
    // otherwise defaults apply.
    void setHomePath(const wxString& homePath);
    void setUserHomePath(const wxString& userHomePath);

    // return true if value exists, false if not
    virtual bool keyExists(const wxString& key) const;
    virtual bool getValue(const wxString& key, wxString& value);
    bool getValue(const wxString& key, int& value);
    bool getValue(const wxString& key, double& value);
    bool getValue(const wxString& key, bool& value);
    bool getValue(const wxString& key, StorageGranularity& value);
    bool getValue(const wxString& key, wxArrayString& value);

    // returns the value for key if it exists, or default value if it doesn't.
    template <typename T>
    T get(const wxString& key, const T& defaultValue)
    {
        T temp;
        if (getValue(key, temp))
            return temp;
        else
            return defaultValue;
    }

    // return true if value existed, false if not.
    virtual bool setValue(const wxString& key, const wxString& value);
    bool setValue(const wxString& key, int value);
    bool setValue(const wxString& key, double value);
    bool setValue(const wxString& key, bool value);
    bool setValue(const wxString& key, StorageGranularity value);
    bool setValue(const wxString& key, const wxArrayString& value);
};

//! Class used to contain all FlameRobin and database configuration info sets.
class FRConfig: public Config
{
public:
    // this class has a fixed file name - setting it through
    // setConfigFileName() is ineffective.
    virtual wxFileName getConfigFileName() const;
    // returns the path from which to load HTML templates.
    wxString getHtmlTemplatesPath() const;
    // returns the path from which to load code templates.
    wxString getCodeTemplatesPath() const;
    // returns the path from which to load user code templates and overrides.
    wxString getUserCodeTemplatesPath() const;
    // returns the path from which to load system templates.
    wxString getSysTemplatesPath() const;
    // returns the path from which to load user overrides of system templates.
    wxString getUserSysTemplatesPath() const;
    // returns the path containing the docs.
    wxString getDocsPath() const;
    // returns the path containing the confdefs.
    wxString getConfDefsPath() const;
    // returns the path containing the images.
    wxString getImagesPath() const;
    // returns the file name (with full path) of the file containing
    // registered databases.
    wxString getDBHFileName() const;
    // Returns the full pathname of the specified system template, giving
    // precedence to any existing user override.
    const wxString getSysTemplateFileName(const wxString& templateName);
    // returns the path containing the xml styles.
    wxString getXmlStylesPath() const;

    bool getUseLocalConfig() const;
};

FRConfig& config();

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

#endif
