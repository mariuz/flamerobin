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

//-----------------------------------------------------------------------------
#include "wx/fileconf.h"
#include "wx/filename.h"

#include <fstream>
#include <sstream>

#include "Config.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
const wxString Config::pathSeparator = wxT("/");
//-----------------------------------------------------------------------------
Config& config()
{
    static Config c;
    return c;
}
//-----------------------------------------------------------------------------
Config::Config()
    : homePathM(wxT("")), userHomePathM(wxT("")), configM(0)
{
#ifdef FR_CONFIG_USE_PRIVATE_STDPATHS
    standardPathsM.SetInstallPrefix(wxT("/usr/local"));
#endif
}
//-----------------------------------------------------------------------------
Config::~Config()
{
    delete configM;
}
//-----------------------------------------------------------------------------
wxFileConfig* Config::getConfig() const
{
    if (!configM)
    {
        wxFileName configFileName = getConfigFileName();
        if (!wxDirExists(configFileName.GetPath()))
            wxMkdir(configFileName.GetPath());
        configM = new wxFileConfig(wxT(""), wxT(""),
            configFileName.GetFullPath(), wxT(""), wxCONFIG_USE_LOCAL_FILE);
    }
    return configM;
}
//-----------------------------------------------------------------------------
//! return true if value exists, false if not
bool Config::keyExists(const wxString& key) const
{
    return getConfig()->HasEntry(key);
}
//-----------------------------------------------------------------------------
//! return true if value exists, false if not
bool Config::getValue(wxString key, wxString& value)
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
            return getValue(pathPart.substr(0, separatorPosInPath) +
                pathSeparator + keyPart, value);
    }
}
//-----------------------------------------------------------------------------
bool Config::getValue(wxString key, int& value)
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
//-----------------------------------------------------------------------------
bool Config::getValue(wxString key, double& value)
{
    wxString s;
    if (!getValue(key, s))
        return false;

    if (!s.ToDouble(&value))
        return false;

    return true;
}
//-----------------------------------------------------------------------------
bool Config::getValue(wxString key, bool& value)
{
    wxString s;
    if (!getValue(key, s))
        return false;

    value = (s == wxT("1"));
    return true;
}
//-----------------------------------------------------------------------------
bool Config::getValue(wxString key, StorageGranularity& value)
{
    int intValue = 0;
    bool ret = getValue(key, intValue);
    if (ret)
        value = StorageGranularity(intValue);
    return ret;
}
//-----------------------------------------------------------------------------
bool Config::getValue(wxString key, vector<wxString>& value)
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
//-----------------------------------------------------------------------------
//! return true if value existed, false if not
bool Config::setValue(wxString key, wxString value)
{
    bool result = getConfig()->Write(key, value);
    getConfig()->Flush();
    return result;
}
//-----------------------------------------------------------------------------
bool Config::setValue(wxString key, int value)
{
    wxString s;
    s << value;
    return setValue(key, s);
}
//-----------------------------------------------------------------------------
bool Config::setValue(wxString key, double value)
{
    wxString s;
    s << value;
    return setValue(key, s);
}
//-----------------------------------------------------------------------------
bool Config::setValue(wxString key, bool value)
{
    if (value)
        return setValue(key, wxString(wxT("1")));
    else
        return setValue(key, wxString(wxT("0")));
}
//-----------------------------------------------------------------------------
bool Config::setValue(wxString key, StorageGranularity value)
{
    return setValue(key, int(value));
}
//-----------------------------------------------------------------------------
bool Config::setValue(wxString key, vector<wxString> value)
{
    wxString s;
    for (vector<wxString>::iterator it = value.begin(); it != value.end(); it++)
    {
        if (it != value.begin())
            s += wxT(",");
        // this is just a parachute, if this should ever be triggered we
        // will need to quote and unquote this wxString or all in value
        wxASSERT((*it).find(',') == wxString::npos);
        s += *it;
    }
    return setValue(key, s);
}
//-----------------------------------------------------------------------------
wxString Config::getHomePath() const
{
    if (!homePathM.empty())
        return homePathM + wxT("/");
    else
        return getDataDir() + wxT("/");
}
//-----------------------------------------------------------------------------
wxString Config::getHtmlTemplatesPath() const
{
    return getHomePath() + wxT("html-templates/");
}
//-----------------------------------------------------------------------------
wxString Config::getDocsPath() const
{
    return getHomePath() + wxT("docs/");
}
//-----------------------------------------------------------------------------
wxString Config::getConfDefsPath() const
{
    return getHomePath() + wxT("confdefs/");
}
//-----------------------------------------------------------------------------
wxString Config::getUserHomePath() const
{
    if (!userHomePathM.empty())
        return userHomePathM + wxT("/");
    else
        return getUserLocalDataDir() + wxT("/");
}
//-----------------------------------------------------------------------------
wxString Config::getDBHFileName() const
{
    return getUserHomePath() + wxT("fr_databases.conf");
}
//-----------------------------------------------------------------------------
wxString Config::getConfigFileName() const
{
    return getUserHomePath() + wxT("fr_settings.conf");
}
//-----------------------------------------------------------------------------
wxString Config::getDataDir() const
{
#ifdef FR_CONFIG_USE_PRIVATE_STDPATHS
    return standardPathsM.GetDataDir();
#else
    return wxStandardPaths::Get().GetDataDir();
#endif
}
//-----------------------------------------------------------------------------
wxString Config::getLocalDataDir() const
{
#ifdef FR_CONFIG_USE_PRIVATE_STDPATHS
    return standardPathsM.GetLocalDataDir();
#else
    return wxStandardPaths::Get().GetLocalDataDir();
#endif
}
//-----------------------------------------------------------------------------
wxString Config::getUserLocalDataDir() const
{
#ifdef FR_CONFIG_USE_PRIVATE_STDPATHS
    return standardPathsM.GetUserLocalDataDir();
#else
    return wxStandardPaths::Get().GetUserLocalDataDir();
#endif
}
//-----------------------------------------------------------------------------
void Config::setHomePath(const wxString& homePath)
{
    homePathM = homePath;
}
//-----------------------------------------------------------------------------
void Config::setUserHomePath(const wxString& userHomePath)
{
    userHomePathM = userHomePath;
}
//-----------------------------------------------------------------------------
