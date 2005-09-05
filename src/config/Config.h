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

#include <map>
#include <string>
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

class Config
{
public:
    // return true if value exists, false if not
    virtual bool keyExists(const std::string& key) const;
    virtual bool getValue(std::string key, std::string& value);
    bool getValue(std::string key, int& value);
    bool getValue(std::string key, double& value);
    bool getValue(std::string key, bool& value);
    bool getValue(std::string key, StorageGranularity& value);
    bool getValue(std::string key, std::vector<std::string>& value);

    // returns the value for key if it exists, or default value if it doesn't.
    template <typename T>
    T get(std::string key, const T& defaultValue)
    {
        T temp;
        if (getValue(key, temp))
            return temp;
        else
            return defaultValue;
    }

    // these should be called before calling the get* functions below,
    // otherwise defaults apply.
    void setHomePath(const std::string& homePath);
    void setUserHomePath(const std::string& userHomePath);

    // returns the home path to use as the basis for the following calls.
    std::string getHomePath() const;
    // returns the path from which to load the HTML templates.
    std::string getHtmlTemplatesPath() const;
    // returns the path containing the docs.
    std::string getDocsPath() const;
    // returns the path containing the confdefs.
    std::string getConfDefsPath() const;

    // returns the home path to use as the basis for the following call.
    std::string getUserHomePath() const;
    // returns the file name (with full path) of the file containing
    // registered databases.
    std::string getDBHFileName() const;

    // return true if value existed, false if not.
    virtual bool setValue(std::string key, std::string value);
    bool setValue(std::string key, int value);
    bool setValue(std::string key, double value);
    bool setValue(std::string key, bool value);
    bool setValue(std::string key, StorageGranularity value);
    bool setValue(std::string key, std::vector<std::string> value);

    static const std::string pathSeparator;

    Config();
    virtual ~Config();
private:
    mutable wxFileConfig* configM;
    // performs lazy initialization of configM.
    wxFileConfig* getConfig() const;
    std::string getConfigFileName() const;
    std::string homePathM;
    std::string userHomePathM;
};
//-----------------------------------------------------------------------------
Config& config();
//-----------------------------------------------------------------------------
#endif
