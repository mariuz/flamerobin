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

//! Class that handles reading/writing of configuration options
//! and provides interface to all interested parties
//
#ifndef FR_CONFIG_H
#define FR_CONFIG_H

#include <map>
#include <vector>

enum StorageGranularity
{
	sgFrame,		// Store settings per frame type.
	sgObjectType,	// Store settings per object type.
	sgObject,		// Store settings per object.
};

//---------------------------------------------------------------------------------------
//! Do not instantiate objects of this class. Use config() function (see below).
class YConfig
{
public:
	bool save();
	bool load();

	// return true if value exists, false if not
	bool getValue(std::string key, std::string& value);
	bool getValue(std::string key, int& value);
	bool getValue(std::string key, double& value);
	bool getValue(std::string key, bool& value);
    bool getValue(std::string key, StorageGranularity& value);
    bool getValue(std::string key, std::vector<std::string>& value);

	// returns the path from which to load the HTML templates.
	std::string getHtmlTemplatesPath();
	// returns the file name with full path of servers.xml.
	std::string getDBHFileName();

	// return true if value existed, false if not. Pass false in saveIt
	// to prevent saving the config file before setValue() returns (f. ex.
	// when a number of items are being stored at one time).
	bool setValue(std::string key, std::string value, bool saveIt = true);
	bool setValue(std::string key, int value, bool saveIt = true);
	bool setValue(std::string key, double value, bool saveIt = true);
	bool setValue(std::string key, bool value, bool saveIt = true);
    bool setValue(std::string key, StorageGranularity value, bool saveIt = true);
    bool setValue(std::string key, std::vector<std::string> value, bool saveIt = true);

	YConfig();
	~YConfig();
private:
	std::map<std::string, std::string> dataM;		//! key -> value
	std::string getConfigFileName();
	std::string configFileNameM;
};

YConfig& config();
//---------------------------------------------------------------------------------------
#endif
