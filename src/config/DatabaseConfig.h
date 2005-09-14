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
  are Copyright (C) 2005 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/

//! This class is interface between Config and Database classes
//  Preferences dialog takes Config class as parameter, so interface like this
//  is needed
//
#ifndef FR_DATABASE_CONFIG_H
#define FR_DATABASE_CONFIG_H

#include <map>
#include <vector>

#include "Config.h"

class Database;
//-----------------------------------------------------------------------------
class DatabaseConfig: public Config
{
private:
    Database *databaseM;
    wxString addPathToKey(const wxString key) const;

public:
    DatabaseConfig(Database *d);

    // unhides methods of base class, for details see:
    // http://www.parashift.com/c++-faq-lite/strange-inheritance.html#faq-23.7
    using Config::getValue;
    using Config::setValue;

    // transform the key based on Database, and call regular config
    virtual bool keyExists(const wxString& key) const;
    virtual bool getValue(wxString key, wxString& value);
    virtual bool setValue(wxString key, wxString value);
};
//-----------------------------------------------------------------------------
#endif
