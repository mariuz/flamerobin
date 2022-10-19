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


#include "wx/fileconf.h"
#include "wx/filename.h"

#include "config/Config.h"
#ifdef HAVE_FRCONFIG_H
    #include "frconfig.h"
#endif
#include "core/FRError.h"
#include"gui/FRStyle.h"

const wxString Config::pathSeparator = "/";

FRConfig& config()
{
    static FRConfig c;    
    return c;
}

Config::Config()
    : configM(0), needsFlushM(false)
{
#ifdef FR_CONFIG_USE_PRIVATE_STDPATHS    
    ((wxStandardPaths&)wxStandardPaths::Get()).SetInstallPrefix(FR_INSTALL_PREFIX);
#endif
}

Config::~Config()
{
    delete configM;
}

wxFileConfig* Config::getConfig() const
{
    if (!configM)
    {
        wxFileName configFileName = getConfigFileName();
        if (!wxDirExists(configFileName.GetPath()))
            wxMkdir(configFileName.GetPath());
        configM = new wxFileConfig("", "",
            configFileName.GetFullPath(), "", wxCONFIG_USE_LOCAL_FILE);
        configM->SetExpandEnvVars(false);
    }
    return configM;
}

void Config::lockedChanged(bool locked)
{
    // delay getConfig()->Flush() until object is completely unlocked again
    if (!locked && needsFlushM)
    {
        needsFlushM = false;
        getConfig()->Flush();
    }
}

//! return true if value exists, false if not
bool Config::keyExists(const wxString& key) const
{
    return getConfig()->HasEntry(key);
}

//! return true if value exists, false if not
bool Config::getValue(const wxString& key, wxString& value)
{
    // if complete key is found, then return (recursion exit condition).
    wxString configValue;
    if (getConfig()->Read(key, &configValue))
    {
        value = configValue;
        return true;
    }
    // does key contain a separator? If not, then the key is not found and
    // we're done.
    wxString::size_type separatorPos = key.rfind(pathSeparator);
    if (separatorPos == wxString::npos)
        return false;
    else
    {
        // split key into keyPart and pathPart; remove last component of
        // pathPart and recurse.
        wxString keyPart = key.substr(separatorPos + 1, key.length());
        wxString pathPart = key.substr(0, separatorPos);
        wxString::size_type separatorPosInPath = pathPart.rfind(pathSeparator);
        if (separatorPosInPath == wxString::npos)
            return getValue(keyPart, value);
        else
        {
            return getValue(pathPart.substr(0, separatorPosInPath) +
                pathSeparator + keyPart, value);
        }
    }
}

bool Config::getValue(const wxString& key, int& value)
{
    wxString s;
    if (!getValue(key, s))
        return false;

    // This variable is only needed because the compiler considers
    // int* and long* incompatible. It may be ditched if a better solution
    // is found.
    long longValue;
    if (!s.ToLong(&longValue))
        return false;

    value = longValue;
    return true;
}

bool Config::getValue(const wxString& key, double& value)
{
    wxString s;
    if (!getValue(key, s))
        return false;

    if (!s.ToDouble(&value))
        return false;

    return true;
}

bool Config::getValue(const wxString& key, bool& value)
{
    wxString s;
    if (!getValue(key, s))
        return false;

    value = (s == "1");
    return true;
}

bool Config::getValue(const wxString& key, StorageGranularity& value)
{
    int intValue = 0;
    bool ret = getValue(key, intValue);
    if (ret)
        value = StorageGranularity(intValue);
    return ret;
}

bool Config::getValue(const wxString& key, wxArrayString& value)
{
    wxString s;
    if (!getValue(key, s))
        return false;

    value.clear();
    wxString item;
    size_t pos = 0, sep = s.find(',');
    while (sep != wxString::npos)
    {
        item = s.substr(pos, sep - pos);
        if (!item.empty())
            value.push_back(item);
        sep = s.find(',', pos = sep + 1);
    }
    if (!(item = s.substr(pos)).empty())
        value.push_back(item);
    return true;
}

//! return true if value existed, false if not
bool Config::setValue(const wxString& key, const wxString& value)
{
    bool result = getConfig()->Write(key, value);
    if (!isLocked())
        getConfig()->Flush();
    else
        needsFlushM = true;
    notifyObservers();
    return result;
}

bool Config::setValue(const wxString& key, int value)
{
    wxString s;
    s << value;
    return setValue(key, s);
}

bool Config::setValue(const wxString& key, double value)
{
    wxString s;
    s << value;
    return setValue(key, s);
}

bool Config::setValue(const wxString& key, bool value)
{
    if (value)
        return setValue(key, wxString("1"));
    else
        return setValue(key, wxString("0"));
}

