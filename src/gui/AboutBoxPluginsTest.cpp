/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

// Regression test for GitHub issue #722:
// "Fatal error on help -> about"
//
// Once a database had been opened the About box enumerated the Firebird
// plugins by asking the plugin manager for every plugin type with a null
// names list.  fbclient takes that list by value and calls strlen() on it,
// so the null pointer crashed the whole application.  The enumeration must
// read the configured list for each plugin type first.

#include <iostream>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <firebird/Interface.h>

#include "gui/AboutBoxPlugins.h"

namespace
{

static bool check(bool condition, const char* testName)
{
    std::cout << (condition ? "  PASSED: " : "  FAILED: ") << testName << "\n";
    return condition;
}

// The plugin types the About box enumerates, with the firebird.conf key that
// holds the list of plugin names for each of them.  Kept in sync with
// AboutBoxPlugins.cpp - a type without a key would make the query fall back to
// a null names list again.
struct TypeAndKey
{
    unsigned type;
    const char* configKey;
};

const TypeAndKey typesAndKeys[] = {
    { Firebird::IPluginManager::TYPE_PROVIDER,             "Providers"       },
    { Firebird::IPluginManager::TYPE_AUTH_CLIENT,          "AuthClient"      },
    { Firebird::IPluginManager::TYPE_AUTH_SERVER,          "AuthServer"      },
    { Firebird::IPluginManager::TYPE_AUTH_USER_MANAGEMENT, "UserManager"     },
    { Firebird::IPluginManager::TYPE_TRACE,                "TracePlugin"     },
    { Firebird::IPluginManager::TYPE_WIRE_CRYPT,           "WireCryptPlugin" },
    { Firebird::IPluginManager::TYPE_KEY_HOLDER,           "KeyHolderPlugin" }
};

} // namespace

int main()
{
    std::cout << "Running About box plugin enumeration tests (issue #722)...\n";
    bool ok = true;

    Firebird::IMaster* master = Firebird::fb_get_master_interface();
    if (!master)
    {
        std::cout << "  SKIPPED: no Firebird client library available\n";
        return 0;
    }

    // Test 1: every plugin type the About box asks about resolves to a real
    // names list, so that a null pointer never reaches fbclient.
    {
        Firebird::IConfigManager* cm = master->getConfigManager();
        Firebird::IFirebirdConf* conf = cm ? cm->getFirebirdConf() : nullptr;
        if (!conf)
        {
            std::cout << "  SKIPPED: Firebird configuration not available\n";
            return 0;
        }

        for (const TypeAndKey& tk : typesAndKeys)
        {
            const char* names = conf->asString(conf->getKey(tk.configKey));
            ok = check(names != nullptr, tk.configKey) && ok;
        }
        conf->release();
    }

    // Test 2: the enumeration itself must complete without crashing, and the
    // provider list configured by every Firebird installation must show up.
    {
        wxString plugins = getFirebirdLoadedPlugins();
        std::cout << "  plugins: " << plugins.ToStdString() << "\n";
        ok = check(plugins.Contains("Remote") || plugins.Contains("Loopback"),
            "Enumeration returns the configured providers") && ok;
    }

    std::cout << "\nAll About box plugin tests " << (ok ? "PASSED" : "FAILED") << ".\n";
    return ok ? 0 : 1;
}
