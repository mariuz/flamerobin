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

//
//
//
//
//------------------------------------------------------------------------------
#include <string>
#include <fstream>
#include <sstream>
#include "config.h"
#include "frutils.h"
//------------------------------------------------------------------------------
// This code should be wx and IBPP clean. Only std library
using namespace std;

//-----------------------------------------------------------------------------
YConfig& config()
{
	static YConfig c;
	return c;
}
//-----------------------------------------------------------------------------
//! return true if value exists, false if not
bool YConfig::getValue(string key, string& value)
{
	if (dataM.find(key) == dataM.end())
		return false;

	value = dataM[key];
	return true;
}
//-----------------------------------------------------------------------------
bool YConfig::getValue(string key, int& value)
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
bool YConfig::getValue(string key, double& value)
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
bool YConfig::getValue(string key, bool& value)
{
	string s;
	if (!getValue(key, s))
		return false;

	value = (s == "1");
	return true;
}
//-----------------------------------------------------------------------------
bool YConfig::getValue(std::string key, StorageGranularity& value)
{
	int intValue = 0;
	bool ret = getValue(key, intValue);
	if (ret)
		value = StorageGranularity(intValue);
	return ret;
}
//-----------------------------------------------------------------------------
bool YConfig::getValue(std::string key, std::vector<std::string>& value)
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
bool YConfig::setValue(string key, string value, bool saveIt)
{
	bool ret = (dataM.end() != dataM.find(key));
	if (ret)
		dataM.erase(key);
	dataM[key] = value;
	if (saveIt)
		save();
	return ret;
}
//-----------------------------------------------------------------------------
bool YConfig::setValue(string key, int value, bool saveIt)
{
	stringstream ss;
	ss << value;
	return setValue(key, ss.str(), saveIt);
}
//-----------------------------------------------------------------------------
bool YConfig::setValue(string key, double value, bool saveIt)
{
	stringstream ss;
	ss << value;
	return setValue(key, ss.str(), saveIt);
}
//-----------------------------------------------------------------------------
bool YConfig::setValue(string key, bool value, bool saveIt)
{
	if (value)
		return setValue(key, string("1"), saveIt);
	else
		return setValue(key, string("0"), saveIt);
}
//-----------------------------------------------------------------------------
bool YConfig::setValue(string key, StorageGranularity value, bool saveIt)
{
	return setValue(key, int(value), saveIt);
}
//-----------------------------------------------------------------------------
bool YConfig::setValue(string key, vector<string> value, bool saveIt)
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
    return setValue(key, s, saveIt);
}
//-----------------------------------------------------------------------------
YConfig::YConfig()
	: configFileNameM("")
{
	load();
}
//-----------------------------------------------------------------------------
YConfig::~YConfig()
{
	save();
}
//-----------------------------------------------------------------------------
std::string YConfig::getConfigFileName()
{
	if (configFileNameM.empty())
	{
		configFileNameM = getApplicationPath();
		if (configFileNameM.empty())
			configFileNameM = "config.ini";
		else
			configFileNameM += "/config.ini";
	}
	return configFileNameM;
}
//-----------------------------------------------------------------------------
bool YConfig::save()
{
	std::string path(getConfigFileName());

	std::ofstream file(path.c_str());
	if (!file)
		return false;

	file << "; FlameRobin configuration file." << endl << endl << "[Settings]" << endl;
	for (map<string, string>::const_iterator it = dataM.begin(); it != dataM.end(); ++it)
	{
		file << (*it).first << "=" << (*it).second << endl;
	}
	file.close();
	return true;
}
//-----------------------------------------------------------------------------
// this gets called from main() so we're sure config.ini is in the right place
bool YConfig::load()
{
	std::string path(getConfigFileName());

	std::ifstream file(path.c_str());
	if (!file)
		return false;

	// I had to do it this way, since standard line << file, doesn't work good if data has spaces in it.
	std::stringstream ss;		// read entire file into string buffer
	ss << file.rdbuf();
	std::string s(ss.str());

	dataM.clear();
	while (true)
	{
		string::size_type t = s.find('\n');
		if (t == string::npos)
			break;

		string line = s.substr(0, t);
		s.erase(0, t+1);

		string::size_type p = line.find('=');
		if (p == string::npos)
			continue;

		string key = line.substr(0, p);
		line.erase(0, p + 1);
		line.erase(line.find_last_not_of(" \t\n\r")+1);	// right trim

		setValue(key, line, false);
	}

	file.close();
	return true;
}
//-----------------------------------------------------------------------------
std::string YConfig::getHtmlTemplatesPath()
{
	std::string ret(getApplicationPath());
	if (ret.empty())
		ret = "html-templates/";
	else
		ret += "/html-templates/";
	return ret;
}
//-----------------------------------------------------------------------------
std::string YConfig::getDBHFileName()
{
	std::string ret(getApplicationPath());
	if (ret.empty())
		ret = "servers.xml";
	else
		ret += "/servers.xml";
	return ret;
}
//-----------------------------------------------------------------------------