bool Config::setValue(const wxString& key, StorageGranularity value)
{
    return setValue(key, int(value));
}

bool Config::setValue(const wxString& key, const wxArrayString& value)
{
    wxString s;
    for (wxArrayString::const_iterator it = value.begin(); it != value.end();
        it++)
    {
        if (it != value.begin())
            s += ",";
        // this is just a parachute, if this should ever be triggered we
        // will need to quote and unquote this wxString or all in value
        wxASSERT((*it).find(',') == wxString::npos);
        s += *it;
    }
    return setValue(key, s);
}

wxString Config::getHomePath() const
{
    if (!homePathM.empty())
        return homePathM + wxFileName::GetPathSeparator();
    else
        return getDataDir() + wxFileName::GetPathSeparator();
}

wxString Config::getUserHomePath() const
{
    if (!userHomePathM.empty())
        return userHomePathM + wxFileName::GetPathSeparator();
    else
        return getUserLocalDataDir() + wxFileName::GetPathSeparator();
}

wxFileName Config::getConfigFileName() const
{
    return configFileNameM;
}

void Config::setConfigFileName(const wxFileName& fileName)
{
    configFileNameM = fileName;
}

wxString Config::getDataDir() const
{
    return wxStandardPaths::Get().GetDataDir();
}

wxString Config::getLocalDataDir() const
{    
    return wxStandardPaths::Get().GetLocalDataDir();
}

wxString Config::getUserLocalDataDir() const
{
    return wxStandardPaths::Get().GetUserLocalDataDir();
}

void Config::setHomePath(const wxString& homePath)
{
    homePathM = homePath;
}

void Config::setUserHomePath(const wxString& userHomePath)
{
    userHomePathM = userHomePath;
}

// class ConfigCache
ConfigCache::ConfigCache(Config& config)
    : Observer(), cacheValidM(false)
{
    config.attachObserver(this, false);
}

void ConfigCache::ensureCacheValid()
{
    if (!cacheValidM)
    {
        loadFromConfig();
        cacheValidM = true;
    }
}

void ConfigCache::loadFromConfig()
{
}

void ConfigCache::update()
{
    // next call to ensureCacheValid() will reload the cached information
    cacheValidM = false;
}

wxString FRConfig::getHtmlTemplatesPath() const
{
    return getHomePath() + "html-templates"
        + wxFileName::GetPathSeparator();
}

wxString FRConfig::getCodeTemplatesPath() const
{
    return getHomePath() + "code-templates"
        + wxFileName::GetPathSeparator();
}

wxString FRConfig::getUserCodeTemplatesPath() const
{
    return getUserHomePath() + "code-templates"
        + wxFileName::GetPathSeparator();
}

wxString FRConfig::getSysTemplatesPath() const
{
    return getHomePath() + "sys-templates"
        + wxFileName::GetPathSeparator();
}

wxString FRConfig::getUserSysTemplatesPath() const
{
    return getUserHomePath() + "sys-templates"
        + wxFileName::GetPathSeparator();
}

wxString FRConfig::getDocsPath() const
{
    return getHomePath() + "docs" + wxFileName::GetPathSeparator();
}

wxString FRConfig::getConfDefsPath() const
{
    return getHomePath() + "conf-defs" + wxFileName::GetPathSeparator();
}

wxString FRConfig::getImagesPath() const
{
    return getHomePath() + "images" + wxFileName::GetPathSeparator();
}

wxString FRConfig::getDBHFileName() const
{
    return getUserHomePath() + "fr_databases.conf";
}

wxFileName FRConfig::getConfigFileName() const
{
    return wxFileName(getUserHomePath(), "fr_settings.conf");
}

const wxString FRConfig::getSysTemplateFileName(const wxString& templateName)
{
    wxFileName fileName = getUserSysTemplatesPath() + templateName
        + ".template";
    if (!fileName.FileExists())
        fileName = getSysTemplatesPath() + templateName + ".template";
    if (!fileName.FileExists())
        fileName = getUserCodeTemplatesPath() + templateName + ".template";
    if (!fileName.FileExists())
        fileName = getCodeTemplatesPath() + templateName + ".template";
    if (!fileName.FileExists())
    {
        fileName = getSysTemplatesPath() + templateName + ".template";
        if (!fileName.FileExists())
        {
            throw FRError(wxString::Format(_("Template \"%s\" not found."),
                fileName.GetFullPath().c_str()));
        }
    }
    return fileName.GetFullPath();
}

wxString FRConfig::getXmlStylesPath() const
{
    return getHomePath() + "xml-styles"
        + wxFileName::GetPathSeparator();
}

bool FRConfig::getUseLocalConfig() const
{
    bool b;
    if (!config().getValue("UseLocalConfig", b))
        b = config().get("UseLocalConfig", false);
    return b;
}

