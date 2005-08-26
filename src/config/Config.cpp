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

//------------------------------------------------------------------------------
#include "wx/fileconf.h"
#include "wx/stdpaths.h"

#include <string>
#include <fstream>
#include <sstream>

#include "config.h"
#include "frutils.h"
#include "ugly.h"
//------------------------------------------------------------------------------
using namespace std;
//-----------------------------------------------------------------------------
const string Config::pathSeparator = "/";
//------------------------------------------------------------------------------
Config& config()
{
    static Config c;
    return c;
}
//-----------------------------------------------------------------------------
Config::Config()
    : homePathM(""), userHomePathM(""), configM(0)
{
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
        configM = new wxFileConfig(wxT(""), wxT(""), 
            std2wx(getConfigFileName()));
    }
    return configM;
}
//-----------------------------------------------------------------------------
//! return true if value exists, false if not
bool Config::keyExists(const string& key) const
{
    return getConfig()->HasEntry(std2wx(key));
}
//-----------------------------------------------------------------------------
//! return true if value exists, false if not
bool Config::getValue(string key, string& value)
{
    // if complete key is found, then return (recursion exit condition).
    wxString configValue;
    if (getConfig()->Read(std2wx(key), &configValue))
    {
        value = wx2std(configValue);
        return true;
    }
    // does key contain a separator? If not, then the key is not found and
    // we're done.
    string::size_type separatorPos = key.rfind(pathSeparator);
    if (separatorPos == string::npos)
        return false;
    else
    {
        // split key into keyPart and pathPart; remove last component of
        // pathPart and recurse.
        string keyPart = key.substr(separatorPos + 1, key.length());
        string pathPart = key.substr(0, separatorPos);
        string::size_type separatorPosInPath = pathPart.rfind(pathSeparator);
        if (separatorPosInPath == string::npos)
            return getValue(keyPart, value);
        else
            return getValue(pathPart.substr(0, separatorPosInPath) +
                pathSeparator + keyPart, value);
    }
}
//-----------------------------------------------------------------------------
bool Config::getValue(string key, int& value)
{
    string s;
    if (!getValue(key, s))
        return false;

    stringstream ss;
    ss << s;
    ss >> value;
    return true;
}
//-----------------------------------------------------------------------------
bool Config::getValue(string key, double& value)
{
    string s;
    if (!getValue(key, s))
        return false;

    stringstream ss;
    ss << s;
    ss >> value;
    return true;
}
//-----------------------------------------------------------------------------
bool Config::getValue(string key, bool& value)
{
    string s;
    if (!getValue(key, s))
        return false;

    value = (s == "1");
    return true;
}
//-----------------------------------------------------------------------------
bool Config::getValue(string key, StorageGranularity& value)
{
    int intValue = 0;
    bool ret = getValue(key, intValue);
    if (ret)
        value = StorageGranularity(intValue);
    return ret;
}
//-----------------------------------------------------------------------------
bool Config::getValue(string key, vector<string>& value)
{
    string s;
    if (!getValue(key, s))
        return false;

    value.clear();
    string item;
    size_t pos = 0, sep = s.find(',');
    while (sep != string::npos)
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
bool Config::setValue(string key, string value)
{
    bool result = getConfig()->Write(std2wx(key), std2wx(value));
    getConfig()->Flush();
    return result;
}
//-----------------------------------------------------------------------------
bool Config::setValue(string key, int value)
{
    stringstream ss;
    ss << value;
    return setValue(key, ss.str());
}
//-----------------------------------------------------------------------------
bool Config::setValue(string key, double value)
{
    stringstream ss;
    ss << value;
    return setValue(key, ss.str());
}
//-----------------------------------------------------------------------------
bool Config::setValue(string key, bool value)
{
    if (value)
        return setValue(key, string("1"));
    else
        return setValue(key, string("0"));
}
//-----------------------------------------------------------------------------
bool Config::setValue(string key, StorageGranularity value)
{
    return setValue(key, int(value));
}
//-----------------------------------------------------------------------------
bool Config::setValue(string key, vector<string> value)
{
    string s;
    for (vector<string>::iterator it = value.begin(); it != value.end(); it++)
    {
        if (it != value.begin())
            s += ",";
        // this is just a parachute, if this should ever be triggered we
        // will need to quote and unquote this string or all in value
        wxASSERT((*it).find(',') == string::npos);
        s += *it;
    }
    return setValue(key, s);
}
//-----------------------------------------------------------------------------
string Config::getHomePath() const
{
    if (!homePathM.empty())
        return homePathM + "/";
    else
        return getApplicationPath() + "/";
}
//-----------------------------------------------------------------------------
string Config::getHtmlTemplatesPath() const
{
    return getHomePath() + "html-templates/";
}
//-----------------------------------------------------------------------------
string Config::getDocsPath() const
{
    return getHomePath() + "docs/";
}
//-----------------------------------------------------------------------------
string Config::getConfDefsPath() const
{
    return getHomePath() + "confdefs/";
}
//-----------------------------------------------------------------------------
string Config::getUserHomePath() const
{
    if (!userHomePathM.empty())
        return userHomePathM + "/";
    else
        return wx2std(wxStandardPaths::Get().GetUserLocalDataDir()) + "/";
}
//-----------------------------------------------------------------------------
string Config::getDBHFileName() const
{
    return getUserHomePath() + "fr_databases.conf";
}
//-----------------------------------------------------------------------------
string Config::getConfigFileName() const
{
    return getUserHomePath() + "fr_settings.conf";
}
//-----------------------------------------------------------------------------
void Config::setHomePath(const string& homePath)
{
    homePathM = homePath;
}
//-----------------------------------------------------------------------------
void Config::setUserHomePath(const string& userHomePath)
{
    userHomePathM = userHomePath;
}
//-----------------------------------------------------------------------------
